#include "parser.h"
#include "utility.h"

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

Instruction sv_to_instruction(SV word)
{
    // TODO: make instruction parsing case insensitive
    if (SV_CMP_CSTR(word, "mov") == 0)
    {
        return MOV;
    }
    if (SV_CMP_CSTR(word, "ret") == 0)
    {
        return RET;
    }
    if (SV_CMP_CSTR(word, "syscall") == 0)
    {
        return MOV;
    }
    return INSTR_INVALID;
}
Token token_from_sv(SV word)
{
    Token ret_token = {0};
    if (is_number(word.pointer[0]))
    {
        Bases base = bases_from_char(word.pointer[1]);
        if (base == INVALID)
        {
            aasm_log(LOG_ERROR, "Invalid number base: %c", word.pointer[1]);
            return ret_token;
        }
        i32 num = sv_to_i32(word, base);
        ret_token.type = IMM32;
        ret_token.value = num;
    }
    else if (word.pointer[word.length-1] == ':')
    {
        //TODO: It's a label! introduce handling of labels
    }
    else
    {
        Instruction instr = sv_to_instruction(word);
        if (instr == INSTR_INVALID)
        {
            aasm_log(LOG_ERROR, "Invalid instruction: %.*s", word.length, word.pointer);
            return ret_token;
        }
        ret_token.type = INSTRUCTION;
        ret_token.value = instr;
    }
    return ret_token;
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
        Token curr_token = token_from_sv(word);
        if (curr_token.type == TOK_INVALID)
        {
            result = false;
        }
        i -= 1; // skip_until_separator() includes the separator which we need to analyze
    }
    return result;
}

