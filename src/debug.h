#ifndef CLOCKWORK_DEBUG_H
#define CLOCKWORK_DEBUG_H

#include "chunk.h"

void cw_disassemble_chunk(const Chunk* chunk, const char* name);
int  cw_disassemble_instruction(const Chunk* chunk, int offset);

void cw_print_value(Value val);
void cw_print_object(Value val);


/* Error Handling */
void cw_runtime_error(cwRuntime* cw, const char* format, ...);

void cw_syntax_error(cwRuntime* cw, int line, const char* fmt, ...);
void cw_syntax_error_at(cwRuntime* cw, Token* token, const char* msg);

#endif /* !CLOCKWORK_DEBUG_H */