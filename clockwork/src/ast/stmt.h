#ifndef AST_STMT_H
#define AST_STMT_H

#include "ast_common.h"

typedef enum
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_SWITCH,
    STMT_ASSIGN,
    STMT_AUTO,
    STMT_EXPR,
} StmtType;

typedef struct
{
    Expr* expr;
} ReturnStmt;

typedef struct
{
    Expr* cond;
    StmtBlock block;
} ElseIf;

typedef struct
{
    Expr* cond;
    StmtBlock then_block;
    ElseIf* elseifs;
    size_t num_elseifs;
    StmtBlock else_block;
} IfStmt;

typedef struct
{
    Expr* cond;
    StmtBlock block;
} WhileStmt;

typedef struct
{
    StmtBlock init;
    Expr* cond;
    StmtBlock next;
    StmtBlock block;
} ForStmt;

typedef struct
{
    Expr** exprs;
    size_t num_exprs;
    bool is_default;
    StmtBlock block;
} SwitchCase;

typedef struct
{
    Expr* expr;
    SwitchCase* cases;
    size_t num_cases;
} SwitchStmt;

typedef struct
{
    TokenType op;
    Expr* left;
    Expr* right;
} AssignStmt;

typedef struct
{
    const char* name;
    Expr* expr;
} AutoStmt;

struct Stmt
{
    StmtType type;
    union
    {
        ReturnStmt return_stmt;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        SwitchStmt switch_stmt;
        StmtBlock block;
        AssignStmt assign;
        AutoStmt auto_stmt;
        Expr* expr;
    };
};

Stmt* stmt_return(Expr* expr);
Stmt* stmt_break();
Stmt* stmt_continue();
Stmt* stmt_block(StmtBlock block);
Stmt* stmt_if(Expr* cond, StmtBlock then_block, ElseIf* elseifs, size_t num_elseifs, StmtBlock else_block);
Stmt* stmt_while(Expr* cond, StmtBlock block);
Stmt* stmt_do_while(Expr* cond, StmtBlock block);
Stmt* stmt_for(StmtBlock init, Expr* cond, StmtBlock next, StmtBlock block);
Stmt* stmt_switch(Expr* expr, SwitchCase* cases, size_t num_cases);
Stmt* stmt_assign(TokenType op, Expr* left, Expr* right);
Stmt* stmt_auto(const char* name, Expr* expr);
Stmt* stmt_expr(Expr* expr);

#endif // !AST_STMT_H
