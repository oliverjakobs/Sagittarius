#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum
{
    TOKEN_EOF = 0,
    /* Reserve first 128 values for single character tokens */
    TOKEN_LAST_CHAR = 127,
    TOKEN_KEYWORD,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STR,
    TOKEN_NAME,
    TOKEN_LSHIFT,
    TOKEN_RSHIFT,
    TOKEN_EQ,
    TOKEN_NOTEQ,
    TOKEN_LTEQ,
    TOKEN_GTEQ,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_COLON_ASSIGN,
    TOKEN_ADD_ASSIGN,
    TOKEN_FIRST_ASSIGN = TOKEN_ADD_ASSIGN,
    TOKEN_SUB_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_LSHIFT_ASSIGN,
    TOKEN_RSHIFT_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_LAST_ASSIGN = TOKEN_MOD_ASSIGN,
} TokenType;

typedef enum
{
    TOKENMOD_NONE,
    TOKENMOD_HEX,
    TOKENMOD_BIN,
    TOKENMOD_OCT,
    TOKENMOD_CHAR,
} TokenMod;

typedef struct
{
    TokenType type;
    TokenMod mod;
    const char* start;
    const char* end;
    union
    {
        uint64_t ival;
        double fval;
        const char* strval;
        const char* name;
    };
} Token;

const char* token_type_name(TokenType type);

size_t copy_token_type_str(char* dest, size_t dest_size, TokenType type);
const char* temp_token_type_str(TokenType type);

void print_token(Token token);

#endif /* TOKEN_H */
