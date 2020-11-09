#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>

void* xmalloc(size_t num_bytes);
void* xcalloc(size_t num_elems, size_t elem_size);
void* xrealloc(void* ptr, size_t num_bytes);

#define ALIGN_DOWN(n, a) ((n) & ~((a) - 1))
#define ALIGN_UP(n, a) ALIGN_DOWN((n) + (a) - 1, (a))
#define ALIGN_DOWN_PTR(p, a) ((void *)ALIGN_DOWN((uintptr_t)(p), (a)))
#define ALIGN_UP_PTR(p, a) ((void *)ALIGN_UP((uintptr_t)(p), (a)))

// Arena allocator

typedef struct
{
    char* ptr;
    char* end;
    char** blocks;
} Arena;

#define ARENA_ALIGNMENT 8
//#define ARENA_BLOCK_SIZE (1024 * 1024)
#define ARENA_BLOCK_SIZE 1024

void arena_grow(Arena* arena, size_t min_size);
void* arena_alloc(Arena* arena, size_t size);

void arena_free(Arena* arena);


#endif // !MEMORY_H
