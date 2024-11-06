#include <stdio.h>
#include <windows.h>
#include "snaptrack_lib.c"
#include "stringutils.c"

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

    default:
        fprintf(stderr, "Unkwon command: %s\n", argv[1]);
        return 1;
    }
    return 0;
}