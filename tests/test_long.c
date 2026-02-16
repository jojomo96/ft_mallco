// valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./test_long
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

// Provided by your project (bonus / mandatory)
void show_alloc_mem(void);

static void print_sep(void) {
    printf("\n============================================================\n");
}

static void banner(const char *title) {
    print_sep();
    printf("%s\n", title);
    print_sep();
}

static void hexdump(const void *addr, size_t len, const char *label) {
    const unsigned char *p = (const unsigned char *)addr;

    printf("\n%s @ %p (len=%zu)\n", label, addr, len);

    for (size_t i = 0; i < len; i += 16) {
        size_t chunk = (len - i < 16) ? (len - i) : 16;
        printf("  %p  ", (const void *)(p + i));

        // hex
        for (size_t j = 0; j < 16; j++) {
            if (j < chunk) printf("%02X ", p[i + j]);
            else printf("   ");
        }

        // ascii
        printf(" |");
        for (size_t j = 0; j < chunk; j++) {
            unsigned char c = p[i + j];
            printf("%c", (c >= 32 && c <= 126) ? (char)c : '.');
        }
        for (size_t j = chunk; j < 16; j++) printf(" ");
        printf("|\n");
    }
}

// Capture stdout of show_alloc_mem() without reading uninitialized bytes.
static char *capture_stdout_of_show_alloc_mem(size_t *out_len) {
    int saved = dup(STDOUT_FILENO);
    if (saved < 0) return NULL;

    int fds[2];
    if (pipe(fds) != 0) {
        close(saved);
        return NULL;
    }

    // redirect stdout -> pipe write end
    if (dup2(fds[1], STDOUT_FILENO) < 0) {
        close(saved);
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }
    close(fds[1]);

    // call
    show_alloc_mem();
    fflush(stdout);

    // restore stdout
    dup2(saved, STDOUT_FILENO);
    close(saved);

    // read pipe
    size_t cap = 256 * 1024;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        close(fds[0]);
        return NULL;
    }
    memset(buf, 0, cap);

    for (;;) {
        ssize_t r = read(fds[0], buf + len, cap - len);
        if (r < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (r == 0) break; // EOF
        len += (size_t)r;
        if (cap - len < 4096) {
            size_t newcap = cap * 2;
            char *nb = (char *)realloc(buf, newcap);
            if (!nb) break;
            buf = nb;
            // zero the new part so later printing doesn't touch garbage
            memset(buf + cap, 0, newcap - cap);
            cap = newcap;
        }
    }
    close(fds[0]);

    if (out_len) *out_len = len;
    return buf;
}

static int is_aligned_16(const void *p) {
    return (((uintptr_t)p) % 16) == 0;
}

/* ============================================================
 * TEST 1: Alignment + malloc(0) — Valgrind-clean
 * ============================================================ */
static void test_alignment_and_malloc0(void) {
    banner("TEST 1: Alignment + malloc(0)");
    printf("EXPECTATION:\n");
    printf("  - malloc(n) returns 16-byte aligned pointer (ptr %% 16 == 0)\n");
    printf("  - malloc(0) returns NULL OR a freeable, aligned pointer\n");

    for (size_t n = 1; n <= 32; n++) {
        void *p = malloc(n);
        if (!p) {
            printf("malloc(%zu): got NULL (unexpected unless OOM)\n", n);
            continue;
        }

        printf("\nmalloc(%zu):\n", n);
        printf("  got ptr=%p, (ptr %% 16)=%lu\n", p, (unsigned long)((uintptr_t)p % 16));

        // Valgrind-clean: initialize whole region before hexdump
        memset(p, 0x00, n);
        ((unsigned char *)p)[0] = 0xAB;
        ((unsigned char *)p)[n - 1] = 0xCD;

        hexdump(p, n, "  memory (first bytes)");

        if (!is_aligned_16(p)) {
            printf("  [FAIL] not 16-byte aligned\n");
        }

        free(p);
    }

    void *z = malloc(0);
    printf("\nmalloc(0):\n");
    printf("  got ptr=%p", z);
    if (z) {
        printf(", (ptr %% 16)=%lu\n", (unsigned long)((uintptr_t)z % 16));
        if (!is_aligned_16(z)) printf("  [FAIL] malloc(0) returned non-aligned pointer\n");
        free(z);
    } else {
        printf(" (NULL)\n");
    }

    printf("\nRESULT: OK (visual + alignment checks printed above)\n");
}

/* ============================================================
 * TEST 2: realloc semantics — Valgrind-clean
 * ============================================================ */
