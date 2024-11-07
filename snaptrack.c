#include <stdio.h>
#include <windows.h>
#include "snaptrack_lib.c"

int main(int argc, char *argv[]) {
    if (argc < 2)
        return 1;
    
    const char *repo_path = ".";

    switch (which_command(argv[1])) {
    case Init:
        init_repository(repo_path);
        break;
    
    case Status:
        check_status(repo_path);
        break;

    case Stage:
        stage_files(repo_path);
        break;

    case CommitChanges:
        if (argc < 3) {
            fprintf(stderr, "Wrong usage: snaptrack commit \"commit message...\"\n");
            return 1;
        }
        commit_changes(argv[2]);
        break;

    default:
        fprintf(stderr, "Unkwon command: %s\n", argv[1]);
        return 1;
    }
    return 0;
}