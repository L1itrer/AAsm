#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    nob_cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-o", "./build/aasm", "./src/C/main.c");
    nob_cmd_append(&cmd, "-ggdb");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}