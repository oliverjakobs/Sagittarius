#include "resolve.h"

static Type* type_alloc(TypeID id)
{
    Type* type = xcalloc(1, sizeof(Type));
    type->id = id;
    return type;
}

static Type type_int_val = { TYPEID_INT };
static Type type_float_val = { TYPEID_FLOAT };

Type* type_int() { return &type_int_val; }
Type* type_float() { return &type_float_val; }

typedef struct
{
    Type* base;
    Type* ptr;
} CachedPointerType;

static CachedPointerType* cached_ptr_types;

Type* type_pointer(Type* base)
{
    for (CachedPointerType* it = cached_ptr_types; it != tb_stretchy_last(cached_ptr_types); it++)
    {
        if (it->base == base)
            return it->ptr;
    }

    Type* type = type_alloc(TYPEID_POINTER);
    type->ptr.base = base;
    tb_stretchy_push(cached_ptr_types, ((CachedPointerType){base, type}));
    return type;
}

typedef struct
{
    Type* base;
    size_t size;
    Type* ptr;
} CachedArrayType;

static CachedArrayType* cached_array_types;

Type* type_array(Type* base, size_t size)
{
    for (CachedArrayType* it = cached_array_types; it != tb_stretchy_last(cached_array_types); it++)
    {
        if (it->base == base && it->size == size)
            return it->ptr;
    }

    Type* type = type_alloc(TYPEID_POINTER);
    type->array.base = base;
    type->array.size = size;
    tb_stretchy_push(cached_array_types, ((CachedArrayType){base, size, type}));
    return type;
}

typedef struct
{
    Type** params;
    size_t num_params;
    Type* ret;
    Type* func;
} CachedFuncType;

static CachedFuncType* cached_func_types;

Type* type_func(Type** params, size_t num_params, Type* ret)
{
    for (CachedFuncType* it = cached_func_types; it != tb_stretchy_last(cached_func_types); it++)
    {
        if (it->num_params == num_params && it->ret == ret)
        {
            for (size_t i = 0; i < num_params; ++i)
            {
                if (it->params[i] != params[i])
                    goto next;
            }
            return it->func;
        }
    next:;
    }
    Type* type = type_alloc(TYPEID_FUNC);
    type->func.params = xcalloc(num_params, sizeof(Type*));
    memcpy(type->func.params, params, num_params * sizeof(Type*));
    type->func.num_params = num_params;
    type->func.ret = ret;
    tb_stretchy_push(cached_func_types, ((CachedFuncType){params, num_params, ret, type}));
    return type;
}

static Type* type_aggregate(TypeID id, TypeField* fields, size_t num_fields)
{
    Type* type = type_alloc(id);
    type->aggregate.fields = xcalloc(num_fields, sizeof(TypeField));
    memcpy(type->aggregate.fields, fields, num_fields * sizeof(TypeField));
    type->aggregate.num_fields = num_fields;
    return type;
}

Type* type_struct(TypeField* fields, size_t num_fields)
{
    return type_aggregate(TYPEID_STRUCT, fields, num_fields);
}

Type* type_union(TypeField* fields, size_t num_fields)
{
    return type_aggregate(TYPEID_UNION, fields, num_fields);
}

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

ConstEntity* resolve_const_expr(Expr* expr)
{
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
