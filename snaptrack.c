#include <stdio.h>
#include <windows.h>

typedef void (*InitRepositoryFunc)(const char *);
typedef void (*StageFilesFunc)(const char *);
typedef void (*CheckStatusFunc)(const char *);

int main(int argc, char *argv[]) {
    if (strcmp(argv[1], "init") == 0) {
        HINSTANCE hLib = LoadLibrary("snaptrack_lib.dll");
        if (!hLib) {
            fprintf(stderr, "Failed to load snaptrack_lib.dll\n");
            return 1;
        }

        InitRepositoryFunc init_repository = (InitRepositoryFunc)GetProcAddress(hLib, "init_repository");
        if (!init_repository) {
            fprintf(stderr, "Failed to find init_repository in snaptrack_lib.dll\n");
            FreeLibrary(hLib);
            return 1;
        }

        init_repository(".");
        FreeLibrary(hLib);

    } else if (strcmp(argv[1], "stage") == 0) {
        HINSTANCE hLib = LoadLibrary("snaptrack_lib.dll");
        if (!hLib) {
            fprintf(stderr, "Failed to load snaptrack_lib.dll\n");
            return 1;
        }

        StageFilesFunc stage_files = (StageFilesFunc)GetProcAddress(hLib, "stage_files");
        if (!stage_files) {
            fprintf(stderr, "Failed to find stage_file in snaptrack_lib.dll\n");
            FreeLibrary(hLib);
            return 1;
        }

        stage_files(".");

        FreeLibrary(hLib);

    } else if (strcmp(argv[1], "status") == 0) {
        HINSTANCE hLib = LoadLibrary("snaptrack_lib.dll");
        if (!hLib) {
            fprintf(stderr, "Failed to load snaptrack_lib.dll\n");
            return 1;
        }

        CheckStatusFunc check_status = (CheckStatusFunc)GetProcAddress(hLib, "check_status");
        if (!check_status) {
            fprintf(stderr, "Failed to find check_status in snaptrack_lib.dll\n");
            FreeLibrary(hLib);
            return 1;
        }

        check_status(".");

        FreeLibrary(hLib);

    } else {
        fprintf(stderr, "Unkwon command: %s\n", argv[1]);
        return 1;
    }
    return 0;
}