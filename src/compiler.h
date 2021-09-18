#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "scanner.h"

#define OP_MOD_FLOAT 1

typedef enum
{
    OP_PUSH_I = 0 << 1, OP_PUSH_F = OP_PUSH_I | OP_MOD_FLOAT,
    OP_PUSH_NULL,       OP_PUSH_TRUE,       OP_PUSH_FALSE,
    OP_POP,
    /* arithmetic operations */
    OP_ADD_I = 3 << 1,  OP_ADD_F = OP_ADD_I | OP_MOD_FLOAT, 
    OP_SUB_I = 4 << 1,  OP_SUB_F = OP_SUB_I | OP_MOD_FLOAT,
    OP_MUL_I = 5 << 1,  OP_MUL_F = OP_MUL_I | OP_MOD_FLOAT,
    OP_DIV_I = 6 << 1,  OP_DIV_F = OP_DIV_I | OP_MOD_FLOAT,
    OP_NEG_I = 7 << 1,  OP_NEG_F = OP_NEG_I | OP_MOD_FLOAT,
    OP_NOT,
    /* equality operations */
    OP_EQ, OP_NOTEQ,
    /* comparison operations */
    OP_LT_I   = 10 << 1, OP_LT_F   = OP_LT_I   | OP_MOD_FLOAT,
    OP_LTEQ_I = 11 << 1, OP_LTEQ_F = OP_LTEQ_I | OP_MOD_FLOAT,
    OP_GT_I   = 12 << 1, OP_GT_F   = OP_GT_I   | OP_MOD_FLOAT,
    OP_GTEQ_I = 13 << 1, OP_GTEQ_F = OP_GTEQ_I | OP_MOD_FLOAT,
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
void cw_emit_bytes(cwChunk* chunk, uint8_t a, uint8_t b, int line);
void cw_emit_uint32(cwChunk* chunk, uint32_t value, int line);

int  cw_emit_jump(cwChunk* chunk, uint8_t instruction, int line);
void cw_emit_loop(cwRuntime* cw, int start);
void cw_patch_jump(cwRuntime* cw, int offset);

#endif /* !CLOCKWORK_COMPILER_H */