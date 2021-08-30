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
    cw->objects = NULL;
    cw_table_init(&cw->globals);
    cw_table_init(&cw->strings);
    cw_reset_stack(cw);
}

void cw_free(cwRuntime* cw)
{
    cw_table_free(&cw->strings);
    cw_table_free(&cw->globals);
    cw_free_objects(cw);
}

static InterpretResult cw_run(cwRuntime* cw)
{
#define READ_BYTE()     (*cw->ip++)
#define READ_CONSTANT() (cw->chunk->constants[READ_BYTE()])
#define BINARY_OP_NUM(op) {                                                                         \
        if (!IS_NUMBER(cw_peek_stack(cw, 0)) || !IS_NUMBER(cw_peek_stack(cw, 1)))                   \
        {                                                                                           \
            cw_runtime_error(cw, "Operands must be numbers.");                                      \
            return INTERPRET_RUNTIME_ERROR;                                                         \
        }                                                                                           \
        cwValue b = cw_pop_stack(cw);                                                               \
        cwValue a = cw_pop_stack(cw);                                                               \
        if (IS_FLOAT(a) || IS_FLOAT(b)) cw_push_stack(cw, MAKE_FLOAT(AS_FLOAT(a) op AS_FLOAT(b)));  \
        else                            cw_push_stack(cw, MAKE_INT(AS_INT(a) op AS_INT(b)));        \
    } break
#define BINARY_OP_BOOL(op) {                                                                        \
        if (!IS_NUMBER(cw_peek_stack(cw, 0)) || !IS_NUMBER(cw_peek_stack(cw, 1)))                   \
        {                                                                                           \
            cw_runtime_error(cw, "Operands must be numbers.");                                      \
            return INTERPRET_RUNTIME_ERROR;                                                         \
        }                                                                                           \
        cwValue b = cw_pop_stack(cw);                                                               \
        cwValue a = cw_pop_stack(cw);                                                               \
        if (IS_FLOAT(a) || IS_FLOAT(b)) cw_push_stack(cw, MAKE_BOOL(AS_FLOAT(a) op AS_FLOAT(b)));   \
        else                            cw_push_stack(cw, MAKE_BOOL(AS_INT(a) op AS_INT(b)));       \
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
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                cwValue constant = READ_CONSTANT();
                cw_push_stack(cw, constant);
                break;
            }
            case OP_NULL:     cw_push_stack(cw, MAKE_NULL()); break;
            case OP_TRUE:     cw_push_stack(cw, MAKE_BOOL(true)); break;
            case OP_FALSE:    cw_push_stack(cw, MAKE_BOOL(false)); break;
            case OP_POP:      cw_pop_stack(cw); break;
            case OP_GET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                cw_push_stack(cw, cw->stack[slot]);
                break;
            }
            case OP_SET_LOCAL:
            {
                uint8_t slot = READ_BYTE();
                cw->stack[slot] = cw_peek_stack(cw, 0);
                break;
            }
            case OP_DEF_GLOBAL:
            {
                cwString* name = AS_STRING(READ_CONSTANT());
                cw_table_insert(&cw->globals, name, cw_peek_stack(cw, 0));
                cw_pop_stack(cw);
                break;
            }
            case OP_SET_GLOBAL:
            {
                cwString* name = AS_STRING(READ_CONSTANT());
                if (cw_table_insert(&cw->globals, name, cw_peek_stack(cw, 0)))
                {
                    cw_table_remove(&cw->globals, name); 
                    cw_runtime_error(cw, "Undefined variable '%s'.", name->raw);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_GLOBAL:
            {
                cwString* name = AS_STRING(READ_CONSTANT());
                cwValue* value = cw_table_find(&cw->globals, name);
                if (!value)
                {
                    cw_runtime_error(cw, "Undefined variable '%s'.", name->raw);
                    return INTERPRET_RUNTIME_ERROR;
                }
                cw_push_stack(cw, *value);
                break;
            }
            case OP_EQ: case OP_NOTEQ:
            {
                cwValue b = cw_pop_stack(cw);
                cwValue a = cw_pop_stack(cw);
                bool eq = cw_values_equal(a, b);
                cw_push_stack(cw, MAKE_BOOL((instruction == OP_EQ ? eq : !eq)));
                break;
            }
            case OP_LT:   BINARY_OP_BOOL(<);
            case OP_GT:   BINARY_OP_BOOL(>);
            case OP_LTEQ: BINARY_OP_BOOL(<=);
            case OP_GTEQ: BINARY_OP_BOOL(>=);
            case OP_ADD:
            {
                if (IS_STRING(cw_peek_stack(cw, 0)) && IS_STRING(cw_peek_stack(cw, 1)))
                {
                    cwString* b = AS_STRING(cw_pop_stack(cw));
                    cwString* a = AS_STRING(cw_pop_stack(cw));
                    cw_push_stack(cw, MAKE_OBJECT(cw_str_concat(cw, a, b)));
                }
                else if (IS_NUMBER(cw_peek_stack(cw, 0)) && IS_NUMBER(cw_peek_stack(cw, 1)))
                {
                    cwValue b = cw_pop_stack(cw);
                    cwValue a = cw_pop_stack(cw);
                    if (IS_FLOAT(a) || IS_FLOAT(b)) cw_push_stack(cw, MAKE_FLOAT(AS_FLOAT(a) + AS_FLOAT(b)));
                    else                            cw_push_stack(cw, MAKE_INT(AS_INT(a) + AS_INT(b)));
                }
                else
                {
                    cw_runtime_error(cw, "Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP_NUM(-);
            case OP_MULTIPLY: BINARY_OP_NUM(*);
            case OP_DIVIDE:   BINARY_OP_NUM(/);
            case OP_NOT:      cw_push_stack(cw, MAKE_BOOL(cw_is_falsey(cw_pop_stack(cw)))); break;
            case OP_NEGATE:
            {
                if (!IS_NUMBER(cw_peek_stack(cw, 0)))
                {
                    cw_runtime_error(cw, "Operand must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                
                cwValue val = cw_pop_stack(cw);
                if (IS_FLOAT(val)) cw_push_stack(cw, MAKE_FLOAT(-AS_FLOAT(val)));
                else               cw_push_stack(cw, MAKE_INT(-AS_INT(val)));
                break;
            }
            case OP_PRINT:
                cw_print_value(cw_pop_stack(cw));
                printf("\n");
                break;
            case OP_RETURN:
                return INTERPRET_OK;
        }
    }

#undef BINARY_OP_NUM
#undef BINARY_OP_BOOL
#undef READ_CONSTANT
#undef READ_BYTE
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

cwValue cw_pop_stack(cwRuntime* cw)         { return cw->stack[--cw->stack_index]; }
void    cw_reset_stack(cwRuntime* cw)       { cw->stack_index = 0; }
cwValue cw_peek_stack(cwRuntime* cw, int d) { return cw->stack[cw->stack_index - 1 - d]; }