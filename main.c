
#define CW_DEBUG_PRINT_CODE
#define CW_DEBUG_TRACE_EXECUTION

#define CLOCKWORK_IMPLEMENTATION
#include "clockwork.h"

static void repl(cw_virtual_machine_t* vm)
{
    printf("Clockwork v%d.%d\n", CW_VERSION_MAJOR, CW_VERSION_MINOR);

    char line[1024];

    while (true)
    {
        printf(">>> ");

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

static void run_file(cw_virtual_machine_t* vm, const char* path)
{
    char* source = read_file(path);
    cw_interpret_result result = cw_interpret(vm, source);
    free(source);

    if (result == CW_INTERPRET_COMPILE_ERROR) exit(65);
    if (result == CW_INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) 
{
    cw_virtual_machine_t vm;
    cw_init(&vm);

    // todo: add functions
    // _cw_define_native(&vm, "name", function);

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

    cw_free(&vm);
    return 0;
}