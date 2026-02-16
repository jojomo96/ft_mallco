#include "ft_malloc.h"

/* Convert one byte to two uppercase hex digits and print it. */
static void print_byte_hex(unsigned char c) {
    char *base = "0123456789ABCDEF";

    ft_putchar_fd(base[(c >> 4) & 0xF], 1);
    ft_putchar_fd(base[c & 0xF], 1);
}

/*
 * Print a block payload as classic hexdump lines.
 *
 * Per line:
 * - absolute address
 * - up to 16 hex bytes
 * - printable ASCII mirror (non-printable -> '.')
 */
static void hexdump_block(void *ptr, size_t size) {
    unsigned char *data = ptr;
    size_t         count;

    for (size_t i = 0; i < size; i += 16) {
        /* Address column (start of current 16-byte slice). */
        ft_putptr_fd(data + i, 1);
        ft_putstr_fd("  ", 1);

        /* Hex byte column. */
        for (count = 0; count < 16; count++) {
            if (i + count < size) {
                print_byte_hex(data[i + count]);
                ft_putchar_fd(' ', 1);
            } else {
                /* Pad last line so ASCII column stays aligned. */
                ft_putstr_fd("   ", 1);
            }
        }

        /* ASCII preview column. */
        ft_putstr_fd(" |", 1);
        for (count = 0; count < 16; count++) {
            if (i + count < size) {
                unsigned char c = data[i + count];
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
 * Extended memory view:
 * - same zone walk as show_alloc_mem
 * - plus full hexdump of each allocated block payload
 */
void show_alloc_mem_ex(void) {
    pthread_mutex_lock(&g_mutex);

    t_zone *zone = g_zones;
    while (zone) {
        /* Print zone header. */
        if (zone->type == TINY)
            ft_putstr_fd("TINY : ", 1);
        else if (zone->type == SMALL)
            ft_putstr_fd("SMALL : ", 1);
        else
            ft_putstr_fd("LARGE : ", 1);

        ft_putptr_fd(zone, 1);
        ft_putchar_fd('\n', 1);

        /* Print details for each allocated block in this zone. */
        t_block *block = zone->blocks;
        while (block) {
            if (!block->free) {
                ft_putstr_fd("BLOCK: ", 1);
                ft_putptr_fd((void *)((char *)block + BLOCK_HDR_SIZE), 1);
                ft_putstr_fd(" - SIZE: ", 1);
                ft_putsize_fd(block->size, 1);
                ft_putstr_fd(" bytes\n", 1);

                hexdump_block((void *)((char *)block + BLOCK_HDR_SIZE), block->size);
                ft_putchar_fd('\n', 1);
            }
            block = block->next;
        }

        zone = zone->next;
    }

    pthread_mutex_unlock(&g_mutex);
}
