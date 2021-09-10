#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "scanner.h"

typedef enum
{
    OP_PUSH,
    OP_POP,
    /* arithmetic operations */
    OP_ADD, OP_SUB,
    OP_MUL, OP_DIV,
    OP_NEG,
    OP_NOT,
    /* comparison operations */
    OP_EQ, OP_NOTEQ,
    OP_LT, OP_LTEQ,
    OP_GT, OP_GTEQ,
    /* control flow operations */
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    OP_RETURN,
} cwOpCode;

uint8_t cw_make_constant(cwRuntime* cw, cwValue value);

bool cw_compile(cwRuntime* cw, const char* src, cwChunk* chunk);

/* writing byte code */
void cw_emit_byte(cwChunk* chunk, uint8_t byte, int line);
void cw_emit_bytes(cwChunk* chunk, uint8_t a, uint8_t b, int line);

int  cw_emit_jump(cwChunk* chunk, uint8_t instruction, int line);
void cw_emit_loop(cwRuntime* cw, int start);
void cw_patch_jump(cwRuntime* cw, int offset);

#endif /* !CLOCKWORK_COMPILER_H */