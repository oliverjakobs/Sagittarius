
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.c"

int main(int argc, const char* argv[])
{
    Token token = {
        .type = TOKEN_UNKOWN,
        "ERROR"
    };

    printf(token.start);
    return 0;
}