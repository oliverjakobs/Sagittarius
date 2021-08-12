#include "debug.h"

#include <stdio.h>

void cw_disassemble_chunk(const Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    int offset = 0;
    while (offset < chunk->len)
    {
        offset = cw_disassemble_instruction(chunk, offset);
    }
}

static int cw_disassemble_constant(const char* name, const Chunk* chunk, int offset)
{
    uint8_t constant = chunk->bytes[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants[constant]);
    printf("'\n");
    return offset + 2;
}

static int cw_disassemble_simple(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int  cw_disassemble_instruction(const Chunk* chunk, int offset)
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
    case OP_RETURN:     return cw_disassemble_simple("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void print_value(Value val)
{
    switch (val.type)
    {
    case VAL_NULL:    printf("null"); break;
    case VAL_BOOL:    printf(AS_BOOL(val) ? "true" : "false"); break;
    case VAL_NUMBER:  printf("%g", AS_NUMBER(val)); break;
    case VAL_OBJECT:  print_object(val); break;
    }
}

void print_object(Value val)
{
    switch (OBJECT_TYPE(val))
    {
    case OBJ_STRING: printf("%s", AS_RAWSTRING(val)); break;
  }
}