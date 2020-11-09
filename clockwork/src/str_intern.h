#ifndef STR_INTERN_H
#define STR_INTERN_H

#include <string.h>

#include "memory.h"

const char* str_intern_range(const char* start, const char* end);
const char* str_intern(const char* str);

Arena* get_str_arena();

#endif /* !STR_INTERN_H */
