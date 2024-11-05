#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int wildcard_match(const char *pattern, const char *str) {
    while (*pattern) {
        if (*pattern == '*') {
            pattern++;
            if (!*pattern) return 1;  // Trailing * matches everything
            while (*str) {
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

int should_ignore(const char *filename, char **ignore_patterns, int ignore_count) {
    for (int i = 0; i < ignore_count; i++) {
        if (wildcard_match(ignore_patterns[i], filename)) {
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
        line[strcspn(line, "\n")] = 0;  // Remove newline
        *ignore_patterns = realloc(*ignore_patterns, (*ignore_count + 1) * sizeof(char *));
        (*ignore_patterns)[*ignore_count] = strdup(line);
        (*ignore_count)++;
    }
    fclose(file);
}

void store_filenames(const char *path, char ***filenames, int *count, const char *ignore_file) {
    // Load ignore patterns from the ignore file
    char **ignore_patterns = NULL;
    int ignore_count = 0;
    load_ignore_patterns(ignore_file, &ignore_patterns, &ignore_count);

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

        if (should_ignore(full_path, ignore_patterns, ignore_count))
            continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            store_filenames(full_path, filenames, count, ignore_file);
        } else {
            *filenames = realloc(*filenames, (*count + 1) * sizeof(char *));
            (*filenames)[*count] = _strdup(full_path);
            (*count)++;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);

    // Free ignore patterns
    for (int i = 0; i < ignore_count; i++) {
        free(ignore_patterns[i]);
    }
    free(ignore_patterns);
}

*/

void store_filenames(const char *path, char ***filenames, int *count, const char *ignore_file_path) {
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