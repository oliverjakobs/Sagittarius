#include "resolve.h"

static Symbol* symbol_list;

void symbol_put(Decl* decl)
{
    assert(decl->name);
    assert(!symbol_get(decl->name));
    tb_stretchy_push(symbol_list, ((Symbol){ decl->name, decl, SYMBOL_UNRESOLVED }));
}

Symbol* symbol_get(const char* name)
{
    for (Symbol* it = symbol_list; it != tb_stretchy_last(symbol_list); it++)
    {
        if (it->name == name)
            return it;
    }
    return NULL;
}

void resolve_decl(Decl* decl)
{
    switch (decl->type)
    {
    case DECL_CONST:
        break;
    }
}

void resolve_symbol(Symbol* sym)
{
    if (sym->state == SYMBOL_UNRESOLVED)
        return;

    if (sym->state == SYMBOL_RESOLVING)
    {
        fatal("Cyclic dependency");
        return;
    }
    resolve_decl(sym->decl);
}

Symbol* resolve_name(const char* name)
{
    Symbol* sym = symbol_get(name);
    if (!sym)
    {
        fatal("Unkown symbol name");
        return NULL;
    }
    resolve_symbol(sym);
    return sym;
}

void resolve_symbols()
{
    for (Symbol* it = symbol_list; it != tb_stretchy_last(symbol_list); it++)
        resolve_symbol(it);
}
