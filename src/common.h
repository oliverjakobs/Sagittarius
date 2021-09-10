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
} cwValueType;

typedef struct
{
    cwValueType type;
    int32_t ival;
} cwValue;

#define CW_MAKE_NULL()      ((cwValue){ .type = CW_VALUE_NULL, .ival = 0})
#define CW_MAKE_BOOL(val)   ((cwValue){ .type = CW_VALUE_BOOL, .ival = val})
#define CW_MAKE_INT(val)    ((cwValue){ .type = CW_VALUE_INT,  .ival = val})

cwValue* cw_value_add(cwValue* a, const cwValue* b);
cwValue* cw_value_sub(cwValue* a, const cwValue* b);
cwValue* cw_value_mul(cwValue* a, const cwValue* b);
cwValue* cw_value_div(cwValue* a, const cwValue* b);

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
