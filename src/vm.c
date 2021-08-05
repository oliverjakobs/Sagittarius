#include "vm.h"

#include <stdio.h>
#include "debug.h"

static void cw_reset_stack(VM* vm)
{
    vm->stack_top = vm->stack;
}

void cw_vm_init(VM* vm)
{
    cw_reset_stack(vm);
}

void cw_vm_free(VM* vm)
{

}

static InterpretResult cw_run(VM* vm)
{
#define READ_BYTE()     ((vm->ip++)->data)
#define READ_CONSTANT() (vm->chunk->constants[READ_BYTE()])
#define BINARY_OP(op)               \
    do {                            \
      double b = cw_pop_stack(vm);  \
      double a = cw_pop_stack(vm);  \
      cw_push_stack(vm, a op b);    \
    } while (false)

    while (true)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stack_top; ++slot)
        {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");
        cw_disassemble_instruction(vm->chunk, (int)(vm->ip - vm->chunk->bytes));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT();
                cw_push_stack(vm, constant);
                break;
            }
            case OP_ADD:      BINARY_OP(+); break;
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE:   BINARY_OP(/); break;
            case OP_NEGATE: cw_push_stack(vm, -cw_pop_stack(vm)); break;
            case OP_RETURN:
            {
                printValue(cw_pop_stack(vm));
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult cw_interpret(VM* vm, Chunk* chunk)
{
    vm->chunk = chunk;
    vm->ip = vm->chunk->bytes;
    return cw_run(vm);
}

void  cw_push_stack(VM* vm, Value val)
{
    *vm->stack_top = val;
    vm->stack_top++;
}

Value cw_pop_stack(VM* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}
