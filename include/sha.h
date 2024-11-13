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

#endif // SHA_H 