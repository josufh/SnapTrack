#include <stdio.h>
#include <string.h>
#include "file.h"

// gcc -g -o test\test test\test.c src\file.c src\ignore.c src\sha.c -Iinclude -L. -lsha1

int main() {
    Files *index_files = new_files_entry();
    Files *repo_files = new_files_entry();
    
    char index_path[MAX_PATH];
    snprintf(index_path, MAX_PATH, "%s\\.snaptrack\\index", REPO_PATH);
    load_index_files(index_path, index_files);

    load_path_files(REPO_PATH, repo_files);

    foreach_file(*repo_files, file) {
        char *path = file->path;
    }

    cleanup_cabinet();
    return 1;
}