#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>
#include <time.h>
#include "print.h"
#include "config.h"
#include "snaptrack.h"
#include "ignore.h"
#include "branch.h"
#include "commit.h"

Command which_command(const char *command) {
    if (same_string(command, "init")) return Init;
    else if (same_string(command, "status")) return Status;
    else if (same_string(command, "stage")) return Stage;
    else if (same_string(command, "unstage")) return Unstage;
    else if (same_string(command, "commit")) return CommitChanges;
    else if (same_string(command, "config")) return Config;
    else if (same_string(command, "revert")) return Revert;
    else if (same_string(command, "branch")) return Branch;
    else if (same_string(command, "checkout")) return Checkout;
    else return UnknownCommand;
}

// Init
void init_repository() {
    check_repo_already_exists(REPO_PATH);
    
    make_directory("");
    make_directory("objects");
    make_directory("refs");
    make_directory("refs\\branches");
    
    create_file("refs\\branches\\main", "");
    create_file("HEAD", "refs\\branches\\main");
    create_file("index", NULL);

    print_out(Green, "Initialized local empty SnapTrack repository\n");
}

// Status
void check_status() {
    repo_must_exist(REPO_PATH);

    Files *repo_files = get_path_files(REPO_PATH);
    Files *index_files = get_index_files(INDEX_PATH);
    Files *last_files = get_branch_index_files(get_current_branch_name());

    foreach_file(last_files, last_file) {
        foreach_file(repo_files, repo_file) {
            if (same_path(last_file, repo_file)) {
                last_file->staged = 1;

                if (same_hash(last_file, repo_file)) {
                    // File is unchanged
                    last_file->status = Unchanged; repo_file->status = Unchanged;

                } else {
                    // File was modified
                    last_file->status = Modified; repo_file->status = Modified;
                    foreach_file(index_files, index_file) 
                        if (same_file(repo_file, index_file))
                            // Modification is staged
                            repo_file->staged = 1;
                }
            }
        }
        if (!last_file->staged && !does_dir_exist(last_file->path)) {
            // File was deleted
            last_file->status = Deleted;

            foreach_file(index_files, index_file)
                if (same_path(last_file, index_file) && index_file->status == Deleted)
                    // Deletion is staged
                    last_file->staged = 1;
        }
    }

    foreach_file(repo_files, repo_file)
        // For each new file
        if (repo_file->status == New)
            foreach_file(index_files, index_file)
                // Check for staging status
                if (same_file(repo_file, index_file))
                    repo_file->staged = 1;

    const char *file_status_string[] = { "Unchanged", "New", "Modified", "Deleted" };

    print_out(White, "Staged for commit:\n");
    foreach_file(index_files, index_file) {
        if (index_file->status != Unchanged)
            print_out(Green, "\t%s: %s\n", file_status_string[index_file->status], index_file->path);
    }

    print_out(White, "\nNot staged:\n");
    foreach_file(repo_files, repo_file) {
        if (repo_file->status != Unchanged && !repo_file->staged)
            print_out(Red, "\t%s: %s\n", file_status_string[repo_file->status], repo_file->path);
    }
    foreach_file(last_files, last_file) {
        if (last_file->status == Deleted && !last_file->staged)
            print_out(Red, "\t%s: %s\n", file_status_string[last_file->status], last_file->path);
    }
}

// Stage
void stage_files(const char *to_stage_path) {
    repo_must_exist(REPO_PATH);

    Files *to_stage_files = get_path_files(to_stage_path);

    Files *index_files = get_index_files(INDEX_PATH);

    FILE *temp_file = file_open(TEMP_INDEX_PATH, "w");

    foreach_file(index_files, index_file) {
        index_file->staged = 0;
        foreach_file(to_stage_files, to_stage_file) {
            if (same_path(to_stage_file, index_file)) {
                index_file->staged = 1;

                if (to_stage_file->status == Deleted) {
                    // Stage deletion (for single file stage)
                    fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, Deleted);
                    print_out(Red, "Staging deletion of file: %s\n", index_file->path);

                } else if (!same_hash(to_stage_file, index_file)) {
                    // Stage modification
                    to_stage_file->status = Modified;
                    fprintf(temp_file, "%s %s %d\n", to_stage_file->path, to_stage_file->hash, Modified);
                    create_object(*to_stage_file);
                    print_out(Yellow, "Staging modification of file %s with hash %s\n", to_stage_file->path, to_stage_file->hash);

                } else {
                    // File is unchanged
                    fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, index_file->status);
                    to_stage_file->status = Unchanged;
                }
            }
        }
        if (!index_file->staged) {
            if (strncmp(index_file->path, to_stage_path, strlen(to_stage_path)) == 0 && !does_dir_exist(index_file->path)) {
                // Stage deletion (for dir stage)
                fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, Deleted);
                print_out(Red, "Staging deletion of file: %s\n", index_file->path);

            } else {
                // No changes
                fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, index_file->status);
            }
        }
    }

    foreach_file(to_stage_files, path_file) {
        if (path_file->status == New) {
            // Stage new file
            fprintf(temp_file, "%s %s %d\n", path_file->path, path_file->hash, New);
            create_object(*path_file);
            print_out(Green, "Staging new file %s with hash %s\n", path_file->path, path_file->hash);
        }
    }

    fclose(temp_file);
    remove(INDEX_PATH);
    rename(TEMP_INDEX_PATH, INDEX_PATH);
}

