#include "network_manager.hpp"
#include <iostream>

NetworkManager::NetworkManager() : mode(Mode::NONE), socket(nullptr), discovery_socket(nullptr), gameStarted(false), localPlayerId(-1) {
}

NetworkManager::~NetworkManager() {
    if (socket) {
        socket->close();
        delete socket;
    }
}

bool NetworkManager::StartServer(int port) {
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
        socket->set_option(asio::socket_base::broadcast(true));
        socket->non_blocking(true);
        mode = Mode::SERVER;
        localPlayerId = 0;
        std::cout << "ASIO Server started on port " << port << std::endl;
        return true;
    } catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::StartClient(const std::string& ip, int port) {
    try {
        socket = new asio::ip::udp::socket(io_context, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
        socket->non_blocking(true);
        
        asio::ip::udp::resolver resolver(io_context);
        server_target_endpoint = *resolver.resolve(asio::ip::udp::v4(), ip, std::to_string(port)).begin();
        
        mode = Mode::CLIENT;
        std::cout << "ASIO Client started, targeting " << ip << ":" << port << std::endl;
        return true;
    } catch (std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::StartDiscovery() {
    try {
        discovery_socket = new asio::ip::udp::socket(io_context);
        discovery_socket->open(asio::ip::udp::v4());
        discovery_socket->set_option(asio::socket_base::reuse_address(true));
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
    // Discovery (Broadcast presence if server, listen if none)
    if (mode == Mode::SERVER && socket) {
        static auto lastBroadcast = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastBroadcast).count() >= 1) {
            std::string msg = "AdasServer:1234";
            asio::ip::udp::endpoint broadcast_ep(asio::ip::address_v4::broadcast(), 1235);
            asio::error_code ec;
            socket->send_to(asio::buffer(msg), broadcast_ep, 0, ec);
            
            // Also send specifically to localhost for local testing compatibility
            asio::ip::udp::endpoint local_ep(asio::ip::address_v4::loopback(), 1235);
            socket->send_to(asio::buffer(msg), local_ep, 0, ec);
            
            lastBroadcast = now;
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
        HandlePacket(len, sender_endpoint);
    }
}

void NetworkManager::HandlePacket(size_t bytes_recvd, const asio::ip::udp::endpoint& sender) {
    if (bytes_recvd == sizeof(PlayerData)) {
        PlayerData* data = (PlayerData*)recv_buffer;
        if (data->id != localPlayerId) {
            if (!data->active) {
                remotePlayers.erase(data->id);
                if (mode == Mode::SERVER) {
                    known_clients.erase(data->id);
                    for (auto const& [cid, endpoint] : known_clients) socket->send_to(asio::buffer(recv_buffer, bytes_recvd), endpoint);
                }
            } else {
                if (mode == Mode::SERVER && remotePlayers.count(data->id)) {
                    // SERVER AUTHORITY: Keep existing HP and Money, only update position and input
                    int oldHp = remotePlayers[data->id].hp;
                    int oldMoney = remotePlayers[data->id].money;
                    remotePlayers[data->id] = *data;
                    remotePlayers[data->id].hp = oldHp;
                    remotePlayers[data->id].money = oldMoney;
                } else {
                    remotePlayers[data->id] = *data;
                }
                
                if (mode == Mode::SERVER) {
                    known_clients[data->id] = sender;
                    for (auto const& [cid, endpoint] : known_clients) if (cid != data->id) socket->send_to(asio::buffer(recv_buffer, bytes_recvd), endpoint);
                }
            }
        }
    } else if (bytes_recvd > sizeof(PlayerData) && mode == Mode::CLIENT) {
        // WORLD UPDATE: [f:BaseHP][i:Wave][b:Started][i:PlayerCount][PlayerSync...][i:EnemyCount][EnemySync...]
        char* ptr = recv_buffer;
        remoteBaseHP = *(float*)ptr; ptr += sizeof(float);
        remoteWave = *(int*)ptr; ptr += sizeof(int);
        gameStarted = *(bool*)ptr; ptr += sizeof(bool);
        
        int pCount = *(int*)ptr; ptr += sizeof(int);
        PlayerSyncData* pList = (PlayerSyncData*)ptr;
        for (int i = 0; i < pCount; i++) {
             remotePlayers[pList[i].id].hp = pList[i].hp;
             remotePlayers[pList[i].id].money = pList[i].money;
             remotePlayers[pList[i].id].id = pList[i].id; // Ensure ID is set
             remotePlayers[pList[i].id].active = true;
        }
        ptr += pCount * sizeof(PlayerSyncData);

        int eCount = *(int*)ptr; ptr += sizeof(int);
        EnemySync* eList = (EnemySync*)ptr;
        syncedEnemies.clear();
        for (int i = 0; i < eCount; i++) syncedEnemies[eList[i].id] = eList[i];
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
                discoveredServers[ip] = { ip, 1234, "LAN SERVER", (float)GetTime() };
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

    static char world_buffer[8192];
    char* ptr = world_buffer;
    
    *(float*)ptr = baseHP; ptr += sizeof(float);
    *(int*)ptr = wave; ptr += sizeof(int);
    *(bool*)ptr = gameStarted; ptr += sizeof(bool);
    
    *(int*)ptr = (int)players.size(); ptr += sizeof(int);
    memcpy(ptr, players.data(), players.size() * sizeof(PlayerSyncData));
    ptr += players.size() * sizeof(PlayerSyncData);

    int eCount = (int)enemies.size();
    if (eCount > 150) eCount = 150;
    *(int*)ptr = eCount; ptr += sizeof(int);
    memcpy(ptr, enemies.data(), eCount * sizeof(EnemySync));
    ptr += eCount * sizeof(EnemySync);

    size_t total_size = ptr - world_buffer;
    for (auto const& [cid, endpoint] : known_clients) {
        socket->send_to(asio::buffer(world_buffer, total_size), endpoint);
    }
}

void NetworkManager::SendPlayerUpdate(int id, Vector3 pos, Vector3 target, int hp, int money, bool firing) {
    if (!socket) return;
    PlayerData data = { id, pos, target, hp, money, firing, true };
    if (mode == Mode::CLIENT) {
        socket->send_to(asio::buffer(&data, sizeof(PlayerData)), server_target_endpoint);
    } else if (mode == Mode::SERVER) {
        for (auto const& [cid, endpoint] : known_clients) {
            socket->send_to(asio::buffer(&data, sizeof(PlayerData)), endpoint);
        }
    }
}

void NetworkManager::SendDisconnect(int id) {
    if (!socket) return;
    PlayerData data = { id, {0,0,0}, {0,0,0}, 0, false, false }; 
    if (mode == Mode::CLIENT) {
        socket->send_to(asio::buffer(&data, sizeof(PlayerData)), server_target_endpoint);
    } else if (mode == Mode::SERVER) {
        for (auto const& [cid, endpoint] : known_clients) {
            socket->send_to(asio::buffer(&data, sizeof(PlayerData)), endpoint);
        }
    }
}
