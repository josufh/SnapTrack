#ifndef IGNORE_H
#define IGNORE_H

#include <minwindef.h>

#include "dynamic_array.h"

typedef DynamicArray IgnorePatterns;
extern IgnorePatterns ignore_patterns;

int wildcard_match(const char *pattern, const char *str);
int should_ignore(const char *filename, IgnorePatterns patterns, int is_dir);

void load_ignore_patterns();
void free_ignore_patterns();

#endif // IGNORE_H