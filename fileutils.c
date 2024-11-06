#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

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
        char *relative_path = _strdup(full_path+2);

        int is_directory = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (should_ignore(relative_path, ignore_patterns, ignore_count, is_directory))
            continue;

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            store_filenames(full_path, filenames, count, ignore_file_path);
        } else {
            *filenames = realloc(*filenames, (*count + 1) * sizeof(char *));
            (*filenames)[*count] = relative_path;
            (*count)++;
        }
    } while (FindNextFile(hFind, &find_data) != 0);

    FindClose(hFind);

    for (int i = 0; i < ignore_count; i++) {
        free(ignore_patterns[i]);
    }
    free(ignore_patterns);
}