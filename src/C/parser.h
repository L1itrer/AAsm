#ifndef AASM_PARSER_H
#define AASM_PARSER_H
#include "utility.h"

typedef enum TokenKind {
    TOK_INVALID= 0,
    INSTRUCTION,
    COMMA,
    SEMI_COLON,
    REG8,
    REG16,
    REG32,
    REG64,
    MEM8,
    MEM16,
    MEM32,
    MEM64,
    IMM8,
    IMM16,
    IMM32,
    IMM64
}TokenKind;

typedef struct Token {
    TokenKind type;
    i64 value;
} Token;

typedef struct Tokens{
    Token* items;
    size_t count;
    size_t capacity;
}Tokens;

typedef enum Registers{
    AX,
    BX,
    CX,
    DX,
    BP,
    SI,
    DI,
    SP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
}Registers;

typedef enum Instruction {
    INSTR_INVALID,
    MOV,
    SYSCALL,
    RET,
}Instruction;

bool parse_file(String* file, Tokens* tokens);

#endif //AASM_PARSER_H
