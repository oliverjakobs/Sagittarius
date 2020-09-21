#include "value.h"

#include "memory.h"

#include <stdio.h>

void value_array_init(ValueArray* arr)
{
    arr->values = NULL;
    arr->capacity = 0;
    arr->count = 0;
}

void value_array_free(ValueArray* arr)
{
    FREE_ARRAY(Value, arr->values, arr->capacity);
    value_array_init(arr);
}

void value_array_write(ValueArray* arr, Value value)
{
    if (arr->capacity < arr->count + 1)
    {
        size_t old_cap = arr->capacity;
        arr->capacity = GROW_CAPACITY(old_cap);
        arr->values = GROW_ARRAY(Value, arr->values, old_cap, arr->capacity);
    }

    arr->values[arr->count] = value;
    arr->count++;
}

void print_value(Value value)
{
    printf("%g", value);
}