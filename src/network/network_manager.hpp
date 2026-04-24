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
#include <string>
#include <map>
#include <mutex>

struct PlayerData {
    int id;
    Vector3 position;
    Vector3 camTarget;
    int hp;
    int money;
    bool firing;
    bool active;
};

struct PlayerSyncData {
    int id;
    int hp;
    int money;
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

    bool StartServer(int port);
    bool StartClient(const std::string& ip, int port);
    void StartDiscovery(); 
    void Update();
    void SendPlayerUpdate(int id, Vector3 pos, Vector3 target, int hp, int money, bool firing);
    void SendWorldUpdate(float baseHP, int wave, const std::vector<EnemySync>& enemies, const std::vector<PlayerSyncData>& players);
    void SendDisconnect(int id);

    struct ServerInfo {
        std::string ip;
        int port;
        std::string name;
        float lastSeen;
    };
    std::map<std::string, ServerInfo> discoveredServers;

    Mode mode;
    std::map<int, PlayerData> remotePlayers;
    std::map<int, EnemySync> syncedEnemies;
    float remoteBaseHP;
    int remoteWave;
    bool gameStarted;
    int localPlayerId;

private:
    asio::io_context io_context;
    asio::ip::udp::socket* socket;
    asio::ip::udp::endpoint sender_endpoint;
    asio::ip::udp::endpoint server_target_endpoint;
    asio::ip::udp::socket* discovery_socket;
    
    char recv_buffer[4096]; // Increased for world state
    char disc_buffer[1024];

    void HandlePacket(size_t bytes_recvd, const asio::ip::udp::endpoint& sender);
    void HandleDiscovery();
    
    std::map<int, asio::ip::udp::endpoint> known_clients; 
};

#endif
