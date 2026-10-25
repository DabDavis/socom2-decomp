/* zsys_memory.cpp -- the system allocator's wrappers: newlib's malloc family called with the
 * interrupts disabled, each result checked (OVL1 0x34EAE0-0x34ECF3). The file name is SOCOM 1's
 * (its asserts name zsys_memory.cpp); SOCOM II's asserts pass neither file nor line, and its
 * _strdup, zsys_MemoryReport and the _new/_delete family are not linked. Defined in reverse of
 * their image order (-inline deferred). */

#include <stdlib.h>
#include <string.h>

#include "zsys_public.h"
#include "../zvid/zvid.h"

extern "C" {
void *func_00197808(void *p, unsigned int size);        /* newlib's realloc (MAIN) */
void *func_00194A00(unsigned int align, unsigned int size); /* newlib's memalign (MAIN) */
}

/* SOCOM 1's assert, its file and line dropped */
#define zsys_Assert(c) zVid_Assert((c), 0x800000FF, 0, 0)

int __HeapSize;     /* 0x49E280 (SOCOM 1's) */

/* the largest block the heap can give, found by asking (SOCOM 1's) */
int zsys_FullAllocAndFree()
{
    unsigned int size;

    for (size = 4096; size <= 0x2000000; size += 4096) {
        void *p = malloc(size);

        if (!p)
            break;
        free(p);
    }
    __HeapSize = size - 16;
    return __HeapSize;
}

void *_malloc(unsigned int size, const char *file, int line)
{
    void *p;

    DIntr();
    p = malloc(size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    return p;
}

void *_memalign(unsigned int align, unsigned int size, const char *file, int line)
{
    void *p;

    DIntr();
    p = func_00194A00(align, size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    return p;
}

void *_realloc(void *old, unsigned int size, const char *file, int line)
{
    void *p;

    DIntr();
    p = func_00197808(old, size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    return p;
}

void *_calloc(unsigned int n, unsigned int size, const char *file, int line)
{
    void *p;
    unsigned int total = n * size;

    DIntr();
    p = malloc(total);
    EIntr();
    zsys_Assert(total ? p != 0 : true);
    memset(p, 0, total);
    return p;
}