// Unstage
void unstage_files(const char *to_unstage_path) {
    repo_must_exist(REPO_PATH);

    Files *to_unstage_files = get_path_files(to_unstage_path);

    Files *index_files = get_index_files(INDEX_PATH);

    FILE *temp_file = file_open(TEMP_INDEX_PATH, "w");

    print_out(White, "Unstaged files:\n");
    foreach_file(index_files, index_file) {
        int found = 0;
        foreach_file(to_unstage_files, unstage_file) {
            if (same_path(index_file, unstage_file)) {
                if (index_file->status != Unchanged) {
                    found = 1;
                    print_out(Yellow, "\t%s\n", index_file->path);
                }
                _i_unstage_file = to_unstage_files->count;
            }
        }
        if (!found) {
            fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, index_file->status);
        }
    }

    fclose(temp_file);
    remove(INDEX_PATH);
    rename(TEMP_INDEX_PATH, INDEX_PATH);
}

// Commit
void commit_changes(const char *commit_message) {
    repo_must_exist(REPO_PATH);

    Files *index_files = get_index_files(INDEX_PATH);
    FILE *temp_file = file_open(TEMP_INDEX_PATH, "w");

    foreach_file(index_files, index_file) {
        if (index_file->status != Deleted)
            fprintf(temp_file, "%s %s %d\n", index_file->path, index_file->hash, Unchanged);
    }

    fclose(temp_file);

    remove(INDEX_PATH);
    rename(TEMP_INDEX_PATH, INDEX_PATH);
    
    char *index_hash = new_hash(INDEX_PATH);

    Commit commit = {0};

    strncpy(commit.index_hash, index_hash, SHA1_STRING_SIZE);

    File index_file = {INDEX_PATH, index_hash, 0, 0};
    create_object(index_file);

    // Get last commit hash
    char *last_commit = get_branch_commit_hash(get_current_branch_name());

    strncpy(commit.parent, last_commit, sizeof(commit.parent));

    // Get user info
    User user = {0};
    get_user_info(&user);

    strcpy(commit.author_name, user.name);
    strcpy(commit.author_email, user.email);
    strcpy(commit.author_userid, user.userid);

    // Get message
    strncpy(commit.message, commit_message, sizeof(commit.message));

    // Get time
    commit.timestamp = time(NULL);

    // Get this commit hash and store
    char commit_content[1024];
    snprintf(commit_content, sizeof(commit_content),
                "index %s\nparent %s\nauthor\n\t%s\n\t%s\n\t%s\ndate %ld\n\n%s",
                commit.index_hash, commit.parent, commit.author_name, commit.author_email, commit.author_userid,
                commit.timestamp, commit.message);
    
    File temp_commit = {0};
    temp_commit.path = new_path("%s\\.snaptrack\\temp_commit", REPO_PATH);
    FILE *temp_commit_file = file_open(temp_commit.path, "w");
    fprintf(temp_commit_file, "%s", commit_content);
    fclose(temp_commit_file);

    temp_commit.hash = new_hash(temp_commit.path);

    create_object(temp_commit);

    // Update last commit
    char *current_branch_path = new_path("%s%s", BRANCHES_PATH, get_current_branch_name());
    FILE *current_branch_file = file_open(current_branch_path, "w");
    fprintf(current_branch_file, "%s", temp_commit.hash);
    fclose(current_branch_file);

    remove(temp_commit.path);

    printf("TODO: Commit done message\n");
}

