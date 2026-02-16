#include <stdint.h>

#include "ft_malloc.h"

/*
 * Scribble only the new tail after in-place growth / copied growth.
 *
 * We intentionally do not rewrite the old prefix so previously valid data stays
 * untouched and user-visible semantics remain intuitive.
 */
static void scribble_new_bytes(void *ptr, size_t old_size, size_t new_size) {
    if (g_malloc_scribble && new_size > old_size)
        ft_memset((char *)ptr + old_size, 0xAA, new_size - old_size);
}

/*
 * Find block metadata from a user pointer.
 *
 * Caller must hold g_mutex to prevent list mutations while traversing.
 */
static t_block *find_block_by_ptr(void *ptr, t_zone **out_zone) {
    t_zone *zone = g_zones;

    while (zone) {
        char *start = (char *)zone;
        char *end   = start + zone->size;

        /* First confirm pointer is within this zone mapping. */
        if ((char *)ptr >= start && (char *)ptr < end) {
            t_block *block = zone->blocks;

            while (block) {
                if ((void *)((char *)block + BLOCK_HDR_SIZE) == ptr) {
                    *out_zone = zone;
                    return block;
                }
                block = block->next;
            }

            /* Pointer was in zone range, but not at any block boundary. */
            break;
        }
        zone = zone->next;
    }
    return NULL;
}

/*
 * Attempt in-place expansion by consuming the immediate next free block.
 *
 * Returns 1 on success, 0 if expansion in place is impossible.
 */
static int try_merge_next(t_block *block, size_t need) {
    t_block *next = block->next;

    /* Must have an adjacent free neighbor to expand without moving. */
    if (!next || !next->free)
        return 0;

    /* Compute payload size after hypothetical merge. */
    size_t merged = block->size + BLOCK_HDR_SIZE + next->size;
    if (merged < need)
        return 0;

    /* Commit merge and re-split so final payload is close to requested size. */
    coalesce_right(block);
    block->free = 0;
    split_block(block, need);
    return 1;
}

/*
 * realloc behavior summary:
 * - realloc(NULL, n)   -> malloc(n)
 * - realloc(p, 0)      -> free(p), return NULL
 * - grow/shrink in place when possible
 * - otherwise allocate-copy-free
 */
void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        debug_log_event("realloc", NULL, size, "acts as malloc");
        return malloc(size);
    }
    if (size == 0) {
        debug_log_event("realloc", ptr, 0, "acts as free");
        free(ptr);
        return NULL;
    }

    /* Guard against overflow before alignment step. */
    if (size > SIZE_MAX - (size_t)15u) {
        debug_log_event("realloc", ptr, size, "failed: size overflow");
        return NULL;
    }

    size_t aligned_size = align_size(size);

    pthread_mutex_lock(&g_mutex);

    /* Validate ptr and recover metadata. */
    t_zone * zone  = NULL;
    t_block *block = find_block_by_ptr(ptr, &zone);
    if (!block || block->free || !zone) {
        pthread_mutex_unlock(&g_mutex);
        debug_log_event("realloc", ptr, size, "failed: invalid pointer");
        return NULL;
    }

    size_t old_size = block->size;

    /*
     * Shrink/no-op path:
     * we keep current block as-is for simplicity and stability.
     * (Could split here, but not required for correctness.)
     */
    if (aligned_size <= old_size) {
        pthread_mutex_unlock(&g_mutex);
        debug_log_event("realloc", ptr, size, "in-place shrink/no-op");
        return ptr;
    }

    /*
     * In-place growth path (pooled zones only).
     * LARGE zones are dedicated mappings and are not expanded this way.
     */
    if (zone->type != LARGE && try_merge_next(block, aligned_size)) {
        scribble_new_bytes(ptr, old_size, block->size);
        pthread_mutex_unlock(&g_mutex);
        debug_log_event("realloc", ptr, size, "in-place growth");
        return ptr;
    }

    /*
     * Fallback move path:
     * - allocate a new block
     * - copy old payload
     * - free old block
     */
    void *new_ptr = malloc_nolock(aligned_size);
    if (!new_ptr) {
        pthread_mutex_unlock(&g_mutex);
        debug_log_event("realloc", ptr, size, "failed: malloc");
        return NULL;
    }

    ft_memcpy(new_ptr, ptr, old_size);
    scribble_new_bytes(new_ptr, old_size, aligned_size);
    free_nolock(ptr);

    pthread_mutex_unlock(&g_mutex);
    debug_log_event("realloc", new_ptr, size, "moved");
    return new_ptr;
}
