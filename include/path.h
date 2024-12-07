#ifndef PATH_H
#define PATH_H

#include "dynamic_array.h"
#include <stdarg.h>

#define REPO_PATH "."
#define HEAD_PATH_(path) path "\\.snaptrack\\HEAD"
#define HEAD_PATH HEAD_PATH_(REPO_PATH)
#define INDEX_PATH_(path) path "\\.snaptrack\\index"
#define INDEX_PATH INDEX_PATH_(REPO_PATH)
#define TEMP_INDEX_PATH_(path) path "\\.snaptrack\\temp_index"
#define TEMP_INDEX_PATH TEMP_INDEX_PATH_(REPO_PATH)
#define OBJECTS_PATH_(path) path "\\.snaptrack\\objects\\"
#define OBJECTS_PATH OBJECTS_PATH_(REPO_PATH)
#define BRANCHES_PATH_(path) path "\\.snaptrack\\refs\\branches\\"
#define BRANCHES_PATH BRANCHES_PATH_(REPO_PATH)

#define STRINGS_EQUAL(str1, str2) (strcmp((str1), (str2)) == 0 ? 1 : 0)
#define same_path(file1, file2) STRINGS_EQUAL((file1)->path, (file2)->path)
#define same_hash(file1, file2) STRINGS_EQUAL((file1)->hash, (file2)->hash)
#define same_file(file1, file2) (same_path((file1), (file2)) && same_hash((file1), (file2)))
#define same_string(s1, s2) STRINGS_EQUAL(s1, s2)
#define STRING_EMPTY(str) (strcmp((str), "") == 0 ? 1 : 0)

typedef DynamicArray Strings;
extern Strings strings;

char *malloc_string(size_t size);

char *new_hash(const char *path);
char *new_path(const char *format, ...);
void cleanup_paths();

#endif // PATH_H