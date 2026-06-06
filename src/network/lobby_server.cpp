#include "lobby_server.hpp"

#include <asio.hpp>
#include <chrono>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cstring>

// ---- Lobby List (legacy, kept for backwards compat) ----

struct LobbyEntry {
    std::string ip;
    int port = 1234;
    std::string name = "LOBBY";
    int players = 0;
    int maxPlayers = 8;
    bool started = false;
    uint32_t roomId = 0;   // Relay room ID (0 = not relayed / direct)
    std::chrono::steady_clock::time_point lastSeen;
};

// ---- Relay Room ----

struct RoomParticipant {
    asio::ip::udp::endpoint endpoint;
    uint32_t slotId;  // 0 = host, 1+ = clients
    std::chrono::steady_clock::time_point lastSeen;
};

struct RelayRoom {
    uint32_t roomId;
    std::string name;
    std::vector<RoomParticipant> participants;
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::steady_clock::time_point lastActivity;
};

// ---- Helpers ----

static std::string CleanField(const std::string& value, size_t maxLen) {
    std::string out;
    for (char c : value) {
        if (out.size() >= maxLen) break;
        if (c == '|' || c == '\n' || c == '\r') continue;
        if ((unsigned char)c < 32) continue;
        out.push_back(c);
    }
    return out;
}

static void PrintLobbyList(const std::map<std::string, LobbyEntry>& lobbies) {
    if (lobbies.empty()) {
        std::cout << "No registered lobbies." << std::endl;
        return;
    }
    std::cout << "Registered lobbies:" << std::endl;
    for (const auto& [key, lobby] : lobbies) {
        std::cout << "  " << lobby.name
                  << " | " << lobby.ip << ":" << lobby.port
                  << " | " << lobby.players << "/" << lobby.maxPlayers
                  << " | started=" << (lobby.started ? "yes" : "no")
                  << " | roomId=" << lobby.roomId
                  << std::endl;
    }
}

static void PrintRooms(const std::map<uint32_t, RelayRoom>& rooms) {
    if (rooms.empty()) {
        std::cout << "No active relay rooms." << std::endl;
        return;
    }
    std::cout << "Active relay rooms:" << std::endl;
    for (const auto& [id, room] : rooms) {
        std::cout << "  Room " << id << " (" << room.name << ") - "
                  << room.participants.size() << " participants" << std::endl;
        for (const auto& p : room.participants) {
            std::cout << "    slot " << p.slotId << " @ "
                      << p.endpoint.address().to_string() << ":" << p.endpoint.port()
                      << std::endl;
        }
    }
}

// Simple hash for room IDs
static uint32_t HashRoomName(const std::string& name, const asio::ip::udp::endpoint& ep) {
    uint32_t hash = 5381;
    for (char c : name) hash = ((hash << 5) + hash) + c;
    hash ^= ep.port();
    hash ^= (uint32_t)(std::chrono::steady_clock::now().time_since_epoch().count() & 0xFFFFFFFF);
    if (hash == 0) hash = 1; // Never use 0
    return hash;
}

// ---- Main Server ----

