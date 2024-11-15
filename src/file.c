#include <direct.h>
#include <io.h>
#include <windows.h>
#include "print.h"

Cabinet cabinet = {0};

Files *new_files_entry() {
    Files *files = (Files *)malloc(sizeof(Files));
    files->items = NULL;
    files->capacity = 0;
    files->count = 0;
    DA_ADD(cabinet, files);
    return files;
}

void cleanup_cabinet() {
    foreach_files(cabinet, files) {
        DA_FREE(*files);
        free(files);
    }
    DA_FREE(cabinet);
}

int is_same_string(const char *string1, const char *string2) {
    return strcmp(string1, string2) == 0;
}

File *malloc_file() {
    return (File *)malloc(sizeof(File));
}

char *malloc_string(size_t size) {
    return (char *)malloc(size);
}

FILE *file_open(const char* filepath, const char* mode) {
    FILE *file = fopen(filepath, mode);
    if (!file) {
        exit_error("Failed to open file: %s\n", filepath);
    }
    return file;
}

void get_files_from_path(const char *path, Files *files) {
    if (path[strlen(path)-1] != '\\' && *path != '.') {
        File *new_file = malloc_file();
        strcpy(new_file->path, path);
        new_file->status = New;
        new_file->staged = 0;
        DA_ADD(*files, new_file);
        return;
    }

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", path);

    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        perror("Failed to open directory");
        return;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            continue;

        char temp_path[MAX_PATH];
        if (strcmp(path, ".") == 0)
            snprintf(temp_path, MAX_PATH, "%s", find_data.cFileName);
        else
            snprintf(temp_path, MAX_PATH, "%s%s", path, find_data.cFileName);

        char *file_path = temp_path;
        if (strncmp(temp_path, ".\\", 2) == 0)
            file_path = temp_path + 2;

        int is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (should_ignore(file_path, ignore_patterns, is_dir))
            continue;

        if (is_dir) {
            char new_path[MAX_PATH];
            snprintf(new_path, MAX_PATH, "%s\\%s\\", path, find_data.cFileName);
            get_files_from_path(new_path, files);
            continue;
        }

        File *new_file = malloc_file();
        strcpy(new_file->path, file_path);
        new_file->status = New;
        new_file->staged = 0;
        DA_ADD(*files, new_file);
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

void load_path_files(const char *path, Files *files) {
    load_ignore_patterns();
    get_files_from_path(path, files);
    free_ignore_patterns();

    init_sha_file();

    foreach_file(*files, file) {
        file->staged = 0;
        if (!does_dir_exist(file->path)) {
            file->status = Deleted;
        } else {
            unsigned char hash[SHA1_BLOCK_SIZE];
            sha_file(file->path, hash);
            sha1_to_hex(hash, file->hash);
        }
    }
    
    free_sha_file();
}

void load_index_files(const char *path, Files *files) {
    FILE *index_file = file_open(path, "r");
    
    char line[1024];
    while (fgets(line, 1024, index_file)) {
        File *new_file = malloc_file();
        sscanf(line, "%s %s %d", new_file->path, new_file->hash, (int *)&(new_file->status));
        new_file->staged = 0;
        DA_ADD(*files, new_file);
    }

    fclose(index_file);
}

void create_object(File file) {
    char object_path[MAX_PATH];
    snprintf(object_path, MAX_PATH, "%s\\.snaptrack\\objects\\%s", REPO_PATH, file.hash);
    FILE *object_file = file_open(object_path, "wb");
    FILE *src_file = file_open(file.path, "rb");

    int c;
    while ((c = fgetc(src_file)) != EOF) {
        fputc(c, object_file);
    }

    fclose(object_file); fclose(src_file);
}

int does_dir_exist(const char *path) {
    return _access(path, 0) == 0;
}

void path_must_exist(const char *path) {
    if (!does_dir_exist(path)) {
        fprintf(stderr, "File or directory %s does not exist\n", path);
        exit(EXIT_FAILURE);
    }
}

void check_repo_already_exists() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", REPO_PATH);
    if (does_dir_exist(path)) {
        exit_error("A SnapTrack repository already exists at this location.\n");
    }
}

void repo_must_exist() {
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\.snaptrack", REPO_PATH);
    if (!does_dir_exist(path)) {
        fprintf(stderr, "A SnapTrack repository doesn't exist at this location. Execute 'snaptrack init' before continuing.\n");
        exit(EXIT_FAILURE);
    }
}

void make_directory(const char *path) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s\\.snaptrack\\%s", REPO_PATH, path);
    if (_mkdir(full_path) != 0 && errno != EEXIST) {
        exit_error("Error creating directory %s\n", path);
    }
}

void create_file(const char *path, const char *content) {
    char full_path[MAX_PATH];
    snprintf(full_path, sizeof(full_path), "%s\\.snaptrack\\%s", REPO_PATH, path);
    FILE *file = file_open(full_path, "w");
    if (content) fprintf(file, "%s", content);
    fclose(file);
}

int is_directory(const char *path) {
    return path[strlen(path)-1] == '\\';
}