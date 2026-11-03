/* zsys_memory.cpp, MAIN's copy (MAIN 0x181950-0x181C1F): the system allocator's wrappers --
 * newlib's malloc family called with the interrupts disabled, each result checked.
 *
 * Each overlay is linked on its own (README "The map"), and MAIN links this unit too: OVL1's copy
 * (src/gamez/zsys/zsys_memory.cpp, 0x34EAE0) keeps _calloc, _realloc, _memalign, _malloc and
 * zsys_FullAllocAndFree; MAIN's keeps _strdup, _calloc, _malloc and the _new/_delete family, which
 * only MAIN's pool unit (alloc.cpp) calls. ONE source order reproduces both images and SOCOM 1's
 * zsys_memory.cpp (its symbols: zsys_MemoryReport, _strdup, _calloc, _realloc, _memalign, _malloc,
 * _new_vec, _delete_vec, _new, _delete, _free, zsys_FullAllocAndFree, in address order); defined
 * here in reverse of the image (-inline deferred), the functions MAIN strips left out.
 *
 * Two names stay placeholders: SOCOM II's game code calls the POOL versions of _strdup and _free
 * (alloc.cpp, 0x1801E0 and 0x180280: SOCOM 1's _strdup/_free call sites), so this unit's own
 * _strdup and _free (0x181950, 0x181BE0) must have been named otherwise in SOCOM II -- which
 * name is not known. The game's compiler and flags (scdiff's default). */

#include <stdlib.h>
#include <string.h>

#include "../gamez/zsys/zsys_public.h"
#include "../gamez/zvid/zvid.h"

/* SOCOM 1's assert, its file and line dropped */
#define zsys_Assert(c) zVid_Assert((c), 0x800000FF, 0, 0)

/* zsys_memory's _free (SOCOM 1's name; taken in SOCOM II by the pool's, see above) */
extern "C" void func_00181BE0(void *p, const char *file, int line)
{
    DIntr();
    free(p);
    EIntr();
}

void _delete(void *p, const char *file, int line)
{
    DIntr();
    free(p);
    EIntr();
}

void *_new(unsigned int size, const char *file, int line)
{
    void *p;

    DIntr();
    p = malloc(size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    return p;
}

void _delete_vec(void *p, const char *file, int line)
{
    if (p) {
        DIntr();
        free(p);
        EIntr();
    }
}

void *_new_vec(unsigned int size, const char *file, int line)
{
    void *p;

    DIntr();
    p = malloc(size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    return p;
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

/* zsys_memory's _strdup (SOCOM 1's name; taken in SOCOM II by the pool's, see above) */
extern "C" char *func_00181950(const char *s, const char *file, int line)
{
    unsigned int size = strlen(s) + 1;
    char *p;

    DIntr();
    p = (char *)malloc(size);
    EIntr();
    zsys_Assert(size ? p != 0 : true);
    strcpy(p, s);
    return p;
}
