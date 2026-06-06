#include "network_manager.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <algorithm>

NetworkManager::NetworkManager()
    : mode(Mode::NONE), socket(nullptr), discovery_socket(nullptr),
      gameStarted(false), localPlayerId(-1), serverName("LAN SERVER"),
      shouldQuit(false), localPlayerAdmin(false), localPlayerName("Gooner"),
      lobbyPort(1240), nextClientId(1),
      lastLobbyHeartbeat(std::chrono::steady_clock::now()),
      lastLobbyQuery(std::chrono::steady_clock::now()),
      lastConnectRequest(std::chrono::steady_clock::now()),
      lastRelayJoinRequest(std::chrono::steady_clock::now()),
      lobby_server_available(false) {
    remoteBaseHP = 0;
    remoteWave = 0;
    ConfigureLobbyServer();
}

NetworkManager::~NetworkManager() {
    if (useRelay && relayRoomId != 0 && socket) {
        SendRelayLeave();
    }
    if (socket) {
        socket->close();
        delete socket;
    }
    if (discovery_socket) {
        discovery_socket->close();
        delete discovery_socket;
    }
}

bool NetworkManager::StartServer(int port, const std::string& name) {
    if (socket) {
        socket->close();
        delete socket;
        socket = nullptr;
    }
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
        socket->set_option(asio::socket_base::broadcast(true));
        socket->non_blocking(true);
        mode = Mode::SERVER;
        localPlayerId = 0;
        localPlayerAdmin = true;
        serverName = name;
        activePort = port;
        waitingForServerAccept = false;
        useRelay = false;
        relayRoomId = 0;
        relaySlotId = 0;
        nextClientId = 1;
        known_clients.clear();
        remotePlayers.clear();
        std::cout << "ASIO Server started on port " << port << " with name: " << name << std::endl;
        RegisterInternetLobby();
        return true;
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::StartServerAutoPort(int startPort, int maxTries, const std::string& name) {
    for (int i = 0; i < maxTries; i++) {
        int port = startPort + i;
        if (StartServer(port, name))
            return true;
        // Reset socket if failed
        if (socket) {
            delete socket;
            socket = nullptr;
        }
    }
    return false;
}

bool NetworkManager::StartClient(const std::string& ip, int port) {
    if (socket) {
        socket->close();
        delete socket;
        socket = nullptr;
    }
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
        socket->non_blocking(true);
        
        asio::ip::udp::resolver resolver(io_context);
        server_target_endpoint = *resolver.resolve(asio::ip::udp::v4(), ip, std::to_string(port)).begin();
        
        mode = Mode::CLIENT;
        localPlayerId = -1;
        localPlayerAdmin = false;
        waitingForServerAccept = true;
        useRelay = false;
        relayRoomId = 0;
        relaySlotId = 0;
        remotePlayers.clear();
        syncedEnemies.clear();
        std::cout << "ASIO Client started, targeting " << ip << ":" << port << std::endl;
        SendConnectRequest();
        return true;
    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================
// RELAY MODE
// ============================================================

bool NetworkManager::StartServerRelay(const std::string& name) {
    if (!lobby_server_available) {
        std::cerr << "Relay: lobby server not configured!" << std::endl;
        return false;
    }
    if (socket) {
        socket->close();
        delete socket;
        socket = nullptr;
    }
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
        socket->non_blocking(true);
        
        mode = Mode::SERVER;
        localPlayerId = 0;
        localPlayerAdmin = true;
        serverName = name;
        activePort = socket->local_endpoint().port(); // ephemeral port
        waitingForServerAccept = false;
        useRelay = true;
        relayRoomId = 0;
        relaySlotId = 0;
        waitingForRelayAccept = true;
        nextClientId = 1;
        known_clients.clear();
        remotePlayers.clear();
        
        // Send RELAY_CREATE to the relay server
        char createBuf[sizeof(RelayHeader) + 48] = {};
        RelayHeader* hdr = (RelayHeader*)createBuf;
        hdr->magic = RELAY_MAGIC;
        hdr->roomId = 0;
        hdr->senderId = 0;
        hdr->packetType = (uint32_t)RelayPacketType::RELAY_CREATE;
        // Append room name
        std::strncpy(createBuf + sizeof(RelayHeader), name.c_str(), 47);
        
        asio::error_code ec;
        socket->send_to(asio::buffer(createBuf, sizeof(RelayHeader) + std::min(name.size() + 1, (size_t)48)),
                        lobby_server_endpoint, 0, ec);
        
        if (ec) {
            std::cerr << "Relay: failed to send CREATE: " << ec.message() << std::endl;
            return false;
        }
        
        std::cout << "Relay server mode: waiting for room assignment from relay..." << std::endl;
        return true;
    } catch (std::exception& e) {
        std::cerr << "Relay server error: " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::StartClientRelay(uint32_t roomId) {
    if (!lobby_server_available || roomId == 0) {
        std::cerr << "Relay: invalid room or lobby not configured!" << std::endl;
        return false;
    }
    if (socket) {
        socket->close();
        delete socket;
        socket = nullptr;
    }
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
        socket->non_blocking(true);
        
        // In relay mode, we talk to the relay server, not the game host directly
        server_target_endpoint = lobby_server_endpoint;
        
        mode = Mode::CLIENT;
        localPlayerId = -1;
        localPlayerAdmin = false;
        waitingForServerAccept = true;
        useRelay = true;
        relayRoomId = roomId;
        relaySlotId = 0;
        waitingForRelayAccept = true;
        remotePlayers.clear();
        syncedEnemies.clear();
        
        // Send RELAY_JOIN
        RelayHeader joinHdr;
        joinHdr.magic = RELAY_MAGIC;
        joinHdr.roomId = roomId;
        joinHdr.senderId = 0;
        joinHdr.packetType = (uint32_t)RelayPacketType::RELAY_JOIN;
        
        asio::error_code ec;
        socket->send_to(asio::buffer(&joinHdr, sizeof(joinHdr)), lobby_server_endpoint, 0, ec);
        lastRelayJoinRequest = std::chrono::steady_clock::now();
        
        std::cout << "Relay client: joining room " << roomId << "..." << std::endl;
        return true;
    } catch (std::exception& e) {
        std::cerr << "Relay client error: " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::SendViaRelay(const void* data, size_t len) {
    if (!socket || !useRelay || relayRoomId == 0) return;
    
    // Prepend RelayHeader to the game data
    static char relay_send_buf[32768];
    if (len + sizeof(RelayHeader) > sizeof(relay_send_buf)) return;
    
    RelayHeader* hdr = (RelayHeader*)relay_send_buf;
    hdr->magic = RELAY_MAGIC;
    hdr->roomId = relayRoomId;
    hdr->senderId = relaySlotId;
    hdr->packetType = (uint32_t)RelayPacketType::RELAY_DATA;
    
    std::memcpy(relay_send_buf + sizeof(RelayHeader), data, len);
    
    asio::error_code ec;
    socket->send_to(asio::buffer(relay_send_buf, sizeof(RelayHeader) + len),
                    lobby_server_endpoint, 0, ec);
}

void NetworkManager::SendRelayPing() {
    if (!lobby_server_available) return;
    
    if (ping_socket) {
        ping_socket->close();
        delete ping_socket;
        ping_socket = nullptr;
    }
    ping_socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    ping_socket->non_blocking(true);
    
    RelayHeader ping;
    ping.magic = RELAY_MAGIC;
    ping.roomId = 0;
    ping.senderId = 0;
    ping.packetType = (uint32_t)RelayPacketType::RELAY_PING;
    
    asio::error_code ec;
    ping_socket->send_to(asio::buffer(&ping, sizeof(ping)), lobby_server_endpoint, 0, ec);
    if (!ec) {
        lastPingTime = GetTime();
        pinging = true;
    }
}

void NetworkManager::SendRelayLeave() {
    if (!socket || relayRoomId == 0) return;
    RelayHeader hdr;
    hdr.magic = RELAY_MAGIC;
    hdr.roomId = relayRoomId;
    hdr.senderId = relaySlotId;
    hdr.packetType = (uint32_t)RelayPacketType::RELAY_LEAVE;
    asio::error_code ec;
    socket->send_to(asio::buffer(&hdr, sizeof(hdr)), lobby_server_endpoint, 0, ec);
}

// ============================================================

void NetworkManager::SetLocalPlayerName(const std::string& name) {
    localPlayerName = SanitizeLobbyField(name.empty() ? "Gooner" : name, 31);
}

void NetworkManager::SetLobbyServer(const std::string& host, int port) {
    std::string cleanHost = SanitizeLobbyField(host.empty() ? "127.0.0.1" : host, 128);
    int cleanPort = port > 0 ? port : 1240;
    if (cleanHost == lobbyHost && cleanPort == lobbyPort) return;
    lobbyHost = cleanHost;
    lobbyPort = cleanPort;
    ConfigureLobbyServer();
    discoveredServers.clear();
    lastLobbyQuery = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    lastLobbyHeartbeat = std::chrono::steady_clock::now() - std::chrono::seconds(3);
}

void NetworkManager::StartDiscovery() {
    try {
        if (discovery_socket) return;
        discovery_socket = new asio::ip::udp::socket(io_context);
        discovery_socket->open(asio::ip::udp::v4());
        discovery_socket->set_option(asio::socket_base::reuse_address(true));
        discovery_socket->set_option(asio::socket_base::broadcast(true));
        discovery_socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 1235));
        discovery_socket->non_blocking(true);
    } catch (...) {
        if (discovery_socket) {
            delete discovery_socket;
            discovery_socket = nullptr;
        }
    }
}

void NetworkManager::Update() {
    // Relay mode: retry CREATE if we haven't got ACCEPT yet
    if (useRelay && waitingForRelayAccept && socket) {
        auto now = std::chrono::steady_clock::now();
        if (mode == Mode::SERVER && relayRoomId == 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRelayJoinRequest).count();
            if (elapsed >= 1000) {
                // Resend RELAY_CREATE
                char createBuf[sizeof(RelayHeader) + 48] = {};
                RelayHeader* hdr = (RelayHeader*)createBuf;
                hdr->magic = RELAY_MAGIC;
                hdr->roomId = 0;
                hdr->senderId = 0;
                hdr->packetType = (uint32_t)RelayPacketType::RELAY_CREATE;
                std::strncpy(createBuf + sizeof(RelayHeader), serverName.c_str(), 47);
                asio::error_code ec;
                socket->send_to(asio::buffer(createBuf, sizeof(RelayHeader) + std::min(serverName.size() + 1, (size_t)48)),
                                lobby_server_endpoint, 0, ec);
                lastRelayJoinRequest = now;
            }
        } else if (mode == Mode::CLIENT) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRelayJoinRequest).count();
            if (elapsed >= 500) {
                // Resend RELAY_JOIN
                RelayHeader joinHdr;
                joinHdr.magic = RELAY_MAGIC;
                joinHdr.roomId = relayRoomId;
                joinHdr.senderId = 0;
                joinHdr.packetType = (uint32_t)RelayPacketType::RELAY_JOIN;
                asio::error_code ec;
                socket->send_to(asio::buffer(&joinHdr, sizeof(joinHdr)), lobby_server_endpoint, 0, ec);
                lastRelayJoinRequest = now;
            }
        }
    }

    if (pinging && ping_socket) {
        asio::error_code ec;
        asio::ip::udp::endpoint sender;
        char pbuffer[128];
        while (true) {
            size_t len = ping_socket->receive_from(asio::buffer(pbuffer), sender, 0, ec);
            if (ec) break;
            if (len >= sizeof(RelayHeader)) {
                RelayHeader* hdr = (RelayHeader*)pbuffer;
                if (hdr->magic == RELAY_MAGIC && (RelayPacketType)hdr->packetType == RelayPacketType::RELAY_PONG) {
                    currentPingMs = (GetTime() - lastPingTime) * 1000.0f;
                    pinging = false;
                    ping_socket->close();
                    delete ping_socket;
                    ping_socket = nullptr;
                }
            }
        }
    }

    // Client (non-relay): retry connect request
    if (mode == Mode::CLIENT && waitingForServerAccept && !useRelay && socket) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastConnectRequest).count() >= 500) {
            SendConnectRequest();
        }
    }

    // Discovery (Broadcast presence if server, listen if none)
    if (mode == Mode::SERVER && socket) {
        auto now = std::chrono::steady_clock::now();
        
        // LAN broadcast (only in non-relay mode)
        if (!useRelay) {
            static auto lastBroadcast = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastBroadcast).count() >= 1) {
                std::string msg = "AdasServer:" + std::to_string(activePort) + ":" + serverName;
                asio::ip::udp::endpoint broadcast_ep(asio::ip::address_v4::broadcast(), 1235);
                asio::error_code ec;
                socket->send_to(asio::buffer(msg), broadcast_ep, 0, ec);
                
                // Also send specifically to localhost for local testing compatibility
                asio::ip::udp::endpoint local_ep(asio::ip::address_v4::loopback(), 1235);
                socket->send_to(asio::buffer(msg), local_ep, 0, ec);
                
                lastBroadcast = now;
            }
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLobbyHeartbeat).count() >= 2) {
            RegisterInternetLobby();
            lastLobbyHeartbeat = now;
        }
    }
    
    if (mode == Mode::NONE && discovery_socket) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLobbyQuery).count() >= 1) {
            QueryInternetLobbies();
            lastLobbyQuery = now;
        }
    }

    if (discovery_socket) HandleDiscovery();

    if (!socket) return;

    asio::error_code ec;
    while (true) {
        size_t len = socket->receive_from(asio::buffer(recv_buffer), sender_endpoint, 0, ec);
        if (ec == asio::error::would_block || ec == asio::error::try_again) break;
        if (ec) {
            std::cerr << "Receive error: " << ec.message() << std::endl;
            break;
        }
        
        // If in relay mode, check for relay headers
        if (useRelay && len >= sizeof(RelayHeader)) {
            RelayHeader* hdr = (RelayHeader*)recv_buffer;
            if (hdr->magic == RELAY_MAGIC) {
                // Handle RELAY_ACCEPT
                if ((RelayPacketType)hdr->packetType == RelayPacketType::RELAY_ACCEPT) {
                    if (mode == Mode::SERVER && waitingForRelayAccept) {
                        if (hdr->roomId != 0) {
                            relayRoomId = hdr->roomId;
                            relaySlotId = hdr->senderId; // Should be 0 for host
                            waitingForRelayAccept = false;
                            std::cout << "Relay: room created with ID " << relayRoomId << std::endl;
                            RegisterInternetLobby(); // Register with room ID
                        }
                    } else if (mode == Mode::CLIENT && waitingForRelayAccept) {
                        if (hdr->roomId != 0) {
                            relaySlotId = hdr->senderId;
                            waitingForRelayAccept = false;
                            std::cout << "Relay: joined room " << relayRoomId 
                                      << " as slot " << relaySlotId << std::endl;
                            // Now send game ConnectRequest through relay
                            SendConnectRequest();
                        } else {
                            std::cerr << "Relay: room not found!" << std::endl;
                            shouldQuit = true;
                        }
                    }
                    continue;
                }
                
                // Handle RELAY_DATA — strip header and process inner game packet
                if ((RelayPacketType)hdr->packetType == RelayPacketType::RELAY_DATA) {
                    size_t innerLen = len - sizeof(RelayHeader);
                    if (innerLen > 0) {
                        // Move inner data to beginning of buffer for HandlePacket
                        std::memmove(recv_buffer, recv_buffer + sizeof(RelayHeader), innerLen);
                        // In relay mode, the "sender" is virtual — we use the relay slot to build
                        // a virtual endpoint for known_clients tracking
                        asio::ip::udp::endpoint virtualSender(
                            asio::ip::address_v4(hdr->senderId + 1),  // virtual IP from slot
                            hdr->senderId + 10000  // virtual port from slot
                        );
                        HandlePacket(innerLen, virtualSender);
                    }
                    continue;
                }
                
                continue; // Skip other relay packet types
            }
        }
        
        HandlePacket(len, sender_endpoint);
    }
}