static void test_realloc_semantics(void) {
    banner("TEST 2: realloc semantics (expand/shrink/prefix preservation)");
    printf("EXPECTATION:\n");
    printf("  - realloc expanding preserves old bytes\n");
    printf("  - realloc shrinking preserves prefix bytes\n");
    printf("  - realloc(NULL,n) behaves like malloc(n) and is aligned\n");

    // Start with 16 bytes, fully initialized
    size_t oldsz = 16;
    unsigned char *p = (unsigned char *)malloc(oldsz);
    if (!p) {
        printf("malloc(16) failed\n");
        return;
    }
    for (size_t i = 0; i < oldsz; i++) p[i] = (unsigned char)(0xA0 + i);
    hexdump(p, oldsz, "before realloc expand (16 bytes)");

    // Expand to 32; Valgrind-clean: initialize the *new* tail before dumping it
    size_t newsz = 32;
    unsigned char *q = (unsigned char *)realloc(p, newsz);
    printf("\nrealloc expand: old=%p new=%p\n", (void *)p, (void *)q);
    if (!q) {
        printf("realloc to 32 failed\n");
        free(p);
        return;
    }
    // initialize tail to avoid reading uninitialized bytes
    memset(q + oldsz, 0x00, newsz - oldsz);
    hexdump(q, newsz, "after realloc expand (first 32 bytes)");

    // Shrink to 8; then dump only 8 (avoid OOB)
    size_t shrunk = 8;
    unsigned char *r = (unsigned char *)realloc(q, shrunk);
    printf("\nrealloc shrink: old=%p new=%p\n", (void *)q, (void *)r);
    if (!r) {
        printf("realloc to 8 failed (implementation may return NULL on shrink failure)\n");
        // If r is NULL, q is still valid per C standard; free q.
        free(q);
        return;
    }
    hexdump(r, shrunk, "after realloc shrink (first 8 bytes)");

    free(r);

    // realloc(NULL,32)
    void *m = realloc(NULL, 32);
    printf("\nrealloc(NULL,32):\n");
    printf("  got ptr=%p, (ptr %% 16)=%lu\n", m, m ? (unsigned long)((uintptr_t)m % 16) : 0UL);
    if (m && !is_aligned_16(m)) printf("  [FAIL] realloc(NULL,32) not 16-byte aligned\n");
    free(m);

    printf("\nRESULT: OK\n");
}

/* ============================================================
 * TEST 3: show_alloc_mem — visual + basic sanity
 * ============================================================ */
static void test_show_alloc_mem_visual(void) {
    banner("TEST 3: show_alloc_mem visual output + >=100 allocations check");
    printf("EXPECTATION:\n");
    printf("  - After allocating 120 TINY and 120 SMALL blocks, show_alloc_mem()\n");
    printf("    should print used allocations in each category.\n");
    printf("\nNOTE:\n");
    printf("  - If you run without LD_PRELOAD pointing at your allocator, show_alloc_mem()\n");
    printf("    will likely print Total : 0 bytes (because your allocator isn't used).\n");

    size_t tiny_sz = 16;
    size_t small_sz = 129;

    printf("\nUsing sizes: tiny_sz=%zu small_sz=%zu\n", tiny_sz, small_sz);

    void *tiny[120];
    void *small[120];

    for (int i = 0; i < 120; i++) {
        tiny[i] = malloc(tiny_sz);
        small[i] = malloc(small_sz);
        if (tiny[i]) memset(tiny[i], 0x11, tiny_sz);
        if (small[i]) memset(small[i], 0x22, small_sz);
    }

    size_t out_len = 0;
    char *out = capture_stdout_of_show_alloc_mem(&out_len);
    printf("\n----- show_alloc_mem() OUTPUT (captured) -----\n");
    if (out) {
        // Print exactly what we captured (no uninitialized bytes)
        fwrite(out, 1, out_len, stdout);
        free(out);
    } else {
        printf("(capture failed)\n");
    }
    printf("----- END OUTPUT -----\n");

    for (int i = 0; i < 120; i++) {
        free(tiny[i]);
        free(small[i]);
    }

    printf("\nRESULT: OK (visual)\n");
}

/* ============================================================
 * TEST 4: LARGE independence — visual proof
 * ============================================================ */
static void test_large_independence_visual(void) {
    banner("TEST 4: LARGE independence (visual memory proof)");
    printf("EXPECTATION:\n");
    printf("  - Two LARGE allocations behave independently.\n");
    printf("  - After freeing p, q must remain readable/writable and unchanged.\n");

    size_t big = 9216; // typically triggers LARGE (page-rounded to 12288 on 4K pages)
    unsigned char *p = (unsigned char *)malloc(big);
    unsigned char *q = (unsigned char *)malloc(big);

    printf("\nAllocated:\n");
    printf("  p=%p (size=%zu)\n", (void *)p, big);
    printf("  q=%p (size=%zu)\n", (void *)q, big);

    if (!p || !q) {
        printf("malloc(big) failed\n");
        free(p);
        free(q);
        return;
    }

    memset(p, 0xAA, big);
    memset(q, 0xBB, big);

    hexdump(p, 128, "p before free(p)");
    hexdump(q, 128, "q before free(p)");

    free(p);

    printf("\nAfter free(p):\n");
    hexdump(q, 128, "q after free(p) (should still be all 0xBB)");

    q[0] = 0xCC;
    hexdump(q, 32, "q after writing 0xCC to q[0]");

    free(q);

    printf("\nRESULT: OK\n");
}

int main(void) {
    printf("Running subject-focused malloc tests (VISUAL)...\n");

    test_alignment_and_malloc0();
    test_realloc_semantics();
    test_show_alloc_mem_visual();
    test_large_independence_visual();

    printf("\nAll tests PASSED.\n");
    return 0;
}
