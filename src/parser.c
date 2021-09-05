#include "parser.h"

#include "statement.h"

#include "debug.h"
#include "runtime.h"

/* --------------------------| parse rules |--------------------------------------------- */
typedef void (*ParseCallback)(cwRuntime* cw, bool can_assign);

typedef struct
{
    ParseCallback prefix;
    ParseCallback infix;
    Precedence precedence;
} ParseRule;

static void cw_parse_integer(cwRuntime* cw, bool can_assign);
static void cw_parse_float(cwRuntime* cw, bool can_assign);
static void cw_parse_string(cwRuntime* cw, bool can_assign);
static void cw_parse_grouping(cwRuntime* cw, bool can_assign);
static void cw_parse_unary(cwRuntime* cw, bool can_assign);
static void cw_parse_binary(cwRuntime* cw, bool can_assign);
static void cw_parse_and(cwRuntime* cw, bool can_assign);
static void cw_parse_or(cwRuntime* cw, bool can_assign);
static void cw_parse_literal(cwRuntime* cw, bool can_assign);
static void cw_parse_variable(cwRuntime* cw, bool can_assign);

ParseRule rules[] = {
    [TOKEN_EOF]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_LPAREN]      = { cw_parse_grouping,  NULL,               PREC_NONE },
    [TOKEN_RPAREN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_LBRACE]      = { NULL,               NULL,               PREC_NONE }, 
    [TOKEN_RBRACE]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PERIOD]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_COMMA]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_COLON]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_SEMICOLON]   = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PLUS]        = { NULL,               cw_parse_binary,    PREC_TERM },
    [TOKEN_MINUS]       = { cw_parse_unary,     cw_parse_binary,    PREC_TERM },
    [TOKEN_ASTERISK]    = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_SLASH]       = { NULL,               cw_parse_binary,    PREC_FACTOR },
    [TOKEN_EXCLAMATION] = { cw_parse_unary,     NULL,               PREC_NONE },
    [TOKEN_ASSIGN]      = { NULL,               NULL,               PREC_NONE },
    // Comparison tokens.
    [TOKEN_EQ]          = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_NOTEQ]       = { NULL,               cw_parse_binary,    PREC_EQUALITY },
    [TOKEN_LT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_LTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GT]          = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    [TOKEN_GTEQ]        = { NULL,               cw_parse_binary,    PREC_COMPARISON },
    // Literals.
    [TOKEN_IDENTIFIER]  = { cw_parse_variable,  NULL,               PREC_NONE },
    [TOKEN_STRING]      = { cw_parse_string,    NULL,               PREC_NONE },
    [TOKEN_INTEGER]     = { cw_parse_integer,   NULL,               PREC_NONE },
    [TOKEN_FLOAT]       = { cw_parse_float,     NULL,               PREC_NONE },
    // Keywords.
    [TOKEN_NULL]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_TRUE]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_FALSE]       = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_AND]         = { NULL,               cw_parse_and,       PREC_AND },
    [TOKEN_OR]          = { NULL,               cw_parse_or,        PREC_OR },
    [TOKEN_IF]          = { NULL,               NULL,               PREC_NONE },
    [TOKEN_ELSE]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_WHILE]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FOR]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_LET]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FUNC]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_DATATYPE]    = { NULL,               NULL,               PREC_NONE },
    [TOKEN_RETURN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PRINT]       = { NULL,               NULL,               PREC_NONE },
};

void cw_parse_precedence(cwRuntime* cw, Precedence precedence)
{
    cw_advance(cw);
    ParseCallback prefix_rule = rules[cw->previous.type].prefix;

    if (!prefix_rule)
    {
        cw_syntax_error_at(cw, &cw->previous, "Expect expression");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(cw, can_assign);

    while (precedence <= rules[cw->current.type].precedence)
    {
        cw_advance(cw);
        ParseCallback infix_rule = rules[cw->previous.type].infix;
        infix_rule(cw, can_assign);
    }

    if (can_assign && cw_match(cw, TOKEN_ASSIGN))
    {
        cw_syntax_error_at(cw, &cw->previous, "Invalid assignment target.");
    }
}

/* --------------------------| parse callbacks |----------------------------------------- */
static void cw_parse_integer(cwRuntime* cw, bool can_assign)
{
    int32_t value = strtol(cw->previous.start, NULL, cw_token_get_base(&cw->previous));
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
    cwTokenType operator = cw->previous.type;
    cw_parse_precedence(cw, PREC_UNARY);

    switch (operator)
    {
    case TOKEN_EXCLAMATION: cw_emit_byte(cw, OP_NOT); break;
    case TOKEN_MINUS:       cw_emit_byte(cw, OP_NEGATE); break;
    }
}

static void cw_parse_binary(cwRuntime* cw, bool can_assign)
{
    cwTokenType operator = cw->previous.type;
    cw_parse_precedence(cw, (Precedence)(rules[operator].precedence + 1));

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

static void cw_parse_and(cwRuntime* cw, bool can_assign)
{
    int end_jump = cw_emit_jump(cw, OP_JUMP_IF_FALSE);

    cw_emit_byte(cw, OP_POP);
    cw_parse_precedence(cw, PREC_AND);

    cw_patch_jump(cw, end_jump);
}

static void cw_parse_or(cwRuntime* cw, bool can_assign)
{
    int else_jump = cw_emit_jump(cw, OP_JUMP_IF_FALSE);
    int end_jump  = cw_emit_jump(cw, OP_JUMP);

    cw_patch_jump(cw, else_jump);
    cw_emit_byte(cw, OP_POP);

    cw_parse_precedence(cw, PREC_OR);
    cw_patch_jump(cw, end_jump);
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
    uint8_t get_op, set_op;
    int arg = cw_resolve_local(cw, &cw->previous);
    if (arg >= 0)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else
    {
        arg = cw_identifier_constant(cw, &cw->previous);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && cw_match(cw, TOKEN_ASSIGN))
    {
        cw_parse_expression(cw);
        cw_emit_bytes(cw, set_op, (uint8_t)arg);
    }
    else 
    {
        cw_emit_bytes(cw, get_op, (uint8_t)arg);
    }
}

/* --------------------------| utility |------------------------------------------------- */
void cw_advance(cwRuntime* cw)
{
    cw->previous = cw->current;
    const char* cursor = cw->previous.end;
    int line = cw->previous.line;
    while (true)
    {
        cursor = cw_scan_token(cw, &cw->current, cursor, line);
        if (!cw->error) break;
    }
}

void cw_consume(cwRuntime* cw, cwTokenType type, const char* message)
{
    if (cw->current.type == type)   cw_advance(cw);
    else                            cw_syntax_error_at(cw, &cw->current, message);
}

bool cw_match(cwRuntime* cw, cwTokenType type)
{
    if (cw->current.type != type) return false;
    cw_advance(cw);
    return true;
}

void cw_parser_synchronize(cwRuntime* cw)
{
    cw->panic = false;

    while (cw->current.type != TOKEN_EOF)
    {
        if (cw->previous.type == TOKEN_SEMICOLON) return;
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