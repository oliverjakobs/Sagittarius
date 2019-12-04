// -----------------------------------------------------------------------------
// ----| Version |--------------------------------------------------------------
// -----------------------------------------------------------------------------

#define VERSION_MAJOR   0
#define VERSION_MINOR   1

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

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#define STACK_MAX       256
#define UINT8_COUNT     (UINT8_MAX + 1)

bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

// -----------------------------------------------------------------------------
// ----| typedefs |-------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct _cw_struct_table         table_t;
typedef struct _cw_struct_table_entry   table_entry_t;

typedef struct _cw_struct_value         value_t;
typedef struct _cw_struct_value_array   value_array_t;

typedef struct _cw_struct_obj           obj_t;
typedef struct _cw_struct_obj_string    obj_string_t;

typedef struct _cw_struct_chunk         chunk_t;

typedef struct _cw_struct_token         token_t;
typedef struct _cw_struct_scanner       scanner_t;

typedef struct _cw_struct_vm            VM;

typedef struct _cw_struct_parser        parser_t;
typedef struct _cw_struct_parse_rule    parse_rule_t;

typedef struct _cw_struct_local         local_t;
typedef struct _cw_struct_compiler      compiler_t;

typedef void (*parse_fn)(VM* vm, parser_t* parser, bool can_assign);

// -----------------------------------------------------------------------------
// ----| forward declarations |-------------------------------------------------
// -----------------------------------------------------------------------------

void vm_objects_set(VM* vm, obj_t* objects);
obj_t* vm_objects_get(VM* vm);
table_t* vm_strings_get(VM* vm);

bool table_set(table_t* table, obj_string_t* key, value_t value);
obj_string_t* table_find_string(table_t* table, const char* chars, int length, uint32_t hash);

// -----------------------------------------------------------------------------
// ----| Memory |---------------------------------------------------------------
// -----------------------------------------------------------------------------

// calculates a new capacity based on a given current capacity. It scales based 
// on the old size and grows by a factor of two. If the current capacity is zero
// we jump straight to eight elements instead of starting at one.
#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

// wrapper for reallocate
#define ALLOCATE(type, count)   (type*)reallocate(NULL, 0, sizeof(type) * (count))
#define FREE(type, pointer)     reallocate(pointer, sizeof(type), 0)

#define GROW_ARRAY(previous, type, oldCount, count) (type*)reallocate(previous, sizeof(type) * (oldCount), sizeof(type) * (count))
#define FREE_ARRAY(type, pointer, oldCount) reallocate(pointer, sizeof(type) * (oldCount), 0)

// The single function for all dynamic memory management
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

typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} value_type;

struct _cw_struct_value
{
    value_type type;
    union
    {
        bool boolean;
        double number;
        obj_t* obj;
    } as;
};

struct _cw_struct_value_array
{
    int capacity;
    int count;
    value_t* values;
};

#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
#define AS_OBJ(value)       ((value).as.obj)

#define BOOL_VAL(value)     ((value_t) { VAL_BOOL, { .boolean = value }})
#define NIL_VAL             ((value_t) { VAL_NIL, { .number = 0 }})
#define NUMBER_VAL(value)   ((value_t) { VAL_NUMBER, { .number = value }})
#define OBJ_VAL(value)      ((value_t) { VAL_OBJ, { .obj = (obj_t*)value }})

void value_array_init(value_array_t* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void value_array_free(value_array_t* array)
{
    FREE_ARRAY(value_t, array->values, array->capacity);
    value_array_init(array);
}

void value_array_write(value_array_t* array, value_t v)
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

// -----------------------------------------------------------------------------
// ----| Object |---------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    OBJ_STRING,
} obj_type;

struct _cw_struct_obj
{
    obj_type type;
    obj_t* next;
};

struct _cw_struct_obj_string
{
    obj_t obj;
    int length;
    char* chars;
    uint32_t hash;
};

#define ALLOCATE_OBJ(vm, type, object_type) (type*)_obj_allocate(vm, sizeof(type), object_type)

obj_t* _obj_get_next(obj_t* object)
{
    return object->next;
}

