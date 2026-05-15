#include "core/game.hpp"

#ifdef _WIN32
#include <sys/stat.h>
extern "C" {
    int stat64i32(const char *path, struct _stat64i32 *buffer) {
        return _stat64i32(path, buffer);
    }
}
#endif

int main(int argc, char *argv[]) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "AdasGooner - NYC Ultra Graphics");
    SetRandomSeed(time(NULL));
    InitAudioDevice();

    Game game;
    game.Init();
    game.Run();
    return 0;
}
