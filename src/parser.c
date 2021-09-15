#include "parser.h"

#include "debug.h"
#include "runtime.h"

#include <stdio.h>

cwValueType cw_parse_expression(cwRuntime* cw)
{
    return cw_parse_precedence(cw, PREC_ASSIGNMENT);
}

/* --------------------------| parse rules |--------------------------------------------- */
typedef cwValueType (*cwPrefixRule)(cwRuntime* cw, bool can_assign);
typedef cwValueType (*cwInfixRule)(cwRuntime* cw, cwValueType left, bool can_assign);

typedef struct
{
    cwPrefixRule prefix;
    cwInfixRule infix;
    Precedence precedence;
} ParseRule;

static cwValueType cw_parse_integer (cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_float   (cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_string  (cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_grouping(cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_unary   (cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_literal (cwRuntime* cw, bool can_assign);
static cwValueType cw_parse_variable(cwRuntime* cw, bool can_assign);

static cwValueType cw_parse_binary(cwRuntime* cw, cwValueType left, bool can_assign);
static cwValueType cw_parse_and   (cwRuntime* cw, cwValueType left, bool can_assign);
static cwValueType cw_parse_or    (cwRuntime* cw, cwValueType left, bool can_assign);

ParseRule rules[] = {
    [TOKEN_EOF]         = { NULL,               NULL,             PREC_NONE },
    [TOKEN_LPAREN]      = { cw_parse_grouping,  NULL,             PREC_NONE },
    [TOKEN_RPAREN]      = { NULL,               NULL,             PREC_NONE },
    [TOKEN_LBRACE]      = { NULL,               NULL,             PREC_NONE }, 
    [TOKEN_RBRACE]      = { NULL,               NULL,             PREC_NONE },
    [TOKEN_PERIOD]      = { NULL,               NULL,             PREC_NONE },
    [TOKEN_COMMA]       = { NULL,               NULL,             PREC_NONE },
    [TOKEN_COLON]       = { NULL,               NULL,             PREC_NONE },
    [TOKEN_SEMICOLON]   = { NULL,               NULL,             PREC_NONE },
    [TOKEN_PLUS]        = { NULL,               cw_parse_binary,  PREC_TERM },
    [TOKEN_MINUS]       = { cw_parse_unary,     cw_parse_binary,  PREC_TERM },
    [TOKEN_ASTERISK]    = { NULL,               cw_parse_binary,  PREC_FACTOR },
    [TOKEN_SLASH]       = { NULL,               cw_parse_binary,  PREC_FACTOR },
    [TOKEN_EXCLAMATION] = { cw_parse_unary,     NULL,             PREC_NONE },
    [TOKEN_ASSIGN]      = { NULL,               NULL,             PREC_NONE },
    [TOKEN_AND]         = { NULL,               cw_parse_and,     PREC_AND },
    [TOKEN_OR]          = { NULL,               cw_parse_or,      PREC_OR },
    // Comparison tokens.
    [TOKEN_EQ]          = { NULL,               cw_parse_binary,  PREC_EQUALITY },
    [TOKEN_NOTEQ]       = { NULL,               cw_parse_binary,  PREC_EQUALITY },
    [TOKEN_LT]          = { NULL,               cw_parse_binary,  PREC_COMPARISON },
    [TOKEN_LTEQ]        = { NULL,               cw_parse_binary,  PREC_COMPARISON },
    [TOKEN_GT]          = { NULL,               cw_parse_binary,  PREC_COMPARISON },
    [TOKEN_GTEQ]        = { NULL,               cw_parse_binary,  PREC_COMPARISON },
    // Literals.
    [TOKEN_IDENTIFIER]  = { cw_parse_variable,  NULL,             PREC_NONE },
    [TOKEN_STRING]      = { cw_parse_string,    NULL,             PREC_NONE },
    [TOKEN_INTEGER]     = { cw_parse_integer,   NULL,             PREC_NONE },
    [TOKEN_FLOAT]       = { cw_parse_float,     NULL,             PREC_NONE },
    // Keywords.
    [TOKEN_NULL]        = { NULL,               NULL,             PREC_NONE },
    [TOKEN_TRUE]        = { NULL,               NULL,             PREC_NONE },
    [TOKEN_FALSE]       = { NULL,               NULL,             PREC_NONE },
    [TOKEN_IF]          = { NULL,               NULL,             PREC_NONE },
    [TOKEN_ELSE]        = { NULL,               NULL,             PREC_NONE },
    [TOKEN_WHILE]       = { NULL,               NULL,             PREC_NONE },
    [TOKEN_FOR]         = { NULL,               NULL,             PREC_NONE },
    [TOKEN_LET]         = { NULL,               NULL,             PREC_NONE },
    [TOKEN_FUNC]        = { NULL,               NULL,             PREC_NONE },
    [TOKEN_DATATYPE]    = { NULL,               NULL,             PREC_NONE },
    [TOKEN_RETURN]      = { NULL,               NULL,             PREC_NONE },
};

cwValueType cw_parse_precedence(cwRuntime* cw, Precedence precedence)
{
    cw_next_token(&cw->current, &cw->previous);
    cwPrefixRule prefix_rule = rules[cw->previous.type].prefix;

    if (!prefix_rule)
    {
        cw_syntax_error_at(&cw->previous, "Expect expression");
        return CW_VALUE_NULL;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    cwValueType left = prefix_rule(cw, can_assign);

    cwValueType right = CW_VALUE_NULL;
    while (precedence <= rules[cw->current.type].precedence)
    {
        cw_next_token(&cw->current, &cw->previous);
        cwInfixRule infix_rule = rules[cw->previous.type].infix;
        right = infix_rule(cw, left, can_assign);
    }

    if (can_assign && cw_match(cw, TOKEN_ASSIGN))
        cw_syntax_error_at(&cw->previous, "Invalid assignment target.");

    return cw_valuetype_max(left, right);
}

/* --------------------------| parse callbacks |----------------------------------------- */
static cwValueType cw_parse_integer(cwRuntime* cw, bool can_assign)
{
    int32_t value = strtol(cw->previous.start, NULL, cw_token_get_base(&cw->previous));
    cw_emit_bytes(cw->chunk, OP_PUSH, cw_make_constant(cw, CW_MAKE_INT(value)), cw->previous.line);
    return CW_VALUE_INT;
}

static cwValueType cw_parse_float(cwRuntime* cw, bool can_assign)
{
    float value = strtod(cw->previous.start, NULL);
    cw_emit_bytes(cw->chunk, OP_PUSH, cw_make_constant(cw, CW_MAKE_FLOAT(value)), cw->previous.line);
    return CW_VALUE_FLOAT;
}

static cwValueType cw_parse_string(cwRuntime* cw, bool can_assign)
{
    return CW_VALUE_NULL;
}

static cwValueType cw_parse_variable(cwRuntime* cw, bool can_assign)
{
    return CW_VALUE_NULL;
}

static cwValueType cw_parse_grouping(cwRuntime* cw, bool can_assign)
{
    cwValueType return_type = cw_parse_expression(cw);
    cw_consume(cw, TOKEN_RPAREN, "Expect ')' after expression.");
    return return_type;
}

static cwValueType cw_parse_unary(cwRuntime* cw, bool can_assign)
{
    cwTokenType operator = cw->previous.type;
    cwValueType result_type = cw_parse_precedence(cw, PREC_UNARY);

    switch (operator)
    {
    case TOKEN_EXCLAMATION: cw_emit_byte(cw->chunk, OP_NOT, cw->previous.line); break;
    case TOKEN_MINUS:       
        cw_emit_byte(cw->chunk, result_type == CW_VALUE_FLOAT ? OP_NEG_F : OP_NEG_I, cw->previous.line); 
        break;
    }
    return result_type;
}

static cwValueType cw_parse_binary(cwRuntime* cw, cwValueType left, bool can_assign)
{
    cwTokenType operator = cw->previous.type;
    cwValueType right = cw_parse_precedence(cw, (Precedence)(rules[operator].precedence + 1));

    cwOpCode op_code = OP_RETURN;
    switch (operator)
    {
    /* equality operators */
    case TOKEN_EQ:    cw_emit_byte(cw->chunk, OP_EQ,    cw->previous.line); return CW_VALUE_BOOL;
    case TOKEN_NOTEQ: cw_emit_byte(cw->chunk, OP_NOTEQ, cw->previous.line); return CW_VALUE_BOOL;
    /* comparsion operators */
    case TOKEN_LT:        op_code = OP_LT_I; break;
    case TOKEN_LTEQ:      op_code = OP_LTEQ_I; break;
    case TOKEN_GT:        op_code = OP_GT_I; break;
    case TOKEN_GTEQ:      op_code = OP_GTEQ_I; break;
    /* arithmetic operators */
    case TOKEN_PLUS:      op_code = OP_ADD_I; break;
    case TOKEN_MINUS:     op_code = OP_SUB_I; break;
    case TOKEN_ASTERISK:  op_code = OP_MUL_I; break;
    case TOKEN_SLASH:     op_code = OP_DIV_I; break;
    }

    if (!(cw_valuetype_numeric(left) && cw_valuetype_numeric(right)))
        cw_syntax_error_at(&cw->previous, "Operands musst be numeric values.");

    if (left == CW_VALUE_FLOAT || right == CW_VALUE_FLOAT) op_code |= OP_MOD_FLOAT; 

    cw_emit_byte(cw->chunk, op_code, cw->previous.line);
    return (op_code < OP_LT_I) ? cw_valuetype_max(left, right) : CW_VALUE_BOOL;
}

static cwValueType cw_parse_and(cwRuntime* cw, cwValueType left, bool can_assign)
{
    int end_jump = cw_emit_jump(cw->chunk, OP_JUMP_IF_FALSE, cw->previous.line);

    cw_emit_byte(cw->chunk, OP_POP, cw->previous.line);
    cw_parse_precedence(cw, PREC_AND);

    cw_patch_jump(cw, end_jump);
    return CW_VALUE_BOOL;
}

static cwValueType cw_parse_or(cwRuntime* cw, cwValueType left, bool can_assign)
{
    int or_jump = cw_emit_jump(cw->chunk, OP_JUMP_IF_FALSE, cw->previous.line);
    int end_jump  = cw_emit_jump(cw->chunk, OP_JUMP, cw->previous.line);

    cw_patch_jump(cw, or_jump);
    cw_emit_byte(cw->chunk, OP_POP, cw->previous.line);

    cw_parse_precedence(cw, PREC_OR);
    cw_patch_jump(cw, end_jump);
    return CW_VALUE_BOOL;
}

/* --------------------------| utility |------------------------------------------------- */
void cw_consume(cwRuntime* cw, cwTokenType type, const char* message)
{
    if (cw->current.type == type)   cw_next_token(&cw->current, &cw->previous);
    else                            cw_syntax_error_at(&cw->current, message);
}

bool cw_match(cwRuntime* cw, cwTokenType type)
{
    if (cw->current.type != type) return false;
    cw_next_token(&cw->current, &cw->previous);
    return true;
}