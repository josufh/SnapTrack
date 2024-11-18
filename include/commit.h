#ifndef COMMIT_H
#define COMMIT_H

#include <stdio.h>
#include "file.h"

typedef struct {
    char index_hash[SHA1_STRING_SIZE];
    char parent[SHA1_STRING_SIZE];
    char hash[SHA1_STRING_SIZE];
    char author_name[256];
    char author_email[256];
    char author_userid[256];
    char message[512];
    time_t timestamp;
} Commit;

//typedef DynamicArray Commits;

FILE* obj_file_open(const char *hash, const char *mode);
char *get_index_hash_from_commit_hash(const char *commit_hash);
Files *get_index_files_from_index_hash(const char *hash);
void create_src_from_object(File *file);
void rewrite_index_file(Files *files);
int are_there_changes();
void get_commit_info(Commit *commit, const char *commit_hash);

#endif // COMMIT_H