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
```

This produces:

- `libft_malloc_$(HOSTTYPE).so`
- `libft_malloc.so` (symlink)

---

## Usage

### Using LD_PRELOAD (recommended)

You can preload the allocator into any executable:

```sh
export MY_MALLOC=./libft_malloc.so
LD_PRELOAD=$MY_MALLOC ls
```

Example:

```sh
LD_PRELOAD=$MY_MALLOC grep "malloc" Makefile
```

---

## Debug Options

### Scribble Mode (bonus)

If enabled, memory is filled with recognizable patterns:

- `0xAA` on allocation
- `0x55` on free

Enable with:

```sh
export MallocScribble=1
```

---

## Project Structure

```
.
├── include/
│   └── ft_malloc.h
├── src/
│   ├── malloc.c
│   ├── free.c
│   ├── realloc.c
│   ├── calloc.c
│   ├── zone_utils.c
│   ├── show_alloc_mem.c
│   └── show_alloc_mem_ex.c
├── Makefile
└── README.md
```

---

## Notes

- This allocator is designed to be compatible with real-world binaries when used through `LD_PRELOAD`.
- Behavior for edge cases such as `malloc(0)` follows common libc-compatible allocator behavior.
- Thread safety is ensured through a global mutex.

---

## Author

**jmoritz**  
42 School project — custom memory allocator implementation.
