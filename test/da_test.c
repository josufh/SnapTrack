#include <stdio.h>
#include "dynamic_array.h"

// gcc -o da_test.exe test\da_test.c -Iinclude

int main() {
    DynamicArray numbers = {0};
    int ten = 10;
    DA_ADD(numbers, &ten);

    int *number = (int *)DA_GET(numbers, 0);
    printf("Number: %d\n", *number);

    DA_FREE(numbers);
    return 0;
}