void NetworkManager::HandlePacket(size_t bytes_recvd, const asio::ip::udp::endpoint& sender) {
    if (bytes_recvd == sizeof(ConnectRequest) && mode == Mode::SERVER) {
        ConnectRequest* req = (ConnectRequest*)recv_buffer;
        if (req->magic == PROTOCOL_MAGIC && req->type == (uint32_t)PacketType::CONNECT_REQUEST) {
            int assignedId = -1;
            for (auto const& [id, endpoint] : known_clients) {
                if (endpoint == sender) {
                    assignedId = id;
                    break;
                }
            }
            if (assignedId < 0) {
                assignedId = nextClientId++;
                known_clients[assignedId] = sender;
                std::cout << "Client joined from " << sender.address().to_string()
                          << ":" << sender.port() << " as ID " << assignedId << std::endl;
            }

            bool wasKnownPlayer = remotePlayers.count(assignedId) > 0;
            PlayerData& player = remotePlayers[assignedId];
            bool keepAdmin = wasKnownPlayer && player.admin;
            player.id = assignedId;
            std::memset(player.name, 0, sizeof(player.name));
            std::strncpy(player.name, req->name[0] ? req->name : "Gooner", sizeof(player.name) - 1);
            player.position = {0.0f, 2.0f, 0.0f};
            player.camTarget = {0.0f, 2.0f, 1.0f};
            if (player.hp <= 0) player.hp = 100;
            player.active = true;
            player.admin = keepAdmin;

            ConnectAccept accept = {};
            accept.magic = PROTOCOL_MAGIC;
            accept.type = (uint32_t)PacketType::CONNECT_ACCEPT;
            accept.id = assignedId;
            std::strncpy(accept.serverName, serverName.c_str(), sizeof(accept.serverName) - 1);
            
            if (useRelay) {
                SendViaRelay(&accept, sizeof(accept));
                PlayerData snapshot = player;
                SendViaRelay(&snapshot, sizeof(snapshot));
            } else {
                asio::error_code ec;
                socket->send_to(asio::buffer(&accept, sizeof(accept)), sender, 0, ec);
                PlayerData snapshot = player;
                for (auto const& [cid, endpoint] : known_clients) {
                    socket->send_to(asio::buffer(&snapshot, sizeof(snapshot)), endpoint, 0, ec);
                }
            }
            return;
        }
    }

    if (bytes_recvd == sizeof(ConnectAccept) && mode == Mode::CLIENT) {
        ConnectAccept* accept = (ConnectAccept*)recv_buffer;
        if (accept->magic == PROTOCOL_MAGIC && accept->type == (uint32_t)PacketType::CONNECT_ACCEPT) {
            localPlayerId = accept->id;
            waitingForServerAccept = false;
            serverName = accept->serverName[0] ? accept->serverName : serverName;
            localPlayerAdmin = false;
            std::cout << "Accepted by server as player ID " << localPlayerId << std::endl;
            return;
        }
    }

    if (bytes_recvd == sizeof(PlayerData)) {
        PlayerData* data = (PlayerData*)recv_buffer;
        if (mode == Mode::CLIENT && waitingForServerAccept) return;
        
        // Client: check if we've been kicked
        if (data->id == localPlayerId && !data->active && mode == Mode::CLIENT) {
            if (data->hp == -999) hostDisconnected = true;
            shouldQuit = true;
            return;
        }

        if (data->id == localPlayerId && data->active && mode == Mode::CLIENT) {
            localPlayerAdmin = data->admin;
            remotePlayers[data->id] = *data;
            return;
        }
        
        if (data->id != localPlayerId) {
            if (!data->active) {
                // Player disconnected
                remotePlayers.erase(data->id);
                if (mode == Mode::SERVER) {
                    known_clients.erase(data->id);
                    // Relay disconnect to all other clients
                    if (useRelay) {
                        SendViaRelay(recv_buffer, bytes_recvd);
                    } else {
                        for (auto const& [cid, endpoint] : known_clients) {
                            socket->send_to(asio::buffer(recv_buffer, bytes_recvd), endpoint);
                        }
                    }
                }
            } else {
                // Player update
                if (mode == Mode::SERVER && known_clients.count(data->id) && known_clients[data->id] == sender && remotePlayers.count(data->id)) {
                    // SERVER AUTHORITY: Keep existing HP and Money, only update position and input
                    int oldHp = remotePlayers[data->id].hp;
                    int oldMoney = remotePlayers[data->id].money;
                    bool oldAdmin = remotePlayers[data->id].admin;
                    remotePlayers[data->id] = *data;
                    remotePlayers[data->id].hp = oldHp;
                    remotePlayers[data->id].money = oldMoney;
                    remotePlayers[data->id].admin = oldAdmin;
                } else {
                    remotePlayers[data->id] = *data;
                    if (mode == Mode::SERVER && hostPlayerId == -1 && data->id != 0) {
                        hostPlayerId = data->id;
                        std::cout << "Assigned Host ID: " << hostPlayerId << std::endl;
                    }
                }
                
                if (mode == Mode::SERVER) {
                    if (!known_clients.count(data->id) || known_clients[data->id] != sender) return;
                    // Relay to all other clients
                    if (useRelay) {
                        SendViaRelay(recv_buffer, bytes_recvd);
                    } else {
                        for (auto const& [cid, endpoint] : known_clients) {
                            if (cid != data->id) {
                                socket->send_to(asio::buffer(recv_buffer, bytes_recvd), endpoint);
                            }
                        }
                    }
                }
            }
        }
    } else if (bytes_recvd > sizeof(PlayerData) && mode == Mode::CLIENT) {
        // WORLD UPDATE from server
        // Protocol: [float:BaseHP][int:Wave][int:GameStarted][int:PlayerCount][PlayerSync...][int:EnemyCount][EnemySync...]
        // ALL fields are 4-byte aligned (no bool!) to prevent alignment bugs
        char* ptr = recv_buffer;
        
        float baseHP = *(float*)ptr; ptr += sizeof(float);
        int wave = *(int*)ptr; ptr += sizeof(int);
        int startedFlag = *(int*)ptr; ptr += sizeof(int);  // int instead of bool for alignment!
        
        remoteBaseHP = baseHP;
        remoteWave = wave;
        gameStarted = (startedFlag != 0);
        
        int pCount = *(int*)ptr; ptr += sizeof(int);
        // Sanity check
        if (pCount < 0 || pCount > 32) return;
        
        PlayerSyncData* pList = (PlayerSyncData*)ptr;
        for (int i = 0; i < pCount; i++) {
             remotePlayers[pList[i].id].hp = pList[i].hp;
             remotePlayers[pList[i].id].money = pList[i].money;
             remotePlayers[pList[i].id].admin = (pList[i].admin != 0);
             remotePlayers[pList[i].id].id = pList[i].id;
             remotePlayers[pList[i].id].active = true;
             if (pList[i].id == localPlayerId) {
                 localPlayerAdmin = remotePlayers[pList[i].id].admin;
             }
        }
        ptr += pCount * sizeof(PlayerSyncData);

        int eCount = *(int*)ptr; ptr += sizeof(int);
        // Sanity check
        if (eCount < 0 || eCount > 200) return;
        
        EnemySync* eList = (EnemySync*)ptr;
        syncedEnemies.clear();
        for (int i = 0; i < eCount; i++) {
            syncedEnemies[eList[i].id] = eList[i];
        }
    }
}

