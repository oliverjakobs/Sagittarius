#include "print.h"

static int indent;

static char* print_buf = NULL;
static bool use_print_buf;

#define PRINT(s, ...) (use_print_buf ? (void)tb_stretchy_printf(&print_buf, s, __VA_ARGS__) : (void)printf(s, __VA_ARGS__))

void print_to_buf(bool b)
{
    use_print_buf = b;
}

void flush_print_buf(FILE* file)
{
    if (print_buf)
    {
        if (file) fputs(print_buf, file);
        tb_stretchy_clear(print_buf);
    }
}

static void print_newline()
{
    PRINT("\n%.*s", 2 * indent, "                                                                      ");
}

void print_typespec(Typespec* type)
{
    switch (type->type)
    {
    case TYPESPEC_NAME:
        PRINT("%s", type->name);
        break;
    case TYPESPEC_FUNC:
    {
        FuncTypespec func = type->func;
        PRINT("(func (");
        for (size_t i = 0; i < func.num_args; ++i)
        {
            PRINT(" ");
            print_typespec(func.args[i]);
        }
        PRINT(") ");
        print_typespec(func.ret);
        PRINT(")");
        break;
    }
    case TYPESPEC_ARRAY:
        PRINT("(arr ");
        print_typespec(type->array.elem);
        PRINT(" ");
        print_expr(type->array.size);
        PRINT(")");
        break;
    case TYPESPEC_POINTER:
        PRINT("(ptr ");
        print_typespec(type->ptr.elem);
        PRINT(")");
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
        PRINT("%llu", expr->ival);
        break;
    case EXPR_FLOAT:
        PRINT("%f", expr->fval);
        break;
    case EXPR_STR:
        PRINT("\"%s\"", expr->strval);
        break;
    case EXPR_NAME:
        PRINT("%s", expr->name);
        break;
    case EXPR_CAST:
        PRINT("(cast ");
        print_typespec(expr->cast.type);
        PRINT(" ");
        print_expr(expr->cast.expr);
        PRINT(")");
        break;
    case EXPR_CALL:
        PRINT("(");
        print_expr(expr->call.expr);
        for (size_t i = 0; i < expr->call.num_args; ++i)
        {
            PRINT(" ");
            print_expr(expr->call.args[i]);
        }
        PRINT(")");
        break;
    case EXPR_INDEX:
        PRINT("(index ");
        print_expr(expr->index.expr);
        PRINT(" ");
        print_expr(expr->index.index);
        PRINT(")");
        break;
    case EXPR_FIELD:
        PRINT("(field ");
        print_expr(expr->field.expr);
        PRINT(" %s)", expr->field.name);
        break;
    case EXPR_COMPOUND:
        PRINT("(compound ");
        if (expr->compound.type)
            print_typespec(expr->compound.type);
        else
            PRINT("nil");

        for (size_t i = 0; i < expr->compound.num_args; ++i)
        {
            PRINT(" ");
            print_expr(expr->compound.args[i]);
        }
        PRINT(")");
        break;
    case EXPR_UNARY:
        PRINT("(%s ", temp_token_type_str(expr->unary.op));
        print_expr(expr->unary.expr);
        PRINT(")");
        break;
    case EXPR_BINARY:
        PRINT("(%s ", temp_token_type_str(expr->binary.op));
        print_expr(expr->binary.left);
        PRINT(" ");
        print_expr(expr->binary.right);
        PRINT(")");
        break;
    case EXPR_TERNARY:
        PRINT("(if ");
        print_expr(expr->ternary.cond);
        PRINT(" ");
        print_expr(expr->ternary.then_expr);
        PRINT(" ");
        print_expr(expr->ternary.else_expr);
        PRINT(")");
        break;
    case EXPR_SIZEOF_EXPR:
        PRINT("(sizeof ");
        print_expr(expr->sizeof_expr);
        PRINT(")");
        break;
    case EXPR_SIZEOF_TYPE:
        PRINT("(sizeof ");
        print_typespec(expr->sizeof_type);
        PRINT(")");
        break;
    default:
        assert(0);
        break;
    }
}

static void print_stmt_block(StmtBlock block, bool newlines)
{
    assert(block.num_stmts != 0);
    PRINT("(block");
    indent++;
    for (Stmt** it = block.stmts; it != block.stmts + block.num_stmts; it++)
    {
        if (newlines)
            print_newline();
        else
            PRINT(" ");

        print_stmt(*it);
    }
    indent--;
    PRINT(")");
}

