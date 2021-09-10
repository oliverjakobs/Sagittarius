#include "runtime.h"

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"
#include "memory.h"
#include "compiler.h"

void cw_init(cwRuntime* cw)
{
    cw->chunk = NULL;
    cw->ip = NULL;
    cw_reset_stack(cw);
}

void cw_free(cwRuntime* cw)
{

}

static InterpretResult cw_run(cwRuntime* cw)
{
#define READ_BYTE()     (*cw->ip++)
#define READ_CONSTANT() (cw->chunk->constants[READ_BYTE()])

    while (true)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (cwValue* slot = cw->stack; slot < cw->stack + cw->stack_index; ++slot)
        {
            printf("[ ");
            cw_print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        cw_disassemble_instruction(cw->chunk, (int)(cw->ip - cw->chunk->bytes));
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction)
        {
            case OP_PUSH:
            {
                cwValue constant = READ_CONSTANT();
                cw_push_stack(cw, constant);
                break;
            }
            case OP_POP:    cw_pop_stack(cw); break;
            case OP_ADD:
            {
                if (!cw_value_add(cw_peek_stack(cw, 1), cw_peek_stack(cw, 0)))
                {
                    cw_runtime_error(cw, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_pop_stack(cw);
                break;
            }
            case OP_SUB:
            {
                if (!cw_value_sub(cw_peek_stack(cw, 1), cw_peek_stack(cw, 0)))
                {
                    cw_runtime_error(cw, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_pop_stack(cw);
                break;
            }
            case OP_MUL:
            {
                if (!cw_value_mul(cw_peek_stack(cw, 1), cw_peek_stack(cw, 0)))
                {
                    cw_runtime_error(cw, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_pop_stack(cw);
                break;
            }
            case OP_DIV:
            {
                if (!cw_value_div(cw_peek_stack(cw, 1), cw_peek_stack(cw, 0)))
                {
                    cw_runtime_error(cw, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_pop_stack(cw);
                break;
            }
            case OP_RETURN:
                cw_print_value(cw_pop_stack(cw));
                printf("\n");
                return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult cw_interpret(cwRuntime* cw, const char* src)
{
    cwChunk chunk;
    cw_chunk_init(&chunk);

    InterpretResult result = INTERPRET_COMPILE_ERROR;
    if (cw_compile(cw, src, &chunk))
    {
        cw->chunk = &chunk;
        cw->ip = cw->chunk->bytes;

        result = cw_run(cw);
    }

    cw_chunk_free(&chunk);
    return result;
}

/* stack operations */
void  cw_push_stack(cwRuntime* cw, cwValue val)
{
    if (cw->stack_index >= CW_STACK_MAX)
    {
        cw_runtime_error(cw, "Stack overflow");
        return;
    }

    cw->stack[cw->stack_index++] = val;
}

cwValue  cw_pop_stack(cwRuntime* cw)         { return cw->stack[--cw->stack_index]; }
void     cw_reset_stack(cwRuntime* cw)       { cw->stack_index = 0; }
cwValue* cw_peek_stack(cwRuntime* cw, int d) { return &cw->stack[cw->stack_index - 1 - d]; }