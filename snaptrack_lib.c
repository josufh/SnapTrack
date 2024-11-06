#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>
#include "fileutils.c"

// NEVER CHANGE VALUE
#define SHA1_BLOCK_SIZE 20

typedef struct {
    char *filename;
    char blob_hash[SHA1_BLOCK_SIZE*2+1];
} File;

typedef struct {
    File file;
    int status;
} StagedFile;

typedef void (*SHA1FileFunc)(const char *filename, unsigned char output[SHA1_BLOCK_SIZE]);

__declspec(dllexport) void init_repository(const char *repo_path) {
    char path[1024];

    // Create main .snaptrack directory
    snprintf(path, sizeof(path), "%s\\.snaptrack", repo_path);
    if (_mkdir(path) != 0) {
        perror("Failed to create .snaptrack direcotry");
        return;
    }

    // Create objects direcotry
    snprintf(path, sizeof(path), "%s\\.snaptrack\\objects", repo_path);
    if (_mkdir(path) != 0) {
        perror("Failed to create objects direcotry");
        return;
    }

    // Create the refs/heads directory
    snprintf(path, sizeof(path), "%s\\.snaptrack\\refs", repo_path);
    if (_mkdir(path) != 0) {
        perror("Failed to create refs directory");
        return;
    }
    snprintf(path, sizeof(path), "%s\\.snaptrack\\refs\\heads", repo_path);
    if (_mkdir(path) != 0) {
        perror("Failed to create refs/heads directory");
        return;
    }

    // Create HEAD file
    snprintf(path, sizeof(path), "%s\\.snaptrack\\HEAD", repo_path);
    FILE *head_file = fopen(path, "w");
    if (head_file == NULL) {
        perror("Failed to create HEAD file");
        return;
    }
    fprintf(head_file, "ref: refs/heads/main\n");
    fclose(head_file);

    // Create index file
    snprintf(path, sizeof(path), "%s\\.snaptrack\\index", repo_path);
    FILE *index_file = fopen(path, "w");
    if (index_file == NULL) {
        perror("Failed to create HEAD file");
        return;
    }
    fclose(index_file);

    fprintf(stdout, "Initialized local empty SnapTrack repository\n");
}

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_BLOCK_SIZE*2+1]) {
    for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
        sprintf(output + (i*2), "%02x", hash[i]);
    }
    output[SHA1_BLOCK_SIZE*2] = '\0';
}

int is_file_modified_or_new(const char *index_path, const char *filename, const char *current_hash) {
    FILE *index_file = fopen(index_path, "r");
    if (!index_file) return 1;

    char line[1024];
    while (fgets(line, sizeof(line), index_file)) {
        char stored_filename[512];
        char stored_hash[SHA1_BLOCK_SIZE*2+1];

        if (sscanf(line, "%s %s", stored_filename, stored_hash) == 2) {
            if (strcmp(stored_filename, filename) == 0) {
                fclose(index_file);
                return strcmp(stored_hash, current_hash) != 0;
            }
        }
    }
    fclose(index_file);
    return 1;
}

__declspec(dllexport) void stage_files(const char *repo_path) {
    char **filenames = NULL;
    int file_count = 0;
    store_filenames(".", &filenames, &file_count, ".snaptrackignore");
    
    char hash_string[SHA1_BLOCK_SIZE*2+1];
    unsigned char hash[SHA1_BLOCK_SIZE];
    char index_path[MAX_PATH];
    char object_path[MAX_PATH];

    HMODULE hSHA1Dll = LoadLibrary("sha1.dll");
    if (!hSHA1Dll) {
        perror("Failed to load sha1.dll");
        return;
    }

    SHA1FileFunc sha1_file = (SHA1FileFunc)GetProcAddress(hSHA1Dll, "sha1_file");
    if (!sha1_file) {
        perror("Failed to locate sha1_file in DLL");
        FreeLibrary(hSHA1Dll);
        return;
    }

    for (int i = 0; i < file_count; i++) {
        sha1_file(filenames[i], hash);
        sha1_to_hex(hash, hash_string);

        snprintf(index_path, sizeof(index_path), "%s\\.snaptrack\\index", repo_path);
        snprintf(object_path, sizeof(object_path), "%s\\.snaptrack\\objects\\%s", repo_path, hash_string);

        if (is_file_modified_or_new(index_path, filenames[i], hash_string)) {
            FILE *existing_blob = fopen(object_path, "rb");
            if (!existing_blob) {
                FILE *object_file = fopen(object_path, "wb");
                if (!object_file) {
                    perror("Failed to create object file");
                    return;
                }
                
                FILE *src_file = fopen(filenames[i], "rb");
                if (!src_file) {
                    perror("Failed to open source file");
                    fclose(object_file);
                    return;
                }
                
                int c;
                while ((c = fgetc(src_file)) != EOF) {
                    fputc(c, object_file);
                }
                fclose(src_file);
                fclose(object_file);
            } else {
                fclose(existing_blob);
            }

            FILE *index_file = fopen(index_path, "r+");
            FILE *temp_file = fopen("temp_index", "w");

            char line[1024];
            int found = 0;
            if (index_file) {
                while (fgets(line, sizeof(line), index_file)) {
                    char stored_filename[512];
                    char stored_hash[SHA1_BLOCK_SIZE*2+1];

                    if (sscanf(line, "%s %s", stored_filename, stored_hash) == 2) {
                        if (strcmp(stored_filename, filenames[i]) == 0) {
                            fprintf(temp_file, "%s %s\n", filenames[i], hash_string);
                            found = 1;
                        } else {
                            fprintf(temp_file, "%s", line);
                        }
                    }
                }
                fclose(index_file);
            }

            if (!found)
                fprintf(temp_file, "%s %s\n", filenames[i], hash_string);

            fclose(temp_file);
            remove(index_path);
            rename("temp_index", index_path);

            fprintf(stdout, "Staged %s with hash %s\n", filenames[i], hash_string);
        }
        free(filenames[i]);
    }

    FreeLibrary(hSHA1Dll);
    free(filenames);
}

