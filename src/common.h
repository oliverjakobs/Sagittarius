#ifndef CLOCKWORK_COMMON_H
#define CLOCKWORK_COMMON_H

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

typedef struct VM VM;

typedef struct Object Object;
typedef struct cwString cwString;

typedef enum
{
    VAL_NULL = 0, 
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJECT
} ValueType;

typedef struct
{
    ValueType type;
    union
    {
        bool boolean;
        double num;
        Object* object;
    } as;
} Value;

#define IS_NULL(value)    ((value).type == VAL_NULL)
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
#define IS_OBJECT(value)  ((value).type == VAL_OBJECT)

#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.num)
#define AS_OBJECT(value)  ((value).as.object)

#define MAKE_NULL(val)    ((Value){ .type = VAL_NULL,   { .num = 0 }})
#define MAKE_BOOL(val)    ((Value){ .type = VAL_BOOL,   { .boolean = val }})
#define MAKE_NUMBER(val)  ((Value){ .type = VAL_NUMBER, { .num = val }})
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

cwString* cw_str_copy(VM* vm, const char* src, size_t len);
cwString* cw_str_concat(VM* vm, cwString* a, cwString* b);

uint32_t cw_hash_str(const char* str, size_t len);

#endif /* !CLOCKWORK_COMMON_H */
