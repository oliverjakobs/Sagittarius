#include "chunk.h"

void cw_chunk_init(Chunk* chunk)
{
    chunk->bytes = NULL;
    chunk->constants = NULL;
}

void cw_chunk_free(Chunk* chunk)
{
    tb_array_free(chunk->bytes);
    tb_array_free(chunk->constants);
    cw_chunk_init(chunk);
}

void cw_chunk_write(Chunk* chunk, uint8_t byte, int line)
{
    tb_array_push(chunk->bytes, ((ByteCode){ byte, line }));
}

int cw_chunk_add_constant(Chunk* chunk, Value val)
{
    tb_array_push(chunk->constants, val);
    return tb_array_len(chunk->constants) - 1;
}