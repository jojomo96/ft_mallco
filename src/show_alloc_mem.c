#include "ft_malloc.h"

/*
 * Print current allocator state in subject-friendly format.
 *
 * Output groups allocations by zone type and ends with global total bytes.
 */
void show_alloc_mem(void) {
    size_t total_bytes = 0;

    /* Freeze allocator state while traversing internal lists. */
    pthread_mutex_lock(&g_mutex);

    t_zone *zone = g_zones;
    while (zone) {
        /* 1) Print zone class + zone base address. */
        if (zone->type == TINY)
            ft_putstr_fd("TINY : ", 1);
        else if (zone->type == SMALL)
            ft_putstr_fd("SMALL : ", 1);
        else
            ft_putstr_fd("LARGE : ", 1);

        ft_putptr_fd(zone, 1);
        ft_putchar_fd('\n', 1);

        /* 2) Print every allocated block in this zone. */
        t_block *block = zone->blocks;
        while (block) {
            if (!block->free) {
                /* User-visible memory range = [start, end). */
                void *start = (void *)((char *)block + BLOCK_HDR_SIZE);
                void *end   = (void *)((char *)block + BLOCK_HDR_SIZE + block->size);

                ft_putptr_fd(start, 1);
                ft_putstr_fd(" - ", 1);
                ft_putptr_fd(end, 1);
                ft_putstr_fd(" : ", 1);
                ft_putsize_fd(block->size, 1);
                ft_putstr_fd(" bytes\n", 1);

                total_bytes += block->size;
            }
            block = block->next;
        }

        zone = zone->next;
    }

    /* 3) Final aggregate for quick fragmentation/usage checks. */
    ft_putstr_fd("Total : ", 1);
    ft_putsize_fd(total_bytes, 1);
    ft_putstr_fd(" bytes\n", 1);

    pthread_mutex_unlock(&g_mutex);
}
