#ifndef AST_TYPESPEC_H
#define AST_TYPESPEC_H

#include "ast_common.h"

typedef enum
{
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_POINTER,
} TypespecType;

typedef struct
{
    Typespec** args;
    size_t num_args;
    Typespec* ret;
} FuncTypespec;

typedef struct
{
    Typespec* elem;
} PointerTypespec;

typedef struct
{
    Typespec* elem;
    Expr* size;
} ArrayTypespec;

struct Typespec
{
    TypespecType type;
    union
    {
        const char* name;
        FuncTypespec func;
        ArrayTypespec array;
        PointerTypespec ptr;
    };
};

Typespec* typespec_name(const char* name);
Typespec* typespec_pointer(Typespec* elem);
Typespec* typespec_array(Typespec* elem, Expr* size);
Typespec* typespec_func(Typespec** args, size_t num_args, Typespec* ret);

#endif // !AST_TYPESPEC_H
