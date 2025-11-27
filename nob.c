#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define EXAMPLE_DIR "examples/"
#define SRC_DIR "src/"
#define BUILD_DIR "build/"

// Platform selection
#if defined(__linux__) && defined(__x86_64__)
#define ARCH_DIR "linux_x86_64/"
#define ARCH "linux_x86_64"
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
#define ARCH_DIR "macos_aarch64/"
#define ARCH "macos_aarch64"
#else
#error "Unsupported platform"
#endif

#define LIB_DIR BUILD_DIR "coroutine_" ARCH "/"

#define STATIC_LIB_NAME "libcoroutine.a"

#ifdef __APPLE__
#define LINK_FLAGS "-lcoroutine"
#else
#define LINK_FLAGS "-l:" STATIC_LIB_NAME
#endif

#define WARNING_FLAGS(cmd) cmd_append(cmd, "-Wall", "-Wextra", "-Wpedantic");

Cmd cmd = {};

bool create_library(bool dbg) {

  const char *sources[] = {
      SRC_DIR "coroutine.c",
      SRC_DIR ARCH_DIR "asm.s",
      SRC_DIR ARCH_DIR "platform.c",
  };

  const char *objects[] = {
      BUILD_DIR "coroutine.o",
      BUILD_DIR "asm.o",
      BUILD_DIR "platform.o",
  };

  for (size_t i = 0; i < NOB_ARRAY_LEN(sources); i++) {
    cmd_append(&cmd, "cc");

    WARNING_FLAGS(&cmd);

    if (dbg)
      cmd_append(&cmd, "-g");
    else
      cmd_append(&cmd, "-O3");

    cmd_append(&cmd, "-I", SRC_DIR);
    cmd_append(&cmd, "-c", "-fPIC", sources[i], "-o", objects[i]);

    if (!cmd_run(&cmd))
      return false;
  }

  // Reset archive to avoid stale members (e.g., previous shared objects)
  cmd_append(&cmd, "rm", "-f", BUILD_DIR STATIC_LIB_NAME);
  if (!cmd_run(&cmd))
    return false;

  // Create static library
  cmd_append(&cmd, "ar", "rcs", BUILD_DIR STATIC_LIB_NAME, objects[0],
             objects[1], objects[2]);

  if (!cmd_run(&cmd))
    return false;

  // Setup the library for linking
  if (!mkdir_if_not_exists(LIB_DIR))
    return false;
  if (!mkdir_if_not_exists(LIB_DIR "lib"))
    return false;
  if (!mkdir_if_not_exists(LIB_DIR "include"))
    return false;

  if (!copy_file(BUILD_DIR STATIC_LIB_NAME, LIB_DIR "lib/" STATIC_LIB_NAME))
    return false;
  if (!copy_file(SRC_DIR "coroutine.h", LIB_DIR "include/coroutine.h"))
    return false;

  nob_log(NOB_INFO, "Created library at: %s", LIB_DIR);

  return true;
}

bool build_example(char *name, bool debug) {
  cmd_append(&cmd, "cc");
  WARNING_FLAGS(&cmd);

  if (debug)
    cmd_append(&cmd, "-g");
  else
    cmd_append(&cmd, "-O3");

  cmd_append(&cmd, "-I", LIB_DIR "include");
  String_Builder source_path = {};
  String_Builder exe_path = {};

  nob_sb_appendf(&source_path, EXAMPLE_DIR "%s.c", name);
  nob_sb_appendf(&exe_path, BUILD_DIR "%s", name);

  cmd_append(&cmd, source_path.items);
  cmd_append(&cmd, "-L", LIB_DIR "lib");
  cmd_append(&cmd, LINK_FLAGS);
  cmd_append(&cmd, "-o", exe_path.items);

  if (!cmd_run(&cmd))
    return false;

  nob_log(NOB_INFO, "Built example: %s", exe_path.items);

  return true;
}

void print_help() {
  nob_log(NOB_INFO, "Usage: build_tool [options] [example_name]");
  nob_log(NOB_INFO, "Options:");
  nob_log(NOB_INFO, "  -g, --debug       Build with debug symbols");
  nob_log(NOB_INFO, "  -r, --run         Run the example after building");
  nob_log(NOB_INFO, "  -h, --help        Show this help message");
  nob_log(NOB_INFO, "");
  nob_log(NOB_INFO,
          "If example_name is provided, the specified example will be built.");
}

struct args {
  bool debug;
  bool run;
  char *example_name;
};

bool parse_args(struct args *args, int argc, char **argv) {

#define check(ref) (strcmp(argv[i], ref) == 0)

  args->debug = false;
  args->run = false;
  args->example_name = NULL;

  for (int i = 1; i < argc; i++) {
    if (check("--debug") || check("-g")) {
      args->debug = true;
    } else if (check("--run") || check("-r")) {
      args->run = true;
    } else if (check("--help") || check("-h")) {
      print_help();
      exit(0);
    } else {
      args->example_name = argv[i];
    }
  }

  return true;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  struct args args = {};

  if (!parse_args(&args, argc, argv))
    return 1;

  if (!mkdir_if_not_exists(BUILD_DIR))
    return 1;
  if (!create_library(args.debug))
    return 1;

  if (args.example_name != NULL) {
    if (!build_example(args.example_name, args.debug))
      return 1;
    if (args.run) {
      String_Builder exe_path = {};
      nob_sb_appendf(&exe_path, BUILD_DIR "%s", args.example_name);
      cmd_append(&cmd, exe_path.items);
      if (!cmd_run(&cmd))
        return 1;
    }
  }
  return 0;
}
