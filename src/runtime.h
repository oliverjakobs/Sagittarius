#ifndef CW_RUNTIME_H
#define CW_RUNTIME_H

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>

#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "table.h"

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#define CW_STACK_MAX 256

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

struct cwRuntime
{
    /* scanner */
    const char* start;
    const char* cursor;
    int line;
    
    /* Parser */
    Chunk* chunk;

    Token current;
    Token previous;
    
    bool error;
    bool panic;

    /* VM */
    uint8_t* ip;

    Value stack[CW_STACK_MAX];
    size_t stack_index;

    Object* objects;
    Table globals;
    Table strings;
};

void cw_init(cwRuntime* cw);
void cw_free(cwRuntime* cw);

InterpretResult cw_interpret(cwRuntime* cw, const char* src);

/* stack operations */
void  cw_push_stack(cwRuntime* cw, Value val);
Value cw_pop_stack(cwRuntime* cw);
void  cw_reset_stack(cwRuntime* cw);
Value cw_peek_stack(cwRuntime* cw, int distance);

#endif /* !CW_RUNTIME_H */
