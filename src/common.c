#include "common.h"

#include "memory.h"
#include "runtime.h"

#include <string.h>
#include <math.h>

/* --------------------------| value |--------------------------------------------------- */
bool cw_is_falsey(cwValue val)
{
    return IS_NULL(val) || (IS_BOOL(val) && !AS_BOOL(val)) 
        || (IS_NUMBER(val) && (AS_INT(val) == 0 || fpclassify(AS_FLOAT(val)) == FP_ZERO));
}

bool cw_values_equal(cwValue a, cwValue b)
{
    if (a.type == b.type)
    {
        switch (a.type)
        {
        case VAL_NULL:   return true;
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_INT:    return AS_INT(a) == AS_INT(b);
        case VAL_FLOAT:  return AS_FLOAT(a) == AS_FLOAT(b);
        case VAL_OBJECT: return AS_OBJECT(a) == AS_OBJECT(b);
        }
    }

    return false;
}

cwValue* cw_value_add(cwValue* a, const cwValue* b)
{
    if (!cw_is_number(*a) || !cw_is_number(*b)) return NULL;

    if (a->type == VAL_FLOAT || b->type == VAL_FLOAT)
    {
        a->as.fval = cw_valtof(*a) + cw_valtof(*b);
        a->type = VAL_FLOAT;
    }
    else
    {
        a->as.ival += b->as.ival;
        a->type = VAL_INT;
    }

    return a;
}

cwValue* cw_value_sub(cwValue* a, const cwValue* b)
{
    if (!cw_is_number(*a) || !cw_is_number(*b)) return NULL;

    if (a->type == VAL_FLOAT || b->type == VAL_FLOAT)
    {
        a->as.fval = cw_valtof(*a) - cw_valtof(*b);
        a->type = VAL_FLOAT;
    }
    else
    {
        a->as.ival -= b->as.ival;
        a->type = VAL_INT;
    }

    return a;
}

cwValue* cw_value_mult(cwValue* a, const cwValue* b)
{
    if (!cw_is_number(*a) || !cw_is_number(*b)) return NULL;

    if (a->type == VAL_FLOAT || b->type == VAL_FLOAT)
    {
        a->as.fval = cw_valtof(*a) * cw_valtof(*b);
        a->type = VAL_FLOAT;
    }
    else
    {
        a->as.ival *= b->as.ival;
        a->type = VAL_INT;
    }

    return a;
}

cwValue* cw_value_div(cwValue* a, const cwValue* b)
{
    if (!cw_is_number(*a) || !cw_is_number(*b)) return NULL;

    if (a->type == VAL_FLOAT || b->type == VAL_FLOAT)
    {
        a->as.fval = cw_valtof(*a) / cw_valtof(*b);
        a->type = VAL_FLOAT;
    }
    else
    {
        a->as.ival /= b->as.ival;
        a->type = VAL_INT;
    }

    return a;
}

/* --------------------------| chunk |--------------------------------------------------- */
void cw_chunk_init(cwChunk* chunk)
{
    chunk->bytes = NULL;
    chunk->lines = NULL;
    chunk->len = 0;
    chunk->cap = 0;
    chunk->constants = NULL;
    chunk->const_len = 0;
    chunk->const_cap = 0;
}

void cw_chunk_free(cwChunk* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->bytes, chunk->cap);
    CW_FREE_ARRAY(int, chunk->lines, chunk->cap);
    CW_FREE_ARRAY(cwValue, chunk->constants, chunk->const_cap);
    cw_chunk_init(chunk);
}

/* --------------------------| objects |------------------------------------------------- */
static cwObject* cw_object_alloc(cwRuntime* cw, size_t size, cwObjectType type)
{
    cwObject* object = cw_reallocate(NULL, 0, size);
    object->type = type;
    object->next = cw->objects;
    cw->objects = object;
    return object;
}

static void cw_object_free(cwObject* object)
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
    cwObject* object = cw->objects;
    while (object != NULL)
    {
        cwObject* next = object->next;
        cw_object_free(object);
        object = next;
    }
}

/* --------------------------| strings |------------------------------------------------- */
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