#include "network/network_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    NetworkManager net;
    if (!net.StartServer(1234)) {
        std::cerr << "Could not start server on port 1234" << std::endl;
        return 1;
    }

    std::cout << "====================================" << std::endl;
    std::cout << "   AdasGooner Headless Server       " << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "Listening on port 1234..." << std::endl;

    while (true) {
        net.Update();
        
        // Output status occasionally
        static auto lastLog = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLog).count() >= 5) {
            std::cout << "Server active. Connected players: " << net.remotePlayers.size() << std::endl;
            lastLog = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
