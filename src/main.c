#include "runtime.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void repl(cwRuntime* cw)
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

        cw_interpret(cw, line);
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

static InterpretResult run_file(cwRuntime* cw, const char* path)
{
    char* source = read_file(path);
    if (!source) return INTERPRET_COMPILE_ERROR;

    InterpretResult result = cw_interpret(cw, source);
    free(source); 

    return result;
}

int main(int argc, const char* argv[])
{
    cwRuntime cw = { 0 };
    cw_init(&cw);

    int status = 0;
    if (argc == 1) 
        repl(&cw);
    else if (argc == 2)
        status = run_file(&cw, argv[1]);
    else
        fprintf(stderr, "Usage: clockwork <path>\n");

    cw_free(&cw);

    return status;
    
    /* 
    uint8_t bytes[4];

    int32_t value = 23;
    
    bytes[0] = (value >> 24) & 0xFF;
    bytes[1] = (value >> 16) & 0xFF;
    bytes[2] = (value >> 8) & 0xFF;
    bytes[3] = value & 0xFF;
    
    printf("%x %x %x %x\n", bytes[0], bytes[1], bytes[2], bytes[3]);
    printf("%d\n", (int32_t)((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]));
    */

    /*
    uint8_t bytes[4];

    float value = 4.2f;

    uint32_t as_i = *((int32_t*)&value);
    printf("%d\n", as_i);
    
    bytes[0] = (as_i >> 24) & 0xFF;
    bytes[1] = (as_i >> 16) & 0xFF;
    bytes[2] = (as_i >> 8) & 0xFF;
    bytes[3] = as_i & 0xFF;
    
    printf("%x %x %x %x\n", bytes[0], bytes[1], bytes[2], bytes[3]);

    uint32_t raw = (uint32_t)((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
    printf("%f\n", *((float*)&raw));
    */
}