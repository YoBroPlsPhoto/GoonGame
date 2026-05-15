#include "network/network_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>

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

    while (!net.shouldQuit) {
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

    std::cout << "Server shutting down (Host disconnected or quit signal)." << std::endl;
    return 0;
}
