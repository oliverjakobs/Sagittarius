#include "chunk.h"

#include "memory.h"

void chunk_init(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_write(Chunk* chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        size_t old_cap = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_cap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old_cap, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

uint8_t chunk_add_constant(Chunk* chunk, Value value)
{
    value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}
