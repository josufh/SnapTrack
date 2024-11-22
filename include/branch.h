#ifndef BRANCH_H
#define BRANCH_H

#include "dynamic_array.h"
#include "path.h"
#include "file.h"

#define WORKING_BRANCH get_current_branch_name()

typedef DynamicArray Branches;
#define foreach_branch(branches, branch) DA_FOREACH((*branches), branch, char *)
#define add_branch(branches, branch) DA_ADD((*branches), branch)

Branches *get_branches();
void free_branches(Branches *branches);
int does_branch_exist(const char *branch_name);
char *get_branch_commit_hash(const char* branch);
Files *get_branch_index_files(const char *branch);
void change_head(const char *branch);
char *get_current_branch_name();
char *get_current_branch_path();

#endif // BRANCH_H