#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"
#include "scanner.h"

#include "debug.h"

static uint8_t cw_make_constant(cwRuntime* cw, Value value)
{
    int constant = cw_chunk_add_constant(cw->chunk, value);
    if (constant > UINT8_MAX)
    {
        cw_syntax_error(cw, "Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static uint8_t cw_identifier_constant(cwRuntime* cw, Token* name)
{
    return cw_make_constant(cw, MAKE_OBJECT(cw_str_copy(cw, name->start, name->end - name->start)));
}

/* ----------------------------------| PARSING |--------------------------------------------- */
static void cw_advance(cwRuntime* cw)
{
    cw->previous = cw->current;
    const char* cursor = cw->previous.end;
    int line = cw->previous.line + (cw->previous.type == TOKEN_TERMINATOR);
    while (true)
    {
        cursor = cw_scan_token(&cw->current, cursor, line);
        if (cw->current.type != TOKEN_ERROR) break;

        cw_syntax_error_at(cw, &cw->current, cw->current.start);
    }
}

static void cw_consume(cwRuntime* cw, TokenType type, const char* message)
{
    if (cw->current.type == type)   cw_advance(cw);
    else                            cw_syntax_error_at(cw, &cw->current, message);
}

static void cw_consume_terminator(cwRuntime* cw, const char* message)
{
    TokenType type = cw->current.type;
    if (type == TOKEN_EOF) return; /* dont consume EOF */
    if (type == TOKEN_SEMICOLON || type == TOKEN_TERMINATOR)  cw_advance(cw);
    else                                                      cw_syntax_error_at(cw, &cw->current, message);
}

static bool cw_match(cwRuntime* cw, TokenType type)
{
    if (cw->current.type != type) return false;
    cw_advance(cw);
    return true;
}

static bool cw_match_terminator(cwRuntime* cw)
{
    TokenType type = cw->current.type;
    if (type == TOKEN_EOF) return true; /* dont consume EOF */
    if (type != TOKEN_SEMICOLON && type != TOKEN_TERMINATOR) return false;

    cw_advance(cw);
    return true;
}

static void cw_parser_synchronize(cwRuntime* cw)
{
    cw->panic = false;

    while (cw->current.type != TOKEN_EOF)
    {
        if (cw->previous.type == TOKEN_SEMICOLON || cw->previous.type == TOKEN_TERMINATOR) return;
        switch (cw->current.type)
        {
        case TOKEN_IF:
        case TOKEN_FOR:
        case TOKEN_WHILE:
        case TOKEN_LET:
        case TOKEN_FUNC:
        case TOKEN_DATATYPE: 
        case TOKEN_RETURN:
        case TOKEN_PRINT:
            return;
        }

        cw_advance(cw);
    }
}

static void cw_parse_integer(cwRuntime* cw, bool can_assign);
static void cw_parse_float(cwRuntime* cw, bool can_assign);
static void cw_parse_string(cwRuntime* cw, bool can_assign);
static void cw_parse_grouping(cwRuntime* cw, bool can_assign);
static void cw_parse_unary(cwRuntime* cw, bool can_assign);
static void cw_parse_binary(cwRuntime* cw, bool can_assign);
static void cw_parse_literal(cwRuntime* cw, bool can_assign);
static void cw_parse_variable(cwRuntime* cw, bool can_assign);

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
    [TOKEN_TERMINATOR]  = { NULL,               NULL,               PREC_NONE },
    [TOKEN_SLASH]       = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_ASTERISK]    = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_EXCLAMATION] = { cw_parse_unary,     NULL,               PREC_NONE },
    [TOKEN_ASSIGN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_NOTEQ]       = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_EQ]          = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_LT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_LTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_IDENTIFIER]  = { cw_parse_variable,  NULL,               PREC_NONE },
    [TOKEN_STRING]      = { cw_parse_string,    NULL,               PREC_NONE },
    [TOKEN_INTEGER]     = { cw_parse_integer,   NULL,               PREC_NONE },
    [TOKEN_FLOAT]       = { cw_parse_float,     NULL,               PREC_NONE },
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
        cw_syntax_error(cw, "Expect expression");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(cw, can_assign);

    while (precedence <= cw_get_parserule(cw->current.type)->precedence)
    {
        cw_advance(cw);
        ParseCallback infix_rule = cw_get_parserule(cw->previous.type)->infix;
        infix_rule(cw, can_assign);
    }

    if (can_assign && cw_match(cw, TOKEN_ASSIGN))
    {
        cw_syntax_error(cw, "Invalid assignment target.");
    }
}

static void cw_parse_expression(cwRuntime* cw)
{
    cw_parse_precedence(cw, PREC_ASSIGNMENT);
}

static void cw_expr_statement(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume_terminator(cw, "Expect terminator after expression.");
    cw_emit_byte(cw, OP_POP);
}

