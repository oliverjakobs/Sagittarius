#include "parse.h"

#include "lex.h"

const char* parse_name()
{
    const char* name = get_token()->name;
    expect_token(TOKEN_NAME);
    return name;
}

/*
 * --------------------------------------------------------------------------------------------------
 * TYPESPEC
 * --------------------------------------------------------------------------------------------------
 */

Typespec* parse_typespec_func()
{
    Typespec** args = NULL;
    expect_token(TOKEN_LPAREN);
    if (!is_token(TOKEN_RPAREN))
    {
        tb_stretchy_push(args, parse_typespec());
        while (match_token(TOKEN_COMMA))
            tb_stretchy_push(args, parse_typespec());
    }
    expect_token(TOKEN_RPAREN);
    Typespec* ret = NULL;
    if (match_token(TOKEN_COMMA))
    {
        ret = parse_typespec();
    }
    return typespec_func(ast_dup(args, tb_stretchy_sizeof(args)), tb_stretchy_size(args), ret);
}

Typespec* parse_typespec_base()
{
    if (is_token(TOKEN_NAME))
    {
        const char* name = get_token()->name;
        next_token();
        return typespec_name(name);
    }
    else if (match_keyword(func_keyword))
    {
        return parse_typespec_func();
    }
    else if (match_token(TOKEN_LPAREN))
    {
        return parse_typespec();
    }
    else 
    {
        fatal_syntax_error("Unexpected token %s in type", temp_token_type_str(get_token_type()));
        return NULL;
    }
}

Typespec* parse_typespec()
{
    Typespec* type = parse_typespec_base();
    while (is_token(TOKEN_LBRACKET) || is_token(TOKEN_MUL))
    {
        if (match_token(TOKEN_LBRACKET))
        {
            Expr* expr = NULL;
            if (!is_token(TOKEN_RBRACKET))
                expr = parse_expr();

            expect_token(TOKEN_RBRACKET);
            type = typespec_array(type, expr);
        }
        else
        {
            assert(is_token(TOKEN_MUL));
            next_token();
            type = typespec_pointer(type);
        }
    }
    return type;
}


/*
 * --------------------------------------------------------------------------------------------------
 * EXPR
 * --------------------------------------------------------------------------------------------------
 */

Expr* parse_expr_compound(Typespec* type)
{
    expect_token(TOKEN_LBRACE);
    Expr** args = NULL;
    if (!is_token(TOKEN_RBRACE))
    {
        tb_stretchy_push(args, parse_expr());
        while (match_token(TOKEN_COMMA))
            tb_stretchy_push(args, parse_expr());
    }
    expect_token(TOKEN_RBRACE);
    return expr_compound(type, ast_dup(args, tb_stretchy_sizeof(args)), tb_stretchy_size(args));
}

Expr* parse_expr_operand()
{
    if (is_token(TOKEN_INT))
    {
        uint64_t val = get_token()->ival;
        next_token();
        return expr_int(val);
    }
    else if (is_token(TOKEN_FLOAT))
    {
        double val = get_token()->fval;
        next_token();
        return expr_float(val);
    }
    else if (is_token(TOKEN_STR))
    {
        const char* val = get_token()->strval;
        next_token();
        return expr_str(val);
    }
    else if (is_token(TOKEN_NAME))
    {
        const char* name = get_token()->name;
        next_token();
        if (is_token(TOKEN_LBRACE))
            return parse_expr_compound(typespec_name(name));

        return expr_name(name);
    }
    else if (match_keyword(sizeof_keyword))
    {
        expect_token(TOKEN_LPAREN);
        if (match_token(TOKEN_COLON))
        {
            Typespec* type = parse_typespec();
            expect_token(TOKEN_RPAREN);
            return expr_sizeof_type(type);
        }
        else
        {
            Expr* expr = parse_expr();
            expect_token(TOKEN_RPAREN);
            return expr_sizeof_expr(expr);
        }
    }
    else if (is_token(TOKEN_LBRACE))
    {
        return parse_expr_compound(NULL);
    }
    else if (match_token(TOKEN_LPAREN))
    {
        if (is_token(TOKEN_COLON))
        {
            Typespec* type = parse_typespec();
            expect_token(TOKEN_RPAREN);
            return parse_expr_compound(type);
        }
        else
        {
            Expr* expr = parse_expr();
            expect_token(TOKEN_RPAREN);
            return expr;
        }
    }
    else
    {
        fatal_syntax_error("Unexpected token %s in expression", temp_token_type_str(get_token_type()));
        return NULL;
    }
}

