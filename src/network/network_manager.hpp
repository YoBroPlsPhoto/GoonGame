#ifndef NETWORK_MANAGER_HPP
#define NETWORK_MANAGER_HPP

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #define NOGDI
    #define NOUSER
#endif

#include <asio.hpp>

#if defined(_WIN32)
    // Undefine colliding macros from windows.h
    #undef DrawText
    #undef CloseWindow
    #undef ShowCursor
    #undef Rectangle
#endif

#include "raylib.h"
#include "lobby_server.hpp"  // For RelayHeader, RelayPacketType, RELAY_MAGIC
#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <chrono>
#include <cstdint>

struct PlayerData {
    int id;
    char name[32];
    Vector3 position;
    Vector3 camTarget;
    int hp;
    int money;
    bool firing;
    bool active;
    bool admin;
};

struct PlayerSyncData {
    int id;
    int hp;
    int money;
    int admin;
};

struct EnemySync {
    int id;
    Vector3 pos;
    int hp;
    int maxHp;
    int type;
    int weapon;
    float angle;
    bool isMoving;
    float walkTimer;
    float attackTimer;
};

class NetworkManager {
public:
    enum class Mode { NONE, SERVER, CLIENT };

    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port, const std::string& name = "LAN SERVER");
    bool StartServerAutoPort(int startPort, int maxTries, const std::string& name = "LAN SERVER");
    bool StartClient(const std::string& ip, int port);
    
    // Relay-based connections (no port forwarding needed)
    bool StartServerRelay(const std::string& name = "RELAY SERVER");
    bool StartClientRelay(uint32_t roomId);
    void SendRelayPing();
    
    void SetLocalPlayerName(const std::string& name);
    void SetLobbyServer(const std::string& host, int port);
    void StartDiscovery(); 
    void Update();
    void SendPlayerUpdate(int id, const char* name, Vector3 pos, Vector3 target, int hp, int money, bool firing);
    void SendWorldUpdate(float baseHP, int wave, const std::vector<EnemySync>& enemies, const std::vector<PlayerSyncData>& players);
    void SendDisconnect(int id);
    bool GrantAdmin(int id);
    bool KickPlayer(int id);

    struct ServerInfo {
        std::string ip;
        int port;
        std::string name;
        float lastSeen;
        int players = 0;
        int maxPlayers = 8;
        bool started = false;
        bool internet = false;
        uint32_t relayRoomId = 0;  // Non-zero = relay server
    };
    std::map<std::string, ServerInfo> discoveredServers;
    
    float currentPingMs = -1.0f;
    bool pinging = false;
    float lastPingTime = -1.0f;

    Mode mode;
    std::map<int, PlayerData> remotePlayers;
    std::map<int, EnemySync> syncedEnemies;
    float remoteBaseHP;
    int remoteWave;
    bool gameStarted;
    int localPlayerId;
    std::string serverName;
    bool shouldQuit;
    bool hostDisconnected = false;
    int activePort = 0;
    int hostPlayerId = -1;
    bool waitingForServerAccept = false;
    bool localPlayerAdmin = false;
    
    // Relay state
    bool useRelay = false;
    uint32_t relayRoomId = 0;
    uint32_t relaySlotId = 0;  // Our slot in the relay room

private:
    static constexpr uint32_t PROTOCOL_MAGIC = 0xA6A5DA55;
    enum class PacketType : uint32_t { CONNECT_REQUEST = 1, CONNECT_ACCEPT = 2 };

    struct ConnectRequest {
        uint32_t magic;
        uint32_t type;
        char name[32];
    };

    struct ConnectAccept {
        uint32_t magic;
        uint32_t type;
        int id;
        char serverName[32];
    };

    asio::io_context io_context;
    asio::ip::udp::socket* socket;
    asio::ip::udp::endpoint sender_endpoint;
    asio::ip::udp::endpoint server_target_endpoint;
    asio::ip::udp::socket* discovery_socket;
    asio::ip::udp::socket* ping_socket = nullptr;
    
    char recv_buffer[32768];  // Larger for relay header + game data
    char disc_buffer[1024];
    std::string localPlayerName;
    std::string lobbyHost;
    int lobbyPort;
    int nextClientId;
    std::chrono::steady_clock::time_point lastLobbyHeartbeat;
    std::chrono::steady_clock::time_point lastLobbyQuery;
    std::chrono::steady_clock::time_point lastConnectRequest;
    std::chrono::steady_clock::time_point lastRelayJoinRequest;
    asio::ip::udp::endpoint lobby_server_endpoint;
    bool lobby_server_available;
    bool waitingForRelayAccept = false;

    void HandlePacket(size_t bytes_recvd, const asio::ip::udp::endpoint& sender);
    void HandleDiscovery();
    void SendConnectRequest();
    void RegisterInternetLobby();
    void QueryInternetLobbies();
    void ConfigureLobbyServer();
    std::string SanitizeLobbyField(const std::string& value, size_t maxLen) const;
    
    // Relay helpers
    void SendViaRelay(const void* data, size_t len);
    void SendRelayLeave();
    
    std::map<int, asio::ip::udp::endpoint> known_clients; 
};

#endif
