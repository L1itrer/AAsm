#ifndef AASM_UTILITY_H
#define AASM_UTILITY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef stdout
#define stdout 1
#define stderr 2
#endif //stdout

typedef unsigned char byte;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef struct String
{
    byte* data;
    u64 count;
    u64 capacity;
} String;

typedef struct StringView
{
    byte* pointer;
    u64 length;
} StringView;

typedef StringView SV;

bool string_read_file(const char* path, String* str);


//TODO: To rewrite this thing in itself I need my own implementations of libc functions
// memory related functions:
void* oasm_memset(void* buffer, i32 value, u64 count);
void* oasm_memcpy(void* dst, void* src, u64 count);
void* oasm_malloc(u64 count);

// standard output

typedef enum OasmLogLevel
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
}OasmLogLevel;

void oasm_log(OasmLogLevel level, const char* format, ...);

// number conversion

typedef enum Bases{
    BIN = 2,
    OCT = 8,
    DEC = 10,
    HEX = 16
}Bases;

i32 sv_to_i32(SV slice, Bases base);
u64 string_to_u64(SV slice, Bases base);
bool i32_to_string(String* str, i32 number, Bases base);


#endif //AASM_UTILITY_H

