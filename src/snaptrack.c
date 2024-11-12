#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>
#include <time.h>
#include "file.h"
#include "config.h"
#include "snaptrack.h"
#include "ignore.h"

Command which_command(const char *command) {
    if (is_same_string(command, "init")) return Init;
    else if (is_same_string(command, "status")) return Status;
    else if (is_same_string(command, "add")) return Add;
    else if (is_same_string(command, "commit")) return CommitChanges;
    else if (is_same_string(command, "config")) return Config;
    else if (is_same_string(command, "revert")) return Revert;
    else if (is_same_string(command, "branch")) return Branch;
    else return UnknownCommand;
}

// Init
void init_repository() {
    check_repo_already_exists(REPO_PATH);
    make_directory(REPO_PATH, "");
    make_directory(REPO_PATH, "objects");
    make_directory(REPO_PATH, "refs");
    make_directory(REPO_PATH, "refs\\branches");
    create_file(REPO_PATH, "refs\\branches\\main", "");

    create_file(REPO_PATH, "HEAD", "refs\\branches\\main");
    create_file(REPO_PATH, "index", NULL);

    fprintf(stdout, "Initialized local empty SnapTrack repository\n");
}

void compare_repo_index(Files *staged_files, Files *repo_files) {
    for (int i = 0; i < staged_files->count; i++) {
        File *filei = get_file_at_index(*staged_files, i);
        for (int j = 0; j < repo_files->count; j++) {
            File *filer = get_file_at_index(*repo_files, j);
            if (is_same_string(filei->path, filer->path)) {
                if (is_same_string(filei->hash, filer->hash)) {
                    filei->status = Staged;
                    filer->status = Staged;
                    break;
                }
                filei->status = Modified;
                filer->status = Modified;
                break;
            }
        }
    }
}

// Stage
void stage_files(const char *to_stage_path) {
    repo_must_exist(REPO_PATH);
    int path_exists = does_dir_exist(to_stage_path);

    // Get files from the passed path
    init_path_files(to_stage_path);

    // Get branch index files
    char index_path[MAX_PATH];
    snprintf(index_path, MAX_PATH, "%s\\.snaptrack\\index", REPO_PATH);
    init_index_files(index_path);

    // Temporary index file
    char temp_path[MAX_PATH];
    snprintf(temp_path, MAX_PATH, "%s\\.snaptrack\\temp_index", REPO_PATH);
    FILE *temp_file = file_open(temp_path, "w");

    if (!is_directory(to_stage_path)) {
        File file_to_stage = *get_file_at_index(path_files, 0);
        if (file_to_stage.status == Deleted) {
            foreach_file(index_files, index_file) {
                if (is_same_string(index_file->path, file_to_stage.path)) {
                    
                }
            }
            fprintf(stderr, "Error: %s did not match any files\n", to_stage_path);
            exit(EXIT_FAILURE);
        }

        foreach_file(index_files, index_file) {
            if (is_same_string(index_file->path, file_to_stage.path)) {
                if (is_same_string(index_file->hash, file_to_stage.hash)) {
                    file_to_stage.status = Staged;
                } else {
                    file_to_stage.status = Modified;
                    create_object(file_to_stage);
                }
                fprintf(temp_file, "%s %s\n", file_to_stage.path, file_to_stage.hash);
            }
            fprintf(temp_file, "%s %s\n", index_file->path, index_file->hash);
        }
    }

    fclose(temp_file);
    remove(index_path);
    rename(temp_path, index_path);

    free_index_files();
    free_path_files();
}

// Status
void check_status() {
    repo_must_exist(REPO_PATH);

    DLL sha1_dll;
    load_function(&sha1_dll, "sha1.dll", "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    char index_path[MAX_PATH];
    snprintf(index_path, sizeof(index_path), "%s\\.snaptrack\\index", REPO_PATH);

    Files staged_files = {0};
    init_index_files(index_path);

    IgnorePatterns ignore_patterns = {0};
    load_ignore_patterns(&ignore_patterns, ".snaptrackignore");

    Files repo_files = {0};
    get_files_from_path(REPO_PATH);

    DA_FREE(ignore_patterns);

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        File *file = get_file_at_index(repo_files, i);
        sha1_file(file->path, hash);
        sha1_to_hex(hash, file->hash);
    }

    compare_repo_index(&staged_files, &repo_files);

    print_files_by_status(staged_files, Modified);
    print_files_by_status(repo_files, New);
    print_files_by_status(staged_files, Deleted);

    free_index_files();
    free_files(&repo_files);

    free_library(&sha1_dll);
}

