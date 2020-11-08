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
    FuncParam* params;
    size_t num_params;
    Typespec* ret_type;
    StmtBlock block;
} FuncDecl;

typedef struct
{
    const char* name;
    Expr* expr;
} EnumItem;

typedef struct
{
    EnumItem* items;
    size_t num_items;
} EnumDecl;

typedef struct
{
    const char** names;
    size_t num_names;
    Typespec* type;
} AggregateItem;

typedef struct
{
    AggregateItem* items;
    size_t num_items;
} AggregateDecl;

typedef struct
{
    Typespec* type;
} TypedefDecl;

typedef struct
{
    Typespec* type;
    Expr* expr;
} VarDecl;

typedef struct
{
    Expr* expr;
} ConstDecl;

struct Decl
{
    DeclType type;
    const char* name;
    union
    {
        EnumDecl enum_decl;
        AggregateDecl aggregate_decl;
        FuncDecl func_decl;
        TypedefDecl typedef_decl;
        VarDecl var_decl;
        ConstDecl const_decl;
    };
};

Decl* decl_enum(const char* name, EnumItem* items, size_t num_items);
Decl* decl_struct(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_union(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_var(const char* name, Typespec* type, Expr* expr);
Decl* decl_func(const char* name, FuncParam* params, size_t num_params, Typespec* ret_type, StmtBlock block);
Decl* decl_const(const char* name, Expr* expr);
Decl* decl_typedef(const char* name, Typespec* type);

#endif // !AST_DECL_H
