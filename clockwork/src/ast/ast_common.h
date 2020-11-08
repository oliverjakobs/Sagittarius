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

static void* ast_alloc(size_t size)
{
    assert(size != 0);
    void* block = malloc(size);

    if (!block)
    {
        perror("ast_alloc failed");
        exit(1);
    }

    memset(block, 0, size);
    return block;
}

static void* ast_dup(const void* src, size_t size)
{
    if (size == 0) return NULL;

    void* block = malloc(size);

    if (!block)
    {
        perror("ast_dup failed");
        exit(1);
    }

    memcpy(block, src, size);
    return block;
}

#endif // !AST_COMMON_H