void NetworkManager::HandleDiscovery() {
    asio::error_code ec;
    asio::ip::udp::endpoint disc_sender;
    while (true) {
        size_t len = discovery_socket->receive_from(asio::buffer(disc_buffer), disc_sender, 0, ec);
        if (ec == asio::error::would_block || ec == asio::error::try_again) break;
        if (ec) break;
        if (len > 0) {
            std::string msg(disc_buffer, len);
            if (msg.find("AdasServer:") == 0) {
                std::string ip = disc_sender.address().to_string();
                size_t firstColon = msg.find(':');
                size_t secondColon = msg.find(':', firstColon + 1);
            int serverPort = 1234;
            std::string sName = "LAN SERVER";
            bool internet = false;
            int players = 0;
            int maxPlayers = 8;
            bool started = false;
            if (secondColon != std::string::npos) {
                try { serverPort = std::stoi(msg.substr(firstColon + 1, secondColon - firstColon - 1)); } catch (...) {}
                sName = msg.substr(secondColon + 1);
                } else if (firstColon != std::string::npos) {
                    try { serverPort = std::stoi(msg.substr(firstColon + 1)); } catch (...) {}
            }
            std::string serverKey = ip + ":" + std::to_string(serverPort);
            discoveredServers[serverKey] = { ip, serverPort, sName, (float)GetTime(), players, maxPlayers, started, internet, 0 };
        } else if (msg.find("LOBBY|") == 0) {
            std::stringstream ss(msg);
            std::string line;
            while (std::getline(ss, line)) {
                std::stringstream ls(line);
                std::string tag, ip, portStr, name, playersStr, maxStr, startedStr, roomIdStr;
                if (!std::getline(ls, tag, '|') || tag != "LOBBY") continue;
                if (!std::getline(ls, ip, '|')) continue;
                if (!std::getline(ls, portStr, '|')) continue;
                if (!std::getline(ls, name, '|')) continue;
                std::getline(ls, playersStr, '|');
                std::getline(ls, maxStr, '|');
                std::getline(ls, startedStr, '|');
                std::getline(ls, roomIdStr, '|');
                int port = 1234;
                try { port = std::stoi(portStr); } catch (...) {}
                int players = 0;
                int maxPlayers = 8;
                try { players = std::stoi(playersStr); } catch (...) {}
                try { maxPlayers = std::stoi(maxStr); } catch (...) {}
                bool started = (startedStr == "1");
                uint32_t roomId = 0;
                try { roomId = (uint32_t)std::stoul(roomIdStr); } catch (...) {}
                std::string serverKey = ip + ":" + std::to_string(port);
                discoveredServers[serverKey] = { ip, port, name.empty() ? "INTERNET LOBBY" : name, (float)GetTime(), players, maxPlayers, started, true, roomId };
            }
        }
    }
    }
    for (auto it = discoveredServers.begin(); it != discoveredServers.end(); ) {
        if (GetTime() - it->second.lastSeen > 3.0f) it = discoveredServers.erase(it);
        else ++it;
    }
}

