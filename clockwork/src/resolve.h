#ifndef RESOLVE_H
#define RESOLVE_H

#include "ast/decl.h"

typedef enum
{
    SYMBOL_UNRESOLVED,
    SYMBOL_RESOLVING,
    SYMBOL_RESOLVED
} SymbolState;

typedef struct
{
    const char* name;
    Decl* decl;
    SymbolState state;
} Symbol;

void symbol_put(Decl* decl);
Symbol* symbol_get(const char* name);

void resolve_symbols();

#endif // !RESOLVE_H
