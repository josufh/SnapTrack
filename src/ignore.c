#include "ignore.h"

IgnorePatterns ignore_patterns = {0};

void add_pattern(const char *pattern) {
    if (ignore_patterns.capacity == 0)
        DA_INIT(ignore_patterns, MAX_PATH);
    DA_ADD(ignore_patterns, pattern);
}

void load_ignore_patterns() {
    FILE *file = fopen(".snaptrackignore", "r");
    if (!file) return;

    char line[MAX_PATH];
    while (fgets(line, MAX_PATH, file)) {
        line[strcspn(line, "\n")] = 0;
        add_pattern(line);
    }
    fclose(file);

    add_pattern(".snaptrack\\");
}

void free_ignore_patterns() {
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