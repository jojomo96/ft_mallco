#include "ft_malloc.h"

t_zone *        g_zones = NULL;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

t_zone *request_new_zone(t_zone_type type, size_t request_size);

/*
 * HELPER
 * We align to 16 bytes (128 bits) to accommodate strict alignment requirements
 * on modern CPUs (like vectors/SSE) and to ensure our headers are safe.
 */
static size_t align_size(const size_t size) {
    return (size + 15) & ~15; // Align to 16 bytes
}

/*
 * HELPER
 * Determines if a request is TINY, SMALL, or LARGE based on your header limits.
 */
static t_zone_type get_zone_type(const size_t size) {
    if (size <= TINY_MALLOC_LIMIT)
        return TINY;
    if (size <= SMALL_MALLOC_LIMIT)
        return SMALL;
    return LARGE;
}

/*
 * HELPER: Split a block into two
 * We only split if there is enough space for a new header + minimal alignment.
 */
static void split_block(t_block *block, const size_t size) {
    // Calculate the remaining space
    // minimal size for a split = size requested + size of a new header + alignment padding
    if (block->size >= size + sizeof(t_block) + 16) {
        // 1. Create the pointer for the new block
        // We start at: (current address) + (current header size) + (requested user size)
        t_block *new_block = (void *) block + sizeof(t_block) + size;

        // 2. Set up the new block's metadata
        new_block->size = block->size - size - sizeof(t_block);
        new_block->next = block->next;
        new_block->free = 1; // The new block is free

        // 3. Update the original block's metadata
        block->size = size;
        block->next = new_block;
        block->free = 0; // The original block is now allocated
    } else {
        // Not enough space to split, just mark the whole block as used
        block->free = 0;
    }
}

/*
 * HELPER: Find a free block in our existing zones
 */
static t_block *find_free_block(const t_zone_type type, const size_t size) {
    const t_zone *zone = g_zones;
    while (zone) {
        // We only look in zones of the correct type
        // (TINY requests go to TINY zones, SMALL to SMALL)
        if (zone->type == type) {
            t_block *block = zone->blocks;
            while (block) {
                // Is it free? Is it big enough?
                if (block->free && block->size >= size)
                    return block;
                block = block->next;
            }
        }
        zone = zone->next;
    }
    // If we looked everywhere and found nothing:
    return NULL;
}

void *malloc(size_t size) {
    if (size == 0)
        return (NULL);

    // 1. Align size to 16 bytes
    const size_t      aligned_size = align_size(size);
    const t_zone_type type         = get_zone_type(aligned_size);

    pthread_mutex_lock(&g_mutex);

    // 2. Try to find an existing free block
    t_block *block = find_free_block(type, aligned_size);

    if (!block) {
        // 3. No block found? Request a new zone from the OS
        t_zone *zone = request_new_zone(type, aligned_size);
        if (!zone) {
            // Failed to mmap (OOM)
            pthread_mutex_unlock(&g_mutex);
            return NULL;
        }

        // The new zone comes with one big free block (except for LARGE)
        block = zone->blocks;
    }

    // 4. If it's a LARGE zone, we don't split (it's custom fit).
    //    For TINY/SMALL, we try to split the block to save the rest.
    if (type != LARGE) {
        split_block(block, aligned_size);
    } else {
        // Ensure large blocks are marked used
        block->free = 0;
    }

    pthread_mutex_unlock(&g_mutex);

    // 5. Return the pointer to the USER data (skip the header!)
    //    (void*)block + sizeof(t_block)
    return (void *) block + sizeof(t_block);
}
