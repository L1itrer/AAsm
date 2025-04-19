#include "utility.h"
#include "parser.h"

const char msg[] = "Ready? Set! GO!\x0A";
const u8 number[] = "1001";


int main(int argc, const char** argv)
{
    write(1, msg, sizeof(msg) - 1);


    String content = {0};
    Tokens tokens = {0};
    if (!string_read_file("./test/test.asm", &content)) return 1;

    write(1, content.data, content.count);

    printf("\n\n");

    parse_file(&content, &tokens);

    return 0;
}