#ifndef CLOCKWORK_COMMON_H
#define CLOCKWORK_COMMON_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct cwRuntime cwRuntime;
typedef struct cwToken cwToken;

typedef struct cwObject cwObject;
typedef struct cwString cwString;
typedef struct cwFunction cwFunction;

/* value */
typedef enum
{
    VAL_NULL = 0, 
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_OBJECT
} cwValueType;

typedef struct
{
    cwValueType type;
    bool mut;
    union
    {
        int32_t ival;
        float fval;
        cwObject* object;
    } as;
} cwValue;

#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_INT(value)     ((value).type == VAL_INT)
#define IS_FLOAT(value)   ((value).type == VAL_FLOAT)
#define IS_NUMBER(value)  (cw_is_number(value))
#define IS_OBJECT(value)  ((value).type == VAL_OBJECT)

static inline bool cw_is_number(cwValue val) { return val.type > VAL_NULL && val.type <= VAL_FLOAT; }
static inline int32_t cw_valtoi(cwValue val) { return IS_FLOAT(val) ? (int32_t)val.as.fval : val.as.ival; }
static inline float   cw_valtof(cwValue val) { return IS_FLOAT(val) ? val.as.fval : (float)val.as.ival; }

#define AS_BOOL(value)    ((value).as.ival)
#define AS_INT(value)     (cw_valtoi(value))
#define AS_FLOAT(value)   (cw_valtof(value))
#define AS_OBJECT(value)  ((value).as.object)

#define MAKE_NULL(val)    ((cwValue){ .type = VAL_NULL,   .mut = false, { .ival = 0 }})
#define MAKE_BOOL(val)    ((cwValue){ .type = VAL_BOOL,   .mut = false, { .ival = val }})
#define MAKE_INT(val)     ((cwValue){ .type = VAL_INT,    .mut = false, { .ival = val }})
#define MAKE_FLOAT(val)   ((cwValue){ .type = VAL_FLOAT,  .mut = false, { .fval = val }})
#define MAKE_OBJECT(obj)  ((cwValue){ .type = VAL_OBJECT, .mut = false, { .object = (cwObject*)obj }})

cwValue* cw_value_add(cwValue* a, const cwValue* b);
cwValue* cw_value_sub(cwValue* a, const cwValue* b);
cwValue* cw_value_mult(cwValue* a, const cwValue* b);
cwValue* cw_value_div(cwValue* a, const cwValue* b);

/* null, false and 0 are falsey and every other value behaves like true */
bool cw_is_falsey(cwValue val);
bool cw_values_equal(cwValue a, cwValue b);

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

/* objects */
typedef enum
{
    OBJ_STRING,
} cwObjectType;

struct cwObject
{
    cwObjectType type;
    cwObject* next;
};

struct cwFunction
{
    cwObject obj;
    cwString* name;
    cwChunk chunk;
    int arity;
};

static inline bool cw_is_obj_type(cwValue value, cwObjectType type) 
{ 
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#define OBJECT_TYPE(value)  (AS_OBJECT(value)->type)
#define IS_STRING(value)    cw_is_obj_type(value, OBJ_STRING)

#define AS_STRING(value)    ((cwString*)AS_OBJECT(value))
#define AS_RAWSTRING(value) (AS_STRING(value)->raw)

void cw_free_objects(cwRuntime* cw);

/* strings */
struct cwString
{
    cwObject obj;
    char* raw;
    size_t len;
    uint32_t hash;
};

cwString* cw_str_take(cwRuntime* cw, char* src, size_t len);
cwString* cw_str_copy(cwRuntime* cw, const char* src, size_t len);
cwString* cw_str_concat(cwRuntime* cw, cwString* a, cwString* b);

cwString* cw_find_str(cwRuntime* cw, const char* str, size_t len);
uint32_t cw_hash_str(const char* str, size_t len);

#endif /* !CLOCKWORK_COMMON_H */
