#ifndef CLOCKWORK_MEMORY
#define CLOCKWORK_MEMORY

#include "common.h"

#define CW_GROW_CAPACITY(cap) ((cap) < 8 ? 8 : (cap) * 2)
#define CW_GROW_ARRAY(type, arr, old, size) cw_reallocate(arr, sizeof(type) * (old), sizeof(type) * (size))
#define CW_FREE_ARRAY(type, arr, old)       cw_reallocate(arr, sizeof(type) * (old), 0)

void* cw_reallocate(void* block, size_t old_size, size_t new_size);

cwString* cw_str_alloc(VM* vm, char* src, size_t len);

void cw_free_objects(VM* vm);

#endif /* !CLOCKWORK_MEMORY */