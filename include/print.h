#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include "file.h"

void print(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
}

void exit_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    cleanup_cabinet();
    exit(EXIT_FAILURE);
}

#endif // PRINT_H