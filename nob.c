#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_STRIP_PREFIX
#include "nob.h"

#define OUTPUT_DIR "./out"


int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    nob_mkdir_if_not_exists(OUTPUT_DIR);
    Cmd cmd = {0};
#ifdef WIN32
	Nob_Procs procs = {0};
	nob_cmd_append(&cmd, "cl", "-Wall", "-Zi", "-Fo:build\\", "-Fd:build\\", "-Fe:build\\aasm.exe", "-EHsc", "-D_CRT_SECURE_NO_WARNINGS", "src\\C\\aasm.c");
#else
    cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-o", OUTPUT_DIR"/aasm", "-ggdb", "./src/C/aasm.c");
#endif
    if (!cmd_run(&cmd)) return 1;
    return 0;
}
