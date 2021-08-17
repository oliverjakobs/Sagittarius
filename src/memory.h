#ifndef CLOCKWORK_MEMORY
#define CLOCKWORK_MEMORY

#include "common.h"

#define CW_GROW_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)

#define CW_ALLOCATE(type, size)             cw_reallocate(NULL, 0, sizeof(type) * (size))
#define CW_GROW_ARRAY(type, arr, old, size) cw_reallocate(arr, sizeof(type) * (old), sizeof(type) * (size))
#define CW_FREE_ARRAY(type, arr, old)       cw_reallocate(arr, sizeof(type) * (old), 0)

void* cw_reallocate(void* block, size_t old_size, size_t new_size);


#endif /* !CLOCKWORK_MEMORY */