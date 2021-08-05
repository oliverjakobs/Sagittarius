#include "vm.h"
#include "debug.h"

int main(int argc, const char* argv[])
{
    VM vm;
    cw_vm_init(&vm);

    Chunk chunk;
    cw_chunk_init(&chunk);
    
    int constant = cw_chunk_add_constant(&chunk, 1.2);
    cw_chunk_write(&chunk, OP_CONSTANT, 123);
    cw_chunk_write(&chunk, constant, 123);

    constant = cw_chunk_add_constant(&chunk, 3.4);
    cw_chunk_write(&chunk, OP_CONSTANT, 123);
    cw_chunk_write(&chunk, constant, 123);

    cw_chunk_write(&chunk, OP_ADD, 123);

    constant = cw_chunk_add_constant(&chunk, 5.6);
    cw_chunk_write(&chunk, OP_CONSTANT, 123);
    cw_chunk_write(&chunk, constant, 123);

    cw_chunk_write(&chunk, OP_DIVIDE, 123);

    cw_chunk_write(&chunk, OP_NEGATE, 123);
    cw_chunk_write(&chunk, OP_RETURN, 123);
    // cw_disassemble_chunk(&chunk, "test chunk");

    cw_interpret(&vm, &chunk);

    cw_chunk_free(&chunk);    
    cw_vm_free(&vm);

    return 0;
}