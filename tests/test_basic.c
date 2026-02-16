#include "../include/ft_malloc.h"
#include <string.h>   // strcpy


int main(void)
{
	ft_putstr_fd("=== TEST START ===\n", 1);

	// 1. Tiny Allocation
	char *str1 = (char *)malloc(42);
	strcpy(str1, "Hello Tiny");
	ft_putstr_fd("Allocated Tiny: ", 1);
	ft_putptr_fd(str1, 1);
	ft_putchar_fd('\n', 1);

	// 2. Small Allocation
	char *str2 = (char *)malloc(500);
	strcpy(str2, "Hello Small");
	ft_putstr_fd("Allocated Small: ", 1);
	ft_putptr_fd(str2, 1);
	ft_putchar_fd('\n', 1);

	// 3. Large Allocation
	char *str3 = (char *)malloc(4000);
	strcpy(str3, "Hello Large");
	ft_putstr_fd("Allocated Large: ", 1);
	ft_putptr_fd(str3, 1);
	ft_putchar_fd('\n', 1);

	// 4. Show Memory
	ft_putstr_fd("\n--- VISUALIZER ---\n", 1);
	show_alloc_mem();
	show_alloc_mem_ex();
	ft_putstr_fd("------------------\n", 1);

	// 5. Free
	free(str1);
	free(str2);
	free(str3);

	ft_putstr_fd("\nAfter Free (your allocations should be gone):\n", 1);
	show_alloc_mem();

	// Optional bonus dump
	show_alloc_mem_ex();

	return 0;
}