void NetworkManager::SendWorldUpdate(float baseHP, int wave, const std::vector<EnemySync>& enemies, const std::vector<PlayerSyncData>& players) {
    if (mode != Mode::SERVER || !socket) return;

    static char world_buffer[32768];
    char* ptr = world_buffer;
    
    // ALL fields are int/float (4 bytes) for proper alignment
    *(float*)ptr = baseHP; ptr += sizeof(float);
    *(int*)ptr = wave; ptr += sizeof(int);
    *(int*)ptr = gameStarted ? 1 : 0; ptr += sizeof(int);  // int instead of bool!
    
    *(int*)ptr = (int)players.size(); ptr += sizeof(int);
    memcpy(ptr, players.data(), players.size() * sizeof(PlayerSyncData));
    ptr += players.size() * sizeof(PlayerSyncData);

    int eCount = (int)enemies.size();
    if (eCount > 150) eCount = 150;
    *(int*)ptr = eCount; ptr += sizeof(int);
    memcpy(ptr, enemies.data(), eCount * sizeof(EnemySync));
    ptr += eCount * sizeof(EnemySync);

    size_t total_size = ptr - world_buffer;
    
    if (useRelay) {
        SendViaRelay(world_buffer, total_size);
    } else {
        for (auto const& [cid, endpoint] : known_clients) {
            asio::error_code ec;
            socket->send_to(asio::buffer(world_buffer, total_size), endpoint, 0, ec);
        }
    }
}

