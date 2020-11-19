#ifndef AST_DECL_H
#define AST_DECL_H

#include "ast_common.h"

typedef enum
{
    DECL_NONE,
    DECL_ENUM,
    DECL_STRUCT,
    DECL_UNION,
    DECL_VAR,
    DECL_CONST,
    DECL_TYPEDEF,
    DECL_FUNC,
} DeclType;

typedef struct
{
    const char* name;
    Typespec* type;
} FuncParam;

typedef struct
{
    const char* name;
    Expr* expr;
} EnumItem;

typedef struct
{
    const char** names;
    size_t num_names;
    Typespec* type;
} AggregateItem;

struct Decl
{
    DeclType type;
    const char* name;
    union
    {
        struct
        {
            EnumItem* items;
            size_t num_items;
        } enum_decl;
        struct
        {
            AggregateItem* items;
            size_t num_items;
        } aggregate_decl;
        struct
        {
            FuncParam* params;
            size_t num_params;
            Typespec* ret_type;
            StmtList block;
        } func_decl;
        struct
        {
            Typespec* type;
        } typedef_decl;
        struct
        {
            Typespec* type;
            Expr* expr;
        } var_decl;
        struct
        {
            Expr* expr;
        } const_decl;
    };
};

AggregateItem aggregate_item(const char** names, size_t num_names, Typespec* type);

Decl* decl_enum(const char* name, EnumItem* items, size_t num_items);
Decl* decl_struct(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_union(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_var(const char* name, Typespec* type, Expr* expr);
Decl* decl_func(const char* name, FuncParam* params, size_t num_params, Typespec* ret_type, StmtList block);
Decl* decl_const(const char* name, Expr* expr);
Decl* decl_typedef(const char* name, Typespec* type);

#endif // !AST_DECL_H
