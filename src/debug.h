#ifndef CLOCKWORK_DEBUG_H
#define CLOCKWORK_DEBUG_H

#include "compiler.h"

void cw_disassemble_chunk(const cwChunk* chunk, const char* name);
int  cw_disassemble_instruction(const cwChunk* chunk, int offset);

void cw_print_value(cwValue val);
void cw_print_object(cwValue val);


/* Error Handling */
void cw_runtime_error(cwRuntime* cw, const char* format, ...);

void cw_syntax_error(cwRuntime* cw, int line, const char* fmt, ...);
void cw_syntax_error_at(cwRuntime* cw, cwToken* token, const char* msg);

#endif /* !CLOCKWORK_DEBUG_H */