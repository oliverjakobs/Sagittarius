#ifndef CLOCKWORK_CHUNK_H
#define CLOCKWORK_CHUNK_H

#include "common.h"

typedef enum
{
    OP_CONSTANT,
    OP_NULL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    /* global variables */
    OP_DEF_GLOBAL,
    OP_GET_GLOBAL,
    /* comparison operations */
    OP_EQ,
    OP_NOTEQ,
    OP_LT,
    OP_GT,
    OP_LTEQ,
    OP_GTEQ,
    /* arithmetic operations */
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
} OpCode;

struct Chunk
{
    /* byte code with line information */
    uint8_t* bytes;
    int*     lines;
    size_t len;
    size_t cap;

    /* constants */
    Value* constants;
    size_t const_len;
    size_t const_cap;
};

void cw_chunk_init(Chunk* chunk);
void cw_chunk_free(Chunk* chunk);
void cw_chunk_write(Chunk* chunk, uint8_t byte, int line);
int  cw_chunk_add_constant(Chunk* chunk, Value val);

void cw_emit_byte(cwRuntime* cw, uint8_t byte);
void cw_emit_bytes(cwRuntime* cw, uint8_t a, uint8_t b);
void cw_emit_return(cwRuntime* cw);

#endif /* !CLOCKWORK_CHUNK_H */
