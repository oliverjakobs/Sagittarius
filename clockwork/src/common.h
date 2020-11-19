#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "tb_stretchy.h"
#include "str_intern.h"

static void fatal(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("FATAL: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

static void syntax_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("Syntax Error: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

static void fatal_syntax_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    printf("Syntax Error: ");
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
    exit(1);
}

#define MAX(a, b) ((a) >= (b) ? (a) : (b))

#endif /* !COMMON_H */
