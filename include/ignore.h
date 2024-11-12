#ifndef IGNORE_H
#define IGNORE_H

#include <minwindef.h>

#include "dynamic_array.h"

#define IgnorePatterns DynamicArray
extern IgnorePatterns ignore_patterns;

int wildcard_match(const char *pattern, const char *str);
char *get_pattern_at_index(IgnorePatterns patterns, size_t index);
int should_ignore(const char *filename, IgnorePatterns patterns, int is_dir);

void add_pattern(const char *pattern);
void load_ignore_patterns();
void free_ignore_patterns();

#endif // IGNORE_H