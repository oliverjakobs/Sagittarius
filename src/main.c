#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void printToken(cwToken token)
{
    printf("%s: %.*s\n", cwTokenTypeName(token.type), token.end - token.start, token.start);
}

static void repl()
{
    char stream[1024];
    while (true)
    {
        printf("> ");

        if (!fgets(stream, sizeof(stream), stdin))
        {
            printf("\n");
            break;
        }

        cwToken token;
        const char* cursor = stream;
        int line = 0;
        do 
        {
            cursor = cwNextToken(cursor, &line, &token);
            printToken(token);
        }
        while (token.type != TOKEN_EOF && token.type != TOKEN_UNKOWN);

        if (token.type == TOKEN_EOF) break;
    }
}


int main(int argc, const char* argv[])
{
    repl();
    return 0;
}