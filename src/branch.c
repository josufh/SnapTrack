#include "branch.h"
#include <minwindef.h>
#include <dirent.h>
#include "print.h"
#include "commit.h"

Branches *get_branches() {
    Branches *branches = (Branches *)malloc(sizeof(Branches));
    if (!branches)
        exit_error("Error allocating memory @ get_branches\n");
    DA_INIT(*branches);

    DIR *dir = opendir(BRANCHES_PATH);
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if (same_string(entry->d_name, ".") || same_string(entry->d_name, "..")) continue;
        char *branch = malloc_string(entry->d_namlen);
        strcpy(branch, entry->d_name);
        add_branch(branches, branch);
    }
    closedir(dir);

    return branches;
}

void free_branches(Branches *branches) {
    DA_FREE(*branches);
    free(branches);
}

int does_branch_exist(const char *branch_name) {
    Branches *branches = get_branches();

    foreach_branch(branches, branch) {
        if (same_string(branch, branch_name)) {
            free_branches(branches);
            return 1;
        }
    }
    free_branches(branches);
    return 0;
}

char *get_branch_commit_hash(const char* branch) {
    if (!does_branch_exist(branch))
        exit_error("Branch %s does not exist\n", branch);

    char *branch_path = new_path("%s%s", BRANCHES_PATH, branch);
    FILE *branch_file = file_open(branch_path, "r");

    char *hash = new_hash(NULL);
    fgets(hash, SHA1_STRING_SIZE, branch_file);
    return hash;
}

Files *get_branch_index_files(const char *branch) {
    char *commit_hash = get_branch_commit_hash(branch);
    if (STRING_EMPTY(commit_hash)) return new_files_entry();

    char *index_hash = get_index_hash_from_commit_hash(commit_hash);

    return get_index_files_from_index_hash(index_hash);
}

void change_head(const char *branch) {
    FILE *head_file = file_open(HEAD_PATH, "w");
    fprintf(head_file, "refs\\branches\\%s", branch);
    fclose(head_file);
}

char *get_current_branch_path() {
    char *branch_path = new_path("");
    FILE *head_file = file_open(HEAD_PATH, "r");
    fscanf(head_file, "%s", branch_path);
    fclose(head_file);
    return branch_path;
}

char *get_current_branch_name() {
    repo_must_exist(REPO_PATH);

    char *branch_path = get_current_branch_path();

    static char branch_name[256];
    strcpy(branch_name, strrchr(branch_path, '\\')+1);

    return branch_name;
}
