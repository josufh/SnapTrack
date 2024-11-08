#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <minwindef.h>
#include "dynamic_array.h"

#define REPO_PATH "."

#define SHA1_BLOCK_SIZE 20
#define SHA1_STRING_SIZE 41 // SHA1_BLOCK_SIZE * 2 + 1
#define Files DynamicArray

// SHA stuff

const char **file_status_string = {"Staged", "New", "Modified", "Deleted"};

typedef enum {
    Staged = 0,
    New,
    Modified,
    Deleted
} FileStatus;

typedef struct {
    char path[MAX_PATH];
    char hash[SHA1_STRING_SIZE];
    FileStatus status;
} File;

File *get_file_at_index(Files files, size_t index);

void print_files_by_status(Files files, FileStatus status);

FILE *file_open(const char* filepath, const char* mode);

void free_files(Files *files);

#endif // FILE_H