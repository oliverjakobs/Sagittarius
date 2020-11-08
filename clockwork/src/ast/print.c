#include "print.h"

static int indent;

static void print_newline()
{
    printf("\n%.*s", 2 * indent, "                                                                      ");
}

void print_typespec(Typespec* type)
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
            print_typespec(func.args[i]);
        }
        printf(") ");
        print_typespec(func.ret);
        printf(")");
        break;
    }
    case TYPESPEC_ARRAY:
        printf("(arr ");
        print_typespec(type->array.elem);
        printf(" ");
        print_expr(type->array.size);
        printf(")");
        break;
    case TYPESPEC_POINTER:
        printf("(ptr ");
        print_typespec(type->ptr.elem);
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
        print_typespec(expr->cast.type);
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
        printf("(compound ");
        if (expr->compound.type)
            print_typespec(expr->compound.type);
        else
            printf("nil");

        for (size_t i = 0; i < expr->compound.num_args; ++i)
        {
            printf(" ");
            print_expr(expr->compound.args[i]);
        }
        printf(")");
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

static void print_stmt_block(StmtBlock block, bool newlines)
{
    assert(block.num_stmts != 0);
    printf("(block");
    indent++;
    for (Stmt** it = block.stmts; it != block.stmts + block.num_stmts; it++)
    {
        if (newlines)
            print_newline();
        else
            printf(" ");

        print_stmt(*it);
    }
    indent--;
    printf(")");
}

void print_stmt(Stmt* stmt)
{
    switch (stmt->type)
    {
    case STMT_RETURN:
        printf("(return ");
        print_expr(stmt->return_stmt.expr);
        printf(")");
        break;
    case STMT_BREAK:
        printf("(break)");
        break;
    case STMT_CONTINUE:
        printf("(continue)");
        break;
    case STMT_BLOCK:
        print_stmt_block(stmt->block, true);
        break;
    case STMT_IF:
        printf("(if ");
        print_expr(stmt->if_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->if_stmt.then_block, true);
        for (ElseIf* it = stmt->if_stmt.elseifs; it != stmt->if_stmt.elseifs + stmt->if_stmt.num_elseifs; it++)
        {
            print_newline();
            printf("elseif ");
            print_expr(it->cond);
            print_newline();
            print_stmt_block(it->block, true);
        }
        if (stmt->if_stmt.else_block.num_stmts != 0)
        {
            print_newline();
            printf("else ");
            print_newline();
            print_stmt_block(stmt->if_stmt.else_block, true);
        }
        printf(")");
        indent--;
        break;
    case STMT_WHILE:
        printf("(while ");
        print_expr(stmt->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->while_stmt.block, true);
        indent--;
        printf(")");
        break;
    case STMT_DO_WHILE:
        printf("(do-while ");
        print_expr(stmt->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->while_stmt.block, true);
        indent--;
        printf(")");
        break;
    case STMT_FOR:
        printf("(for ");
        print_stmt_block(stmt->for_stmt.init, false);
        print_expr(stmt->for_stmt.cond);
        print_stmt_block(stmt->for_stmt.next, false);
        indent++;
        print_newline();
        print_stmt_block(stmt->for_stmt.block, true);
        indent--;
        break;
    case STMT_SWITCH:
        printf("(switch ");
        print_expr(stmt->switch_stmt.expr);
        indent++;
        for (SwitchCase* it = stmt->switch_stmt.cases; it != stmt->switch_stmt.cases + stmt->switch_stmt.num_cases; it++)
        {
            print_newline();
            printf("(case (");
            if (it->is_default)
                printf("default");
            else
                printf("nil");

            for (Expr** expr = it->exprs; expr != it->exprs + it->num_exprs; expr++)
            {
                printf(" ");
                print_expr(*expr);
            }
            printf(") ");
            indent++;
            print_newline();
            print_stmt_block(it->block, true);
            indent--;
        }
        printf(")");
        indent--;
        break;
    case STMT_ASSIGN:
        printf("(%s ", token_type_name(stmt->assign.op));
        print_expr(stmt->assign.left);
        printf(" ");
        print_expr(stmt->assign.right);
        printf(")");
        break;
    case STMT_AUTO:
        printf("(:= %s ", stmt->auto_stmt.name);
        print_expr(stmt->auto_stmt.expr);
        printf(")");
        break;
    case STMT_EXPR:
        print_expr(stmt->expr);
        break;
    default:
        assert(0);
        break;
    }
}

static void print_aggregate_decl(Decl* decl)
{
    for (AggregateItem* it = decl->aggregate_decl.items; it != decl->aggregate_decl.items + decl->aggregate_decl.num_items; it++)
    {
        print_newline();
        printf("(");
        print_typespec(it->type);
        for (const char** name = it->names; name != it->names + it->num_names; name++)
            printf(" %s", *name);

        printf(")");
    }
}

void print_decl(Decl* decl)
{
    switch (decl->type)
    {
    case DECL_ENUM:
        printf("(enum %s", decl->name);
        indent++;
        for (EnumItem* it = decl->enum_decl.items; it != decl->enum_decl.items + decl->enum_decl.num_items; it++)
        {
            print_newline();
            printf("(%s ", it->name);
            if (it->expr)
                print_expr(it->expr);
            else
                printf("nil");

            printf(")");
        }
        indent--;
        printf(")");
        break;
    case DECL_STRUCT:
        printf("(struct %s", decl->name);
        indent++;
        print_aggregate_decl(decl);
        indent--;
        printf(")");
        break;
    case DECL_UNION:
        printf("(union %s", decl->name);
        indent++;
        print_aggregate_decl(decl);
        indent--;
        printf(")");
        break;
    case DECL_VAR:
        printf("(var %s ", decl->name);
        print_typespec(decl->var_decl.type);
        printf(" ");
        print_expr(decl->var_decl.expr);
        printf(")");
        break;
    case DECL_CONST:
        printf("(const %s ", decl->name);
        print_expr(decl->var_decl.expr);
        printf(")");
        break;
    case DECL_TYPEDEF:
        printf("(typedef %s ", decl->name);
        print_typespec(decl->typedef_decl.type);
        printf(")");
        break;
    case DECL_FUNC:
        printf("(func %s ", decl->name);
        printf("(");
        for (FuncParam* it = decl->func_decl.params; it != decl->func_decl.params + decl->func_decl.num_params; it++)
        {
            printf(" %s", it->name);
            print_typespec(it->type);
        }
        printf(" ) ");
        print_typespec(decl->func_decl.ret_type);
        indent++;
        print_newline();
        print_stmt_block(decl->func_decl.block, true);
        indent--;
        printf(")");
        break;
    default:
        assert(0);
        break;
    }
}
