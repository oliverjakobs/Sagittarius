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

static int cw_disassemble_int32(const char* name, const cwChunk* chunk, int offset)
{
    uint8_t b1 = chunk->bytes[offset + 1];
    uint8_t b2 = chunk->bytes[offset + 2];
    uint8_t b3 = chunk->bytes[offset + 3];
    uint8_t b4 = chunk->bytes[offset + 4];
    printf("%-16s %4x %x %x %x  ", name, b1, b2, b3, b4);

    printf("'%d'\n", (int32_t)((b1 << 24) | (b2 << 16) | (b3 << 8) | b4));
    return offset + 5;
}

static int cw_disassemble_float(const char* name, const cwChunk* chunk, int offset)
{
    uint8_t b1 = chunk->bytes[offset + 1];
    uint8_t b2 = chunk->bytes[offset + 2];
    uint8_t b3 = chunk->bytes[offset + 3];
    uint8_t b4 = chunk->bytes[offset + 4];
    printf("%-16s %4x %x %x %x  ", name, b1, b2, b3, b4);
    
    uint32_t ival = (uint32_t)((b1 << 24) | (b2 << 16) | (b3 << 8) | b4);
    printf("'%f'\n", *((float*)&ival));
    return offset + 5;
}

static int cw_disassemble_byte(const char* name, const cwChunk* chunk, int offset)
{
    uint8_t slot = chunk->bytes[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2; 
}

static int cw_disassemble_jump(const char* name, int sign, const cwChunk* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->bytes[offset + 1] << 8) | chunk->bytes[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
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
    case OP_PUSH_I:         return cw_disassemble_int32("OP_PUSH_I", chunk, offset);
    case OP_PUSH_F:         return cw_disassemble_float("OP_PUSH_F", chunk, offset);
    case OP_POP:            return cw_disassemble_simple("OP_POP", offset);
    case OP_ADD_I:          return cw_disassemble_simple("OP_ADD_I", offset);
    case OP_SUB_I:          return cw_disassemble_simple("OP_SUB_I", offset);
    case OP_MUL_I:          return cw_disassemble_simple("OP_MUL_I", offset);
    case OP_DIV_I:          return cw_disassemble_simple("OP_DIV_I", offset);
    case OP_ADD_F:          return cw_disassemble_simple("OP_ADD_F", offset);
    case OP_SUB_F:          return cw_disassemble_simple("OP_SUB_F", offset);
    case OP_MUL_F:          return cw_disassemble_simple("OP_MUL_F", offset);
    case OP_DIV_F:          return cw_disassemble_simple("OP_DIV_F", offset);
    case OP_NEG_I:          return cw_disassemble_simple("OP_NEG_I", offset);
    case OP_NEG_F:          return cw_disassemble_simple("OP_NEG_F", offset);
    case OP_NOT:            return cw_disassemble_simple("OP_NOT", offset);
    case OP_EQ:             return cw_disassemble_simple("OP_EQ", offset);
    case OP_NOTEQ:          return cw_disassemble_simple("OP_NOTEQ", offset);
    case OP_LT_I:           return cw_disassemble_simple("OP_LT_I", offset);
    case OP_LTEQ_I:         return cw_disassemble_simple("OP_LTEQ_I", offset);
    case OP_GT_I:           return cw_disassemble_simple("OP_GT_I", offset);
    case OP_GTEQ_I:         return cw_disassemble_simple("OP_GTEQ_I", offset);
    case OP_LT_F:           return cw_disassemble_simple("OP_LT_F", offset);
    case OP_LTEQ_F:         return cw_disassemble_simple("OP_LTEQ_F", offset);
    case OP_GT_F:           return cw_disassemble_simple("OP_GT_F", offset);
    case OP_GTEQ_F:         return cw_disassemble_simple("OP_GTEQ_F", offset);
    case OP_JUMP_IF_FALSE:  return cw_disassemble_jump("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_JUMP:           return cw_disassemble_jump("OP_JUMP", 1, chunk, offset);
    case OP_LOOP:           return cw_disassemble_jump("OP_LOOP", -1, chunk, offset);
    case OP_RETURN:         return cw_disassemble_simple("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void cw_print_value(cwValue val)
{
    switch (val.type)
    {
    case CW_VALUE_NULL:     printf("null"); break;
    case CW_VALUE_BOOL:     printf(val.ival ? "true" : "false"); break;
    case CW_VALUE_INT:      printf("%d", val.ival); break;
    case CW_VALUE_FLOAT:    printf("%g", val.fval); break;
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

void cw_syntax_error(int line, const char* fmt, ...)
{
    fprintf(stderr, "[line %d] Syntax error: ", line);

    va_list args;
    va_start(args, fmt);
    fprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
    exit(0);
}

void cw_syntax_error_at(cwToken* token, const char* msg)
{
    fprintf(stderr, "[line %d] Syntax error", token->line);

    if (token->type == TOKEN_EOF)
        fprintf(stderr, " at end");
    else
        fprintf(stderr, " at '%.*s'", token->end - token->start, token->start);

    fprintf(stderr, ": %s\n", msg);
    exit(0);
}

