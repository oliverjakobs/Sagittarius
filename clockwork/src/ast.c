#include "ast.h"

static Typespec* typespec_alloc(TypespecType type)
{
    Typespec* typespec = calloc(1, sizeof(Typespec));

    if (!typespec)
    {
        perror("typespec_alloc failed");
        exit(1);
    }

    typespec->type = type;
    return typespec;
}

Typespec* typespec_name(const char* name)
{
    Typespec* typespec = typespec_alloc(TYPESPEC_NAME);
    typespec->name = name;
    return typespec;
}

Typespec* typespec_pointer(Typespec* elem)
{
    Typespec* typespec = typespec_alloc(TYPESPEC_POINTER);
    typespec->ptr.elem = elem;
    return typespec;
}

Typespec* typespec_array(Typespec* elem, Expr* size)
{
    Typespec* typespec = typespec_alloc(TYPESPEC_ARRAY);
    typespec->array.elem = elem;
    typespec->array.size = size;
    return typespec;
}

Typespec* typespec_func(Typespec** args, size_t num_args, Typespec* ret)
{
    Typespec* typespec = typespec_alloc(TYPESPEC_FUNC);
    typespec->func.args = args;
    typespec->func.num_args = num_args;
    typespec->func.ret = ret;
    return typespec;
}

static Decl* decl_alloc(DeclType type, const char* name)
{
    Decl* decl = calloc(1, sizeof(Decl));

    if (!decl)
    {
        perror("decl_alloc failed");
        exit(1);
    }

    decl->type = type;
    decl->name = name;
    return decl;
}

Decl* decl_enum(const char* name, EnumItem* items, size_t num_items)
{
    Decl* decl = decl_alloc(DECL_ENUM, name);
    decl->enum_decl.items = items;
    decl->enum_decl.num_items = num_items;
    return decl;
}

Decl* decl_struct(const char* name, AggregateItem* items, size_t num_items)
{
    Decl* decl = decl_alloc(DECL_STRUCT, name);
    decl->aggregate_decl.items = items;
    decl->aggregate_decl.num_items = num_items;
    return decl;
}

Decl* decl_union(const char* name, AggregateItem* items, size_t num_items)
{
    Decl* decl = decl_alloc(DECL_UNION, name);
    decl->aggregate_decl.items = items;
    decl->aggregate_decl.num_items = num_items;
    return decl;
}

Decl* decl_var(const char* name, Typespec* type, Expr* expr)
{
    Decl* decl = decl_alloc(DECL_VAR, name);
    decl->var_decl.type = type;
    decl->var_decl.expr = expr;
    return decl;
}

Decl* decl_func(const char* name, FuncParam* params, size_t num_params, Typespec* ret_type, StmtBlock block)
{
    Decl* decl = decl_alloc(DECL_FUNC, name);
    decl->func_decl.params = params;
    decl->func_decl.num_params = num_params;
    decl->func_decl.ret_type = ret_type;
    decl->func_decl.block = block;
    return decl;
}

Decl* decl_const(const char* name, Expr* expr)
{
    Decl* decl = decl_alloc(DECL_CONST, name);
    decl->const_decl.expr = expr;
    return decl;
}

Decl* decl_typedef(const char* name, Typespec* type)
{
    Decl* decl = decl_alloc(DECL_TYPEDEF, name);
    decl->typedef_decl.type = type;
    return decl;
}

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
    Expr* e = expr_alloc(EXPR_CAST);
    e->cast.type = type;
    e->cast.expr = expr;
    return e;
}

Expr* expr_call(Expr* expr, Expr** args, size_t num_args)
{
    Expr* e = expr_alloc(EXPR_CALL);
    e->call.expr = expr;
    e->call.args = args;
    e->call.num_args = num_args;
    return e;
}

Expr* expr_index(Expr* expr, Expr* index)
{
    Expr* e = expr_alloc(EXPR_INDEX);
    e->index.expr = expr;
    e->index.index = index;
    return e;
}

Expr* expr_field(Expr* expr, const char* name)
{
    Expr* e = expr_alloc(EXPR_FIELD);
    e->field.expr = expr;
    e->field.name = name;
    return e;
}

