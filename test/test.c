#include <stdio.h>
#include "dynamic_array.h"

// gcc -g -o test\test test\test.c -Iinclude -L. -lsha1

int main() {
    DynamicArray da;
    DA_INIT(da);

    size_t allocation_count = 0;

    while (1) {
        void *ptr = malloc(1); // Allocate a small memory block for testing
        if (!ptr) {
            fprintf(stderr, "System ran out of memory after %zu allocations\n", allocation_count);
            break;
        }
        DA_ADD(da, ptr);
        allocation_count++;
        if (allocation_count % 100000 == 0) {
            printf("Allocated %zu pointers so far\n", allocation_count);
        }
    }

    // Free all allocated memory
    for (size_t i = 0; i < da.count; i++) {
        free(da.items[i]);
    }
    DA_FREE(da);

    return 0;
}