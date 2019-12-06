#ifndef CW_INCLUDE_CLOCKWORK_H
#define CW_INCLUDE_CLOCKWORK_H

// DOCUMENTATION
//

#ifdef __cplusplus
extern "C"
{
#endif

// -----------------------------------------------------------------------------
// ----| Version |--------------------------------------------------------------
// -----------------------------------------------------------------------------

#define CW_VERSION_MAJOR    0
#define CW_VERSION_MINOR    1

// -----------------------------------------------------------------------------
// ----| Common |---------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdarg.h>

// #define CW_DEBUG_PRINT_CODE
// #define CW_DEBUG_TRACE_EXECUTION

#define CW_UINT8_COUNT      (UINT8_MAX + 1)

// -----------------------------------------------------------------------------
// ----| typedefs |-------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct _cw_struct_vm        cw_virtual_machine_t;
typedef struct _cw_struct_obj       cw_obj_t;

// -----------------------------------------------------------------------------
// ----| Memory |---------------------------------------------------------------
// -----------------------------------------------------------------------------

// calculates a new capacity based on a given current capacity. It scales based 
// on the old size and grows by a factor of two. If the current capacity is zero
// we jump straight to eight elements instead of starting at one.
#define CW_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// wrapper for cw_memory_reallocate
#define CW_ALLOCATE(type, count)    (type*)cw_memory_reallocate(NULL, 0, sizeof(type) * (count))
#define CW_FREE(type, pointer)      cw_memory_reallocate(pointer, sizeof(type), 0)

#define CW_GROW_ARRAY(previous, type, oldCount, count)  (type*)cw_memory_reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define CW_FREE_ARRAY(type, pointer, oldCount)          cw_memory_reallocate(pointer, sizeof(type) * (oldCount), 0)

// The single function for all dynamic memory management
// The two size arguments passed to cw_memory_reallocate() control which 
// operation to perform:
// oldSize      newSize                 Operation
// 0            Non‑zero                Allocate new block.
// Non‑zero     0                       Free allocation.
// Non‑zero     Smaller than oldSize    Shrink existing allocation.
// Non‑zero 	Larger than oldSize     Grow existing allocation.
void* cw_memory_reallocate(void* previous, size_t old_size, size_t new_size);

// -----------------------------------------------------------------------------
// ----| Value |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    CW_VAL_BOOL,
    CW_VAL_NIL,
    CW_VAL_NUMBER,
    CW_VAL_OBJ
} cw_value_type;

typedef struct
{
    cw_value_type type;
    union
    {
        bool boolean;
        double number;
        cw_obj_t* obj;
    } as;
} cw_value_t;

#define CW_IS_BOOL(value)       ((value).type == CW_VAL_BOOL)
#define CW_IS_NIL(value)        ((value).type == CW_VAL_NIL)
#define CW_IS_NUMBER(value)     ((value).type == CW_VAL_NUMBER)
#define CW_IS_OBJ(value)        ((value).type == CW_VAL_OBJ)

#define CW_AS_BOOL(value)       ((value).as.boolean)
#define CW_AS_NUMBER(value)     ((value).as.number)
#define CW_AS_OBJ(value)        ((value).as.obj)

#define CW_BOOL_VAL(value)      ((cw_value_t) { CW_VAL_BOOL,    { .boolean = value }})
#define CW_NIL_VAL              ((cw_value_t) { CW_VAL_NIL,     { .number = 0 }})
#define CW_NUMBER_VAL(value)    ((cw_value_t) { CW_VAL_NUMBER,  { .number = value }})
#define CW_OBJ_VAL(value)       ((cw_value_t) { CW_VAL_OBJ,     { .obj = (cw_obj_t*)value }})

typedef struct
{
    int capacity;
    int count;
    cw_value_t* values;
} cw_value_array_t;

void cw_value_array_init(cw_value_array_t* array);
void cw_value_array_free(cw_value_array_t* array);
void cw_value_array_write(cw_value_array_t* array, cw_value_t value);

bool cw_values_equal(cw_value_t a, cw_value_t b);

void cw_print_value(cw_value_t value);

// -----------------------------------------------------------------------------
// ----| Chunk |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    CW_OP_CONSTANT,
    CW_OP_NIL,
    CW_OP_TRUE,
    CW_OP_FALSE,
    CW_OP_POP,
    CW_OP_GET_LOCAL,
    CW_OP_SET_LOCAL,
    CW_OP_GET_GLOBAL,
    CW_OP_DEFINE_GLOBAL,
    CW_OP_SET_GLOBAL,
    CW_OP_EQUAL,
    CW_OP_GREATER,
    CW_OP_LESS,
    CW_OP_ADD,
    CW_OP_SUBTRACT,
    CW_OP_MULTIPLY,
    CW_OP_DIVIDE,
    CW_OP_NOT,
    CW_OP_NEGATE,
    CW_OP_PRINT,
    CW_OP_JUMP,
    CW_OP_JUMP_IF_FALSE,
    CW_OP_LOOP,
    CW_OP_CALL,
    CW_OP_CLOSURE,
    CW_OP_RETURN,
} cw_op_code;

typedef struct
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    cw_value_array_t constants;
} cw_chunk_t;

void cw_chunk_init(cw_chunk_t* chunk);
void cw_chunk_free(cw_chunk_t* chunk);
void cw_chunk_write(cw_chunk_t* chunk, uint8_t byte, int line);

int cw_chunk_add_constant(cw_chunk_t* chunk, cw_value_t value);

// -----------------------------------------------------------------------------
// ----| Object |---------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    CW_OBJ_CLOSURE,
    CW_OBJ_FUNCTION,
    CW_OBJ_NATIVE,
    CW_OBJ_STRING,
} cw_obj_type;

struct _cw_struct_obj
{
    cw_obj_type type;
    cw_obj_t* next;
};

typedef struct
{
    cw_obj_t obj;
    int length;
    char* chars;
    uint32_t hash;
} cw_obj_string_t;

typedef struct
{
    cw_obj_t obj;
    int arity;  // number of parameters the function expects
    cw_chunk_t chunk;
    cw_obj_string_t* name;
} cw_obj_function_t;

typedef struct
{
    cw_obj_t obj;
    cw_obj_function_t* function;
} cw_obj_closure_t;


typedef cw_value_t (*cw_native_fn)(int arg_count, cw_value_t* args);

typedef struct
{
    cw_obj_t obj;
    cw_native_fn function;
} cw_obj_native_t;


bool cw_obj_is_type(cw_value_t value, cw_obj_type type);

#define CW_OBJ_TYPE(value)      (CW_AS_OBJ(value)->type)
#define CW_IS_CLOSURE(value)    cw_obj_is_type(value, CW_OBJ_CLOSURE)
#define CW_IS_FUNCTION(value)   cw_obj_is_type(value, CW_OBJ_FUNCTION)
#define CW_IS_NATIVE(value)     cw_obj_is_type(value, CW_OBJ_NATIVE)
#define CW_IS_STRING(value)     cw_obj_is_type(value, CW_OBJ_STRING)

#define CW_AS_CLOSURE(value)    ((cw_obj_closure_t*)CW_AS_OBJ(value))
#define CW_AS_FUNCTION(value)   ((cw_obj_function_t*)CW_AS_OBJ(value))
#define CW_AS_NATIVE(value)     (((cw_obj_native_t*)CW_AS_OBJ(value))->function)
#define CW_AS_STRING(value)     ((cw_obj_string_t*)CW_AS_OBJ(value))
#define CW_AS_CSTRING(value)    (((cw_obj_string_t*)CW_AS_OBJ(value))->chars)

void cw_print_obj(cw_value_t value);

cw_obj_string_t* cw_obj_string_move(cw_virtual_machine_t* vm, char* chars, int length);
cw_obj_string_t* cw_obj_string_copy(cw_virtual_machine_t* vm, const char* chars, int length);

cw_obj_function_t* cw_function_new(cw_virtual_machine_t* vm);
cw_obj_native_t* cw_native_new(cw_virtual_machine_t* vm, cw_native_fn function);
cw_obj_closure_t* cw_closure_new(cw_virtual_machine_t* vm, cw_obj_function_t* function);

void cw_objects_free(cw_virtual_machine_t* vm);

// -----------------------------------------------------------------------------
// ----| Table |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct
{
    cw_obj_string_t* key;
    cw_value_t value;
} cw_table_entry_t;

typedef struct
{
    int count;
    int capacity;
    cw_table_entry_t* entries;
} cw_table_t;

void cw_table_init(cw_table_t* table);
void cw_table_free(cw_table_t* table);

void cw_table_copy(cw_table_t* from, cw_table_t* to);

bool cw_table_set(cw_table_t* table, cw_obj_string_t* key, cw_value_t value);
bool cw_table_get(cw_table_t* table, cw_obj_string_t* key, cw_value_t* value);
bool cw_table_delete(cw_table_t* table, cw_obj_string_t* key);

cw_obj_string_t* cw_table_find_string(cw_table_t* table, const char* chars, int length, uint32_t hash);

// -----------------------------------------------------------------------------
// ----| Debug |----------------------------------------------------------------
// -----------------------------------------------------------------------------

int cw_disassemble_instruction(cw_chunk_t* chunk, int offset);
void cw_disassemble_chunk(cw_chunk_t* chunk, const char* name);