void NetworkManager::SendPlayerUpdate(int id, const char* name, Vector3 pos, Vector3 target, int hp, int money, bool firing) {
    if (!socket) return;
    if (mode == Mode::CLIENT && waitingForServerAccept) return;
    PlayerData data = { id, "", pos, target, hp, money, firing, true, localPlayerAdmin };
    if (name) strncpy(data.name, name, 31);
    
    if (useRelay) {
        SendViaRelay(&data, sizeof(PlayerData));
    } else {
        asio::error_code ec;
        if (mode == Mode::CLIENT) {
            socket->send_to(asio::buffer(&data, sizeof(PlayerData)), server_target_endpoint, 0, ec);
        } else if (mode == Mode::SERVER) {
            for (auto const& [cid, endpoint] : known_clients) {
                socket->send_to(asio::buffer(&data, sizeof(PlayerData)), endpoint, 0, ec);
            }
        }
    }
}

void NetworkManager::SendDisconnect(int id) {
    if (!socket) return;
    PlayerData data = { id, "", {0,0,0}, {0,0,0}, 0, 0, false, false, false }; 
    
    if (useRelay) {
        if (mode == Mode::SERVER) {
            // Notify all clients via relay
            for (auto const& [cid, _] : known_clients) {
                PlayerData kickData = { cid, "", {0,0,0}, {0,0,0}, -999, 0, false, false, false };
                SendViaRelay(&kickData, sizeof(PlayerData));
            }
        }
        SendViaRelay(&data, sizeof(PlayerData));
        SendRelayLeave();
    } else {
        asio::error_code ec;
        if (mode == Mode::CLIENT) {
            socket->send_to(asio::buffer(&data, sizeof(PlayerData)), server_target_endpoint, 0, ec);
        } else if (mode == Mode::SERVER) {
            for (auto const& [cid, endpoint] : known_clients) {
                socket->send_to(asio::buffer(&data, sizeof(PlayerData)), endpoint, 0, ec);
                
                // Kick the client with -999 HP flag
                PlayerData kickData = { cid, "", {0,0,0}, {0,0,0}, -999, 0, false, false, false };
                socket->send_to(asio::buffer(&kickData, sizeof(PlayerData)), endpoint, 0, ec);
            }
        }
    }
}

