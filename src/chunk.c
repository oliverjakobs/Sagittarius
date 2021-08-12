#include "chunk.h"

#include "memory.h"

void cw_chunk_init(Chunk* chunk)
{
    chunk->bytes = NULL;
    chunk->lines = NULL;
    chunk->len = 0;
    chunk->cap = 0;
    chunk->constants = NULL;
}

void cw_chunk_free(Chunk* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->bytes, chunk->cap);
    CW_FREE_ARRAY(uint8_t, chunk->lines, chunk->cap);
    tb_array_free(chunk->constants);
    cw_chunk_init(chunk);
}

void cw_chunk_write(Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->cap < chunk->len + 1)
    {
        int old_cap = chunk->cap;
        chunk->cap = CW_GROW_CAPACITY(old_cap);
        chunk->bytes = CW_GROW_ARRAY(uint8_t, chunk->bytes, old_cap, chunk->cap);
        chunk->lines = CW_GROW_ARRAY(uint8_t, chunk->lines, old_cap, chunk->cap);
    }

    chunk->bytes[chunk->len] = byte;
    chunk->lines[chunk->len] = line;
    chunk->len++;
}

int cw_chunk_add_constant(Chunk* chunk, Value val)
{
    tb_array_push(chunk->constants, val);
    return tb_array_len(chunk->constants) - 1;
}