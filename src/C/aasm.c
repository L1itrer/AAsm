/* TODO:
 * Lexer:
 * - return specific registers instrucitons etc from the lexer rather than just IDENT
 * - allow to get more than one token (opt)
 * Memory:
 * - arenas
 * - hash tables (will be usefull for labels)
 * - linked lists (opt)
 * Labels:
 * - record labels
 * - jump instructions
 * Field addressing:
 * - yes
 * Raw byte declarations:
 * - yes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef _WIN32
#else
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include <errno.h>
#include <assert.h>

// ---------------------------------------------------------
// -----------------------UTILITY---------------------------
#ifndef stdout
#define stdout 1
#define stderr 2
#endif //stdout

#define UNREACHABLE(msg) do { fprintf(stderr, "%s:%d UNREACHABLE: %s at", __FILE__, __LINE__, msg); abort(); } while(0)
typedef unsigned char byte;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define local_persist static
#define global static

#define DA_INIT_CAP 1024*8

typedef struct String
{
    char* items;
    u64 count;
    u64 capacity;
} String;

typedef struct StringView
{
    char* ptr;
    u64 len;
} StringView;

typedef StringView SV;

#define SV_PRINT(sv) (int)(sv).len, (sv).ptr

bool string_read_file(const char* path, String* str);


//TODO: To rewrite this thing in itself I need my own implementations of libc functions
// memory related functions:

void* aasm_memset(void* buffer, i32 value, u64 count);
void* aasm_memcpy(void* dst, void* src, u64 count);
void* aasm_malloc(u64 count);
void* aasm_realloc(void* buffer, u64 count);
#define aasm_append(da, item)                                                          \
    do {                                                                                 \
        if ((da)->count >= (da)->capacity) {                                             \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = aasm_realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Buy more RAM lol");                       \
        }                                                                                \
                                                                                         \
        (da)->items[(da)->count++] = (item);                                             \
    } while (0)


// standard output

typedef enum AasmLogLevel
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
}AasmLogLevel;

void aasm_log(AasmLogLevel level, const char* format, ...);

// number conversion

typedef enum Bases{
    BASE_INVALID,
    BASE_BIN = 2,
    BASE_OCT = 8,
    BASE_DEC = 10,
    BASE_HEX = 16,
}Bases;

i32 sv_to_i32(SV slice, Bases base);
u64 string_to_u64(SV slice, Bases base);
bool i32_to_string(String* str, i32 number, Bases base);
bool is_number(u8 c);
Bases bases_from_char(char c);
i32 sv_cmp(SV a, SV b);

i32 sv_cmp_cstr(SV a, const char* b);



bool string_read_file(const char* path, String* str)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) goto failure;
    if (fseek(file, 0, SEEK_END) == -1) goto failure;
    long file_size = ftell(file);
    if (file_size < 0) goto failure;
    if (fseek(file, 0, SEEK_SET) < 0) goto failure;

    u64 new_count = str->count + file_size;
    if (new_count > str->capacity)
    {
        str->capacity = new_count;
        str->items = aasm_realloc(str->items, str->capacity);
    }
    fread(str->items + str->count, file_size, 1, file);
    if (ferror(file)) goto failure;
    str->count = new_count;
    fclose(file);
    return true;

failure:
    aasm_log(LOG_ERROR, "Could not read file %s: %s", path, strerror(errno));
    fclose(file);
    return false;
}


// memory related functions:

void* aasm_memset(void* buffer, i32 value, u64 count)
{
    return memset(buffer, value, count);
}

void* aasm_memcpy(void* dst, void* src, u64 count)
{
    return memcpy(dst, src, count);
}

void* aasm_malloc(u64 count)
{
    return malloc(count);
}

void* aasm_realloc(void* buffer, u64 count)
{
    return realloc(buffer, count);
}

// standard output

void aasm_log(AasmLogLevel level, const char* format, ...)
{
    switch(level)
    {
        case LOG_INFO:
            fprintf(stderr, "[INFO]: ");
            break;
        case LOG_WARNING:
            fprintf(stderr, "[WARNING]: ");
            break;
        case LOG_ERROR:
            fprintf(stderr, "[ERROR]: ");
            break;
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

static bool is_literal_character_valid(unsigned char c, i32 base)
{
    if (base != BASE_HEX)
    {
        return c >= '0' && c <= '9' && (c - '0' < base);
    }
    if (!(c >= '0' && c <= '9'))
    {
        if (!((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

#define CONVERT_ONE_CHAR(c, result, base) do { \
    result *= base; \
    if (c >= 'A' && c <= 'F') result += c - 55; \
    else if (c >= 'a' && c <= 'f') result += c - 87; \
    else result += c - '0';                       \
    } while(0)

bool is_number(u8 c)
{
    return c >= '0' && c <= '9';
}

Bases bases_from_char(char c)
{
    if (c == 'x') return BASE_HEX;
    if (c == 'o') return BASE_OCT;
    if (c == 'b') return BASE_BIN;
    if (c == 'd' || is_number(c)) return BASE_DEC;
    return BASE_INVALID;
}

i32 sv_to_i32(SV slice, Bases base)
{
    i32 result = 0;
    i32 multiply_by = (i32)base;
    assert((base == BASE_DEC) || (base == BASE_BIN) || (base == BASE_OCT) || (base == BASE_HEX));

    for (u64 i = 0; i < slice.len;i += 1)
    {
        u8 c = slice.ptr[i];
        if (c == '_') continue;
        assert(is_literal_character_valid(c, multiply_by));
        CONVERT_ONE_CHAR(c, result, multiply_by);
    }
    return result;
}

i32 sv_cmp(SV str1, SV str2)
{
    // NOTE: some size considerations are necessery
    if (str1.len != str2.len) return (i32)((i32)str1.len - (i32)str2.len);
    for (u64 i = 0;i < str1.len;++i)
    {
        if (str1.ptr[i] != str2.ptr[i])
            return str1.ptr[i] - str2.ptr[i];
    }
    return 0;
}


i32 sv_cmp_cstr(SV a, const char* b)
{
    size_t blen = strlen(b);
    if (a.len != blen) return (i32)((i32)a.len - (i32)blen);
    for (u64 i = 0;i < a.len;++i)
    {
        if (a.ptr[i] != b[i])
            return a.ptr[i] - b[i];
    }
    return 0;
}




// -------------------------------------------------------------
// -----------------------REGISTERS-----------------------------
// -------------------------------------------------------------
// TODO: segment registers
typedef enum Type{
    TYPE_INVALID,
    TYPE__GprsBegin,
    TYPE_R8,
    TYPE_R16,
    TYPE_R32,
    TYPE_R64,
    TYPE__GprsEnd,
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


bool type_is_imm(Type type)
{
    return type > TYPE__ImmsBegin && type < TYPE__ImmsEnd;
}

bool type_is_gpr(Type type)
{
    return type > TYPE__GprsBegin && type < TYPE__GprsEnd;
}

bool type_is_rm(Type type)
{
    return type > TYPE__RmsBegin && type < TYPE__RmsEnd;
}


// enum, textual rep, encoding, type
#define REGISTERS \
REG_DEF(REG__Invaild, "", 0xff, TYPE_INVALID) \
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
    return REG__Invaild;
}

// -------------------------------------------------------------------
// ------------------------INSTRUCTIONS-------------------------------
// -------------------------------------------------------------------

#define INSTR_ARGS1(first) (first)
#define INSTR_ARGS2(first, second) ((first) | (second << 8))
#define INSTR_ARGS3(first, second, third) ((first) | (second << 8) | (third << 16))
#define INSTR_ARGS4(first, second, third, fourth) INSTR_ARGS3(first, second, third) | (fourth << 24)

#define INSTR_GET_ARGS(pt) Type args[4] = {0}; args[0] = pt & 0xff; args[1] = (pt >> 8) & 0xff;\
args[2] = (pt >> 16) & 0xff; args[3] = pt >> 24;


// these are extra opcode flags to be OR-ed
#define NO_EXTRA 0x00
#define HAS_IMM 0x04
#define IMM_8 0x00
#define IMM_16 0x01
#define IMM_32 0x02
#define IMM_64 0x03
#define REG_IN_OPCODE 0x08


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
INSTR_DEF(INSTR_MOV_R16IMM16, "mov", 0xB8, REG_IN_OPCODE | HAS_IMM | IMM_16, 2, INSTR_ARGS2(TYPE_R16, TYPE_IMM16)) \
INSTR_DEF(INSTR_MOV_R32IMM32, "mov", 0xB8, REG_IN_OPCODE | HAS_IMM | IMM_32, 2, INSTR_ARGS2(TYPE_R32, TYPE_IMM32)) \
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


InstructionKind instr_match_types(InstructionKind instr_group, i32 argc, Type types[4])
{
    for (u32 i = instr_group+1;i < (instr_argc[instr_group] + instr_group+1);++i)
    {
        if (instr_argc[i] != argc) continue;
        INSTR_GET_ARGS(instr_args[i]);
        bool match = true;
        for (i32 j = 0;j < argc;++j)
        {
            if (type_is_imm(types[j]))
            {
                // a smaller imm can be promoted to a bigger one, but not the other way around
                if (!(args[j] >= types[j] && args[j] < TYPE__ImmsEnd))
                {
                    match = false;
                    break;
                }
            }
            else if (type_is_gpr(types[j]))
            {
                if (type_is_gpr(args[j]))
                {
                    if (args[j] != types[j])
                    {
                        match = false;
                        break;
                    }
                }
                else if (type_is_rm(args[j]))
                {
                    if ((args[j] - TYPE__RmsBegin) != (types[j] - TYPE__GprsBegin))
                    {
                        match = false;
                        break;
                    }
                }
                else
                {
                    match = false;
                    break;
                }
            }
        }
        if (match) return (InstructionKind)i;
    }
    return INSTR__Invalid;
}
#define REX_DEFAULT 0x40
#define REX_W 0b00001000
#define REX_R 0b00000100
#define REX_X 0b00000010
#define REX_B 0b00000001

int ipow(int base, int exp)
{
    int result = 1;
    for (;;)
    {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return result;
}

static void write_imm(String* code, i64 imm, i32 byte_count)
{
    for (i32 i = 0;i < byte_count;++i)
    {
        aasm_append(code, (u8)(imm & 0xff));
        imm >>= 8;
    }
      //    u8 bytes[24] = {0};
//    i32 count = 0;
//    while (imm != 0)
//    {
//        bytes[count] = (imm & 0xff);
//        imm >>= 8;
//        count += 1;
//    }
//    for (int i = 0;i < byte_count;++i)
//    {
//        aasm_append(code, bytes[count-i-1]);
//    }
}

static void write_opcode(String* code, u32 opcode)
{
    u8 bytes[8] = {0};
    i32 count = 0;
    while (opcode != 0)
    {
        bytes[count] = (opcode & 0xff);
        opcode >>= 8;
        count += 1;
    }
    for (i32 i = count-1;i >= 0;--i)
    {
        aasm_append(code, bytes[i]);
    }
}

bool instr_assemble(String* code, InstructionKind instruction, Type given_args[4], i32 imms[4], RegisterKind regs[4])
{
    INSTR_GET_ARGS(instr_args[instruction]);
    i32 argc = instr_argc[instruction];
    u8 rex_prefix = REX_DEFAULT;
    bool bit16_prefix = false;
    u32 opcode = instr_opcode[instruction];
    i64 imm = 0;
    // TODO: add other modes to mod_rm, there is no handling of memory operands so far so it's
    // impossible to infer them, for now it's hardcoded as a direct register-register addressing
    // u8 sib = 0;
    u8 mod_rm = 0b11000000;
    bool write_modrm = false;
    for (i32 i = 0;i < argc;++i)
    {
        switch (args[i])
        {
        case TYPE_IMM8:
        case TYPE_IMM16:
        case TYPE_IMM32:
        case TYPE_IMM64:
            imm = imms[i];
            break;
        case TYPE_RM64:
            rex_prefix |= REX_W;
            if (regs[i] > REG__RequiresREXR) rex_prefix |= REX_B;
            goto type_rm32;
        case TYPE_RM16:
            bit16_prefix = true;
            [[fallthrough]];
        case TYPE_RM8:
        case TYPE_RM32:
            type_rm32:
            {
                if (type_is_gpr(given_args[i]))
                {
                    u8 reg_code = register_encodings[regs[i]];
                    mod_rm |= reg_code;
                }
                else assert(false && "memory not yet implemented");
                write_modrm = true;
            }
            break;
        case TYPE_R64:
            rex_prefix |= REX_W;
            if (regs[i] > REG__RequiresREXR) rex_prefix |= REX_R;
            goto type_r32;
        case TYPE_R16:
            bit16_prefix = true;
            [[fallthrough]];
        case TYPE_R8:
        case TYPE_R32:
            type_r32:
            {
                u8 reg_code = register_encodings[regs[i]];
                if (instr_opcextra[instruction] & REG_IN_OPCODE)
                {
                    opcode |= reg_code;
                    break;
                }
                mod_rm |= (reg_code << 3);
                write_modrm = true;
            }
            break;
        default:
            assert(false && "not yet implemented type");
        }
    }
    if (bit16_prefix && (rex_prefix != REX_DEFAULT))
    {
        assert(false && "Somehow the instruction is both 16 bit and 64 bit");
    }
    if (bit16_prefix) aasm_append(code, 0x66);
    if (rex_prefix != REX_DEFAULT) aasm_append(code, rex_prefix);
    write_opcode(code, opcode);
    if (write_modrm) aasm_append(code, mod_rm);
    // TODO:: here be sib
    // TODO:: here be disp
    if (instr_opcextra[instruction] & HAS_IMM)
    {
        write_imm(code, imm, ipow(2, (instr_opcextra[instruction] & 0x03)));
    }
    return true;
}

// ---------------------------------------------------------
// ------------------------LEXER----------------------------
// ---------------------------------------------------------

// token, printable string, keyword string (if applicable), char (if applicable)
#define TOKENS \
TOKEN_DEF(LEX_EOF, "eof", "", ' ') \
TOKEN_DEF(LEX_PARSE_ERROR, "err", "", ' ') \
TOKEN_DEF(LEX_LINE_FEED, "newline", "", ' ') \
TOKEN_DEF(LEX_COMMENT, "comment", "", '#') \
TOKEN_DEF(LEX_COMMA, ",", "", ',') \
TOKEN_DEF(LEX_INT_LIT, "int", "", ' ' ) \
TOKEN_DEF(LEX_LABELDEF, "label", "", ' ') \
TOKEN_DEF(LEX_IDENTIFIER, "id", "", ' ') \
TOKEN_DEF(LEX_REGISTER, "regster", "", ' ') \
TOKEN_DEF(LEX_INSTRUCTION, "instruction", "", ' ') \

typedef enum Token{
    #define TOKEN_DEF(tok, prt, str,char) tok,
    TOKENS
    #undef TOKEN_DEF
}Token;

const char* token_printable[] = {
    #define TOKEN_DEF(tok, print, str, char) print,
    TOKENS
    #undef TOKEN_DEF
};

const char token_chars[] = {
    #define TOKEN_DEF(t, prt, s, char) char,
    TOKENS
    #undef TOKEN_DEF
};


typedef struct Lexer{
    SV input_stream;
    u32 current_line;
    u32 line_character;
    u32 byte_offset;
    Token token;

    RegisterKind reg;
    InstructionKind instr;
    SV identifier;
    i64 value;

    String* string_storage;
}Lexer;

void token_print(Lexer* l, Token tok)
{
    printf("Token: |%s|", token_printable[tok]);
    if (token_chars[tok] != ' ')
    {
        printf(", %c", token_chars[tok]);
    }
    if (tok == LEX_REGISTER)
    {
        printf(", |%s|", register_strings[l->reg]);
    }
    else if (tok == LEX_INSTRUCTION)
    {
        printf(", |%s|", instr_text[l->instr]);
    }
    else if (tok == LEX_IDENTIFIER)
    {
        printf(", |%.*s|", (int)l->identifier.len, l->identifier.ptr);
    }
    else if (tok == LEX_INT_LIT)
    {
        printf(", |%ld|", l->value);
    }
    printf("\n");
}

void lexer_init(Lexer *l, SV input_stream, String* storage)
{
    *l = (Lexer){
        .input_stream = input_stream,
        .string_storage = storage,
    };
}

static char get_character(Lexer* l)
{
    char c = l->input_stream.ptr[l->byte_offset];
    l->byte_offset += 1;
    return c;
}

static char peek_character(Lexer* l)
{
    if (l->byte_offset >= l->input_stream.len) return 0;
    return l->input_stream.ptr[l->byte_offset];
}

static bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r'  || c == '\f';
}
// static void skip_whitespace(Lexer* l)
// {
//     char c = 0;
//     do
//     {
//         l->byte_offset += 1;
//         c = l->input_stream.ptr[l->byte_offset];
//     } while (is_space(c) && l->byte_offset < l->input_stream.len);
//     l->byte_offset -= 1; // go back newline is a token
// }

static void skip_until_newline(Lexer* l)
{
    char c = 0;
    do
    {
        l->byte_offset += 1;
        c = l->input_stream.ptr[l->byte_offset];
    } while (c != '\n' && l->byte_offset < l->input_stream.len);
}

Token lexer_get(Lexer* l)
{
    // u64 flen = l->input_stream.len;
    for (;l->byte_offset < l->input_stream.len;)
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

//        while (!is_space(c))
//        {
//            if (l->byte_offset >= flen)
//            {
//                return LEX_EOF;
//            }
//            c = get_character(l);
//        }
        while (true) 
        {
            c = peek_character(l);
            if (c == 0) return LEX_EOF;
            if (c == ',')
            {
                break;
            }
            if (is_space(c) || c == '\n') break;
            get_character(l);
        }
        SV word = (SV){
            .ptr = l->input_stream.ptr + curr_offset, 
            .len = l->byte_offset - curr_offset,
        };
        l->identifier = word;
        if (is_number(word.ptr[0]))
        {
            // TODO: introduce base from sv not from char
            Bases base = BASE_INVALID;
            //printf("word: %.*s, len %zu\n", SV_PRINT(word), word.len);
            if (word.len > 1)
            {
                base = bases_from_char(word.ptr[1]);
                if (base == BASE_INVALID)
                {
                    aasm_log(LOG_ERROR, "Invalid number base: %c at line %u", c, l->current_line);
                    return LEX_PARSE_ERROR;
                }
                // HACK: 
                word.ptr += 2;
                word.len -= 2;
            }
            else
            {
                base = BASE_DEC;
            }
            i32 num = sv_to_i32(word, base);
            u32 unum = *(u32*)&num;
            l->value = unum;
            return LEX_INT_LIT;
        }
        else if (word.ptr[word.len-1] == ':')
        {
            // TODO: storing the labels
            return LEX_LABELDEF;
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

typedef struct TranslationUnit{
    i32 err_count, warn_count;
    const char* file_path;
    const char* output_path;
    Lexer l;
    String file_content;
    String str_storage;
    String code;
}TranslationUnit;


bool tu_init(TranslationUnit* tu, const char* file_path, const char* output_path)
{
    tu->file_path = file_path;
    tu->output_path = output_path;
    if (!string_read_file(file_path, &tu->file_content)) return 1;
    SV file_sv = (SV){
        .ptr = tu->file_content.items, 
        .len = tu->file_content.count
    };

    lexer_init(&tu->l, file_sv, &tu->str_storage);
    
    return true;
}

void parse_error(TranslationUnit* tu, const char* format, ...)
{
    tu->err_count += 1;
    fprintf(stderr, "%s:%u error: ", tu->file_path, tu->l.current_line);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    // TODO: parse error sometimes skips more than it should
    skip_until_newline(&tu->l);
}


int main(void)
{
    TranslationUnit unit = {0};
    Lexer* l = &unit.l;
    tu_init(&unit, "./test/hello.asm", "");
    
    Token tok = LEX_PARSE_ERROR;
    bool shouldKeepParsing = true;
    while (shouldKeepParsing)
    {
        tok = lexer_get(l);
        switch (tok)
        {
            case LEX_EOF:
                {
                    shouldKeepParsing = false;
                    break;
                }
            case LEX_COMMENT:
                {
                    aasm_log(LOG_INFO, "Comment");
                    break;
                }
            case LEX_LINE_FEED: break;
            case LEX_IDENTIFIER: // HACK: identifier is not acceptable
                {
                    // NOTE: this is currently more like "parse instruction"
                    InstructionKind instr = instr_get_from_sv(l->identifier);
                    if (instr == INSTR__Invalid)
                    {
                        parse_error(&unit, "Invalid identifier: |%.*s|", (int)l->identifier.len, l->identifier.ptr);
                        break;
                    }
                    assert(instr_opcode[instr] == 0);

                    Type arg_type[4] = {0};
                    RegisterKind reg[4] = {0};
                    i32 imm[4] = {0};
                    int argc = 0;
                    for (;argc < 4;)
                    {
                        tok = lexer_get(l);
                        if (tok == LEX_COMMA) continue; // NOTE: now no commas are necessery for now
                        if (tok == LEX_LINE_FEED) break;
                        if (tok == LEX_EOF)
                        {
                            shouldKeepParsing = false;
                            break;
                        }
                        if (tok == LEX_INT_LIT)
                        {
                            arg_type[argc] = TYPE_IMM8;
                            imm[argc] = l->value;
                            argc += 1;
                        }
                        if (tok == LEX_IDENTIFIER)
                        {
                            RegisterKind curr = register_kind_from_sv(l->identifier);
                            if (curr == REG__Invaild) parse_error(&unit, "Wrong register: %.*s or something", SV_PRINT(l->identifier));
                            arg_type[argc] = register_types[curr];
                            reg[argc] = curr;
                            argc += 1;
                        }
                    }
                    InstructionKind specific_instr = instr_match_types(instr, argc, arg_type);
                    // TODO: better message for type mismatch
                    instr_assemble(&unit.code, specific_instr, arg_type, imm, reg);
                    break;
                }
            default:
                {
                    parse_error(&unit, "Unexpected token |%s| at the beginning of a line", token_printable[tok]);
                }
        }
    }
    printf("1 passes, %lu bytes\n", unit.code.count);
    printf("Compilation finished, %d errors, %d warnings\n", unit.err_count, unit.warn_count);
    for (u64 i = 0;i < unit.code.count;++i)
    {
        printf("%02x ", unit.code.items[i] & 0xff);
    }
    printf("\n");


    int (*executable_code)(const char*, unsigned long) = mmap(0, unit.code.count, PROT_EXEC | PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (executable_code == MAP_FAILED)
    {
        aasm_log(LOG_ERROR, "Could not map memory: %s\n", strerror(errno));
    }
    memcpy(executable_code, unit.code.items, unit.code.count);
    int res = executable_code("Hello world!\n", 13);
    printf("res = %d\n", res);


    return 0;
}
