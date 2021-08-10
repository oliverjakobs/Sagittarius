#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "vm.h"

bool cw_compile(const char* src, Chunk* chunk);

#endif /* !CLOCKWORK_COMPILER_H */