int RunLobbyServer(int port) {
    asio::io_context io;
    asio::ip::udp::socket socket(io);
    asio::error_code ec;

    socket.open(asio::ip::udp::v4(), ec);
    if (ec) {
        std::cerr << "Relay server socket open failed: " << ec.message() << std::endl;
        return 1;
    }

    socket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port), ec);
    if (ec) {
        std::cerr << "Relay server bind UDP " << port << " failed: " << ec.message() << std::endl;
        return 1;
    }

    socket.non_blocking(true, ec);
    if (ec) {
        std::cerr << "Relay server non-blocking mode failed: " << ec.message() << std::endl;
        return 1;
    }

    // Increase socket buffer sizes for relay throughput
    socket.set_option(asio::socket_base::receive_buffer_size(262144), ec);
    socket.set_option(asio::socket_base::send_buffer_size(262144), ec);

    std::map<std::string, LobbyEntry> lobbies;
    std::map<uint32_t, RelayRoom> rooms;
    char buffer[32768]; // Larger buffer for relay (header + game data)

    std::cout << "====================================" << std::endl;
    std::cout << "   Adas Relay / Lobby Server        " << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Listening on UDP " << port << std::endl;
    std::cout << "Commands: list | rooms | status | help | quit" << std::endl;

    // Input thread
    std::queue<std::string> commands;
    std::mutex commandsMutex;
    bool running = true;
    std::thread inputThread([&]() {
        std::string line;
        while (running && std::getline(std::cin, line)) {
            std::lock_guard<std::mutex> lock(commandsMutex);
            commands.push(line);
        }
    });

    auto handleCommands = [&]() {
        std::lock_guard<std::mutex> lock(commandsMutex);
        while (!commands.empty()) {
            std::string line = commands.front();
            commands.pop();
            std::stringstream ss(line);
            std::string cmd;
            ss >> cmd;

            if (cmd == "list") {
                PrintLobbyList(lobbies);
            } else if (cmd == "rooms") {
                PrintRooms(rooms);
            } else if (cmd == "status") {
                std::cout << "Relay server UDP " << port
                          << " | lobbies: " << lobbies.size()
                          << " | relay rooms: " << rooms.size()
                          << std::endl;
            } else if (cmd == "help") {
                std::cout << "Commands: list | rooms | status | help | quit" << std::endl;
            } else if (cmd == "quit" || cmd == "exit") {
                running = false;
            } else if (!cmd.empty()) {
                std::cout << "Unknown command. Type: help" << std::endl;
            }
        }
    };

    uint64_t totalRelayed = 0;

    while (running) {
        handleCommands();
        ec = {};
        asio::ip::udp::endpoint sender;
        size_t len = socket.receive_from(asio::buffer(buffer), sender, 0, ec);

        if (ec == asio::error::would_block || ec == asio::error::try_again) {
            // Expire old lobbies
            auto now = std::chrono::steady_clock::now();
            for (auto it = lobbies.begin(); it != lobbies.end();) {
                if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen).count() > 10) {
                    it = lobbies.erase(it);
                } else {
                    ++it;
                }
            }

            // Expire old rooms (30s idle)
            for (auto it = rooms.begin(); it != rooms.end();) {
                if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastActivity).count() > 30) {
                    std::cout << "Room " << it->first << " (" << it->second.name << ") expired." << std::endl;
                    it = rooms.erase(it);
                } else {
                    ++it;
                }
            }

            handleCommands();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (ec) {
            std::cerr << "Receive error: " << ec.message() << std::endl;
            continue;
        }

        // ============================================================
        // Check if it's a RELAY packet (binary, starts with RELAY_MAGIC)
        // ============================================================
        if (len >= sizeof(RelayHeader)) {
            RelayHeader* hdr = (RelayHeader*)buffer;
            if (hdr->magic == RELAY_MAGIC) {
                auto now = std::chrono::steady_clock::now();

                switch ((RelayPacketType)hdr->packetType) {
                case RelayPacketType::RELAY_PING: {
                    std::cout << "[RELAY] Ping received from " << sender.address().to_string() << ":" << sender.port() << std::endl;
                    RelayHeader pongHdr;
                    pongHdr.magic = RELAY_MAGIC;
                    pongHdr.roomId = 0;
                    pongHdr.senderId = 0;
                    pongHdr.packetType = (uint32_t)RelayPacketType::RELAY_PONG;
                    socket.send_to(asio::buffer(&pongHdr, sizeof(pongHdr)), sender, 0, ec);
                    break;
                }

                case RelayPacketType::RELAY_CREATE: {
                    // Host wants to create a room
                    // Payload after header: room name (null-terminated string)
                    std::string roomName = "RELAY ROOM";
                    if (len > sizeof(RelayHeader)) {
                        roomName = CleanField(std::string(buffer + sizeof(RelayHeader),
                                              std::min(len - sizeof(RelayHeader), (size_t)48)), 48);
                    }
                    
                    uint32_t newRoomId = HashRoomName(roomName, sender);
                    RelayRoom room;
                    room.roomId = newRoomId;
                    room.name = roomName;
                    room.createdAt = now;
                    room.lastActivity = now;

                    RoomParticipant host;
                    host.endpoint = sender;
                    host.slotId = 0; // Host is always slot 0
                    host.lastSeen = now;
                    room.participants.push_back(host);

                    rooms[newRoomId] = room;

                    // Send RELAY_ACCEPT back with the roomId
                    RelayHeader accept;
                    accept.magic = RELAY_MAGIC;
                    accept.roomId = newRoomId;
                    accept.senderId = 0;
                    accept.packetType = (uint32_t)RelayPacketType::RELAY_ACCEPT;
                    socket.send_to(asio::buffer(&accept, sizeof(accept)), sender, 0, ec);

                    std::cout << "Room created: " << newRoomId << " (" << roomName << ") by "
                              << sender.address().to_string() << ":" << sender.port() << std::endl;
                    break;
                }

                case RelayPacketType::RELAY_JOIN: {
                    uint32_t roomId = hdr->roomId;
                    auto roomIt = rooms.find(roomId);
                    if (roomIt == rooms.end()) {
                        // Room not found — send accept with roomId=0 to indicate failure
                        RelayHeader fail;
                        fail.magic = RELAY_MAGIC;
                        fail.roomId = 0;
                        fail.senderId = 0;
                        fail.packetType = (uint32_t)RelayPacketType::RELAY_ACCEPT;
                        socket.send_to(asio::buffer(&fail, sizeof(fail)), sender, 0, ec);
                        break;
                    }

                    RelayRoom& room = roomIt->second;
                    room.lastActivity = now;

                    // Check if already in room
                    bool alreadyIn = false;
                    uint32_t existingSlot = 0;
                    for (auto& p : room.participants) {
                        if (p.endpoint == sender) {
                            alreadyIn = true;
                            existingSlot = p.slotId;
                            p.lastSeen = now;
                            break;
                        }
                    }
                    
                    uint32_t assignedSlot = existingSlot;
                    if (!alreadyIn) {
                        // Find next slot ID
                        uint32_t maxSlot = 0;
                        for (auto& p : room.participants) {
                            if (p.slotId > maxSlot) maxSlot = p.slotId;
                        }
                        assignedSlot = maxSlot + 1;

                        RoomParticipant client;
                        client.endpoint = sender;
                        client.slotId = assignedSlot;
                        client.lastSeen = now;
                        room.participants.push_back(client);

                        std::cout << "Client joined room " << roomId << " as slot " << assignedSlot
                                  << " from " << sender.address().to_string() << ":" << sender.port()
                                  << std::endl;
                    }

                    // Send accept with assigned slot
                    RelayHeader accept;
                    accept.magic = RELAY_MAGIC;
                    accept.roomId = roomId;
                    accept.senderId = assignedSlot;
                    accept.packetType = (uint32_t)RelayPacketType::RELAY_ACCEPT;
                    socket.send_to(asio::buffer(&accept, sizeof(accept)), sender, 0, ec);
                    break;
                }

                case RelayPacketType::RELAY_LEAVE: {
                    uint32_t roomId = hdr->roomId;
                    auto roomIt = rooms.find(roomId);
                    if (roomIt != rooms.end()) {
                        auto& participants = roomIt->second.participants;
                        bool wasHost = false;
                        for (auto it = participants.begin(); it != participants.end(); ++it) {
                            if (it->endpoint == sender) {
                                wasHost = (it->slotId == 0);
                                std::cout << "Participant left room " << roomId
                                          << " (slot " << it->slotId << ")" << std::endl;
                                participants.erase(it);
                                break;
                            }
                        }
                        // If host left or room empty, destroy room
                        if (wasHost || participants.empty()) {
                            std::cout << "Room " << roomId << " destroyed (host left or empty)." << std::endl;
                            rooms.erase(roomIt);
                        }
                    }
                    break;
                }

                case RelayPacketType::RELAY_DATA: {
                    uint32_t roomId = hdr->roomId;
                    auto roomIt = rooms.find(roomId);
                    if (roomIt == rooms.end()) break;

                    RelayRoom& room = roomIt->second;
                    room.lastActivity = now;

                    // Update sender's lastSeen
                    for (auto& p : room.participants) {
                        if (p.endpoint == sender) {
                            p.lastSeen = now;
                            break;
                        }
                    }

                    // Forward the ENTIRE packet (header + payload) to all other participants
                    for (const auto& p : room.participants) {
                        if (p.endpoint != sender) {
                            socket.send_to(asio::buffer(buffer, len), p.endpoint, 0, ec);
                        }
                    }
                    totalRelayed++;
                    break;
                }

                default:
                    break;
                }

                continue; // Don't process relay packets as lobby text commands
            }
        }

        // ============================================================
        // Legacy text-based lobby protocol (unchanged)
        // ============================================================
        std::string msg(buffer, len);
        if (msg.rfind("ADAS_LOBBY_REGISTER|", 0) == 0) {
            std::stringstream ss(msg);
            std::string tag, portStr, name, playersStr, maxPlayersStr, startedStr, roomIdStr;
            std::getline(ss, tag, '|');
            std::getline(ss, portStr, '|');
            std::getline(ss, name, '|');
            std::getline(ss, playersStr, '|');
            std::getline(ss, maxPlayersStr, '|');
            std::getline(ss, startedStr, '|');
            std::getline(ss, roomIdStr, '|');

            LobbyEntry entry;
            entry.ip = sender.address().to_string();
            try { entry.port = std::stoi(portStr); } catch (...) { entry.port = 1234; }
            entry.name = CleanField(name.empty() ? "LOBBY" : name, 48);
            try { entry.players = std::stoi(playersStr); } catch (...) { entry.players = 0; }
            try { entry.maxPlayers = std::stoi(maxPlayersStr); } catch (...) { entry.maxPlayers = 8; }
            entry.started = (startedStr == "1");
            try { entry.roomId = (uint32_t)std::stoul(roomIdStr); } catch (...) { entry.roomId = 0; }
            entry.lastSeen = std::chrono::steady_clock::now();

            std::string key = entry.ip + ":" + std::to_string(entry.port);
            lobbies[key] = entry;
            std::cout << "Registered lobby: " << entry.name
                      << " from " << key
                      << " players=" << entry.players << "/" << entry.maxPlayers
                      << " roomId=" << entry.roomId
                      << std::endl;
        } else if (msg == "ADAS_LOBBY_LIST") {
            std::string response;
            for (const auto& [key, lobby] : lobbies) {
                response += "LOBBY|" + lobby.ip + "|" + std::to_string(lobby.port) + "|" +
                            lobby.name + "|" + std::to_string(lobby.players) + "|" +
                            std::to_string(lobby.maxPlayers) + "|" +
                            (lobby.started ? "1" : "0") + "|" +
                            std::to_string(lobby.roomId) + "\n";
            }
            socket.send_to(asio::buffer(response), sender, 0, ec);
        }
    }

    running = false;
    if (inputThread.joinable()) inputThread.detach();
    return 0;
}
