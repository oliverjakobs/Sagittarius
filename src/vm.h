#ifndef CLOCKWORK_VM_H
#define CLOCKWORK_VM_H

#include "chunk.h"

#define STACK_MAX 256

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

struct VM
{
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX];
    size_t stack_index;

    Object* objects;
};

void cw_vm_init(VM* vm);
void cw_vm_free(VM* vm);

InterpretResult cw_interpret(VM* vm, const char* src);

void  cw_push_stack(VM* vm, Value val);
Value cw_pop_stack(VM* vm);

#endif /* !CLOCKWORK_VM_H */