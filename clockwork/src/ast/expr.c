#include "expr.h"

static Expr* expr_new(ExprType type)
{
    Expr* expr = ast_alloc(sizeof(Expr));
    expr->type = type;
    return expr;
}

Expr* expr_int(uint64_t val)
{
    Expr* expr = expr_new(EXPR_INT);
    expr->ival = val;
    return expr;
}

Expr* expr_float(double val)
{
    Expr* expr = expr_new(EXPR_FLOAT);
    expr->fval = val;
    return expr;
}

Expr* expr_str(const char* str)
{
    Expr* expr = expr_new(EXPR_STR);
    expr->strval = str;
    return expr;
}

Expr* expr_name(const char* name)
{
    Expr* expr = expr_new(EXPR_NAME);
    expr->name = name;
    return expr;
}

Expr* expr_cast(Typespec* type, Expr* expr)
{
    Expr* e = expr_new(EXPR_CAST);
    e->cast.type = type;
    e->cast.expr = expr;
    return e;
}

Expr* expr_call(Expr* expr, Expr** args, size_t num_args)
{
    Expr* e = expr_new(EXPR_CALL);
    e->call.expr = expr;
    e->call.args = args;
    e->call.num_args = num_args;
    return e;
}

Expr* expr_index(Expr* expr, Expr* index)
{
    Expr* e = expr_new(EXPR_INDEX);
    e->index.expr = expr;
    e->index.index = index;
    return e;
}

Expr* expr_field(Expr* expr, const char* name)
{
    Expr* e = expr_new(EXPR_FIELD);
    e->field.expr = expr;
    e->field.name = name;
    return e;
}

Expr* expr_compound(Typespec* type, Expr** args, size_t num_args)
{
    Expr* expr = expr_new(EXPR_COMPOUND);
    expr->compound.type = type;
    expr->compound.args = args;
    expr->compound.num_args = num_args;
    return expr;
}

Expr* expr_unary(TokenType op, Expr* expr)
{
    Expr* e = expr_new(EXPR_UNARY);
    e->unary.op = op;
    e->unary.expr = expr;
    return e;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right)
{
    Expr* expr = expr_new(EXPR_BINARY);
    expr->binary.op = op;
    expr->binary.left = left;
    expr->binary.right = right;
    return expr;
}

Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr)
{
    Expr* expr = expr_new(EXPR_TERNARY);
    expr->ternary.cond = cond;
    expr->ternary.then_expr = then_expr;
    expr->ternary.else_expr = else_expr;
    return expr;
}

Expr* expr_sizeof_expr(Expr* expr)
{
    Expr* e = expr_new(EXPR_SIZEOF_EXPR);
    e->sizeof_expr = expr;
    return e;
}

Expr* expr_sizeof_type(Typespec* type)
{
    Expr* expr = expr_new(EXPR_SIZEOF_TYPE);
    expr->sizeof_type = type;
    return expr;
}