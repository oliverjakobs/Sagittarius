#include "common.h"

#include "memory.h"
#include "runtime.h"

#include <string.h>
#include <math.h>

int cw_value_is_falsey(const cwValue* val)
{
    return val->type == CW_VALUE_NULL                                                       /* null is falsey */
        || ((val->type == CW_VALUE_BOOL || val->type == CW_VALUE_INT) && val->ival == 0)    /* value of 0 is falsey if type is bool or int */
        || (val->type == CW_VALUE_FLOAT && fpclassify(val->fval) == FP_ZERO);               /* a value that classifies as FP_ZERO is falsey if type is float */
}

/* --------------------------| chunk |--------------------------------------------------- */
void cw_chunk_init(cwChunk* chunk)
{
    chunk->bytes = NULL;
    chunk->lines = NULL;
    chunk->len = 0;
    chunk->cap = 0;
}

void cw_chunk_free(cwChunk* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->bytes, chunk->cap);
    CW_FREE_ARRAY(int,     chunk->lines, chunk->cap);
    cw_chunk_init(chunk);
}