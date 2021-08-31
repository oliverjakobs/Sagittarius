#include "chunk.h"

#include "debug.h"
#include "memory.h"
#include "runtime.h"

void cw_chunk_init(cwChunk* chunk)
{
    chunk->bytes = NULL;
    chunk->lines = NULL;
    chunk->len = 0;
    chunk->cap = 0;
    chunk->constants = NULL;
    chunk->const_len = 0;
    chunk->const_cap = 0;
}

void cw_chunk_free(cwChunk* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->bytes, chunk->cap);
    CW_FREE_ARRAY(int, chunk->lines, chunk->cap);
    CW_FREE_ARRAY(cwValue, chunk->constants, chunk->const_cap);
    cw_chunk_init(chunk);
}

void cw_chunk_write(cwChunk* chunk, uint8_t byte, int line)
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

int cw_chunk_add_constant(cwChunk* chunk, cwValue val)
{
    if (chunk->const_cap < chunk->const_len + 1)
    {
        int old_cap = chunk->const_cap;
        chunk->const_cap = CW_GROW_CAPACITY(old_cap);
        chunk->constants = CW_GROW_ARRAY(cwValue, chunk->constants, old_cap, chunk->const_cap);
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

int cw_emit_jump(cwRuntime* cw, uint8_t instruction)
{
    cw_emit_byte(cw, instruction);
    cw_emit_byte(cw, 0xff);
    cw_emit_byte(cw, 0xff);
    return cw->chunk->len - 2;
}

void cw_patch_jump(cwRuntime* cw, int offset)
{
    /* -2 to adjust for the bytecode for the jump offset itself. */
    int jump = cw->chunk->len - offset - 2;

    if (jump > UINT16_MAX)
        cw_syntax_error_at(cw, &cw->previous, "Too much code to jump over.");

    cw->chunk->bytes[offset] = (jump >> 8) & 0xff;
    cw->chunk->bytes[offset + 1] = jump & 0xff;
}