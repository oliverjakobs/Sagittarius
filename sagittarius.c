
// -----------------------------------------------------------------------------
// ----| Common |---------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#define STACK_MAX 256

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// -----------------------------------------------------------------------------
// ----| typedefs |-------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct _value value_t;
typedef struct _value_array value_array_t;

typedef struct _obj obj_t;
typedef struct _obj_string obj_string_t;

typedef struct _chunk chunk_t;

typedef struct _token token_t;
typedef struct _scanner scanner_t;

typedef struct _virtual_machine VM;

typedef struct _parser parser_t;
typedef struct _parse_rule parse_rule_t;

// forward declarations
void _obj_free(obj_t* object);
obj_t* _obj_get_next(obj_t* object);

void _vm_set_objects(VM* vm, obj_t* objects);
obj_t* _vm_get_objects(VM* vm);

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

void free_objects(VM* vm)
{
    obj_t* object = _vm_get_objects(vm);
    while (object != NULL)
    { 
        obj_t* next = _obj_get_next(object);
        _obj_free(object);
        object = next;
    }
}

// -----------------------------------------------------------------------------
// ----| Value |----------------------------------------------------------------
// -----------------------------------------------------------------------------

// value_t
typedef enum
{
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
} value_type;

struct _value
{
    value_type type;
    union
    {
        bool boolean;
        double number;
        obj_t* obj;
    } as;
};

// value_array_t
struct _value_array
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

struct _obj
{
    obj_type type;
    struct _obj* next;
};

