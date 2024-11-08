#include "ignore.h"

// gcc -g -o ignore_test.exe test\ignore_test.c src\ignore.c -Iinclude

int main() {
    const char *file_path = ".snaptrackignore";

    IgnorePatterns patterns = {0};

    load_ignore_patterns(&patterns, file_path);
    const char *pattern = get_pattern_at_index(patterns, 4);
    printf(pattern);
    //DA_PRINT(patterns);
}