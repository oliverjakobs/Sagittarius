#ifndef PARSE_H
#define PARSE_H

#include "ast/typespec.h"
#include "ast/decl.h"
#include "ast/stmt.h"
#include "ast/expr.h"

Typespec* parse_typespec();
Decl* parse_decl();
Stmt* parse_stmt();
Expr* parse_expr();

#endif // !PARSE_H
