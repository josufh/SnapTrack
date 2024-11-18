#include <minwindef.h>
#include "commit.h"
#include "path.h"
#include "print.h"
#include "branch.h"

FILE* obj_file_open(const char *hash, const char *mode) {
    char *path = new_path("%s%s", OBJECTS_PATH, hash);
    return file_open(path, mode);
}

char *get_index_hash_from_commit_hash(const char *commit_hash) {
    FILE *commit_file = obj_file_open(commit_hash, "r");
    
    char *index_hash = new_hash(NULL);
    char _[256] = {0};
    fscanf(commit_file, "%s", _);
    fscanf(commit_file, "%s", index_hash);

    fclose(commit_file);

    return index_hash;
}

Files *get_index_files_from_index_hash(const char *hash) {
    char *path = new_path("%s%s", OBJECTS_PATH, hash);
    return get_index_files(path);
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

    foreach_file(files, file)
        fprintf(temp_file, "%s %s 0\n", file->path, file->hash);

    fclose(temp_file);
    remove(INDEX_PATH);
    rename(TEMP_INDEX_PATH, INDEX_PATH);
}

int are_there_changes() {
    Files *repo_files = get_path_files(REPO_PATH);
    Files *last_files = get_branch_index_files(get_current_branch_name());

    if (repo_files->count != last_files->count) return 1;

    foreach_file(repo_files, repo_file)
        foreach_file(last_files, last_file) {
            if (same_file(repo_file, last_file) && !same_hash(repo_file, last_file)) return 1;
        }
    return 0;
}

void get_commit_info(Commit *commit, const char *commit_hash) {
    FILE *object_file = obj_file_open(commit_hash, "rb");

    strcpy(commit->hash, commit_hash);

    char line[256], _[256];
    fgets(line, 256, object_file);
    sscanf(line, "%s %s", _, commit->index_hash);
    fgets(line, 256, object_file);
    sscanf(line, "%s %s", _, commit->parent);
    fgets(line, 256, object_file);
    fgets(line, 256, object_file);
    sscanf(line, "%s", commit->author_name);
    fgets(line, 256, object_file);
    sscanf(line, "%s", commit->author_email);
    fgets(line, 256, object_file);
    sscanf(line, "%s", commit->author_userid);
    fgets(line, 256, object_file);
    sscanf(line, "%s %ld", _, &commit->timestamp);
    fgets(line, 256, object_file);
    fgets(line, 256, object_file);
    strcpy(commit->message, line);

    fclose(object_file);
}