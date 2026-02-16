#include <stdint.h>
#include <stdlib.h>

#include "ft_malloc.h"

int g_malloc_scribble = 0;
int g_malloc_debug    = 0;

static size_t append_text(char *buf, size_t offset, const char *text)
{
    while (text && *text)
        buf[offset++] = *text++;
    return offset;
}

static size_t append_size(char *buf, size_t offset, size_t value)
{
    char   digits[32];
    size_t index = 0;

    if (value == 0)
        return append_text(buf, offset, "0");
    while (value > 0) {
        digits[index++] = (char)('0' + (value % 10));
        value /= 10;
    }
    while (index > 0)
        buf[offset++] = digits[--index];
    return offset;
}

static size_t append_ptr(char *buf, size_t offset, const void *ptr)
{
    char                digits[2 * sizeof(uintptr_t)];
    size_t              index = 0;
    uintptr_t           value = (uintptr_t) ptr;
    static const char  *hex   = "0123456789abcdef";

    if (!ptr)
        return append_text(buf, offset, "(nil)");
    offset = append_text(buf, offset, "0x");
    while (value > 0) {
        digits[index++] = hex[value & 0xF];
        value >>= 4;
    }
    while (index > 0)
        buf[offset++] = digits[--index];
    return offset;
}

void debug_log_event(const char *event, const void *ptr, size_t size, const char *detail)
{
    char   buffer[256];

    if (!g_malloc_debug)
        return;
    size_t len = 0;
    len = append_text(buffer, len, "[ft_malloc] ");
    len = append_text(buffer, len, event);
    len = append_text(buffer, len, " ptr=");
    len = append_ptr(buffer, len, ptr);
    len = append_text(buffer, len, " size=");
    len = append_size(buffer, len, size);
    if (detail && *detail) {
        len = append_text(buffer, len, " ");
        len = append_text(buffer, len, detail);
    }
    buffer[len++] = '\n';
    write(STDERR_FILENO, buffer, len);
}

// Bonus

void debug_log_block_merge(const t_block *left, const t_block *right, size_t merged_size)
{
    char   buffer[256];
    size_t len;

    if (!g_malloc_debug)
        return;
    len = 0;
    len = append_text(buffer, len, "[ft_malloc] coalesce left=");
    len = append_ptr(buffer, len, left);
    len = append_text(buffer, len, " right=");
    len = append_ptr(buffer, len, right);
    len = append_text(buffer, len, " merged_size=");
    len = append_size(buffer, len, merged_size);
    buffer[len++] = '\n';
    write(STDERR_FILENO, buffer, len);
}

void debug_log_block_split(const t_block *block, size_t requested_size, size_t remainder_size)
{
    char   buffer[256];
    size_t len;

    if (!g_malloc_debug)
        return;
    len = 0;
    len = append_text(buffer, len, "[ft_malloc] split block=");
    len = append_ptr(buffer, len, block);
    len = append_text(buffer, len, " requested=");
    len = append_size(buffer, len, requested_size);
    len = append_text(buffer, len, " remainder=");
    len = append_size(buffer, len, remainder_size);
    buffer[len++] = '\n';
    write(STDERR_FILENO, buffer, len);
}

void __attribute__((constructor)) init_malloc_debug(void)
{
    const char *scribble = getenv("MallocScribble");
    const char *debug    = getenv("MallocDebug");

    g_malloc_scribble = 0;
    g_malloc_debug    = 0;
    if (scribble && scribble[0] != '\0' && !(scribble[0] == '0' && scribble[1] == '\0'))
        g_malloc_scribble = 1;
    if (debug && debug[0] != '\0' && !(debug[0] == '0' && debug[1] == '\0'))
        g_malloc_debug = 1;
}
