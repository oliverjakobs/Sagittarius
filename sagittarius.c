
// -----------------------------------------------------------------------------
// ----| Common |---------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_TRACE_EXECUTION

// -----------------------------------------------------------------------------
// ----| Memory |---------------------------------------------------------------
// -----------------------------------------------------------------------------

// calculates a new capacity based on a given current capacity. It scales based 
// on the old size and grows by a factor of two. If the current capacity is zero
// we jump straight to eight elements instead of starting at one.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// wrapper for reallocate
#define GROW_ARRAY(previous, type, oldCount, count) (type*)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

// the single function for all dynamic memory management
// The two size arguments passed to reallocate() control which operation to perform:
// oldSize      newSize                 Operation
// 0            Non‑zero                Allocate new block.
// Non‑zero     0                       Free allocation.
// Non‑zero     Smaller than oldSize    Shrink existing allocation.
// Non‑zero 	Larger than oldSize     Grow existing allocation.
void* reallocate(void* previous, size_t oldSize, size_t newSize)
{
    if (newSize == 0) 
    {
        free(previous);
        return NULL;
    }

    return realloc(previous, newSize);
}

// -----------------------------------------------------------------------------
// ----| Value |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef double value_t;

typedef struct
{
    int capacity;
    int count;
    value_t* values;
} value_array;

void init_value_array(value_array* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void free_value_array(value_array* array)
{
    FREE_ARRAY(value_t, array->values, array->capacity);
    init_value_array(array);
}

void write_value_array(value_array* array, value_t v)
{
    if (array->capacity < array->count + 1)
    {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(array->values, value_t, old_capacity, array->capacity);
    }
    array->values[array->count] = v;
    array->count++;
}

void print_value(value_t v)
{
    printf("%g", v);
}

// -----------------------------------------------------------------------------
// ----| Chunk |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} op_code;

typedef struct
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    value_array constants;
} chunk_t;

void init_chunk(chunk_t* c)
{
    c->count = 0;
    c->capacity = 0;
    c->code = NULL;
    c->lines = NULL;
    init_value_array(&c->constants);
}

void free_chunk(chunk_t* c)
{
    FREE_ARRAY(uint8_t, c->code, c->capacity);
    FREE_ARRAY(int, c->lines, c->capacity);
    free_value_array(&c->constants);
    init_chunk(c);
}

void write_chunk(chunk_t* c, uint8_t byte, int line)
{
    if (c->capacity < c->count + 1)
    {
        int old_capacity = c->capacity;
        c->capacity = GROW_CAPACITY(old_capacity);
        c->code = GROW_ARRAY(c->code, uint8_t, old_capacity, c->capacity);
        c->lines = GROW_ARRAY(c->lines, int, old_capacity, c->capacity);
    }

    c->code[c->count] = byte;
    c->lines[c->count] = line;
    c->count++;
}

int add_constant(chunk_t* c, value_t v)
{
    write_value_array(&c->constants, v);
    return c->constants.count - 1;
}

// -----------------------------------------------------------------------------
// ----| Debug |----------------------------------------------------------------
// -----------------------------------------------------------------------------
static int simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int constant_instruction(const char* name, chunk_t* c, int offset)
{
    uint8_t constant = c->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(c->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

int disassemble_instruction(chunk_t* c, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && c->lines[offset] == c->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", c->lines[offset]);
    } 

    uint8_t instruction = c->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:
        return constant_instruction("OP_CONSTANT", c, offset);
    case OP_ADD:
        return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simple_instruction("OP_DIVIDE", offset);
    case OP_NEGATE:
        return simple_instruction("OP_NEGATE", offset);
    case OP_RETURN:
        return simple_instruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void disassemble_chunk(chunk_t* c, const char* name) 
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < c->count;) 
    {
        offset = disassemble_instruction(c, offset);
    }
}

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

#define STACK_MAX 256

typedef struct
{
    chunk_t* chunk;
    uint8_t* ip;
    value_t stack[STACK_MAX];
    value_t* stack_top;
} VM;

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} interpret_result; 

static void reset_stack(VM* vm)
{
    vm->stack_top = vm->stack;
}

void init_VM(VM* vm)
{
    reset_stack(vm);
}

void free_VM(VM* vm)
{

}

void push(VM* vm, value_t v)
{
    *vm->stack_top = v;
    vm->stack_top++;
}

value_t pop(VM* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}

static interpret_result run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])

#define BINARY_OP(vm, op) do { value_t b = pop(vm); value_t a = pop(vm); push(vm, a op b); } while (false)

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("          ");
        for (value_t* slot = vm->stack; slot < vm->stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm->chunk, (int)(vm->ip - vm->chunk->code));
#endif  

        uint8_t instruction;
        switch (instruction = READ_BYTE()) 
        {
        case OP_CONSTANT:
        {
            value_t constant = READ_CONSTANT();
            push(vm, constant);
            break;
        }
        case OP_ADD:        BINARY_OP(vm, +); break;
        case OP_SUBTRACT:   BINARY_OP(vm, -); break;
        case OP_MULTIPLY:   BINARY_OP(vm, *); break;
        case OP_DIVIDE:     BINARY_OP(vm, /); break;
        case OP_NEGATE:     push(vm, -pop(vm)); break;
        case OP_RETURN:
        {
            print_value(pop(vm));
            printf("\n");
            return INTERPRET_OK;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT

#undef BINARY_OP
}

interpret_result interpret(VM* vm, chunk_t* c)
{
    vm->chunk = c;
    vm->ip = vm->chunk->code;

    return run(vm);
}


// -----------------------------------------------------------------------------
// ----| main |-----------------------------------------------------------------
// -----------------------------------------------------------------------------

int main(int argc, const char* argv[]) 
{
    VM vm;
    init_VM(&vm);

    chunk_t chunk;
    init_chunk(&chunk);

    int constant = add_constant(&chunk, 1.2);
    write_chunk(&chunk, OP_CONSTANT, 123);
    write_chunk(&chunk, constant, 123);

    constant = add_constant(&chunk, 3.4);    
    write_chunk(&chunk, OP_CONSTANT, 123);   
    write_chunk(&chunk, constant, 123);

    write_chunk(&chunk, OP_ADD, 123);        

    constant = add_constant(&chunk, 5.6);    
    write_chunk(&chunk, OP_CONSTANT, 123);   
    write_chunk(&chunk, constant, 123);      

    write_chunk(&chunk, OP_DIVIDE, 123);     

    write_chunk(&chunk, OP_NEGATE, 123);  

    write_chunk(&chunk, OP_RETURN, 123);
    disassemble_chunk(&chunk, "test chunk");

    interpret(&vm, &chunk);

    free_VM(&vm);
    free_chunk(&chunk);
    return 0;
}