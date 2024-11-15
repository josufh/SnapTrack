#ifndef COMMIT_H
#define COMMIT_H

#include <stdio.h>
#include "file.h"

FILE* obj_file_open(const char *hash, const char *mode);
void get_index_hash_from_commit_hash(const char *commit_hash, char *index_hash);
void load_index_files_from_index_hash(const char *hash, Files *files);
void create_src_from_object(File *file);
void rewrite_index_file(Files *files);

#endif // COMMIT_H