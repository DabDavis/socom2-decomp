/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 -fno-strict-aliasing -funroll-loops */
/* rsrc: the recogniser's resource blocks (OVL2 0x50D9B8-0x50DC48, SOCOM 1's object at 0x3780C8:
 * FreeRsrc, rsrc_Open, rsrc_Close, InitRsrcFunction), ScanSoft's speech recogniser, prebuilt with
 * ee-gcc 2.96 at -O2 -G0 -fno-strict-aliasing -funroll-loops (README "The GCC front"). Names are
 * SOCOM 1's, the object's two globals from its symtab (g_lhNullHandle, g_NullResource: the
 * object's .rodata). A resource is a 20-byte heap block behind a safe handle (safeh.c) whose
 * check word is the block's magic; the null handle stands for the static null resource. SOCOM 1's
 * static FreeRsrc is inlined in SOCOM II, and so is the handle-to-block lookup in rsrc_Close. */

#define RSRC_MAGIC      7777

typedef struct SAFEH {
    void *p;
    int check;
} SAFEH;

typedef struct RSRC {
    unsigned short magic;               /* +0: RSRC_MAGIC */
    void *heap;                         /* +4 */
    int f8;                             /* +8 */
    int f12;                            /* +12 */
    int f16;                            /* +16 */
} RSRC;                                 /* 20 bytes */

extern int osspi_IsGoodWritePtr(void *p, int size);
extern int safeh_HandleCheck(SAFEH h, int check, int size);
extern int safeh_HandlesEqual(SAFEH a, SAFEH b);
extern void *heap_Calloc(void *heap, int count, int size);
extern void heap_Free(void *heap, void *p);

const SAFEH g_lhNullHandle = { 0, 0 };
const RSRC g_NullResource = { RSRC_MAGIC };

static __inline__ void FreeRsrc(RSRC *r)
{
    if (r) {
        r->magic = 0;
        heap_Free(r->heap, r);
    }
}

/* the block behind a handle: the null resource for the null handle */
static __inline__ int GetRsrc(SAFEH h, RSRC **out)
{
    int err = 0;

    if (safeh_HandlesEqual(h, g_lhNullHandle))
        *out = (RSRC *)&g_NullResource;
    else {
        err = safeh_HandleCheck(h, RSRC_MAGIC, sizeof(RSRC));
        if (err < 0)
            return (err & 0x7F) == 9 ? 0x80160101 : err;
        *out = h.p;
        if (*out == 0 || (*out)->magic != RSRC_MAGIC)
            return 0x80160101;
    }
    return err;
}

int rsrc_Open(void *heap, int a1, int a2, int a3, SAFEH *out)
{
    int err;
    RSRC *r;

    err = osspi_IsGoodWritePtr(out, sizeof(SAFEH));
    r = 0;
    if (err >= 0) {
        r = heap_Calloc(heap, sizeof(RSRC), 1);
        if (r == 0)
            err = 0x80160186;
        else {
            r->heap = heap;
            r->f12 = a2;
            r->f8 = a1;
            r->f16 = a3;
            out->p = r;
            out->check = RSRC_MAGIC;
            r->f12 = a2;
            r->f8 = a1;
            r->f16 = a3;
            r->heap = heap;
            r->magic = RSRC_MAGIC;
        }
    }
    if (err < 0)
        FreeRsrc(r);
    return err;
}

int rsrc_Close(SAFEH h)
{
    RSRC *r;
    int err;

    err = GetRsrc(h, &r);
    if (err >= 0)
        FreeRsrc(r);
    return err;
}

int InitRsrcFunction(SAFEH h, RSRC **out)
{
    int err = 0;

    if (safeh_HandlesEqual(h, g_lhNullHandle))
        *out = (RSRC *)&g_NullResource;
    else {
        err = safeh_HandleCheck(h, RSRC_MAGIC, sizeof(RSRC));
        if (err < 0)
            return (err & 0x7F) == 9 ? 0x80160101 : err;
        *out = h.p;
        if (*out == 0 || (*out)->magic != RSRC_MAGIC)
            return 0x80160101;
    }
    return err;
}
