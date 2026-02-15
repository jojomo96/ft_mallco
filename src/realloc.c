#include <stdint.h>
#include "ft_malloc.h"

static void scribble_new_bytes(void *ptr, size_t old_size, size_t new_size) {
    if (g_malloc_scribble && new_size > old_size) {
        ft_memset((char *) ptr + old_size, 0xAA, new_size - old_size);
    }
}

/*
 * HELPER: Find the block metadata for a given user pointer.
 * Assumes g_mutex is locked.
 */
static t_block *find_block_by_ptr(void *ptr, t_zone **out_zone) {
    t_zone *zone = g_zones;

    while (zone) {
        char *start = (char *) zone;
        char *end   = start + zone->size;

        if ((char *) ptr >= start && (char *) ptr < end) {
            t_block *block = zone->blocks;
            while (block) {
                if ((void *) ((char *) block + BLOCK_HDR_SIZE) == ptr) {
                    *out_zone = zone;
                    return block;
                }
                block = block->next;
            }
            break;
        }
        zone = zone->next;
    }
    return NULL;
}


/*
 * HELPER: Try to merge with the next block to satisfy request
 */
static int try_merge_next(t_block *block, size_t need) {
    t_block *next = block->next;

    // We can only merge if the next block exists and is free
    if (!next || !next->free)
        return 0;

    size_t merged = block->size + BLOCK_HDR_SIZE + next->size;
    if (merged < need)
        return 0;

    coalesce_right(block);
    block->free = 0;
    split_block(block, need);
    return 1;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr)
        return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    // overflow guard for align_size(size) == (size + 15) & ~15
    if (size > SIZE_MAX - (size_t) 15u)
        return NULL;

    // 1. Align the requested size
    size_t aligned_size = align_size(size);

    pthread_mutex_lock(&g_mutex);

    // 2. Find the block metadata
    t_zone * zone  = NULL;
    t_block *block = find_block_by_ptr(ptr, &zone);
    if (!block || block->free || !zone) {
        pthread_mutex_unlock(&g_mutex);
        return NULL; // Invalid pointer
    }

    size_t old_size = block->size;

    // 3. Optimization: Shrinking
    if (aligned_size <= old_size) {
        // TODO: We could split here to return memory, but the subject
        // doesn't strictly require shrinking. Keeping it simple is safe.
        // If you want to shrink: split_block(block, aligned_size);
        pthread_mutex_unlock(&g_mutex);
        return ptr;
    }

    // 4. Optimization: In-Place Growth (Merge with next) - Only for TINY/SMALL
    if (zone->type != LARGE) {
        if (try_merge_next(block, aligned_size)) {
            scribble_new_bytes(ptr, old_size, block->size);
            pthread_mutex_unlock(&g_mutex);
            return ptr;
        }
    }

    // 5. Fallback: Malloc-Copy-Free
    // We use the _exec functions because we ALREADY hold the lock.
    void *new_ptr = malloc_nolock(aligned_size); // malloc_exec will align the size again
    if (!new_ptr) {
        pthread_mutex_unlock(&g_mutex);
        return NULL;
    }

    // 6. Copy the old data
    size_t copy_size = old_size; // here aligned_size > old_size, so copy old_size
    ft_memcpy(new_ptr, ptr, copy_size);
    scribble_new_bytes(new_ptr, old_size, aligned_size);
    free_nolock(ptr);

    pthread_mutex_unlock(&g_mutex);
    return new_ptr;
}
