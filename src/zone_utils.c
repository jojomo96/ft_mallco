#include <stdint.h>

#include "ft_malloc.h"

/*
 * HELPER: Calculate the zone size
 * Returns a multiple of getpagesize() that can hold at least 100 allocations.
 */
static size_t calculate_zone_size(const t_zone_type type, const size_t request_size) {
    const size_t page_size = getpagesize();
    size_t size_needed;

    if (type == TINY)
        // 100 blocks of (MAX_TINY + Header)
        size_needed = ZONE_HDR_SIZE + MIN_ALLOCS * (TINY_MALLOC_LIMIT + BLOCK_HDR_SIZE);
    else if (type == SMALL)
        // 100 blocks of (MAX_SMALL + Header)
        size_needed = ZONE_HDR_SIZE + MIN_ALLOCS * (SMALL_MALLOC_LIMIT + BLOCK_HDR_SIZE);
    else
        // LARGE: Just the request + headers
        size_needed = ZONE_HDR_SIZE + request_size + BLOCK_HDR_SIZE; // request size is already aligned

    // Round up to the nearest multiple of page size
    return (size_needed + page_size - 1) / page_size * page_size;
}

/*
 * HELPER: Initialize the new zone
 * 1. Write the Zone Header
 * 2. Write the first Block Header (which covers the entire remaining space)
 */
static t_zone *init_zone(void *ptr, const t_zone_type type, const size_t zone_size) {
    // 1. Place the Zone Header at the start of the new memory
    t_zone *zone = (t_zone *)ptr;
    zone->type   = type;
    zone->size   = zone_size;
    zone->next   = NULL;

    // 2. Place the first Block Header immediately after the Zone Header
    t_block *first_block = (t_block *)((char *)zone + ZONE_HDR_SIZE);
    zone->blocks = first_block;

    first_block->next = NULL;
    first_block->size = zone_size - ZONE_HDR_SIZE - BLOCK_HDR_SIZE;
    first_block->free = (type != LARGE); // LARGE zones effectively have one block that is already "used"

    return zone;
}

/*
 * MAIN FUNCTION: Request a new zone from the system
 * WARNING: Expects g_mutex to be LOCKED by the caller!
 * Now with Thread Safety and List Appending
 */
t_zone *request_new_zone(const t_zone_type type, const size_t request_size) {
    // 1. Calculate the total size needed for this zone (including headers)
    const size_t zone_size = calculate_zone_size(type, request_size);

    // 2. Ask the kernel for memory
    // PROT_READ | PROT_WRITE : We need to read/write this memory
    // MAP_ANON | MAP_PRIVATE : Not a file, private to our process
    void *ptr = mmap(NULL, zone_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    // 3. Check for failure (mmap returns MAP_FAILED, not NULL)
    if (ptr == MAP_FAILED) {
        debug_log_event("zone", NULL, zone_size, "failed: mmap");
        return NULL;
    }

    // 4. Initialize the structures inside this raw memory
    t_zone *zone = init_zone(ptr, type, zone_size);

    // 5. Add this new zone to the global list
    t_zone **pp = &g_zones;
    while (*pp && (uintptr_t)(*pp) < (uintptr_t)zone)
        pp = &(*pp)->next;
    zone->next = *pp;
    *pp = zone;

    debug_log_event("zone", zone, zone_size, type == LARGE ? "new large zone" : "new pooled zone");
    return zone;
}
