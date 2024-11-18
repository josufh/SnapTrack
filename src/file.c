#include <direct.h>
#include <io.h>
#include <windows.h>
#include "print.h"

Cabinet cabinet = {0};

Files *new_files_entry() {
    Files *files = (Files *)malloc(sizeof(Files));
    if (!files)
        exit_error("Error allocating memory @ new_files_entry\n");

    DA_INIT(*files);
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

void init_file(File *file) {
    file->staged = 0;
    file->status = Unchanged;
    file->path = NULL;
    file->hash = NULL;
}

File *malloc_file() {
    File *file = (File *)malloc(sizeof(File));
    if (!file)
        exit_error("Error allocating memory @ malloc_file\n");
    
    init_file(file);
    return file;
}

FILE *file_open(const char* filepath, const char* mode) {
    FILE *file = fopen(filepath, mode);
    if (!file)
        exit_error("Failed to open file: %s\n", filepath);
        
    return file;
}

void get_files_from_path(const char *path, Files *files) {
    if (path[strlen(path)-1] != '\\' && *path != '.') {
        File *new_file = malloc_file();
        new_file->path = new_path(path);
        new_file->status = New;
        new_file->staged = 0;
        DA_ADD(*files, new_file);
        return;
    }

    char *search_path = new_path("%s\\*", path);

    WIN32_FIND_DATA find_data;
    HANDLE hFind;
    hFind = FindFirstFile(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        exit_error("Failed to open directory\n");
    }

    do {
        if (same_string(find_data.cFileName, ".") || same_string(find_data.cFileName, ".."))
            continue;

        char *temp_path = same_string(path, ".") ? new_path("%s", find_data.cFileName) : new_path("%s%s", path, find_data.cFileName);
        
        char *file_path = temp_path;
        if (strncmp(temp_path, ".\\", 2) == 0)
            file_path = temp_path + 2;

        int is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        if (should_ignore(file_path, ignore_patterns, is_dir))
            continue;

        if (is_dir) {
            char *new_path_ = new_path("%s\\%s\\", path, find_data.cFileName);
            get_files_from_path(new_path_, files);
            continue;
        }

        File *new_file = malloc_file();
        new_file->path = new_path(file_path);
        new_file->status = New;
        new_file->staged = 0;
        DA_ADD(*files, new_file);
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}

Files *get_path_files(const char *path) {
    load_ignore_patterns();
    Files *files = new_files_entry();
    get_files_from_path(path, files);
    free_ignore_patterns();

    foreach_file(files, file) {
        file->staged = 0;
        if (!does_dir_exist(file->path)) {
            file->status = Deleted;
        } else {
            file->hash = new_hash(file->path);
        }
    }
    return files;
}

Files *get_index_files(const char *path) {
    Files *files = new_files_entry();

    FILE *index_file = file_open(path, "r");
    
    char line[1024];
    while (fgets(line, 1024, index_file)) {
        File *new_file = malloc_file();
        new_file->path = new_path("");
        new_file->hash = new_hash(NULL);

        sscanf(line, "%s %s %d", new_file->path, new_file->hash, (int *)&(new_file->status));
        new_file->staged = 0;
        DA_ADD(*files, new_file);
    }

    fclose(index_file);

    return files;
}

void copy_file(const char *dest, const char *src) {
    FILE *dest_file = file_open(dest, "wb");
    FILE *src_file = file_open(src, "rb");

    size_t buffer_size = 4096;
    size_t bytes_read;
    char buffer[buffer_size];
    while ((bytes_read = fread(buffer, 1, buffer_size, src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }

    fclose(dest_file);
    fclose(src_file);
}

void create_object(File file) {
    char *object_path = new_path("%s%s", OBJECTS_PATH, file.hash);
    copy_file(object_path, file.path);
}

int does_dir_exist(const char *path) {
    return _access(path, 0) == 0;
}

void path_must_exist(const char *path) {
    if (!does_dir_exist(path))
        exit_error("File or directory %s does not exist\n", path);
}

void check_repo_already_exists() {
    char *path = new_path("%s\\.snaptrack", REPO_PATH);
    if (does_dir_exist(path))
        exit_error("A SnapTrack repository already exists at this location.\n");
}

void repo_must_exist() {
    char *path = new_path("%s\\.snaptrack", REPO_PATH);
    if (!does_dir_exist(path))
        exit_error("A SnapTrack repository doesn't exist at this location. Execute 'snaptrack init' before continuing.\n");
}

void make_directory(const char *path) {
    char *full_path = new_path("%s\\.snaptrack\\%s", REPO_PATH, path);
    if (_mkdir(full_path) != 0 && errno != EEXIST) {
        exit_error("Error creating directory %s\n", path);
    }
}

void create_file(const char *path, const char *content) {
    char *full_path = new_path("%s\\.snaptrack\\%s", REPO_PATH, path);
    FILE *file = file_open(full_path, "w");
    if (content) fprintf(file, "%s", content);
    fclose(file);
}

int is_directory(const char *path) {
    if (!path)
        exit_error("is_directory got a NULL path\n");

    return path[strlen(path)-1] == '\\';
}