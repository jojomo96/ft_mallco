#include "ft_malloc.h"

/* -------------------------------------------------------------------------- */
/* VISUALIZER LOGIC                                                           */
/* -------------------------------------------------------------------------- */

void show_alloc_mem(void) {
    size_t total_bytes = 0;

    pthread_mutex_lock(&g_mutex);

    t_zone *zone = g_zones;
    while (zone) {
        // 1. Print Zone Header: "TINY : 0xA0000"
        if (zone->type == TINY)
            ft_putstr_fd("TINY : ", 1);
        else if (zone->type == SMALL)
            ft_putstr_fd("SMALL : ", 1);
        else
            ft_putstr_fd("LARGE : ", 1);

        ft_putptr_fd(zone, 1);
        ft_putchar_fd('\n', 1);

        // 2. Iterate Blocks
        t_block *block = zone->blocks;
        while (block) {
            // We only print ALLOCATED blocks (not free ones)
            if (!block->free) {
                // Format: 0xA0020 - 0xA004A : 42 bytes
                ft_putptr_fd((void *) block + sizeof(t_block), 1);
                ft_putstr_fd(" - ", 1);
                ft_putptr_fd((void *) block + sizeof(t_block) + block->size, 1);
                ft_putstr_fd(" : ", 1);
                ft_putsize_fd(block->size, 1);
                ft_putstr_fd(" bytes\n", 1);

                total_bytes += block->size;
            }
            block = block->next;
        }
        zone = zone->next;
    }

    // 3. Print Total
    ft_putstr_fd("Total : ", 1);
    ft_putsize_fd(total_bytes, 1);
    ft_putstr_fd(" bytes\n", 1);

    pthread_mutex_unlock(&g_mutex);
}