bool NetworkManager::GrantAdmin(int id) {
    if (mode != Mode::SERVER) return false;
    if (id == 0) {
        localPlayerAdmin = true;
        return true;
    }
    auto it = remotePlayers.find(id);
    if (it == remotePlayers.end() || !it->second.active) return false;
    it->second.admin = true;
    
    if (useRelay) {
        SendViaRelay(&it->second, sizeof(PlayerData));
    } else {
        asio::error_code ec;
        auto clientIt = known_clients.find(id);
        if (clientIt != known_clients.end() && socket) {
            socket->send_to(asio::buffer(&it->second, sizeof(PlayerData)), clientIt->second, 0, ec);
        }
    }
    return true;
}

bool NetworkManager::KickPlayer(int id) {
    if (mode != Mode::SERVER || id == 0) return false;
    auto clientIt = known_clients.find(id);
    if (clientIt == known_clients.end() || !socket) return false;
    PlayerData kickData = { id, "", {0,0,0}, {0,0,0}, -999, 0, false, false, false };
    
    if (useRelay) {
        SendViaRelay(&kickData, sizeof(PlayerData));
    } else {
        asio::error_code ec;
        socket->send_to(asio::buffer(&kickData, sizeof(PlayerData)), clientIt->second, 0, ec);
    }
    known_clients.erase(id);
    remotePlayers.erase(id);
    return true;
}

