
// -----------------------------------------------------------------------------
// ----| Common |---------------------------------------------------------------
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

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

typedef struct
{
    token_type type;
    const char* start;
    int length;
    int line;
} token_t;


typedef struct
{     
  const char* start;
  const char* current;
  int line;
} scanner_t;

scanner_t scanner;

void init_scanner(const char* src)
{
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
}

static bool is_at_end()
{
  return *scanner.current == '\0';
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1];
}

// returns the current character, but doesn’t consume it
static char peek()
{
    return *scanner.current;
}

// like peek() but for one character past the current one
static char peek_next()
{
    if (is_at_end()) return '\0';
    return scanner.current[1]; 
}

static bool match(char expected)
{
    if (is_at_end()) return false;
    if (*scanner.current != expected) return false;

    scanner.current++;
    return true;
}

static token_type check_keyword(int start, int length, const char* rest, token_type type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
    {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static token_type identifier_type()
{
    switch (scanner.start[0])
    {
    case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
    case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (scanner.current - scanner.start > 1) 
        {
            switch (scanner.start[1])
            {
            case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
            case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
            case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
    case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
    case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
    case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
            case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break; 
    case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static token_t make_token(token_type type)
{
    token_t token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;

    return token;
}

static token_t error_token(const char* message)
{
    token_t token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;

    return token;
}

static token_t string()
{
    while (peek() != '"' && !is_at_end())
    {
        if (peek() == '\n') scanner.line++;
        advance();
    }

    if (is_at_end()) return error_token("Unterminated string.");

    // The closing quote.
    advance();
    return make_token(TOKEN_STRING); 
}

static token_t number()
{
    while (is_digit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next()))
    {
        // Consume the ".".
        advance();

        while (is_digit(peek())) advance();
    }

  return make_token(TOKEN_NUMBER);
}

static token_t identifier()
{
    while (is_alpha(peek()) || is_digit(peek())) advance();

    return make_token(identifier_type());
}

static void skip_whitespace()
{
    while(true)
    {
        char c = peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peek_next() == '/')
            {
                while (peek() != '\n' && !is_at_end()) advance();
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

token_t scan_token()
{
    skip_whitespace();

    scanner.start = scanner.current;

    if (is_at_end()) return make_token(TOKEN_EOF);

    char c = advance();

    // identifiers and keywords
    if (is_alpha(c)) return identifier();

    if (is_digit(c)) return number();

    switch (c)
    {
    // single character tokens
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
    // two-character punctuation tokens
    case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);  
    case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);  
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    // literal tokens
    case '"': return string();
    }

    return error_token("Unexpected character.");
}

// -----------------------------------------------------------------------------
// ----| compiler |-------------------------------------------------------------
// -----------------------------------------------------------------------------

void compile(const char* src)
{
    init_scanner(src);

    int line = -1;
    while(true)
    {
        token_t token = scan_token();
        if (token.line != line)
        {
            printf("%4d ", token.line);
            line = token.line;
        }
        else
        {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start); 

        if (token.type == TOKEN_EOF) break;
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

interpret_result interpret(VM* vm, const char* src)
{
    compile(src);

    return INTERPRET_OK;
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

        interpret(vm, line);
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
    interpret_result result = interpret(vm, source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}


int main(int argc, const char* argv[]) 
{
    VM vm;
    init_VM(&vm);

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


    free_VM(&vm);
    return 0;
}