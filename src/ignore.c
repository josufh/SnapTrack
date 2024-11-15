#include "ignore.h"
#include "file.h"

IgnorePatterns ignore_patterns = {0};

void add_pattern(char *pattern) {
    DA_ADD(ignore_patterns, pattern);
}

void load_ignore_patterns() {
    FILE *file = fopen(".snaptrackignore", "r");
    if (!file) return;

    char line[MAX_PATH];
    while (fgets(line, MAX_PATH, file)) {
        line[strcspn(line, "\n")] = 0;
        char *pattern = malloc_string(MAX_PATH);
        strcpy(pattern, line);
        add_pattern(pattern);
    }
    fclose(file);

    char *pattern = malloc_string(MAX_PATH);
    strcpy(pattern, ".snaptrack\\");
    add_pattern(pattern);
}

void free_ignore_patterns() {
    DA_FOREACH(ignore_patterns, pattern, char *) {
        free(pattern);
    }
    DA_FREE(ignore_patterns);
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

char *get_pattern_at_index(IgnorePatterns patterns, size_t index) {
    return (char *)DA_GET(patterns, index);
}

int should_ignore(const char *filename, IgnorePatterns patterns, int is_dir) {
    for (int i = 0; i < patterns.count; i++) {
        const char *pattern = get_pattern_at_index(patterns, i);
        
        char full_path[MAX_PATH];
        if (is_dir) {
            snprintf(full_path, MAX_PATH, "%s\\", filename);
        } else {
            strncpy(full_path, filename, MAX_PATH);
        }

        if (wildcard_match(pattern, full_path)) {
            return 1;
        }
    }
    return 0;
}