void NetworkManager::SendConnectRequest() {
    if (mode != Mode::CLIENT || !socket) return;
    ConnectRequest req = {};
    req.magic = PROTOCOL_MAGIC;
    req.type = (uint32_t)PacketType::CONNECT_REQUEST;
    std::strncpy(req.name, localPlayerName.c_str(), sizeof(req.name) - 1);
    
    if (useRelay) {
        SendViaRelay(&req, sizeof(req));
    } else {
        asio::error_code ec;
        socket->send_to(asio::buffer(&req, sizeof(req)), server_target_endpoint, 0, ec);
    }
    lastConnectRequest = std::chrono::steady_clock::now();
}

void NetworkManager::RegisterInternetLobby() {
    if (!lobby_server_available || !socket || mode != Mode::SERVER) return;
    std::string name = SanitizeLobbyField(serverName, 48);
    int players = 1;
    for (auto const& [id, data] : remotePlayers) {
        if (data.active) players++;
    }
    std::string msg = "ADAS_LOBBY_REGISTER|" + std::to_string(activePort) + "|" +
                      name + "|" + std::to_string(players) + "|8|" +
                      (gameStarted ? "1" : "0") + "|" +
                      std::to_string(relayRoomId);
    asio::error_code ec;
    socket->send_to(asio::buffer(msg), lobby_server_endpoint, 0, ec);
}

