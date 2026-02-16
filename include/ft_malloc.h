#ifndef FT_MALLOC_LIBRARY_H
#define FT_MALLOC_LIBRARY_H

#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "libft/libft.h"

/* -------------------------------------------------------------------------- */
/* Constants                                                                   */
/* -------------------------------------------------------------------------- */

# define TINY_MALLOC_LIMIT 128
# define SMALL_MALLOC_LIMIT 1024

#define MIN_ALLOCS 100

#define MALLOC_ALIGN 16UL
#define ALIGN_UP(x) (((x) + (MALLOC_ALIGN - 1)) & ~(MALLOC_ALIGN - 1))

#define ZONE_HDR_SIZE  ALIGN_UP(sizeof(t_zone))
#define BLOCK_HDR_SIZE ALIGN_UP(sizeof(t_block))


/* -------------------------------------------------------------------------- */
/* Data structures                                                             */
/* -------------------------------------------------------------------------- */

/*
 * Block Header (Metadata for each allocation)
 * This sits immediately before the memory returned to the user.
 */
typedef struct s_block {
    struct s_block *next; /* Next block in the same zone. */
    size_t          size; /* User payload size (excluding metadata header). */
    int             free; /* 1 when available, 0 when currently allocated. */
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
    struct s_zone *next;   /* Next zone in the global zone list. */
    t_block *      blocks; /* First block contained in this zone. */
    size_t         size;   /* Total mapped zone size, metadata included. */
    t_zone_type    type;   /* Zone class: TINY, SMALL, or LARGE. */
} t_zone;


/* -------------------------------------------------------------------------- */
/* Globals                                                                     */
/* -------------------------------------------------------------------------- */

extern t_zone *g_zones;          /* Global linked list of all zones. */
extern pthread_mutex_t g_mutex;  /* Allocator-wide mutex for thread safety. */
extern int g_malloc_scribble;    /* Fill allocated/free memory with patterns when enabled. */
extern int g_malloc_debug;       /* Emit allocator debug traces to stderr when enabled. */

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

void *malloc(size_t size);
void  free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);
void  show_alloc_mem(void);
void  show_alloc_mem_ex(void);

/* -------------------------------------------------------------------------- */
/* Internal shared helpers (not part of the public API)                        */
/* -------------------------------------------------------------------------- */

/* Core logic without locks (used by realloc to avoid deadlocks) */
void *malloc_nolock(size_t size);
void  free_nolock(void *ptr);

/* Utility helpers shared across files. */
size_t  align_size(size_t size);
void    split_block(t_block *block, size_t size);
void    coalesce_right(t_block *current);
t_zone *request_new_zone(t_zone_type type, size_t request_size);

/* Debug helpers. */
void debug_log_event(const char *event, const void *ptr, size_t size, const char *detail);
void debug_log_block_merge(const t_block *left, const t_block *right, size_t merged_size);
void debug_log_block_split(const t_block *block, size_t requested_size, size_t remainder_size);
void debug_log_malloc_placement(const t_zone *zone, const t_block *block, size_t requested_size,
                                size_t aligned_size, size_t original_block_size, int from_new_zone);

#endif
