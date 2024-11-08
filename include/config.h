#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char name[255];
    char email[255];
    char userid[255];
} User;

#define CONFIG_ELEMENT_COUNT 3
#define CONFIG_INITIAL_VALUE "name not_set\nemail not@set.com\nuserid 00000000\n"

char *get_config_file_path();
void set_config(const char* element, const char *new_value);
void get_config(const char* element);
void get_user_info(User *user);

#endif // CONFG_H