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
    int32_t val;
} cwValue;

#define CW_MAKE_NULL()      ((cwValue){ .type = CW_VALUE_NULL, .val = 0})
#define CW_MAKE_BOOL(value) ((cwValue){ .type = CW_VALUE_BOOL, .val = value})
#define CW_MAKE_INT(value)  ((cwValue){ .type = CW_VALUE_INT,  .val = value})

static inline bool cw_valuetype_numeric(cwValueType type) { return type >= CW_VALUE_BOOL && type <= CW_VALUE_INT; }
static inline cwValueType cw_valuetype_max(cwValueType a, cwValueType b) { return a < b ? b : a; }

int cw_value_is_falsey(const cwValue* val);
bool cw_value_equal(cwValue a, cwValue b);

/* chunk */
typedef struct
{
    /* byte code with line information */
    uint8_t* bytes;
    int*     lines;
    size_t len;
    size_t cap;
} cwChunk;

void cw_chunk_init(cwChunk* chunk);
void cw_chunk_free(cwChunk* chunk);

#endif /* !CLOCKWORK_COMMON_H */
