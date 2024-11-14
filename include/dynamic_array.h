#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct DA {
    void **items;
    size_t count;
    size_t capacity;
} DynamicArray;

#define DA_INIT_CAPACITY 1

#define DA_INIT(da) \
    do { \
        (da).items = malloc(DA_INIT_CAPACITY*sizeof(void *)); \
        (da).count = 0; \
        (da).capacity = DA_INIT_CAPACITY; \
    } while (0)

#define DA_RESIZE(da) \
    do { \
        (da).capacity *= 2; \
        (da).items = realloc((da).items, (da).capacity * sizeof(void *)); \
        if ((da).items == NULL) { \
            fprintf(stderr, "Memory allocation failed during resizing\n"); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)                                                                 

#define DA_ADD(da, element) \
    do { \
        if ((da).capacity == 0) { DA_INIT(da); } \
        else if ((da).count == (da).capacity) { DA_RESIZE(da); } \
        (da).items[(da).count++] = (element); \
    } while (0)

#define DA_FREE(da)         \
    do {                    \
        free((da).items);   \
        (da).items = NULL;  \
        (da).count = 0;     \
        (da).capacity = 0;  \
    } while (0)

#define DA_GET(da, index) \
    (((size_t)(index) < (da).count) ? \
    (da).items[index] : \
    (fprintf(stderr, "Error: Index %zu, is out of bounds (0-%zu)\n", (size_t)(index), (da).count-1), exit(EXIT_FAILURE), NULL))

#define DA_FOREACH(da, var, type) \
    for (size_t _i_##var = 0; _i_##var < (da).count; ++_i_##var) \
        for (type var = (type)(da).items[_i_##var]; var != NULL; var = NULL)

#endif // DYNAMIC_ARRAY_H