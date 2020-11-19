#ifndef RESOLVE_H
#define RESOLVE_H

#include "ast/decl.h"
#include "ast/expr.h"
#include "ast/typespec.h"

typedef enum
{
    TYPE_NONE,
    TYPE_INCOMPLETE,
    TYPE_COMPLETING,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_FUNC
} TypeID;

typedef struct Type Type;
typedef struct Entity Entity;

typedef struct TypeField
{
    const char* name;
    Type* type;
} TypeField;

struct Type
{
    TypeID id;
    size_t size;
    Entity* entity;
    union
    {
        struct
        {
            Type* elem;
        } ptr;
        struct
        {
            Type* elem;
            size_t size;
        } array;
        struct
        {
            TypeField* fields;
            size_t num_fields;
        } aggregate;
        struct
        {
            Type** params;
            size_t num_params;
            Type* ret;
        } func;
    };
};

Type* type_int();
Type* type_float();
Type* type_pointer(Type* elem);
Type* type_array(Type* elem, size_t size);
Type* type_func(Type** params, size_t num_params, Type* ret);

void complete_type(Type* type);

typedef enum
{
    ENTITY_NONE,
    ENTITY_VAR,
    ENTITY_CONST,
    ENTITY_FUNC,
    ENTITY_TYPE,
    ENTITY_ENUM_CONST,
} EntityKind;

typedef enum
{
    ENTITY_UNRESOLVED,
    ENTITY_RESOLVING,
    ENTITY_RESOLVED,
} EntityState;

struct Entity
{
    const char* name;
    EntityKind kind;
    EntityState state;
    Decl* decl;
    Type* type;
    int64_t val;
};

Entity** get_entities();
Entity** get_ordered_entities();

Entity* entity_decl(Decl* decl);
Entity* entity_enum_const(const char* name, Decl* decl);

Entity* entity_get(const char* name);

Entity* entity_install_decl(Decl* decl);
Entity* entity_install_type(const char* name, Type* type);

void complete_entity(Entity* entity);


#endif // !RESOLVE_H
