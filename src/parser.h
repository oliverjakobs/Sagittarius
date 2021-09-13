#ifndef CLOCKWORK_PARSER_H
#define CLOCKWORK_PARSER_H

#include "scanner.h"

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

cwValueType cw_parse_expression(cwRuntime* cw);

cwValueType cw_parse_precedence(cwRuntime* cw, Precedence precedence);

/* utility */
void cw_advance(cwRuntime* cw);
void cw_consume(cwRuntime* cw, cwTokenType type, const char* message);
bool cw_match(cwRuntime* cw, cwTokenType type);
void cw_parser_synchronize(cwRuntime* cw);

#endif /* !CLOCKWORK_PARSER_H */