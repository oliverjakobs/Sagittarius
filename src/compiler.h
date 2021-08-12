#ifndef CLOCKWORK_COMPILER_H
#define CLOCKWORK_COMPILER_H

#include "vm.h"
#include "common.h"
#include "scanner.h"

typedef struct
{
    Scanner scanner;
    Chunk* chunk;

    Token current;
    Token previous;
    
    bool error;
    bool panic;
} Parser;

bool cw_compile(VM* vm, const char* src, Chunk* chunk);

#endif /* !CLOCKWORK_COMPILER_H */