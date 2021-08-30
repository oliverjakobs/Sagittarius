#include "debug.h"

#include <stdio.h>
#include <stdarg.h>

#include "runtime.h"

void cw_disassemble_chunk(const cwChunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    int offset = 0;
    while (offset < chunk->len)
    {
        offset = cw_disassemble_instruction(chunk, offset);
    }
}

static int cw_disassemble_simple(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int cw_disassemble_constant(const char* name, const cwChunk* chunk, int offset)
{
    uint8_t constant = chunk->bytes[offset + 1];
    printf("%-16s %4d '", name, constant);
    cw_print_value(chunk->constants[constant]);
    printf("'\n");
    return offset + 2;
}

static int cw_disassemble_byte(const char* name, const cwChunk* chunk, int offset)
{
    uint8_t slot = chunk->bytes[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2; 
}

int  cw_disassemble_instruction(const cwChunk* chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->bytes[offset];
    switch (instruction)
    {
    case OP_CONSTANT:   return cw_disassemble_constant("OP_CONSTANT", chunk, offset);
    case OP_NULL:       return cw_disassemble_simple("OP_NULL", offset);
    case OP_TRUE:       return cw_disassemble_simple("OP_TRUE", offset);
    case OP_FALSE:      return cw_disassemble_simple("OP_FALSE", offset);
    case OP_POP:        return cw_disassemble_simple("OP_POP", offset);
    case OP_SET_LOCAL:  return cw_disassemble_byte("OP_SET_LOCAL", chunk, offset);
    case OP_GET_LOCAL:  return cw_disassemble_byte("OP_GET_LOCAL", chunk, offset);
    case OP_DEF_GLOBAL: return cw_disassemble_constant("OP_DEF_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL: return cw_disassemble_constant("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL: return cw_disassemble_constant("OP_GET_GLOBAL", chunk, offset);
    case OP_EQ:         return cw_disassemble_simple("OP_EQ", offset);
    case OP_NOTEQ:      return cw_disassemble_simple("OP_NOTEQ", offset);
    case OP_LT:         return cw_disassemble_simple("OP_LT", offset);
    case OP_GT:         return cw_disassemble_simple("OP_GT", offset);
    case OP_LTEQ:       return cw_disassemble_simple("OP_LTEQ", offset);
    case OP_GTEQ:       return cw_disassemble_simple("OP_GTEQ", offset);
    case OP_ADD:        return cw_disassemble_simple("OP_ADD", offset);
    case OP_SUBTRACT:   return cw_disassemble_simple("OP_SUBTRACT", offset);
    case OP_MULTIPLY:   return cw_disassemble_simple("OP_MULTIPLY", offset);
    case OP_DIVIDE:     return cw_disassemble_simple("OP_DIVIDE", offset);
    case OP_NOT:        return cw_disassemble_simple("OP_NOT", offset);
    case OP_NEGATE:     return cw_disassemble_simple("OP_NEGATE", offset);
    case OP_PRINT:      return cw_disassemble_simple("OP_PRINT", offset);
    case OP_RETURN:     return cw_disassemble_simple("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void cw_print_value(cwValue val)
{
    switch (val.type)
    {
    case VAL_NULL:    printf("null"); break;
    case VAL_BOOL:    printf(AS_BOOL(val) ? "true" : "false"); break;
    case VAL_INT:     printf("%d", AS_INT(val)); break;
    case VAL_FLOAT:   printf("%g", AS_FLOAT(val)); break;
    case VAL_OBJECT:  cw_print_object(val); break;
    }
}

void cw_print_object(cwValue val)
{
    switch (OBJECT_TYPE(val))
    {
    case OBJ_STRING: printf("%s", AS_RAWSTRING(val)); break;
    }
}

void cw_runtime_error(cwRuntime* cw, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = cw->ip - cw->chunk->bytes - 1;
    int line = cw->chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    cw_reset_stack(cw);
}


void cw_syntax_error(cwRuntime* cw, int line, const char* fmt, ...)
{
    if (cw->panic) return;
    cw->panic = true;

    fprintf(stderr, "[line %d] Syntax error: ", line);

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);

    cw->error = true;
}

void cw_syntax_error_at(cwRuntime* cw, cwToken* token, const char* msg)
{
    if (cw->panic) return;
    cw->panic = true;

    fprintf(stderr, "[line %d] Syntax error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else
        fprintf(stderr, " at '%.*s'", token->end - token->start, token->start);

    fprintf(stderr, ": %s\n", msg);
    cw->error = true;
}

