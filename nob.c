#define NOB_IMPLEMENTATION
#define NOB_EXPERIMENTAL_DELETE_OLD
#define NOB_STRIP_PREFIX
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    nob_mkdir_if_not_exists("./build");
    Cmd cmd = {0};
    Nob_File_Paths file_paths = {0};
    if (!nob_read_entire_dir("./src/C", &file_paths)) return 1;
    nob_cmd_append(&cmd, "gcc", "-Wall", "-Wextra", "-o", "./build/aasm", "-ggdb");
    for (size_t i = 0;i < file_paths.count;++i)
    {
        const char* curr_file = file_paths.items[i];
        if (strcmp(".", curr_file) == 0 || strcmp("..", curr_file) == 0) continue;
        nob_cmd_append(&cmd, temp_sprintf("./src/C/%s", curr_file));
    }

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}