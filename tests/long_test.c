#define _GNU_SOURCE
#include "../include/ft_malloc.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


/* --- tiny test framework --- */
static int g_fail = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        g_fail = 1; \
        fprintf(stderr, "FAIL: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
    } \
} while(0)

#define PASS(msg) do { fprintf(stdout, "OK: %s\n", msg); } while(0)

static int is_aligned(void *p, size_t align) {
    return ((uintptr_t)p % align) == 0;
}

/* --- capture stdout of show_alloc_mem into a heap buffer --- */
static char *capture_show_alloc_mem(void (*fn)(void)) {
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("pipe");
        return NULL;
    }

    int saved = dup(STDOUT_FILENO);
    if (saved < 0) {
        perror("dup");
        close(pipefd[0]); close(pipefd[1]);
        return NULL;
    }

    if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
        perror("dup2");
        close(pipefd[0]); close(pipefd[1]);
        close(saved);
        return NULL;
    }

    close(pipefd[1]); /* stdout now goes to pipe */

    fn(); /* call show_alloc_mem */

    /* restore stdout */
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);

    /* read all */
    size_t cap = 4096, len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { close(pipefd[0]); return NULL; }

    for (;;) {
        char tmp[2048];
        ssize_t r = read(pipefd[0], tmp, sizeof(tmp));
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("read");
            break;
        }
        if (r == 0) break;

        if (len + (size_t)r + 1 > cap) {
            cap = (cap * 2) + (size_t)r + 1;
            char *nb = (char *)realloc(buf, cap);
            if (!nb) { free(buf); close(pipefd[0]); return NULL; }
            buf = nb;
        }
        memcpy(buf + len, tmp, (size_t)r);
        len += (size_t)r;
    }
    close(pipefd[0]);
    buf[len] = '\0';
    return buf;
}

/* --- /proc/self/maps helpers --- */
typedef struct s_maprange {
    uintptr_t start;
    uintptr_t end;
} t_maprange;

