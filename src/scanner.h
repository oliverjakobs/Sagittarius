#ifndef CLOCKWORK_SCANNER_H
#define CLOCKWORK_SCANNER_H

#include "common.h"

typedef enum
{
    // Single-character tokens.
    TOKEN_LPAREN,   TOKEN_RPAREN,
    TOKEN_LBRACE,   TOKEN_RBRACE,
    TOKEN_LBRACKET, TOKEN_RBRACKET,
    TOKEN_PERIOD,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_TERMINATOR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_EXCLAMATION, 
    TOKEN_ASSIGN,

    // Comparison tokens.
    TOKEN_EQ, TOKEN_NOTEQ,
    TOKEN_LT, TOKEN_LTEQ,
    TOKEN_GT, TOKEN_GTEQ,

    // Literals.
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    
    // Keywords.
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_LET,
    TOKEN_FUNC,
    TOKEN_DATATYPE,
    TOKEN_RETURN,
    TOKEN_PRINT,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

struct Token
{
  TokenType type;
  const char* start;
  const char* end;
  int line;
};

void cw_init_scanner(cwRuntime* cw, const char* src);

const char* cw_scan_token(Token* token, const char* cursor, int line);

#endif /* !CLOCKWORK_SCANNER_H */