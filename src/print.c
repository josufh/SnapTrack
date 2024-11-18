#include "print.h"
#include "path.h"
                            //  Reset      Red         Yellow      White       Green
static const char *colors[] = { "\033[0m", "\033[31m", "\033[33m", "\033[37m", "\033[32m" };

void print_out(Color color, const char *format, ...) {
    
    va_list args;
    va_start(args, format);
    fprintf(stdout, "%s", colors[color]);
    vfprintf(stdout, format, args);
    fprintf(stdout, "%s", colors[Reset]);
    va_end(args);
}

void exit_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "%s", colors[Red]);
    vfprintf(stderr, format, args);
    fprintf(stderr, "%s", colors[Reset]);
    va_end(args);

    free_sha_file();
    cleanup_cabinet();
    cleanup_paths();
    
    exit(EXIT_FAILURE);
}