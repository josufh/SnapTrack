#include <stdio.h>
#include "snaptrack.h"
#include "config.h"
#include "print.h"

/*
TODO asap
revert add

TODO later
not commit if last commit index is same
snapshot clean (clean blobs that arent used, clean after commit)
revert -soft -hard?
no revert if last commit
redmine(進捗管理)


*/

int main(int argc, char *argv[]) {
    if (argc < 2)
        exit_error("show error message");

    switch (which_command(argv[1])) {
    case Init:
        init_repository();
        break;
    
    case Status:
        check_status();
        break;

    case Add:
        if (argc < 3) {
            exit_error("Wrong usage: snaptrack add <file/dir name>\n");
        }
        stage_files(argv[2]);
        break;

    case CommitChanges:
        if (argc < 3) {
            exit_error("Wrong usage: snaptrack commit \"commit message...\"\n             snaptrack commit -l\n");
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
            exit_error("Wrong usage: snaptrack revert <commit hash>\n");
            return 1;
        }
        revert_commit(argv[2]);
        break;

    case Branch:
        if (argc < 3) {
            current_branch();
            break;
        }
        if (strcmp(argv[2], "-l") == 0) {
            list_branches();
        } else if (strcmp(argv[2], "-d") == 0 && argc > 3) {
            delete_branch(argv[3]);
        } else {
            create_branch(argv[2]);
        }
        break;

    default:
        exit_error("Unkwon command: %s\n", argv[1]);
    }
    cleanup_cabinet();
    return 0;
}