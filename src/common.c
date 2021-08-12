#include "common.h"

#define TB_ARRAY_IMPLEMENTATION
#include "tb_array.h"

#include "memory.h"

#include <string.h>

bool cw_is_falsey(Value val)
{
    return IS_NULL(val) || (IS_BOOL(val) && !AS_BOOL(val)) || (IS_NUMBER(val) && AS_NUMBER(val) == 0);
}

bool cw_values_equal(Value a, Value b)
{
    if (a.type == b.type)
    {
        switch (a.type)
        {
        case VAL_NULL:   return true;
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJECT:
        {
            cwString* a_str = AS_STRING(a);
            cwString* b_str = AS_STRING(b);
            return a_str->len == b_str->len && memcmp(a_str->raw, b_str->raw, a_str->len) == 0;
        }
        }
    } 
    return false;
}

cwString* cw_str_copy(VM* vm, const char* src, size_t len)
{
    char* raw = cw_reallocate(NULL, 0, len + 1);
    memcpy(raw, src, len);
    raw[len] = '\0';
    return cw_str_alloc(vm, raw, len);
}

cwString* cw_str_concat(VM* vm, cwString* a, cwString* b)
{
    size_t len = a->len + b->len;
    char* raw = cw_reallocate(NULL, 0, len + 1);
    memcpy(raw, a->raw, a->len);
    memcpy(raw + a->len, b->raw, b->len);
    raw[len] = '\0';

    return cw_str_alloc(vm, raw, len);
}