void print_stmt(Stmt* stmt)
{
    switch (stmt->type)
    {
    case STMT_RETURN:
        PRINT("(return ");
        print_expr(stmt->return_stmt.expr);
        PRINT(")");
        break;
    case STMT_BREAK:
        PRINT("(break)");
        break;
    case STMT_CONTINUE:
        PRINT("(continue)");
        break;
    case STMT_BLOCK:
        print_stmt_block(stmt->block, true);
        break;
    case STMT_IF:
        PRINT("(if ");
        print_expr(stmt->if_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->if_stmt.then_block, true);
        for (ElseIf* it = stmt->if_stmt.elseifs; it != stmt->if_stmt.elseifs + stmt->if_stmt.num_elseifs; it++)
        {
            print_newline();
            PRINT("elseif ");
            print_expr(it->cond);
            print_newline();
            print_stmt_block(it->block, true);
        }
        if (stmt->if_stmt.else_block.num_stmts != 0)
        {
            print_newline();
            PRINT("else ");
            print_newline();
            print_stmt_block(stmt->if_stmt.else_block, true);
        }
        PRINT(")");
        indent--;
        break;
    case STMT_WHILE:
        PRINT("(while ");
        print_expr(stmt->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->while_stmt.block, true);
        indent--;
        PRINT(")");
        break;
    case STMT_DO_WHILE:
        PRINT("(do-while ");
        print_expr(stmt->while_stmt.cond);
        indent++;
        print_newline();
        print_stmt_block(stmt->while_stmt.block, true);
        indent--;
        PRINT(")");
        break;
    case STMT_FOR:
        PRINT("(for ");
        print_stmt(stmt->for_stmt.init);
        print_expr(stmt->for_stmt.cond);
        print_stmt(stmt->for_stmt.next);
        indent++;
        print_newline();
        print_stmt_block(stmt->for_stmt.block, true);
        indent--;
        break;
    case STMT_SWITCH:
        PRINT("(switch ");
        print_expr(stmt->switch_stmt.expr);
        indent++;
        for (SwitchCase* it = stmt->switch_stmt.cases; it != stmt->switch_stmt.cases + stmt->switch_stmt.num_cases; it++)
        {
            print_newline();
            PRINT("(case (");
            if (it->is_default)
                PRINT("default");
            else
                PRINT("nil");

            for (Expr** expr = it->exprs; expr != it->exprs + it->num_exprs; expr++)
            {
                PRINT(" ");
                print_expr(*expr);
            }
            PRINT(") ");
            indent++;
            print_newline();
            print_stmt_block(it->block, true);
            indent--;
        }
        PRINT(")");
        indent--;
        break;
    case STMT_ASSIGN:
        PRINT("(%s ", token_type_name(stmt->assign.op));
        print_expr(stmt->assign.left);
        if (stmt->assign.right)
        {
            PRINT(" ");
            print_expr(stmt->assign.right);
        }
        PRINT(")");
        break;
    case STMT_AUTO:
        PRINT("(:= %s ", stmt->auto_stmt.name);
        print_expr(stmt->auto_stmt.expr);
        PRINT(")");
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
        PRINT("(");
        print_typespec(it->type);
        for (const char** name = it->names; name != it->names + it->num_names; name++)
            PRINT(" %s", *name);

        PRINT(")");
    }
}

void print_decl(Decl* decl)
{
    switch (decl->type)
    {
    case DECL_ENUM:
        PRINT("(enum %s", decl->name);
        indent++;
        for (EnumItem* it = decl->enum_decl.items; it != decl->enum_decl.items + decl->enum_decl.num_items; it++)
        {
            print_newline();
            PRINT("(%s ", it->name);
            if (it->expr)
                print_expr(it->expr);
            else
                PRINT("nil");

            PRINT(")");
        }
        indent--;
        PRINT(")");
        break;
    case DECL_STRUCT:
        PRINT("(struct %s", decl->name);
        indent++;
        print_aggregate_decl(decl);
        indent--;
        PRINT(")");
        break;
    case DECL_UNION:
        PRINT("(union %s", decl->name);
        indent++;
        print_aggregate_decl(decl);
        indent--;
        PRINT(")");
        break;
    case DECL_VAR:
        PRINT("(var %s ", decl->name);
        if (decl->var_decl.type)
            print_typespec(decl->var_decl.type);
        else
            PRINT("nil");
        PRINT(" ");
        print_expr(decl->var_decl.expr);
        PRINT(")");
        break;
    case DECL_CONST:
        PRINT("(const %s ", decl->name);
        print_expr(decl->const_decl.expr);
        PRINT(")");
        break;
    case DECL_TYPEDEF:
        PRINT("(typedef %s ", decl->name);
        print_typespec(decl->typedef_decl.type);
        PRINT(")");
        break;
    case DECL_FUNC:
        PRINT("(func %s ", decl->name);
        PRINT("(");
        for (FuncParam* it = decl->func_decl.params; it != decl->func_decl.params + decl->func_decl.num_params; it++)
        {
            PRINT(" %s", it->name);
            print_typespec(it->type);
        }
        PRINT(" ) ");
        if (decl->func_decl.ret_type)
            print_typespec(decl->func_decl.ret_type);
        else
            PRINT("nil");
        indent++;
        print_newline();
        print_stmt_block(decl->func_decl.block, true);
        indent--;
        PRINT(")");
        break;
    default:
        assert(0);
        break;
    }
}
