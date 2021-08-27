#include "scanner.h"

#include "runtime.h"

#include <string.h>

void cw_init_scanner(cwRuntime* cw, const char* src)
{
    cw->current.type = TOKEN_NULL;
    cw->current.start = src;
    cw->current.length = 0;
    cw->current.line = 1;
}

static inline bool cw_isdigit(char c) { return c >= '0' && c <= '9'; }
static inline bool cw_isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static const char* cw_skip_whitespaces(const char* cursor)
{
    while (*cursor != '\0')
    {
        switch (*cursor)
        {
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

static const char* cw_make_token(Token* token, TokenType type, const char* start, int len, int line)
{
    token->type = type;
    token->start = start;
    token->length = len;
    token->line = line;
    return start + len;
}

static const char* cw_make_double(Token* token, char c, TokenType type1, TokenType type2, const char* start, int line)
{
    if (start[1] == c) return cw_make_token(token, type1, start, 2, line);
    return cw_make_token(token, type2, start, 1, line);
}

static const char* cw_make_error(Token* token, const char* cursor, int line, const char* message)
{
    token->type = TOKEN_ERROR;
    token->start = message;
    token->length = strlen(message);
    token->line = line;
    return cursor;
}

static const char* cw_make_string(Token* token, const char* start, int line)
{
    const char* cursor = start + 1;
    while (*cursor != '"')
    {
        if (*cursor == '\0' || *cursor == '\n') return cw_make_error(token, cursor, line, "Unterminated string.");
        cursor++;
    }

    cursor++; /* skip the closing quote */
    return cw_make_token(token, TOKEN_STRING, start, cursor - start, line);
}

static const char* cw_make_number(Token* token, const char* start, int line)
{
    const char* cursor = start + 1;
    while (cw_isdigit(*cursor)) cursor++;

    // Look for a fractional part.
    if (cursor[0] == '.' && cw_isdigit(cursor[1]))
    {
        cursor++; /* Consume the ".". */
        while (cw_isdigit(*cursor)) cursor++;
    }

    return cw_make_token(token, TOKEN_NUMBER, start, cursor - start, line);
}

static TokenType cw_check_keyword(const char* name, int len, int offset, const char* rest, TokenType type)
{
    int rest_len = strlen(rest);
    if (len == offset + rest_len && memcmp(name + offset, rest, rest_len) == 0) return type;
    return TOKEN_IDENTIFIER;
}

static TokenType cw_identifier_type(const char* name, size_t len)
{
    switch (name[0])
    {
    case 'a': return cw_check_keyword(name, len, 1, "nd", TOKEN_AND);
    case 'd': return cw_check_keyword(name, len, 1, "atatype", TOKEN_DATATYPE);
    case 'e': return cw_check_keyword(name, len, 1, "lse", TOKEN_ELSE);
    case 'i': return cw_check_keyword(name, len, 1, "f", TOKEN_IF);
    case 'f':
        if (len > 1)
        {
            switch (name[1])
            {
            case 'a': return cw_check_keyword(name, len, 2, "lse", TOKEN_FALSE);
            case 'o': return cw_check_keyword(name, len, 2, "r", TOKEN_FOR);
            case 'u': return cw_check_keyword(name, len, 2, "nction", TOKEN_FUNC);
            }
        }
        break;
    case 'l': return cw_check_keyword(name, len, 1, "et", TOKEN_LET);
    case 'n': return cw_check_keyword(name, len, 1, "ull", TOKEN_NULL);
    case 'o': return cw_check_keyword(name, len, 1, "r", TOKEN_OR);
    case 'p': return cw_check_keyword(name, len, 1, "rint", TOKEN_PRINT);
    case 'r': return cw_check_keyword(name, len, 1, "eturn", TOKEN_RETURN);
    case 't': return cw_check_keyword(name, len, 1, "rue", TOKEN_RETURN);
    case 'w': return cw_check_keyword(name, len, 1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static const char* cw_make_identifier(Token* token, const char* start, int line)
{
    const char* cursor = start + 1;
    while (cw_isalpha(*cursor) || cw_isdigit(*cursor)) cursor++;
    int len = cursor - start;
    return cw_make_token(token, cw_identifier_type(start, len), start, len, line);
}

const char* cw_scan_token(Token* token, const char* cursor, int line)
{
    cursor = cw_skip_whitespaces(cursor);
    const char* start = cursor;

    switch (*cursor)
    {
    case '\0': return cw_make_token(token, TOKEN_EOF,        cursor, 0, line);
    case '\n': return cw_make_token(token, TOKEN_TERMINATOR, cursor, 1, line);
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        return cw_make_number(token, cursor, line);
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_':
        return cw_make_identifier(token, cursor, line);
    case '(': return cw_make_token(token, TOKEN_LPAREN,     cursor, 1, line);
    case ')': return cw_make_token(token, TOKEN_RPAREN,     cursor, 1, line);
    case '{': return cw_make_token(token, TOKEN_LBRACE,     cursor, 1, line);
    case '}': return cw_make_token(token, TOKEN_RBRACE,     cursor, 1, line);
    case ';': return cw_make_token(token, TOKEN_SEMICOLON,  cursor, 1, line);
    case ',': return cw_make_token(token, TOKEN_COMMA,      cursor, 1, line);
    case '.': return cw_make_token(token, TOKEN_PERIOD,     cursor, 1, line);
    case '-': return cw_make_token(token, TOKEN_MINUS,      cursor, 1, line);
    case '+': return cw_make_token(token, TOKEN_PLUS,       cursor, 1, line);
    case '/': return cw_make_token(token, TOKEN_SLASH,      cursor, 1, line);
    case '*': return cw_make_token(token, TOKEN_ASTERISK,   cursor, 1, line);
    case '!': return cw_make_double(token, '=', TOKEN_NOTEQ, TOKEN_EXCLAMATION, cursor, line);
    case '=': return cw_make_double(token, '=', TOKEN_EQ, TOKEN_ASSIGN, cursor, line);
    case '<': return cw_make_double(token, '=', TOKEN_LTEQ, TOKEN_LT, cursor, line); 
    case '>': return cw_make_double(token, '=', TOKEN_GTEQ, TOKEN_GT, cursor, line);
    case '"': return cw_make_string(token, cursor, line);
    default:  return cw_make_error(token, cursor, line, "Unexpected character.");
    }
}