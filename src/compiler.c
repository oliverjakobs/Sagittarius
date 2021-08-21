#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

/* ----------------------------------| ERROR |----------------------------------------------- */
static void cw_error_at(cwRuntime* cw, Token* token, const char* msg)
{
    if (cw->panic) return;
    cw->panic = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type != TOKEN_ERROR)
        fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", msg);
    cw->error = true;
}

static void cw_error_at_current(cwRuntime* cw, const char* msg)
{
    cw_error_at(cw, &cw->current, msg);
}

static void cw_error(cwRuntime* cw, const char* msg)
{
    cw_error_at(cw, &cw->previous, msg);
}

/* ----------------------------------| EMIT BYTES |------------------------------------------ */
static void cw_emit_byte(cwRuntime* cw, uint8_t byte)
{
    cw_chunk_write(cw->chunk, byte, cw->previous.line);
}

static void cw_emit_bytes(cwRuntime* cw, uint8_t a, uint8_t b)
{
    cw_emit_byte(cw, a);
    cw_emit_byte(cw, b);
}

static void cw_emit_return(cwRuntime* cw)
{
    cw_emit_byte(cw, OP_RETURN);
}

static void cw_emit_constant(cwRuntime* cw, Value value)
{
    int constant = cw_chunk_add_constant(cw->chunk, value);
    if (constant > UINT8_MAX)
    {
        cw_error(cw, "Too many constants in one chunk.");
        return;
    }

    cw_emit_bytes(cw, OP_CONSTANT, (uint8_t)constant);
}

/* ----------------------------------| PARSING |--------------------------------------------- */
static void cw_advance(cwRuntime* cw)
{
    cw->previous = cw->current;
    while (true)
    {
        cw->current = cw_scan_token(cw);
        if (cw->current.type != TOKEN_ERROR) break;

        cw_error_at_current(cw, cw->current.start);
    }
}

static void cw_consume(cwRuntime* cw, TokenType type, const char* message)
{
    if (cw->current.type == type)   cw_advance(cw);
    else                            cw_error_at_current(cw, message);
}

static bool cw_match(cwRuntime* cw, TokenType type)
{
    if (cw->current.type != type) return false;
    cw_advance(cw);
    return true;
}

static void cw_parser_synchronize(cwRuntime* cw)
{
    cw->panic = false;

    while (cw->current.type != TOKEN_EOF)
    {
        if (cw->previous.type == TOKEN_SEMICOLON) return;
        switch (cw->current.type)
        {
        case TOKEN_DATATYPE: 
        case TOKEN_FUNC:
        case TOKEN_LET:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        }

        cw_advance(cw);
    }
}

static void cw_parse_number(cwRuntime* cw);
static void cw_parse_string(cwRuntime* cw);
static void cw_parse_grouping(cwRuntime* cw);
static void cw_parse_unary(cwRuntime* cw);
static void cw_parse_binary(cwRuntime* cw);
static void cw_parse_literal(cwRuntime* cw);

ParseRule rules[] = {
    [TOKEN_LPAREN]      = { cw_parse_grouping,  NULL,               PREC_NONE },
    [TOKEN_RPAREN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_LBRACE]      = { NULL,               NULL,               PREC_NONE }, 
    [TOKEN_RBRACE]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_COMMA]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PERIOD]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_MINUS]       = { cw_parse_unary,     cw_parse_binary,    PREC_TERM },
    [TOKEN_PLUS]        = { NULL,               cw_parse_binary,    PREC_TERM },
    [TOKEN_SEMICOLON]   = { NULL,               NULL,               PREC_NONE },
    [TOKEN_SLASH]       = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_ASTERISK]    = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_EXCLAMATION] = { cw_parse_unary,     NULL,               PREC_NONE },
    [TOKEN_ASSIGN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_NOTEQ]       = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_EQ]          = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_LT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_LTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_IDENTIFIER]  = { NULL,               NULL,               PREC_NONE },
    [TOKEN_STRING]      = { cw_parse_string,    NULL,               PREC_NONE },
    [TOKEN_NUMBER]      = { cw_parse_number,    NULL,               PREC_NONE },
    [TOKEN_AND]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_DATATYPE]    = { NULL,               NULL,               PREC_NONE },
    [TOKEN_ELSE]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FALSE]       = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_FOR]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FUNC]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_IF]          = { NULL,               NULL,               PREC_NONE },
    [TOKEN_NULL]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_OR]          = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PRINT]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_RETURN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_TRUE]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_LET]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_WHILE]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_ERROR]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_EOF]         = { NULL,               NULL,               PREC_NONE },
};

static ParseRule* cw_get_parserule(TokenType type) { return &rules[type]; }

