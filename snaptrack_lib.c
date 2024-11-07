#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>
#include <time.h>
#include "fileutils.c"
#include "stringutils.c"

#define REPO_PATH "."

// Command check
typedef enum {
    Init = 0,
    Status,
    Stage,
    CommitChanges,
    UnknownCommand
} Command;

Command which_command(const char *command) {
    if (is_same_string(command, "init")) return Init;
    else if (is_same_string(command, "status")) return Status;
    else if (is_same_string(command, "stage")) return Stage;
    else if (is_same_string(command, "commit")) return CommitChanges;
    else return UnknownCommand;
}

// SHA1
typedef void (*SHA1FileFunc)(const char *filename, unsigned char hash[SHA1_BLOCK_SIZE]);

typedef struct {
    HMODULE handle;
    void *func;
} DLL;

void load_library(DLL *dll, const char *dll_path) {
    dll->handle = LoadLibrary(dll_path);
    if (!dll->handle) {
        fprintf(stderr, "Failed to load %s\n", dll_path);
        exit(EXIT_FAILURE);
    }
}

void load_function(DLL *dll, const char *function_name) {
    dll->func = (void *)GetProcAddress(dll->handle, function_name);
    if (!dll->func) {
        fprintf(stderr, "Failed to locate %s in DLL", function_name);
        FreeLibrary(dll->handle);
        exit(EXIT_FAILURE);
    }
}

