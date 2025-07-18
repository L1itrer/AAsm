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
#include <assert.h>

#ifndef stdout
#define stdout 1
#define stderr 2
#endif //stdout

#define UNREACHABLE(msg) do { fprintf(stderr, "%s:%d UNREACHABLE: %s at", __FILE__, __LINE__, msg); abort(); } while(0)
typedef unsigned char byte;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

#define DA_INIT_CAP 1024*8

typedef struct String
{
    char* data;
    u64 count;
    u64 capacity;
} String;

typedef struct StringView
{
    char* pointer;
    u64 length;
} StringView;

typedef StringView SV;

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
i64 sv_cmp(SV a, SV b);

#define SV_CMP_CSTR(sv, cstr) sv_cmp(sv, (SV){.pointer = cstr, .length = strlen(cstr)})
#endif //AASM_UTILITY_H