Expr* parse_expr_base()
{
    Expr* expr = parse_expr_operand();
    while (is_token(TOKEN_LPAREN) || is_token(TOKEN_LBRACKET) || is_token(TOKEN_DOT))
    {
        if (match_token(TOKEN_LPAREN))
        {
            Expr** args = NULL;
            if (!is_token(TOKEN_RPAREN))
            {
                tb_stretchy_push(args, parse_expr());
                while (match_token(TOKEN_COMMA))
                    tb_stretchy_push(args, parse_expr());
            }
            expect_token(TOKEN_RPAREN);
            expr = expr_call(expr, ast_dup(args, tb_stretchy_sizeof(args)), tb_stretchy_size(args));
        }
        else if (match_token(TOKEN_LBRACKET))
        {
            Expr* index = parse_expr();
            expect_token(TOKEN_RBRACKET);
            expr = expr_index(expr, index);
        }
        else
        {
            assert(is_token(TOKEN_DOT));
            next_token();
            const char* field = get_token()->name;
            expect_token(TOKEN_NAME);
            expr = expr_field(expr, field);
        }
    }
    return expr;
}

Expr* parse_expr_unary()
{
    if (is_unary_op())
    {
        TokenType op = get_token_type();
        next_token();
        return expr_unary(op, parse_expr_unary());
    }
    return parse_expr_base();
}

Expr* parse_expr_mul()
{
    Expr* expr = parse_expr_unary();
    while (is_mul_op())
    {
        TokenType op = get_token_type();
        next_token();
        expr = expr_binary(op, expr, parse_expr_unary());
    }
    return expr;
}

Expr* parse_expr_add()
{
    Expr* expr = parse_expr_mul();
    while (is_add_op())
    {
        TokenType op = get_token_type();
        next_token();
        expr = expr_binary(op, expr, parse_expr_mul());
    }
    return expr;
}

Expr* parse_expr_cmp()
{
    Expr* expr = parse_expr_add();
    while (is_cmp_op())
    {
        TokenType op = get_token_type();
        next_token();
        expr = expr_binary(op, expr, parse_expr_add());
    }
    return expr;
}

Expr* parse_expr_and()
{
    Expr* expr = parse_expr_cmp();
    while (match_token(TOKEN_AND))
    {
        expr = expr_binary(TOKEN_AND, expr, parse_expr_cmp());
    }
    return expr;
}

Expr* parse_expr_or()
{
    Expr* expr = parse_expr_and();
    while (match_token(TOKEN_OR))
    {
        expr = expr_binary(TOKEN_OR, expr, parse_expr_and());
    }
    return expr;
}

Expr* parse_expr_ternary()
{
    Expr* expr = parse_expr_or();
    if (match_token(TOKEN_QUESTION))
    {
        Expr* then_expr = parse_expr_ternary();
        expect_token(TOKEN_COLON);
        Expr* else_expr = parse_expr_ternary();
        expr = expr_ternary(expr, then_expr, else_expr);
    }
    return expr;
}

Expr* parse_paren_expr()
{
    expect_token(TOKEN_LPAREN);
    Expr* expr = parse_expr();
    expect_token(TOKEN_RPAREN);
    return expr;
}

Expr* parse_expr()
{
    return parse_expr_ternary();
}

/*
 * --------------------------------------------------------------------------------------------------
 * STMT
 * --------------------------------------------------------------------------------------------------
 */
StmtBlock parse_stmt_block()
{
    expect_token(TOKEN_LBRACE);
    Stmt** stmts = NULL;
    while (!is_token_eof() && !is_token(TOKEN_RBRACE))
        tb_stretchy_push(stmts, parse_stmt());

    expect_token(TOKEN_RBRACE);
    return (StmtBlock) { ast_dup(stmts, tb_stretchy_sizeof(stmts)), tb_stretchy_size(stmts) };
}

Stmt* parse_stmt_if()
{
    Expr* cond = parse_paren_expr();
    StmtBlock then_block = parse_stmt_block();
    StmtBlock else_block = { 0 };
    ElseIf* elseifs = NULL;
    while (match_keyword(else_keyword))
    {
        if (!match_keyword(if_keyword))
        {
            else_block = parse_stmt_block();
            break;
        }

        ElseIf elseif;
        elseif.cond = parse_paren_expr();
        elseif.block = parse_stmt_block();
        tb_stretchy_push(elseifs, elseif);
    }
    return stmt_if(cond, then_block, ast_dup(elseifs, tb_stretchy_sizeof(elseifs)), tb_stretchy_size(elseifs), else_block);
}

Stmt* parse_stmt_while()
{
    Expr* cond = parse_paren_expr();
    return stmt_while(cond, parse_stmt_block());
}