// -----------------------------------------------------------------------------
// ----| scanner |--------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    // Single-character tokens.
    CW_TOKEN_LEFT_PAREN, CW_TOKEN_RIGHT_PAREN,
    CW_TOKEN_LEFT_BRACE, CW_TOKEN_RIGHT_BRACE,
    CW_TOKEN_COMMA, CW_TOKEN_DOT, CW_TOKEN_MINUS, CW_TOKEN_PLUS,
    CW_TOKEN_SEMICOLON, CW_TOKEN_SLASH, CW_TOKEN_STAR,
    // One or two character tokens.
    CW_TOKEN_BANG, CW_TOKEN_BANG_EQUAL,
    CW_TOKEN_EQUAL, CW_TOKEN_EQUAL_EQUAL,
    CW_TOKEN_GREATER, CW_TOKEN_GREATER_EQUAL,
    CW_TOKEN_LESS, CW_TOKEN_LESS_EQUAL,
    // Literals.
    CW_TOKEN_IDENTIFIER, CW_TOKEN_STRING, CW_TOKEN_NUMBER,
    // Keywords.
    CW_TOKEN_AND, CW_TOKEN_CLASS, CW_TOKEN_ELSE, CW_TOKEN_FALSE,
    CW_TOKEN_FOR, CW_TOKEN_FUN, CW_TOKEN_IF, CW_TOKEN_NIL, CW_TOKEN_OR,
    CW_TOKEN_PRINT, CW_TOKEN_RETURN, CW_TOKEN_SUPER, CW_TOKEN_THIS,
    CW_TOKEN_TRUE, CW_TOKEN_VAR, CW_TOKEN_WHILE,
    // Specials.
    CW_TOKEN_ERROR,
    CW_TOKEN_EOF
} cw_token_type;

typedef struct
{
    cw_token_type type;
    const char* start;
    int length;
    int line;
} cw_token_t;

typedef struct
{
  const char* start;
  const char* current;
  int line;
} cw_scanner_t;

void cw_scanner_init(const char* src);

cw_token_t cw_token_scan();

// -----------------------------------------------------------------------------
// ----| compiler |-------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct
{
    cw_token_t current;
    cw_token_t previous;
    bool had_error;
    bool panic_mode;
} cw_parser_t;

cw_obj_function_t* cw_compile(cw_virtual_machine_t* vm, const char* src);

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

#define CW_FRAMES_MAX   64
#define CW_STACK_MAX    (CW_FRAMES_MAX * CW_UINT8_COUNT)

typedef struct
{
    cw_obj_closure_t* closure;
    uint8_t* ip;
    cw_value_t* slots;
} cw_call_frame_t;

struct _cw_struct_vm
{
    cw_call_frame_t frames[CW_FRAMES_MAX];
    int frame_count;

    cw_value_t stack[CW_STACK_MAX];
    cw_value_t* stack_top;

    cw_table_t globals;
    cw_table_t strings;
    cw_obj_t* objects;
};


typedef enum
{
    CW_INTERPRET_OK,
    CW_INTERPRET_COMPILE_ERROR,
    CW_INTERPRET_RUNTIME_ERROR
} cw_interpret_result;

void cw_init(cw_virtual_machine_t* vm);
void cw_free(cw_virtual_machine_t* vm);

void cw_push(cw_virtual_machine_t* vm, cw_value_t v);
cw_value_t cw_pop(cw_virtual_machine_t* vm);

cw_interpret_result cw_interpret(cw_virtual_machine_t* vm, const char* src);

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

//
//
////   end header file   /////////////////////////////////////////////////////
#endif // CW_INCLUDE_CLOCKWORK_H

#ifdef CLOCKWORK_IMPLEMENTATION

// -----------------------------------------------------------------------------
// ----| Memory |---------------------------------------------------------------
// -----------------------------------------------------------------------------

void* cw_memory_reallocate(void* previous, size_t old_size, size_t new_size)
{
    if (new_size == 0) 
    {
        free(previous);
        return NULL;
    }
    return realloc(previous, new_size);
}

// -----------------------------------------------------------------------------
// ----| Value |----------------------------------------------------------------
// -----------------------------------------------------------------------------

