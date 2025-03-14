#include "utility.h"

//bool string_read_file(const char* path, String* str)
//{
//    int file = open(path, O_RDONLY);
//    if (file == -1)
//    {
//        const char msg[] = "[ERROR]: Could not open file";
//        write(stderr, msg, sizeof(msg) - 1);
//    }
//
//}


// memory related functions:

void* oasm_memset(void* buffer, i32 value, u64 count)
{
    return memset(buffer, value, count);
}

void* oasm_memcpy(void* dst, void* src, u64 count)
{
    return memcpy(dst, src, count);
}

void* oasm_malloc(u64 count)
{
    return malloc(count);
}

void* oasm_realloc(void* buffer, u64 count)
{
    return realloc(buffer, count);
}

// standard output

void oasm_log(OasmLogLevel level, const char* format, ...)
{
    switch(level)
    {
        case LOG_INFO:
            write(2, "[INFO]: ", 8);
            break;
        case LOG_WARNING:
            write(2, "[WARNING]: ", 11);
            break;
        case LOG_ERROR:
            write(2, "[ERROR]: ", 9);
            break;
    }
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    write(2, "\n", 1);
}

static bool is_literal_character_valid(unsigned char c, i32 base)
{
    if (base != HEX)
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




i32 sv_to_i32(SV slice, Bases base)
{
    i32 result = 0;
    i32 multiply_by = (i32)base;
//    u64 i = slice.pointer[0] != '-' ? 3 : 2; // load up the first digit index
//    if (c == 'b' || c == 'B' && slice.pointer[i-1] == '0') multiply_by = 2;
//    else if (c == 'o' || c == 'O' && slice.pointer[i-1] == '0') multiply_by = 8;
//    else if (c == 'x' || c == 'X' && slice.pointer[i-1] == '0') multiply_by = 16;
//    else if (c == 'd' || c == 'D' && slice.pointer[i-1] == '0') multiply_by = 10;
//    else
//    {
//        multiply_by = 10;
//        i = slice.pointer[0] != '-' ? 1 : 0;
//    }

    for (u64 i = 0; i < slice.length;i += 1)
    {
        u8 c = slice.pointer[i];
        if (!is_literal_character_valid(c, multiply_by))
        {
            oasm_log(LOG_ERROR, "Invalid number literal conversion");
            result = 0;
            break;
        }
        CONVERT_ONE_CHAR(c, result, multiply_by);
    }
    return result;
}