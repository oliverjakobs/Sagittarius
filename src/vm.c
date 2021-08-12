#include "vm.h"

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "memory.h"
#include "compiler.h"

static void cw_reset_stack(VM* vm)
{
    vm->stack_index = 0;
}

static void cw_runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->chunk->bytes - 1;
    int line = vm->chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    cw_reset_stack(vm);
}

void cw_vm_init(VM* vm)
{
    vm->chunk = NULL;
    vm->ip = NULL;
    vm->objects = NULL;
    cw_reset_stack(vm);
}

void cw_vm_free(VM* vm)
{
    cw_free_objects(vm);
}

void  cw_push_stack(VM* vm, Value val)
{
    if (vm->stack_index >= STACK_MAX)
    {
        cw_runtime_error(vm, "Stack overflow");
        return;
    }

    vm->stack[vm->stack_index++] = val;
}

Value cw_pop_stack(VM* vm)
{
    return vm->stack[--vm->stack_index];
}

static Value cw_peek_stack(VM* vm, int distance)
{
    return vm->stack[vm->stack_index - 1 -distance];
}

static InterpretResult cw_run(VM* vm)
{
#define READ_BYTE()     (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants[READ_BYTE()])
#define BINARY_OP(value_type, op)                                                   \
    do {                                                                            \
        if (!IS_NUMBER(cw_peek_stack(vm, 0)) || !IS_NUMBER(cw_peek_stack(vm, 1)))   \
        {                                                                           \
            cw_runtime_error(vm, "Operands must be numbers.");                      \
            return INTERPRET_RUNTIME_ERROR;                                         \
        }                                                                           \
        double b = AS_NUMBER(cw_pop_stack(vm));                                     \
        double a = AS_NUMBER(cw_pop_stack(vm));                                     \
        cw_push_stack(vm, value_type(a op b));                                      \
    } while (false)

    while (true)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (Value* slot = vm->stack; slot < vm->stack + vm->stack_index; ++slot)
        {
            printf("[ ");
            cw_print_value(*slot);
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
            case OP_NULL:     cw_push_stack(vm, MAKE_NULL()); break;
            case OP_TRUE:     cw_push_stack(vm, MAKE_BOOL(true)); break;
            case OP_FALSE:    cw_push_stack(vm, MAKE_BOOL(false)); break;
            case OP_EQ: case OP_NOTEQ:
            {
                Value b = cw_pop_stack(vm);
                Value a = cw_pop_stack(vm);
                bool eq = cw_values_equal(a, b);
                cw_push_stack(vm, MAKE_BOOL((instruction == OP_EQ ? eq : !eq)));
                break;
            }
            case OP_LT:       BINARY_OP(MAKE_BOOL, <); break;
            case OP_GT:       BINARY_OP(MAKE_BOOL, >); break;
            case OP_LTEQ:     BINARY_OP(MAKE_BOOL, <=); break;
            case OP_GTEQ:     BINARY_OP(MAKE_BOOL, >=); break;
            case OP_ADD:
            {
                if (IS_STRING(cw_peek_stack(vm, 0)) && IS_STRING(cw_peek_stack(vm, 1)))
                {
                    cwString* b = AS_STRING(cw_pop_stack(vm));
                    cwString* a = AS_STRING(cw_pop_stack(vm));
                    cw_push_stack(vm, MAKE_OBJECT(cw_str_concat(vm, a, b)));
                }
                else if (IS_NUMBER(cw_peek_stack(vm, 0)) && IS_NUMBER(cw_peek_stack(vm, 1)))
                {
                    double b = AS_NUMBER(cw_pop_stack(vm));
                    double a = AS_NUMBER(cw_pop_stack(vm));
                    cw_push_stack(vm, MAKE_NUMBER(a + b));
                }
                else
                {
                    cw_runtime_error(vm, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(MAKE_NUMBER, -); break;
            case OP_MULTIPLY: BINARY_OP(MAKE_NUMBER, *); break;
            case OP_DIVIDE:   BINARY_OP(MAKE_NUMBER, /); break;
            case OP_NOT:      cw_push_stack(vm, MAKE_BOOL(cw_is_falsey(cw_pop_stack(vm)))); break;
            case OP_NEGATE:   
                if (!IS_NUMBER(cw_peek_stack(vm, 0)))
                {
                    cw_runtime_error(vm, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_push_stack(vm, MAKE_NUMBER(-AS_NUMBER(cw_pop_stack(vm))));
                break;
            case OP_RETURN:
            {
                cw_print_value(cw_pop_stack(vm));
                printf("\n");
                return INTERPRET_OK;
            }
        }
    }

#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
}

InterpretResult cw_interpret(VM* vm, const char* src)
{
    Chunk chunk;
    cw_chunk_init(&chunk);

    InterpretResult result = INTERPRET_COMPILE_ERROR;
    if (cw_compile(vm, src, &chunk))
    {
        vm->chunk = &chunk;
        vm->ip = vm->chunk->bytes;

        result = cw_run(vm);
    }

    cw_chunk_free(&chunk);
    return result;
}
