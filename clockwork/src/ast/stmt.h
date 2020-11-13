#ifndef AST_STMT_H
#define AST_STMT_H

#include "ast_common.h"

typedef enum
{
    STMT_NONE,
    STMT_DECL,
    STMT_EXPR,
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
} StmtType;

typedef struct
{
    Expr* cond;
    StmtList block;
} ElseIf;

typedef struct
{
    Expr** exprs;
    size_t num_exprs;
    bool is_default;
    StmtList block;
} SwitchCase;

struct Stmt
{
    StmtType type;
    union
    {
        Expr* expr;
        Decl* decl;
        StmtList block;
        struct
        {
            Expr* cond;
            StmtList then_block;
            ElseIf* elseifs;
            size_t num_elseifs;
            StmtList else_block;
        } if_stmt;
        struct
        {
            Expr* cond;
            StmtList block;
        } while_stmt;
        struct
        {
            Stmt* init;
            Expr* cond;
            Stmt* next;
            StmtList block;
        } for_stmt;
        struct
        {
            Expr* expr;
            SwitchCase* cases;
            size_t num_cases;
        } switch_stmt;
        struct
        {
            TokenType op;
            Expr* left;
            Expr* right;
        } assign;
        struct
        {
            const char* name;
            Expr* expr;
        } auto_stmt;
    };
};

Stmt* stmt_decl(Decl* decl);
Stmt* stmt_return(Expr* expr);
Stmt* stmt_break();
Stmt* stmt_continue();
Stmt* stmt_block(StmtList block);
Stmt* stmt_if(Expr* cond, StmtList then_block, ElseIf* elseifs, size_t num_elseifs, StmtList else_block);
Stmt* stmt_while(Expr* cond, StmtList block);
Stmt* stmt_do_while(Expr* cond, StmtList block);
Stmt* stmt_for(Stmt* init, Expr* cond, Stmt* next, StmtList block);
Stmt* stmt_switch(Expr* expr, SwitchCase* cases, size_t num_cases);
Stmt* stmt_assign(TokenType op, Expr* left, Expr* right);
Stmt* stmt_auto(const char* name, Expr* expr);
Stmt* stmt_expr(Expr* expr);

#endif // !AST_STMT_H
