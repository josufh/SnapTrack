#include <direct.h>
#include <io.h>
#include <windows.h>
#include "file.h"

Files index_files = {0}, path_files = {0};

int is_same_string(const char *string1, const char *string2) {
    return strcmp(string1, string2) == 0;
}

File *get_file_at_index(Files files, size_t index) {
    return (File *)DA_GET(files, index);
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

void get_files_from_path(const char *path) { 
    if (path[strlen(path)-1] != '\\' && *path != '.') {
        File new_file = {0};
        strcpy(new_file.path, path);
        new_file.status = New;
        new_file.staged = 0;
        DA_ADD(path_files, &new_file);
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
            get_files_from_path(new_path);
            continue;
        }

        File new_file = {0};
        strcpy(new_file.path, file_path);
        new_file.status = New;
        DA_ADD(path_files, &new_file);
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

void add_file(Files *files, File *file) {
    DA_ADD(*files, file);
}

void load_index_files(const char *path, Files *files) {
    FILE *index_file = file_open(path, "r");
    
    char line[1024];
    while (fgets(line, 1024, index_file)) {
        File new_file = {0};
        sscanf(line, "%s %s %d", new_file.path, new_file.hash, (int *)&(new_file.status));
        new_file.staged = 0;
        
        add_file(files, &new_file);
    }

    fclose(index_file);
}

void init_index_files(const char *path) {
    load_index_files(path, &index_files);
}

void free_index_files(Files *files) {
    free_files(files);
}

void init_path_files(const char *path) {
    load_ignore_patterns();
    get_files_from_path(path);
    free_ignore_patterns();

    init_sha_file();

    foreach_file(path_files, file) {
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

void free_path_files() {
    free_files(&path_files);
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

void path_must_be_valid(const char *path) {
    if (!does_dir_exist(path)) {
        fprintf(stderr, "File or directory %s does not exist\n", path);
        exit(EXIT_FAILURE);
    }
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

int is_directory(const char *path) {
    return path[strlen(path)-1] == '\\';
}