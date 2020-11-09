#include "memory.h"

#include <assert.h>

#include "tb_stretchy.h"

void* xmalloc(size_t num_bytes)
{
    void* ptr = malloc(num_bytes);
    if (!ptr)
    {
        perror("xmalloc failed");
        exit(1);
    }
    return ptr;
}

void* xcalloc(size_t num_elems, size_t elem_size) 
{
    void* ptr = calloc(num_elems, elem_size);
    if (!ptr)
    {
        perror("xcalloc failed");
        exit(1);
    }
    return ptr;
}

void* xrealloc(void* ptr, size_t num_bytes)
{
    ptr = realloc(ptr, num_bytes);
    if (!ptr)
    {
        perror("xrealloc failed");
        exit(1);
    }
    return ptr;
}

void arena_grow(Arena* arena, size_t min_size)
{
    size_t size = ALIGN_UP((ARENA_BLOCK_SIZE > min_size) ? ARENA_BLOCK_SIZE : min_size, ARENA_ALIGNMENT);
    arena->ptr = xmalloc(size);
    arena->end = arena->ptr + size;
    tb_stretchy_push(arena->blocks, arena->ptr);
}

void* arena_alloc(Arena* arena, size_t size)
{
    if (size > (size_t)(arena->end - arena->ptr))
    {
        arena_grow(arena, size);
        assert(size <= (size_t)(arena->end - arena->ptr));
    }

    void* ptr = arena->ptr;
    arena->ptr = ALIGN_UP_PTR(arena->ptr + size, ARENA_ALIGNMENT);
    assert(arena->ptr <= arena->end);
    assert(ptr == ALIGN_DOWN_PTR(ptr, ARENA_ALIGNMENT));
    return ptr;
}

void arena_free(Arena* arena)
{
    for (char** it = arena->blocks; it != tb_stretchy_last(arena->blocks); it++)
    {
        free(*it);
    }
}