static void cw_parse_precedence(cwRuntime* cw, Precedence precedence)
{
    cw_advance(cw);
    ParseCallback prefix_rule = cw_get_parserule(cw->previous.type)->prefix;

    if (!prefix_rule)
    {
        cw_error(cw, "Expect expression");
        return;
    }

    prefix_rule(cw);

    while (precedence <= cw_get_parserule(cw->current.type)->precedence)
    {
        cw_advance(cw);
        ParseCallback infix_rule = cw_get_parserule(cw->previous.type)->infix;
        infix_rule(cw);
    }
}

static void cw_parse_expression(cwRuntime* cw)
{
    cw_parse_precedence(cw, PREC_ASSIGNMENT);
}

static void cw_expr_statement(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_SEMICOLON, "Expect ';' after expression.");
    cw_emit_byte(cw, OP_POP);
}

static void cw_print_statement(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_SEMICOLON, "Expect ';' after value.");
    cw_emit_byte(cw, OP_PRINT);
}

static void cw_parse_statement(cwRuntime* cw)
{
    if (cw_match(cw, TOKEN_PRINT))  cw_print_statement(cw);
    else                            cw_expr_statement(cw);
}

static void cw_var_decl(cwRuntime* cw)
{

}

static void cw_parse_declaration(cwRuntime* cw)
{
    if (cw_match(cw, TOKEN_LET))    cw_var_decl(cw);
    else                            cw_parse_statement(cw); 

    if (cw->panic) cw_parser_synchronize(cw);
}

static void cw_parse_number(cwRuntime* cw)
{
    double value = strtod(cw->previous.start, NULL);
    cw_emit_constant(cw, MAKE_NUMBER(value));
}

static void cw_parse_string(cwRuntime* cw)
{
    cwString* value = cw_str_copy(cw, cw->previous.start + 1, cw->previous.length - 2);
    cw_emit_constant(cw, MAKE_OBJECT(value));
}

static void cw_parse_grouping(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_RPAREN, "Expect ')' after expression.");
}

static void cw_parse_unary(cwRuntime* cw)
{
    TokenType operator = cw->previous.type;
    cw_parse_precedence(cw, PREC_UNARY);

    switch (operator)
    {
        case TOKEN_EXCLAMATION: cw_emit_byte(cw, OP_NOT); break;
        case TOKEN_MINUS:       cw_emit_byte(cw, OP_NEGATE); break;
    }
}

static void cw_parse_binary(cwRuntime* cw)
{
    TokenType operator = cw->previous.type;
    ParseRule* rule = cw_get_parserule(operator);
    cw_parse_precedence(cw, (Precedence)(rule->precedence + 1));

    switch (operator)
    {
    case TOKEN_EQ:        cw_emit_byte(cw, OP_EQ); break;
    case TOKEN_NOTEQ:     cw_emit_byte(cw, OP_NOTEQ); break;
    case TOKEN_LT:        cw_emit_byte(cw, OP_LT); break;
    case TOKEN_GT:        cw_emit_byte(cw, OP_GT); break;
    case TOKEN_LTEQ:      cw_emit_byte(cw, OP_LTEQ); break;
    case TOKEN_GTEQ:      cw_emit_byte(cw, OP_GTEQ); break;
    case TOKEN_PLUS:      cw_emit_byte(cw, OP_ADD); break;
    case TOKEN_MINUS:     cw_emit_byte(cw, OP_SUBTRACT); break;
    case TOKEN_ASTERISK:  cw_emit_byte(cw, OP_MULTIPLY); break;
    case TOKEN_SLASH:     cw_emit_byte(cw, OP_DIVIDE); break;
    }
}

static void cw_parse_literal(cwRuntime* cw)
{
    switch (cw->previous.type)
    {
    case TOKEN_FALSE: cw_emit_byte(cw, OP_FALSE); break;
    case TOKEN_NULL:  cw_emit_byte(cw, OP_NULL); break;
    case TOKEN_TRUE:  cw_emit_byte(cw, OP_TRUE); break;
    }
}

void cw_compiler_end(cwRuntime* cw)
{
    cw_emit_return(cw);
#ifdef DEBUG_PRINT_CODE
    if (!cw->error)
    {
        cw_disassemble_chunk(cw->chunk, "code");
    }
#endif 
}

bool cw_compile(cwRuntime* cw, const char* src, Chunk* chunk)
{
    cw_init_scanner(cw, src);

    cw->chunk = chunk;
    cw->error = false;
    cw->panic = false;

    cw_advance(cw);

    while (!cw_match(cw, TOKEN_EOF))
    {
        cw_parse_declaration(cw);
    }

    cw_compiler_end(cw);
    return !cw->error;
}