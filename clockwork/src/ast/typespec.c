#include "typespec.h"

static Typespec* typespec_new(TypespecType type)
{
    Typespec* typespec = ast_alloc(sizeof(Typespec));
    typespec->type = type;
    return typespec;
}

Typespec* typespec_name(const char* name)
{
    Typespec* typespec = typespec_new(TYPESPEC_NAME);
    typespec->name = name;
    return typespec;
}

Typespec* typespec_pointer(Typespec* elem)
{
    Typespec* typespec = typespec_new(TYPESPEC_POINTER);
    typespec->ptr.elem = elem;
    return typespec;
}

Typespec* typespec_array(Typespec* elem, Expr* size)
{
    Typespec* typespec = typespec_new(TYPESPEC_ARRAY);
    typespec->array.elem = elem;
    typespec->array.size = size;
    return typespec;
}

Typespec* typespec_func(Typespec** args, size_t num_args, Typespec* ret)
{
    Typespec* typespec = typespec_new(TYPESPEC_FUNC);
    typespec->func.args = args;
    typespec->func.num_args = num_args;
    typespec->func.ret = ret;
    return typespec;
}