#include <stdio.h>
#include <libloaderapi.h>
#include "sha.h"

DLL sha_dll = {0};
SHA1FileFunc sha_file = NULL;

void free_sha_file() {
    FreeLibrary(sha_dll.handle);
}

void init_sha_file() {
    sha_dll.handle = LoadLibrary("sha1.dll");
    if (!sha_dll.handle) {
        fprintf(stderr, "Failed to load %s\n", "sha1.dll");
        exit(EXIT_FAILURE);
    }
    
    sha_dll.func = (void *)GetProcAddress(sha_dll.handle, "sha1_file");
    if (!sha_dll.func) {
        fprintf(stderr, "Failed to locate %s in DLL", "sha1_file");
        free_sha_file();
        exit(EXIT_FAILURE);
    }

    sha_file = (SHA1FileFunc)sha_dll.func;
}

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_STRING_SIZE]) {
    for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
        sprintf(output + (i*2), "%02x", hash[i]);
    }
    output[SHA1_BLOCK_SIZE*2] = '\0';
}