static void cw_print_statement(cwRuntime* cw)
{
    cw_parse_expression(cw);
    cw_consume_terminator(cw, "Expect terminator after value.");
    cw_emit_byte(cw, OP_PRINT);
}

static void cw_parse_statement(cwRuntime* cw)
{
    if (cw_match(cw, TOKEN_PRINT))  cw_print_statement(cw);
    else                            cw_expr_statement(cw);
}

static void cw_var_decl(cwRuntime* cw)
{
    /* parse variable name */
    cw_consume(cw, TOKEN_IDENTIFIER, "Expect variable name.");
    uint8_t global = cw_identifier_constant(cw, &cw->previous);

    /* parse variable initialization value */
    if (cw_match(cw, TOKEN_ASSIGN)) cw_parse_expression(cw);
    else                            cw_syntax_error(cw, "Undefined variable.");

    /* define variable */
    cw_consume_terminator(cw, "Expect terminator after var declaration.");
    cw_emit_bytes(cw, OP_DEF_GLOBAL, global);
}

static void cw_parse_declaration(cwRuntime* cw)
{
    if (cw_match_terminator(cw)) return;

    if (cw_match(cw, TOKEN_LET))    cw_var_decl(cw);
    else                            cw_parse_statement(cw); 

    if (cw->panic) cw_parser_synchronize(cw);
}

static void cw_parse_integer(cwRuntime* cw, bool can_assign)
{
    int32_t value = strtol(cw->previous.start, NULL, 10);
    cw_emit_bytes(cw, OP_CONSTANT, cw_make_constant(cw, MAKE_INT(value)));
}

static void cw_parse_float(cwRuntime* cw, bool can_assign)
{
    float value = strtod(cw->previous.start, NULL);
    cw_emit_bytes(cw, OP_CONSTANT, cw_make_constant(cw, MAKE_FLOAT(value)));
}

static void cw_parse_string(cwRuntime* cw, bool can_assign)
{
    cwString* value = cw_str_copy(cw, cw->previous.start + 1, cw->previous.end - cw->previous.start - 2);
    cw_emit_bytes(cw, OP_CONSTANT, cw_make_constant(cw, MAKE_OBJECT(value)));
}

static void cw_parse_grouping(cwRuntime* cw, bool can_assign)
{
    cw_parse_expression(cw);
    cw_consume(cw, TOKEN_RPAREN, "Expect ')' after expression.");
}

static void cw_parse_unary(cwRuntime* cw, bool can_assign)
{
    TokenType operator = cw->previous.type;
    cw_parse_precedence(cw, PREC_UNARY);

    switch (operator)
    {
    case TOKEN_EXCLAMATION: cw_emit_byte(cw, OP_NOT); break;
    case TOKEN_MINUS:       cw_emit_byte(cw, OP_NEGATE); break;
    }
}

static void cw_parse_binary(cwRuntime* cw, bool can_assign)
{
    TokenType operator = cw->previous.type;
    ParseRule* rule = cw_get_parserule(operator);
    cw_parse_precedence(cw, (Precedence)(rule->precedence + 1));

    switch (operator)
    {
    case TOKEN_EQ:        cw_emit_byte(cw, OP_EQ); break;
    case TOKEN_NOTEQ:     cw_emit_byte(cw, OP_NOTEQ); break;
    case TOKEN_LT:        cw_emit_byte(cw, OP_LT); break;
    case TOKEN_LTEQ:      cw_emit_byte(cw, OP_LTEQ); break;
    case TOKEN_GT:        cw_emit_byte(cw, OP_GT); break;
    case TOKEN_GTEQ:      cw_emit_byte(cw, OP_GTEQ); break;
    case TOKEN_PLUS:      cw_emit_byte(cw, OP_ADD); break;
    case TOKEN_MINUS:     cw_emit_byte(cw, OP_SUBTRACT); break;
    case TOKEN_ASTERISK:  cw_emit_byte(cw, OP_MULTIPLY); break;
    case TOKEN_SLASH:     cw_emit_byte(cw, OP_DIVIDE); break;
    }
}

static void cw_parse_literal(cwRuntime* cw, bool can_assign)
{
    switch (cw->previous.type)
    {
    case TOKEN_FALSE: cw_emit_byte(cw, OP_FALSE); break;
    case TOKEN_NULL:  cw_emit_byte(cw, OP_NULL); break;
    case TOKEN_TRUE:  cw_emit_byte(cw, OP_TRUE); break;
    }
}

static void cw_parse_variable(cwRuntime* cw, bool can_assign)
{
    uint8_t arg = cw_identifier_constant(cw, &cw->previous);

    if (can_assign && cw_match(cw, TOKEN_ASSIGN))
    {
        cw_parse_expression(cw);
        cw_emit_bytes(cw, OP_SET_GLOBAL, arg);
    }
    else 
    {
        cw_emit_bytes(cw, OP_GET_GLOBAL, arg);
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