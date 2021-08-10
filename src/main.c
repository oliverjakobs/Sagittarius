#include "vm.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

        cw_interpret(vm, line);
    }
}

static char* read_file(const char* path)
{
    char* buffer = NULL;
    FILE* file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        goto error;
    }

    fseek(file, 0L, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    buffer = malloc(filesize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        goto error;
    }

    size_t bytesread = fread(buffer, sizeof(char), filesize, file);
    if (bytesread < filesize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        goto error;
    }

    buffer[bytesread] = '\0';
    return buffer;

error:
    if (file) fclose(file);
    if (buffer) free(buffer);
    return NULL;
}

static InterpretResult run_file(VM* vm, const char* path)
{
    char* source = read_file(path);
    if (!source) return INTERPRET_COMPILE_ERROR;

    InterpretResult result = cw_interpret(vm, source);
    free(source); 

    return result;
}

int main(int argc, const char* argv[])
{
    VM vm;
    cw_vm_init(&vm);

    int status = 0;
    if (argc == 1) 
        repl(&vm);
    else if (argc == 2)
        status = run_file(&vm, argv[1]);
    else
        fprintf(stderr, "Usage: clockwork <path>\n");

    cw_vm_free(&vm);

    return status;
}