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

struct Typespec
{
    TypespecType type;
    union
    {
        const char* name;
        struct
        {
            Typespec** args;
            size_t num_args;
            Typespec* ret;
        } func;
        struct
        {
            Typespec* elem;
            Expr* size;
        } array;
        struct
        {
            Typespec* elem;
        } ptr;
    };
};

Typespec* typespec_name(const char* name);
Typespec* typespec_pointer(Typespec* elem);
Typespec* typespec_array(Typespec* elem, Expr* size);
Typespec* typespec_func(Typespec** args, size_t num_args, Typespec* ret);

#endif // !AST_TYPESPEC_H