Stmt* parse_stmt_do_while()
{
    StmtBlock block = parse_stmt_block();
    if (!match_keyword(while_keyword))
    {
        fatal_syntax_error("Expected 'while' after 'do' block");
        return NULL;
    }
    Expr* cond = parse_paren_expr();
    Stmt* stmt = stmt_do_while(parse_paren_expr(), block);
    expect_token(TOKEN_SEMICOLON);
    return stmt;
}

Stmt* parse_simple_stmt()
{
    Expr* expr = parse_expr();
    Stmt* stmt;
    if (match_token(TOKEN_COLON_ASSIGN))
    {
        if (expr->type != EXPR_NAME)
            fatal_syntax_error(":= must be preceded by a name");

        stmt = stmt_auto(expr->name, parse_expr());
    }
    else if (is_assign_op())
    {
        TokenType op = get_token_type();
        next_token();
        stmt = stmt_assign(op, expr, parse_expr());
    }
    else if (is_token(TOKEN_INC) || is_token(TOKEN_DEC))
    {
        TokenType op = get_token_type();
        next_token();
        stmt = stmt_assign(op, expr, NULL);
    }
    else
    {
        stmt = stmt_expr(expr);
    }
    return stmt;
}

Stmt* parse_stmt_for()
{
    expect_token(TOKEN_LPAREN);
    Stmt* init = NULL;
    if (!is_token(TOKEN_SEMICOLON))
        init = parse_simple_stmt();

    expect_token(TOKEN_SEMICOLON);
    Expr* cond = NULL;
    if (!is_token(TOKEN_SEMICOLON))
        cond = parse_expr();

    expect_token(TOKEN_SEMICOLON);
    Stmt* next = NULL;
    if (!is_token(TOKEN_RPAREN))
    {
        next = parse_simple_stmt();
        if (next->type == STMT_AUTO)
            syntax_error("Auto statements not allowed in for statement's next clause");
    }
    expect_token(TOKEN_RPAREN);
    return stmt_for(init, cond, next, parse_stmt_block());
}

SwitchCase parse_stmt_switch_case()
{
    Expr** exprs = NULL;
    bool is_default = false;
    while (is_keyword(case_keyword) || is_keyword(default_keyword))
    {
        if (match_keyword(case_keyword))
        {
            tb_stretchy_push(exprs, parse_expr());
        }
        else
        {
            assert(is_keyword(default_keyword));
            next_token();
            if (is_default)
                syntax_error("Duplicate default labels in same switch clause");

            is_default = true;
        }
        expect_token(TOKEN_COLON);
    }
    Stmt** stmts = NULL;
    while (!is_token_eof() && !is_token(TOKEN_RBRACE) && !is_keyword(case_keyword) && !is_keyword(default_keyword))
        tb_stretchy_push(stmts, parse_stmt());

    StmtBlock block = { ast_dup(stmts, tb_stretchy_sizeof(stmts)), tb_stretchy_size(stmts) };
    return (SwitchCase) { ast_dup(exprs, tb_stretchy_sizeof(exprs)), tb_stretchy_size(exprs), is_default, block };
}

Stmt* parse_stmt_switch()
{
    Expr* expr = parse_paren_expr();
    SwitchCase* cases = NULL;
    expect_token(TOKEN_LBRACE);
    while (!is_token_eof() && !is_token(TOKEN_RBRACE))
        tb_stretchy_push(cases, parse_stmt_switch_case());

    expect_token(TOKEN_RBRACE);
    return stmt_switch(expr, ast_dup(cases, tb_stretchy_sizeof(cases)), tb_stretchy_size(cases));
}

Stmt* parse_stmt()
{
    if (match_keyword(if_keyword))
        return parse_stmt_if();
    else if (match_keyword(while_keyword))
        return parse_stmt_while();
    else if (match_keyword(do_keyword))
        return parse_stmt_do_while();
    else if (match_keyword(for_keyword))
        return parse_stmt_for();
    else if (match_keyword(switch_keyword))
        return parse_stmt_switch();
    else if (is_token(TOKEN_LBRACE))
        return stmt_block(parse_stmt_block());
    else if (match_keyword(return_keyword))
    {
        Stmt* stmt = stmt_return(parse_expr());
        expect_token(TOKEN_SEMICOLON);
        return stmt;
    }
    else if (match_keyword(break_keyword))
    {
        expect_token(TOKEN_SEMICOLON);
        return stmt_break();
    }
    else if (match_keyword(continue_keyword))
    {
        expect_token(TOKEN_SEMICOLON);
        return stmt_continue();
    }
    else
    {
        Stmt* stmt = parse_simple_stmt();
        expect_token(TOKEN_SEMICOLON);
        return stmt;
    }
}

