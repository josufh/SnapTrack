#ifndef FILE_H
#define FILE_H

#include <minwindef.h>
#include "dynamic_array.h"
#include "ignore.h"
#include "sha.h"

#define REPO_PATH "."

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

typedef DynamicArray Files;
#define foreach_file(files, file) DA_FOREACH(files, file, File *)
#define files_get(files, index) DA_GET(fiels, index)

typedef DynamicArray Cabinet;
extern Cabinet cabinet;
#define foreach_files(cab, files) DA_FOREACH(cab, files, Files *)

Files *new_files_entry();
void cleanup_cabinet();

void load_index_files(const char *path, Files *files);
void load_path_files(const char *path, Files *files);

File *get_file_at_index(Files files, size_t index);

void create_object(File file);

FILE *file_open(const char* path, const char* mode);
/* FILE *file_open_read(const char* path);
FILE *file_open_write(const char* path);
void file_close(FILE *file); */

void check_repo_already_exists();
void repo_must_exist();
int does_dir_exist(const char *path);
void path_must_exist(const char *path);
void make_directory(const char *path);
void create_file(const char *path, const char *content);
int is_directory(const char *path);

#endif // FILE_H