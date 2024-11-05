#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_FILES 1000

void store_filenames(const char *path, char ***filenames, int *count) {
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

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            store_filenames(full_path, filenames, count);
        } else {
            *filenames = realloc(*filenames, (*count + 1) * sizeof(char *));
            (*filenames)[*count] = _strdup(full_path);
            (*count)++;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);
}