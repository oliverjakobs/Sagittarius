#ifndef CLOCKWORK_VALUE_H
#define CLOCKWORK_VALUE_H

#include "common.h"

typedef double Value;

typedef struct
{
    int capacity;
    int count;
    Value* values;
} ValueArray;

void value_array_init(ValueArray* arr);
void value_array_free(ValueArray* arr);
void value_array_write(ValueArray* arr, Value value);

void print_value(Value value);

#endif /* !CLOCKWORK_VALUE_H */
