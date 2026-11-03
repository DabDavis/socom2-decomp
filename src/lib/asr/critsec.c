/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* critsec: the recogniser's critical sections (OVL2 0x50D810-0x50D8D0, SOCOM 1's object at
 * 0x377F20: the same five functions in the same order, s1:order), ScanSoft's speech recogniser,
 * prebuilt with ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC
 * front"). On the PS2 a section is a heap block and entering one does nothing. Errors are
 * ScanSoft's 0x80MMxxxx codes, MM the module (0x14 here). */

typedef struct CRITSEC {
    int f0;
    void *heap;                         /* +4 */
} CRITSEC;

extern void *func_00243298(void *heap, int size);                  /* the heap's malloc */
extern void heap_Free(void *heap, void *p);

int critsec_GetVersion(long *version)
{
    if (version == 0)
        return 0x80140183;
    *version = 0x01000000;
    return 0;
}

int critsec_ObjOpen(void *user, void *heap, void *rsrc, CRITSEC **out)
{
    CRITSEC *cs;

    if (out == 0)
        return 0x80140183;
    cs = func_00243298(heap, sizeof(CRITSEC));
    if (cs == 0)
        return 0x80140186;
    cs->heap = heap;
    *out = cs;
    return 0;
}

int critsec_ObjClose(CRITSEC *cs)
{
    heap_Free(cs->heap, cs);
    return 0;
}

int critsec_Enter(CRITSEC *cs)
{
    return 0;
}

int critsec_Leave(CRITSEC *cs)
{
    return 0;
}
