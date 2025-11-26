#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define SRC_FOLDER "src/"
#define BUILD_FOLDER "build/"

// If linux x86_64, set app name to app_linux_x86_64
#if defined(__linux__) && defined(__x86_64__)
    #define ARCH_FOLDER "linux_x86_64/"
#else
    #error "Unsupported platform"
#endif // __linux__ && __x86_64__

#define APP_NAME "app"

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (!mkdir_if_not_exists(BUILD_FOLDER)) {
        return 1;
    }

    Cmd cmd = {};

    cmd_append(&cmd, "cc", "-Wall", "-Wextra");
    cmd_append(&cmd, "-g");

    cmd_append(&cmd, "-I", SRC_FOLDER);

    cmd_append(&cmd,
        SRC_FOLDER "main.c",
        SRC_FOLDER "coroutine.c",
        SRC_FOLDER ARCH_FOLDER "asm.s",
        SRC_FOLDER ARCH_FOLDER "platform.c",
    );


    cmd_append(&cmd, "-o", BUILD_FOLDER APP_NAME);

    if (!cmd_run(&cmd)) {
        return 1;
    }

    return 0;
}
