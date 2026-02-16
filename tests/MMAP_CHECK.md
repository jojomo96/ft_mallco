# mmap / munmap verification (ft_malloc)

This note documents how to **prove** that `TINY/SMALL` use preallocated zones and that each `LARGE` allocation is in its **own mmap()** (and is **munmap()'d** on `free()`), as required by the subject.

---

## 1) Why `wc -l` on raw `strace` is misleading

If you run:

```sh
strace -f -e trace=mmap,munmap ./test_long 2>&1 | wc -l
```

the number will often be **huge** (hundreds). That count includes:
- dynamic loader mappings (`ld-linux`)
- shared library segments (`libc`, `libpthread`, your .so)
- TLS / thread stacks / libc internals

So it does **not** measure your allocator’s zone behavior.

---

## 2) Correct metric: count only anonymous mappings

Your allocator’s zones are **anonymous private** mappings:
- `MAP_PRIVATE|MAP_ANONYMOUS`
- `PROT_READ|PROT_WRITE`

Count only those:

```sh
strace -f -e trace=mmap,munmap ./test_long 2>&1 \
  | grep 'MAP_ANONYMOUS' \
  | wc -l
```

### What “good” looks like
You should see a **small** number (single digits to low tens), because:
- **TINY** typically needs ~1 zone mmap (maybe a few)
- **SMALL** typically needs ~1 zone mmap (maybe a few)
- **LARGE** uses 1 mmap **per** large allocation

If this number is close to the number of allocations, it would indicate a bug (mmap per alloc). If it’s small, your allocator is amortizing correctly.

---

## 3) Attribute anonymous mmaps to their call site (proof it’s yours)

Use `-k` to include a short stack trace per syscall:

```sh
strace -f -k -e trace=mmap,munmap ./test_long 2>&1 \
  | grep -A6 'MAP_ANONYMOUS'
```

### What to look for
You will see two kinds of backtraces:

#### A) Loader / libc noise (not your allocator)
Example frames contain:
- `/usr/lib/.../ld-linux-x86-64.so.2(...)`
- `/usr/lib/.../libc.so.6(...)`

These are normal process setup and not allocator behavior.

#### B) Your allocator’s zone creation (this is what you want)
Look for frames like:
- `libft_malloc... request_new_zone`
- `malloc_nolock`
- `malloc` / `realloc`

This proves the anonymous mapping was created by your allocator.

---

## 4) Prove each LARGE allocation is a separate mmap and is munmap()’d on free()

A simple and very strong check is to filter for `12288` (example page-rounded size)
or any known LARGE zone size seen in your runs.

Run (noisy but quick):

```sh
strace -f -e trace=mmap,munmap ./test_long 2>&1 \
  | grep -E 'mmap|munmap' \
  | grep -E '12288|9216|8192'
```

### What “correct” looks like in the output

You should see patterns like:

- **allocation**:
  ```
  mmap(NULL, 12288, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x....
  ```

- **free**:
  ```
  munmap(0x...., 12288) = 0
  ```

Key points:
- each `LARGE` allocation corresponds to a **distinct** `mmap(...) = 0xADDR`
- freeing that allocation triggers a matching `munmap(0xADDR, size) = 0`

That is exactly “LARGE is in a zone on its own”:
- it is created via its own mapping
- it is released back to the OS (munmap)

> Note: Linux may merge adjacent VMAs in `/proc/self/maps`. Therefore,
> tests that compare `/proc/self/maps` *ranges* can false-fail even when
> `mmap()` was called twice. `strace` is the authoritative proof of syscalls.

---

## 5) Clean logs (avoid interleaving your program output with strace)

If your test prints a lot, stderr/stdout can interleave with strace output.
Write the trace to a file:

```sh
strace -f -e trace=mmap,munmap -o /tmp/ft_malloc_trace.txt ./test_long
grep 'MAP_ANONYMOUS' /tmp/ft_malloc_trace.txt | wc -l
grep -E 'mmap|munmap' /tmp/ft_malloc_trace.txt | grep -E '12288|9216|8192'
```

This makes the evidence easy to read and share.

---

## Summary: what we proved

- `TINY/SMALL` do **not** mmap per allocation (anonymous mmap count stays low).
- `LARGE` allocations show **one mmap per large alloc** and **one munmap per free**.
- `/proc/self/maps` range checks are unreliable for “same mapping” because the kernel
  can merge adjacent VMAs; syscall traces (`strace`) are definitive.
