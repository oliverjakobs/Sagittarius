#ifndef PRINT_H
#define PRINT_H

#include "ast/stmt.h"
#include "ast/expr.h"
#include "ast/decl.h"
#include "ast/typespec.h"

#include "token.h"

void print_to_buf(bool b);
void flush_print_buf(FILE* file);

void print_typespec(Typespec* type);

void print_expr(Expr* expr);

void print_stmt(Stmt* stmt);

void print_decl(Decl* decl);

void print_token(Token token);

#endif // !PRINT_H