struct _obj_string
{
    obj_t obj;
    int length;
    char* chars;
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
    object->next = _vm_get_objects(vm);
    _vm_set_objects(vm, object);

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

obj_string_t* _obj_allocate_string(VM* vm, char* chars, int length)
{
    obj_string_t* string = ALLOCATE_OBJ(vm, obj_string_t, OBJ_STRING);
    string->length = length;
    string->chars = chars;

    return string;
}

obj_string_t* obj_string_move(VM* vm, char* chars, int length)
{
    return _obj_allocate_string(vm, chars, length);
}

obj_string_t* obj_string_copy(VM* vm, const char* chars, int length) 
{
    char* heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return _obj_allocate_string(vm, heap_chars, length);
}

bool obj_is_type(value_t value, obj_type type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#define OBJ_TYPE(value)         (AS_OBJ(value)->type)
#define IS_STRING(value)        obj_is_type(value, OBJ_STRING)

#define AS_STRING(value)        ((obj_string_t*)AS_OBJ(value))
#define AS_CSTRING(value)       (((obj_string_t*)AS_OBJ(value))->chars)

bool is_falsey(value_t v)
{
    return IS_NIL(v) || (IS_BOOL(v) && !AS_BOOL(v));
}

bool values_equal(value_t a, value_t b)
{
    if (a.type != b.type) return false;

    switch (a.type)
    {
    case VAL_BOOL:      return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:       return true;
    case VAL_NUMBER:    return AS_NUMBER(a) == AS_NUMBER(b);
    case VAL_OBJ:
    {
        obj_string_t* str_a = AS_STRING(a);
        obj_string_t* str_b = AS_STRING(b);
        return str_a->length == str_b->length && memcmp(str_a->chars, str_b->chars, str_a->length) == 0;
    }
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
// ----| Chunk |----------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_RETURN,
} op_code;

struct _chunk
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

int disassemble_instruction(chunk_t* chunk, int offset)
{
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
    {
        printf("   | ");
    }
    else
    {
        printf("%4d ", chunk->lines[offset]);
    } 

    uint8_t instruction = chunk->code[offset];
    switch (instruction)
    {
    case OP_CONSTANT:   return _constant_instruction("OP_CONSTANT", chunk, offset);
    case OP_NIL:        return _simple_instruction("OP_NIL", offset);
    case OP_TRUE:       return _simple_instruction("OP_TRUE", offset);
    case OP_FALSE:      return _simple_instruction("OP_FALSE", offset);
    case OP_EQUAL:      return _simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:    return _simple_instruction("OP_GREATER", offset);
    case OP_LESS:       return _simple_instruction("OP_LESS", offset);
    case OP_ADD:        return _simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:   return _simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:   return _simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:     return _simple_instruction("OP_DIVIDE", offset);
    case OP_NOT:        return _simple_instruction("OP_NOT", offset);
    case OP_NEGATE:     return _simple_instruction("OP_NEGATE", offset);
    case OP_RETURN:     return _simple_instruction("OP_RETURN", offset);
    default:    
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}

void disassemble_chunk(chunk_t* chunk, const char* name) 
{
    printf("== %s ==\n", name);

    for (int offset = 0; offset < chunk->count;) 
    {
        offset = disassemble_instruction(chunk, offset);
    }
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

    TOKEN_ERROR,
    TOKEN_EOF
} token_type;

struct _token
{
    token_type type;
    const char* start;
    int length;
    int line;
};

struct _scanner
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
    {
        return type;
    }

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
            {
                while (scanner_peek() != '\n' && !scanner_end()) scanner_advance();
            }
            else
            {
                return;
            }
        default:
            return;
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
        {
            switch (scanner.start[1])
            {
            case 'a': return scanner_check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return scanner_check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u': return scanner_check_keyword(2, 1, "n", TOKEN_FUN);
            }
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
        {
            switch (scanner.start[1])
            {
            case 'h': return scanner_check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r': return scanner_check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
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

struct _parser
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

typedef void (*parse_fn)(VM* vm, parser_t* parser);

struct _parse_rule
{
    parse_fn prefix;
    parse_fn infix;
    precedence precedence;
};

parse_rule_t* parser_get_rule(token_type type);

chunk_t* compiling_chunk;

chunk_t* current_chunk()
{
    return compiling_chunk;
}

static void parser_error_at(parser_t* parser, token_t* token, const char* message)
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

static void parser_error_at_current(parser_t* parser, const char* message)
{
    parser_error_at(parser, &parser->current, message);
}

static void parser_error(parser_t* parser, const char* message)
{
    parser_error_at(parser, &parser->previous, message);
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

void parser_emit_constant(parser_t* parser, value_t value)
{
    parser_emit_bytes(parser, OP_CONSTANT, parser_make_constant(parser, value));
}

static void end_compiler(parser_t* parser)
{
    parser_emit_return(parser);
#ifdef DEBUG_PRINT_CODE
    if (!parser->had_error)
    {
        disassemble_chunk(current_chunk(), "code");
    }
#endif
}

void parse_precedence(VM* vm, parser_t* parser, precedence p)
{
    parser_advance(parser);

    parse_fn prefix_rule = parser_get_rule(parser->previous.type)->prefix;

    if (prefix_rule == NULL)
    {
        parser_error(parser, "Expect expression.");
        return;
    }

    prefix_rule(vm, parser);

    while (p <= parser_get_rule(parser->current.type)->precedence)
    {
        parser_advance(parser);
        parse_fn infix_rule = parser_get_rule(parser->previous.type)->infix;
        infix_rule(vm, parser);
    }
}

void parse_number(VM* vm, parser_t* parser)
{
    double value = strtod(parser->previous.start, NULL);
    parser_emit_constant(parser, NUMBER_VAL(value));
}

void parse_string(VM* vm, parser_t* parser)
{
    parser_emit_constant(parser, OBJ_VAL(obj_string_copy(vm, parser->previous.start + 1, parser->previous.length - 2)));
}

void parse_expression(VM* vm, parser_t* parser)
{
    parse_precedence(vm, parser, PREC_ASSIGNMENT);
}

void parse_grouping(VM* vm, parser_t* parser)
{
    parse_expression(vm, parser);
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void parse_unary(VM* vm, parser_t* parser)
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

void parse_binary(VM* vm, parser_t* parser)
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

void parse_literal(VM* vm, parser_t* parser)
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
    { NULL,             NULL,           PREC_NONE },        // TOKEN_IDENTIFIER
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
    parse_expression(vm, &parser);

    parser_consume(&parser, TOKEN_EOF, "Expect end of expression.");
    end_compiler(&parser);

    return !parser.had_error;
}

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

#include <stdarg.h>

struct _virtual_machine
{
    chunk_t* chunk;
    uint8_t* ip;
    value_t stack[STACK_MAX];
    value_t* stack_top;

    obj_t* objects;
};

void _vm_set_objects(VM* vm, obj_t* objects)
{
    vm->objects = objects;
}

obj_t* _vm_get_objects(VM* vm)
{
    return vm->objects;
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
}

void vm_free(VM* vm)
{
    free_objects(vm);
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
        case OP_RETURN:
            print_value(vm_pop(vm));
            printf("\n");
            return INTERPRET_OK;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT

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
    char line[1024];

    while (true)
    {
        printf("> ");

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
        fprintf(stderr, "Usage: sagittarius [path]\n");
    }

    vm_free(&vm);
    return 0;
}