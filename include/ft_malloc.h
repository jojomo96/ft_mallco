#ifndef FT_MALLOC_LIBRARY_H
#define FT_MALLOC_LIBRARY_H

#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "../libft/include/libft/libft.h"

/* -------------------------------------------------------------------------- */
/* CONSTANTS                                  */
/* -------------------------------------------------------------------------- */

# define TINY_MALLOC_LIMIT 128
# define SMALL_MALLOC_LIMIT 1024

#define MIN_ALLOCS 100 //512

#define MALLOC_ALIGN 16UL
#define ALIGN_UP(x) (((x) + (MALLOC_ALIGN - 1)) & ~(MALLOC_ALIGN - 1))

#define ZONE_HDR_SIZE  ALIGN_UP(sizeof(t_zone))
#define BLOCK_HDR_SIZE ALIGN_UP(sizeof(t_block))


/* -------------------------------------------------------------------------- */
/* STRUCTURES                                  */
/* -------------------------------------------------------------------------- */

/*
 * Block Header (Metadata for each allocation)
 * This sits immediately before the memory returned to the user.
 */
typedef struct s_block {
    struct s_block *next; // Pointer to the next block in this zone
    size_t          size; // Size of the user data (excluding this header)
    int             free; // 1 if available, 0 if used
} t_block;

/*
 * Zone Header (Metadata for the big mmap regions)
 * This sits at the very beginning of a new memory page.
 */
typedef enum e_zone_type {
    TINY,
    SMALL,
    LARGE
} t_zone_type;

typedef struct s_zone {
    struct s_zone *next;   // Pointer to the next zone in the global list
    t_block *      blocks; // Pointer to the first block in this zone
    size_t         size;   // Total size of this zone (including metadata)
    t_zone_type    type;   // Type of this zone (TINY, SMALL, LARGE)
} t_zone;


/* -------------------------------------------------------------------------- */
/* GLOBALS                                   */
/* -------------------------------------------------------------------------- */

extern t_zone *g_zones; // Global linked list of all zones
extern pthread_mutex_t g_mutex; // Mutex for thread safety
extern int g_malloc_scribble; // If set, fill allocated memory with 0xAA and freed memory with 0x55 for debugging

/* -------------------------------------------------------------------------- */
/* PROTOTYPES                                  */
/* -------------------------------------------------------------------------- */

void *malloc(size_t size);
void  free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void  show_alloc_mem(void);
void  show_alloc_mem_ex(void);

/* -------------------------------------------------------------------------- */
/* INTERNAL SHARED HELPERS (Do not call these from outside)                   */
/* -------------------------------------------------------------------------- */

/* Core logic without locks (used by realloc to avoid deadlocks) */
void *malloc_nolock(size_t size);
void  free_nolock(void *ptr);

/* Utility functions shared across files */
size_t  align_size(size_t size);
void    split_block(t_block *block, size_t size);
void    coalesce_right(t_block *current);
t_zone *request_new_zone(t_zone_type type, size_t request_size);

#endif // FT_MALLOC_LIBRARY_H
