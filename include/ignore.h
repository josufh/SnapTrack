#ifndef IGNORE_H
#define IGNORE_H

#include <minwindef.h>

#include "dynamic_array.h"

#define IgnorePatterns DynamicArray

int wildcard_match(const char *pattern, const char *str);
char *get_pattern_at_index(IgnorePatterns patterns, size_t index);
int should_ignore(const char *filename, IgnorePatterns *patterns, int is_directory);

void add_pattern(IgnorePatterns *patterns, const char *pattern);
void load_ignore_patterns(IgnorePatterns *patterns, const char *file_path);

#endif // IGNORE_H