// Commit
void commit_changes(const char *commit_message) {
    repo_must_exist(REPO_PATH);
    Commit commit = {0};

    DLL sha1_dll;
    load_function(&sha1_dll, "sha1.dll", "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    // Get index file hash and store object
    File index_file = {0};
    snprintf(index_file.path, MAX_PATH, "%s\\.snaptrack\\index", REPO_PATH);
    
    char index_hash[SHA1_STRING_SIZE];
    unsigned char hash[SHA1_BLOCK_SIZE];
    sha1_file(index_file.path, hash);
    sha1_to_hex(hash, index_file.hash);

    strncpy(commit.index_hash, index_file.hash, sizeof(index_file.hash));

    char object_path[MAX_PATH];
    snprintf(object_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, index_file.hash);
    FILE *object_file = file_open(object_path, "wb");
    FILE *src_file = file_open(index_file.path, "rb");

    int c;
    while ((c = fgetc(src_file)) != EOF) {
        fputc(c, object_file);
    }

    fclose(object_file); fclose(src_file);

    // Get last commit hash
    char head_path[MAX_PATH];
    snprintf(head_path, MAX_PATH, "%s\\.snaptrack\\HEAD", REPO_PATH);
    FILE *head_file = file_open(head_path, "r");
    char branch_path[MAX_PATH];
    fgets(branch_path, MAX_PATH, head_file);
    fclose(head_file);

    char current_branch_path[MAX_PATH];
    snprintf(current_branch_path, MAX_PATH, "%s\\.snaptrack\\%s", REPO_PATH, branch_path);
    FILE *current_branch_file = file_open(current_branch_path, "r");
    char last_commit[SHA1_STRING_SIZE] = {0};
    fgets(last_commit, sizeof(last_commit), current_branch_file);

    strncpy(commit.parent, last_commit, sizeof(commit.parent));

    // Get user info
    User user = {0};
    get_user_info(&user);

    strcpy(commit.author_name, user.name);
    strcpy(commit.author_email, user.email);
    strcpy(commit.author_userid, user.userid);

    // Get message
    strncpy(commit.message, commit_message, sizeof(commit.message));

    // Get time
    commit.timestamp = time(NULL);

    // Get this commit hash and store
    char commit_content[1024];
    snprintf(commit_content, sizeof(commit_content),
                "index %s\nparent %s\nauthor\n\t%s\n\t%s\n\t%s\ndate %ld\n\n%s",
                commit.index_hash, commit.parent, commit.author_name, commit.author_email, commit.author_userid,
                commit.timestamp, commit.message);
    
    File temp_commit = {0};
    snprintf(temp_commit.path, MAX_PATH, "%s\\.snaptrack\\temp_commit", REPO_PATH);
    FILE *temp_commit_file = file_open(temp_commit.path, "w");
    fprintf(temp_commit_file, "%s", commit_content);
    fclose(temp_commit_file);

    sha1_file(temp_commit.path, hash);
    sha1_to_hex(hash, temp_commit.hash);
    
    snprintf(object_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, temp_commit.hash);
    object_file = file_open(object_path, "wb");
    src_file = file_open(temp_commit.path, "rb");

    while ((c = fgetc(src_file)) != EOF) {
        fputc(c, object_file);
    }
    fclose(object_file); fclose(src_file);

    // Update last commit
    current_branch_file = file_open(current_branch_path, "w");
    fprintf(current_branch_file, "%s", temp_commit.hash);
    fclose(current_branch_file);

    remove(temp_commit.path);
    free_library(&sha1_dll);

    printf("TODO: Commit done message\n");
}

void get_commit_info(Commit *commit, const char *commit_hash) {
    char object_path[MAX_PATH];
    snprintf(object_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, commit_hash);
    FILE *object_file = file_open(object_path, "rb");

    // get hash
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
    sscanf(line, "%s %s", _, commit->timestamp);
    fgets(line, 256, object_file);
    fgets(line, 256, object_file);
    strcpy(commit->message, line);

    fclose(object_file);
}

// Show commits
void list_commits() {
    Commit *commits = NULL;
    int commit_count = 0;

    // Get last commit hash
    char head_path[MAX_PATH];
    snprintf(head_path, MAX_PATH, "%s\\.snaptrack\\HEAD", REPO_PATH);
    FILE *head_file = file_open(head_path, "r");
    char branch_path[MAX_PATH];
    fgets(branch_path, MAX_PATH, head_file);
    fclose(head_file);

    char current_branch_path[MAX_PATH];
    snprintf(current_branch_path, MAX_PATH, "%s\\.snaptrack\\%s", REPO_PATH, branch_path);
    FILE *current_branch_file = file_open(current_branch_path, "r");
    char last_commit_hash[SHA1_STRING_SIZE] = {0};
    fgets(last_commit_hash, sizeof(last_commit_hash), current_branch_file);

    while (strcmp(last_commit_hash, "") != 0) {
        Commit commit = {0};
        get_commit_info(&commit, last_commit_hash);
        strcpy(last_commit_hash, commit.parent);

        commits = realloc(commits, (commit_count+1)*sizeof(Commit));
        commits[commit_count] = commit;
        commit_count++;
    }

    fprintf(stdout, "Commit list:\n");
    for (int i = 0; i < commit_count; i++) {
        fprintf(stdout, "\t   Hash: %s\n", commits[i].hash);
        fprintf(stdout, "\tMessage: %s\n", commits[i].message);
        fprintf(stdout, "\n");
    }
    free(commits);
}

// Revert commit
void revert_commit(const char *revert_hash) {
    DLL sha1_dll;
    load_function(&sha1_dll, "sha1.dll", "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    Commit revert_commit = {0};
    get_commit_info(&revert_commit, revert_hash);

    char revert_index_blob_path[MAX_PATH] = {0};
    snprintf(revert_index_blob_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, revert_commit.index_hash);
    Files revert_files = {0};
    init_index_files(revert_index_blob_path);

    IgnorePatterns ignore_patterns = {0};
    load_ignore_patterns(&ignore_patterns, ".snaptrackignore");

    Files repo_files = {0};
    get_files_from_path(REPO_PATH);

    DA_FREE(ignore_patterns);

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        File *file = get_file_at_index(repo_files, i);
        sha1_file(file->path, hash);
        sha1_to_hex(hash, file->hash);
    }

    for (int i = 0; i < revert_files.count; i++) {
        File *revert_file = get_file_at_index(revert_files, i);
        char blob_path[MAX_PATH];
        snprintf(blob_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, revert_file->hash);
        FILE *read = file_open(blob_path, "r");
        FILE *write = file_open(revert_file->path, "w");

        char buffer[1024];
        size_t bytes;

        while ((bytes = fread(buffer, 1, sizeof(buffer), read)) > 0) {
            fwrite(buffer, 1, bytes, write);
        }

        fclose(read); fclose(write);

        for (int j = 0; j < repo_files.count; j++) {
            File *repo_file = get_file_at_index(repo_files, j);
            if (is_same_string(revert_file->path, repo_file->path)) {
                repo_file->status = Staged;
                break;
            }
        }
    }

    for (int i = 0; i < repo_files.count; i++) {
        File *repo_file = get_file_at_index(repo_files, i);
        if (repo_file->status == New) {
            remove(repo_file->path);
        }
    }

    stage_files(REPO_PATH);
    char message[256];
    snprintf(message, 256, "REVERT TO COMMIT HASH: %s", revert_commit.hash);
    commit_changes(message);

    free_files(&repo_files);
    free_index_files();
}

// Branch
void get_current_branch_path(char *branch_string) {
    char head_path[MAX_PATH];
    snprintf(head_path, MAX_PATH, "%s\\.snaptrack\\HEAD", REPO_PATH);
    FILE *head_file = file_open(head_path, "r");

    char line[MAX_PATH];
    fgets(line, MAX_PATH, head_file);
    strcpy(branch_string, line);
}

void current_branch() {
    repo_must_exist(REPO_PATH);
    char branch_path[MAX_PATH];
    get_current_branch_path(branch_path);

    char branch_name[256];
    strcpy(branch_name, strrchr(branch_path, '\\')+1);

    fprintf(stdout, "Current branch: %s\n", branch_name);
}

void list_branches() {
    repo_must_exist(REPO_PATH);
    char branches_path[MAX_PATH];
    snprintf(branches_path, MAX_PATH, "%s\\.snaptrack\\refs\\branches", REPO_PATH);

    DynamicArray branches = {0};
    DA_INIT(branches, 256);

    DIR *dir = opendir(branches_path);
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if (is_same_string(entry->d_name, ".") || is_same_string(entry->d_name, "..")) continue;
        DA_ADD(branches, entry->d_name);
    }
    closedir(dir);

    fprintf(stdout, "Branch list:\n");
    for (int i = 0; i < branches.count; i++) {
        char *branch = DA_GET(branches, i);
        fprintf(stdout, "\t%s\n", branch);
    }

    DA_FREE(branches);
}