/*
 * --------------------------------------------------------------------------------------------------
 * DECL
 * --------------------------------------------------------------------------------------------------
 */
EnumItem parse_decl_enum_item()
{
    const char* name = parse_name();
    Expr* init = NULL;
    if (match_token(TOKEN_ASSIGN))
        init = parse_expr();

    return (EnumItem) { name, init };
}

Decl* parse_decl_enum()
{
    const char* name = parse_name();
    expect_token(TOKEN_LBRACE);
    EnumItem* items = NULL;
    if (!is_token(TOKEN_RBRACE))
    {
        tb_stretchy_push(items, parse_decl_enum_item());
        while (match_token(TOKEN_COMMA))
            tb_stretchy_push(items, parse_decl_enum_item());
    }
    expect_token(TOKEN_RBRACE);
    return decl_enum(name, ast_dup(items, tb_stretchy_sizeof(items)), tb_stretchy_size(items));
}

AggregateItem parse_decl_aggregate_item()
{
    const char** names = NULL;
    tb_stretchy_push(names, parse_name());
    while (match_token(TOKEN_COMMA))
        tb_stretchy_push(names, parse_name());

    expect_token(TOKEN_COLON);
    Typespec* type = parse_typespec();
    expect_token(TOKEN_SEMICOLON);
    return (AggregateItem) { ast_dup(names, tb_stretchy_sizeof(names)), tb_stretchy_size(names), type };
}

Decl* parse_decl_aggregate(DeclType type)
{
    assert(type == DECL_STRUCT || type == DECL_UNION);
    const char* name = parse_name();
    expect_token(TOKEN_LBRACE);
    AggregateItem* items = NULL;
    while (!is_token_eof() && !is_token(TOKEN_RBRACE))
        tb_stretchy_push(items, parse_decl_aggregate_item());

    expect_token(TOKEN_RBRACE);

    if (type == DECL_STRUCT)
        return decl_struct(name, ast_dup(items, tb_stretchy_sizeof(items)), tb_stretchy_size(items));
    else
        return decl_union(name, ast_dup(items, tb_stretchy_sizeof(items)), tb_stretchy_size(items));
}

Decl* parse_decl_var()
{
    const char* name = parse_name();
    if (match_token(TOKEN_ASSIGN))
    {
        return decl_var(name, NULL, parse_expr());
    }
    else if (match_token(TOKEN_COLON))
    {
        Typespec* type = parse_typespec();
        Expr* expr = NULL;
        if (match_token(TOKEN_ASSIGN))
            expr = parse_expr();

        return decl_var(name, type, expr);
    }
    else
    {
        fatal_syntax_error("Expected : or = after var, got %s", temp_token_type_str(get_token_type()));
        return NULL;
    }
}

Decl* parse_decl_const()
{
    const char* name = parse_name();
    expect_token(TOKEN_ASSIGN);
    return decl_const(name, parse_expr());
}

Decl* parse_decl_typedef()
{
    const char* name = parse_name();
    expect_token(TOKEN_ASSIGN);
    return decl_typedef(name, parse_typespec());
}

FuncParam parse_decl_func_param()
{
    const char* name = parse_name();
    expect_token(TOKEN_COLON);
    Typespec* type = parse_typespec();
    return (FuncParam) { name, type };
}

Decl* parse_decl_func()
{
    const char* name = parse_name();
    expect_token(TOKEN_LPAREN);
    FuncParam* params = NULL;
    if (!is_token(TOKEN_RPAREN))
    {
        tb_stretchy_push(params, parse_decl_func_param());
        while (match_token(TOKEN_COMMA))
            tb_stretchy_push(params, parse_decl_func_param());
    }
    expect_token(TOKEN_RPAREN);
    Typespec* ret_type = NULL;
    if (match_token(TOKEN_COLON))
        ret_type = parse_typespec();

    StmtBlock block = parse_stmt_block();
    return decl_func(name, ast_dup(params, tb_stretchy_sizeof(params)), tb_stretchy_size(params), ret_type, block);
}

Decl* parse_decl()
{
    if (match_keyword(enum_keyword))
        return parse_decl_enum();
    else if (match_keyword(struct_keyword))
        return parse_decl_aggregate(DECL_STRUCT);
    else if (match_keyword(union_keyword))
        return parse_decl_aggregate(DECL_UNION);
    else if (match_keyword(var_keyword))
        return parse_decl_var();
    else if (match_keyword(const_keyword))
        return parse_decl_const();
    else if (match_keyword(typedef_keyword))
        return parse_decl_typedef();
    else if (match_keyword(func_keyword))
        return parse_decl_func();

    fatal_syntax_error("Expected declaration keyword, got %s", temp_token_type_str(get_token_type()));
    return NULL;
}