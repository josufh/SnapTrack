#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <minwindef.h>
#include "dynamic_array.h"
#include "ignore.h"

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

typedef enum {
    Unchanged,
    New,
    Modified,
    Deleted
} FileStatus;

typedef struct {
    char path[MAX_PATH];
    char hash[SHA1_STRING_SIZE];
    FileStatus status;
    int staged;
} File;

#define Files DynamicArray
#define foreach_file(files, file) DA_FOREACH(files, file, File)

extern Files index_files, path_files;

void load_index_files(const char *path, Files *files);
void init_index_files(const char *path);
void free_index_files(Files *files);

void init_path_files(const char *path);
void free_path_files();

void get_files_from_path(const char *path);
File *get_file_at_index(Files files, size_t index);

void create_object(File file);

void print_repo_status();

FILE *file_open(const char* filepath, const char* mode);
void free_files(Files *files);

int does_dir_exist(const char *path);
void check_repo_already_exists(const char *repo_path);
void repo_must_exist(const char *repo_path);
void path_must_be_valid(const char *path);

void make_directory(const char *repo_path, const char *subdir);
void create_file(const char *repo_path, const char *subpath, const char *content);
int is_directory(const char *path);

#endif // FILE_H