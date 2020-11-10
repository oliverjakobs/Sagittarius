#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "ast_common.h"

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
    EXPR_COMPOUND,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
    EXPR_SIZEOF_EXPR,
    EXPR_SIZEOF_TYPE,
} ExprType;

typedef struct
{
    Typespec* type;
    Expr** args;
    size_t num_args;
} CompoundExpr;

typedef struct
{
    Typespec* type;
    Expr* expr;
} CastExpr;

typedef struct
{
    TokenType op;
    Expr* expr;
} UnaryExpr;

typedef struct
{
    TokenType op;
    Expr* left;
    Expr* right;
} BinaryExpr;

typedef struct
{
    Expr* cond;
    Expr* then_expr;
    Expr* else_expr;
} TernaryExpr;

typedef struct
{
    Expr* expr;
    Expr** args;
    size_t num_args;
} CallExpr;

typedef struct
{
    Expr* expr;
    Expr* index;
} IndexExpr;

typedef struct
{
    Expr* expr;
    const char* name;
} FieldExpr;

struct Expr
{
    ExprType type;
    union
    {
        uint64_t ival;
        double fval;
        const char* strval;
        const char* name;
        Expr* sizeof_expr;
        Typespec* sizeof_type;
        CompoundExpr compound;
        CastExpr cast;
        UnaryExpr unary;
        BinaryExpr binary;
        TernaryExpr ternary;
        CallExpr call;
        IndexExpr index;
        FieldExpr field;
    };
};

Expr* expr_int(uint64_t val);
Expr* expr_float(double val);
Expr* expr_str(const char* str);
Expr* expr_name(const char* name);
Expr* expr_cast(Typespec* type, Expr* expr);
Expr* expr_call(Expr* operand, Expr** args, size_t num_args);
Expr* expr_index(Expr* operand, Expr* index);
Expr* expr_field(Expr* operand, const char* field);
Expr* expr_compound(Typespec* type, Expr** args, size_t num_args);
Expr* expr_unary(TokenType op, Expr* expr);
Expr* expr_binary(TokenType op, Expr* left, Expr* right);
Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr);
Expr* expr_sizeof_expr(Expr* expr);
Expr* expr_sizeof_type(Typespec* type);

#endif // !AST_EXPR_H
