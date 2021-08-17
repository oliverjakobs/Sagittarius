#include "common.h"

#include "memory.h"
#include "runtime.h"

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
        case VAL_OBJECT: return AS_OBJECT(a) == AS_OBJECT(b);
        }
    } 
    return false;
}


/* objects */
static Object* cw_object_alloc(cwRuntime* cw, size_t size, ObjectType type)
{
    Object* object = cw_reallocate(NULL, 0, size);
    object->type = type;
    object->next = cw->objects;
    cw->objects = object;
    return object;
}

static void cw_object_free(Object* object)
{
    switch (object->type)
    {
    case OBJ_STRING:
    {
        cwString* str = (cwString*)object;
        CW_FREE_ARRAY(char, str->raw, str->len + 1);
        cw_reallocate(object, sizeof(cwString), 0);
        break;
    }
    }
}

void cw_free_objects(cwRuntime* cw)
{
    Object* object = cw->objects;
    while (object != NULL)
    {
        Object* next = object->next;
        cw_object_free(object);
        object = next;
    }
}

/* strings */
static cwString* cw_str_alloc(cwRuntime* cw, char* src, size_t len, uint32_t hash)
{
    cwString* str = (cwString*)cw_object_alloc(cw, sizeof(cwString), OBJ_STRING);
    str->raw = src;
    str->len = len;
    str->hash = hash;

    cw_table_insert(&cw->strings, str, MAKE_NULL());

    return str;
}

cwString* cw_str_take(cwRuntime* cw, char* src, size_t len)
{
    uint32_t hash = cw_hash_str(src, len);
    cwString* interned = cw_table_find_key(&cw->strings, src, len, hash);
    if (interned != NULL)
    {
        CW_FREE_ARRAY(char, src, len + 1);
        return interned;
    } 

    return cw_str_alloc(cw, src, len, hash);
}

cwString* cw_str_copy(cwRuntime* cw, const char* src, size_t len)
{
    uint32_t hash = cw_hash_str(src, len);
    cwString* interned = cw_table_find_key(&cw->strings, src, len, hash);
    if (interned != NULL) return interned;

    char* raw = cw_reallocate(NULL, 0, len + 1);
    memcpy(raw, src, len);
    raw[len] = '\0';
    return cw_str_alloc(cw, raw, len, hash);
}

cwString* cw_str_concat(cwRuntime* cw, cwString* a, cwString* b)
{
    size_t len = a->len + b->len;
    char* raw = cw_reallocate(NULL, 0, len + 1);
    memcpy(raw, a->raw, a->len);
    memcpy(raw + a->len, b->raw, b->len);
    raw[len] = '\0';

    return cw_str_take(cw, raw, len);
}

uint32_t cw_hash_str(const char* str, size_t len)
{
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++)
    {
        hash ^= (uint8_t)str[i];
        hash *= 16777619;
    }
    return hash;
}