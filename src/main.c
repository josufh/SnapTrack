#include <stdio.h>
#include "snaptrack.h"
#include "config.h"
#include "print.h"
#include "branch.h"

int main(int argc, char *argv[]) {
    if (argc < 2)
        exit_error("show snaptrack usage\n");
    init_sha_file();
    switch (which_command(argv[1])) {
    case Init:
        init_repository();
        break;
    
    case Status:
        check_status();
        break;

    case Stage:
        if (argc < 3) {
            exit_error("Wrong usage: snaptrack stage <file/dir name>\n");
        }
        stage_files(argv[2]);
        break;

    case Unstage:
        if (argc < 3) {
            exit_error("Wrong usage: snaptrack unstage <file/dir name>\n");
        }
        unstage_files(argv[2]);
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
        }
        revert_commit(argv[2]);
        break;

    case Branch:
        if (argc < 3) {
            print_out(White, "Current branch: %s\n", WORKING_BRANCH);

        } else if (strcmp(argv[2], "-l") == 0) {
            list_branches();

        } else if (strcmp(argv[2], "-d") == 0) {
            if (argc > 3)
                delete_branch(argv[3]);
            else
                exit_error("Wrong usage: snaptrack branch -d <branch>");

        } else {
            create_branch(argv[2]);
        }
        break;

    case Checkout:
        if (argc < 3) {
            list_branches();
        } else {
            checkout_branch(argv[2]);
        }
        break;

    default:
        exit_error("Unkwon command: %s\n", argv[1]);
    }
    free_sha_file();
    cleanup_cabinet();
    cleanup_paths();
    return 0;
}