#include "commit.h"
#include "path.h"
#include "print.h"
#include <minwindef.h>

FILE* obj_file_open(const char *hash, const char *mode) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s%s", OBJECTS_PATH, hash);
    return file_open(path, mode);
}

void get_index_hash_from_commit_hash(const char *commit_hash, char *index_hash) {
    FILE *commit_file = obj_file_open(commit_hash, "r");

    char _[256] = {0};
    fscanf(commit_file, "%s", _);
    fscanf(commit_file, "%s", index_hash);

    fclose(commit_file);
}

void load_index_files_from_index_hash(const char *hash, Files *files) {
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s%s", OBJECTS_PATH, hash);
    load_index_files(path, files);
}

void create_src_from_object(File *file) {
    FILE *read = obj_file_open(file->hash, "r");
    FILE *write = file_open(file->path, "w");

    char buffer[1024];
    size_t bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), read)) > 0) {
        fwrite(buffer, 1, bytes, write);
    }

    fclose(read); fclose(write);
}

void rewrite_index_file(Files *files) {
    FILE *temp_file = file_open(TEMP_INDEX_PATH, "w");

    foreach_file(*files, file)
        fprintf(temp_file, "%s %s 0\n", file->path, file->hash);

    fclose(temp_file);
    remove(INDEX_PATH);
    rename(TEMP_INDEX_PATH, INDEX_PATH);
}