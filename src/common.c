#include "common.h"

#include "memory.h"
#include "runtime.h"

#include <string.h>
#include <math.h>

int cw_value_cmp(cwValue a, cwValue b)
{
    if (a.type == CW_VALUE_FLOAT || b.type == CW_VALUE_FLOAT) return cw_valtof(a) - cw_valtof(b);
    return a.ival - b.ival;
}

int cw_value_is_falsey(const cwValue* val)
{
    return val->type == CW_VALUE_NULL                                                       /* null is falsey */
        || ((val->type == CW_VALUE_BOOL || val->type == CW_VALUE_INT) && val->ival == 0)    /* value of 0 is falsey if type is bool or int */
        || (val->type == CW_VALUE_FLOAT && fpclassify(val->fval) == FP_ZERO);               /* a value that classifies as FP_ZERO is falsey if type is float */
}

cwValue* cw_value_add(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)  a->fval = cw_valtof(*a) + cw_valtof(*b);
    else                                a->ival += b->ival;

    a->type = result_type;
    return a;
}

cwValue* cw_value_sub(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)  a->fval = cw_valtof(*a) - cw_valtof(*b);
    else                                a->ival -= b->ival;

    a->type = result_type;
    return a;
}

cwValue* cw_value_mul(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)  a->fval = cw_valtof(*a) * cw_valtof(*b);
    else                                a->ival *= b->ival;

    a->type = result_type;
    return a;
}

cwValue* cw_value_div(cwValue* a, const cwValue* b, cwValueType result_type)
{
    if (result_type == CW_VALUE_FLOAT)  a->fval = cw_valtof(*a) / cw_valtof(*b);
    else                                a->ival /= b->ival;

    a->type = result_type;
    return a;
}

cwValue* cw_value_neg(cwValue* val)
{
    if (val->type == CW_VALUE_FLOAT)  val->fval = -val->fval;
    else                              val->ival = -val->ival;
    return val;
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