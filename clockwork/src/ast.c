#include "ast.h"

static Expr* expr_alloc(ExprType type)
{
    Expr* expr = calloc(1, sizeof(Expr));

    if (!expr)
    {
        perror("expr_alloc failed");
        exit(1);
    }

    expr->type = type;
    return expr;
}

Expr* expr_int(uint64_t val)
{
    Expr* expr = expr_alloc(EXPR_INT);
    expr->ival = val;
    return expr;
}

Expr* expr_float(double val)
{
    Expr* expr = expr_alloc(EXPR_FLOAT);
    expr->fval = val;
    return expr;
}

Expr* expr_str(const char* str)
{
    Expr* expr = expr_alloc(EXPR_STR);
    expr->strval = str;
    return expr;
}

Expr* expr_name(const char* name)
{
    Expr* expr = expr_alloc(EXPR_NAME);
    expr->name = name;
    return expr;
}

Expr* expr_cast(Typespec* type, Expr* expr)
{
    Expr* new_expr = expr_alloc(EXPR_CAST);
    new_expr->cast_type = type;
    new_expr->cast_expr = expr;
    return new_expr;
}

Expr* expr_unary(TokenType op, Expr* operand)
{
    Expr* expr = expr_alloc(EXPR_UNARY);
    expr->op = op;
    expr->operand = operand;
    return expr;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right)
{
    Expr* expr = expr_alloc(EXPR_BINARY);
    expr->op = op;
    expr->left = left;
    expr->right = right;
    return expr;
}

Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr)
{
    Expr* expr = expr_alloc(EXPR_TERNARY);
    expr->cond = cond;
    expr->then_expr = then_expr;
    expr->else_expr = else_expr;
    return expr;
}

static void print_type(Typespec* type)
{
    switch (type->type)
    {
    case TYPESPEC_NAME:
        printf("%s", type->name);
        break;
    case TYPESPEC_FUNC:
    {
        FuncTypespec func = type->func;
        printf("(func (");
        for (Typespec** it = func.args; it != tb_stretchy_last(func.args); it++)
        {
            printf(" ");
            print_type(*it);
        }
        printf(") ");
        print_type(func.ret);
        printf(")");
        break;
    }
    case TYPESPEC_ARRAY:
        printf("(arr ");
        print_type(type->base);
        printf(" ");
        print_expr(type->size);
        printf(")");
        break;
    case TYPESPEC_POINTER:
        printf("(ptr ");
        print_type(type->base);
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}

void print_expr(Expr* expr)
{
    switch (expr->type)
    {
    case EXPR_INT:
        printf("%llu", expr->ival);
        break;
    case EXPR_FLOAT:
        printf("%f", expr->fval);
        break;
    case EXPR_STR:
        printf("\"%s\"", expr->strval);
        break;
    case EXPR_NAME:
        printf("%s", expr->name);
        break;
    case EXPR_CAST:
        printf("(cast ");
        print_type(expr->cast_type);
        printf(" ");
        print_expr(expr->cast_expr);
        printf(")");
        break;
    case EXPR_CALL:
        printf("(");
        print_expr(expr->operand);
        for (Expr** it = expr->args; it != tb_stretchy_last(expr->args); it++)
        {
            printf(" ");
            print_expr(*it);
        }
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(index ");
        print_expr(expr->operand);
        printf(" ");
        print_expr(expr->index);
        printf(")");
        break;
    case EXPR_FIELD:
        printf("(field ");
        print_expr(expr->operand);
        printf(" %s)", expr->field);
    case EXPR_COMPUND:
        printf("(compund ...)");
        break;
    case EXPR_UNARY:
        printf("(%c ", expr->op);
        print_expr(expr->operand);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(%c ", expr->op);
        print_expr(expr->left);
        printf(" ");
        print_expr(expr->right);
        printf(")");
        break;
    case EXPR_TERNARY:
        printf("(ternary ");
        print_expr(expr->cond);
        printf(" ");
        print_expr(expr->then_expr);
        printf(" ");
        print_expr(expr->else_expr);
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}