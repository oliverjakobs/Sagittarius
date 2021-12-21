#ifndef CLOCKWORK_SCANNER_H
#define CLOCKWORK_SCANNER_H

#include "common.h"

typedef enum
{
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
} cwTokenType;

struct cwToken
{
    cwTokenType type;
    const char* start;
    const char* end;
    int line;
};

const char* cwNextToken(const char* stream, int* line, cwToken* token);

const char* cwTokenTypeName(cwTokenType type);

#endif /* !CLOCKWORK_SCANNER_H */