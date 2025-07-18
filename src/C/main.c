#include "utility.h"
//#include "parser.h"
#include "lexer.h"

const char msg[] = "Ready? Set! GO!\x0A";
const u8 number[] = "1001";


int main(int argc, const char** argv)
{
    write(1, msg, sizeof(msg) - 1);


    String content = {0};
    if (!string_read_file("./test/hello.asm", &content)) return 1;

    write(1, content.data, content.count);

    printf("\n\n");

//    parse_file(&content, &tokens);

	Lexer l = {0};
	String str_storage = {0};
	SV file_sv = (SV){.pointer = content.data, .length = content.count};
	lexer_init(&l, file_sv, &str_storage);
	
	while (true)
	{
		Token tok = lexer_get(&l);
		if (tok == LEX_EOF) break;
		printf("Tok id: |%d|, word: |%.*s|, value: |%lu|\n", tok, l.identifier.length, l.identifier.pointer, l.value);
	}
    return 0;
}
