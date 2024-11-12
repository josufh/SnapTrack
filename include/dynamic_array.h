#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct DA {
    void *items;
    size_t count;
    size_t capacity;
    size_t item_size;
} DynamicArray;

#define DA_INIT_CAPACITY 1

#define DA_INIT(da, size) \
    do { \
        (da).items = malloc(size * DA_INIT_CAPACITY); \
        (da).count = 0; \
        (da).capacity = DA_INIT_CAPACITY; \
        (da).item_size = size; \
    } while (0)

#define DA_RESIZE(da) \
    do { \
        (da).capacity *= 2; \
        (da).items = realloc((da).items, (da).item_size * (da).capacity); \
        if ((da).items == NULL) { \
            fprintf(stderr, "Memory allocation failed during resizing\n"); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)                                                                 

#define DA_ADD(da, element) \
    do { \
        if ((da).capacity == 0) { DA_INIT(da, sizeof(typeof(*element))); } \
        else if ((da).count == (da).capacity) { DA_RESIZE(da); } \
        memcpy(((char *)(da).items) + ((da).count * (da).item_size), element, (da).item_size); \
        (da).count++; \
    } while (0)

#define DA_FREE(da)         \
    do {                    \
        free((da).items);   \
        (da).items = NULL;  \
        (da).count = 0;     \
        (da).capacity = 0;  \
        (da).item_size = 0; \
    } while (0)

#define DA_GET(da, index) \
    (((index) >= 0 && (index) < (da).count) ? \
    (void *)((char *)(da).items + ((index)*(da).item_size)) : \
    (fprintf(stderr, "Error: Index %d, is out of bounds (0-%d)\n", (index), (da).count-1), exit(EXIT_FAILURE), (void *)NULL))

#define DA_FOREACH(da, var, type) \
    for (size_t _i = 0; _i < (da).count; ++_i) \
        for (type *var = (type *)((char *)(da).items + _i * (da).item_size); var != NULL; var = NULL)

#endif // DYNAMIC_ARRAY_H