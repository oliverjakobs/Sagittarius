#include "memory.h"

#include "runtime.h"

void* cw_reallocate(void* block, size_t old_size, size_t new_size)
{
    if (new_size == 0)
    {
        free(block);
        return NULL;
    }

    void* result = realloc(block, new_size);
    if (result == NULL) exit(1);
    return result;
}