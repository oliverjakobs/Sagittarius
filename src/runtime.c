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
#define READ_SHORT()    (cw->ip += 2, (uint16_t)((cw->ip[-2] << 8) | cw->ip[-1]))
#define READ_CONSTANT() (cw->chunk->constants[READ_BYTE()])
#define OP_BINARY(op, return_type)                                  \
    op(cw_peek_stack(cw, 1), cw_peek_stack(cw, 0), return_type);    \
    cw_pop_stack(cw);                                               \
    break
#define OP_COMPARISON(op) {                                         \
        cwValue b = cw_pop_stack(cw);                               \
        cwValue a = cw_pop_stack(cw);                               \
        cw_push_stack(cw, CW_MAKE_BOOL(cw_value_cmp(a, b) op 0));   \
    } break

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
            case OP_ADD_I:  OP_BINARY(cw_value_add, CW_VALUE_INT);
            case OP_SUB_I:  OP_BINARY(cw_value_sub, CW_VALUE_INT);
            case OP_MUL_I:  OP_BINARY(cw_value_mul, CW_VALUE_INT);
            case OP_DIV_I:  OP_BINARY(cw_value_div, CW_VALUE_INT);
            case OP_ADD_F:  OP_BINARY(cw_value_add, CW_VALUE_FLOAT);
            case OP_SUB_F:  OP_BINARY(cw_value_sub, CW_VALUE_FLOAT);
            case OP_MUL_F:  OP_BINARY(cw_value_mul, CW_VALUE_FLOAT);
            case OP_DIV_F:  OP_BINARY(cw_value_div, CW_VALUE_FLOAT);
            case OP_NEG:    cw_value_neg(cw_peek_stack(cw, 0)); break;
            case OP_LT:     OP_COMPARISON(<);
            case OP_LTEQ:   OP_COMPARISON(<=);
            case OP_GT:     OP_COMPARISON(>);
            case OP_GTEQ:   OP_COMPARISON(>=);
            // case OP_NOT:    cw_push_stack(cw, CW_MAKE_BOOL(cw_is_falsey(cw_pop_stack(cw)))); break;
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                if (cw_value_is_falsey(cw_peek_stack(cw, 0))) cw->ip += offset;
                break;
            }
            /* NOTE: combine OP_JUMP and OP_LOOP */
            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                cw->ip += offset;
                break;
            }
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                cw->ip -= offset;
                break;
            }
            case OP_RETURN:
                cw_print_value(cw_pop_stack(cw));
                printf("\n");
                return INTERPRET_OK;
            default:
                cw_runtime_error(cw, "Unknown instruction %d", instruction);
                return INTERPRET_RUNTIME_ERROR;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef OP_BINARY
#undef OP_COMPARISON
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