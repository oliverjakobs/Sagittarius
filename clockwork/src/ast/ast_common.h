#ifndef AST_COMMON_H
#define AST_COMMON_H

#include "..\common.h"
#include "..\token.h"

typedef struct Expr Expr;
typedef struct Decl Decl;
typedef struct Stmt Stmt;

typedef struct
{
    Stmt** stmts;
    size_t num_stmts;
} StmtBlock;

typedef struct Typespec Typespec;

void* ast_alloc(size_t size);
void* ast_dup(const void* src, size_t size);

#endif // !AST_COMMON_H
