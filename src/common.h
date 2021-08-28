#ifndef CLOCKWORK_COMMON_H
#define CLOCKWORK_COMMON_H

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

typedef struct cwRuntime cwRuntime;
typedef struct Token Token;
typedef struct Chunk Chunk;

typedef struct Object Object;
typedef struct cwString cwString;

typedef enum
{
    VAL_NULL = 0, 
    VAL_BOOL,
    VAL_INT,
    VAL_FLOAT,
    VAL_OBJECT
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        int32_t ival;
        float fval;
        Object* object;
    } as;
} Value;

#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_INT(value)     ((value).type == VAL_INT)
#define IS_FLOAT(value)   ((value).type == VAL_FLOAT)
#define IS_NUMBER(value)  (cw_is_number(value))
#define IS_OBJECT(value)  ((value).type == VAL_OBJECT)

static inline bool cw_is_number(Value val) { return val.type > VAL_NULL && val.type <= VAL_FLOAT; }
static inline int32_t cw_valtoi(Value val) { return IS_FLOAT(val) ? (int32_t)val.as.fval : val.as.ival; }
static inline float   cw_valtof(Value val) { return IS_FLOAT(val) ? val.as.fval : (float)val.as.ival; }

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_INT(value)     (cw_valtoi(value))
#define AS_FLOAT(value)   (cw_valtof(value))
#define AS_OBJECT(value)  ((value).as.object)

#define MAKE_NULL(val)    ((Value){ .type = VAL_NULL,   { .ival = 0 }})
#define MAKE_BOOL(val)    ((Value){ .type = VAL_BOOL,   { .boolean = val }})
#define MAKE_INT(val)     ((Value){ .type = VAL_INT,    { .ival = val }})
#define MAKE_FLOAT(val)   ((Value){ .type = VAL_FLOAT,  { .fval = val }})
#define MAKE_OBJECT(obj)  ((Value){ .type = VAL_OBJECT, { .object = (Object*)obj }})

/* null, false and 0 are falsey and every other value behaves like true */
bool cw_is_falsey(Value val);
bool cw_values_equal(Value a, Value b);

typedef enum
{
    OBJ_STRING,
} ObjectType;

struct Object
{
    ObjectType type;
    Object* next;
};

struct cwString
{
    Object obj;
    char* raw;
    size_t len;
    uint32_t hash;
};

static inline bool cw_is_obj_type(Value value, ObjectType type) 
{ 
    return IS_OBJECT(value) && AS_OBJECT(value)->type == type;
}

#define OBJECT_TYPE(value)  (AS_OBJECT(value)->type)
#define IS_STRING(value)    cw_is_obj_type(value, OBJ_STRING)

#define AS_STRING(value)    ((cwString*)AS_OBJECT(value))
#define AS_RAWSTRING(value) (AS_STRING(value)->raw)

void cw_free_objects(cwRuntime* cw);

cwString* cw_str_take(cwRuntime* cw, char* src, size_t len);
cwString* cw_str_copy(cwRuntime* cw, const char* src, size_t len);
cwString* cw_str_concat(cwRuntime* cw, cwString* a, cwString* b);

cwString* cw_find_str(cwRuntime* cw, const char* str, size_t len);

uint32_t cw_hash_str(const char* str, size_t len);

#endif /* !CLOCKWORK_COMMON_H */
