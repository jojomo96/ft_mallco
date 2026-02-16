#include "ft_malloc.h"

/*
 * Merge current block with its immediate right neighbor.
 *
 * Preconditions for merge:
 * - current->next exists
 * - that next block is free
 */
void coalesce_right(t_block *current) {
    const t_block *next_block = current->next;

    if (next_block && next_block->free) {
        /* Payload grows by: next payload + next header now reclaimed. */
        current->size += BLOCK_HDR_SIZE + next_block->size;

        /* Skip merged node in linked list. */
        current->next = next_block->next;

        debug_log_block_merge(current, next_block, current->size);
    }
}

/*
 * Free path dedicated to LARGE zones.
 *
 * LARGE allocations are mapped independently, so we must:
 * 1) unlink zone from global list
 * 2) munmap entire zone in one operation
 */
static void free_large_zone(t_zone *zone, t_zone *prev_zone) {
    if (prev_zone)
        prev_zone->next = zone->next;
    else
        g_zones = zone->next;

    munmap(zone, zone->size);
}

/*
 * Core free implementation (expects caller already holds g_mutex).
 *
 * Validation strategy:
 * - null pointer: ignore
 * - unknown pointer: ignore
 * - pointer inside zone but not block start: ignore
 * - double free: ignore
 */
void free_nolock(void *ptr) {
    if (!ptr) {
        debug_log_event("free", NULL, 0, "ignored: null pointer");
        return;
    }

    t_zone *zone      = g_zones;
    t_zone *prev_zone = NULL;

    while (zone) {
        char *zone_start = (char *)zone;
        char *zone_end   = zone_start + zone->size;

        /*
         * Fast range filter:
         * if ptr is outside this zone address range, skip block scan entirely.
         */
        if ((char *)ptr >= zone_start && (char *)ptr < zone_end) {
            t_block *block      = zone->blocks;
            t_block *prev_block = NULL;

            while (block) {
                /* Valid user pointer for this block = header + BLOCK_HDR_SIZE. */
                const void *data_ptr = (char *)block + BLOCK_HDR_SIZE;

                if (data_ptr == ptr) {
                    /* Already free => double free attempt. */
                    if (block->free) {
                        debug_log_event("free", ptr, 0, "ignored: double free");
                        return;
                    }

                    /* Optional debug mode: poison released bytes with 0x55. */
                    if (g_malloc_scribble)
                        ft_memset(ptr, 0x55, block->size);

                    /* Dedicated unmap path for LARGE blocks. */
                    if (zone->type == LARGE) {
                        debug_log_event("free", ptr, block->size, "large");
                        free_large_zone(zone, prev_zone);
                        return;
                    }

                    /* Mark reusable and reduce fragmentation via coalescing. */
                    block->free = 1;

                    /* Step 1: merge right if possible. */
                    coalesce_right(block);

                    /*
                     * Step 2: merge left (by merging previous block rightward)
                     * when previous block is also free.
                     */
                    if (prev_block && prev_block->free)
                        coalesce_right(prev_block);

                    debug_log_event("free", ptr, block->size, "zone");
                    return;
                }

                prev_block = block;
                block = block->next;
            }

            /*
             * Pointer lands inside zone mapping but is not a valid block start.
             * Example: free(ptr + 1) or free(middle_of_payload).
             */
            debug_log_event("free", ptr, 0, "ignored: invalid pointer");
            return;
        }

        prev_zone = zone;
        zone = zone->next;
    }

    /* No zone owned this pointer. */
    debug_log_event("free", ptr, 0, "ignored: pointer not owned");
}

/* Public free wrapper: lock -> core logic -> unlock. */
void free(void *ptr) {
    pthread_mutex_lock(&g_mutex);
    free_nolock(ptr);
    pthread_mutex_unlock(&g_mutex);
}
