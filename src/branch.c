#include "branch.h"
#include <minwindef.h>
#include <dirent.h>
#include "print.h"
#include "commit.h"

int does_branch_exist(const char *branch_name) {
    Branches branches = {0};

    DIR *dir = opendir(BRANCHES_PATH);
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if (is_same_string(entry->d_name, ".") || is_same_string(entry->d_name, "..")) continue;
        char *branch = malloc_string(entry->d_namlen);
        strcpy(branch, entry->d_name);
        add_branch(branches, branch);
    }
    closedir(dir);

    foreach_branch(branches, branch) {
        if (is_same_string(branch, branch_name)) {
            free_branches(branches);
            return 1;
        }
    }
    free_branches(branches);
    return 0;
}

void get_branch_commit_hash(const char* branch, char *hash) {
    if (!does_branch_exist(branch))
        exit_error("Branch %s does not exist\n", branch);

    char branch_path[MAX_PATH];
    snprintf(branch_path, MAX_PATH, "%s%s", BRANCHES_PATH, branch);
    FILE *branch_file = file_open(branch_path, "r");

    fgets(hash, SHA1_STRING_SIZE, branch_file);
}

void get_branch_index_files(const char *branch, Files *index_files) {
    char commit_hash[SHA1_STRING_SIZE] = {0};
    get_branch_commit_hash(branch, commit_hash);

    char index_hash[SHA1_STRING_SIZE] = {0};
    get_index_hash_from_commit_hash(commit_hash, index_hash);

    load_index_files_from_index_hash(index_hash, index_files);
}

void change_head(const char *branch) {
    FILE *head_file = file_open(HEAD_PATH, "w");
    fprintf(head_file, "refs\\branches\\%s", branch);
    fclose(head_file);
}

void current_branch_path(char *branch_path) {
    FILE *head_file = file_open(HEAD_PATH, "r");
    fscanf(head_file, "%s", branch_path);
    fclose(head_file);
}

char *current_branch() {
    repo_must_exist(REPO_PATH);

    char branch_path[MAX_PATH];
    current_branch_path(branch_path);

    static char branch_name[256];
    strcpy(branch_name, strrchr(branch_path, '\\')+1);

    return branch_name;
}

