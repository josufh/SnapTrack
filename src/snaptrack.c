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

Command which_command(const char *command) {
    if (is_same_string(command, "init")) return Init;
    else if (is_same_string(command, "status")) return Status;
    else if (is_same_string(command, "stage")) return Stage;
    else if (is_same_string(command, "commit")) return CommitChanges;
    else if (is_same_string(command, "config")) return Config;
    else if (is_same_string(command, "revert")) return Revert;
    else return UnknownCommand;
}

// Init
void init_repository(const char *repo_path) {
    check_repo_already_exists(repo_path);
    make_directory(repo_path, "");
    make_directory(repo_path, "objects");
    make_directory(repo_path, "branches");
    create_file(repo_path, "branches\\main", "");

    create_file(repo_path, "HEAD", "branches\\main");
    create_file(repo_path, "index", NULL);

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
void stage_files(const char *repo_path) {
    repo_must_exist(repo_path);

    DLL sha1_dll;
    load_function(&sha1_dll, "sha1.dll", "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    Files repo_files = {0};
    get_repo_files(repo_path, &repo_files, ".snaptrackignore");

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        File *file = get_file_at_index(repo_files, i);
        sha1_file(file->path, hash);
        sha1_to_hex(hash, file->hash);
    }

    char index_path[MAX_PATH];
    char temp_path[MAX_PATH];
    snprintf(index_path, MAX_PATH, "%s\\.snaptrack\\index", REPO_PATH);
    snprintf(temp_path, MAX_PATH, "%s\\.snaptrack\\temp_index", REPO_PATH);

    Files staged_files = {0};
    get_index_files(index_path, &staged_files, Deleted);

    compare_repo_index(&staged_files, &repo_files);

    char object_path[MAX_PATH];
    FILE *temp_file = file_open(temp_path, "w");

    for (int i = 0; i < repo_files.count; i++) {
        File file = *get_file_at_index(repo_files, i);
        if (file.status == Modified || file.status == New) {
            snprintf(object_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", repo_path, file.hash);
            FILE *object_file = file_open(object_path, "wb");
            FILE *src_file = file_open(file.path, "rb");

            int c;
            while ((c = fgetc(src_file)) != EOF) {
                fputc(c, object_file);
            }

            fclose(object_file); fclose(src_file);

            fprintf(stdout, "Stated changes for file %s with hash %s\n", file.path, file.hash);
        }
        fprintf(temp_file, "%s %s\n", file.path, file.hash);

    }
    fclose(temp_file);
    remove(index_path);
    rename(temp_path, index_path);

    free_files(&staged_files);
    free_files(&repo_files);

    free_library(&sha1_dll);
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
    get_index_files(index_path, &staged_files, Deleted);

    Files repo_files = {0};
    get_repo_files(REPO_PATH, &repo_files, ".snaptrackignore");

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

    free_files(&staged_files);
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
    get_index_files(revert_index_blob_path, &revert_files, Staged);

    Files repo_files = {0};
    get_repo_files(REPO_PATH, &repo_files, ".snaptrackignore");

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
    free_files(&revert_files);
}