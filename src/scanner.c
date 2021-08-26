#include "scanner.h"

#include "runtime.h"

#include <string.h>
#include <stdio.h>

void cw_init_scanner(cwRuntime* cw, const char* src)
{
    cw->start = src;
    cw->cursor = src;
    cw->line = 1;
}

static inline bool cw_isdigit(char c) { return c >= '0' && c <= '9'; }
static inline bool cw_isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static inline bool cw_isend(const cwRuntime* cw)     { return *cw->cursor == '\0'; }
static inline char cw_peek(const cwRuntime* cw)      { return *cw->cursor; }
static inline char cw_peek_next(const cwRuntime* cw) { return cw->cursor[1]; }
static inline char cw_advance(cwRuntime* cw)         { cw->cursor++; return *(cw->cursor-1); }

static inline int cw_cursor_offset(const cwRuntime* cw) { return cw->cursor - cw->start; }

static bool cw_match(cwRuntime* cw, char expected)
{
    if (cw_isend(cw) || *cw->cursor != expected) return false;
    cw->cursor++;
    return true;
}

static const char* cw_skip_whitespaces(const char* cursor)
{
    while (true)
    {
        switch (*cursor)
        {
        case ' ': case '\t': case '\r':
            cursor++;
            break;
        case '#':
            while (*cursor != '\0' && *cursor != '\n') cursor++;
            break;
        default:
            return cursor;
        }
    }
}

static Token cw_make_token(const cwRuntime* cw, TokenType type)
{
    Token token = { .type = type, .start = cw->start, .length = cw_cursor_offset(cw), .line = cw->line };
    return token;
}

static Token cw_make_error(const cwRuntime* cw, const char* message)
{
    Token token = { .type = TOKEN_ERROR, .start = message, .length = strlen(message), .line = cw->line };
    return token;
}

static Token cw_make_string(cwRuntime* cw)
{
    while (cw_peek(cw) != '"')
    {
        if (cw_isend(cw)) return cw_make_error(cw, "Unterminated string.");
        if (cw_peek(cw) == '\n') cw->line++;
        cw_advance(cw);
    }

    cw_advance(cw); /* skip the closing quote */
    return cw_make_token(cw, TOKEN_STRING);
}

static Token cw_make_number(cwRuntime* cw)
{
    while (cw_isdigit(cw_peek(cw))) cw_advance(cw);

    // Look for a fractional part.
    if (cw_peek(cw) == '.' && cw_isdigit(cw_peek_next(cw)))
    {
        cw_advance(cw); /* Consume the ".". */
        while (cw_isdigit(cw_peek(cw))) cw_advance(cw);
    }
  
    return cw_make_token(cw, TOKEN_NUMBER);
}

static TokenType cw_check_keyword(cwRuntime* cw, int offset, int len, const char* rest, TokenType type)
{
    if (cw_cursor_offset(cw) == offset + len && memcmp(cw->start + offset, rest, len) == 0) return type;
    return TOKEN_IDENTIFIER;
}

static TokenType cw_identifier_type(cwRuntime* cw)
{
    switch (cw->start[0])
    {
    case 'a': return cw_check_keyword(cw, 1, 2, "nd", TOKEN_AND);
    case 'd': return cw_check_keyword(cw, 1, 7, "atatype", TOKEN_DATATYPE);
    case 'e': return cw_check_keyword(cw, 1, 3, "lse", TOKEN_ELSE);
    case 'i': return cw_check_keyword(cw, 1, 1, "f", TOKEN_IF);
    case 'f':
        if (cw_cursor_offset(cw) > 1)
        {
            switch (cw->start[1])
            {
            case 'a': return cw_check_keyword(cw, 2, 3, "lse", TOKEN_FALSE);
            case 'o': return cw_check_keyword(cw, 2, 1, "r", TOKEN_FOR);
            case 'u': return cw_check_keyword(cw, 2, 6, "nction", TOKEN_FUNC);
            }
        }
        break;
    case 'l': return cw_check_keyword(cw, 1, 2, "et", TOKEN_LET);
    case 'n': return cw_check_keyword(cw, 1, 3, "ull", TOKEN_NULL);
    case 'o': return cw_check_keyword(cw, 1, 1, "r", TOKEN_OR);
    case 'p': return cw_check_keyword(cw, 1, 4, "rint", TOKEN_PRINT);
    case 'r': return cw_check_keyword(cw, 1, 5, "eturn", TOKEN_RETURN);
    case 't': return cw_check_keyword(cw, 1, 3, "rue", TOKEN_RETURN);
    case 'w': return cw_check_keyword(cw, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token cw_make_identifier(cwRuntime* cw)
{
    while (cw_isalpha(cw_peek(cw)) || cw_isdigit(cw_peek(cw))) cw_advance(cw);
    return cw_make_token(cw, cw_identifier_type(cw));
}

Token cw_scan_token(cwRuntime* cw)
{
    cw->cursor = cw_skip_whitespaces(cw->cursor);
    cw->start = cw->cursor;

    if (cw_isend(cw)) return cw_make_token(cw, TOKEN_EOF);

    switch (cw_advance(cw))
    {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        return cw_make_number(cw);
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_':
        return cw_make_identifier(cw);
    case '(': return cw_make_token(cw, TOKEN_LPAREN);
    case ')': return cw_make_token(cw, TOKEN_RPAREN);
    case '{': return cw_make_token(cw, TOKEN_LBRACE);
    case '}': return cw_make_token(cw, TOKEN_RBRACE);
    case ';': return cw_make_token(cw, TOKEN_SEMICOLON);
    case ',': return cw_make_token(cw, TOKEN_COMMA);
    case '.': return cw_make_token(cw, TOKEN_PERIOD);
    case '-': return cw_make_token(cw, TOKEN_MINUS);
    case '+': return cw_make_token(cw, TOKEN_PLUS);
    case '/': return cw_make_token(cw, TOKEN_SLASH);
    case '*': return cw_make_token(cw, TOKEN_ASTERISK);
    case '!': return cw_make_token(cw, cw_match(cw, '=') ? TOKEN_NOTEQ : TOKEN_EXCLAMATION);
    case '=': return cw_make_token(cw, cw_match(cw, '=') ? TOKEN_EQ : TOKEN_ASSIGN);
    case '<': return cw_make_token(cw, cw_match(cw, '=') ? TOKEN_LTEQ : TOKEN_LT); 
    case '>': return cw_make_token(cw, cw_match(cw, '=') ? TOKEN_GTEQ : TOKEN_GT);
    case '"': return cw_make_string(cw);
    case '\n': cw->line++; return cw_make_token(cw, TOKEN_TERMINATOR);
    }

    return cw_make_error(cw, "Unexpected character.");
}