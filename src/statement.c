#include "statement.h"

#include "runtime.h"
#include "parser.h"

static int cw_parse_stmt_expr(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_SEMICOLON, "Expect terminator after expression.");
    cw_emit_byte(cw, OP_POP);
}

static int cw_parse_stmt_block(cwRuntime* cw)
{
    cw->scope_depth++; /* begin scope */

    while (cw->current.type != TOKEN_RBRACE && cw->current.type != TOKEN_EOF)
        cw_parse_declaration(cw);

    /* end scope */
    cw_consume(cw, TOKEN_RBRACE, "Expect '}' after block.");
    cw->scope_depth--;

    while (cw->local_count > 0 && cw->locals[cw->local_count - 1].depth > cw->scope_depth)
    {
        cw_emit_byte(cw, OP_POP);
        cw->local_count--;
    }
}

static int cw_parse_stmt_if(cwRuntime* cw)
{
    cw_consume(cw, TOKEN_LPAREN, "Expect '(' after if statement.");
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
    if (cw_match(cw, TOKEN_PRINT))      return cw_parse_stmt_print(cw);
    if (cw_match(cw, TOKEN_LBRACE))     return cw_parse_stmt_block(cw);

    return cw_parse_stmt_expr(cw);
}


