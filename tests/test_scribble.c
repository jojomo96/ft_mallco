#include "../include/ft_malloc.h"
#include <string.h>

//MallocScribble=1 ./test_scribble

void show_alloc_mem_ex(void);

static int	check_range_is(const unsigned char *p, size_t from, size_t to, unsigned char v)
{
    for (size_t i = from; i < to; i++)
    {
        if (p[i] != v)
            return 0;
    }
    return 1;
}

int main(void)
{
    ft_putstr_fd("=== SCRIBBLE FREE TEST (0x55) ===\n", 1);
    ft_putstr_fd("- malloc scribble: 0xAA on requested_size\n", 1);
    ft_putstr_fd("- free scribble:   0x55 on full block size\n\n", 1);

    // 1) Allocate a 64-byte block and fill it with 0xAA ourselves (just to be sure)
    unsigned char *a = (unsigned char *)malloc(64);
    if (!a)
        return 1;
    memset(a, 0xAA, 64);

    // 2) Free it -> your free() should scribble 0x55 over the whole 64-byte payload
    free(a);

    // 3) Reallocate a SMALLER size that should reuse the same free block.
    //    malloc scribble will write 0xAA only over requested_size (here: 1 byte),
    //    so bytes [1..15] (aligned block) should still be 0x55 if free-scribble worked.
    unsigned char *b = (unsigned char *)malloc(1);
    if (!b)
        return 1;

    ft_putstr_fd("Reallocated 1 byte at: ", 1);
    ft_putptr_fd(b, 1);
    ft_putchar_fd('\n', 1);

    // Dump for manual inspection too
    show_alloc_mem_ex();

    // Check: first byte likely 0xAA (malloc scribble), the rest should be 0x55 (free scribble residue)
    // We check the next 15 bytes to avoid dependence on requested_size.
    int ok = check_range_is(b, 1, 16, 0x55);

    if (ok)
        ft_putstr_fd("\n[SUCCESS] free scribble detected: bytes [1..15] are 0x55\n", 1);
    else
        ft_putstr_fd("\n[FAIL] free scribble NOT detected in bytes [1..15]\n", 1);

    free(b);
    return 0;
}
