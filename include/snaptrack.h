#ifndef SNAPTRACK_H
#define SNAPTRACK_H

#include "file.h"

typedef enum {
    Init = 0,
    Status,
    Add,
    CommitChanges,
    Config,
    Revert,
    Branch,
    UnknownCommand
} Command;

Command which_command(const char *command);
void init_repository();
void stage_files(const char *to_stage);
void check_status();

typedef struct {
    char index_hash[SHA1_STRING_SIZE];
    char parent[SHA1_STRING_SIZE];
    char hash[SHA1_STRING_SIZE];
    char author_name[256];
    char author_email[256];
    char author_userid[256];
    char message[512];
    time_t timestamp;
} Commit;

void commit_changes(const char *commit_message);
void get_commit_info(Commit *commit, const char *commit_hash);
void list_commits();
void revert_commit(const char *revert_hash);
void current_branch();
void create_branch(const char *branch_name);
void list_branches();
void delete_branch(const char *branch_name_to_delete);

#endif // SNAPTRACK_H