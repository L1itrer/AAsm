#include "utility.h"

const char msg[] = "Ready? Set! GO!\x0A";
const u8 number[] = "1001";


int main(int argc, const char** argv)
{
    write(1, msg, sizeof(msg) - 1);


    SV str_number = (SV){.pointer = number, 4};
    i32 result = sv_to_i32(str_number, BIN);
    printf("%d\n", result);

    return 0;
}