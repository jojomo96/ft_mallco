#include "../include/ft_malloc.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

/* * TOGGLE THIS TO 1 FOR CLEAN VALGRIND RUNS
 * When 1: Skips illegal operations (Double Free, Stack Free, Use-After-Free)
 * When 0: Runs full robustness tests (Good for defense, bad for Valgrind)
 */
#define VALGRIND_MODE 1

/* Colors */
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define BOLD    "\033[1m"

/* Prototype for bonus function */
void show_alloc_mem_ex(void);

void print_header(char *title) {
    printf("\n%s" BLUE "========================================\n", BOLD);
    printf(" %s\n", title);
    printf("========================================" RESET "\n");
}

void print_result(char *test_name, int condition) {
    printf("%-40s [%s%s" RESET "]\n", test_name, condition ? GREEN : RED, condition ? "OK" : "FAIL");
}

/* -------------------------------------------------------------------------- */
/* TEST 1: Mandatory Allocations                                              */
/* -------------------------------------------------------------------------- */
void test_mandatory(void) {
    print_header("TEST 1: Mandatory Categories");

    void *t1 = malloc(10);
    void *t2 = malloc(120);
    print_result("Malloc Tiny", t1 != NULL && t2 != NULL);

    void *s1 = malloc(500);
    void *s2 = malloc(1000);
    print_result("Malloc Small", s1 != NULL && s2 != NULL);

    void *l1 = malloc(10000);
    void *l2 = malloc(20000);
    print_result("Malloc Large", l1 != NULL && l2 != NULL);

    free(t1); free(t2);
    free(s1); free(s2);
    free(l1); free(l2);
    print_result("Free All", 1);
}

/* -------------------------------------------------------------------------- */
/* TEST 2: Realloc Logic                                                      */
/* -------------------------------------------------------------------------- */
void test_realloc(void) {
    print_header("TEST 2: Realloc Mechanics");

    char *ptr = malloc(20);
    strcpy(ptr, "Hello World");

    // Expansion
    char *new_ptr = realloc(ptr, 60);
    print_result("Realloc Expansion", new_ptr != NULL);

    // Shrinking
    char *shrunk_ptr = realloc(new_ptr, 10);
    print_result("Realloc Shrink", shrunk_ptr != NULL);

    // Realloc NULL -> Malloc
    void *r_null = realloc(NULL, 50);
    print_result("Realloc(NULL)", r_null != NULL);

#if !VALGRIND_MODE
    // Realloc 0 -> Free
    // Valgrind considers realloc(ptr, 0) an error because behavior varies by OS/Std.
    void *r_zero = realloc(r_null, 0);
    if (r_zero) free(r_zero);
    print_result("Realloc(ptr, 0)", 1);
#else
    // In Valgrind mode, we manually free r_null since we skip the realloc(0) check
    free(r_null);
    printf(YELLOW "[SKIPPING] realloc(ptr, 0) (Valgrind dislikes 0 size)\n" RESET);
#endif

    free(shrunk_ptr);
}

/* -------------------------------------------------------------------------- */
/* TEST 3: Edge Cases                                                         */
/* -------------------------------------------------------------------------- */
void test_edge_cases(void) {
    print_header("TEST 3: Edge Cases");

    // 1. Malloc 0
    void *p0 = malloc(0);
    if (p0) free(p0); // Fix for Valgrind: if it returns a ptr for size 0, free it.
    print_result("Malloc(0) handled", 1);

    // 2. Free NULL
    free(NULL);
    print_result("Free(NULL) safe", 1);

#if !VALGRIND_MODE
    // THESE TESTS INTENTIONALLY BREAK RULES.
    // They are disabled in VALGRIND_MODE to avoid "Invalid Free" errors.

    // 3. Free Stack
    int stack_var = 42;
    free((void *)&stack_var);
    print_result("Free(StackPtr) ignored", 1);

    // 4. Double Free
    void *df = malloc(50);
    free(df);
    free(df);
    print_result("Double Free ignored", 1);
#else
    printf(YELLOW "[SKIPPING] Stack Free & Double Free (Incompatible with Valgrind)\n" RESET);
#endif
}

/* -------------------------------------------------------------------------- */
/* TEST 4: Thread Safety (Stress Test)                                        */
/* -------------------------------------------------------------------------- */
#define THREAD_COUNT 10
#define ITERATIONS 50

void *thread_routine(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++) {
        size_t size = (rand() % 2000) + 1;
        void *ptr = malloc(size);
        if (ptr) {
            memset(ptr, 0x11, size);
            if (rand() % 2 == 0) free(ptr);
            else {
                ptr = realloc(ptr, size + 50);
                free(ptr);
            }
        }
    }
    return NULL;
}

void test_threads(void) {
    print_header("TEST 4: Thread Safety");

    pthread_t threads[THREAD_COUNT];
    printf("Spawning %d threads...\n", THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_create(&threads[i], NULL, thread_routine, NULL);

    for (int i = 0; i < THREAD_COUNT; i++)
        pthread_join(threads[i], NULL);

    print_result("Multi-threaded stress test", 1);
}

/* -------------------------------------------------------------------------- */
/* TEST 5: Bonus Features                                                     */
/* -------------------------------------------------------------------------- */
void test_bonus(void) {
    print_header("TEST 5: Bonus Features");

    char *s = malloc(64);
    if (!s) return;

    strcpy(s, "Check HexDump for 0xAA...");

    // Only show visualizer if not in automated Valgrind (too much output)
    // show_alloc_mem_ex();

    free(s);

#if !VALGRIND_MODE
    // Reading freed memory is ILLEGAL in standard C.
    // Valgrind will correctly scream "Invalid Read" here.
    unsigned char *unsafe_ptr = (unsigned char *)s;
    if (unsafe_ptr[0] == 0x55) {
        print_result("Scribble (0x55) Verified", 1);
    }
#else
    printf(YELLOW "[SKIPPING] Use-After-Free Check (Incompatible with Valgrind)\n" RESET);
    printf("To verify scribble: Check HexDump manually or run without Valgrind.\n");
#endif
}

/* -------------------------------------------------------------------------- */
/* MAIN                                                                       */
/* -------------------------------------------------------------------------- */
int main(void) {
    test_mandatory();
    test_realloc();
    test_edge_cases();
    test_bonus();
    test_threads();
    return 0;
}