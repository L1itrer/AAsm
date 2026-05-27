#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_STRIP_PREFIX
#include "nob.h"

#define OUTPUT_DIR "./out"

void cflags(Cmd* cmd)
{
    cmd_append(cmd, "-Wall", "-Wextra");
    cmd_append(cmd, "-Werror=implicit-fallthrough");
}

void debug_flags(Cmd* cmd)
{
    cmd_append(cmd, "-g");
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    nob_mkdir_if_not_exists(OUTPUT_DIR);
    Cmd cmdStack = {0};
    Cmd* cmd = &cmdStack;
    cmd_append(cmd, "gcc");
    cmd_append(cmd, "-o", temp_sprintf("%s/aasm", OUTPUT_DIR));
    cflags(cmd);
    debug_flags(cmd);
    cmd_append(cmd, "./src/C/aasm.c");
    if (!cmd_run(cmd)) return 1;
    return 0;
}
