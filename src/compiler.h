#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "scanner.h"

typedef enum
{
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    /* local variables */
    OP_SET_LOCAL,
    OP_GET_LOCAL,
    /* global variables */
    OP_DEF_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_GLOBAL,
    /* comparison operations */
    OP_EQ, OP_NOTEQ,
    OP_LT, OP_LTEQ,
    OP_GT, OP_GTEQ,
    /* arithmetic operations */
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_NOT,
    /* control flow operations */
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    OP_PRINT,
    OP_RETURN,
} cwOpCode;

typedef struct
{
    cwToken name;
    int depth;
} cwLocal;

bool cw_compile(cwRuntime* cw, const char* src, cwChunk* chunk);

/* constants identitfiers */
uint8_t cw_make_constant(cwRuntime* cw, cwValue value);
uint8_t cw_identifier_constant(cwRuntime* cw, cwToken* name);
bool cw_identifiers_equal(const cwToken* a, const cwToken* b);

/* locals */
void cw_add_local(cwRuntime* cw, cwToken* name);
int  cw_resolve_local(cwRuntime* cw, cwToken* name);

/* writing byte code */
void cw_emit_byte(cwChunk* chunk, uint8_t byte, int line);
void cw_emit_bytes(cwChunk* chunk, uint8_t a, uint8_t b, int line);

int  cw_emit_jump(cwChunk* chunk, uint8_t instruction, int line);
void cw_emit_loop(cwRuntime* cw, int start);
void cw_patch_jump(cwRuntime* cw, int offset);

#endif /* !CLOCKWORK_COMPILER_H */