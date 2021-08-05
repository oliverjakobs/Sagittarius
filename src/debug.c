#include "debug.h"

#include <stdio.h>

void cw_disassemble_chunk(const Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);
    int offset = 0;
    while (offset < tb_array_len(chunk->bytes))
    {
        offset = cw_disassemble_instruction(chunk, offset);
    }
}

static int cw_disassemble_constant(const char* name, const Chunk* chunk, int offset)
{
    uint8_t constant = chunk->bytes[offset + 1].data;
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants[constant]);
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
    if (offset > 0 && chunk->bytes[offset].line == chunk->bytes[offset - 1].line)
        printf("   | ");
    else
        printf("%4d ", chunk->bytes[offset].line);

    uint8_t instruction = chunk->bytes[offset].data;
    switch (instruction)
    {
        case OP_CONSTANT:   return cw_disassemble_constant("OP_CONSTANT", chunk, offset);
        case OP_ADD:        return cw_disassemble_simple("OP_ADD", offset);
        case OP_SUBTRACT:   return cw_disassemble_simple("OP_SUBTRACT", offset);
        case OP_MULTIPLY:   return cw_disassemble_simple("OP_MULTIPLY", offset);
        case OP_DIVIDE:     return cw_disassemble_simple("OP_DIVIDE", offset);
        case OP_NEGATE:     return cw_disassemble_simple("OP_NEGATE", offset);
        case OP_RETURN:     return cw_disassemble_simple("OP_RETURN", offset);
        default:
            printf("Unknown opcode %d\n", instruction);
            return offset + 1;
    }
}


void printValue(Value val)
{
    printf("%g", val);
}