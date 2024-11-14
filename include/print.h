#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include "file.h"

typedef enum {
    Reset,
    Red,
    Yellow,
    White,
    Green
} Color;

void print_out(Color color, const char *format, ...);

void exit_error(const char *format, ...);

#endif // PRINT_H