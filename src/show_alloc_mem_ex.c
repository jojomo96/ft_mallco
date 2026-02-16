#include "ft_malloc.h"

/*
 * HELPER: Print a single byte in Hex (2 chars)
 */
static void print_byte_hex(unsigned char c) {
    char *base = "0123456789ABCDEF";
    ft_putchar_fd(base[(c >> 4) & 0xF], 1);
    ft_putchar_fd(base[c & 0xF], 1);
}

/*
 * HELPER: Dump a region of memory
 * Format: 0xADDR  XX XX XX ...  |ascii...|
 */
static void hexdump_block(void *ptr, size_t size) {
    unsigned char *data = ptr;
    size_t         count;

    for (size_t i = 0; i < size; i += 16) {
        // 1. Print Address offset
        // (Optional: You can print relative offset or absolute address)
        ft_putptr_fd(data + i, 1);
        ft_putstr_fd("  ", 1);

        // 2. Print Hex Bytes (up to 16)
        for (count = 0; count < 16; count++) {
            if (i + count < size) {
                print_byte_hex(data[i + count]);
                ft_putchar_fd(' ', 1);
            } else {
                ft_putstr_fd("   ", 1); // Pad if less than 16 bytes
            }
        }

        ft_putstr_fd(" |", 1);

        // 3. Print ASCII characters
        for (count = 0; count < 16; count++) {
            if (i + count < size) {
                unsigned char c = data[i + count];
                // Check printable range (32-126)
                if (c >= 32 && c <= 126)
                    ft_putchar_fd(c, 1);
                else
                    ft_putchar_fd('.', 1);
            }
        }
        ft_putstr_fd("|\n", 1);
    }
}

/*
 * BONUS VISUALIZER
 * Iterates through all blocks and dumps their content.
 */
void show_alloc_mem_ex(void) {
    pthread_mutex_lock(&g_mutex);

    t_zone *zone = g_zones;
    while (zone) {
        // Print Zone Header
        if (zone->type == TINY) ft_putstr_fd("TINY : ", 1);
        else if (zone->type == SMALL) ft_putstr_fd("SMALL : ", 1);
        else ft_putstr_fd("LARGE : ", 1);

        ft_putptr_fd(zone, 1);
        ft_putchar_fd('\n', 1);

        t_block *block = zone->blocks;
        while (block) {
            if (!block->free) {
                ft_putstr_fd("BLOCK: ", 1);
                ft_putptr_fd((void *)((char *)block + BLOCK_HDR_SIZE), 1);
                ft_putstr_fd(" - SIZE: ", 1);
                ft_putsize_fd(block->size, 1);
                ft_putstr_fd(" bytes\n", 1);

                // Dump the user data
                hexdump_block((void *)((char *)block + BLOCK_HDR_SIZE), block->size);
                ft_putchar_fd('\n', 1);
            }
            block = block->next;
        }
        zone = zone->next;
    }

    pthread_mutex_unlock(&g_mutex);
}
