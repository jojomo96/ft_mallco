#include <stdlib.h>
#include "ft_malloc.h"

int g_malloc_scribble = 0;

//Bonus
void __attribute__((constructor)) init_malloc_debug(void)
{
    const char *v = getenv("MallocScribble");

    g_malloc_scribble = 0;
    if (v && v[0] != '\0' && !(v[0] == '0' && v[1] == '\0'))
        g_malloc_scribble = 1;
}