void free_library(DLL *dll) {
    FreeLibrary(dll->handle);
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

// Stage
void stage_files(const char *repo_path) {
    repo_must_exist(repo_path);

    DLL sha1_dll;
    load_library(&sha1_dll, "sha1.dll");
    load_function(&sha1_dll, "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    Files repo_files = {0};
    get_repo_files(repo_path, &repo_files, ".snaptrackignore");

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha1_file(repo_files.items[i].path, hash);
        sha1_to_hex(hash, repo_files.items[i].hash);
    }

    char index_path[PATH_SIZE];
    char temp_path[PATH_SIZE];
    snprintf(index_path, sizeof(index_path), "%s\\.snaptrack\\index", repo_path);
    snprintf(temp_path, sizeof(temp_path), "%s\\.snaptrack\\temp_index", repo_path);
    FILE *index_file = file_open(index_path, "r");
    
    Files staged_files = {0};
    char line[1024];
    while (fgets(line, sizeof(line), index_file)) {
        File *new_items = realloc(staged_files.items, (staged_files.count+1)*sizeof(File));
        if (!new_items) {
            fprintf(stderr, "Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        staged_files.items = new_items;

        sscanf(line, "%s %s", staged_files.items[staged_files.count].path, staged_files.items[staged_files.count].hash);
        staged_files.items[staged_files.count].status = Deleted;
        staged_files.count++;
    }

    fclose(index_file);


    for (int i = 0; i < staged_files.count; i++) {
        File *filei = &staged_files.items[i];
        for (int j = 0; j < repo_files.count; j++) {
            File *filer = &repo_files.items[j];
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

    char object_path[PATH_SIZE];
    FILE *temp_file = file_open(temp_path, "w");

    for (int i = 0; i < repo_files.count; i++) {
        File file = repo_files.items[i];
        if (file.status == Modified || file.status == New) {
            snprintf(object_path, PATH_SIZE, "%s\\.snaptrack\\objects\\%s", repo_path, file.hash);
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
void check_status(const char *repo_path) {
    repo_must_exist(repo_path);

    char index_path[PATH_SIZE];
    snprintf(index_path, sizeof(index_path), "%s\\.snaptrack\\index", repo_path);

    FILE *index_file = file_open(index_path, "r");

    DLL sha1_dll;
    load_library(&sha1_dll, "sha1.dll");
    load_function(&sha1_dll, "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    Files staged_files = {0};
    char line[1024];
    while (fgets(line, sizeof(line), index_file)) {
        File *new_items = realloc(staged_files.items, (staged_files.count+1)*sizeof(File));
        if (!new_items) {
            fprintf(stderr, "Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        staged_files.items = new_items;

        sscanf(line, "%s %s", staged_files.items[staged_files.count].path, staged_files.items[staged_files.count].hash);
        staged_files.items[staged_files.count].status = Deleted;
        staged_files.count++;
    }

    Files repo_files = {0};
    get_repo_files(repo_path, &repo_files, ".snaptrackignore");

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha1_file(repo_files.items[i].path, hash);
        sha1_to_hex(hash, repo_files.items[i].hash);
    }

    for (int i = 0; i < staged_files.count; i++) {
        File *filei = &staged_files.items[i];
        for (int j = 0; j < repo_files.count; j++) {
            File *filer = &repo_files.items[j];
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

    print_files_by_status(staged_files, Modified);
    print_files_by_status(repo_files, New);
    print_files_by_status(staged_files, Deleted);

    free_files(&staged_files);
    free_files(&repo_files);

    free_library(&sha1_dll);
    fclose(index_file);
}

// Commit
typedef struct {
    char index_hash[SHA1_BLOCK_SIZE*2+1];
    char parent[SHA1_BLOCK_SIZE*2+1];
    char author[256];
    char message[512];
    time_t timestamp;
} Commit;

void commit_changes(const char *commit_message) {
    Commit commit = {0};

    DLL sha1_dll;
    load_library(&sha1_dll, "sha1.dll");
    load_function(&sha1_dll, "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    // Get index file hash and store object
    File index_file = {0};
    snprintf(index_file.path, PATH_SIZE, "%s\\.snaptrack\\index", REPO_PATH);
    
    char index_hash[SHA1_BLOCK_SIZE*2+1];
    unsigned char hash[SHA1_BLOCK_SIZE];
    sha1_file(index_file.path, hash);
    sha1_to_hex(hash, index_file.hash);

    strncpy(commit.index_hash, index_file.hash, sizeof(index_file.hash));

    char object_path[PATH_SIZE];
    snprintf(object_path, PATH_SIZE, "%s\\.snaptrack\\objects\\%s", REPO_PATH, index_file.hash);
    FILE *object_file = file_open(object_path, "wb");
    FILE *src_file = file_open(index_file.path, "rb");

    int c;
    while ((c = fgetc(src_file)) != EOF) {
        fputc(c, object_file);
    }

    fclose(object_file); fclose(src_file);

    // Get last commit hash
    char head_path[PATH_SIZE];
    snprintf(head_path, PATH_SIZE, "%s\\.snaptrack\\HEAD", REPO_PATH);
    FILE *head_file = file_open(head_path, "r");
    char branch_path[PATH_SIZE];
    fgets(branch_path, PATH_SIZE, head_file);
    fclose(head_file);

    char current_branch_path[PATH_SIZE];
    snprintf(current_branch_path, PATH_SIZE, "%s\\.snaptrack\\%s", REPO_PATH, branch_path);
    FILE *current_branch_file = file_open(current_branch_path, "r");
    char last_commit[SHA1_BLOCK_SIZE*2+1] = {0};
    fgets(last_commit, sizeof(last_commit), current_branch_file);

    strncpy(commit.parent, last_commit, sizeof(commit.parent));

    // Get author
    strcpy(commit.author, "not yet implemented");

    // Get message
    strncpy(commit.message, commit_message, sizeof(commit.message));

    // Get time
    commit.timestamp = time(NULL);

    // Get this commit hash and store
    char commit_content[1024];
    snprintf(commit_content, sizeof(commit_content),
                "index %s\nparent %s\nauthor %s\ndate %ld\n\n%s",
                commit.index_hash, commit.parent, commit.author,
                commit.timestamp, commit.message);
    
    File temp_commit = {0};
    snprintf(temp_commit.path, PATH_SIZE, "%s\\.snaptrack\\temp_commit", REPO_PATH);
    FILE *temp_commit_file = file_open(temp_commit.path, "w");
    fprintf(temp_commit_file, "%s", commit_content);
    fclose(temp_commit_file);

    sha1_file(temp_commit.path, hash);
    sha1_to_hex(hash, temp_commit.hash);
    
    snprintf(object_path, PATH_SIZE, "%s\\.snaptrack\\objects\\%s", REPO_PATH, temp_commit.hash);
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