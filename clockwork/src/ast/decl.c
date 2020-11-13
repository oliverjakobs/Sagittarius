#include "decl.h"

static Decl* decl_new(DeclType type, const char* name)
{
    Decl* decl = ast_alloc(sizeof(Decl));
    decl->type = type;
    decl->name = name;
    return decl;
}

Decl* decl_enum(const char* name, EnumItem* items, size_t num_items)
{
    Decl* decl = decl_new(DECL_ENUM, name);
    decl->enum_decl.items = items;
    decl->enum_decl.num_items = num_items;
    return decl;
}

Decl* decl_struct(const char* name, AggregateItem* items, size_t num_items)
{
    Decl* decl = decl_new(DECL_STRUCT, name);
    decl->aggregate_decl.items = items;
    decl->aggregate_decl.num_items = num_items;
    return decl;
}

Decl* decl_union(const char* name, AggregateItem* items, size_t num_items)
{
    Decl* decl = decl_new(DECL_UNION, name);
    decl->aggregate_decl.items = items;
    decl->aggregate_decl.num_items = num_items;
    return decl;
}

Decl* decl_var(const char* name, Typespec* type, Expr* expr)
{
    Decl* decl = decl_new(DECL_VAR, name);
    decl->var_decl.type = type;
    decl->var_decl.expr = expr;
    return decl;
}

Decl* decl_func(const char* name, FuncParam* params, size_t num_params, Typespec* ret_type, StmtList block)
{
    Decl* decl = decl_new(DECL_FUNC, name);
    decl->func_decl.params = params;
    decl->func_decl.num_params = num_params;
    decl->func_decl.ret_type = ret_type;
    decl->func_decl.block = block;
    return decl;
}

Decl* decl_const(const char* name, Expr* expr)
{
    Decl* decl = decl_new(DECL_CONST, name);
    decl->const_decl.expr = expr;
    return decl;
}

Decl* decl_typedef(const char* name, Typespec* type)
{
    Decl* decl = decl_new(DECL_TYPEDEF, name);
    decl->typedef_decl.type = type;
    return decl;
}