void create_branch(const char *branch_name) {
    repo_must_exist(REPO_PATH);
    char branches_path[MAX_PATH];
    snprintf(branches_path, MAX_PATH, "%s\\.snaptrack\\refs\\branches", REPO_PATH);

    DynamicArray branches = {0};
    DA_INIT(branches, 256);

    DIR *dir = opendir(branches_path);
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        DA_ADD(branches, entry->d_name);
    }
    closedir(dir);

    int is_new_branch = 1;
    for (int i = 0; i < branches.count; i++) {
        char *branch = DA_GET(branches, i);

        if (is_same_string(branch, branch_name)) {
            is_new_branch = 0;
            break;
        }
    }
    DA_FREE(branches);

    if (!is_new_branch) {
        fprintf(stdout, "Branch with name %s already exists\n", branch_name);
        return;
    }

    char current_branch_relative_path[MAX_PATH];
    get_current_branch_path(current_branch_relative_path);
    char current_branch_path[MAX_PATH];
    snprintf(current_branch_path, MAX_PATH, "%s\\.snaptrack\\%s", REPO_PATH, current_branch_relative_path);
    FILE *current_branch_file = file_open(current_branch_path, "r");

    char commit_hash[SHA1_STRING_SIZE];
    fgets(commit_hash, SHA1_STRING_SIZE, current_branch_file);
    fclose(current_branch_file);

    char new_branch_file_path[MAX_PATH];
    snprintf(new_branch_file_path, MAX_PATH, "%s\\.snaptrack\\refs\\branches\\%s", REPO_PATH, branch_name);

    FILE *new_branch_file = file_open(new_branch_file_path, "w");
    fputs(commit_hash, new_branch_file);
    fclose(new_branch_file);

    char head_path[MAX_PATH];
    snprintf(head_path, MAX_PATH, "%s\\.snaptrack\\HEAD", REPO_PATH);
    FILE *head_file = file_open(head_path, "w");

    fprintf(head_file, "refs\\branches\\%s", branch_name);
    fclose(head_file);
}

void delete_branch(const char *branch_name_to_delete) {
    repo_must_exist(REPO_PATH);
    char branch_path[MAX_PATH];
    get_current_branch_path(branch_path);

    char branch_name[256];
    strcpy(branch_name, strrchr(branch_path, '\\')+1);

    if (is_same_string(branch_name_to_delete, branch_name)) {
        printf("Failed to delete: trying to delete current branch\n");
        return;
    }

    char branch_file_path_to_delete[MAX_PATH];
    snprintf(branch_file_path_to_delete, MAX_PATH, "%s\\.snaptrack\\refs\\branches\\%s", REPO_PATH, branch_name_to_delete);

    remove(branch_file_path_to_delete);
}