__declspec(dllexport) void check_status(const char *repo_path) {
    char index_path[MAX_PATH];
    snprintf(index_path, sizeof(index_path), "%s\\.snaptrack\\index", repo_path);

    FILE *index_file = fopen(index_path, "r");
    if (!index_file) {
        perror("Failed to open .snaptrack/index");
        return;
    }

    HMODULE hSHA1Dll = LoadLibrary("sha1.dll");
    if (!hSHA1Dll) {
        perror("Failed to load sha1.dll");
        return;
    }

    SHA1FileFunc sha1_file = (SHA1FileFunc)GetProcAddress(hSHA1Dll, "sha1_file");
    if (!sha1_file) {
        perror("Failed to locate sha1_file in DLL");
        FreeLibrary(hSHA1Dll);
        return;
    }

    StagedFile *staged_files = NULL;
    int staged_count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), index_file)) {
        staged_files = realloc(staged_files, (staged_count+1)*sizeof(StagedFile));
        StagedFile *current = &staged_files[staged_count];
        current->file.filename = malloc(512);

        sscanf(line, "%s %s", current->file.filename, current->file.blob_hash);
        current->status = 2;
        staged_count++;
    }
    fclose(index_file);

    char **filenames = NULL;
    int file_count = 0;
    store_filenames(repo_path, &filenames, &file_count, ".snaptrackignore");
    int print = 1;

    for (int i = 0; i < file_count; i++) {
        char current_hash[SHA1_BLOCK_SIZE*2+1];
        unsigned char hash[SHA1_BLOCK_SIZE];
        sha1_file(filenames[i], hash);
        sha1_to_hex(hash, current_hash);
        int found = 0;

        for (int j = 0; j < staged_count; j++) {
            if (strcmp(staged_files[j].file.filename, filenames[i]) == 0) {
                found = 1;
                staged_files[j].status = 0;
                if (strcmp(staged_files[j].file.blob_hash, current_hash) != 0) {
                    staged_files[j].status = 1;
                }
                break;
            }
        }
        if (!found && print) {
            printf("Untracked:\n");
            print = 0;
        }
        if (!found) printf("\t%s\n", filenames[i]);

        free(filenames[i]);
    }
    free(filenames);

    print = 1;
    for (int i = 0; i < staged_count; i++) {
        if (staged_files[i].status == 2) {
            if (print) {
                print = 0;
                printf("Deleted:\n");
            }
            printf("\t%s\n", staged_files[i].file.filename);
        }
    }
    print = 1;
    for (int i = 0; i < staged_count; i++) {
        if (staged_files[i].status == 1) {
            if (print) {
                print = 0;
                printf("Tracked but modified:\n");
            }
            printf("\t%s\n", staged_files[i].file.filename);
        }
    }
    print = 1;
    for (int i = 0; i < staged_count; i++) {
        if (staged_files[i].status == 0) {
            if (print) {
                print = 0;
                printf("Staged:\n");
            }
            printf("\t%s\n", staged_files[i].file.filename);
        }
    }

    for (int i = 0; i < staged_count; i++) {
        free(staged_files[i].file.filename);
    }
    free(staged_files);

    FreeLibrary(hSHA1Dll);
}