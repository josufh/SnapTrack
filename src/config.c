#include <stdio.h>
#include <stdlib.h>
#include <minwindef.h>
#include <direct.h>
#include "config.h"
#include "file.h"

char *get_config_file_path() {
    char *app_data_path = getenv("APPDATA");
    if (!app_data_path) {
        fprintf(stderr, "Error: APPDATA environment variable not found\n");
        exit(EXIT_FAILURE);
    }

    char config_path[MAX_PATH];
    snprintf(config_path, MAX_PATH, "%s\\SnapTrack", app_data_path);
    char *config_file_path = (char *)malloc(MAX_PATH);
    snprintf(config_file_path, MAX_PATH, "%s\\config", config_path);
    
    if (!does_dir_exist(config_path)) {
        _mkdir(config_path);
        FILE *config_file = file_open(config_file_path, "w");
        fputs(CONFIG_INITIAL_VALUE, config_file);
        fclose(config_file);
    }

    return config_file_path;
}

void set_config(const char* element, const char *new_value) {
    char *config_file_path = get_config_file_path();
    FILE *config_file = file_open(config_file_path, "r");

    char lines[CONFIG_ELEMENT_COUNT][255] = {0};
    int line_count = 0;
    if (config_file) {
        while (fgets(lines[line_count], 255, config_file)) {
            char key[255], old_value[255];
            sscanf(lines[line_count], "%s %s", key, old_value);
            if (strcmp(key, element) == 0) {
                snprintf(lines[line_count], 255, "%s %s\n", key, new_value);
            }
            line_count++;
        }
        fclose(config_file);
    }

    config_file = file_open(config_file_path, "w");
    for (int i = 0; i < line_count; i++) {
        fputs(lines[i], config_file);
    }
    fclose(config_file);
    free(config_file_path);
}

void get_config(const char* element) {
    char *config_file_path = get_config_file_path();
    FILE *config_file = file_open(config_file_path, "r");

    char line[255] = {0};
    if (config_file) {
        while (fgets(line, 255, config_file)) {
            char key[255], value[255];
            sscanf(line, "%s %s", key, value);
            if (strcmp(key, element) == 0) {
                fprintf(stdout, "\t%s = %s\n", element, value);
                break;
            }
        }
        fclose(config_file);
    }
    free(config_file_path);
}

void get_user_info(User *user) {
    char *config_file_path = get_config_file_path();
    FILE *config_file = file_open(config_file_path, "r");

    char line[255] = {0};
    if (config_file) {
        while (fgets(line, 255, config_file)) {
            char key[255], value[255];
            sscanf(line, "%s %s", key, value);
            if (strcmp(key, "name") == 0) {
                strcpy(user->name, value);
            } else if (strcmp(key, "email") == 0) {
                strcpy(user->email, value);
            } else if (strcmp(key, "userid") == 0) {
                strcpy(user->userid, value);
            }
        }
        fclose(config_file);
    }
    free(config_file_path);
}