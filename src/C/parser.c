#include "parser.h"

bool is_separator(u8 c)
{
    return c == ',' || c == ' ' || c == ';' || c == '\n';
}

void skip_until_newline(String* file, u64* i)
{
    do
    {
        *i += 1;
    } while (file->data[*i] != '\n' && *i < file->count);
}

void skip_until_separator(String* file, u64* i)
{
    do
    {
        *i += 1;
    } while (!is_separator(file->data[*i]) && *i < file->count);
}

Token token_from_sv(SV word)
{

}

bool parse_file(String* file, Tokens* tokens)
{
    bool result = true;
    u64 line_count = 1;
    for (u64 i = 0;i < file->count;++i)
    {
        u8 c = file->data[i];
        switch (c)
        {
            case ' ':
            case ',':
            {
                continue;
            }
            case ';':
                skip_until_newline(file, &i); // fallthrough
            case '\n':
            {
                line_count += 1;
                continue;
            }
            default:
                break;
        }
        u64 si = i;
        skip_until_separator(file, &i);
        SV word = (SV){.pointer = file->data + si, .length = i - si};
        i -= 1; // skip_until_separator() includes the separator which we need to analyze
    }
    return result;
}

