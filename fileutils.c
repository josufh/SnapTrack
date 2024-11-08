#ifndef FILEUTILS
#define FILEUTILS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <errno.h>

#define SHA1_BLOCK_SIZE 20

typedef enum {
    Staged = 0,
    New,
    Modified,
    Deleted
} FileStatus;

typedef struct {
    char path[MAX_PATH];
    char hash[SHA1_BLOCK_SIZE*2+1];
    FileStatus status;
} File;

typedef struct {
    File *items;
    int count;
} Files;

const char *file_status_string[] = {"Staged", "New", "Modified", "Deleted"};

void print_files_by_status(Files files, FileStatus status) {
    int print = 1;
    for (int i = 0; i < files.count; i++) {
        if (files.items[i].status == status) {
            if (print) {
                print = 0;
                fprintf(stdout, "%s files:\n", file_status_string[status]);
            }
            fprintf(stdout, "\t%s\n", files.items[i].path);
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
    free(files->items);
}

int wildcard_match(const char *pattern, const char *str) {
    while (*pattern)
    {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return 1;
            while (*str)
            {
                if (wildcard_match(pattern, str)) return 1;
                str++;
            }
            return 0;
        } else if (*pattern == *str) {
            pattern++;
            str++;
        } else {
            return 0;
        }
    }
    return *str == '\0';
}

int should_ignore(const char *filename, char **ignore_patterns, int ignore_count, int is_directory) {
    for (int i = 0; i < ignore_count; i++) {
        int pattern_is_directory = ignore_patterns[i][strlen(ignore_patterns[i])-1] == '\\';
        
        char full_pattern[MAX_PATH];
        if (is_directory) {
            snprintf(full_pattern, MAX_PATH, "%s\\", filename);
        } else {
            strncpy(full_pattern, filename, MAX_PATH);
        }
        
        if (is_directory && !pattern_is_directory) continue;
        if (!is_directory && pattern_is_directory) continue;

        if (wildcard_match(ignore_patterns[i], full_pattern)) {
            return 1;
        }
    }
    return 0;
}

void load_ignore_patterns(const char *ignore_file_path, char ***ignore_patterns, int *ignore_count) {
    FILE *file = fopen(ignore_file_path, "r");
    if (!file) return;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        *ignore_patterns = realloc(*ignore_patterns, (*ignore_count + 1)*sizeof(char *));
        (*ignore_patterns)[*ignore_count] = strdup(line);
        (*ignore_count)++;
    }
    fclose(file);

    *ignore_patterns = realloc(*ignore_patterns, (*ignore_count + 1)*sizeof(char *));
    (*ignore_patterns)[*ignore_count] = strdup(".snaptrack\\");
    (*ignore_count)++;
}

void get_repo_files(const char *path, Files *repo_files, const char *ignore_file_path) {
    char **ignore_patterns = NULL;
    int ignore_count = 0;
    load_ignore_patterns(ignore_file_path, &ignore_patterns, &ignore_count);
    
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

        if (should_ignore(relative_path, ignore_patterns, ignore_count, is_directory))
            continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            get_repo_files(full_path, repo_files, ignore_file_path);
        } else {
            File *new_items = realloc(repo_files->items, (repo_files->count + 1)*sizeof(File));
            if (!new_items) {
                fprintf(stderr, "Failed to allocate memory\n");
                exit(EXIT_FAILURE);
            }
            repo_files->items = new_items;
            strcpy(repo_files->items[repo_files->count].path, relative_path);
            repo_files->items[repo_files->count].status = New;
            repo_files->count++;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);

    for (int i = 0; i < ignore_count; i++) {
        free(ignore_patterns[i]);
    }
    free(ignore_patterns);
}

void check_repo_already_exists(const char *repo_path) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", repo_path);
    if (_access(path, 0) == 0) {
        fprintf(stderr, "A SnapTrack repository already exists at this location.\n");
        exit(EXIT_FAILURE);
    }
}

void repo_must_exist(const char *repo_path) {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", repo_path);
    if (_access(path, 0) != 0) {
        fprintf(stderr, "A SnapTrack repository doesn't exist at this location. Execute 'snaptrack init' before continuing.\n");
        exit(EXIT_FAILURE);
    }
}

int does_dir_exist(const char *path) {
    return _access(path, 0) == 0;
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
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error at snaptrack init\n");
        perror("Failed to create file");
        exit(EXIT_FAILURE);
    }
    if (content) fprintf(file, "%s", content);
    fclose(file);
}

#endif // FILEUTILS