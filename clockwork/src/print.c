#include "print.h"

void print_type(Typespec* type)
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
        for (size_t i = 0; i < func.num_args; ++i)
        {
            printf(" ");
            print_type(func.args[i]);
        }
        printf(") ");
        print_type(func.ret);
        printf(")");
        break;
    }
    case TYPESPEC_ARRAY:
        printf("(arr ");
        print_type(type->array.elem);
        printf(" ");
        print_expr(type->array.size);
        printf(")");
        break;
    case TYPESPEC_POINTER:
        printf("(ptr ");
        print_type(type->ptr.elem);
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
        print_type(expr->cast.type);
        printf(" ");
        print_expr(expr->cast.expr);
        printf(")");
        break;
    case EXPR_CALL:
        printf("(");
        print_expr(expr->call.expr);
        for (size_t i = 0; i < expr->call.num_args; ++i)
        {
            printf(" ");
            print_expr(expr->call.args[i]);
        }
        printf(")");
        break;
    case EXPR_INDEX:
        printf("(index ");
        print_expr(expr->index.expr);
        printf(" ");
        print_expr(expr->index.index);
        printf(")");
        break;
    case EXPR_FIELD:
        printf("(field ");
        print_expr(expr->field.expr);
        printf(" %s)", expr->field.name);
        break;
    case EXPR_COMPOUND:
        printf("(compound ...)");
        break;
    case EXPR_UNARY:
        printf("(%c ", expr->unary.op);
        print_expr(expr->unary.expr);
        printf(")");
        break;
    case EXPR_BINARY:
        printf("(%c ", expr->binary.op);
        print_expr(expr->binary.left);
        printf(" ");
        print_expr(expr->binary.right);
        printf(")");
        break;
    case EXPR_TERNARY:
        printf("(if ");
        print_expr(expr->ternary.cond);
        printf(" ");
        print_expr(expr->ternary.then_expr);
        printf(" ");
        print_expr(expr->ternary.else_expr);
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}
