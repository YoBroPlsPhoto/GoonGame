#include "core/game.hpp"
#include "network/lobby_server.hpp"
#include <string>

#ifdef _WIN32
#include <sys/stat.h>
extern "C" {
    int stat64i32(const char *path, struct _stat64i32 *buffer) {
        return _stat64i32(path, buffer);
    }
}
#endif

int main(int argc, char *argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--lobby-server") {
        int port = 1240;
        if (argc > 2) {
            try { port = std::stoi(argv[2]); } catch (...) {}
        }
        return RunLobbyServer(port);
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "AdasGooner - NYC Ultra Graphics");
    SetRandomSeed(time(NULL));
    InitAudioDevice();

    Game game;
    game.Init();
    game.Run();
    return 0;
}
