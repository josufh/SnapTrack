#ifndef SHA_H
#define SHA_H

#include <minwindef.h>

#define SHA1_BLOCK_SIZE 20
#define SHA1_STRING_SIZE 41 // SHA1_BLOCK_SIZE * 2 + 1

typedef void (*SHA1FileFunc)(const char *filename, unsigned char hash[SHA1_BLOCK_SIZE]);

typedef struct {
    HMODULE handle;
    void *func;
} DLL;

extern DLL sha_dll;
extern SHA1FileFunc sha_file;

void init_sha_file();
void free_sha_file();

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_STRING_SIZE]);

#endif // SHA_H 