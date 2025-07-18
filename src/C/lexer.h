#ifndef LEXER_H
#define LEXER_H
#include "utility.h"

// TODO: Labels
// TODO: sections
// TODO: db, dw and such directives
typedef enum Token{
	LEX_EOF = 256,
	LEX_PARSE_ERROR,
	LEX_LINE_FEED,
	LEX_COMMENT,
	LEX_COMMA,
	LEX_INT_LIT,
	LEX_LABEL,
	LEX_INSTRUCTION,
}Token;

typedef struct Lexer{
	SV input_stream;
	u32 current_line;
	u32 line_character;
	u32 byte_offset;
	Token token;
	SV identifier;
	u64 value;

	String* string_storage;
}Lexer;

void lexer_init(Lexer* l, SV input_stream, String* storage);
Token lexer_get(Lexer* l);
Token lexer_get_and_expect(Lexer* l, Token expected);

#endif //LEXER_H
