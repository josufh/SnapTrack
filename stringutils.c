#ifndef STRINGUTILS
#define STRINGUTILS

#include <string.h>

#define SHA1_BLOCK_SIZE 20

int is_same_string(const char *string1, const char *string2) {
    return strcmp(string1, string2) == 0;
}

void sha1_to_hex(unsigned char hash[SHA1_BLOCK_SIZE], char output[SHA1_BLOCK_SIZE*2+1]) {
    for (int i = 0; i < SHA1_BLOCK_SIZE; i++) {
        sprintf(output + (i*2), "%02x", hash[i]);
    }
    output[SHA1_BLOCK_SIZE*2] = '\0';
}



#endif // STRINGUTILS