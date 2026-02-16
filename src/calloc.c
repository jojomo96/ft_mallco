#include <stdint.h>
#include "ft_malloc.h"
/*
 * calloc: Allocates memory for an array of nmemb elements of size bytes each
 * and returns a pointer to the allocated memory. The memory is set to zero. 
 * If nmemb or size is 0, then calloc() returns either NULL, or a unique pointer
 * value that can later be successfully passed to free().
 */
void *calloc(size_t nmemb, size_t size) {
    // Check for overflow: nmemb * size
    // If nmemb is not 0 and size > SIZE_MAX / nmemb, then nmemb * size would overflow
    if (nmemb != 0 && size > SIZE_MAX / nmemb) {
        debug_log_event("calloc", NULL, size, "failed: multiplication overflow");
        return NULL;
    }

    const size_t total_size = nmemb * size;
    // Use our custom malloc
    void* ptr = malloc(total_size);
    if (ptr) {
        ft_memset(ptr, 0, total_size);
    }
    debug_log_event("calloc", ptr, total_size, ptr ? "ok" : "failed: malloc");
    return ptr;
}