void NetworkManager::QueryInternetLobbies() {
    if (!lobby_server_available || !discovery_socket) return;
    std::string msg = "ADAS_LOBBY_LIST";
    asio::error_code ec;
    discovery_socket->send_to(asio::buffer(msg), lobby_server_endpoint, 0, ec);
}

void NetworkManager::ConfigureLobbyServer() {
    const char* hostEnv = std::getenv("ADAS_LOBBY_HOST");
    const char* portEnv = std::getenv("ADAS_LOBBY_PORT");
    bool usingEnvDefaults = lobbyHost.empty();
    if (usingEnvDefaults) {
        lobbyHost = hostEnv && hostEnv[0] ? hostEnv : "127.0.0.1";
    }
    if (lobbyPort <= 0) {
        lobbyPort = 1240;
    }
    if (usingEnvDefaults && portEnv && portEnv[0] && lobbyPort == 1240) {
        try { lobbyPort = std::stoi(portEnv); } catch (...) { lobbyPort = 1240; }
    }
    std::string host = lobbyHost;
    std::string port = std::to_string(lobbyPort);
    try {
        asio::ip::udp::resolver resolver(io_context);
        lobby_server_endpoint = *resolver.resolve(asio::ip::udp::v4(), host, port).begin();
        lobby_server_available = true;
    } catch (std::exception& e) {
        lobby_server_available = false;
        std::cerr << "Lobby server config error: " << e.what() << std::endl;
    }
}

std::string NetworkManager::SanitizeLobbyField(const std::string& value, size_t maxLen) const {
    std::string out;
    out.reserve(std::min(value.size(), maxLen));
    for (char c : value) {
        if (out.size() >= maxLen) break;
        if (c == '|' || c == '\n' || c == '\r') continue;
        if ((unsigned char)c < 32) continue;
        out.push_back(c);
    }
    return out;
}