// Show commits
void list_commits() {
    Commit *commits = NULL;
    int commit_count = 0;

    char *last_commit_hash = get_branch_commit_hash(get_current_branch_name());

    print_out(White, "\tCommit log in");
    print_out(Green, " %s ", get_current_branch_name());
    print_out(White, "branch\n\n");
    while (!STRING_EMPTY(last_commit_hash)) {
        Commit commit = {0};
        get_commit_info(&commit, last_commit_hash);
        strcpy(last_commit_hash, commit.parent);

        print_out(Yellow, "commit  %s\n", commit.hash);
        print_out(White, "Author: %s %s <%s>\n", commit.author_name, commit.author_userid, commit.author_email);
        print_out(White, "Date:   %s\n", ctime(&commit.timestamp));
        print_out(White, "\t%s\n", commit.message);
        print_out(White, "\n");
    }
}

// Revert commit
void revert_commit(const char *revert_hash) {
    Commit revert_commit = {0};
    get_commit_info(&revert_commit, revert_hash);

    Files *revert_files = get_index_files_from_index_hash(revert_commit.index_hash);
    Files *repo_files = get_path_files(REPO_PATH);

    foreach_file(revert_files, revert_file) {
        revert_file->staged = 0;
        char *object_path = new_path("%s%s",OBJECTS_PATH, revert_file->hash);
        copy_file(revert_file->path, object_path);

        foreach_file(repo_files, repo_file) {
            if (same_path(revert_file, repo_file)) {
                repo_file->staged = 1;
                _i_repo_file = repo_files->count;
            }
        }
    }

    foreach_file(repo_files, repo_file) {
        if (!repo_file->staged) {
            remove(repo_file->path);
        }
    }

    stage_files(REPO_PATH);
    char message[256];
    snprintf(message, 256, "REVERT TO COMMIT HASH: %s", revert_commit.hash);
    commit_changes(message);
}

// Branch
void list_branches() {
    repo_must_exist(REPO_PATH);

    Branches *branches = get_branches();
    print_out(White, "Branch list:\n");
    foreach_branch(branches, branch) {
        print_out(White, "\t%s\n", branch);
    }
    free_branches(branches);
}

void create_branch(const char *branch_name) {
    repo_must_exist(REPO_PATH);

    char *branches_path = new_path("%s\\.snaptrack\\refs\\branches", REPO_PATH);

    Branches *branches = get_branches();

    int is_new_branch = 1;
    foreach_branch(branches, branch)
        if (same_string(branch, branch_name))
            is_new_branch = 0;

    free_branches(branches);

    if (!is_new_branch)
        exit_error("Branch with name %s already exists\n", branch_name);

    char *commit_hash = get_branch_commit_hash(get_current_branch_name());

    char *new_branch_file_path = new_path("%s%s", BRANCHES_PATH, branch_name);

    FILE *new_branch_file = file_open(new_branch_file_path, "w");
    fputs(commit_hash, new_branch_file);
    fclose(new_branch_file);

    change_head(branch_name);
}

void delete_branch(const char *to_delete_branch_name) {
    repo_must_exist(REPO_PATH);

    if (same_string(to_delete_branch_name, "main"))
        exit_error("Unable to delete main branch\n");


    char *current_branch_name = get_current_branch_name();

    if (same_string(to_delete_branch_name, current_branch_name))
        exit_error("Failed to delete: trying to delete current branch\n");

    char *branch_file_path_to_delete = new_path("%s%s", BRANCHES_PATH, to_delete_branch_name);

    remove(branch_file_path_to_delete);
}

void checkout_branch(const char *branch_name) {
    repo_must_exist(REPO_PATH);

    if (are_there_changes())
        exit_error("Uncommited changes in current branch\nCommit changes before changing branch or restore to previous commit\n");
    
    Files *branch_files = get_branch_index_files(branch_name);

    Files *current_files = get_branch_index_files(get_current_branch_name());

    foreach_file(current_files, current_file) {
        foreach_file(branch_files, branch_file) {
            if (same_file(current_file, branch_file)) {
                current_file->staged = 1; branch_file->staged = 1;
                _i_branch_file = branch_files->count;
            }
        }
        if (!current_file->staged)
            remove(current_file->path);
    }

    foreach_file(branch_files, branch_file) {
        if (!branch_file->staged) {
            create_src_from_object(branch_file);
        }
    }

    rewrite_index_file(branch_files);

    change_head(branch_name);
}