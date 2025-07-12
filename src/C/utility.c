#include "utility.h"

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
        str->data = aasm_realloc(str->data, str->capacity);
    }
    fread(str->data + str->count, file_size, 1, file);
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

bool is_number(u8 c)
{
    return c >= '0' && c <= '9';
}

Bases bases_from_char(char c)
{
    if (c == 'x') return HEX;
    if (c == 'o') return OCT;
    if (c == 'b') return BIN;
    if (c == 'd' || is_number(c)) return DEC;
    return INVALID;
}

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
        u8 c = slice.pointer[i];\
        if (c == '_') continue;
        if (!is_literal_character_valid(c, multiply_by))
        {
            aasm_log(LOG_ERROR, "Invalid number literal conversion");
            result = 0;
            break;
        }
        CONVERT_ONE_CHAR(c, result, multiply_by);
    }
    return result;
}

i64 sv_cmp(SV str1, SV str2)
{
    if (str1.length != str2.length) return (i64)(str1.length - str2.length);
    for (u64 i = 0;i < str1.length;++i)
    {
        if (str1.pointer[i] != str2.pointer[i])
            return str1.pointer[i] - str2.pointer[i];
    }
    return 0;
}

