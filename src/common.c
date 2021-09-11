#include "common.h"

#include "memory.h"
#include "runtime.h"

#include <string.h>
#include <math.h>

cwValue* cw_value_add(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)
        a->fval = cw_valtof(*a) + cw_valtof(*b);
    else
        a->ival += b->ival;
    a->type = result_type;
    return a;
}

cwValue* cw_value_sub(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)
        a->fval = cw_valtof(*a) - cw_valtof(*b);
    else
        a->ival -= b->ival;
    a->type = result_type;
    return a;
}

cwValue* cw_value_mul(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)
        a->fval = cw_valtof(*a) * cw_valtof(*b);
    else
        a->ival *= b->ival;
    a->type = result_type;
    return a;
}

cwValue* cw_value_div(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)
        a->fval = cw_valtof(*a) / cw_valtof(*b);
    else
        a->ival /= b->ival;
    a->type = result_type;
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
    CW_FREE_ARRAY(int,     chunk->lines, chunk->cap);
    CW_FREE_ARRAY(cwValue, chunk->constants, chunk->const_cap);
    cw_chunk_init(chunk);
}