#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#include "debug.h"
#include "memory.h"
#include "runtime.h"

/* --------------------------| writing byte code |--------------------------------------- */
void cw_emit_byte(cwChunk* chunk, uint8_t byte, int line)
{
    if (chunk->cap < chunk->len + 1)
    {
        int old_cap = chunk->cap;
        chunk->cap = CW_GROW_CAPACITY(old_cap);
        chunk->bytes = CW_GROW_ARRAY(uint8_t, chunk->bytes, old_cap, chunk->cap);
        chunk->lines = CW_GROW_ARRAY(int,     chunk->lines, old_cap, chunk->cap);
    }

    chunk->bytes[chunk->len] = byte;
    chunk->lines[chunk->len] = line;
    chunk->len++;
}

void cw_emit_bytes(cwChunk* chunk, uint8_t a, uint8_t b, int line)
{
    cw_emit_byte(chunk, a, line);
    cw_emit_byte(chunk, b, line);
}

void cw_emit_uint32(cwChunk* chunk, uint32_t value, int line)
{
    cw_emit_byte(chunk, (value >> 24) & 0xff, line);
    cw_emit_byte(chunk, (value >> 16) & 0xff, line);
    cw_emit_byte(chunk, (value >> 8) & 0xff, line);
    cw_emit_byte(chunk, value & 0xff, line);
}

int cw_emit_jump(cwChunk* chunk, uint8_t instruction, int line)
{
    cw_emit_byte(chunk, instruction, line);
    cw_emit_byte(chunk, 0xff, line);
    cw_emit_byte(chunk, 0xff, line);
    return chunk->len - 2;
}

void cw_patch_jump(cwRuntime* cw, int offset)
{
    /* -2 to adjust for the bytecode for the jump offset itself. */
    int jump = cw->chunk->len - offset - 2;

    if (jump > UINT16_MAX) cw_syntax_error_at(&cw->previous, "Too much code to jump over.");

    cw->chunk->bytes[offset] = (jump >> 8) & 0xff;
    cw->chunk->bytes[offset + 1] = jump & 0xff;
}

void cw_emit_loop(cwRuntime* cw, int start)
{
    cw_emit_byte(cw->chunk, OP_LOOP, cw->previous.line);

    int offset = cw->chunk->len - start + 2;
    if (offset > UINT16_MAX) cw_syntax_error_at(&cw->previous, "Loop body too large.");

    cw_emit_byte(cw->chunk, (offset >> 8) & 0xff, cw->previous.line);
    cw_emit_byte(cw->chunk, offset & 0xff, cw->previous.line);
}

/* --------------------------| compiling |----------------------------------------------- */
static void cw_compiler_end(cwRuntime* cw)
{
    cw_emit_byte(cw->chunk, OP_RETURN, cw->previous.line);
#ifdef DEBUG_PRINT_CODE
    cw_disassemble_chunk(cw->chunk, "code");
#endif 
}

bool cw_compile(cwRuntime* cw, const char* src, cwChunk* chunk)
{
    /* init first token */
    cw->current.type = TOKEN_NULL;
    cw->current.start = src;
    cw->current.end = src;
    cw->current.line = 1;

    /* init compiler */
    cw->chunk = chunk;

    cw_next_token(&cw->current, &cw->previous);

    while (!cw_match(cw, TOKEN_EOF))
    {
        cw_parse_expression(cw);
    }

    cw_compiler_end(cw);
    return true;
}