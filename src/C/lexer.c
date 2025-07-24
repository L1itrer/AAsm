#include "lexer.h"
#include "utility.h"

// TODO: Proper lexer error reporting
void lexer_init(Lexer *l, SV input_stream, String* storage)
{
	*l = (Lexer){
		.input_stream = input_stream,
		.string_storage = storage,
	};
}

static char get_character(Lexer* l)
{
	char c = l->input_stream.pointer[l->byte_offset];
	l->byte_offset += 1;
	return c;
}

static char peek_character(Lexer* l)
{
	if (l->byte_offset >= l->input_stream.length) return 0;
	return l->input_stream.pointer[l->byte_offset+1];
}

static bool is_space(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r'  || c == '\f';
}
static void skip_whitespace(Lexer* l)
{
	char c = 0;
	do
	{
		l->byte_offset += 1;
		c = l->input_stream.pointer[l->byte_offset];
	} while (is_space(c) && l->byte_offset < l->input_stream.length);
	l->byte_offset -= 1; // go back newline is a token
}

static void skip_until_newline(Lexer* l)
{
	char c = 0;
	do
	{
		l->byte_offset += 1;
		c = l->input_stream.pointer[l->byte_offset];
	} while (c != '\n' && l->byte_offset < l->input_stream.length);
}

Token lexer_get(Lexer* l)
{
	u64 flen = l->input_stream.length;
	for (;l->byte_offset < l->input_stream.length;)
	{
		u32 curr_offset = l->byte_offset;
		char c = get_character(l);
		if (is_space(c) && c != '\n')
		{
			continue;
		}
		if (c == '#')
		{	
			skip_until_newline(l);
			return LEX_COMMENT;
		}
		else if (c == ',')
		{
			return LEX_COMMA;
			// TODO: commas
		}
		else if (c == '\n')
		{
			l->current_line += 1;
			return LEX_LINE_FEED;
		}

//		while (!is_space(c))
//		{
//			if (l->byte_offset >= flen)
//			{
//				return LEX_EOF;
//			}
//			c = get_character(l);
//		}
		while (true) 
		{
			c = peek_character(l);
			if (c == 0) return LEX_EOF;
			l->byte_offset += 1;
			if (c == ',')
			{
				break;
			}
			if (is_space(c) || c == '\n') break;
		}
		SV word = (SV){
			.pointer = l->input_stream.pointer + curr_offset, 
			.length = l->byte_offset - curr_offset,
		};
		l->identifier = word;
		if (is_number(word.pointer[0]))
		{
			// TODO: introduce base from sv not from char
			Bases base = bases_from_char(word.pointer[1]);
			if (base == BASE_INVALID)
			{
				aasm_log(LOG_ERROR, "Invalid number base: %c at line %u", c, l->current_line);
				return LEX_PARSE_ERROR;
			}
			// HACK: 
			word.pointer += 2;
			word.length -= 2;
			i32 num = sv_to_i32(word, base);
			u32 unum = *(u32*)&num;
			l->value = unum;
			return LEX_INT_LIT;
		}
		else if (word.pointer[word.length-1] == ':')
		{
			// TODO: storing the labels
			return LEX_LABEL;
		}
		else
		{
			return LEX_IDENTIFIER;
		}
	}
	return LEX_EOF;
}

Token lexer_get_and_expect(Lexer *l, Token expected)
{
	Token tok = lexer_get(l);
	if (tok != expected)
	{
		aasm_log(LOG_ERROR, "Unexpected token %d at line %d\n", tok, l->current_line);
		return LEX_PARSE_ERROR;
	}
	return tok;
}



