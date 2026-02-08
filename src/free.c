#include "ft_malloc.h"

/*
 * HELPER: Merge 'current' block with 'next' block
 * This absorbs the 'next' block into 'current'.
 */
void coalesce_right(t_block *current) {
    const t_block *next_block = current->next;

    if (next_block && next_block->free) {
        // Add the size of the next block + its header to current
        current->size += sizeof(t_block) + next_block->size;

        // Update the pointer to skip the absorbed block
        current->next = next_block->next;
    }
}

/*
 * HELPER: Handle freeing a LARGE zone
 * Large zones are unique because they are mmap'd individually.
 * We must remove the zone node from the global list and munmap it.
 */
static void free_large_zone(t_zone *zone, t_zone *prev_zone) {
    // 1. Unlink the zone from the global list
    if (prev_zone) {
        prev_zone->next = zone->next;
    } else {
        // We are removing the head of the list
        g_zones = zone->next;
    }

    // 2. Return memory to OS
    // Total size = zone header + block header + data
    // (We stored the total mmap size in zone->size)
    munmap(zone, zone->size);
}

void free_nolock(void *ptr) {
    if (!ptr)
        return;

    t_zone *zone      = g_zones;
    t_zone *prev_zone = NULL;

    // 1. Walk through all zones to find where this pointer belongs
    while (zone) {
        char *zone_start = (char *) zone;
        char *zone_end   = zone_start + zone->size;

        // Optimization: Check if ptr is within the address range of this zone
        // (void*)zone is start, (void*)zone + zone->size is end
        if ((char *) ptr >= zone_start && (char *) ptr < zone_end) {
            // Found the zone! Now let's find the specific block.
            t_block *block      = zone->blocks;
            t_block *prev_block = NULL;

            while (block) {
                // Calculate the data pointer for this block header
                const void *data_ptr = (char *) block + sizeof(t_block);

                if (data_ptr == ptr) {
                    // --- FOUND THE BLOCK ---

                    // Double-Free Protection
                    if (block->free) {
                        return;
                    }

                    // LARGE Zone
                    if (zone->type == LARGE) {
                        free_large_zone(zone, prev_zone);
                        return;
                    }

                    // TINY / SMALL Zone
                    block->free = 1;

                    // 1. Coalesce Right (Merge with next)
                    coalesce_right(block);

                    // 2. Coalesce Left (Merge with prev)
                    if (prev_block && prev_block->free) {
                        coalesce_right(prev_block);
                        // After merging left, 'prev_block' is the master.
                        // We don't need to do anything else.
                    }
                    return;
                }

                // Advance block pointers
                prev_block = block;
                block      = block->next;
            }
            // If we are here, ptr is inside this zone's range but NOT a valid block start.
            // It's an invalid pointer (e.g., pointer to middle of a string).
            // We stop searching immediately.
            break;
        }
        // Advance zone pointers
        prev_zone = zone;
        zone      = zone->next;
    }
}

void free(void *ptr) {
    pthread_mutex_lock(&g_mutex);
    free_nolock(ptr);
    pthread_mutex_unlock(&g_mutex);
}
