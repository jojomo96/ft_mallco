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
        size_needed = sizeof(t_zone) + MIN_ALLOCS * (TINY_MALLOC_LIMIT + sizeof(t_block));
    else if (type == SMALL)
        // 100 blocks of (MAX_SMALL + Header)
        size_needed = sizeof(t_zone) + MIN_ALLOCS * (SMALL_MALLOC_LIMIT + sizeof(t_block));
    else
        // LARGE: Just the request + headers
        size_needed = sizeof(t_zone) + request_size + sizeof(t_block);

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
    t_zone *zone = ptr;
    zone->type   = type;
    zone->size   = zone_size;
    zone->next   = NULL;

    // 2. Place the first Block Header immediately after the Zone Header
    t_block *first_block = (void *) zone + sizeof(t_zone);
    zone->blocks         = first_block;
    first_block->next    = NULL;
    first_block->size    = zone_size - sizeof(t_zone) - sizeof(t_block);
    first_block->free    = type != LARGE ? 1 : 0; // LARGE zones effectively have one block that is already "used"

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
    if (ptr == MAP_FAILED)
        return NULL;

    // 4. Initialize the structures inside this raw memory
    t_zone *zone = init_zone(ptr, type, zone_size);

    // 5. Add this new zone to the global list
    t_zone **pp = &g_zones;
    while (*pp)
        pp = &(*pp)->next;
    *pp = zone;

    return zone;
}
