#ifndef AST_PRINT_H
#define AST_PRINT_H

#include "stmt.h"
#include "expr.h"
#include "decl.h"
#include "typespec.h"

void print_typespec(Typespec* type);

void print_expr(Expr* expr);

void print_stmt(Stmt* stmt);

void print_decl(Decl* decl);

#endif // !AST_PRINT_H
