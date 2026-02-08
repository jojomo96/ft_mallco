// tests/test_basic.c
#include "../include/ft_malloc.h"
#include <stdio.h>
#include <string.h>

//gcc tests/test_basic.c -L. -lft_malloc -o test_run

int main(void) {
    printf("=== TEST START ===\n");

    // 1. Tiny Allocation
    char *str1 = malloc(42);
    strcpy(str1, "Hello Tiny");
    printf("Allocated Tiny: %p\n", str1);

    // 2. Small Allocation
    char *str2 = malloc(500);
    strcpy(str2, "Hello Small");
    printf("Allocated Small: %p\n", str2);

    // 3. Large Allocation
    char *str3 = malloc(4000);
    strcpy(str3, "Hello Large");
    printf("Allocated Large: %p\n", str3);

    // 4. Show Memory
    printf("\n--- VISUALIZER ---\n");
    show_alloc_mem();
    printf("------------------\n");

    // 5. Free
    free(str1);
    free(str2);
    free(str3);

    printf("\nAfter Free (Should be empty or show nothing):\n");
    show_alloc_mem();

    return 0;
}