#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,  // '='
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // '==', '!='
    PREC_COMPARISON,  // '<', '>', '<=', '>='
    PREC_TERM,        // '+', '-'
    PREC_FACTOR,      // '*', '/'
    PREC_UNARY,       // '!', '-'
    PREC_CALL,        // '.', '(...)'
    PREC_PRIMARY
} Precedence;

typedef void (*ParseCallback)(VM* vm, Parser* parser);

typedef struct
{
  ParseCallback prefix;
  ParseCallback infix;
  Precedence precedence;
} ParseRule;

/* ----------------------------------| ERROR |----------------------------------------------- */
static void cw_error_at(Parser* parser, Token* token, const char* msg)
{
    if (parser->panic) return;
    parser->panic = true;

    fprintf(stderr, "[line %d] Parser Error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else if (token->type != TOKEN_ERROR)
        fprintf(stderr, " at '%.*s'", token->length, token->start);

    fprintf(stderr, ": %s\n", msg);
    parser->error = true;
}

static void cw_error_at_current(Parser* parser, const char* msg)
{
    cw_error_at(parser, &parser->current, msg);
}

static void cw_error(Parser* parser, const char* msg)
{
    cw_error_at(parser, &parser->previous, msg);
}

/* ----------------------------------| EMIT BYTES |------------------------------------------ */
static void cw_emit_byte(Parser* parser, uint8_t byte)
{
    cw_chunk_write(parser->chunk, byte, parser->previous.line);
}

static void cw_emit_bytes(Parser* parser, uint8_t a, uint8_t b)
{
    cw_emit_byte(parser, a);
    cw_emit_byte(parser, b);
}

static void cw_emit_return(Parser* parser)
{
    cw_emit_byte(parser, OP_RETURN);
}

static void cw_emit_constant(Parser* parser, Value value)
{
    int constant = cw_chunk_add_constant(parser->chunk, value);
    if (constant > UINT8_MAX)
    {
        cw_error(parser, "Too many constants in one chunk.");
        return;
    }

    cw_emit_bytes(parser, OP_CONSTANT, (uint8_t)constant);
}

/* ----------------------------------| PARSING |--------------------------------------------- */
static void cw_advance(Parser* parser)
{
    parser->previous = parser->current;
    while (true)
    {
        parser->current = cw_scan_token(&parser->scanner);
        if (parser->current.type != TOKEN_ERROR) break;

        cw_error_at_current(parser, parser->current.start);
    }
}

static void cw_consume(Parser* parser, TokenType type, const char* message)
{
    if (parser->current.type == type) cw_advance(parser);
    else                              cw_error_at_current(parser, message);
}

static void cw_parse_number(VM* vm, Parser* parser);
static void cw_parse_string(VM* vm, Parser* parser);
static void cw_parse_grouping(VM* vm, Parser* parser);
static void cw_parse_unary(VM* vm, Parser* parser);
static void cw_parse_binary(VM* vm, Parser* parser);
static void cw_parse_literal(VM* vm, Parser* parser);

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
    [TOKEN_CLASS]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_ELSE]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FALSE]       = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_FOR]         = { NULL,               NULL,               PREC_NONE },
    [TOKEN_FUNC]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_IF]          = { NULL,               NULL,               PREC_NONE },
    [TOKEN_NULL]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_OR]          = { NULL,               NULL,               PREC_NONE },
    [TOKEN_PRINT]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_RETURN]      = { NULL,               NULL,               PREC_NONE },
    [TOKEN_SUPER]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_THIS]        = { NULL,               NULL,               PREC_NONE },
    [TOKEN_TRUE]        = { cw_parse_literal,   NULL,               PREC_NONE },
    [TOKEN_DECLARE]     = { NULL,               NULL,               PREC_NONE },
    [TOKEN_WHILE]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_ERROR]       = { NULL,               NULL,               PREC_NONE },
    [TOKEN_EOF]         = { NULL,               NULL,               PREC_NONE },
};

static ParseRule* cw_get_parserule(TokenType type) { return &rules[type]; }

