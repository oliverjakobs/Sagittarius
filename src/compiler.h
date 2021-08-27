#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "common.h"

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,  // '='
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // '==', '!='
    PREC_COMPARISON,  // '<', '>', '<=', '>='
    PREC_TERM,        // '+', '-'
    PREC_FACTOR,      // '*', '/'
    PREC_UNARY,       // '!', '-'
    PREC_CALL,        // '(...)'
    PREC_PRIMARY
} Precedence;

typedef void (*ParseCallback)(cwRuntime* cw, bool can_assign);

typedef struct
{
    ParseCallback prefix;
    ParseCallback infix;
    Precedence precedence;
} ParseRule;

bool cw_compile(cwRuntime* cw, const char* src, Chunk* chunk);

#endif /* !CLOCKWORK_COMPILER_H */