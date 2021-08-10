#ifndef CLOCKWORK_SCANNER_H
#define CLOCKWORK_SCANNER_H

typedef enum
{
    // Single-character tokens.
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_PERIOD,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_ASTERISK,
    TOKEN_SLASH,
    TOKEN_EXCLAMATION, 
    TOKEN_ASSIGN,

    // Two character tokens.
    TOKEN_EQ,
    TOKEN_NOTEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTEQ,
    TOKEN_GTEQ,

    // Literals.
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    
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
    TOKEN_DECLARE,
    TOKEN_FUNC,
    TOKEN_RETURN,
    TOKEN_CLASS,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_PRINT,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct
{
  TokenType type;
  const char* start;
  int length;
  int line;
} Token;

typedef struct
{
    const char* start;
    const char* current;
    int line;
} Scanner;

void cw_scanner_init(Scanner* scanner, const char* src);

Token cw_scan_token(Scanner* scanner);

#endif /* !CLOCKWORK_SCANNER_H */