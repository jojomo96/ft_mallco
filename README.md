# ft_malloc

A custom implementation of `malloc`, `free`, `realloc` and `calloc` written in C, using `mmap()` to manage memory zones.

This project re-implements dynamic memory allocation without relying on the system libc allocator, while respecting common allocator constraints such as alignment, fragmentation handling, and memory visualization.

---

## Features

- Custom implementation of:
    - `malloc`
    - `free`
    - `realloc`
    - `calloc`
- Memory allocation based on `mmap()` (no use of libc malloc)
- Allocation split into three categories:
    - **TINY**
    - **SMALL**
    - **LARGE**
- Block splitting to reduce wasted memory
- Block coalescing to reduce fragmentation
- Thread-safe implementation using a global mutex
- Debug memory visualization:
    - `show_alloc_mem()`
    - `show_alloc_mem_ex()` (bonus)

---

## Allocation Strategy

The allocator is organized into **zones**.  
Each zone contains multiple blocks and is dedicated to one allocation type.

### TINY / SMALL

- TINY and SMALL allocations are stored inside shared preallocated zones.
- Zones are allocated as multiples of the system page size (`getpagesize()`).
- Each zone contains at least 100 allocations worth of space.
- Blocks inside a zone are managed using a linked list.

### LARGE

- LARGE allocations are mapped separately.
- Each LARGE allocation receives its own dedicated `mmap()` zone.

---

## Alignment

All returned memory pointers are aligned to **16 bytes** to satisfy modern CPU alignment requirements and ensure safe usage with any standard type.

---

## Memory Visualization

### `show_alloc_mem()`

Prints all active allocations grouped by zone type:

- TINY
- SMALL
- LARGE

Output includes:
- zone address
- block start/end addresses
- allocation size
- total allocated bytes

### `show_alloc_mem_ex()` (bonus)

Extended memory dump mode which includes a hex view of block contents.

---

## Compilation

Build the shared library:

```sh
make
