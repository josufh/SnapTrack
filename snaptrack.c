#include <stdio.h>
#include <windows.h>
#include "snaptrack_lib.c"

/*
TODO asap
commit deleted files show message

TODO later

not commit if last commit index is same
snapshot clean (clean blobs that arent used, clean after commit)
revert -soft -hard?
no revert if last commit
refactor......

*/

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
            fprintf(stderr, "             snaptrack commit -l\n");
            return 1;
        }
        if (strcmp(argv[2], "-l") == 0) {
            list_commits();
        } else {
            commit_changes(argv[2]);
        }
        break;

    case Config:
        if (argc < 3) {
            fprintf(stdout, "SnapTrack configuration:\n");
            get_config("name");
            get_config("email");
            get_config("userid");
        }
        else if (argc < 4) {
            fprintf(stdout, "SnapTrack configuration:\n");
            get_config(argv[2]);
        } else {
            set_config(argv[2], argv[3]);
        }
        break;

    case Revert:
        if (argc < 3) {
            fprintf(stderr, "Wrong usage: snaptrack revert <commit hash>\n");
            return 1;
        }
        revert_commit(argv[2]);
        break;

    default:
        fprintf(stderr, "Unkwon command: %s\n", argv[1]);
        return 1;
    }
    return 0;
}