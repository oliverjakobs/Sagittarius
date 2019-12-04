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

#define CW_DEBUG_PRINT_CODE
#define CW_DEBUG_TRACE_EXECUTION

#define CW_STACK_MAX        256
#define CW_UINT8_COUNT      (UINT8_MAX + 1)

// -----------------------------------------------------------------------------
// ----| typedefs |-------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef struct _cw_struct_vm        cw_VM;
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

#define CW_BOOL_VAL(value)      ((cw_value_t) { CW_VAL_BOOL, { .boolean = value }})
#define CW_NIL_VAL              ((cw_value_t) { CW_VAL_NIL, { .number = 0 }})
#define CW_NUMBER_VAL(value)    ((cw_value_t) { CW_VAL_NUMBER, { .number = value }})
#define CW_OBJ_VAL(value)       ((cw_value_t) { CW_VAL_OBJ, { .obj = (obj_t*)value }})

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
// ----| Object |---------------------------------------------------------------
// -----------------------------------------------------------------------------

typedef enum
{
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

cw_obj_string_t* cw_obj_string_move(cw_VM* vm, char* chars, int length);
cw_obj_string_t* cw_obj_string_copy(cw_VM* vm, const char* chars, int length);

bool cw_obj_is_type(cw_value_t value, cw_obj_type type);

#define CW_OBJ_TYPE(value)     (CW_AS_OBJ(value)->type)
#define CW_IS_STRING(value)    cw_obj_is_type(value, CW_OBJ_STRING)

#define CW_AS_STRING(value)    ((obj_string_t*)CW_AS_OBJ(value))
#define CW_AS_CSTRING(value)   (((obj_string_t*)CW_AS_OBJ(value))->chars)

void print_obj(cw_value_t value);

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

cw_obj_string_t* table_find_string(cw_table_t* table, const char* chars, int length, uint32_t hash);

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
    CW_TOKEN_AND, TOKEN_CLASS, CW_TOKEN_ELSE, CW_TOKEN_FALSE,
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

typedef void (*cw_parse_fn)(cw_VM* vm, cw_parser_t* parser, bool can_assign);

typedef struct
{
    cw_parse_fn prefix;
    cw_parse_fn infix;
    cw_precedence precedence;
} cw_parse_rule_t;

cw_parse_rule_t* cw_parser_get_rule(cw_token_type type);

typedef struct
{
    cw_token_t name;
    int depth;
} cw_local_t;

typedef struct
{
    cw_local_t locals[CW_UINT8_COUNT];
    int local_count;
    int scope_depth;
} cw_compiler_t;

bool cw_compile(cw_VM* vm, const char* src, cw_chunk_t* chunk);

// -----------------------------------------------------------------------------
// ----| vm |-------------------------------------------------------------------
// -----------------------------------------------------------------------------

struct _cw_struct_vm
{
    cw_chunk_t* chunk;
    uint8_t* ip;
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

void cw_init(cw_VM* vm);
void cw_free(cw_VM* vm);

void cw_push(cw_VM* vm, cw_value_t v);
cw_value_t cw_pop(cw_VM* vm);

cw_interpret_result cw_interpret(cw_VM* vm, const char* src);

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

#endif // CLOCKWORK_IMPLEMENTATION

/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
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
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/