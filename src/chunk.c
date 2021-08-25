#include "chunk.h"

#include "memory.h"
#include "runtime.h"

void cw_chunk_init(Chunk* chunk)
{
    chunk->bytes = NULL;
    chunk->lines = NULL;
    chunk->len = 0;
    chunk->cap = 0;
    chunk->constants = NULL;
    chunk->const_len = 0;
    chunk->const_cap = 0;
}

void cw_chunk_free(Chunk* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->bytes, chunk->cap);
    CW_FREE_ARRAY(int, chunk->lines, chunk->cap);
    CW_FREE_ARRAY(Value, chunk->constants, chunk->const_cap);
    cw_chunk_init(chunk);
}

void cw_chunk_write(Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->cap < chunk->len + 1)
    {
        int old_cap = chunk->cap;
        chunk->cap = CW_GROW_CAPACITY(old_cap);
        chunk->bytes = CW_GROW_ARRAY(uint8_t, chunk->bytes, old_cap, chunk->cap);
        chunk->lines = CW_GROW_ARRAY(int, chunk->lines, old_cap, chunk->cap);
    }

    chunk->bytes[chunk->len] = byte;
    chunk->lines[chunk->len] = line;
    chunk->len++;
}

int cw_chunk_add_constant(Chunk* chunk, Value val)
{
    if (chunk->const_cap < chunk->const_len + 1)
    {
        int old_cap = chunk->const_cap;
        chunk->const_cap = CW_GROW_CAPACITY(old_cap);
        chunk->constants = CW_GROW_ARRAY(Value, chunk->constants, old_cap, chunk->const_cap);
    }

    chunk->constants[chunk->const_len] = val;
    return chunk->const_len++;
}

void cw_emit_byte(cwRuntime* cw, uint8_t byte)
{
    cw_chunk_write(cw->chunk, byte, cw->previous.line);
}

void cw_emit_bytes(cwRuntime* cw, uint8_t a, uint8_t b)
{
    cw_emit_byte(cw, a);
    cw_emit_byte(cw, b);
}

void cw_emit_return(cwRuntime* cw)
{
    cw_emit_byte(cw, OP_RETURN);
}