void cw_value_array_init(cw_value_array_t* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void cw_value_array_free(cw_value_array_t* array)
{
    CW_FREE_ARRAY(cw_value_t, array->values, array->capacity);
    cw_value_array_init(array);
}

void cw_value_array_write(cw_value_array_t* array, cw_value_t value)
{
    if (array->capacity < array->count + 1)
    {
        int old_capacity = array->capacity;
        array->capacity = CW_GROW_CAPACITY(old_capacity);
        array->values = CW_GROW_ARRAY(array->values, cw_value_t, old_capacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

bool cw_values_equal(cw_value_t a, cw_value_t b)
{
    if (a.type != b.type) return false;

    switch (a.type)
    {
    case CW_VAL_BOOL:      return CW_AS_BOOL(a) == CW_AS_BOOL(b);
    case CW_VAL_NIL:       return true;
    case CW_VAL_NUMBER:    return CW_AS_NUMBER(a) == CW_AS_NUMBER(b);
    case CW_VAL_OBJ:       return CW_AS_OBJ(a) == CW_AS_OBJ(b);
    }

    return false;
}

void cw_print_value(cw_value_t value)
{
    switch (value.type)
    {
    case CW_VAL_BOOL:      printf(CW_AS_BOOL(value) ? "true" : "false"); break;
    case CW_VAL_NIL:       printf("nil"); break;
    case CW_VAL_NUMBER:    printf("%g", CW_AS_NUMBER(value)); break;
    case CW_VAL_OBJ:       cw_print_obj(value); break;
    }
}


// -----------------------------------------------------------------------------
// ----| Object |---------------------------------------------------------------
// -----------------------------------------------------------------------------

// FNV-1a hash function
uint32_t _cw_obj_string_hash(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

#define CW_ALLOCATE_OBJ(vm, type, object_type) (type*)_cw__obj_allocate(vm, sizeof(type), object_type)

static cw_obj_t* _cw__obj_allocate(cw_virtual_machine_t* vm, size_t size, cw_obj_type type)
{
    cw_obj_t* object = (cw_obj_t*)cw_memory_reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm->objects;
    vm->objects = object;

    return object;
}

static cw_obj_string_t* _cw_obj_allocate_string(cw_virtual_machine_t* vm, char* chars, int length, uint32_t hash)
{
    cw_obj_string_t* string = CW_ALLOCATE_OBJ(vm, cw_obj_string_t, CW_OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    cw_table_set(&vm->strings, string, CW_NIL_VAL);
    return string;
}

cw_obj_string_t* cw_obj_string_move(cw_virtual_machine_t* vm, char* chars, int length)
{
    uint32_t hash = _cw_obj_string_hash(chars, length);
    cw_obj_string_t* interned = cw_table_find_string(&vm->strings, chars, length, hash);
    if (interned != NULL)
    {
        CW_FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return _cw_obj_allocate_string(vm, chars, length, hash);
}

cw_obj_string_t* cw_obj_string_copy(cw_virtual_machine_t* vm, const char* chars, int length)
{
    uint32_t hash = _cw_obj_string_hash(chars, length);
    cw_obj_string_t* interned = cw_table_find_string(&vm->strings, chars, length, hash);

    if (interned != NULL) return interned;

    char* heap_chars = CW_ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return _cw_obj_allocate_string(vm, heap_chars, length, hash);
}

cw_obj_function_t* cw_function_new(cw_virtual_machine_t* vm)
{
    cw_obj_function_t* function = CW_ALLOCATE_OBJ(vm, cw_obj_function_t, CW_OBJ_FUNCTION);

    function->arity = 0;
    function->name = NULL;
    cw_chunk_init(&function->chunk);
    return function; 
}

cw_obj_native_t* cw_native_new(cw_virtual_machine_t* vm, cw_native_fn function)
{
    cw_obj_native_t* native = CW_ALLOCATE_OBJ(vm, cw_obj_native_t, CW_OBJ_NATIVE);
    native->function = function;
    return native;
}

cw_obj_closure_t* cw_closure_new(cw_virtual_machine_t* vm, cw_obj_function_t* function)
{
    cw_obj_closure_t* closure = CW_ALLOCATE_OBJ(vm, cw_obj_closure_t, CW_OBJ_CLOSURE);
    closure->function = function;
    return closure; 
}

static void _cw_object_free(cw_obj_t* object)
{
    switch (object->type)
    {
    case CW_OBJ_CLOSURE:
    {
        CW_FREE(cw_obj_closure_t, object);
        break;
    } 
    case CW_OBJ_FUNCTION:
    {
        cw_obj_function_t* function = (cw_obj_function_t*)object;
        cw_chunk_free(&function->chunk);
        CW_FREE(cw_obj_function_t, object);
        break;
    }
    case CW_OBJ_NATIVE:
        CW_FREE(cw_obj_native_t, object);
        break;
    case CW_OBJ_STRING: 
    {
        cw_obj_string_t* string = (cw_obj_string_t*)object;
        CW_FREE_ARRAY(char, string->chars, string->length + 1);
        CW_FREE(cw_obj_string_t, object);
        break;
    }
    }
}

void cw_objects_free(cw_virtual_machine_t* vm)
{
    cw_obj_t* object = vm->objects;
    while (object != NULL)
    {
        cw_obj_t* next = object->next;
        _cw_object_free(object);
        object = next;
    }
}

bool cw_obj_is_type(cw_value_t value, cw_obj_type type) { return CW_IS_OBJ(value) && CW_AS_OBJ(value)->type == type; }

static void _cw_print_function(cw_obj_function_t* function)
{
    if (function->name == NULL)
        printf("<script>");
    else
        printf("<fn %s>", function->name->chars);
}

void cw_print_obj(cw_value_t value)
{
    switch (CW_OBJ_TYPE(value))
    {
    case CW_OBJ_CLOSURE:
        _cw_print_function(CW_AS_CLOSURE(value)->function);
        break;
    case CW_OBJ_FUNCTION:
        _cw_print_function(CW_AS_FUNCTION(value));
        break;
    case CW_OBJ_NATIVE:
        printf("<native fn>");
        break;
    case CW_OBJ_STRING:
        printf("%s", CW_AS_CSTRING(value));
        break;
    }
}

// -----------------------------------------------------------------------------
// ----| Table |----------------------------------------------------------------
// -----------------------------------------------------------------------------

#define CW_TABLE_MAX_LOAD 0.75

void cw_table_init(cw_table_t* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void cw_table_free(cw_table_t* table)
{
    CW_FREE_ARRAY(cw_table_entry_t, table->entries, table->capacity);
    cw_table_init(table);
}

void cw_table_copy(cw_table_t* from, cw_table_t* to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        cw_table_entry_t* entry = &from->entries[i];
        if (entry->key != NULL)
            cw_table_set(to, entry->key, entry->value);
    }
}

static cw_table_entry_t* _cw_entry_find(cw_table_entry_t* entries, int capacity, cw_obj_string_t* key)
{
    uint32_t index = key->hash % capacity;
    cw_table_entry_t* tombstone = NULL;

    while (true)
    {
        cw_table_entry_t* entry = &entries[index];
        if (entry->key == NULL)
        {
            if (CW_IS_NIL(entry->value)) // Empty entry
                return tombstone != NULL ? tombstone : entry;
            else // We found a tombstone.
                if (tombstone == NULL) tombstone = entry;
        }
        else if (entry->key == key) // We found the key.
        {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void _cw_table_adjust_capacity(cw_table_t* table, int capacity)
{
    cw_table_entry_t* entries = CW_ALLOCATE(cw_table_entry_t, capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = CW_NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        cw_table_entry_t* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        cw_table_entry_t* dest = _cw_entry_find(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    CW_FREE_ARRAY(cw_table_entry_t, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool cw_table_set(cw_table_t* table, cw_obj_string_t* key, cw_value_t value)
{
    if (table->count + 1 > table->capacity * CW_TABLE_MAX_LOAD)
    {
        int capacity = CW_GROW_CAPACITY(table->capacity);
        _cw_table_adjust_capacity(table, capacity);
    }

    cw_table_entry_t* entry = _cw_entry_find(table->entries, table->capacity, key);

    bool is_new_key = entry->key == NULL;
    if (is_new_key && CW_IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

bool cw_table_get(cw_table_t* table, cw_obj_string_t* key, cw_value_t* value)
{
    if (table->count == 0) return false;

    cw_table_entry_t* entry = _cw_entry_find(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool cw_table_delete(cw_table_t* table, cw_obj_string_t* key)
{
    if (table->count == 0) return false;

    // Find the entry.
    cw_table_entry_t* entry = _cw_entry_find(table->entries, table->capacity, key);
     if (entry->key == NULL) return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = CW_BOOL_VAL(true);

    return true;
}

cw_obj_string_t* cw_table_find_string(cw_table_t* table, const char* chars, int length, uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;

    while (true)
    {
        cw_table_entry_t* entry = &table->entries[index];

        if (entry->key == NULL)
        {
            if (CW_IS_NIL(entry->value)) return NULL; // Stop if we find an empty non-tombstone entry.
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key; // We found it.
        }

        index = (index + 1) % table->capacity;
    }
}

// -----------------------------------------------------------------------------
// ----| Chunk |----------------------------------------------------------------
// -----------------------------------------------------------------------------

void cw_chunk_init(cw_chunk_t* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    cw_value_array_init(&chunk->constants);
}

void cw_chunk_free(cw_chunk_t* chunk)
{
    CW_FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    CW_FREE_ARRAY(int, chunk->lines, chunk->capacity);
    cw_value_array_free(&chunk->constants);
    cw_chunk_init(chunk);
}

void cw_chunk_write(cw_chunk_t* chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int old_capacity = chunk->capacity;
        chunk->capacity = CW_GROW_CAPACITY(old_capacity);
        chunk->code = CW_GROW_ARRAY(chunk->code, uint8_t, old_capacity, chunk->capacity);
        chunk->lines = CW_GROW_ARRAY(chunk->lines, int, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int cw_chunk_add_constant(cw_chunk_t* chunk, cw_value_t value)
{
    cw_value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// -----------------------------------------------------------------------------
// ----| Debug |----------------------------------------------------------------
// -----------------------------------------------------------------------------

static int _cw_simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

static int _cw_constant_instruction(const char* name, cw_chunk_t* chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    cw_print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

static int _cw_byte_instruction(const char* name, cw_chunk_t* chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int _cw_jump_instruction(const char* name, int sign, cw_chunk_t* chunk, int offset)
{
    uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int cw_disassemble_instruction(cw_chunk_t* chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case CW_OP_CONSTANT:        return _cw_constant_instruction("OP_CONSTANT", chunk, offset);
    case CW_OP_NIL:             return _cw_simple_instruction("OP_NIL", offset);
    case CW_OP_TRUE:            return _cw_simple_instruction("OP_TRUE", offset);
    case CW_OP_FALSE:           return _cw_simple_instruction("OP_FALSE", offset);
    case CW_OP_POP:             return _cw_simple_instruction("OP_POP", offset);
    case CW_OP_GET_LOCAL:       return _cw_byte_instruction("OP_GET_LOCAL", chunk, offset);
    case CW_OP_SET_LOCAL:       return _cw_byte_instruction("OP_SET_LOCAL", chunk, offset);
    case CW_OP_GET_GLOBAL:      return _cw_constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case CW_OP_DEFINE_GLOBAL:   return _cw_constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case CW_OP_SET_GLOBAL:      return _cw_constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case CW_OP_EQUAL:           return _cw_simple_instruction("OP_EQUAL", offset);
    case CW_OP_GREATER:         return _cw_simple_instruction("OP_GREATER", offset);
    case CW_OP_LESS:            return _cw_simple_instruction("OP_LESS", offset);
    case CW_OP_ADD:             return _cw_simple_instruction("OP_ADD", offset);
    case CW_OP_SUBTRACT:        return _cw_simple_instruction("OP_SUBTRACT", offset);
    case CW_OP_MULTIPLY:        return _cw_simple_instruction("OP_MULTIPLY", offset);
    case CW_OP_DIVIDE:          return _cw_simple_instruction("OP_DIVIDE", offset);
    case CW_OP_NOT:             return _cw_simple_instruction("OP_NOT", offset);
    case CW_OP_NEGATE:          return _cw_simple_instruction("OP_NEGATE", offset);
    case CW_OP_PRINT:           return _cw_simple_instruction("OP_PRINT", offset);
    case CW_OP_JUMP:            return _cw_jump_instruction("OP_JUMP", 1, chunk, offset);
    case CW_OP_JUMP_IF_FALSE:   return _cw_jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case CW_OP_LOOP:            return _cw_jump_instruction("OP_LOOP", -1, chunk, offset);
    case CW_OP_CALL:            return _cw_byte_instruction("OP_CALL", chunk, offset);
    case CW_OP_CLOSURE:
    {
        offset++;
        uint8_t constant = chunk->code[offset++];
        printf("%-16s %4d ", "OP_CLOSURE", constant);
        cw_print_value(chunk->constants.values[constant]);
        printf("\n");

        return offset;
    }
    case CW_OP_RETURN:          return _cw_simple_instruction("OP_RETURN", offset);
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void cw_disassemble_chunk(cw_chunk_t* chunk, const char* name)
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
        offset = cw_disassemble_instruction(chunk, offset);
}

// -----------------------------------------------------------------------------
// ----| scanner |--------------------------------------------------------------
// -----------------------------------------------------------------------------

cw_scanner_t scanner;

void cw_scanner_init(const char* src)
{
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
}

static bool _cw_scanner_last()
{
    return *scanner.current == '\0';
}

static char _cw_scanner_advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// returns the current character, but doesn’t consume it
static char _cw_scanner_peek()
{
    return *scanner.current;
}

// like peek() but for one character past the current one
static char _cw_scanner_peek_next()
{
    if (_cw_scanner_last()) return '\0';
    return scanner.current[1]; 
}

static bool _cw_scanner_match(char expected)
{
    if (_cw_scanner_last()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static cw_token_type _cw_scanner_check_keyword(int start, int length, const char* rest, cw_token_type type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
        return type;

    return CW_TOKEN_IDENTIFIER;
}

static void _cw_scanner_skip_whitespace()
{
    while(true)
    {
        char c = _cw_scanner_peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            _cw_scanner_advance();
            break;
        case '\n':
            scanner.line++;
            _cw_scanner_advance();
            break;
        case '/':
            if (_cw_scanner_peek_next() == '/')
                while (_cw_scanner_peek() != '\n' && !_cw_scanner_last()) _cw_scanner_advance();
            else
                return;
        default: return;
        }
    }
}

static cw_token_type _cw_scanner_identifier_type()
{
    switch (scanner.start[0])
    {
    case 'a': return _cw_scanner_check_keyword(1, 2, "nd", CW_TOKEN_AND);
    case 'c': return _cw_scanner_check_keyword(1, 4, "lass", CW_TOKEN_CLASS);
    case 'e': return _cw_scanner_check_keyword(1, 3, "lse", CW_TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) 
            switch (scanner.start[1])
            {
            case 'a': return _cw_scanner_check_keyword(2, 3, "lse", CW_TOKEN_FALSE);
            case 'o': return _cw_scanner_check_keyword(2, 1, "r", CW_TOKEN_FOR);
            case 'u': return _cw_scanner_check_keyword(2, 1, "n", CW_TOKEN_FUN);
            }
        break;
    case 'i': return _cw_scanner_check_keyword(1, 1, "f", CW_TOKEN_IF);
    case 'n': return _cw_scanner_check_keyword(1, 2, "il", CW_TOKEN_NIL);
    case 'o': return _cw_scanner_check_keyword(1, 1, "r", CW_TOKEN_OR);
    case 'p': return _cw_scanner_check_keyword(1, 4, "rint", CW_TOKEN_PRINT);
    case 'r': return _cw_scanner_check_keyword(1, 5, "eturn", CW_TOKEN_RETURN);
    case 's': return _cw_scanner_check_keyword(1, 4, "uper", CW_TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1)
            switch (scanner.start[1])
            {
            case 'h': return _cw_scanner_check_keyword(2, 2, "is", CW_TOKEN_THIS);
            case 'r': return _cw_scanner_check_keyword(2, 2, "ue", CW_TOKEN_TRUE);
            }
        break; 
    case 'v': return _cw_scanner_check_keyword(1, 2, "ar", CW_TOKEN_VAR);
    case 'w': return _cw_scanner_check_keyword(1, 4, "hile", CW_TOKEN_WHILE);
    }

    return CW_TOKEN_IDENTIFIER;
}

static bool _cw_is_digit(char c) { return c >= '0' && c <= '9'; }
static bool _cw_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

// token creation
static cw_token_t _cw_token_make(cw_token_type type)
{
    cw_token_t token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

// creates a error token with an error message
static cw_token_t _cw_token_make_error(const char* message)
{
    cw_token_t token;
    token.type = CW_TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

static cw_token_t _cw_token_make_string()
{
    while (_cw_scanner_peek() != '"' && !_cw_scanner_last())
    {
        if (_cw_scanner_peek() == '\n') scanner.line++;
        _cw_scanner_advance();
    }

    if (_cw_scanner_last()) return _cw_token_make_error("Unterminated string.");

    // The closing quote.
    _cw_scanner_advance();
    return _cw_token_make(CW_TOKEN_STRING); 
}

static cw_token_t _cw_token_make_number()
{
    while (_cw_is_digit(_cw_scanner_peek())) _cw_scanner_advance();

    // Look for a fractional part.
    if (_cw_scanner_peek() == '.' && _cw_is_digit(_cw_scanner_peek_next()))
    {
        _cw_scanner_advance(); // Consume the ".".
        while (_cw_is_digit(_cw_scanner_peek())) _cw_scanner_advance();
    }

    return _cw_token_make(CW_TOKEN_NUMBER);
}

static cw_token_t _cw_token_make_identifier()
{
    while (_cw_is_alpha(_cw_scanner_peek()) || _cw_is_digit(_cw_scanner_peek())) _cw_scanner_advance();
    return _cw_token_make(_cw_scanner_identifier_type());
}

cw_token_t cw_token_scan()
{
    _cw_scanner_skip_whitespace();
    scanner.start = scanner.current;
    if (_cw_scanner_last()) return _cw_token_make(CW_TOKEN_EOF);

    char c = _cw_scanner_advance();

    // identifiers and keywords
    if (_cw_is_alpha(c)) return _cw_token_make_identifier();
    if (_cw_is_digit(c)) return _cw_token_make_number();

    switch (c)
    {
    // single character tokens
    case '(': return _cw_token_make(CW_TOKEN_LEFT_PAREN);
    case ')': return _cw_token_make(CW_TOKEN_RIGHT_PAREN);
    case '{': return _cw_token_make(CW_TOKEN_LEFT_BRACE);
    case '}': return _cw_token_make(CW_TOKEN_RIGHT_BRACE);
    case ';': return _cw_token_make(CW_TOKEN_SEMICOLON);
    case ',': return _cw_token_make(CW_TOKEN_COMMA);
    case '.': return _cw_token_make(CW_TOKEN_DOT);
    case '-': return _cw_token_make(CW_TOKEN_MINUS);
    case '+': return _cw_token_make(CW_TOKEN_PLUS);
    case '/': return _cw_token_make(CW_TOKEN_SLASH);
    case '*': return _cw_token_make(CW_TOKEN_STAR);
    // two-character punctuation tokens
    case '!': return _cw_token_make(_cw_scanner_match('=') ? CW_TOKEN_BANG_EQUAL : CW_TOKEN_BANG);
    case '=': return _cw_token_make(_cw_scanner_match('=') ? CW_TOKEN_EQUAL_EQUAL : CW_TOKEN_EQUAL);
    case '<': return _cw_token_make(_cw_scanner_match('=') ? CW_TOKEN_LESS_EQUAL : CW_TOKEN_LESS);
    case '>': return _cw_token_make(_cw_scanner_match('=') ? CW_TOKEN_GREATER_EQUAL : CW_TOKEN_GREATER);
    // literal tokens
    case '"': return _cw_token_make_string();
    }

    return _cw_token_make_error("Unexpected character.");
}

// -----------------------------------------------------------------------------
// ----| compiler |-------------------------------------------------------------
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// parser error
static void _cw_parser_error_at(cw_parser_t* parser, cw_token_t* token, const char* message)
{
    if (parser->panic_mode) return;
    parser->panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == CW_TOKEN_EOF) 
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == CW_TOKEN_ERROR)
    {
        // Nothing.
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser->had_error = true;
}

static void _cw_parser_error_at_current(cw_parser_t* parser, const char* message)
{
    _cw_parser_error_at(parser, &parser->current, message);
}

static void _cw_parser_error(cw_parser_t* parser, const char* message)
{
    _cw_parser_error_at(parser, &parser->previous, message);
}

// -----------------------------------------------------------------------------
// compiler

typedef enum
{
    CW_TYPE_FUNCTION,
    CW_TYPE_SCRIPT
} cw_function_type;

typedef struct
{
    cw_token_t name;
    int depth;
} cw_local_t;

typedef struct _cw_struct_compiler
{
    struct _cw_struct_compiler* enclosing;
    cw_obj_function_t* function;
    cw_function_type type;
    cw_local_t locals[CW_UINT8_COUNT];
    int local_count;
    int scope_depth;
} cw_compiler_t;

static void _cw_compiler_mark_initialized(cw_compiler_t* compiler)
{
    if (compiler->scope_depth == 0) return;

    compiler->locals[compiler->local_count - 1].depth = compiler->scope_depth;
}

cw_compiler_t* current_compiler = NULL;

cw_chunk_t* _cw_current_chunk()
{
    return &current_compiler->function->chunk;
}

static bool _cw_identifiers_equal(cw_token_t* a, cw_token_t* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void _cw_local_add(cw_compiler_t* compiler, cw_parser_t* parser, cw_token_t name)
{
    if (compiler->local_count == CW_UINT8_COUNT)
    {
        _cw_parser_error(parser, "Too many local variables in function.");
        return;
    }

    cw_local_t* local = &compiler->locals[compiler->local_count++];
    local->name = name;
    local->depth = -1; 
}

static int _cw_local_resolve(cw_compiler_t* compiler, cw_parser_t* parser, cw_token_t* name)
{
    for (int i = compiler->local_count - 1; i >= 0; i--)
    {
        cw_local_t* local = &compiler->locals[i];
        if (_cw_identifiers_equal(name, &local->name))
        {
            if (local->depth == -1)
                _cw_parser_error(parser, "Cannot read local variable in its own initializer.");

            return i;
        }
    }
    return -1;
}

// -----------------------------------------------------------------------------
// parser
static void _cw_parser_advance(cw_parser_t* parser)
{
    parser->previous = parser->current;

    while (true)
    {
        parser->current = cw_token_scan();
        if (parser->current.type != CW_TOKEN_ERROR) break;

        _cw_parser_error_at_current(parser, parser->current.start);
    }
}

static void _cw_parser_consume(cw_parser_t* parser, cw_token_type type, const char* message)
{
    if (parser->current.type == type)
    {
        _cw_parser_advance(parser);
        return;
    }
    _cw_parser_error_at_current(parser, message); 
}

static bool _cw_parser_check(cw_parser_t* parser, cw_token_type type)
{
    return parser->current.type == type;
}

static bool _cw_parser_match(cw_parser_t* parser, cw_token_type type)
{
    if (!_cw_parser_check(parser, type)) return false;

    _cw_parser_advance(parser);

    return true;
}

static void _cw_parser_synchronize(cw_parser_t* parser)
{
    parser->panic_mode = false;
    while (parser->current.type != CW_TOKEN_EOF)
    {
        if (parser->previous.type == CW_TOKEN_SEMICOLON) return;

        switch (parser->current.type)
        {
        case CW_TOKEN_CLASS:
        case CW_TOKEN_FUN:
        case CW_TOKEN_VAR:
        case CW_TOKEN_FOR:
        case CW_TOKEN_IF:
        case CW_TOKEN_WHILE:
        case CW_TOKEN_PRINT:
        case CW_TOKEN_RETURN:
            return;
        default:
            break; // Do nothing.
        }
        _cw_parser_advance(parser);
    }
}

// -----------------------------------------------------------------------------
// parser constant

static uint8_t _cw_parser_make_constant(cw_parser_t* parser, cw_value_t value)
{
    int constant = cw_chunk_add_constant(_cw_current_chunk(), value);
    if (constant > UINT8_MAX)
    {
        _cw_parser_error(parser, "Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

static uint8_t _cw_parser_make_constant_indentifier(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_token_t* name)
{
    return _cw_parser_make_constant(parser, CW_OBJ_VAL(cw_obj_string_copy(vm, name->start, name->length)));
}

// -----------------------------------------------------------------------------
// parser emit
static void _cw_parser_emit_byte(cw_parser_t* parser, uint8_t byte)
{
    cw_chunk_write(_cw_current_chunk(), byte, parser->previous.line);
}

static void _cw_parser_emit_return(cw_parser_t* parser)
{
    _cw_parser_emit_byte(parser, CW_OP_NIL);
    _cw_parser_emit_byte(parser, CW_OP_RETURN);
}

// convenience function to write an opcode followed by a one-byte operand
static void _cw_parser_emit_bytes(cw_parser_t* parser, uint8_t byte1, uint8_t byte2)
{
    _cw_parser_emit_byte(parser, byte1);
    _cw_parser_emit_byte(parser, byte2);
}

static void _cw_parser_emit_constant(cw_parser_t* parser, cw_value_t value)
{
    _cw_parser_emit_bytes(parser, CW_OP_CONSTANT, _cw_parser_make_constant(parser, value));
}

static void _cw_emit_loop(cw_parser_t* parser, int loop_start)
{
    _cw_parser_emit_byte(parser, CW_OP_LOOP);

    int offset = _cw_current_chunk()->count - loop_start + 2;
    
    if (offset > UINT16_MAX)
        _cw_parser_error(parser, "Loop body too large.");

    _cw_parser_emit_byte(parser, (offset >> 8) & 0xff);
    _cw_parser_emit_byte(parser, offset & 0xff);
}

static int _cw_emit_jump(cw_parser_t* parser, uint8_t instruction)
{
    _cw_parser_emit_byte(parser, instruction);
    _cw_parser_emit_byte(parser, 0xff);
    _cw_parser_emit_byte(parser, 0xff);
    return _cw_current_chunk()->count - 2;
}

// -----------------------------------------------------------------------------
// parser variable
static void _cw_parser_define_variable(cw_parser_t* parser, uint8_t global)
{
    if (current_compiler->scope_depth > 0)
    {
        _cw_compiler_mark_initialized(current_compiler);
        return;
    }

    _cw_parser_emit_bytes(parser, CW_OP_DEFINE_GLOBAL, global);
}

static void _cw_parser_declare_variable(cw_parser_t* parser)
{
    // Global variables are implicitly declared.
    if (current_compiler->scope_depth == 0) return;

    cw_token_t* name = &parser->previous;
    for (int i = current_compiler->local_count - 1; i >= 0; i--)
    {
        cw_local_t* local = &current_compiler->locals[i];
        if (local->depth != -1 && local->depth < current_compiler->scope_depth)
            break;

        if (_cw_identifiers_equal(name, &local->name))
            _cw_parser_error(parser, "Variable with this name already declared in this scope.");
    }

    _cw_local_add(current_compiler, parser, *name);
}

static uint8_t _cw_parser_make_variable(cw_virtual_machine_t* vm, cw_parser_t* parser, const char* error_message)
{
    _cw_parser_consume(parser, CW_TOKEN_IDENTIFIER, error_message);

    _cw_parser_declare_variable(parser);
    if (current_compiler->scope_depth > 0) return 0;

    return _cw_parser_make_constant_indentifier(vm, parser, &parser->previous);
}

// -----------------------------------------------------------------------------
// jump

static void _cw_patch_jump(cw_parser_t* parser, int offset)
{
    // -2 to adjust for the bytecode for the jump offset itself.
    int jump = _cw_current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX)
        _cw_parser_error(parser, "Too much code to jump over.");

    _cw_current_chunk()->code[offset] = (jump >> 8) & 0xff;
    _cw_current_chunk()->code[offset + 1] = jump & 0xff;
}

// -----------------------------------------------------------------------------
// parse rules

typedef enum
{
    CW_PREC_NONE,
    CW_PREC_ASSIGNMENT,    // =
    CW_PREC_OR,            // or
    CW_PREC_AND,           // and
    CW_PREC_EQUALITY,      // == !=
    CW_PREC_COMPARISON,    // < > <= >=
    CW_PREC_TERM,          // + -
    CW_PREC_FACTOR,        // * /
    CW_PREC_UNARY,         // ! -
    CW_PREC_CALL,          // . ()
    CW_PREC_PRIMARY
} cw_precedence;

typedef void (*cw_parse_fn)(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign);

typedef struct
{
    cw_parse_fn prefix;
    cw_parse_fn infix;
    cw_precedence precedence;
} cw_parse_rule_t;

static cw_parse_rule_t* _cw_get_parse_rule(cw_token_type type);

static void _cw_parse_precedence(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_precedence pre)
{
    _cw_parser_advance(parser);

    cw_parse_fn prefix_rule = _cw_get_parse_rule(parser->previous.type)->prefix;

    if (prefix_rule == NULL)
    {
        _cw_parser_error(parser, "Expect expression.");
        return;
    }

    bool can_assign = pre <= CW_PREC_ASSIGNMENT;
    prefix_rule(vm, parser, can_assign);

    while (pre <= _cw_get_parse_rule(parser->current.type)->precedence)
    {
        _cw_parser_advance(parser);
        cw_parse_fn infix_rule = _cw_get_parse_rule(parser->previous.type)->infix;
        infix_rule(vm, parser, can_assign);
    }

    if (can_assign && _cw_parser_match(parser, CW_TOKEN_EQUAL))
        _cw_parser_error(parser, "Invalid assignment target.");
}

static void _cw_expression(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    _cw_parse_precedence(vm, parser, CW_PREC_ASSIGNMENT);
}

static void _cw_number(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    _cw_parser_emit_constant(parser, CW_NUMBER_VAL(strtod(parser->previous.start, NULL)));
}

static void _cw_string(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    _cw_parser_emit_constant(parser, CW_OBJ_VAL(cw_obj_string_copy(vm, parser->previous.start + 1, parser->previous.length - 2)));
}

static void _cw_variable_named(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_token_t name, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = _cw_local_resolve(current_compiler, parser, &name);
    if (arg != -1)
    {
        get_op = CW_OP_GET_LOCAL;
        set_op = CW_OP_SET_LOCAL;
    }
    else
    {
        arg = _cw_parser_make_constant_indentifier(vm, parser, &name);
        get_op = CW_OP_GET_GLOBAL;
        set_op = CW_OP_SET_GLOBAL;
    }

    if (can_assign && _cw_parser_match(parser, CW_TOKEN_EQUAL))
    {
        _cw_expression(vm, parser);
        _cw_parser_emit_bytes(parser, set_op, (uint8_t)arg);
    }
    else
    {
        _cw_parser_emit_bytes(parser, get_op, (uint8_t)arg);
    }
}

static void _cw_variable(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    _cw_variable_named(vm, parser, parser->previous, can_assign);
}

static void _cw_grouping(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    _cw_expression(vm, parser);
    _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void _cw_unary(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    cw_token_type operatorType = parser->previous.type;

    // Compile the operand.
    _cw_parse_precedence(vm, parser, CW_PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType)
    {
    case CW_TOKEN_BANG:    _cw_parser_emit_byte(parser, CW_OP_NOT); break;
    case CW_TOKEN_MINUS:   _cw_parser_emit_byte(parser, CW_OP_NEGATE); break;
    default: return; // Unreachable.
    }
}

static void _cw_binary(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    // Remember the operator.
    cw_token_type operatorType = parser->previous.type;

    // Compile the right operand.
    cw_parse_rule_t* rule = _cw_get_parse_rule(operatorType);
    _cw_parse_precedence(vm, parser, (cw_precedence)(rule->precedence + 1));

    // Emit the operator instruction.
    switch (operatorType)
    {
    case CW_TOKEN_BANG_EQUAL:       _cw_parser_emit_bytes(parser, CW_OP_EQUAL, CW_OP_NOT); break;
    case CW_TOKEN_EQUAL_EQUAL:      _cw_parser_emit_byte(parser, CW_OP_EQUAL); break;
    case CW_TOKEN_GREATER:          _cw_parser_emit_byte(parser, CW_OP_GREATER); break;
    case CW_TOKEN_GREATER_EQUAL:    _cw_parser_emit_bytes(parser, CW_OP_LESS, CW_OP_NOT); break;
    case CW_TOKEN_LESS:             _cw_parser_emit_byte(parser, CW_OP_LESS); break;
    case CW_TOKEN_LESS_EQUAL:       _cw_parser_emit_bytes(parser, CW_OP_GREATER, CW_OP_NOT); break;
    case CW_TOKEN_PLUS:             _cw_parser_emit_byte(parser, CW_OP_ADD); break;
    case CW_TOKEN_MINUS:            _cw_parser_emit_byte(parser, CW_OP_SUBTRACT); break;
    case CW_TOKEN_STAR:             _cw_parser_emit_byte(parser, CW_OP_MULTIPLY); break;
    case CW_TOKEN_SLASH:            _cw_parser_emit_byte(parser, CW_OP_DIVIDE); break;
    default: return; // Unreachable.
    }
}

static void _cw_literal(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    switch (parser->previous.type) 
    {
    case CW_TOKEN_FALSE:    _cw_parser_emit_byte(parser, CW_OP_FALSE); break;
    case CW_TOKEN_NIL:      _cw_parser_emit_byte(parser, CW_OP_NIL); break;
    case CW_TOKEN_TRUE:     _cw_parser_emit_byte(parser, CW_OP_TRUE); break;
    default: return; // Unreachable.
    }
}

static void _cw_and(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    int end_jump = _cw_emit_jump(parser, CW_OP_JUMP_IF_FALSE);

    _cw_parser_emit_byte(parser, CW_OP_POP);

    _cw_parse_precedence(vm, parser, CW_PREC_AND);
    _cw_patch_jump(parser, end_jump);
}

static void _cw_or(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    int else_jump = _cw_emit_jump(parser, CW_OP_JUMP_IF_FALSE);
    int end_jump = _cw_emit_jump(parser, CW_OP_JUMP);

    _cw_patch_jump(parser, else_jump);
    _cw_parser_emit_byte(parser, CW_OP_POP);

    _cw_parse_precedence(vm, parser, CW_PREC_OR);
    _cw_patch_jump(parser, end_jump);
}

static uint8_t argument_list(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    uint8_t arg_count = 0;
    if (!_cw_parser_check(parser, CW_TOKEN_RIGHT_PAREN))
    {
        do
        {
            _cw_expression(vm, parser);
            if (arg_count == 255) 
                _cw_parser_error(parser, "Cannot have more than 255 arguments."); 
            arg_count++;
        }
        while (_cw_parser_match(parser, CW_TOKEN_COMMA));
    }

    _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return arg_count;
}

static void _cw_call(cw_virtual_machine_t* vm, cw_parser_t* parser, bool can_assign)
{
    uint8_t arg_count = argument_list(vm, parser);
    _cw_parser_emit_bytes(parser, CW_OP_CALL, arg_count);  
}

cw_parse_rule_t _cw_parse_rules[] =
{
    { _cw_grouping, _cw_call,   CW_PREC_CALL },         // TOKEN_LEFT_PAREN
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_RIGHT_PAREN
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_LEFT_BRACE
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_RIGHT_BRACE
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_COMMA
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_DOT
    { _cw_unary,    _cw_binary, CW_PREC_TERM },         // TOKEN_MINUS
    { NULL,         _cw_binary, CW_PREC_TERM },         // TOKEN_PLUS
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_SEMICOLON
    { NULL,         _cw_binary, CW_PREC_FACTOR },       // TOKEN_SLASH
    { NULL,         _cw_binary, CW_PREC_FACTOR },       // TOKEN_STAR
    { _cw_unary,    NULL,       CW_PREC_NONE },         // TOKEN_BANG
    { NULL,         _cw_binary, CW_PREC_EQUALITY },     // TOKEN_BANG_EQUAL
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_EQUAL
    { NULL,         _cw_binary, CW_PREC_EQUALITY },     // TOKEN_EQUAL_EQUAL
    { NULL,         _cw_binary, CW_PREC_COMPARISON },   // TOKEN_GREATER
    { NULL,         _cw_binary, CW_PREC_COMPARISON },   // TOKEN_GREATER_EQUAL
    { NULL,         _cw_binary, CW_PREC_COMPARISON },   // TOKEN_LESS
    { NULL,         _cw_binary, CW_PREC_COMPARISON },   // TOKEN_LESS_EQUAL
    { _cw_variable, NULL,       CW_PREC_NONE },         // TOKEN_IDENTIFIER
    { _cw_string,   NULL,       CW_PREC_NONE },         // TOKEN_STRING
    { _cw_number,   NULL,       CW_PREC_NONE },         // TOKEN_NUMBER
    { NULL,         _cw_and,    CW_PREC_AND },          // TOKEN_AND
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_CLASS
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_ELSE
    { _cw_literal,  NULL,       CW_PREC_NONE },         // TOKEN_FALSE
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_FOR
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_FUN
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_IF
    { _cw_literal,  NULL,       CW_PREC_NONE },         // TOKEN_NIL
    { NULL,         _cw_or,     CW_PREC_OR },           // TOKEN_OR
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_PRINT
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_RETURN
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_SUPER
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_THIS
    { _cw_literal,  NULL,       CW_PREC_NONE },         // TOKEN_TRUE
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_VAR
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_WHILE
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_ERROR
    { NULL,         NULL,       CW_PREC_NONE },         // TOKEN_EOF
};

static cw_parse_rule_t* _cw_get_parse_rule(cw_token_type type)
{
    return &_cw_parse_rules[type];
}

// -----------------------------------------------------------------------------
// scope

static void _cw_begin_scope(cw_parser_t* parser) { current_compiler->scope_depth++; }

static void _cw_end_scope(cw_parser_t* parser)
{
    current_compiler->scope_depth--;

    while (current_compiler->local_count > 0 && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth)
    {
        _cw_parser_emit_byte(parser, CW_OP_POP);
        current_compiler->local_count--;
    }
}

// -----------------------------------------------------------------------------
// statements

static void _cw_statement(cw_virtual_machine_t* vm, cw_parser_t* parser);
static void _cw_function(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_function_type type);

static void _cw_declaration_var(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    uint8_t global = _cw_parser_make_variable(vm, parser, "Expect variable name.");

    if (_cw_parser_match(parser, CW_TOKEN_EQUAL))
        _cw_expression(vm, parser);
    else
        _cw_parser_emit_byte(parser, CW_OP_NIL);

    _cw_parser_consume(parser, CW_TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    _cw_parser_define_variable(parser, global);
}

static void _cw_declaration_fun(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    uint8_t global = _cw_parser_make_variable(vm, parser, "Expect function name.");
    _cw_compiler_mark_initialized(current_compiler);
    _cw_function(vm, parser, CW_TYPE_FUNCTION);
    _cw_parser_define_variable(parser, global);
}

static void _cw_declaration(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    if (_cw_parser_match(parser, CW_TOKEN_FUN))
        _cw_declaration_fun(vm, parser);
    else if (_cw_parser_match(parser, CW_TOKEN_VAR))
        _cw_declaration_var(vm, parser);
    else
        _cw_statement(vm, parser);

    if (parser->panic_mode) _cw_parser_synchronize(parser);
}

static void _cw_statement_expression(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    _cw_expression(vm, parser);
    _cw_parser_consume(parser, CW_TOKEN_SEMICOLON, "Expect ';' after expression.");
    _cw_parser_emit_byte(parser, CW_OP_POP);
}

static void _cw_statement_print(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    _cw_expression(vm, parser);
    _cw_parser_consume(parser, CW_TOKEN_SEMICOLON, "Expect ';' after value.");
    _cw_parser_emit_byte(parser, CW_OP_PRINT);
}

static void _cw_statement_return(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    if (current_compiler->type == CW_TYPE_SCRIPT)
        _cw_parser_error(parser, "Cannot return from top-level code.");

    if (_cw_parser_match(parser, CW_TOKEN_SEMICOLON))
    {
        _cw_parser_emit_return(parser);
    }
    else
    {
        _cw_expression(vm, parser);
        _cw_parser_consume(parser, CW_TOKEN_SEMICOLON, "Expect ';' after return value.");
        _cw_parser_emit_byte(parser, CW_OP_RETURN);
    }
}

static void _cw_statement_if(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    _cw_parser_consume(parser, CW_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    _cw_expression(vm, parser);
    _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    
    int then_jump = _cw_emit_jump(parser, CW_OP_JUMP_IF_FALSE);

    _cw_parser_emit_byte(parser, CW_OP_POP);
    _cw_statement(vm, parser);

    int else_jump = _cw_emit_jump(parser, CW_OP_JUMP);

    _cw_patch_jump(parser, then_jump);
    _cw_parser_emit_byte(parser, CW_OP_POP);

    if (_cw_parser_match(parser, CW_TOKEN_ELSE))
        _cw_statement(vm, parser);
    
    _cw_patch_jump(parser, else_jump);
}

static void _cw_statement_while(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    int loop_start = _cw_current_chunk()->count; 

    _cw_parser_consume(parser, CW_TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    _cw_expression(vm, parser);
    _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int exit_jump = _cw_emit_jump(parser ,CW_OP_JUMP_IF_FALSE);

    _cw_parser_emit_byte(parser, CW_OP_POP);
    _cw_statement(vm, parser);

    _cw_emit_loop(parser, loop_start);

    _cw_patch_jump(parser, exit_jump);
    _cw_parser_emit_byte(parser, CW_OP_POP);
}

static void _cw_statement_for(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    _cw_begin_scope(parser);
    _cw_parser_consume(parser, CW_TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // Initializer clause
    if (_cw_parser_match(parser, CW_TOKEN_SEMICOLON))
    {
        // No initializer.
    }
    else if (_cw_parser_match(parser, CW_TOKEN_VAR))
    {
        _cw_declaration_var(vm, parser);
    }
    else
    {
        _cw_statement_expression(vm, parser);
    }

    int loop_start = _cw_current_chunk()->count;

    // Condition clause
    int exit_jump = -1;
    if (!_cw_parser_match(parser, CW_TOKEN_SEMICOLON))
    {
        _cw_expression(vm, parser);
        _cw_parser_consume(parser, CW_TOKEN_SEMICOLON, "Expect ';' after loop condition.");

        // Jump out of the loop if the condition is false.
        exit_jump = _cw_emit_jump(parser, CW_OP_JUMP_IF_FALSE);
        _cw_parser_emit_byte(parser, CW_OP_POP); // Condition.
    }

    // Increment clause
    if (!_cw_parser_match(parser, CW_TOKEN_RIGHT_PAREN))
    {
        int body_jump = _cw_emit_jump(parser, CW_OP_JUMP);
        int increment_start = _cw_current_chunk()->count;

        _cw_expression(vm, parser);
        _cw_parser_emit_byte(parser, CW_OP_POP);
        _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

        _cw_emit_loop(parser, loop_start);
        loop_start = increment_start;
        _cw_patch_jump(parser, body_jump);
    }

    _cw_statement(vm, parser);
    _cw_emit_loop(parser, loop_start);

    // After the loop body, patch exit jump
    if (exit_jump != -1)
    {
        _cw_patch_jump(parser, exit_jump);
        _cw_parser_emit_byte(parser, CW_OP_POP); // Condition.
    }

    _cw_end_scope(parser);
}

static void _cw_block(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    while (!_cw_parser_check(parser, CW_TOKEN_RIGHT_BRACE) && !_cw_parser_check(parser, CW_TOKEN_EOF))
    {
        _cw_declaration(vm, parser);
    }

    _cw_parser_consume(parser, CW_TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void _cw_statement(cw_virtual_machine_t* vm, cw_parser_t* parser)
{
    if (_cw_parser_match(parser, CW_TOKEN_PRINT))
    {
        _cw_statement_print(vm, parser);
    }
    else if (_cw_parser_match(parser, CW_TOKEN_IF))
    {
        _cw_statement_if(vm, parser);
    }
    else if (_cw_parser_match(parser, CW_TOKEN_RETURN))
    {
        _cw_statement_return(vm, parser);
    }
    else if (_cw_parser_match(parser, CW_TOKEN_WHILE))
    {
        _cw_statement_while(vm, parser);
    }
    else if (_cw_parser_match(parser, CW_TOKEN_FOR))
    {
        _cw_statement_for(vm, parser);
    }
    else if (_cw_parser_match(parser, CW_TOKEN_LEFT_BRACE))
    {
        _cw_begin_scope(parser);
        _cw_block(vm, parser);
        _cw_end_scope(parser);
    }
    else
    {
        _cw_statement_expression(vm, parser);
    }
}

// -----------------------------------------------------------------------------
// compile

static void _cw_compiler_init(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_compiler_t* compiler, cw_function_type type)
{
    compiler->enclosing = current_compiler;
    compiler->function = NULL;
    compiler->type = type;

    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->function = cw_function_new(vm);
    current_compiler = compiler;

    if (type != CW_TYPE_SCRIPT) 
        current_compiler->function->name = cw_obj_string_copy(vm, parser->previous.start, parser->previous.length);

    cw_local_t* local = &current_compiler->locals[current_compiler->local_count++];
    local->depth = 0;
    local->name.start = "";
    local->name.length = 0;
}

static cw_obj_function_t* _cw_compiler_end(cw_parser_t* parser)
{
    _cw_parser_emit_return(parser);

    cw_obj_function_t* function = current_compiler->function;

#ifdef CW_DEBUG_PRINT_CODE
    if (!parser->had_error)
        cw_disassemble_chunk(_cw_current_chunk(), function->name != NULL ? function->name->chars : "<script>");
#endif
    current_compiler = current_compiler->enclosing;
    return function;
}

cw_obj_function_t* cw_compile(cw_virtual_machine_t* vm, const char* src)
{
    cw_scanner_init(src);

    cw_parser_t parser;
    parser.had_error = false;
    parser.panic_mode = false;

    cw_compiler_t compiler;
    _cw_compiler_init(vm, &parser, &compiler, CW_TYPE_SCRIPT);

    _cw_parser_advance(&parser);

    while(!_cw_parser_match(&parser, CW_TOKEN_EOF))
    {
        _cw_declaration(vm, &parser);
    }

    cw_obj_function_t* function = _cw_compiler_end(&parser);
    return parser.had_error ? NULL : function;
}

static void _cw_function(cw_virtual_machine_t* vm, cw_parser_t* parser, cw_function_type type)
{
    cw_compiler_t compiler;
    _cw_compiler_init(vm, parser, &compiler, type);
    _cw_begin_scope(parser);

    // Compile the parameter list.
    _cw_parser_consume(parser, CW_TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!_cw_parser_check(parser, CW_TOKEN_RIGHT_PAREN))
    {
        do 
        {
            current_compiler->function->arity++;
            if (current_compiler->function->arity > 255)
            {
                _cw_parser_error_at_current(parser, "Cannot have more than 255 parameters.");
            }

            uint8_t param_constant = _cw_parser_make_variable(vm, parser, "Expect parameter name.");
            _cw_parser_define_variable(parser, param_constant);
        }
        while (_cw_parser_match(parser, CW_TOKEN_COMMA));
    }
    _cw_parser_consume(parser, CW_TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    // The body.
    _cw_parser_consume(parser, CW_TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    _cw_block(vm, parser);

    // Create the function object.
    cw_obj_function_t* function = _cw_compiler_end(parser);
    _cw_parser_emit_bytes(parser, CW_OP_CLOSURE, _cw_parser_make_constant(parser, CW_OBJ_VAL(function)));
}

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

static void _cw_reset_stack(cw_virtual_machine_t* vm)
{
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
}

static void _cw_runtime_error(cw_virtual_machine_t* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm->frame_count - 1; i >= 0; i--)
    {
        cw_call_frame_t* frame = &vm->frames[i];
        cw_obj_function_t* function = frame->closure->function;
    
        // -1 because the IP is sitting on the next instruction to be // executed.
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        
        if (function->name == NULL)
            fprintf(stderr, "script\n");
        else
            fprintf(stderr, "%s()\n", function->name->chars);
    }
    _cw_reset_stack(vm);
}

// -----------------------------------------------------------------------------
// naitve

static void _cw_define_native(cw_virtual_machine_t* vm, const char* name, cw_native_fn function)
{
    cw_push(vm, CW_OBJ_VAL(cw_obj_string_copy(vm, name, (int)strlen(name))));
    cw_push(vm, CW_OBJ_VAL(cw_native_new(vm, function)));
    cw_table_set(&vm->globals, CW_AS_STRING(vm->stack[0]), vm->stack[1]);
    cw_pop(vm);
    cw_pop(vm);
}

#include <time.h>

static cw_value_t _cw_native_fun_clock(int argCount, cw_value_t* args) 
{
    return CW_NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

void cw_init(cw_virtual_machine_t* vm)
{
    _cw_reset_stack(vm);
    vm->objects = NULL;

    cw_table_init(&vm->globals);
    cw_table_init(&vm->strings);

    // natives
    _cw_define_native(vm, "clock", _cw_native_fun_clock);
}

void cw_free(cw_virtual_machine_t* vm)
{
    cw_table_free(&vm->globals);
    cw_table_free(&vm->strings);
    cw_objects_free(vm);
}

void cw_push(cw_virtual_machine_t* vm, cw_value_t v)
{
    *vm->stack_top = v;
    vm->stack_top++;
}

cw_value_t cw_pop(cw_virtual_machine_t* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}

static cw_value_t _cw_peek(cw_virtual_machine_t* vm, int distance)
{
    return vm->stack_top[-1 - distance];
}

static bool _cw_call_closure(cw_virtual_machine_t* vm, cw_obj_closure_t* closure, int arg_count)
{
    if (arg_count != closure->function->arity)
    {
        _cw_runtime_error(vm, "Expected %d arguments but got %d.", closure->function->arity, arg_count);
        return false;
    }

    if (vm->frame_count == CW_FRAMES_MAX)
    {
        _cw_runtime_error(vm, "Stack overflow.");
        return false;
    }

    cw_call_frame_t* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;

    frame->slots = vm->stack_top - arg_count - 1;
    return true;
} 

static bool _cw_call_value(cw_virtual_machine_t* vm, cw_value_t callee, int arg_count)
{
    if (CW_IS_OBJ(callee))
    {
        switch (CW_OBJ_TYPE(callee))
        {
        case CW_OBJ_CLOSURE: return _cw_call_closure(vm, CW_AS_CLOSURE(callee), arg_count);
        case CW_OBJ_NATIVE:
        {
            cw_native_fn native = CW_AS_NATIVE(callee);
            cw_value_t result = native(arg_count, vm->stack_top - arg_count);
            vm->stack_top -= arg_count + 1;
            cw_push(vm, result);
            return true;
        }
        default: break;// Non-callable object type.
        }
    }

    _cw_runtime_error(vm, "Can only call functions and classes.");
    return false;
}

static void _cw_concatenate(cw_virtual_machine_t* vm)
{
    cw_obj_string_t* b = CW_AS_STRING(cw_pop(vm));
    cw_obj_string_t* a = CW_AS_STRING(cw_pop(vm));

    int length = a->length + b->length;
    char* chars = CW_ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    cw_obj_string_t* result = cw_obj_string_move(vm, chars, length);
    cw_push(vm, CW_OBJ_VAL(result));
}

static bool _cw_is_falsey(cw_value_t v)
{ 
    return CW_IS_NIL(v) || (CW_IS_BOOL(v) && !CW_AS_BOOL(v)); 
}

static cw_interpret_result _cw_run(cw_virtual_machine_t* vm)
{
    cw_call_frame_t* frame = &vm->frames[vm->frame_count -1];

#define _CW_READ_BYTE()         (*frame->ip++)
#define _CW_READ_SHORT()        (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define _CW_READ_CONSTANT()     (frame->closure->function->chunk.constants.values[_CW_READ_BYTE()])
#define _CW_READ_STRING()       CW_AS_STRING(_CW_READ_CONSTANT())
#define _CW_BINARY_OP(type, op)                                                 \
    do                                                                          \
    {                                                                           \
        if (!CW_IS_NUMBER(_cw_peek(vm, 0)) || !CW_IS_NUMBER(_cw_peek(vm, 1)))   \
        {                                                                       \
            _cw_runtime_error(vm, "Operands must be numbers.");                 \
            return CW_INTERPRET_RUNTIME_ERROR;                                  \
        }                                                                       \
        double b = CW_AS_NUMBER(cw_pop(vm));                                    \
        double a = CW_AS_NUMBER(cw_pop(vm));                                    \
        cw_push(vm, type(a op b));                                              \
    } while (false)

    while(true)
    {
#ifdef CW_DEBUG_TRACE_EXECUTION
        printf("          ");
        for (cw_value_t* slot = vm->stack; slot < vm->stack_top; slot++)
        {
            printf("[ ");
            cw_print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        cw_disassemble_instruction(&frame->closure->function->chunk, (int)(frame->ip - frame->closure->function->chunk.code));
#endif

        uint8_t instruction;
        switch (instruction = _CW_READ_BYTE()) 
        {
        case CW_OP_CONSTANT:
        {
            cw_value_t constant = _CW_READ_CONSTANT();
            cw_push(vm, constant);
            break;
        }
        case CW_OP_NIL:        cw_push(vm, CW_NIL_VAL); break;
        case CW_OP_TRUE:       cw_push(vm, CW_BOOL_VAL(true)); break;
        case CW_OP_FALSE:      cw_push(vm, CW_BOOL_VAL(false)); break;
        case CW_OP_POP:        cw_pop(vm); break;
        case CW_OP_GET_LOCAL:
        {
            uint8_t slot = _CW_READ_BYTE();
            cw_push(vm, vm->stack[slot]);
            break;
        }
        case CW_OP_SET_LOCAL:
        {
            uint8_t slot = _CW_READ_BYTE();
            frame->slots[slot] = _cw_peek(vm, 0);
            break;
        }
        case CW_OP_GET_GLOBAL:
        {
            cw_obj_string_t* name = _CW_READ_STRING();
            cw_value_t value;
            if (!cw_table_get(&vm->globals, name, &value))
            {
                _cw_runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return CW_INTERPRET_RUNTIME_ERROR;
            }
            cw_push(vm, value);
            break;
        }
        case CW_OP_DEFINE_GLOBAL:
        {
            cw_obj_string_t* name = _CW_READ_STRING();
            cw_table_set(&vm->globals, name, _cw_peek(vm, 0));
            cw_pop(vm);
            break;
        }
        case CW_OP_SET_GLOBAL:
        {
            cw_obj_string_t* name = _CW_READ_STRING();
            if (cw_table_set(&vm->globals, name, _cw_peek(vm, 0)))
            {
                cw_table_delete(&vm->globals, name); 
                _cw_runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return CW_INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case CW_OP_EQUAL:
        {
            cw_value_t b = cw_pop(vm);
            cw_value_t a = cw_pop(vm);
            cw_push(vm, CW_BOOL_VAL(cw_values_equal(a, b)));
            break;
        }
        case CW_OP_GREATER: _CW_BINARY_OP(CW_BOOL_VAL, >); break;  
        case CW_OP_LESS:    _CW_BINARY_OP(CW_BOOL_VAL, <); break; 
        case CW_OP_ADD:
        {
            if (CW_IS_STRING(_cw_peek(vm, 0)) && CW_IS_STRING(_cw_peek(vm, 1)))
            {
                _cw_concatenate(vm);
            }
            else if (CW_IS_NUMBER(_cw_peek(vm, 0)) && CW_IS_NUMBER(_cw_peek(vm, 1)))
            {
                double b = CW_AS_NUMBER(cw_pop(vm));
                double a = CW_AS_NUMBER(cw_pop(vm));
                cw_push(vm, CW_NUMBER_VAL(a + b));
            }
            else
            {
                _cw_runtime_error(vm, "Operands must be two numbers or two strings.");
                return CW_INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case CW_OP_SUBTRACT:   _CW_BINARY_OP(CW_NUMBER_VAL, -); break;
        case CW_OP_MULTIPLY:   _CW_BINARY_OP(CW_NUMBER_VAL, *); break;
        case CW_OP_DIVIDE:     _CW_BINARY_OP(CW_NUMBER_VAL, /); break;
        case CW_OP_NOT:
            cw_push(vm, CW_BOOL_VAL(_cw_is_falsey(cw_pop(vm))));
            break;
        case CW_OP_NEGATE:
            if (!CW_IS_NUMBER(_cw_peek(vm, 0)))
            {
                _cw_runtime_error(vm, "Operand must be a number.");
                return CW_INTERPRET_RUNTIME_ERROR;
            }
            cw_push(vm, CW_NUMBER_VAL(-CW_AS_NUMBER(cw_pop(vm)))); break;
        case CW_OP_PRINT:
            cw_print_value(cw_pop(vm));
            printf("\n");
            break;
        case CW_OP_JUMP:
        {
            uint16_t offset = _CW_READ_SHORT();
            frame->ip += offset;
            break;
        }
        case CW_OP_JUMP_IF_FALSE:
        {
            uint16_t offset = _CW_READ_SHORT();
            if (_cw_is_falsey(_cw_peek(vm, 0)))
                frame->ip += offset;
            break;
        }
        case CW_OP_LOOP:
        {
            uint16_t offset = _CW_READ_SHORT();
            frame->ip -= offset;
            break;
        }
        case CW_OP_CALL:
        {
            int arg_count = _CW_READ_BYTE();
            if (!_cw_call_value(vm, _cw_peek(vm, arg_count), arg_count))
                return CW_INTERPRET_RUNTIME_ERROR;
            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        case CW_OP_CLOSURE:
        {
            cw_obj_function_t* function = CW_AS_FUNCTION(_CW_READ_CONSTANT());
            cw_obj_closure_t* closure = cw_closure_new(vm, function);
            cw_push(vm, CW_OBJ_VAL(closure));
            break;
        }
        case CW_OP_RETURN:
        {
            cw_value_t result = cw_pop(vm);

            vm->frame_count--;
            if (vm->frame_count == 0)
            {
                cw_pop(vm);
                return CW_INTERPRET_OK;
            }

            vm->stack_top = frame->slots;
            cw_push(vm, result);

            frame = &vm->frames[vm->frame_count - 1];
            break;
        }
        }
    }

#undef _CW_READ_BYTE
#undef _CW_READ_SHORT
#undef _CW_READ_CONSTANT
#undef _CW_READ_STRING
#undef _CW_BINARY_OP
}

cw_interpret_result cw_interpret(cw_virtual_machine_t* vm, const char* src)
{
    cw_obj_function_t* function = cw_compile(vm, src);

    if (function == NULL) 
        return CW_INTERPRET_COMPILE_ERROR;

    cw_push(vm, CW_OBJ_VAL(function));

    _cw_call_value(vm, CW_OBJ_VAL(function), 0);

    cw_obj_closure_t* closure = cw_closure_new(vm, function);
    cw_pop(vm);
    cw_push(vm, CW_OBJ_VAL(closure));
    _cw_call_value(vm, CW_OBJ_VAL(closure), 0);

    return _cw_run(vm);
}

#endif // CLOCKWORK_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under the MIT License.
------------------------------------------------------------------------------
Copyright (c) 2019 Oliver Jakobs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
*/