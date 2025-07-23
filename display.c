#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char** argv)
{
	NOB_GO_REBUILD_URSELF(argc, argv);
	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "fasm", "instruction.asm");
	if (!nob_cmd_run_sync(cmd)) return 1;
	Nob_String_Builder sb = {0};
	if (!nob_read_entire_file(".\\instruction.asm", &sb)) return 1;
	int subtracted = 0;
	for (int i = 0;i < sb.count;++i)
	{
		if (sb.items[i] == '\n')
		{
			printf("%.*s", sb.count - i, sb.items + i);
			break;
		}
	}
	sb.count = 0;
	nob_read_entire_file(".\\instruction.bin", &sb);
	for (int i = 0;i < sb.count;++i)
	{
		printf("%02x ", sb.items[i] & 0xff);
	}
	printf("\n");
	return 0;
}
