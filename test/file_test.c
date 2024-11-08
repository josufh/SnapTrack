#include "file.h"

// gcc -g -o file_test.exe test\file_test.c src\file.c src\ignore.c -Iinclude

int main() {
    Files files = {0};

    // make_directory(REPO_PATH, "make_directory_test");
    // create_file(REPO_PATH, "make_directory_test\\create_file_test", "content");

    // repo_must_exist(REPO_PATH);
    // check_repo_already_exists(REPO_PATH);

    get_repo_files(REPO_PATH, &files, ".snaptrackignore");
    print_files_by_status(files, New);
}