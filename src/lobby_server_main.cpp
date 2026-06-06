#include "network/lobby_server.hpp"
#include <string>

int main(int argc, char** argv) {
    int port = 1240;
    if (argc > 1) {
        try { port = std::stoi(argv[1]); } catch (...) {}
    }

    return RunLobbyServer(port);
}
