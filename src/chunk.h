#ifndef CLOCKWORK_CHUNK_H
#define CLOCKWORK_CHUNK_H

#include "common.h"

typedef enum
{
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

typedef struct
{
    uint8_t data;
    int line;
} ByteCode;

typedef struct 
{
    ByteCode* bytes;
    Value* constants;
} Chunk;

void cw_chunk_init(Chunk* chunk);
void cw_chunk_free(Chunk* chunk);
void cw_chunk_write(Chunk* chunk, uint8_t byte, int line);
int  cw_chunk_add_constant(Chunk* chunk, Value val);

#endif /* !CLOCKWORK_CHUNK_H */
