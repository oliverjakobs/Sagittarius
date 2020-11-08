#include "stmt.h"

static Stmt* stmt_new(StmtType type)
{
    Stmt* stmt = ast_alloc(sizeof(Stmt));
    stmt->type = type;
    return stmt;
}

Stmt* stmt_return(Expr* expr)
{
    Stmt* stmt = stmt_new(STMT_RETURN);
    stmt->return_stmt.expr = expr;
    return stmt;
}

Stmt* stmt_break()
{
    return stmt_new(STMT_BREAK);
}

Stmt* stmt_continue()
{
    return stmt_new(STMT_CONTINUE);
}

Stmt* stmt_block(StmtBlock block)
{
    Stmt* stmt = stmt_new(STMT_BLOCK);
    stmt->block = block;
    return stmt;
}

Stmt* stmt_if(Expr* cond, StmtBlock then_block, ElseIf* elseifs, size_t num_elseifs, StmtBlock else_block)
{
    Stmt* stmt = stmt_new(STMT_IF);
    stmt->if_stmt.cond = cond;
    stmt->if_stmt.then_block = then_block;
    stmt->if_stmt.elseifs = elseifs;
    stmt->if_stmt.num_elseifs = num_elseifs;
    stmt->if_stmt.else_block = else_block;
    return stmt;
}

Stmt* stmt_while(Expr* cond, StmtBlock block)
{
    Stmt* stmt = stmt_new(STMT_WHILE);
    stmt->while_stmt.cond = cond;
    stmt->while_stmt.block = block;
    return stmt;
}

Stmt* stmt_do_while(Expr* cond, StmtBlock block)
{
    Stmt* stmt = stmt_new(STMT_DO_WHILE);
    stmt->while_stmt.cond = cond;
    stmt->while_stmt.block = block;
    return stmt;
}

Stmt* stmt_for(StmtBlock init, Expr* cond, StmtBlock next, StmtBlock block)
{
    Stmt* stmt = stmt_new(STMT_FOR);
    stmt->for_stmt.init = init;
    stmt->for_stmt.cond = cond;
    stmt->for_stmt.next = next;
    stmt->for_stmt.block = block;
    return stmt;
}

Stmt* stmt_switch(Expr* expr, SwitchCase* cases, size_t num_cases)
{
    Stmt* stmt = stmt_new(STMT_SWITCH);
    stmt->switch_stmt.expr = expr;
    stmt->switch_stmt.cases = cases;
    stmt->switch_stmt.num_cases = num_cases;
    return stmt;
}

Stmt* stmt_assign(TokenType op, Expr* left, Expr* right)
{
    Stmt* stmt = stmt_new(STMT_ASSIGN);
    stmt->assign.op = op;
    stmt->assign.left = left;
    stmt->assign.right = right;
    return stmt;
}

Stmt* stmt_auto(const char* name, Expr* expr)
{
    Stmt* stmt = stmt_new(STMT_AUTO);
    stmt->auto_stmt.name = name;
    stmt->auto_stmt.expr = expr;
    return stmt;
}

Stmt* stmt_expr(Expr* expr)
{
    Stmt* stmt = stmt_new(STMT_EXPR);
    stmt->expr = expr;
    return stmt;
}