static void cw_parse_precedence(VM* vm, Parser* parser, Precedence precedence)
{
    cw_advance(parser);
    ParseCallback prefix_rule = cw_get_parserule(parser->previous.type)->prefix;

    if (!prefix_rule)
    {
        cw_error(parser, "Expect expression");
        return;
    }

    prefix_rule(vm, parser);

    while (precedence <= cw_get_parserule(parser->current.type)->precedence)
    {
        cw_advance(parser);
        ParseCallback infix_rule = cw_get_parserule(parser->previous.type)->infix;
        infix_rule(vm, parser);
    }
}

static void cw_parse_expression(VM* vm, Parser* parser)
{
    cw_parse_precedence(vm, parser, PREC_ASSIGNMENT);
}

static void cw_parse_number(VM* vm, Parser* parser)
{
    double value = strtod(parser->previous.start, NULL);
    cw_emit_constant(parser, MAKE_NUMBER(value));
}

static void cw_parse_string(VM* vm, Parser* parser)
{
    cwString* value = cw_str_copy(vm, parser->previous.start + 1, parser->previous.length - 2);
    cw_emit_constant(parser, MAKE_OBJECT(value));
}

static void cw_parse_grouping(VM* vm, Parser* parser)
{
    cw_parse_expression(vm, parser);
    cw_consume(parser, TOKEN_RPAREN, "Expect ')' after expression.");
}

static void cw_parse_unary(VM* vm, Parser* parser)
{
    TokenType operator = parser->previous.type;
    cw_parse_precedence(vm, parser, PREC_UNARY);

    switch (operator)
    {
        case TOKEN_EXCLAMATION: cw_emit_byte(parser, OP_NOT); break;
        case TOKEN_MINUS:       cw_emit_byte(parser, OP_NEGATE); break;
    }
}

static void cw_parse_binary(VM* vm, Parser* parser)
{
    TokenType operator = parser->previous.type;
    ParseRule* rule = cw_get_parserule(operator);
    cw_parse_precedence(vm, parser, (Precedence)(rule->precedence + 1));

    switch (operator)
    {
    case TOKEN_EQ:        cw_emit_byte(parser, OP_EQ); break;
    case TOKEN_NOTEQ:     cw_emit_byte(parser, OP_NOTEQ); break;
    case TOKEN_LT:        cw_emit_byte(parser, OP_LT); break;
    case TOKEN_GT:        cw_emit_byte(parser, OP_GT); break;
    case TOKEN_LTEQ:      cw_emit_byte(parser, OP_LTEQ); break;
    case TOKEN_GTEQ:      cw_emit_byte(parser, OP_GTEQ); break;
    case TOKEN_PLUS:      cw_emit_byte(parser, OP_ADD); break;
    case TOKEN_MINUS:     cw_emit_byte(parser, OP_SUBTRACT); break;
    case TOKEN_ASTERISK:  cw_emit_byte(parser, OP_MULTIPLY); break;
    case TOKEN_SLASH:     cw_emit_byte(parser, OP_DIVIDE); break;
    }
}

static void cw_parse_literal(VM* vm, Parser* parser)
{
    switch (parser->previous.type)
    {
    case TOKEN_FALSE: cw_emit_byte(parser, OP_FALSE); break;
    case TOKEN_NULL:  cw_emit_byte(parser, OP_NULL); break;
    case TOKEN_TRUE:  cw_emit_byte(parser, OP_TRUE); break;
    }
}

void cw_compiler_end(Parser* parser)
{
    cw_emit_return(parser);
#ifdef DEBUG_PRINT_CODE
    if (!parser->error)
    {
        cw_disassemble_chunk(parser->chunk, "code");
    }
#endif 
}

bool cw_compile(VM* vm, const char* src, Chunk* chunk)
{
    Parser parser;
    cw_scanner_init(&parser.scanner, src);

    parser.chunk = chunk;
    parser.error = false;
    parser.panic = false;

    cw_advance(&parser);
    cw_parse_expression(vm, &parser);
    cw_consume(&parser, TOKEN_EOF, "Expect end of expression.");

    cw_compiler_end(&parser);
    return !parser.error;
}