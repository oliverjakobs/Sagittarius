#ifndef CLOCKWORK_SCANNER_H
#define CLOCKWORK_SCANNER_H

#include "common.h"

typedef enum
{
    TOKEN_EOF = 0,
    /* single-character tokens */
    TOKEN_LPAREN,   TOKEN_RPAREN,
    TOKEN_LBRACE,   TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_PERIOD,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_EXCLAMATION,

    /* assignment */
    TOKEN_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_MULT_ASSIGN,
    TOKEN_DIV_ASSIGN,

    /* double-character tokens */
    TOKEN_INC,      TOKEN_DEC,
    TOKEN_BIT_AND,  TOKEN_BIT_OR,
    TOKEN_AND,      TOKEN_OR,

    /* comparison tokens */
    TOKEN_EQ, TOKEN_NOTEQ,
    TOKEN_LT, TOKEN_LTEQ,
    TOKEN_GT, TOKEN_GTEQ,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    
    /* Keywords */
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_CONTINUE,
    TOKEN_BREAK,
    TOKEN_LET,
    TOKEN_MUT,
    TOKEN_FUNC,
    TOKEN_DATATYPE,
    TOKEN_RETURN
} cwTokenType;

typedef enum
{
    TOKENMOD_NONE = 0,
    TOKENMOD_BIN,
    TOKENMOD_OCT,
    TOKENMOD_HEX,
} cwTokenMod;

struct cwToken
{
    cwTokenType type;
    cwTokenMod mod;
    const char* start;
    const char* end;
    int line;
};

const char* cw_scan_token(cwRuntime* cw, cwToken* token, const char* cursor, int line, int* error);

int cw_token_get_base(const cwToken* token);

#endif /* !CLOCKWORK_SCANNER_H */