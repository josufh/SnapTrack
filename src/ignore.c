#include "ignore.h"

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

char *get_pattern_at_index(IgnorePatterns patterns, size_t index) {
    return (char *)DA_GET(patterns, index);
}

int should_ignore(const char *filename, IgnorePatterns *patterns, int is_directory) {
    for (int i = 0; i < patterns->count; i++) {
        const char *pattern = get_pattern_at_index(*patterns, i);
        int pattern_is_directory = pattern[strlen(pattern)-1] == '\\';
        
        char full_path[MAX_PATH];
        if (is_directory) {
            snprintf(full_path, MAX_PATH, "%s\\", filename);
        } else {
            strncpy(full_path, filename, MAX_PATH);
        }
        
        if ((is_directory && !pattern_is_directory) || (!is_directory && pattern_is_directory)) continue;

        if (wildcard_match(pattern, full_path)) {
            return 1;
        }
    }
    return 0;
}

void add_pattern(IgnorePatterns *patterns, const char *pattern) {
    if (patterns->capacity == 0) DA_INIT(*patterns, MAX_PATH);
    DA_ADD(*patterns, pattern);
}

void load_ignore_patterns(IgnorePatterns *patterns, const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) return;

    char line[MAX_PATH];
    while (fgets(line, MAX_PATH, file)) {
        line[strcspn(line, "\n")] = 0;
        add_pattern(patterns, line);
    }
    fclose(file);

    add_pattern(patterns, ".snaptrack\\");
}