obj_t* _obj_allocate(VM* vm, size_t size, obj_type type)
{
    obj_t* object = (obj_t*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm_objects_get(vm);
    vm_objects_set(vm, object);

    return object;
}

void _obj_free(obj_t* object)
{
    switch (object->type)
    {
    case OBJ_STRING: 
    {
        obj_string_t* string = (obj_string_t*)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(obj_string_t, object);
        break;
    }
    }
}

// FNV-1a hash function
uint32_t _obj_string_hash(const char* key, int length)
{
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++)
    {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

obj_string_t* _obj_allocate_string(VM* vm, char* chars, int length, uint32_t hash)
{
    obj_string_t* string = ALLOCATE_OBJ(vm, obj_string_t, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    table_set(vm_strings_get(vm), string, NIL_VAL);
    return string;
}

obj_string_t* obj_string_move(VM* vm, char* chars, int length)
{
    uint32_t hash = _obj_string_hash(chars, length);
    obj_string_t* interned = table_find_string(vm_strings_get(vm), chars, length, hash);
    if (interned != NULL)
    {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return _obj_allocate_string(vm, chars, length, hash);
}

obj_string_t* obj_string_copy(VM* vm, const char* chars, int length) 
{
    uint32_t hash = _obj_string_hash(chars, length);
    obj_string_t* interned = table_find_string(vm_strings_get(vm), chars, length, hash);

    if (interned != NULL) return interned;

    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return _obj_allocate_string(vm, heap_chars, length, hash);
}

bool obj_is_type(value_t value, obj_type type) { return IS_OBJ(value) && AS_OBJ(value)->type == type; }

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    obj_is_type(value, OBJ_STRING)

#define AS_STRING(value)    ((obj_string_t*)AS_OBJ(value))
#define AS_CSTRING(value)   (((obj_string_t*)AS_OBJ(value))->chars)

bool is_falsey(value_t v) { return IS_NIL(v) || (IS_BOOL(v) && !AS_BOOL(v)); }

bool values_equal(value_t a, value_t b)
{
    if (a.type != b.type) return false;

    switch (a.type)
    {
    case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:       return true;
    case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:       return AS_OBJ(a) == AS_OBJ(b);
    }

    return false;
}

void print_obj(value_t value)
{
    switch (OBJ_TYPE(value))
    {
    case OBJ_STRING:
        printf("%s", AS_CSTRING(value));
        break;
    }
}

void print_value(value_t value)
{
    switch (value.type)
    {
    case VAL_BOOL:      printf(AS_BOOL(value) ? "true" : "false"); break;
    case VAL_NIL:       printf("nil"); break;
    case VAL_NUMBER:    printf("%g", AS_NUMBER(value)); break;
    case VAL_OBJ:       print_obj(value); break;
    }
}

// -----------------------------------------------------------------------------
// ----| Table |----------------------------------------------------------------
// -----------------------------------------------------------------------------

struct _cw_struct_table_entry
{
    obj_string_t* key;
    value_t value;
};

struct _cw_struct_table
{
    int count;
    int capacity;
    table_entry_t* entries;
};

#define TABLE_MAX_LOAD 0.75

void table_init(table_t* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void table_free(table_t* table)
{
    FREE_ARRAY(table_entry_t, table->entries, table->capacity);
    table_init(table);
}

table_entry_t* _entry_find(table_entry_t* entries, int capacity, obj_string_t* key)
{
    uint32_t index = key->hash % capacity;
    table_entry_t* tombstone = NULL;

    while (true)
    {
        table_entry_t* entry = &entries[index];
        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value)) // Empty entry
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

void _table_adjust_capacity(table_t* table, int capacity)
{
    table_entry_t* entries = ALLOCATE(table_entry_t, capacity);
    for (int i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
        entries[i].value = NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++)
    {
        table_entry_t* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        table_entry_t* dest = _entry_find(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(table_entry_t, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool table_set(table_t* table, obj_string_t* key, value_t value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        _table_adjust_capacity(table, capacity);
    }

    table_entry_t* entry = _entry_find(table->entries, table->capacity, key);

    bool is_new_key = entry->key == NULL;
    if (is_new_key && IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

void table_copy(table_t* from, table_t* to)
{
    for (int i = 0; i < from->capacity; i++)
    {
        table_entry_t* entry = &from->entries[i];
        if (entry->key != NULL)
            table_set(to, entry->key, entry->value);
    }
}

obj_string_t* table_find_string(table_t* table, const char* chars, int length, uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;

    while (true)
    {
        table_entry_t* entry = &table->entries[index];

        if (entry->key == NULL)
        {
            if (IS_NIL(entry->value)) return NULL; // Stop if we find an empty non-tombstone entry.
        }
        else if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->chars, chars, length) == 0)
        {
            return entry->key; // We found it.
        }

        index = (index + 1) % table->capacity;
    }
}

bool table_get(table_t* table, obj_string_t* key, value_t* value)
{
    if (table->count == 0) return false;

    table_entry_t* entry = _entry_find(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *value = entry->value;
    return true;
}

bool table_delete(table_t* table, obj_string_t* key)
{
    if (table->count == 0) return false;

    // Find the entry.
    table_entry_t* entry = _entry_find(table->entries, table->capacity, key);
     if (entry->key == NULL) return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->value = BOOL_VAL(true);

    return true;
}

// -----------------------------------------------------------------------------
// ----| Chunk |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_RETURN,
} op_code;

struct _cw_struct_chunk
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    value_array_t constants;
};

void chunk_init(chunk_t* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    value_array_init(&chunk->constants);
}

void chunk_free(chunk_t* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    value_array_free(&chunk->constants);
    chunk_init(chunk);
}

void chunk_write(chunk_t* chunk, uint8_t byte, int line)
{
    if (chunk->capacity < chunk->count + 1)
    {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(chunk->lines, int, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int chunk_add_constant(chunk_t* chunk, value_t value)
{
    value_array_write(&chunk->constants, value);
    return chunk->constants.count - 1;
}

// -----------------------------------------------------------------------------
// ----| Debug |----------------------------------------------------------------
// -----------------------------------------------------------------------------
int _simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int _constant_instruction(const char* name, chunk_t* chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    print_value(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

int _byte_instruction(const char* name, chunk_t* chunk, int offset)
{
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

int disassemble_instruction(chunk_t* chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
        printf("   | ");
    else
        printf("%4d ", chunk->lines[offset]);

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:       return _constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:            return _simple_instruction("OP_NIL", offset);
    case OP_TRUE:           return _simple_instruction("OP_TRUE", offset);
    case OP_FALSE:          return _simple_instruction("OP_FALSE", offset);
    case OP_POP:            return _simple_instruction("OP_POP", offset);
    case OP_GET_LOCAL:      return _byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:      return _byte_instruction("OP_SET_LOCAL", chunk, offset);
    case OP_GET_GLOBAL:     return _constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_DEFINE_GLOBAL:  return _constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:     return _constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_EQUAL:          return _simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:        return _simple_instruction("OP_GREATER", offset);
    case OP_LESS:           return _simple_instruction("OP_LESS", offset);
    case OP_ADD:            return _simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:       return _simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:       return _simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:         return _simple_instruction("OP_DIVIDE", offset);
    case OP_NOT:            return _simple_instruction("OP_NOT", offset);
    case OP_NEGATE:         return _simple_instruction("OP_NEGATE", offset);
    case OP_PRINT:          return _simple_instruction("OP_PRINT", offset); 
    case OP_RETURN:         return _simple_instruction("OP_RETURN", offset);
    default:    
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void disassemble_chunk(chunk_t* chunk, const char* name) 
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;)
        offset = disassemble_instruction(chunk, offset);
}

// -----------------------------------------------------------------------------
// ----| scanner |--------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,
    // Specials.
    TOKEN_ERROR,
    TOKEN_EOF
} token_type;

struct _cw_struct_token
{
    token_type type;
    const char* start;
    int length;
    int line;
};

struct _cw_struct_scanner
{
  const char* start;
  const char* current;
  int line;
};

scanner_t scanner;

void scanner_init(const char* src)
{
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
}

bool scanner_end()
{
    return *scanner.current == '\0';
}

char scanner_advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// returns the current character, but doesn’t consume it
char scanner_peek()
{
    return *scanner.current;
}

// like peek() but for one character past the current one
char scanner_peek_next()
{
    if (scanner_end()) return '\0';
    return scanner.current[1]; 
}

bool scanner_match(char expected)
{
    if (scanner_end()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

token_type scanner_check_keyword(int start, int length, const char* rest, token_type type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
        return type;

    return TOKEN_IDENTIFIER;
}

void scanner_skip_whitespace()
{
    while(true)
    {
        char c = scanner_peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            scanner_advance();
            break;
        case '\n':
            scanner.line++;
            scanner_advance();
            break;
        case '/':
            if (scanner_peek_next() == '/')
                while (scanner_peek() != '\n' && !scanner_end()) scanner_advance();
            else
                return;
        default: return;
        }
    }
}

token_type scanner_identifier_type()
{
    switch (scanner.start[0])
    {
    case 'a': return scanner_check_keyword(1, 2, "nd", TOKEN_AND);
    case 'c': return scanner_check_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return scanner_check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) 
            switch (scanner.start[1])
            {
            case 'a': return scanner_check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return scanner_check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u': return scanner_check_keyword(2, 1, "n", TOKEN_FUN);
            }
        break;
    case 'i': return scanner_check_keyword(1, 1, "f", TOKEN_IF);
    case 'n': return scanner_check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o': return scanner_check_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return scanner_check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return scanner_check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return scanner_check_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1)
            switch (scanner.start[1])
            {
            case 'h': return scanner_check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r': return scanner_check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
        break; 
    case 'v': return scanner_check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return scanner_check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

// token creation
token_t token_make(token_type type)
{
    token_t token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

// creates a error token with an error message
token_t token_make_error(const char* message)
{
    token_t token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

token_t token_make_string()
{
    while (scanner_peek() != '"' && !scanner_end())
    {
        if (scanner_peek() == '\n') scanner.line++;
        scanner_advance();
    }

    if (scanner_end()) return token_make_error("Unterminated string.");

    // The closing quote.
    scanner_advance();
    return token_make(TOKEN_STRING); 
}

token_t token_make_number()
{
    while (is_digit(scanner_peek())) scanner_advance();

    // Look for a fractional part.
    if (scanner_peek() == '.' && is_digit(scanner_peek_next()))
    {
        // Consume the ".".
        scanner_advance();
        while (is_digit(scanner_peek())) scanner_advance();
    }

    return token_make(TOKEN_NUMBER);
}

token_t token_make_identifier()
{
    while (is_alpha(scanner_peek()) || is_digit(scanner_peek())) scanner_advance();
    return token_make(scanner_identifier_type());
}

token_t token_scan()
{
    scanner_skip_whitespace();
    scanner.start = scanner.current;
    if (scanner_end()) return token_make(TOKEN_EOF);

    char c = scanner_advance();

    // identifiers and keywords
    if (is_alpha(c)) return token_make_identifier();
    if (is_digit(c)) return token_make_number();

    switch (c)
    {
    // single character tokens
    case '(': return token_make(TOKEN_LEFT_PAREN);
    case ')': return token_make(TOKEN_RIGHT_PAREN);
    case '{': return token_make(TOKEN_LEFT_BRACE);
    case '}': return token_make(TOKEN_RIGHT_BRACE);
    case ';': return token_make(TOKEN_SEMICOLON);
    case ',': return token_make(TOKEN_COMMA);
    case '.': return token_make(TOKEN_DOT);
    case '-': return token_make(TOKEN_MINUS);
    case '+': return token_make(TOKEN_PLUS);
    case '/': return token_make(TOKEN_SLASH);
    case '*': return token_make(TOKEN_STAR);
    // two-character punctuation tokens
    case '!': return token_make(scanner_match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);  
    case '=': return token_make(scanner_match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return token_make(scanner_match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);  
    case '>': return token_make(scanner_match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    // literal tokens
    case '"': return token_make_string();
    }

    return token_make_error("Unexpected character.");
}

// -----------------------------------------------------------------------------
// ----| compiler |-------------------------------------------------------------
// -----------------------------------------------------------------------------

struct _cw_struct_parser
{
    token_t current;
    token_t previous;
    bool had_error;
    bool panic_mode;
};

typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_OR,            // or
    PREC_AND,           // and
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! -
    PREC_CALL,          // . ()
    PREC_PRIMARY
} precedence;

struct _cw_struct_parse_rule
{
    parse_fn prefix;
    parse_fn infix;
    precedence precedence;
};

struct _cw_struct_local
{
    token_t name;
    int depth;
};

struct _cw_struct_compiler
{
    local_t locals[UINT8_COUNT];
    int local_count;
    int scope_depth;
};

parse_rule_t* parser_get_rule(token_type type);

compiler_t* current_compiler = NULL;
chunk_t* compiling_chunk;

chunk_t* current_chunk()
{
    return compiling_chunk;
}

void compiler_init(compiler_t* compiler)
{
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    current_compiler = compiler;
}

void parser_error_at(parser_t* parser, token_t* token, const char* message)
{
    if (parser->panic_mode) return;
    parser->panic_mode = true;

    fprintf(stderr, "[line %d] Error", token->line);

    if (token->type == TOKEN_EOF) 
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
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

void parser_error_at_current(parser_t* parser, const char* message)
{
    parser_error_at(parser, &parser->current, message);
}

void parser_error(parser_t* parser, const char* message)
{
    parser_error_at(parser, &parser->previous, message);
}

void local_add(parser_t* parser, token_t name)
{
    if (current_compiler->local_count == UINT8_COUNT)
    {
        parser_error(parser, "Too many local variables in function.");
        return;
    }

    local_t* local = &current_compiler->locals[current_compiler->local_count++];
    local->name = name;
    local->depth = -1; 
}

bool _parser_identifiers_equal(token_t* a, token_t* b)
{
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

int _parser_resolve_local(parser_t* parser, compiler_t* compiler, token_t* name)
{
    for (int i = compiler->local_count - 1; i >= 0; i--)
    {
        local_t* local = &compiler->locals[i];
        if (_parser_identifiers_equal(name, &local->name))
        {
            if (local->depth == -1)
                parser_error(parser, "Cannot read local variable in its own initializer.");

            return i;
        }
    }

    return -1;
}

void parser_advance(parser_t* parser)
{
    parser->previous = parser->current;

    while (true)
    {
        parser->current = token_scan();
        if (parser->current.type != TOKEN_ERROR) break;

        parser_error_at_current(parser, parser->current.start);
    }
}

void parser_consume(parser_t* parser, token_type type, const char* message)
{
    if (parser->current.type == type)
    {
        parser_advance(parser);
        return;
    }
    parser_error_at_current(parser, message); 
}

bool parser_check(parser_t* parser, token_type type)
{
    return parser->current.type == type;
}

bool parser_match(parser_t* parser, token_type type)
{
    if (!parser_check(parser, type)) return false;

    parser_advance(parser);

    return true;
}

void parser_emit_byte(parser_t* parser, uint8_t byte)
{
    chunk_write(current_chunk(), byte, parser->previous.line);
}

void parser_emit_return(parser_t* parser)
{
    parser_emit_byte(parser, OP_RETURN);
}

// convenience function to write an opcode followed by a one-byte operand
void parser_emit_bytes(parser_t* parser, uint8_t byte1, uint8_t byte2)
{
    parser_emit_byte(parser, byte1);
    parser_emit_byte(parser, byte2);
}

void _parser_mark_initialized()
{
    current_compiler->locals[current_compiler->local_count - 1].depth = current_compiler->scope_depth;
}

void parser_define_variable(parser_t* parser, uint8_t global)
{
    if (current_compiler->scope_depth > 0)
    {
        _parser_mark_initialized();
        return;
    }

    parser_emit_bytes(parser, OP_DEFINE_GLOBAL, global);
}

void parser_declare_variable(parser_t* parser)
{
    // Global variables are implicitly declared.
    if (current_compiler->scope_depth == 0) return;

    token_t* name = &parser->previous;
    for (int i = current_compiler->local_count - 1; i >= 0; i--)
    {
        local_t* local = &current_compiler->locals[i];
        if (local->depth != -1 && local->depth < current_compiler->scope_depth)
            break;

        if (_parser_identifiers_equal(name, &local->name))
            parser_error(parser, "Variable with this name already declared in this scope.");
    }

    local_add(parser, *name);
}

uint8_t parser_make_constant(parser_t* parser, value_t value)
{
    int constant = chunk_add_constant(current_chunk(), value);
    if (constant > UINT8_MAX)
    {
        parser_error(parser, "Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
}

uint8_t _parser_make_constant_indentifier(VM* vm, parser_t* parser, token_t* name)
{
    return parser_make_constant(parser, OBJ_VAL(obj_string_copy(vm, name->start, name->length)));
}

uint8_t parser_make_variable(VM* vm, parser_t* parser, const char* error_message)
{
    parser_consume(parser, TOKEN_IDENTIFIER, error_message);

    parser_declare_variable(parser);
    if (current_compiler->scope_depth > 0) return 0;

    return _parser_make_constant_indentifier(vm, parser, &parser->previous);
}

void parser_emit_constant(parser_t* parser, value_t value)
{
    parser_emit_bytes(parser, OP_CONSTANT, parser_make_constant(parser, value));
}

static void end_compiler(parser_t* parser)
{
    parser_emit_return(parser);
#ifdef DEBUG_PRINT_CODE
    if (!parser->had_error)
        disassemble_chunk(current_chunk(), "code");
#endif
}

void parser_synchronize(parser_t* parser)
{
    parser->panic_mode = false;
    while (parser->current.type != TOKEN_EOF)
    {
        if (parser->previous.type == TOKEN_SEMICOLON) return;

        switch (parser->current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;
        default:
            break; // Do nothing.
        }
        parser_advance(parser);
    }
}

// -----------------------------------------------------------------------------
void parse_precedence(VM* vm, parser_t* parser, precedence pre)
{
    parser_advance(parser);

    parse_fn prefix_rule = parser_get_rule(parser->previous.type)->prefix;

    if (prefix_rule == NULL)
    {
        parser_error(parser, "Expect expression.");
        return;
    }

    bool can_assign = pre <= PREC_ASSIGNMENT;
    prefix_rule(vm, parser, can_assign);

    while (pre <= parser_get_rule(parser->current.type)->precedence)
    {
        parser_advance(parser);
        parse_fn infix_rule = parser_get_rule(parser->previous.type)->infix;
        infix_rule(vm, parser, can_assign);
    }

    if (can_assign && parser_match(parser, TOKEN_EQUAL))
        parser_error(parser, "Invalid assignment target.");
}

void parse_expression(VM* vm, parser_t* parser)
{
    parse_precedence(vm, parser, PREC_ASSIGNMENT);
}

void parse_number(VM* vm, parser_t* parser, bool can_assign)
{
    double value = strtod(parser->previous.start, NULL);
    parser_emit_constant(parser, NUMBER_VAL(value));
}

void parse_string(VM* vm, parser_t* parser, bool can_assign)
{
    parser_emit_constant(parser, OBJ_VAL(obj_string_copy(vm, parser->previous.start + 1, parser->previous.length - 2)));
}

void parse_variable_named(VM* vm, parser_t* parser, token_t name, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = _parser_resolve_local(parser, current_compiler, &name);
    if (arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else
    {
        arg = _parser_make_constant_indentifier(vm, parser, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    if (can_assign && parser_match(parser, TOKEN_EQUAL))
    {
        parse_expression(vm, parser);
        parser_emit_bytes(parser, set_op, (uint8_t)arg);
    }
    else
    {
        parser_emit_bytes(parser, get_op, (uint8_t)arg);
    }
}

void parse_variable(VM* vm, parser_t* parser, bool can_assign)
{
    parse_variable_named(vm, parser, parser->previous, can_assign);
}

// statements
void parse_statement(VM* vm, parser_t* parser);

void parse_statement_print(VM* vm, parser_t* parser)
{
    parse_expression(vm, parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after value.");
    parser_emit_byte(parser, OP_PRINT);
}

void parse_statement_expression(VM* vm, parser_t* parser)
{
    parse_expression(vm, parser);
    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    parser_emit_byte(parser, OP_POP);
}

void parse_declaration_var(VM* vm, parser_t* parser)
{
    uint8_t global = parser_make_variable(vm, parser, "Expect variable name.");

    if (parser_match(parser, TOKEN_EQUAL))
        parse_expression(vm, parser);
    else
        parser_emit_byte(parser, OP_NIL);

    parser_consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    parser_define_variable(parser, global);
}

void parse_declaration(VM* vm, parser_t* parser)
{
    if (parser_match(parser, TOKEN_VAR))
        parse_declaration_var(vm, parser);
    else
        parse_statement(vm, parser);

    if (parser->panic_mode) parser_synchronize(parser);
}

void block(VM* vm, parser_t* parser)
{
    while (!parser_check(parser, TOKEN_RIGHT_BRACE) && !parser_check(parser, TOKEN_EOF))
    {
        parse_declaration(vm, parser);
    }

    parser_consume(parser, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void begin_scope(parser_t* parser) { current_compiler->scope_depth++; }

static void end_scope(parser_t* parser)
{
    current_compiler->scope_depth--;

    while (current_compiler->local_count > 0 && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth)
    {
        parser_emit_byte(parser, OP_POP);
        current_compiler->local_count--;
    }
}

void parse_statement(VM* vm, parser_t* parser)
{
    if (parser_match(parser, TOKEN_PRINT))
    {
        parse_statement_print(vm, parser);
    } 
    else if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        begin_scope(parser);
        block(vm, parser);
        end_scope(parser);
    }
    else
    {
        parse_statement_expression(vm, parser);
    }
}

void parse_grouping(VM* vm, parser_t* parser, bool can_assign)
{
    parse_expression(vm, parser);
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void parse_unary(VM* vm, parser_t* parser, bool can_assign)
{
    token_type operatorType = parser->previous.type;

    // Compile the operand.
    parse_precedence(vm, parser, PREC_UNARY);

    // Emit the operator instruction.
    switch (operatorType)
    {
    case TOKEN_BANG:    parser_emit_byte(parser, OP_NOT); break;
    case TOKEN_MINUS:   parser_emit_byte(parser, OP_NEGATE); break;
    default: return; // Unreachable.
    }
}

void parse_binary(VM* vm, parser_t* parser, bool can_assign)
{
    // Remember the operator.
    token_type operatorType = parser->previous.type;

    // Compile the right operand.
    parse_rule_t* rule = parser_get_rule(operatorType);
    parse_precedence(vm, parser, (precedence)(rule->precedence + 1));

    // Emit the operator instruction.
    switch (operatorType)
    {
    case TOKEN_BANG_EQUAL:      parser_emit_bytes(parser, OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:     parser_emit_byte(parser, OP_EQUAL); break;
    case TOKEN_GREATER:         parser_emit_byte(parser, OP_GREATER); break;
    case TOKEN_GREATER_EQUAL:   parser_emit_bytes(parser, OP_LESS, OP_NOT); break;
    case TOKEN_LESS:            parser_emit_byte(parser, OP_LESS); break;
    case TOKEN_LESS_EQUAL:      parser_emit_bytes(parser, OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:            parser_emit_byte(parser, OP_ADD); break;
    case TOKEN_MINUS:           parser_emit_byte(parser, OP_SUBTRACT); break;
    case TOKEN_STAR:            parser_emit_byte(parser, OP_MULTIPLY); break;
    case TOKEN_SLASH:           parser_emit_byte(parser, OP_DIVIDE); break;
    default: return; // Unreachable.
    }
}

void parse_literal(VM* vm, parser_t* parser, bool can_assign)
{
    switch (parser->previous.type) 
    {
    case TOKEN_FALSE:   parser_emit_byte(parser, OP_FALSE); break;
    case TOKEN_NIL:     parser_emit_byte(parser, OP_NIL); break;
    case TOKEN_TRUE:    parser_emit_byte(parser, OP_TRUE); break;
    default: return; // Unreachable.
    }
}

parse_rule_t rules[] =
{
    { parse_grouping,   NULL,           PREC_NONE },        // TOKEN_LEFT_PAREN
    { NULL,             NULL,           PREC_NONE },        // TOKEN_RIGHT_PAREN
    { NULL,             NULL,           PREC_NONE },        // TOKEN_LEFT_BRACE
    { NULL,             NULL,           PREC_NONE },        // TOKEN_RIGHT_BRACE
    { NULL,             NULL,           PREC_NONE },        // TOKEN_COMMA
    { NULL,             NULL,           PREC_NONE },        // TOKEN_DOT
    { parse_unary,      parse_binary,   PREC_TERM },        // TOKEN_MINUS
    { NULL,             parse_binary,   PREC_TERM },        // TOKEN_PLUS
    { NULL,             NULL,           PREC_NONE },        // TOKEN_SEMICOLON
    { NULL,             parse_binary,   PREC_FACTOR },      // TOKEN_SLASH
    { NULL,             parse_binary,   PREC_FACTOR },      // TOKEN_STAR
    { parse_unary,      NULL,           PREC_NONE },        // TOKEN_BANG
    { NULL,             parse_binary,   PREC_EQUALITY },    // TOKEN_BANG_EQUAL
    { NULL,             NULL,           PREC_NONE },        // TOKEN_EQUAL
    { NULL,             parse_binary,   PREC_EQUALITY },    // TOKEN_EQUAL_EQUAL
    { NULL,             parse_binary,   PREC_COMPARISON },  // TOKEN_GREATER
    { NULL,             parse_binary,   PREC_COMPARISON },  // TOKEN_GREATER_EQUAL
    { NULL,             parse_binary,   PREC_COMPARISON },  // TOKEN_LESS
    { NULL,             parse_binary,   PREC_COMPARISON },  // TOKEN_LESS_EQUAL
    { parse_variable,   NULL,           PREC_NONE },        // TOKEN_IDENTIFIER
    { parse_string,     NULL,           PREC_NONE },        // TOKEN_STRING
    { parse_number,     NULL,           PREC_NONE },        // TOKEN_NUMBER
    { NULL,             NULL,           PREC_NONE },        // TOKEN_AND
    { NULL,             NULL,           PREC_NONE },        // TOKEN_CLASS
    { NULL,             NULL,           PREC_NONE },        // TOKEN_ELSE
    { parse_literal,    NULL,           PREC_NONE },        // TOKEN_FALSE
    { NULL,             NULL,           PREC_NONE },        // TOKEN_FOR
    { NULL,             NULL,           PREC_NONE },        // TOKEN_FUN
    { NULL,             NULL,           PREC_NONE },        // TOKEN_IF
    { parse_literal,    NULL,           PREC_NONE },        // TOKEN_NIL
    { NULL,             NULL,           PREC_NONE },        // TOKEN_OR
    { NULL,             NULL,           PREC_NONE },        // TOKEN_PRINT
    { NULL,             NULL,           PREC_NONE },        // TOKEN_RETURN
    { NULL,             NULL,           PREC_NONE },        // TOKEN_SUPER
    { NULL,             NULL,           PREC_NONE },        // TOKEN_THIS
    { parse_literal,    NULL,           PREC_NONE },        // TOKEN_TRUE
    { NULL,             NULL,           PREC_NONE },        // TOKEN_VAR
    { NULL,             NULL,           PREC_NONE },        // TOKEN_WHILE
    { NULL,             NULL,           PREC_NONE },        // TOKEN_ERROR
    { NULL,             NULL,           PREC_NONE },        // TOKEN_EOF
};

parse_rule_t* parser_get_rule(token_type type)
{
    return &rules[type];
}

bool compile(VM* vm, const char* src, chunk_t* chunk)
{
    scanner_init(src);
    compiling_chunk = chunk;

    parser_t parser;
    parser.had_error = false;
    parser.panic_mode = false;

    parser_advance(&parser);

    while(!parser_match(&parser, TOKEN_EOF))
    {
        parse_declaration(vm, &parser);
    }

    end_compiler(&parser);

    return !parser.had_error;
}

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

struct _cw_struct_vm
{
    chunk_t* chunk;
    uint8_t* ip;
    value_t stack[STACK_MAX];
    value_t* stack_top;

    table_t globals;
    table_t strings;
    obj_t* objects;
};

void vm_objects_free(VM* vm)
{
    obj_t* object = vm->objects;
    while (object != NULL)
    {
        obj_t* next = object->next;
        _obj_free(object);
        object = next;
    }
}

void vm_objects_set(VM* vm, obj_t* objects)
{
    vm->objects = objects;
}

obj_t* vm_objects_get(VM* vm)
{
    return vm->objects;
}

table_t* vm_strings_get(VM* vm)
{
    return &vm->strings;
}

typedef enum
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} interpret_result;

static void vm_reset_stack(VM* vm)
{
    vm->stack_top = vm->stack;
}

void vm_runtime_error(VM* vm, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm->ip - vm->chunk->code;
    int line = vm->chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);

    vm_reset_stack(vm);
}

void vm_init(VM* vm)
{
    vm_reset_stack(vm);
    vm->objects = NULL;

    table_init(&vm->globals);
    table_init(&vm->strings);
}

void vm_free(VM* vm)
{
    table_free(&vm->globals);
    table_free(&vm->strings);
    vm_objects_free(vm);
}

void vm_push(VM* vm, value_t v)
{
    *vm->stack_top = v;
    vm->stack_top++;
}

value_t vm_pop(VM* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(VM* vm, int distance)
{
    return vm->stack_top[-1 - distance];
}

void vm_concatenate(VM* vm)
{
    obj_string_t* b = AS_STRING(vm_pop(vm));
    obj_string_t* a = AS_STRING(vm_pop(vm));

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    obj_string_t* result = obj_string_move(vm, chars, length);
    vm_push(vm, OBJ_VAL(result));
}

static interpret_result vm_run(VM* vm)
{
#define READ_BYTE() (*vm->ip++)
#define READ_CONSTANT() (vm->chunk->constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(vm, type, op)                                     \
    do                                                              \
    {                                                               \
      if (!IS_NUMBER(vm_peek(vm, 0)) || !IS_NUMBER(vm_peek(vm, 1))) \
      {                                                             \
        vm_runtime_error(vm, "Operands must be numbers.");          \
        return INTERPRET_RUNTIME_ERROR;                             \
      }                                                             \
      double b = AS_NUMBER(vm_pop(vm));                             \
      double a = AS_NUMBER(vm_pop(vm));                             \
      vm_push(vm, type(a op b));                                    \
    } while (false)

    while(true)
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
            vm_push(vm, constant);
            break;
        }
        case OP_NIL:        vm_push(vm, NIL_VAL); break;
        case OP_TRUE:       vm_push(vm, BOOL_VAL(true)); break;
        case OP_FALSE:      vm_push(vm, BOOL_VAL(false)); break;
        case OP_POP:        vm_pop(vm); break;
        case OP_GET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            vm_push(vm, vm->stack[slot]);
            break;
        }
        case OP_SET_LOCAL:
        {
            uint8_t slot = READ_BYTE();
            vm->stack[slot] = vm_peek(vm, 0);
            break;
        }
        case OP_GET_GLOBAL:
        {
            obj_string_t* name = READ_STRING();
            value_t value;
            if (!table_get(&vm->globals, name, &value))
            {
                vm_runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            vm_push(vm, value);
            break;
        }
        case OP_DEFINE_GLOBAL:
        {
            obj_string_t* name = READ_STRING();
            table_set(&vm->globals, name, vm_peek(vm, 0));
            vm_pop(vm);
            break;
        }
        case OP_SET_GLOBAL:
        {
            obj_string_t* name = READ_STRING();
            if (table_set(&vm->globals, name, vm_peek(vm, 0)))
            {
                table_delete(&vm->globals, name); 
                vm_runtime_error(vm, "Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_EQUAL:
        {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            vm_push(vm, BOOL_VAL(values_equal(a, b)));
            break;
        }
        case OP_GREATER:    BINARY_OP(vm, BOOL_VAL, >); break;  
        case OP_LESS:       BINARY_OP(vm, BOOL_VAL, <); break; 
        case OP_ADD:
        {
            if (IS_STRING(vm_peek(vm, 0)) && IS_STRING(vm_peek(vm, 1)))
            {
                vm_concatenate(vm);
            }
            else if (IS_NUMBER(vm_peek(vm, 0)) && IS_NUMBER(vm_peek(vm, 1)))
            {
                double b = AS_NUMBER(vm_pop(vm));
                double a = AS_NUMBER(vm_pop(vm));
                vm_push(vm, NUMBER_VAL(a + b));
            }
            else
            {
                vm_runtime_error(vm, "Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
            break;
        }
        case OP_SUBTRACT:   BINARY_OP(vm, NUMBER_VAL, -); break;
        case OP_MULTIPLY:   BINARY_OP(vm, NUMBER_VAL, *); break;
        case OP_DIVIDE:     BINARY_OP(vm, NUMBER_VAL, /); break;
        case OP_NOT:
            vm_push(vm, BOOL_VAL(is_falsey(vm_pop(vm))));
            break;
        case OP_NEGATE:
            if (!IS_NUMBER(vm_peek(vm, 0)))
            {
                vm_runtime_error(vm, "Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            vm_push(vm, NUMBER_VAL(-AS_NUMBER(vm_pop(vm)))); break;
        case OP_PRINT:
            print_value(vm_pop(vm));
            printf("\n");
            break;
        case OP_RETURN:
            // Exit interpreter.
            return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

interpret_result vm_interpret(VM* vm, const char* src)
{
    chunk_t chunk;
    chunk_init(&chunk);

    if (!compile(vm, src, &chunk))
    {
        chunk_free(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm->chunk = &chunk;
    vm->ip = vm->chunk->code;

    interpret_result result = vm_run(vm);

    chunk_free(&chunk);
    return result;
}

// -----------------------------------------------------------------------------
// ----| main |-----------------------------------------------------------------
// -----------------------------------------------------------------------------
static void repl(VM* vm)
{
    printf("Clockwork v%d.%d\n", VERSION_MAJOR, VERSION_MINOR);

    char line[1024];

    while (true)
    {
        printf(">>> ");

        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        vm_interpret(vm, line);
    }
}

static char* read_file(const char* path)
{
    FILE* file = fopen(path, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);

    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);

    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static void run_file(VM* vm, const char* path)
{
    char* source = read_file(path);
    interpret_result result = vm_interpret(vm, source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) 
{
    VM vm;
    vm_init(&vm);

    if (argc == 1)
    {
        repl(&vm);
    }
    else if (argc == 2)
    {
        run_file(&vm, argv[1]);
    }
    else
    {
        fprintf(stderr, "Usage: cw [path]\n");
    }

    vm_free(&vm);
    return 0;
}