#include <stdint.h>

#include "ft_malloc.h"

/*
 * Compute mmap size for a zone class.
 *
 * TINY/SMALL policy:
 * - provision pooled zones large enough for at least MIN_ALLOCS blocks
 *
 * LARGE policy:
 * - allocate just enough for one request (+metadata)
 *
 * Final result is rounded up to page size because mmap works in pages.
 */
static size_t calculate_zone_size(const t_zone_type type, const size_t request_size) {
    const size_t page_size = getpagesize();
    size_t       size_needed;

    if (type == TINY)
        size_needed = ZONE_HDR_SIZE + MIN_ALLOCS * (TINY_MALLOC_LIMIT + BLOCK_HDR_SIZE);
    else if (type == SMALL)
        size_needed = ZONE_HDR_SIZE + MIN_ALLOCS * (SMALL_MALLOC_LIMIT + BLOCK_HDR_SIZE);
    else
        size_needed = ZONE_HDR_SIZE + request_size + BLOCK_HDR_SIZE;

    return (size_needed + page_size - 1) / page_size * page_size;
}

/*
 * Lay out zone metadata and initial block metadata in a fresh mapping.
 *
 * Memory layout:
 * [zone header][first block header][first block payload ...]
 */
static t_zone *init_zone(void *ptr, const t_zone_type type, const size_t zone_size) {
    t_zone *zone = (t_zone *)ptr;

    zone->type = type;
    zone->size = zone_size;
    zone->next = NULL;

    t_block *first_block = (t_block *)((char *)zone + ZONE_HDR_SIZE);
    zone->blocks = first_block;

    first_block->next = NULL;
    first_block->size = zone_size - ZONE_HDR_SIZE - BLOCK_HDR_SIZE;

    /*
     * For pooled zones, first block starts free.
     * For LARGE zones, this block is consumed immediately by allocator path.
     */
    first_block->free = (type != LARGE);

    return zone;
}

/*
 * Create and register a new zone.
 *
 * Caller must hold g_mutex.
 */
t_zone *request_new_zone(const t_zone_type type, const size_t request_size) {
    const size_t zone_size = calculate_zone_size(type, request_size);

    /* Ask kernel for anonymous private memory. */
    void *ptr = mmap(NULL, zone_size, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
        debug_log_event("zone", NULL, zone_size, "failed: mmap");
        return NULL;
    }

    t_zone *zone = init_zone(ptr, type, zone_size);

    /*
     * Insert zone in address order.
     * Keeping a stable order makes traversals/debug output deterministic.
     */
    t_zone **pp = &g_zones;
    while (*pp && (uintptr_t)(*pp) < (uintptr_t)zone)
        pp = &(*pp)->next;

    zone->next = *pp;
    *pp = zone;

    debug_log_event("zone", zone, zone_size,
                    type == LARGE ? "new large zone" : "new pooled zone");
    return zone;
}
