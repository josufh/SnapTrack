#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>
#include "fileutils.c"
#include "stringutils.c"

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

void init_repository(const char *repo_path) {
    check_repo_exists(repo_path);
    make_directory(repo_path, "");
    make_directory(repo_path, "objects");
    make_directory(repo_path, "refs");
    make_directory(repo_path, "refs\\heads");

    create_file(repo_path, "HEAD", "ref: refs\\heads\\main\n");
    create_file(repo_path, "index", NULL);

    fprintf(stdout, "Initialized local empty SnapTrack repository\n");
}

void stage_files(const char *repo_path) {
    DLL sha1_dll;
    load_library(&sha1_dll, "sha1.dll");
    load_function(&sha1_dll, "sha1_file");
    SHA1FileFunc sha1_file = (SHA1FileFunc)sha1_dll.func;

    Files repo_files = {0};
    get_repo_files(repo_path, &repo_files, ".snaptrackignore");

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha1_file(repo_files.items[i].filename, hash);
        sha1_to_hex(hash, repo_files.items[i].blob_hash);
    }

    char index_path[MAX_PATH];
    char temp_path[MAX_PATH];
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

        sscanf(line, "%s %s", staged_files.items[staged_files.count].filename, staged_files.items[staged_files.count].blob_hash);
        staged_files.items[staged_files.count].status = Deleted;
        staged_files.count++;
    }

    fclose(index_file);


    for (int i = 0; i < staged_files.count; i++) {
        File *filei = &staged_files.items[i];
        for (int j = 0; j < repo_files.count; j++) {
            File *filer = &repo_files.items[j];
            if (is_same_string(filei->filename, filer->filename)) {
                if (is_same_string(filei->blob_hash, filer->blob_hash)) {
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

    char object_path[MAX_PATH];
    FILE *temp_file = file_open(temp_path, "w");

    for (int i = 0; i < repo_files.count; i++) {
        File file = repo_files.items[i];
        if (file.status == Modified || file.status == New) {
            snprintf(object_path, sizeof(object_path), "%s\\.snaptrack\\objects\\%s", repo_path, file.blob_hash);
            FILE *object_file = file_open(object_path, "wb");
            FILE *src_file = file_open(file.filename, "rb");

            int c;
            while ((c = fgetc(src_file)) != EOF) {
                fputc(c, object_file);
            }

            fclose(object_file); fclose(src_file);

            fprintf(stdout, "Stated changes for file %s with hash %s\n", file.filename, file.blob_hash);
        }
        fprintf(temp_file, "%s %s\n", file.filename, file.blob_hash);

    }
    fclose(temp_file);
    remove(index_path);
    rename(temp_path, index_path);

    free_files(&staged_files);
    free_files(&repo_files);

    free_library(&sha1_dll);
}

void check_status(const char *repo_path) {
    char index_path[MAX_PATH];
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

        sscanf(line, "%s %s", staged_files.items[staged_files.count].filename, staged_files.items[staged_files.count].blob_hash);
        staged_files.items[staged_files.count].status = Deleted;
        staged_files.count++;
    }

    Files repo_files = {0};
    get_repo_files(repo_path, &repo_files, ".snaptrackignore");

    for (int i = 0; i < repo_files.count; i++) {
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha1_file(repo_files.items[i].filename, hash);
        sha1_to_hex(hash, repo_files.items[i].blob_hash);
    }

    for (int i = 0; i < staged_files.count; i++) {
        File *filei = &staged_files.items[i];
        for (int j = 0; j < repo_files.count; j++) {
            File *filer = &repo_files.items[j];
            if (is_same_string(filei->filename, filer->filename)) {
                if (is_same_string(filei->blob_hash, filer->blob_hash)) {
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

typedef enum {
    Init = 0,
    Status,
    Stage,
    UnknownCommand
} Command;

Command which_command(const char *command) {
    if (is_same_string(command, "init")) return Init;
    else if (is_same_string(command, "status")) return Status;
    else if (is_same_string(command, "stage")) return Stage;
    else return UnknownCommand;
}