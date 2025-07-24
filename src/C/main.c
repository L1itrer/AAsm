#include "utility.h"

const char msg[] = "Ready? Set! GO!\x0A";
const u8 number[] = "1001";



typedef enum Token{
	LEX_EOF,
	LEX_PARSE_ERROR,
	LEX_LINE_FEED,
	LEX_COMMENT,
	LEX_COMMA,
	LEX_INT_LIT,
	LEX_LABEL,
	LEX_IDENTIFIER,
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

// TODO: segment registers
typedef enum Type : u32{
	TYPE_INVALID,
	TYPE_R8,
	TYPE_R16,
	TYPE_R32,
	TYPE_R64,
	TYPE__ImmsBegin,
	TYPE_IMM8,
	TYPE_IMM16,
	TYPE_IMM32,
	TYPE_IMM64,
	TYPE__ImmsEnd,
	TYPE_YMM,
	TYPE_XMM,
	TYPE__RmsBegin,
	TYPE_RM8,
	TYPE_RM16,
	TYPE_RM32,
	TYPE_RM64,
	TYPE__RmsEnd,
	TYPE__Count,
}Type;

// enum, textual rep, encoding, type
#define REGISTERS \
REG_DEF(REG_INVALID, "", 0xff, TYPE_INVALID) \
REG_DEF(REG_AL, "al", 0b000, TYPE_R8) \
REG_DEF(REG_CL, "cl", 0b001, TYPE_R8) \
REG_DEF(REG_DL, "dl", 0b010, TYPE_R8) \
REG_DEF(REG_BL, "bl", 0b011, TYPE_R8) \
REG_DEF(REG_SPL, "spl", 0b100, TYPE_R8) \
REG_DEF(REG_BPL, "bpl", 0b101, TYPE_R8) \
REG_DEF(REG_SIL, "sil", 0b110, TYPE_R8) \
REG_DEF(REG_DIL, "dil", 0b111, TYPE_R8) \
\
REG_DEF(REG_AX, "ax", 0b000, TYPE_R16) \
REG_DEF(REG_CX, "cx", 0b001, TYPE_R16) \
REG_DEF(REG_DX, "dx", 0b010, TYPE_R16) \
REG_DEF(REG_BX, "bx", 0b011, TYPE_R16) \
REG_DEF(REG_SP, "sp", 0b100, TYPE_R16) \
REG_DEF(REG_BP, "bp", 0b101, TYPE_R16) \
REG_DEF(REG_SI, "si", 0b110, TYPE_R16) \
REG_DEF(REG_DI, "di", 0b111, TYPE_R16) \
\
REG_DEF(REG_EAX, "eax", 0b000, TYPE_R32) \
REG_DEF(REG_ECX, "ecx", 0b001, TYPE_R32) \
REG_DEF(REG_EDX, "edx", 0b010, TYPE_R32) \
REG_DEF(REG_EBX, "ebx", 0b011, TYPE_R32) \
REG_DEF(REG_ESP, "esp", 0b100, TYPE_R32) \
REG_DEF(REG_EBP, "ebp", 0b101, TYPE_R32) \
REG_DEF(REG_ESI, "esi", 0b110, TYPE_R32) \
REG_DEF(REG_EDI, "edi", 0b111, TYPE_R32) \
\
REG_DEF(REG_RAX, "rax", 0b000, TYPE_R64) \
REG_DEF(REG_RCX, "rcx", 0b001, TYPE_R64) \
REG_DEF(REG_RDX, "rdx", 0b010, TYPE_R64) \
REG_DEF(REG_RBX, "rbx", 0b011, TYPE_R64) \
REG_DEF(REG_RSP, "rsp", 0b100, TYPE_R64) \
REG_DEF(REG_RBP, "rbp", 0b101, TYPE_R64) \
REG_DEF(REG_RSI, "rsi", 0b110, TYPE_R64) \
REG_DEF(REG_RDI, "rdi", 0b111, TYPE_R64) \
\
REG_DEF(REG_XMM0, "xmm0", 0b000, TYPE_XMM) \
REG_DEF(REG_XMM1, "xmm1", 0b001, TYPE_XMM) \
REG_DEF(REG_XMM2, "xmm2", 0b010, TYPE_XMM) \
REG_DEF(REG_XMM3, "xmm3", 0b011, TYPE_XMM) \
REG_DEF(REG_XMM4, "xmm4", 0b100, TYPE_XMM) \
REG_DEF(REG_XMM5, "xmm5", 0b101, TYPE_XMM) \
REG_DEF(REG_XMM6, "xmm6", 0b110, TYPE_XMM) \
REG_DEF(REG_XMM7, "xmm7", 0b111, TYPE_XMM) \
\
REG_DEF(REG__RequiresREXR, "", 0xff, TYPE_INVALID) \
REG_DEF(REG_R8B, "r8b", 0b000, TYPE_R8) \
REG_DEF(REG_R9B, "r9b", 0b001, TYPE_R8) \
REG_DEF(REG_R10B, "r10b", 0b010, TYPE_R8) \
REG_DEF(REG_R11B, "r11b", 0b011, TYPE_R8) \
REG_DEF(REG_R12B, "r12b", 0b100, TYPE_R8) \
REG_DEF(REG_R13B, "r13b", 0b101, TYPE_R8) \
REG_DEF(REG_R14B, "r14b", 0b110, TYPE_R8) \
REG_DEF(REG_R15B, "r15b", 0b111, TYPE_R8) \
\
REG_DEF(REG_R8W,  "r8w", 0b000, TYPE_R16) \
REG_DEF(REG_R9W,  "r9w", 0b001, TYPE_R16) \
REG_DEF(REG_R10W, "r10w", 0b010, TYPE_R16) \
REG_DEF(REG_R11W, "r11w", 0b011, TYPE_R16) \
REG_DEF(REG_R12W, "r12w", 0b100, TYPE_R16) \
REG_DEF(REG_R13W, "r13w", 0b101, TYPE_R16) \
REG_DEF(REG_R14W, "r14w", 0b110, TYPE_R16) \
REG_DEF(REG_R15W, "r15w", 0b111, TYPE_R16) \
\
REG_DEF( REG_R8D,  "r8d", 0b000, TYPE_R32) \
REG_DEF( REG_R9D,  "r9d", 0b001, TYPE_R32) \
REG_DEF(REG_R10D, "r10d", 0b010, TYPE_R32) \
REG_DEF(REG_R11D, "r11d", 0b011, TYPE_R32) \
REG_DEF(REG_R12D, "r12d", 0b100, TYPE_R32) \
REG_DEF(REG_R13D, "r13d", 0b101, TYPE_R32) \
REG_DEF(REG_R14D, "r14d", 0b110, TYPE_R32) \
REG_DEF(REG_R15D, "r15d", 0b111, TYPE_R32) \
\
REG_DEF( REG_R8,  "r8", 0b000, TYPE_R64) \
REG_DEF( REG_R9,  "r9", 0b001, TYPE_R64) \
REG_DEF(REG_R10, "r10", 0b010, TYPE_R64) \
REG_DEF(REG_R11, "r11", 0b011, TYPE_R64) \
REG_DEF(REG_R12, "r12", 0b100, TYPE_R64) \
REG_DEF(REG_R13, "r13", 0b101, TYPE_R64) \
REG_DEF(REG_R14, "r14", 0b110, TYPE_R64) \
REG_DEF(REG_R15, "r15", 0b111, TYPE_R64) \
REG_DEF(REG__Count, "", 0xff, TYPE_INVALID) \


typedef enum RegisterKind {
	#define REG_DEF(kind, text, encode, type) kind,
	REGISTERS
	#undef REG_DEF
}RegisterKind;


const char* register_strings[] = {
	#define REG_DEF(kind, text, encode, type) text,
	REGISTERS
	#undef REG_DEF
};

const u8 register_encodings[] = {
	#define REG_DEF(kind, text, encode, type) encode,
	REGISTERS
	#undef REG_DEF
};

const Type register_types[] = {
	#define REG_DEF(kind, text, encode, type) type,
	REGISTERS
	#undef REG_DEF
};

RegisterKind register_kind_from_sv(SV str)
{
	for (i32 i = 0;i < REG__Count;++i)
	{
		if (sv_cmp_cstr(str, register_strings[i]) == 0)
			return (RegisterKind)i;
	}
	return REG_INVALID;
}

#define INSTR_ARGS1(first) (first)
#define INSTR_ARGS2(first, second) ((first) | (second << 8))
#define INSTR_ARGS3(first, second, third) ((first) | (second << 8) | (third << 16))
#define INSTR_ARGS4(first, second, third, fourth) INSTR_ARGS3(first, second, third) | (fourth << 24)


// these are extra opcode flags to be OR-ed
#define NO_EXTRA 0x00
#define HAS_IMM 0x02
#define IMM_8 0x00
#define IMM_16 0x04
#define IMM_32 0x08
#define IMM_64 0x0C
#define REG_IN_OPCODE 0x01


// kind, text, opcode, opcode extention, argc, args_packed
#define INSTRUCTIONS \
INSTR_DEF(INSTR__Invalid, "", 0, 0, 0, 0) \
INSTR_DEF(INSTR__Mov, "mov", 0, NO_EXTRA, 16, 0) \
INSTR_DEF(INSTR_MOV_RM8R8, "mov", 0x88, NO_EXTRA, 2, INSTR_ARGS2(TYPE_RM8, TYPE_R8)) \
INSTR_DEF(INSTR_MOV_RM16R16, "mov", 0x89, NO_EXTRA, 2, INSTR_ARGS2(TYPE_RM16, TYPE_R16)) \
INSTR_DEF(INSTR_MOV_RM32R32, "mov", 0x89, NO_EXTRA, 2, INSTR_ARGS2(TYPE_RM32, TYPE_R32)) \
INSTR_DEF(INSTR_MOV_RM64R64, "mov", 0x89, NO_EXTRA, 2, INSTR_ARGS2(TYPE_RM64, TYPE_R64)) \
INSTR_DEF(INSTR_MOV_R8RM8, "mov", 0x8A, NO_EXTRA, 2, INSTR_ARGS2(TYPE_R8, TYPE_RM8)) \
INSTR_DEF(INSTR_MOV_R16RM16, "mov", 0x8B, NO_EXTRA, 2, INSTR_ARGS2(TYPE_R16, TYPE_RM16)) \
INSTR_DEF(INSTR_MOV_R32RM32, "mov", 0x8B, NO_EXTRA, 2, INSTR_ARGS2(TYPE_R32, TYPE_RM32)) \
INSTR_DEF(INSTR_MOV_R64RM64, "mov", 0x8B, NO_EXTRA, 2, INSTR_ARGS2(TYPE_R64, TYPE_RM64)) \
INSTR_DEF(INSTR_MOV_R8IMM8, "mov", 0xB0, REG_IN_OPCODE | HAS_IMM | IMM_8, 2, INSTR_ARGS2(TYPE_R8, TYPE_IMM8)) \
INSTR_DEF(INSTR_MOV_R16IMM16, "mov", 0xB8, REG_IN_OPCODE | HAS_IMM | IMM_16, 2, INSTR_ARGS2(TYPE_R8, TYPE_IMM8)) \
INSTR_DEF(INSTR_MOV_R32IMM32, "mov", 0xB8, REG_IN_OPCODE | HAS_IMM | IMM_32, 2, INSTR_ARGS2(TYPE_R8, TYPE_IMM8)) \
INSTR_DEF(INSTR_MOV_RM64IMM32, "mov", 0xc7, HAS_IMM | IMM_32, 2, INSTR_ARGS2(TYPE_RM64, TYPE_IMM32)) \
INSTR_DEF(INSTR_MOV_R64IMM64, "mov", 0xB8, REG_IN_OPCODE | HAS_IMM | IMM_64, 2, INSTR_ARGS2(TYPE_R8, TYPE_IMM8)) \
INSTR_DEF(INTSR_MOV_RM8IMM8, "mov", 0xc6, HAS_IMM | IMM_8, 2, INSTR_ARGS2(TYPE_RM8, TYPE_IMM8)) \
INSTR_DEF(INSTR_MOV_RM16IMM16, "mov", 0xc7, HAS_IMM | IMM_16, 2, INSTR_ARGS2(TYPE_RM16, TYPE_IMM16)) \
INSTR_DEF(INSTR_MOV_RM32IMM32, "mov", 0xc7,  HAS_IMM | IMM_32, 2, INSTR_ARGS2(TYPE_RM32, TYPE_IMM32)) \
INSTR_DEF(INSTR__Ret, "ret", 0, NO_EXTRA, 2, 0) \
INSTR_DEF(INSTR_RET, "ret", 0xc3, NO_EXTRA, 0, 0) \
INSTR_DEF(INSTR_RET_IMM16, "ret", 0xc2, HAS_IMM | IMM_16, 1, INSTR_ARGS1(TYPE_IMM16)) \
INSTR_DEF(INSTR__Syscall, "syscall", 0, NO_EXTRA, 1, 0) \
INSTR_DEF(INSTR_SYSCALL, "syscall", 0x0f05, NO_EXTRA, 0, 0) \
INSTR_DEF(INSTR__Count, "", 0, 0, 0, 0) \

typedef enum InstructionKind {
	#define INSTR_DEF(kind, t, o, ox, argc, args) kind,
	INSTRUCTIONS
	#undef INSTR_DEF
}InstructionKind;

const char* instr_text[] = {
	#define INSTR_DEF(k, text, o, ox, argc, args) text, 
	INSTRUCTIONS
	#undef INSTR_DEF
};

const u32 instr_opcode[] = {
	#define INSTR_DEF(k, t, opcode, ox, argc, args) opcode, 
	INSTRUCTIONS
	#undef INSTR_DEF
};

const u32 instr_opcextra[] = {
	#define INSTR_DEF(k, t, o, opcode_extra, argc, args) opcode_extra,
	INSTRUCTIONS
	#undef INSTR_DEF
};

const i32 instr_argc[] = {
	#define INSTR_DEF(k, t, o, ox, argc, args) argc,
	INSTRUCTIONS
	#undef INSTR_DEF 
};

const u32 instr_args[] = {
	#define INSTR_DEF(k, t, o, ox, argc, args) args,
	INSTRUCTIONS
	#undef INSTR_DEF
};


InstructionKind instr_get_from_sv(SV sv)
{
	for (i32 i = 1;i < INSTR__Count;i += 1)
	{
		if (sv_cmp_cstr(sv, instr_text[i]) == 0)
		{
			return (InstructionKind)i;
		}
	}
	return INSTR__Invalid;
}

int main(int argc, const char** argv)
{
    String content = {0};
    if (!string_read_file("./test/hello.asm", &content)) return 1;

    printf("%.*s",content.count, content.data);

    printf("\n\n");

//    parse_file(&content, &tokens);

	Lexer l = {0};
	String str_storage = {0};
	SV file_sv = (SV){.pointer = content.data, .length = content.count};
	lexer_init(&l, file_sv, &str_storage);
	
	Token tok = LEX_PARSE_ERROR;
	while (true)
	{
		Token tok = lexer_get(&l);
		if (tok == LEX_EOF) break;
		if (tok == LEX_LINE_FEED)
		{
			// TODO: handling of labels
			tok = lexer_get(&l);
			if (tok != LEX_IDENTIFIER && tok != LEX_COMMENT)
			{
				if (tok != LEX_COMMENT)
				{
					printf("A line needs to begin with an instruction\n");
					continue;
				}
			}
			InstructionKind instr = instr_get_from_sv(l.identifier);
			if (instr == INSTR__Invalid)
			{
				aasm_log(LOG_ERROR, "Oopsy doopsie! you did a fucky wacky! Invalid instruction: %.*s\n at line ", l.identifier.length, l.identifier.pointer, l.current_line);
				// TODO: Advance to next line on error
				continue;
			}
			assert(instr_opcode[instr] == 0);
			printf("Instruction: %s, sub_instr count: %d\n", instr_text[instr], instr_argc[instr]);
			continue;
			// TODO: Special handling of prefixes
		}
		if (tok == LEX_IDENTIFIER)
		{
			RegisterKind reg = register_kind_from_sv(l.identifier);
			if (reg != REG_INVALID)
			{
				printf("Register: %s, encoding: %d\n", register_strings[reg], register_encodings[reg]);
			}
		}
		if (tok == LEX_INT_LIT)
		{
			printf("Int literal: %d\n", l.value);
		}
		//printf("Tok id: |%d|, word: |%.*s|, value: |%lu|\n", tok, l.identifier.length, l.identifier.pointer, l.value);
	}
    return 0;
}
