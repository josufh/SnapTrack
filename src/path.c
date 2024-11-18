#include "path.h"
#include <minwindef.h>
#include "sha.h"
#include "print.h"

Strings strings = {0};

char *malloc_string(size_t size) {
    char *string = (char *)malloc(size);
    if (!string)
        exit_error("Error allocatig memory @ malloc_string\n");
    memset(string, 0, size);
    return string;
}

char *new_hash(const char *path) {
    char *hash_string = malloc_string(SHA1_STRING_SIZE);
    if (path) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha_file(path, hash);

        DA_ADD(strings, hash_string);

        sha1_to_hex(hash, hash_string);
    }

    return hash_string;
}

char *new_path(const char *format, ...) {
    char *path = malloc_string(MAX_PATH);

    va_list args;
    va_start(args, format);
    vsnprintf(path, MAX_PATH, format, args);
    va_end(args);

    DA_ADD(strings, path);
    return path;
}

void cleanup_paths() {
    DA_FOREACH(strings, string, char *)
        free(string);
    DA_FREE(strings);
}