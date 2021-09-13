#ifndef CLOCKWORK_COMMON_H
#define CLOCKWORK_COMMON_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct cwRuntime cwRuntime;
typedef struct cwToken cwToken;

typedef enum
{
    CW_VALUE_NULL,
    CW_VALUE_BOOL,
    CW_VALUE_INT,
    CW_VALUE_FLOAT
} cwValueType;

typedef struct
{
    cwValueType type;
    union
    {
        int32_t ival;
        float   fval;
    };
} cwValue;

#define CW_MAKE_NULL()      ((cwValue){ .type = CW_VALUE_NULL,  { .ival = 0 }})
#define CW_MAKE_BOOL(val)   ((cwValue){ .type = CW_VALUE_BOOL,  { .ival = val }})
#define CW_MAKE_INT(val)    ((cwValue){ .type = CW_VALUE_INT,   { .ival = val }})
#define CW_MAKE_FLOAT(val)  ((cwValue){ .type = CW_VALUE_FLOAT, { .fval = val }})

static inline bool cw_valuetype_numeric(cwValueType type) { return type >= CW_VALUE_BOOL && type <= CW_VALUE_FLOAT; }
static inline cwValueType cw_valuetype_max(cwValueType a, cwValueType b) { return a < b ? b : a; }
static inline int32_t cw_valtoi(cwValue val) { return val.type == CW_VALUE_FLOAT ? (int32_t)val.fval : val.ival; }
static inline float   cw_valtof(cwValue val) { return val.type == CW_VALUE_FLOAT ? val.fval : (float)val.ival; }

cwValue* cw_value_add(cwValue* a, const cwValue* b, cwValueType result_type);
cwValue* cw_value_sub(cwValue* a, const cwValue* b, cwValueType result_type);
cwValue* cw_value_mul(cwValue* a, const cwValue* b, cwValueType result_type);
cwValue* cw_value_div(cwValue* a, const cwValue* b, cwValueType result_type);

/* chunk */
typedef struct
{
    /* byte code with line information */
    uint8_t* bytes;
    int*     lines;
    size_t len;
    size_t cap;

    /* constants */
    cwValue* constants;
    size_t const_len;
    size_t const_cap;
} cwChunk;

void cw_chunk_init(cwChunk* chunk);
void cw_chunk_free(cwChunk* chunk);

#endif /* !CLOCKWORK_COMMON_H */
