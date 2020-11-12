#ifndef RESOLVE_H
#define RESOLVE_H

#include "ast/decl.h"

typedef struct Type Type;
typedef struct TypeField TypeField;

typedef enum
{
    TYPEID_INT,
    TYPEID_FLOAT,
    TYPEID_POINTER,
    TYPEID_ARRAY,
    TYPEID_FUNC,
    TYPEID_STRUCT,
    TYPEID_UNION
} TypeID;

struct Type
{
    TypeID id;
    union
    {
        struct
        {
            Type* base;
        } ptr;
        struct
        {
            Type* base;
            size_t size;
        } array;
        struct
        {
            Type** params;
            size_t num_params;
            Type* ret;
        } func;
        struct
        {
            TypeField* fields;
            size_t num_fields;
        } aggregate;
    };
};

struct TypeField
{
    const char* name;
    Type* type;
};

Type* type_int();
Type* type_float();
Type* type_pointer(Type* base);
Type* type_array(Type* base, size_t size);
Type* type_func(Type** params, size_t num_params, Type* ret);
Type* type_struct(TypeField* fields, size_t num_fields);
Type* type_union(TypeField* fields, size_t num_fields);

typedef struct
{
    Type* type;

} ConstEntity;

typedef struct
{
    // EntityType type;
    union
    {
        ConstEntity const_ent;
    };
} Entity;

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
