#include <stdio.h>
#include <string.h>

int main() {
    const char *str1 = "assets\\";
    int match = strncmp("include\\icon.png", str1, strlen(str1));

    printf("%d\n", match);
    return 1;
}