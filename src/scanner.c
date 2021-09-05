#include "scanner.h"

#include "debug.h"
#include "runtime.h"

#include <string.h>

static inline bool cw_isdigit(char c) { return c >= '0' && c <= '9'; }
static inline bool cw_isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static const char* cw_skip_whitespaces(const char* cursor, int* line)
{
    while (*cursor != '\0')
    {
        switch (*cursor)
        {
        case '\n':
            (*line)++;
        case ' ': case '\t': case '\r':
            cursor++;
            break;
        case '#':   /* skip comments */
            while (*cursor != '\0' && *cursor != '\n') cursor++;
            break;
        default:
            return cursor;
        }
    }
    return cursor;
}

static cwTokenType cw_check_keyword(const char* start, const char* stream, int offset, const char* rest, cwTokenType type)
{
    int rest_len = strlen(rest);
    if ((stream - start) == offset + rest_len && memcmp(start + offset, rest, rest_len) == 0) return type;
    return TOKEN_IDENTIFIER;
}

static cwTokenType cw_identifier_type(const char* start, const char* stream)
{
    switch (start[0])
    {
    case 'a': return cw_check_keyword(start, stream, 1, "nd", TOKEN_AND);
    case 'd': return cw_check_keyword(start, stream, 1, "atatype", TOKEN_DATATYPE);
    case 'e': return cw_check_keyword(start, stream, 1, "lse", TOKEN_ELSE);
    case 'i': return cw_check_keyword(start, stream, 1, "f", TOKEN_IF);
    case 'f':
        if (stream - start > 1)
        {
            switch (start[1])
            {
            case 'a': return cw_check_keyword(start, stream, 2, "lse", TOKEN_FALSE);
            case 'o': return cw_check_keyword(start, stream, 2, "r", TOKEN_FOR);
            case 'u': return cw_check_keyword(start, stream, 2, "nction", TOKEN_FUNC);
            }
        }
        break;
    case 'l': return cw_check_keyword(start, stream, 1, "et", TOKEN_LET);
    case 'n': return cw_check_keyword(start, stream, 1, "ull", TOKEN_NULL);
    case 'o': return cw_check_keyword(start, stream, 1, "r", TOKEN_OR);
    case 'p': return cw_check_keyword(start, stream, 1, "rint", TOKEN_PRINT);
    case 'r': return cw_check_keyword(start, stream, 1, "eturn", TOKEN_RETURN);
    case 't': return cw_check_keyword(start, stream, 1, "rue", TOKEN_RETURN);
    case 'w': return cw_check_keyword(start, stream, 1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

const char* cw_scan_token(cwRuntime* cw, cwToken* token, const char* cursor, int line)
{
#define CW_TOKEN_CASE1(c, t) case c: token->type = t; cursor++; break;
#define CW_TOKEN_CASE2(c1, t1, c2, t2) case c1:         \
    token->type = t1; cursor++;                         \
    if (*cursor == c2) { token->type = t2; cursor++; }  \
    break;
    
    cursor = cw_skip_whitespaces(cursor, &line);

    token->mod = TOKENMOD_NONE;
    token->line = line;
    token->start = cursor; 

    switch (*cursor)
    {
    case '\0': token->type = TOKEN_EOF; break;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        cursor++;
        while (cw_isdigit(*cursor)) cursor++;
        token->type = TOKEN_INTEGER;

        // Look for a fractional part.
        if (cursor[0] == '.' && cw_isdigit(cursor[1]))
        {
            cursor++; /* Consume the ".". */
            while (cw_isdigit(*cursor)) cursor++;
            token->type = TOKEN_FLOAT;
        }
        break;
    }
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_':
    {
        cursor++;
        while (cw_isalpha(*cursor) || cw_isdigit(*cursor)) cursor++;
        token->type = cw_identifier_type(token->start, cursor);
        break;
    }
    case '"':
    {
        cursor++; /* skip the opening quote */
        while (*cursor != '"')
        {
            if (*cursor == '\0' || *cursor == '\n')
            {
                cw_syntax_error(cw, line, "Unterminated string.");
                return cursor;
            }
            cursor++;
        }
        cursor++; /* skip the closing quote */

        token->type = TOKEN_STRING;
        break;
    }
    /* single character token */
    CW_TOKEN_CASE1('(', TOKEN_LPAREN)
    CW_TOKEN_CASE1(')', TOKEN_RPAREN)
    CW_TOKEN_CASE1('{', TOKEN_LBRACE)
    CW_TOKEN_CASE1('}', TOKEN_RBRACE)
    CW_TOKEN_CASE1('[', TOKEN_LBRACKET)
    CW_TOKEN_CASE1(']', TOKEN_RBRACKET)
    CW_TOKEN_CASE1('.', TOKEN_PERIOD)
    CW_TOKEN_CASE1(',', TOKEN_COMMA)
    CW_TOKEN_CASE1(':', TOKEN_COLON)
    CW_TOKEN_CASE1(';', TOKEN_SEMICOLON)
    CW_TOKEN_CASE1('-', TOKEN_MINUS)
    CW_TOKEN_CASE1('+', TOKEN_PLUS)
    CW_TOKEN_CASE1('/', TOKEN_SLASH)
    CW_TOKEN_CASE1('*', TOKEN_ASTERISK)
    /* potential double character token */
    CW_TOKEN_CASE2('!', TOKEN_EXCLAMATION,  '=', TOKEN_NOTEQ)
    CW_TOKEN_CASE2('=', TOKEN_ASSIGN,       '=', TOKEN_EQ)
    CW_TOKEN_CASE2('<', TOKEN_LT,           '=', TOKEN_LTEQ)
    CW_TOKEN_CASE2('>', TOKEN_GT,           '=', TOKEN_GTEQ)
    default:
        cw_syntax_error(cw, line, "Unexpected character.");
        return ++cursor;
    }

    token->end = cursor;
    return cursor;

#undef CW_TOKEN_CASE1
#undef CW_TOKEN_CASE2
}

int cw_token_get_base(const cwToken* token)
{
    switch (token->mod)
    {
    case TOKENMOD_BIN: return 2;
    case TOKENMOD_OCT: return 8;
    case TOKENMOD_HEX: return 16;
    default:           return 10;
    }
}