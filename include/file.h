#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <minwindef.h>
#include "dynamic_array.h"

#define REPO_PATH "."

#define SHA1_BLOCK_SIZE 20
#define SHA1_STRING_SIZE 41 // SHA1_BLOCK_SIZE * 2 + 1

typedef void (*SHA1FileFunc)(const char *filename, unsigned char hash[SHA1_BLOCK_SIZE]);

typedef struct {
    HMODULE handle;
    void *func;
} DLL;

void load_function(DLL *dll, const char *library_name, const char *function_name);
void free_library(DLL *dll);

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_STRING_SIZE]);

int is_same_string(const char *string1, const char *string2);

#define Files DynamicArray

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

void get_repo_files(const char *path, Files *repo_files, const char *ignore_file_path);
void get_index_files(const char *index_path, Files *index_files, FileStatus status);

int does_dir_exist(const char *path);
void check_repo_already_exists(const char *repo_path);
void repo_must_exist(const char *repo_path);

void make_directory(const char *repo_path, const char *subdir);
void create_file(const char *repo_path, const char *subpath, const char *content);

#endif // FILE_H