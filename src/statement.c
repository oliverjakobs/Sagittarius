#include "statement.h"

#include "parser.h"

#include "debug.h"
#include "runtime.h"

/* --------------------------| declarations |-------------------------------------------- */
static void cw_parse_decl_var(cwRuntime* cw)
{
    /* parse variable name */
    cw_consume(cw, TOKEN_IDENTIFIER, "Expect variable name.");

    /* declare variable */
    if (cw->scope_depth > 0)
    {
        cwToken* name = &cw->previous;
        for (int i = cw->local_count - 1; i >= 0; i--)
        {
            cwLocal* local = &cw->locals[i];
            if (local->depth != -1 && local->depth < cw->scope_depth) break;

            if (cw_identifiers_equal(name, &local->name))
                cw_syntax_error_at(cw, &cw->previous, "Already a variable with this name in this scope.");
        }

        cw_add_local(cw, name);
    }
    
    uint8_t id = (cw->scope_depth <= 0) ? cw_identifier_constant(cw, &cw->previous) : 0;

    /* parse variable initialization value */
    if (cw_match(cw, TOKEN_ASSIGN)) cw_parse_expression(cw);
    else                            cw_syntax_error_at(cw, &cw->previous, "Undefined variable.");

    /* define variable */
    cw_consume(cw, TOKEN_SEMICOLON, "Expect terminator after var declaration.");
    if (cw->scope_depth > 0)
        cw->locals[cw->local_count - 1].depth = cw->scope_depth; /* mark initialized */
    else
        cw_emit_bytes(cw, OP_DEF_GLOBAL, id);
}

int cw_parse_declaration(cwRuntime* cw)
{
    if (cw_match(cw, TOKEN_LET))    cw_parse_decl_var(cw);
    else                            cw_parse_statement(cw); 

    if (cw->panic) cw_parser_synchronize(cw);

    return 1;
}

/* --------------------------| statements |---------------------------------------------- */
static inline void cw_begin_scope(cwRuntime* cw) { cw->scope_depth++; }
static inline void cw_end_scope(cwRuntime* cw)
{ 
    cw->scope_depth--;

    /* pop locals */
    while (cw->local_count > 0 && cw->locals[cw->local_count - 1].depth > cw->scope_depth)
    {
        cw_emit_byte(cw, OP_POP);
        cw->local_count--;
    }
}

static int cw_parse_stmt_expr(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_SEMICOLON, "Expect terminator after expression.");
    cw_emit_byte(cw, OP_POP);
}

static int cw_parse_stmt_block(cwRuntime* cw)
{
    cw_begin_scope(cw);

    while (cw->current.type != TOKEN_RBRACE && cw->current.type != TOKEN_EOF)
        cw_parse_declaration(cw);

    cw_consume(cw, TOKEN_RBRACE, "Expect '}' after block.");
    cw_end_scope(cw);
}

static int cw_parse_stmt_if(cwRuntime* cw)
{
    cw_consume(cw, TOKEN_LPAREN, "Expect '(' after 'if'.");
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_RPAREN, "Expect ')' after condition.");

    int then_jump = cw_emit_jump(cw, OP_JUMP_IF_FALSE);
    cw_emit_byte(cw, OP_POP);
    cw_parse_statement(cw);

    int else_jump = cw_emit_jump(cw, OP_JUMP);

    cw_patch_jump(cw, then_jump);
    cw_emit_byte(cw, OP_POP);

    if (cw_match(cw, TOKEN_ELSE)) cw_parse_statement(cw);
    cw_patch_jump(cw, else_jump);

    return 1;
}

static int cw_parse_stmt_while(cwRuntime* cw)
{
    int loop_start = cw->chunk->len;

    cw_consume(cw, TOKEN_LPAREN, "Expect '(' after 'while'.");
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_RPAREN, "Expect ')' after condition.");

    int exit_jump = cw_emit_jump(cw, OP_JUMP_IF_FALSE);
    cw_emit_byte(cw, OP_POP);
    cw_parse_statement(cw);
    cw_emit_loop(cw, loop_start);

    cw_patch_jump(cw, exit_jump);
    cw_emit_byte(cw, OP_POP);
}

/* NOTE: maybe switch to "for x in ..." notation */
static int cw_parse_stmt_for(cwRuntime* cw)
{
    cw_begin_scope(cw);
    cw_consume(cw, TOKEN_LPAREN, "Expect '(' after 'for'.");

    /* initializer clause. */
    if (cw_match(cw, TOKEN_SEMICOLON))  { } /* no initializer. */
    else if (cw_match(cw, TOKEN_LET))   cw_parse_decl_var(cw);
    else                                cw_parse_stmt_expr(cw);

    int loop_start = cw->chunk->len;

    /* condition clause. */
    int exit_jump = -1;
    if (!cw_match(cw, TOKEN_SEMICOLON))
    {
        cw_parse_expression(cw);
        cw_consume(cw, TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        /* jump out of the loop if the condition is false. */
        exit_jump = cw_emit_jump(cw, OP_JUMP_IF_FALSE);
        cw_emit_byte(cw, OP_POP); /* pop condition. */
    }
    
    /* increment clause. */
    if (!cw_match(cw, TOKEN_RPAREN))
    {
        int body_jump = cw_emit_jump(cw, OP_JUMP);
        int inc_start = cw->chunk->len;
        cw_parse_expression(cw);
        cw_emit_byte(cw, OP_POP);
        cw_consume(cw, TOKEN_RPAREN, "Expect ')' after for clauses.");

        cw_emit_loop(cw, loop_start);
        loop_start = inc_start;
        cw_patch_jump(cw, body_jump);
    }

    cw_parse_statement(cw);
    cw_emit_loop(cw, loop_start);

    /* patch condition jump. */
    if (exit_jump > 0)
    {
        cw_patch_jump(cw, exit_jump);
        cw_emit_byte(cw, OP_POP); /* pop condition. */
    }

    cw_end_scope(cw);
}

/* NOTE: make print build in function */
static int cw_parse_stmt_print(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_SEMICOLON, "Expect terminator after value.");
    cw_emit_byte(cw, OP_PRINT);
}

/* NOTE: implement error handling in stmts */
/* NOTE: break cw_match open */
int cw_parse_statement(cwRuntime* cw)
{
    if (cw_match(cw, TOKEN_SEMICOLON))  return 0;
    if (cw_match(cw, TOKEN_IF))         return cw_parse_stmt_if(cw);
    if (cw_match(cw, TOKEN_WHILE))      return cw_parse_stmt_while(cw);
    if (cw_match(cw, TOKEN_FOR))        return cw_parse_stmt_for(cw);
    if (cw_match(cw, TOKEN_PRINT))      return cw_parse_stmt_print(cw);
    if (cw_match(cw, TOKEN_LBRACE))     return cw_parse_stmt_block(cw);

    return cw_parse_stmt_expr(cw);
}

/* --------------------------| expression |---------------------------------------------- */
int cw_parse_expression(cwRuntime* cw)
{
    cw_parse_precedence(cw, PREC_ASSIGNMENT);
    return 1;
}