static int find_mapping_containing(void *ptr, t_maprange *out) {
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return 0;

    uintptr_t p = (uintptr_t)ptr;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        unsigned long a, b;
        if (sscanf(line, "%lx-%lx", &a, &b) == 2) {
            if (p >= (uintptr_t)a && p < (uintptr_t)b) {
                out->start = (uintptr_t)a;
                out->end = (uintptr_t)b;
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

/* --- show_alloc_mem parsing --- */
/*
Expected-ish lines:
TINY : 0xA0000
0xA0020 - 0xA004A : 42 bytes
...
Total : 52698 bytes
*/
static int parse_hex_ptr(const char *s, uintptr_t *out) {
    while (*s == ' ' || *s == '\t') s++;
    if (s[0] != '0' || (s[1] != 'x' && s[1] != 'X')) return 0;
    errno = 0;
    unsigned long long v = strtoull(s, NULL, 16);
    if (errno != 0) return 0;
    *out = (uintptr_t)v;
    return 1;
}

static int parse_alloc_line(const char *line, uintptr_t *start, uintptr_t *end, size_t *sz) {
    /* format: 0xAAA - 0xBBB : N bytes */
    const char *p = line;

    if (!parse_hex_ptr(p, start)) return 0;

    const char *dash = strstr(line, "-");
    if (!dash) return 0;

    if (!parse_hex_ptr(dash + 1, end)) return 0;

    const char *colon = strstr(line, ":");
    if (!colon) return 0;

    unsigned long long n = 0;
    if (sscanf(colon + 1, " %llu", &n) != 1) return 0;

    *sz = (size_t)n;
    return 1;
}

static void test_alignment_and_malloc0(void) {
    for (size_t n = 1; n <= 256; n++) {
        void *p = malloc(n);
        ASSERT(p != NULL, "malloc returned NULL for small size");
        ASSERT(is_aligned(p, 16), "malloc pointer not 16-byte aligned");
        /* touch boundaries */
        ((unsigned char*)p)[0] = 0xAB;
        ((unsigned char*)p)[n - 1] = 0xCD;
        free(p);
    }
    PASS("alignment for sizes 1..256");

    void *p0 = malloc(0);
    /* Accept NULL or non-NULL, but if non-NULL it must be aligned and freeable */
    if (p0) {
        ASSERT(is_aligned(p0, 16), "malloc(0) returned non-aligned pointer");
        free(p0);
    }
    PASS("malloc(0) behavior (NULL or freeable ptr)");
}

static void test_realloc_semantics(void) {
    /* expand: content preserved */
    unsigned char *p = (unsigned char *)malloc(64);
    ASSERT(p != NULL, "malloc failed");
    for (int i = 0; i < 64; i++) p[i] = (unsigned char)i;

    unsigned char *q = (unsigned char *)realloc(p, 512);
    ASSERT(q != NULL, "realloc expand failed");
    for (int i = 0; i < 64; i++) {
        ASSERT(q[i] == (unsigned char)i, "realloc expand did not preserve prefix");
    }

    /* shrink: prefix preserved */
    unsigned char *r = (unsigned char *)realloc(q, 16);
    ASSERT(r != NULL, "realloc shrink failed");
    for (int i = 0; i < 16; i++) {
        ASSERT(r[i] == (unsigned char)i, "realloc shrink did not preserve prefix");
    }
    free(r);

    /* realloc(NULL, n) */
    void *x = realloc(NULL, 32);
    ASSERT(x != NULL, "realloc(NULL, n) failed");
    ASSERT(is_aligned(x, 16), "realloc(NULL, n) not aligned");
    free(x);

    /* realloc(ptr, 0): accept NULL or freeable ptr */
    void *y = malloc(32);
    ASSERT(y != NULL, "malloc failed");
    void *z = realloc(y, 0);
    if (z) free(z);

    PASS("realloc semantics (expand/shrink/NULL/0)");
}

static void test_100_allocs_and_show_alloc_mem_counts(void) {
    const size_t tiny_sz  = (TINY_MALLOC_LIMIT >= 16) ? 16 : (TINY_MALLOC_LIMIT ? TINY_MALLOC_LIMIT : 1);
    const size_t small_sz = (TINY_MALLOC_LIMIT + 1 <= SMALL_MALLOC_LIMIT) ? (TINY_MALLOC_LIMIT + 1) : (SMALL_MALLOC_LIMIT ? SMALL_MALLOC_LIMIT : 256);

    void *tiny[120];
    void *small[120];

    for (int i = 0; i < 120; i++) {
        tiny[i] = malloc(tiny_sz);
        ASSERT(tiny[i] != NULL, "tiny malloc failed");
        ASSERT(is_aligned(tiny[i], 16), "tiny alloc not aligned");
    }
    for (int i = 0; i < 120; i++) {
        small[i] = malloc(small_sz);
        ASSERT(small[i] != NULL, "small malloc failed");
        ASSERT(is_aligned(small[i], 16), "small alloc not aligned");
    }

    char *out = capture_show_alloc_mem(show_alloc_mem);
    ASSERT(out != NULL, "could not capture show_alloc_mem output");

    /* Count allocation lines under each category by scanning until next header.
       This is tolerant to your exact formatting as long as alloc lines start with 0x... */
    int tiny_lines = 0, small_lines = 0;
    int in_tiny = 0, in_small = 0;

    for (char *line = out; line && *line; ) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        if (strncmp(line, "TINY", 4) == 0) { in_tiny = 1; in_small = 0; }
        else if (strncmp(line, "SMALL", 5) == 0) { in_small = 1; in_tiny = 0; }
        else if (strncmp(line, "LARGE", 5) == 0) { in_small = 0; in_tiny = 0; }
        else {
            uintptr_t a,b; size_t n;
            if (parse_alloc_line(line, &a, &b, &n)) {
                if (in_tiny) tiny_lines++;
                if (in_small) small_lines++;
            }
        }

        if (!nl) break;
        line = nl + 1;
    }

    /* We allocated 120 of each; subject says zone must support at least 100.
       If you print free blocks too, counts may be higher; if you coalesce or print only used,
       this should still show at least the number of active allocations. */
    ASSERT(tiny_lines >= 100, "show_alloc_mem: expected >=100 TINY allocations printed");
    ASSERT(small_lines >= 100, "show_alloc_mem: expected >=100 SMALL allocations printed");

    free(out);

    for (int i = 0; i < 120; i++) free(tiny[i]);
    for (int i = 0; i < 120; i++) free(small[i]);

    PASS(">=100 TINY/SMALL allocations + show_alloc_mem prints them");
}

static void test_show_alloc_mem_order_and_total(void) {
    /* create a few allocations */
    void *a = malloc(24);
    void *b = malloc(48);
    void *c = malloc(TINY_MALLOC_LIMIT + 8); /* likely SMALL */
    void *d = malloc(SMALL_MALLOC_LIMIT + 4096); /* likely LARGE */
    ASSERT(a && b && c && d, "setup allocations failed");

    char *out = capture_show_alloc_mem(show_alloc_mem);
    ASSERT(out != NULL, "could not capture show_alloc_mem output");

    uintptr_t last_end = 0;
    size_t sum = 0;
    size_t printed_total = 0;
    int saw_total = 0;

    for (char *line = out; line && *line; ) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        uintptr_t s,e; size_t n;
        if (parse_alloc_line(line, &s, &e, &n)) {
            ASSERT(s < e, "alloc line has start >= end");
            ASSERT(s >= last_end, "show_alloc_mem not ordered by increasing addresses");
            /* Many implementations print end = start + size; allow either exact or end>=start+size */
            ASSERT(e >= s, "end < start");
            sum += n;
            last_end = e;
        } else {
            /* Total line */
            unsigned long long t = 0;
            if (sscanf(line, "Total : %llu", &t) == 1) {
                printed_total = (size_t)t;
                saw_total = 1;
            }
        }

        if (!nl) break;
        line = nl + 1;
    }

    ASSERT(saw_total, "show_alloc_mem did not print Total");
    ASSERT(printed_total == sum, "show_alloc_mem Total != sum of printed allocations");

    free(out);
    free(a); free(b); free(c); free(d);

    PASS("show_alloc_mem ordering + Total sum check");
}

static void test_large_is_own_mapping(void) {
    size_t large_sz = (size_t)SMALL_MALLOC_LIMIT + 8192;
    void *p = malloc(large_sz);
    void *q = malloc(large_sz);
    ASSERT(p && q, "large malloc failed");

    t_maprange mp, mq;
    ASSERT(find_mapping_containing(p, &mp), "could not find mapping containing large p");
    ASSERT(find_mapping_containing(q, &mq), "could not find mapping containing large q");

    /* Subject says each LARGE is "in a zone on their own" => distinct mmap regions */
    ASSERT(!(mp.start == mq.start && mp.end == mq.end), "two LARGE allocations share the same mapping");

    /* Free and ensure still stable */
    free(p);
    free(q);

    PASS("LARGE allocations live in distinct mappings");
}

int main(void) {
    fprintf(stdout, "Running subject-focused malloc tests...\n");

    test_alignment_and_malloc0();
    test_realloc_semantics();
    test_100_allocs_and_show_alloc_mem_counts();
    test_show_alloc_mem_order_and_total();
    test_large_is_own_mapping();

    if (g_fail) {
        fprintf(stderr, "\nSome tests FAILED.\n");
        return 1;
    }
    fprintf(stdout, "\nAll tests PASSED.\n");
    return 0;
}
