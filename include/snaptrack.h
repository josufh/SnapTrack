#ifndef SNAPTRACK_H
#define SNAPTRACK_H

#include "file.h"

typedef enum {
    Init = 0,
    Status,
    Stage,
    Unstage,
    CommitChanges,
    Config,
    Revert,
    Branch,
    Checkout,
    Merge,
    UnknownCommand
} Command;

Command which_command(const char *command);
void init_repository();
void stage_files(const char *to_stage);
void unstage_files(const char *to_stage);
void check_status();
void commit_changes(const char *commit_message);
void list_commits();
void revert_commit(const char *revert_hash);
void create_branch(const char *branch_name);
void list_branches();
void delete_branch(const char *branch_name_to_delete);
void checkout_branch(const char *branch_name);
void merge_branch(const char *branching_name);

#endif // SNAPTRACK_H