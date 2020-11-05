#ifndef AST_H
#define AST_H

#include "common.h"

#include "token.h"

typedef struct Expr Expr;
typedef struct Decl Decl;
typedef struct Stmt Stmt;

typedef struct Typespec Typespec;

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
    Typespec* ret;
} FuncTypespec;

struct Typespec
{
    TypespecType type;
    struct
    {
        const char* name;
        FuncTypespec func;
        struct
        {
            Typespec* base;
            Expr* size;
        };
    };
};

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
    Typespec* typespec;
} EnumItem;

typedef struct
{
    const char** names;
    Typespec* typespec;
} AggregateItem;

typedef struct
{
    const char* name;
    Typespec* typespec;
} FuncParam;

typedef struct
{
    FuncParam* params;
    Typespec* return_type;
} FuncDecl;

struct Decl
{
    DeclType type;
    const char* name;
    union
    {
        EnumItem* enum_items;
        AggregateItem* aggregate_items;
        struct
        {
            Typespec* typespec;
            Expr* expr;
        };
        FuncDecl func_decl;
    };
};

typedef enum
{
    EXPR_NONE,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_NAME,
    EXPR_CAST,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_FIELD,
    EXPR_COMPUND,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
} ExprType;

struct Expr
{
    ExprType type;
    TokenType op;
    union
    {
        /* literals/names */
        uint64_t ival;
        double fval;
        const char* strval;
        const char* name;
        struct /* compund literals */
        {
            Typespec* compound_type;
            Expr** compund_args;
        };
        struct /* casts */
        {
            Typespec* cast_type;
            Expr* cast_expr;
        };
        struct /* unary */
        {
            Expr* operand;
            union
            {
                Expr** args;
                Expr* index;
                const char* field;
            };
        };
        struct /* binary */
        {
            Expr* left;
            Expr* right;
        };
        struct /* ternary */
        {
            Expr* cond;
            Expr* then_expr;
            Expr* else_expr;
        };
    };
};

Expr* expr_int(uint64_t val);
Expr* expr_float(double val);
Expr* expr_str(const char* str);
Expr* expr_name(const char* name);
Expr* expr_cast(Typespec* type, Expr* expr);
Expr* expr_unary(TokenType op, Expr* expr);
Expr* expr_binary(TokenType op, Expr* left, Expr* right);
Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr);

void print_expr(Expr* expr);


typedef enum
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    SMTM_DO,
    STMT_SWITCH,
    STMT_ASSIGN,
    STMT_AUTO_ASSIGN,
    STMT_EXPR,
} StmtType;

typedef struct
{
    Stmt** stmts;
} StmtBlock;

typedef struct
{
    Expr* cond;
    StmtBlock block;
} ElseIf;

typedef struct
{
    Expr** exprs;
    StmtBlock block;
} SwitchCase;

struct Stmt
{
    StmtType type;
    Expr* expr;
    StmtBlock block;
    union
    {
        struct /* if */
        {
            ElseIf* elseifs;
            StmtBlock else_block;
        };
        struct /* for */
        {
            StmtBlock for_init;
            StmtBlock for_next;
        };
        struct /* switch */
        {
            SwitchCase* cases;
        };
        struct /* auto assign */
        {
            const char* name;
        };
        struct /* assign operators */
        {
            Expr* rhs;
        };
    };
};


#endif /* !AST_H */
