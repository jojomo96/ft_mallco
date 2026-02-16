#include <stdint.h>
#include <stdlib.h>

#include "ft_malloc.h"

/*
 * Global allocator state:
 * - g_zones is the head of our zone list.
 * - g_mutex serializes all allocator mutations.
 */
t_zone *        g_zones = NULL;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Align a byte count up to the next 16-byte boundary.
 *
 * Why 16?
 * - It is enough for common scalar types and SIMD-friendly alignment.
 * - It keeps metadata/user boundaries predictable across all requests.
 */
size_t align_size(const size_t size) {
    return (size + 15) & ~15;
}

/*
 * Decide which zone class should handle a request.
 *
 * TINY/SMALL requests are pooled (many blocks per mmap zone),
 * LARGE requests get dedicated zones.
 */
static t_zone_type get_zone_type(const size_t size) {
    if (size <= TINY_MALLOC_LIMIT)
        return TINY;
    if (size <= SMALL_MALLOC_LIMIT)
        return SMALL;
    return LARGE;
}

/*
 * Split one block into:
 *   [allocated block | free remainder block]
 *
 * We only split when the remainder is meaningful:
 * - enough space for a new block header
 * - plus at least MALLOC_ALIGN user bytes
 */
void split_block(t_block *block, const size_t size) {
    /* Guard: don't create unusable tiny fragments. */
    if (block->size >= size + BLOCK_HDR_SIZE + MALLOC_ALIGN) {
        /*
         * New remainder block starts right after:
         * current block header + requested payload.
         */
        t_block *new_block = (t_block *)((char *)block + BLOCK_HDR_SIZE + size);

        /* Initialize remainder metadata. */
        new_block->size = block->size - size - BLOCK_HDR_SIZE;
        new_block->next = block->next;
        new_block->free = 1;

        /* Shrink current block to exactly the allocated size. */
        block->size = size;
        block->next = new_block;
        block->free = 0;

        debug_log_block_split(block, size, new_block->size);
    } else {
        /* No useful split possible: consume full block as one allocation. */
        block->free = 0;
    }
}

/*
 * First-fit search inside zones of the matching type.
 *
 * This keeps policy simple and deterministic:
 * - iterate zones in list order
 * - iterate blocks in each zone
 * - pick first free block large enough
 */
static t_block *find_free_block(const t_zone_type type, const size_t size) {
    const t_zone *zone = g_zones;

    while (zone) {
        /* Never mix types: TINY requests never consume SMALL/LARGE zones. */
        if (zone->type == type) {
            t_block *block = zone->blocks;

            while (block) {
                if (block->free && block->size >= size)
                    return block;
                block = block->next;
            }
        }
        zone = zone->next;
    }
    return NULL;
}

/*
 * Debug helper: map a block pointer back to its owning zone.
 *
 * This is intentionally O(n) and only used for richer debug output.
 */
static t_zone *find_zone_for_block(const t_block *target)
{
    t_zone *zone = g_zones;

    while (zone) {
        t_block *block = zone->blocks;

        while (block) {
            if (block == target)
                return zone;
            block = block->next;
        }
        zone = zone->next;
    }
    return NULL;
}

/*
 * Core malloc implementation (expects caller already holds g_mutex).
 *
 * Central flow:
 * 1) normalize+align request
 * 2) reuse a free block if possible
 * 3) otherwise mmap/register a new zone
 * 4) split pooled blocks when useful
 * 5) return user pointer (header skipped)
 */
void *malloc_nolock(size_t size) {
    /*
     * malloc(0) is implementation-defined; we choose minimum alloc behavior
     * so returned pointer stays safely free-able and practical for callers.
     */
    const size_t requested_size = size == 0 ? (size_t)1u : size;

    /* Prevent wraparound before alignment math. */
    if (requested_size > SIZE_MAX - (size_t)15u) {
        debug_log_event("malloc", NULL, requested_size, "failed: size overflow");
        return NULL;
    }

    /* Work internally with aligned payload size. */
    const size_t      aligned_size = align_size(requested_size);
    const t_zone_type type         = get_zone_type(aligned_size);

    /* Try reuse path first to reduce mmap calls and fragmentation pressure. */
    t_block *block         = find_free_block(type, aligned_size);
    int      from_new_zone = 0;

    if (!block) {
        /* Slow path: acquire fresh zone from kernel. */
        t_zone *zone = request_new_zone(type, aligned_size);

        if (!zone) {
            debug_log_event("malloc", NULL, aligned_size, "failed: mmap");
            return NULL;
        }

        /* Fresh zones expose one initial block covering available payload area. */
        block = zone->blocks;
        from_new_zone = 1;
    }

    /* Snapshot pre-placement state for debug trace. */
    const size_t block_size_before = block->size;
    t_zone *     block_zone        = find_zone_for_block(block);

    debug_log_malloc_placement(block_zone, block, requested_size, aligned_size,
                               block_size_before, from_new_zone);

    /*
     * Allocation finalization policy:
     * - TINY/SMALL: split if profitable so leftovers remain reusable
     * - LARGE: one block per zone, mark it used directly
     */
    if (type != LARGE)
        split_block(block, aligned_size);
    else
        block->free = 0;

    /* User pointer always starts immediately after metadata header. */
    void *ptr = (void *)((char *)block + BLOCK_HDR_SIZE);

    /* Optional debug mode: mark fresh bytes with 0xAA to expose uninitialized use. */
    if (g_malloc_scribble)
        ft_memset(ptr, 0xAA, requested_size);

    debug_log_event("malloc", ptr, requested_size, type == LARGE ? "large" : "zone");
    return ptr;
}

/* Public malloc wrapper: lock -> core logic -> unlock. */
void *malloc(size_t size) {
    pthread_mutex_lock(&g_mutex);
    void *ptr = malloc_nolock(size);
    pthread_mutex_unlock(&g_mutex);
    return ptr;
}
