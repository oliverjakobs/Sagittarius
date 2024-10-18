#include "clockwork.h"

ENUM(TokenType) {
    TOKEN_EOF = 0,
    TOKEN_UNKOWN,

    /* identifiers and literals */
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,

    /* operators */
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_BANG,
    TOKEN_ASTERISK,
    TOKEN_SLASH,

    /* comparison tokens */
    TOKEN_EQ, TOKEN_NOTEQ,
    TOKEN_LT, TOKEN_LTEQ,
    TOKEN_GT, TOKEN_GTEQ,

    /* delimiters */
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,

    /* brackets */
    TOKEN_LPAREN,   TOKEN_RPAREN,
    TOKEN_LBRACE,   TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET,

    /* Keywords */
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_LET,
    TOKEN_MUT,
    TOKEN_FUNC,
    TOKEN_RETURN
};


STRUCT(Token) {
    TokenType type;
    const char* start;
    const char* end;
    int line;
};

const char* const TOKEN_TYPE_NAMES[] = {
    [TOKEN_EOF]         = "EOF",
    [TOKEN_UNKOWN]      = "UNKOWN",
    [TOKEN_IDENTIFIER]  = "IDENTIFIER",
    [TOKEN_NUMBER]      = "NUMBER",
    [TOKEN_ASSIGN]      = "ASSIGN",
    [TOKEN_PLUS]        = "PLUS",
    [TOKEN_MINUS]       = "MINUS",
    [TOKEN_BANG]        = "BANG",
    [TOKEN_ASTERISK]    = "ASTERISK",
    [TOKEN_SLASH]       = "SLASH",
    [TOKEN_EQ]          = "EQ",
    [TOKEN_NOTEQ]       = "NOTEQ",
    [TOKEN_LT]          = "LT", 
    [TOKEN_LTEQ]        = "LTEQ",
    [TOKEN_GT]          = "GT",
    [TOKEN_GTEQ]        = "GTEQ",
    [TOKEN_COMMA]       = "COMMA",
    [TOKEN_SEMICOLON]   = "SEMICOLON",
    [TOKEN_LPAREN]      = "LPAREN",
    [TOKEN_RPAREN]      = "RPAREN",
    [TOKEN_LBRACE]      = "LBRACE",
    [TOKEN_RBRACE]      = "RBRACE",
    [TOKEN_LBRACKET]    = "LBRACKET",
    [TOKEN_RBRACKET]    = "RBRACKET",
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

void print_token(Token token) {
    printf("%s(%.*s)", TOKEN_TYPE_NAMES[token.type], token.end - token.start, token.start);
}

static inline uint8_t is_digit(char c) { return c >= '0' && c <= '9'; }
static inline uint8_t is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static const char* skip_whitespaces(const char* cursor, int* line) {
    while (*cursor != '\0') {
        switch (*cursor) {
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

static TokenType check_keyword(const char* start, const char* stream, int offset, const char* rest, TokenType type) {
    int rest_len = strlen(rest);
    if ((stream - start) == offset + rest_len && memcmp(start + offset, rest, rest_len) == 0) return type;
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(const char* start, const char* stream) {
    switch (start[0]) {
        case 'a': return check_keyword(start, stream, 1, "nd", TOKEN_AND);
        case 'e': return check_keyword(start, stream, 1, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(start, stream, 1, "f", TOKEN_IF);
        case 'f':
            if (stream - start > 1) {
                switch (start[1]) {
                    case 'a': return check_keyword(start, stream, 2, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(start, stream, 2, "r", TOKEN_FOR);
                    case 'u': return check_keyword(start, stream, 2, "nc", TOKEN_FUNC);
                }
            }
            break;
        case 'l': return check_keyword(start, stream, 1, "et", TOKEN_LET);
        case 'm': return check_keyword(start, stream, 1, "ut", TOKEN_MUT);
        case 'n': return check_keyword(start, stream, 1, "ull", TOKEN_NULL);
        case 'o': return check_keyword(start, stream, 1, "r", TOKEN_OR);
        case 'r': return check_keyword(start, stream, 1, "eturn", TOKEN_RETURN);
        case 't': return check_keyword(start, stream, 1, "rue", TOKEN_RETURN);
        case 'w': return check_keyword(start, stream, 1, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

const char* next_token(const char* stream, int* line, Token* token) {
#define TOKEN_CASE1(c, t) case c: token->type = t; break;
#define TOKEN_CASE2(c1, t1, c2, t2) case c1:            \
    token->type = t1;                                   \
    if (*cursor == c2) { token->type = t2; cursor++; }  \
    break;

    const char* cursor = skip_whitespaces(stream, line);

    token->type = TOKEN_UNKOWN;
    token->start = cursor;
    token->end = NULL;
    token->line = *line;

    if (*cursor == '\0') {
        token->type = TOKEN_EOF;
        return cursor;
    }

    switch (*(cursor++)) {
        /* scan number */
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            while (is_digit(*cursor)) cursor++;
            token->type = TOKEN_NUMBER;
            break;
        /* scan identifiers and keywords */
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '_':
            while (is_alpha(*cursor) || is_digit(*cursor)) cursor++;
            token->type = identifier_type(token->start, cursor);
            break;
        TOKEN_CASE1('(', TOKEN_LPAREN)
        TOKEN_CASE1(')', TOKEN_RPAREN)
        TOKEN_CASE1('{', TOKEN_LBRACE)
        TOKEN_CASE1('}', TOKEN_RBRACE)
        TOKEN_CASE1('[', TOKEN_LBRACKET)
        TOKEN_CASE1(']', TOKEN_RBRACKET)
        TOKEN_CASE1(',', TOKEN_COMMA)
        TOKEN_CASE1(';', TOKEN_SEMICOLON)
        TOKEN_CASE1(':', TOKEN_COLON)
        TOKEN_CASE1('-', TOKEN_MINUS)
        TOKEN_CASE1('+', TOKEN_PLUS)
        TOKEN_CASE1('/', TOKEN_SLASH)
        TOKEN_CASE1('*', TOKEN_ASTERISK)
        TOKEN_CASE2('!', TOKEN_BANG,     '=', TOKEN_NOTEQ)
        TOKEN_CASE2('=', TOKEN_ASSIGN,   '=', TOKEN_EQ)
        TOKEN_CASE2('<', TOKEN_LT,       '=', TOKEN_LTEQ)
        TOKEN_CASE2('>', TOKEN_GT,       '=', TOKEN_GTEQ)
        default: return 0;
    }

    token->end = cursor;
    return cursor;

#undef TOKEN_CASE1
#undef TOKEN_CASE2
}