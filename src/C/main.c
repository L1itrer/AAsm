#include "utility.h"
//#include "parser.h"
#include "lexer.h"

const char msg[] = "Ready? Set! GO!\x0A";
const u8 number[] = "1001";

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
		if (SV_CMP_CSTR(str, register_strings[i]) == 0)
			return (RegisterKind)i;
	}
	return REG_INVALID;
}

#define INSTR_ARGS1(first) (first)
#define INSTR_ARGS2(first, second) ((first) | (second << 8))
#define INSTR_ARGS3(first, second, third) ((first) | (second << 8) | (third << 16))
#define INSTR_ARGS4(first, second, third, fourth) INSTR_ARGS3(first, second, third) | (fourth << 24)

// kind, text, opcode, argc, args_packed
#define INSTRUCTIONS \
INSTR_DEF(INSTR__Mov, "mov", 0, 12, 0) \
INSTR_DEF(INSTR_MOV_RM8R8, "mov", 0x88, 2, INSTR_ARGS2(TYPE_RM8, TYPE_R8)) \
INSTR_DEF(INSTR_MOV_RM16R16, "mov", 0x89, 2, INSTR_ARGS2(TYPE_RM16, TYPE_R16)) \
INSTR_DEF(INSTR_MOV_RM32R32, "mov", 0x89, 2, INSTR_ARGS2(TYPE_RM32, TYPE_R32)) \
INSTR_DEF(INSTR_MOV_RM64R64, "mov", 0x89, 2, INSTR_ARGS2(TYPE_RM64, TYPE_R64)) \
INSTR_DEF(INSTR_MOV_R8RM8, "mov", 0x8A, 2, INSTR_ARGS2(TYPE_R8, TYPE_RM8)) \
INSTR_DEF(INSTR_MOV_R16RM16, "mov", 0x8B, 2, INSTR_ARGS2(TYPE_R16, TYPE_RM16)) \
INSTR_DEF(INSTR_MOV_R32RM32, "mov", 0x8B, 2, INSTR_ARGS2(TYPE_R32, TYPE_RM32)) \
INSTR_DEF(INSTR_MOV_R64RM64, "mov", 0x8B, 2, INSTR_ARGS2(TYPE_R64, TYPE_RM64)) \
INSTR_DEF(INTSR_MOV_RM8IMM8, "mov", 0xc6, 2, INSTR_ARGS2(TYPE_RM8, TYPE_IMM8)) \
INSTR_DEF(INSTR_MOV_RM16IMM16, "mov", 0xc7, 2, INSTR_ARGS2(TYPE_RM16, TYPE_IMM16)) \
INSTR_DEF(INSTR_MOV_RM32IMM32, "mov", 0xc7, 2, INSTR_ARGS2(TYPE_RM32, TYPE_IMM32)) \
INSTR_DEF(INSTR_MOV_RM64IMM32, "mov", 0xc7, 2, INSTR_ARGS2(TYPE_RM64, TYPE_IMM32)) \
INSTR_DEF(INSTR__Ret, "ret", 0, 2, 0) \
INSTR_DEF(INSTR_RET, "ret", 0xc3, 0, 0) \
INSTR_DEF(INSTR_RET_IMM16, "ret", 0xc2, 1, INSTR_ARGS1(TYPE_IMM16)) \
INSTR_DEF(INSTR__Syscall, "syscall", 0, 1, 0) \
INSTR_DEF(INSTR_SYSCALL, "syscall", 0x0f05, 0, 0) \

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
			tok = lexer_get_and_expect(&l, LEX_IDENTIFIER);
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
		//printf("Tok id: |%d|, word: |%.*s|, value: |%lu|\n", tok, l.identifier.length, l.identifier.pointer, l.value);
	}
    return 0;
}
