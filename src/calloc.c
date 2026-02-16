#include <stdint.h>

#include "ft_malloc.h"

/*
 * calloc(nmemb, size): allocate nmemb * size bytes and zero-fill them.
 *
 * Important details:
 * - detect multiplication overflow before computing total
 * - delegate allocation to our malloc implementation
 * - explicitly memset returned region to 0 for calloc contract
 */
void *calloc(size_t nmemb, size_t size) {
    /* Overflow check for nmemb * size. */
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        debug_log_event("calloc", NULL, size, "failed: multiplication overflow");
        return NULL;
    }

    const size_t total_size = nmemb * size;

    /* Reuse allocator's central path (alignment, zone selection, etc.). */
    void *ptr = malloc(total_size);

    /* calloc guarantee: every byte is initialized to zero. */
    if (ptr)
        ft_memset(ptr, 0, total_size);

    debug_log_event("calloc", ptr, total_size, ptr ? "ok" : "failed: malloc");
    return ptr;
}
