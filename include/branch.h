#ifndef BRANCH_H
#define BRANCH_H

#include "dynamic_array.h"

typedef DynamicArray Branches;
#define foreach_branch(branches, branch) DA_FOREACH(branches, branch, char *)



#endif // BRANCH_H