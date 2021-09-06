#ifndef CW_RUNTIME_H
#define CW_RUNTIME_H

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "common.h"
#include "compiler.h"
#include "table.h"

#define DEBUG_PRINT_CODE
// #define DEBUG_TRACE_EXECUTION

#define CW_STACK_MAX 256

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

struct cwRuntime
{
    /* Compiler */
    cwChunk* chunk;

    cwLocal locals[UINT8_MAX + 1];
    int local_count;
    int scope_depth;

    /* Parser */
    cwToken current;
    cwToken previous;
    
    bool error;
    bool panic;

    /* VM */
    uint8_t* ip;

    cwValue stack[CW_STACK_MAX];
    size_t stack_index;

    Table globals;
    Table strings;

    /* Garbage Collection */
    cwObject* objects;
};

void cw_init(cwRuntime* cw);
void cw_free(cwRuntime* cw);

InterpretResult cw_interpret(cwRuntime* cw, const char* src);

/* stack operations */
void    cw_push_stack(cwRuntime* cw, cwValue val);
cwValue cw_pop_stack(cwRuntime* cw);
void    cw_reset_stack(cwRuntime* cw);
cwValue cw_peek_stack(cwRuntime* cw, int distance); /* TODO: make peek return a pointer */

#endif /* !CW_RUNTIME_H */
