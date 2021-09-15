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
#define OP_BINARY_INT(op) {                         \
        cwValue b = cw_pop_stack(cw);               \
        cwValue* a = cw_peek_stack(cw, 0);          \
        a->ival = a->ival op b.ival;                \
        a->type = CW_VALUE_INT;                     \
    } break
#define OP_BINARY_FLOAT(op) {                       \
        cwValue b = cw_pop_stack(cw);               \
        cwValue* a = cw_peek_stack(cw, 0);          \
        a->fval = cw_valtof(*a) op cw_valtof(b);    \
        a->type = CW_VALUE_FLOAT;                   \
    } break
#define OP_COMPARE_INT(op) {                        \
        cwValue b = cw_pop_stack(cw);               \
        cwValue* a = cw_peek_stack(cw, 0);          \
        a->ival = (a->ival - b.ival) op 0;          \
        a->type = CW_VALUE_BOOL;                    \
    } break
#define OP_COMPARE_FLOAT(op) {                          \
        cwValue b = cw_pop_stack(cw);                   \
        cwValue* a = cw_peek_stack(cw, 0);              \
        a->ival = (cw_valtof(*a) - cw_valtof(b)) op 0;  \
        a->type = CW_VALUE_BOOL;                        \
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
            case OP_PUSH:   cw_push_stack(cw, READ_CONSTANT()); break;
            case OP_POP:    cw_pop_stack(cw); break;
            case OP_ADD_I:  OP_BINARY_INT(+);   case OP_ADD_F:  OP_BINARY_FLOAT(+);
            case OP_SUB_I:  OP_BINARY_INT(-);   case OP_SUB_F:  OP_BINARY_FLOAT(-);
            case OP_MUL_I:  OP_BINARY_INT(*);   case OP_MUL_F:  OP_BINARY_FLOAT(*);
            case OP_DIV_I:  OP_BINARY_INT(/);   case OP_DIV_F:  OP_BINARY_FLOAT(/);
            case OP_NEG_I:
            {
                cwValue* val = cw_peek_stack(cw, 0);
                val->ival = -val->ival;
                break;
            }
            case OP_NEG_F:
            {
                cwValue* val = cw_peek_stack(cw, 0);
                val->fval = -val->fval;
                break;
            }
            case OP_LT_I:   OP_COMPARE_INT(<);  case OP_LT_F:   OP_COMPARE_FLOAT(<);
            case OP_LTEQ_I: OP_COMPARE_INT(<=); case OP_LTEQ_F: OP_COMPARE_FLOAT(<=);
            case OP_GT_I:   OP_COMPARE_INT(>);  case OP_GT_F:   OP_COMPARE_FLOAT(>);
            case OP_GTEQ_I: OP_COMPARE_INT(>=); case OP_GTEQ_F: OP_COMPARE_FLOAT(>=);
            case OP_NOT:
            {
                cwValue* val = cw_peek_stack(cw, 0);
                val->ival = cw_value_is_falsey(val);
                val->type = CW_VALUE_BOOL;
                break;
            }
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
#undef OP_BINARY_INT
#undef OP_BINARY_FLOAT
#undef OP_COMPARE_INT
#undef OP_COMPARE_FLOAT
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