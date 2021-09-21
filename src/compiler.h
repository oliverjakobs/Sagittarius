#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "scanner.h"

#define OP_MOD_FLOAT 1

typedef enum
{
    OP_PUSH,
    OP_PUSH_NULL,
    OP_PUSH_TRUE,
    OP_PUSH_FALSE,
    OP_POP,
    /* arithmetic operations */
    OP_ADD, 
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_NOT,
    /* equality operations */
    OP_EQ, OP_NOTEQ,
    /* comparison operations */
    OP_LT,
    OP_LTEQ,
    OP_GT,
    OP_GTEQ,
    /* control flow operations */
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    OP_RETURN
} cwOpCode;

uint8_t cw_make_constant(cwRuntime* cw, cwValue value);

bool cw_compile(cwRuntime* cw, const char* src, cwChunk* chunk);

/* writing byte code */
void cw_emit_byte(cwChunk* chunk, uint8_t byte, int line);
void cw_emit_dword(cwChunk* chunk, uint32_t value, int line);

int  cw_emit_jump(cwChunk* chunk, uint8_t instruction, int line);
void cw_emit_loop(cwRuntime* cw, int start);
void cw_patch_jump(cwRuntime* cw, int offset);

#endif /* !CLOCKWORK_COMPILER_H */