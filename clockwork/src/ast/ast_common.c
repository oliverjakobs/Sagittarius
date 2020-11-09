#include "ast_common.h"

static Arena ast_arena;

void* ast_alloc(size_t size)
{
    assert(size != 0);
    void* block = arena_alloc(&ast_arena, size);
    memset(block, 0, size);
    return block;
}

void* ast_dup(const void* src, size_t size)
{
    if (size == 0) return NULL;

    void* block = arena_alloc(&ast_arena, size);
    memcpy(block, src, size);
    return block;
}