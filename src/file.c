#ifndef FILEUTILS
#define FILEUTILS

#include <direct.h>
#include <io.h>
#include <windows.h>
#include "file.h"
#include "ignore.h"

void load_library(DLL *dll, const char *dll_path) {
    dll->handle = LoadLibrary(dll_path);
    if (!dll->handle) {
        fprintf(stderr, "Failed to load %s\n", dll_path);
        exit(EXIT_FAILURE);
    }
}

void load_function(DLL *dll, const char *library_name, const char *function_name) {
    load_library(dll, library_name);
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

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_STRING_SIZE]) {
    for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
        sprintf(output + (i*2), "%02x", hash[i]);
    }
    output[SHA1_BLOCK_SIZE*2] = '\0';
}

int is_same_string(const char *string1, const char *string2) {
    return strcmp(string1, string2) == 0;
}

File *get_file_at_index(Files files, size_t index) {
    return (File *)DA_GET(files, index);
}

const char *file_status_string[] = {"Staged", "New", "Modified", "Deleted"};

void print_files_by_status(Files files, FileStatus status) {
    int print = 1;
    for (int i = 0; i < files.count; i++) {
        File *file = get_file_at_index(files, i);
        if (file->status == status) {
            if (print) {
                print = 0;
                fprintf(stdout, "%s files:\n", file_status_string[status]);
            }
            fprintf(stdout, "\t%s\n", file->path);
        } 
    }
    if (!print) fprintf(stdout, "\n");
}

FILE *file_open(const char* filepath, const char* mode) {
    FILE *file = fopen(filepath, mode);
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        exit(EXIT_FAILURE);
    }
    return file;
}

void free_files(Files *files) {
    DA_FREE(*files);
}

void get_repo_files(const char *path, Files *repo_files, const char *ignore_file_path) {
    IgnorePatterns ignore_patterns = {0};
    load_ignore_patterns(&ignore_patterns, ignore_file_path);
    
    WIN32_FIND_DATA find_data;
    HANDLE hFind;

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", path);

    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Failed to open directory");
        return;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;
        
        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", path, find_data.cFileName);
        char *relative_path = _strdup(full_path+2);

        int is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (should_ignore(relative_path, &ignore_patterns, is_directory))
            continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            get_repo_files(full_path, repo_files, ignore_file_path);
        } else {
            File new_file = {0};
            strcpy(new_file.path, relative_path);
            new_file.status = New;

            DA_ADD(*repo_files, &new_file);
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);

    DA_FREE(ignore_patterns);
}

void get_index_files(const char *index_path, Files *index_files, FileStatus status) {
    FILE *index_file = file_open(index_path, "r");
    
    char line[MAX_PATH];
    while (fgets(line, MAX_PATH, index_file)) {
        File new_file = {0};
        sscanf(line, "%s %s", new_file.path, new_file.hash);
        new_file.status = status;
        
        DA_ADD(*index_files, &new_file);
    }

    fclose(index_file);
}

int does_dir_exist(const char *path) {
    return _access(path, 0) == 0;
}

void check_repo_already_exists(const char *repo_path) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", repo_path);
    if (does_dir_exist(path)) {
        fprintf(stderr, "A SnapTrack repository already exists at this location.\n");
        exit(EXIT_FAILURE);
    }
}

void repo_must_exist(const char *repo_path) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", repo_path);
    if (!does_dir_exist(path)) {
        fprintf(stderr, "A SnapTrack repository doesn't exist at this location. Execute 'snaptrack init' before continuing.\n");
        exit(EXIT_FAILURE);
    }
}

void make_directory(const char *repo_path, const char *subdir) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack\\%s", repo_path, subdir);
    if (_mkdir(path) != 0 && errno != EEXIST) {
        fprintf(stderr, "Error at snaptrack init\n");
        perror("Failed to create directory");
        exit(EXIT_FAILURE);
    }
}

void create_file(const char *repo_path, const char *subpath, const char *content) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack\\%s", repo_path, subpath);
    FILE *file = file_open(path, "w");
    if (content) fprintf(file, "%s", content);
    fclose(file);
}

#endif // FILEUTILS