Expr* expr_compound(Typespec* type, Expr** args, size_t num_args)
{
    Expr* expr = expr_alloc(EXPR_COMPOUND);
    expr->compound.type = type;
    expr->compound.args = args;
    expr->compound.num_args = num_args;
    return expr;
}

Expr* expr_unary(TokenType op, Expr* expr)
{
    Expr* e = expr_alloc(EXPR_UNARY);
    e->unary.op = op;
    e->unary.expr = expr;
    return e;
}

Expr* expr_binary(TokenType op, Expr* left, Expr* right)
{
    Expr* expr = expr_alloc(EXPR_BINARY);
    expr->binary.op = op;
    expr->binary.left = left;
    expr->binary.right = right;
    return expr;
}

Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr)
{
    Expr* expr = expr_alloc(EXPR_TERNARY);
    expr->ternary.cond = cond;
    expr->ternary.then_expr = then_expr;
    expr->ternary.else_expr = else_expr;
    return expr;
}

static Stmt* stmt_alloc(StmtType type)
{
    Stmt* stmt = calloc(1, sizeof(Stmt));

    if (!stmt)
    {
        perror("stmt_alloc failed");
        exit(1);
    }

    stmt->type = type;
    return stmt;
}

Stmt* stmt_return(Expr* expr)
{
    Stmt* stmt = stmt_alloc(STMT_RETURN);
    stmt->return_stmt.expr = expr;
    return stmt;
}

Stmt* stmt_break()
{
    return stmt_alloc(STMT_BREAK);
}

Stmt* stmt_continue()
{
    return stmt_alloc(STMT_CONTINUE);
}

Stmt* stmt_block(StmtBlock block)
{
    Stmt* stmt = stmt_alloc(STMT_BLOCK);
    stmt->block = block;
    return stmt;
}

Stmt* stmt_if(Expr* cond, StmtBlock then_block, ElseIf* elseifs, size_t num_elseifs, StmtBlock else_block)
{
    Stmt* stmt = stmt_alloc(STMT_IF);
    stmt->if_stmt.cond = cond;
    stmt->if_stmt.then_block = then_block;
    stmt->if_stmt.elseifs = elseifs;
    stmt->if_stmt.num_elseifs = num_elseifs;
    stmt->if_stmt.else_block = else_block;
    return stmt;
}

Stmt* stmt_while(Expr* cond, StmtBlock block)
{
    Stmt* stmt = stmt_alloc(STMT_WHILE);
    stmt->while_stmt.cond = cond;
    stmt->while_stmt.block = block;
    return stmt;
}

Stmt* stmt_do_while(Expr* cond, StmtBlock block)
{
    Stmt* stmt = stmt_alloc(STMT_DO_WHILE);
    stmt->while_stmt.cond = cond;
    stmt->while_stmt.block = block;
    return stmt;
}

Stmt* stmt_for(StmtBlock init, Expr* cond, StmtBlock next, StmtBlock block)
{
    Stmt* stmt = stmt_alloc(STMT_FOR);
    stmt->for_stmt.init = init;
    stmt->for_stmt.cond = cond;
    stmt->for_stmt.next = next;
    stmt->for_stmt.block = block;
    return stmt;
}

Stmt* stmt_switch(Expr* expr, SwitchCase* cases, size_t num_cases)
{
    Stmt* stmt = stmt_alloc(STMT_SWITCH);
    stmt->switch_stmt.expr = expr;
    stmt->switch_stmt.cases = cases;
    stmt->switch_stmt.num_cases = num_cases;
    return stmt;
}

Stmt* stmt_assign(TokenType op, Expr* left, Expr* right)
{
    Stmt* stmt = stmt_alloc(STMT_ASSIGN);
    stmt->assign.op = op;
    stmt->assign.left = left;
    stmt->assign.right = right;
    return stmt;
}

Stmt* stmt_init(const char* name, Expr* expr)
{
    Stmt* stmt = stmt_alloc(STMT_INIT);
    stmt->init.name = name;
    stmt->init.expr = expr;
    return stmt;
}

Stmt* stmt_expr(Expr* expr)
{
    Stmt* stmt = stmt_alloc(STMT_EXPR);
    stmt->expr = expr;
    return stmt;
}