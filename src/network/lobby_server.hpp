#ifndef LOBBY_SERVER_HPP
#define LOBBY_SERVER_HPP

#include <cstdint>

// Shared relay protocol constants
static constexpr uint32_t RELAY_MAGIC = 0xADA50E1A;

enum class RelayPacketType : uint32_t {
    RELAY_DATA   = 1,  // Game data forwarding
    RELAY_CREATE = 2,  // Host creates a room
    RELAY_JOIN   = 3,  // Client joins a room
    RELAY_LEAVE  = 4,  // Participant leaves
    RELAY_PING   = 5,  // Ping the relay server
    RELAY_PONG   = 6,  // Pong reply from relay server
    RELAY_ACCEPT = 7,  // Server confirms room creation/join
};

struct RelayHeader {
    uint32_t magic;       // RELAY_MAGIC
    uint32_t roomId;      // Room ID
    uint32_t senderId;    // 0 = host, 1+ = client slot in room
    uint32_t packetType;  // RelayPacketType
};

int RunLobbyServer(int port);

#endif
