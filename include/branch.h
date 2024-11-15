#ifndef BRANCH_H
#define BRANCH_H

#include "dynamic_array.h"
#include "path.h"
#include "file.h"

typedef DynamicArray Branches;
#define foreach_branch(branches, branch) DA_FOREACH(branches, branch, char *)
#define free_branches(branches) DA_FREE(branches)
#define add_branch(branches, branch) DA_ADD(branches, branch)

int does_branch_exist(const char *branch_name);
void get_branch_commit_hash(const char* branch, char *hash);
void get_branch_index_files(const char *branch, Files *index_files);
void change_head(const char *branch);

#endif // BRANCH_H