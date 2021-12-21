#include "lexer.h"

#include <string.h>

static inline bool cwIsDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool cwIsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static const char* cwSkipWhitespaces(const char* cursor, int* line)
{
    while (*cursor != '\0')
    {
        switch (*cursor)
        {
        case '\n':                      (*line)++;
        case ' ': case '\t': case '\r': cursor++; break;
        case '#':   /* skip comments */
            while (*cursor != '\0' && *cursor != '\n') cursor++;
            break;
        default:
            return cursor;
        }
    }
    return cursor;
}

static cwTokenType cwCheckKeyword(const char* start, const char* stream, int offset, const char* rest, cwTokenType type)
{
    int rest_len = strlen(rest);
    if ((stream - start) == offset + rest_len && memcmp(start + offset, rest, rest_len) == 0) return type;
    return TOKEN_IDENTIFIER;
}

static cwTokenType cwIdentifierType(const char* start, const char* stream)
{
    switch (start[0])
    {
    case 'a': return cwCheckKeyword(start, stream, 1, "nd", TOKEN_AND);
    case 'e': return cwCheckKeyword(start, stream, 1, "lse", TOKEN_ELSE);
    case 'i': return cwCheckKeyword(start, stream, 1, "f", TOKEN_IF);
    case 'f':
        if (stream - start > 1)
        {
            switch (start[1])
            {
            case 'a': return cwCheckKeyword(start, stream, 2, "lse", TOKEN_FALSE);
            case 'o': return cwCheckKeyword(start, stream, 2, "r", TOKEN_FOR);
            case 'u': return cwCheckKeyword(start, stream, 2, "nction", TOKEN_FUNC);
            }
        }
        break;
    case 'l': return cwCheckKeyword(start, stream, 1, "et", TOKEN_LET);
    case 'm': return cwCheckKeyword(start, stream, 1, "ut", TOKEN_MUT);
    case 'n': return cwCheckKeyword(start, stream, 1, "ull", TOKEN_NULL);
    case 'o': return cwCheckKeyword(start, stream, 1, "r", TOKEN_OR);
    case 'r': return cwCheckKeyword(start, stream, 1, "eturn", TOKEN_RETURN);
    case 't': return cwCheckKeyword(start, stream, 1, "rue", TOKEN_RETURN);
    case 'w': return cwCheckKeyword(start, stream, 1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

const char* cwNextToken(const char* stream, int* line, cwToken* token)
{
#define CW_TOKEN_CASE1(c, t) case c: token->type = t; break;
#define CW_TOKEN_CASE2(c1, t1, c2, t2) case c1:         \
    token->type = t1;                                   \
    if (*cursor == c2) { token->type = t2; cursor++; }  \
    break;
    
    const char* cursor = cwSkipWhitespaces(stream, line);

    token->type = TOKEN_UNKOWN;
    token->start = cursor;
    token->end = NULL;
    token->line = *line;

    if (*cursor == '\0')
    {
        token->type = TOKEN_EOF;
        return cursor;
    }

    switch (*(cursor++))
    {
    /* scan number */
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        while (cwIsDigit(*cursor)) cursor++;
        token->type = TOKEN_NUMBER;
        break;
    }
    /* scan identifiers and keywords */
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_':
    {
        while (cwIsAlpha(*cursor) || cwIsDigit(*cursor)) cursor++;
        token->type = cwIdentifierType(token->start, cursor);
        break;
    }
    CW_TOKEN_CASE1('(', TOKEN_LPAREN)
    CW_TOKEN_CASE1(')', TOKEN_RPAREN)
    CW_TOKEN_CASE1('{', TOKEN_LBRACE)
    CW_TOKEN_CASE1('}', TOKEN_RBRACE)
    CW_TOKEN_CASE1('[', TOKEN_LBRACKET)
    CW_TOKEN_CASE1(']', TOKEN_RBRACKET)
    CW_TOKEN_CASE1(',', TOKEN_COMMA)
    CW_TOKEN_CASE1(';', TOKEN_SEMICOLON)
    CW_TOKEN_CASE1(':', TOKEN_COLON)
    CW_TOKEN_CASE1('-', TOKEN_MINUS)
    CW_TOKEN_CASE1('+', TOKEN_PLUS)
    CW_TOKEN_CASE1('/', TOKEN_SLASH)
    CW_TOKEN_CASE1('*', TOKEN_ASTERISK)
    CW_TOKEN_CASE2('!', TOKEN_BANG,     '=', TOKEN_NOTEQ)
    CW_TOKEN_CASE2('=', TOKEN_ASSIGN,   '=', TOKEN_EQ)
    CW_TOKEN_CASE2('<', TOKEN_LT,       '=', TOKEN_LTEQ)
    CW_TOKEN_CASE2('>', TOKEN_GT,       '=', TOKEN_GTEQ)
    default:
        return 0;
    }

    token->end = cursor;
    return cursor;

#undef CW_TOKEN_CASE1
#undef CW_TOKEN_CASE2
}

const char* CW_TOKEN_TYPE_NAMES[] = {
    [TOKEN_EOF]         = "EOF",
    [TOKEN_UNKOWN]      = "UNKOWN",
    [TOKEN_IDENTIFIER]  = "IDENTIFIERS",
    [TOKEN_NUMBER]      = "NUMBER",
    [TOKEN_ASSIGN]      = "=",
    [TOKEN_PLUS]        = "+",
    [TOKEN_MINUS]       = "-",
    [TOKEN_BANG]        = "!",
    [TOKEN_ASTERISK]    = "*",
    [TOKEN_SLASH]       = "/",
    [TOKEN_EQ]          = "==",
    [TOKEN_NOTEQ]       = "!=",
    [TOKEN_LT]          = "<", 
    [TOKEN_LTEQ]        = "<=",
    [TOKEN_GT]          = ">",
    [TOKEN_GTEQ]        = ">=",
    [TOKEN_COMMA]       = ",",
    [TOKEN_SEMICOLON]   = ";",
    [TOKEN_LPAREN]      = "(",
    [TOKEN_RPAREN]      = ")",
    [TOKEN_LBRACE]      = "{",
    [TOKEN_RBRACE]      = "}",
    [TOKEN_LBRACKET]    = "[",
    [TOKEN_RBRACKET]    = "]",
    [TOKEN_NULL]        = "NULL",
    [TOKEN_TRUE]        = "TRUE",
    [TOKEN_FALSE]       = "FALSE",
    [TOKEN_AND]         = "AND",
    [TOKEN_OR]          = "OR",
    [TOKEN_IF]          = "IF",
    [TOKEN_ELSE]        = "ELSE",
    [TOKEN_WHILE]       = "WHILE",
    [TOKEN_FOR]         = "FOR",
    [TOKEN_LET]         = "LET",
    [TOKEN_MUT]         = "MUT",
    [TOKEN_FUNC]        = "FUNCTION",
    [TOKEN_RETURN]      = "RETURN"
};

const char* cwTokenTypeName(cwTokenType type) { return CW_TOKEN_TYPE_NAMES[type]; }