#include "network/network_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <queue>
#include <mutex>
#include <sstream>

#ifdef _WIN32
#include <sys/stat.h>
extern "C" {
    int stat64i32(const char *path, struct _stat64i32 *buffer) {
        return _stat64i32(path, buffer);
    }
}
#endif

int main() {
    NetworkManager net;

    const int BASE_PORT = 1234;
    const int MAX_TRIES = 10;

    if (!net.StartServerAutoPort(BASE_PORT, MAX_TRIES, "DEDICATED SERVER")) {
        std::cerr << "Could not start server on ports "
                  << BASE_PORT << "-" << (BASE_PORT + MAX_TRIES - 1) << std::endl;
        return 1;
    }

    std::cout << "====================================" << std::endl;
    std::cout << "   AdasGooner Headless Server       " << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Listening on port " << net.activePort << "..." << std::endl;
    std::cout << "Commands: players | admin <id> | kick <id> | help | quit" << std::endl;

    std::queue<std::string> commands;
    std::mutex commandsMutex;
    bool inputRunning = true;
    std::thread inputThread([&]() {
        std::string line;
        while (inputRunning && std::getline(std::cin, line)) {
            std::lock_guard<std::mutex> lock(commandsMutex);
            commands.push(line);
        }
    });

    while (!net.shouldQuit) {
        net.Update();

        {
            std::lock_guard<std::mutex> lock(commandsMutex);
            while (!commands.empty()) {
                std::string line = commands.front();
                commands.pop();
                std::stringstream ss(line);
                std::string cmd;
                ss >> cmd;

                if (cmd == "players") {
                    std::cout << "Players:" << std::endl;
                    std::cout << "  ID 0 | HOST | admin=yes" << std::endl;
                    for (const auto& [id, p] : net.remotePlayers) {
                        if (!p.active) continue;
                        std::cout << "  ID " << id << " | " << (p.name[0] ? p.name : "Gooner")
                                  << " | hp=" << p.hp
                                  << " | money=" << p.money
                                  << " | admin=" << (p.admin ? "yes" : "no") << std::endl;
                    }
                } else if (cmd == "admin") {
                    int id = -1;
                    ss >> id;
                    if (net.GrantAdmin(id)) {
                        std::cout << "Granted admin to player " << id << std::endl;
                    } else {
                        std::cout << "Could not grant admin to player " << id << std::endl;
                    }
                } else if (cmd == "kick") {
                    int id = -1;
                    ss >> id;
                    if (net.KickPlayer(id)) {
                        std::cout << "Kicked player " << id << std::endl;
                    } else {
                        std::cout << "Could not kick player " << id << std::endl;
                    }
                } else if (cmd == "help") {
                    std::cout << "Commands: players | admin <id> | kick <id> | help | quit" << std::endl;
                } else if (cmd == "quit" || cmd == "exit") {
                    net.shouldQuit = true;
                } else if (!cmd.empty()) {
                    std::cout << "Unknown command. Type: help" << std::endl;
                }
            }
        }
        
        // Output status occasionally
        static auto lastLog = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() >= 5) {
            std::cout << "Server active. Connected players: " << net.remotePlayers.size() << std::endl;
            lastLog = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Server shutting down (Host disconnected or quit signal)." << std::endl;
    inputRunning = false;
    if (inputThread.joinable()) inputThread.detach();
    return 0;
}
