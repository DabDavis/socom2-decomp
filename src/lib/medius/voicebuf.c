/* sc: compiler=ee-gcc2.96; flags=-O2 -G0 */
/* voicebuf.c: the voice library's per-stream pools (OVL2 0x62A870-0x62B168), between the RC cipher
 * and RSA.c in the link (no `$Header`, no SOCOM 1 counterpart: this name says what it holds, every
 * function name is a placeholder), prebuilt with ee-gcc 2.96 at -O2 -G0 (README "The GCC front").
 * audio.c and grid.c call it with a stream record (0x30 bytes, made by 0x62A870): the codec kind
 * and a limit, a pool of frame buffers (rt_window.c's 0x642968 tables) and a pool of decoders
 * (codec.c's), each pool two rings -- in use and free -- of 16-byte nodes with their counts, and
 * the one encoder with its busy flag. A ring is circular through each node's first two words; a
 * node goes on at the tail (before the head) and comes off the free ring's tail.
 *
 * The node and the stream are pointers to nodes throughout, so every store through a node may
 * alias the stream's ring heads (the target reloads them after each); a node whose address is
 * passed to the allocator lives in the frame and is reloaded after every store.
 *
 * Shapes, each measured: a ring search tests the head before a do-while over the rest (`for` with
 * a `break` lays the loop out the other way, 53s); the head read as a declaration initialiser when
 * the target loads it in the first test's delay slot (0x62B090; round 15a's rule); a codec field
 * tested again after the block that may clear it (the join reloads it; a local copy lets the jump
 * thread, 2 words); the result variable zeroed at the top and returned once (its zero stays live
 * in s1; early returns propagate the constant and swap s0/s1); a frame-resident node's two links
 * as one chained store, `p->pPrev = p->pNext = p` (two statements reload p between them); the
 * codec's reset returns an int (declared void, the constant after it takes v0, not v1); the free
 * pool's trim decrements in its test after the unlink, `if (--n <= 0)` (PRE then feeds the body
 * from both loop edges: the target's copy at the bottom test). Not written: the destroy 0x62A8C8. */

extern int rt_memory_allocate_init_zero(void **ppMemory, unsigned long nSize);
extern int rt_memory_free(void **ppMemory);
extern void *func_006132B8(int nKind, int nType, int bEncoder);    /* codec.c: create */
extern void func_00613750(void *pCodec);                            /* codec.c: destroy */
extern int func_00613790(void *pCodec);                             /* codec.c: reset (-1 without a context) */
extern int func_006137E8(void *pCodec);                             /* codec.c: its type */
extern int func_006137F0(void *pCodec);                             /* codec.c: 1 an encoder */
extern void *func_00642968(void);                                   /* rt_window.c: create */
extern void func_006429E8(void *pWindow);                           /* rt_window.c: destroy */
extern void func_00642A10(void *pWindow);                           /* rt_window.c: clear */

typedef struct VNODE {
    struct VNODE *pPrev;                /* 0x00 */
    struct VNODE *pNext;                /* 0x04 */
    int x08;
    void *pData;                        /* 0x0C: a frame buffer or a decoder */
} VNODE;

typedef struct VSTREAM {
    int nKind;                          /* 0x00: the codec kind */
    int nMax;                           /* 0x04: frame buffers at most */
    int nBufUsed;                       /* 0x08 */
    int nBufFree;                       /* 0x0C */
    VNODE *pBufUsed;                    /* 0x10 */
    VNODE *pBufFree;                    /* 0x14 */
    int nDecUsed;                       /* 0x18 */
    int nDecFree;                       /* 0x1C */
    int bEncoding;                      /* 0x20 */
    VNODE *pDecUsed;                    /* 0x24 */
    VNODE *pDecFree;                    /* 0x28 */
    void *pEncoder;                     /* 0x2C */
} VSTREAM;                              /* 0x30 */

VSTREAM *func_0062A870(int nKind, int nMax)
{
    VSTREAM *p = 0;

    rt_memory_allocate_init_zero((void **)&p, sizeof(VSTREAM));
    if (p != 0) {
        p->nKind = nKind;
        p->nMax = nMax;
    }
    return p;
}

int func_0062AAF8(VSTREAM *s)
{
    return s->nMax;
}

int func_0062AB00(VSTREAM *s)
{
    return s->nDecUsed;
}

int func_0062AB08(VSTREAM *s)
{
    return s->nBufUsed;
}

/* the encoder, made (or remade for another type) and reset, while the buffers have room */
void *func_0062AB10(VSTREAM *s, int nType, int nCount)
{
    void *pCodec;

    if (s->bEncoding != 0)
        return 0;
    if (s->nBufUsed >= s->nMax && s->nMax + 1 < nCount)
        return 0;
    if (s->pEncoder != 0) {
        if (func_006137E8(s->pEncoder) != nType) {
            func_00613750(s->pEncoder);
            s->pEncoder = 0;
        }
    }
    if (s->pEncoder == 0)
        s->pEncoder = func_006132B8(s->nKind, nType, 1);
    pCodec = s->pEncoder;
    if (pCodec != 0) {
        func_00613790(pCodec);
        s->bEncoding = 1;
    }
    return pCodec;
}

/* a decoder of the type: a free one of that type, else -- the free pool trimmed while the two
 * pools together are at the limit -- a new one while under it; on the used ring, reset */
void *func_0062ABE0(VSTREAM *s, int nType)
{
    VNODE *pTrim;
    VNODE *p;
    void *pCodec = 0;

    for (p = s->pDecFree; p != 0; ) {
        if (func_006137E8(p->pData) == nType)
            break;
        p = p->pNext;
        if (p == s->pDecFree)
            p = 0;
    }
    if (p != 0) {
        p->pNext->pPrev = p->pPrev;
        p->pPrev->pNext = p->pNext;
        if (--s->nDecFree <= 0)
            s->pDecFree = 0;
        else if (s->pDecFree == p)
            s->pDecFree = s->pDecFree->pNext;
    } else {
        while (s->nDecFree > 0 && s->nDecUsed + s->nDecFree >= s->nMax) {
            pTrim = s->pDecFree->pPrev;
            pTrim->pPrev->pNext = pTrim->pNext;
            pTrim->pNext->pPrev = pTrim->pPrev;
            if (--s->nDecFree <= 0)
                s->pDecFree = 0;
            func_00613750(pTrim->pData);
            rt_memory_free((void **)&pTrim);
        }
        if (s->nDecUsed < s->nMax) {
            rt_memory_allocate_init_zero((void **)&p, sizeof(VNODE));
            if (p != 0) {
                p->pData = func_006132B8(s->nKind, nType, 2);
                if (p->pData == 0)
                    rt_memory_free((void **)&p);
            }
        }
    }
    if (p != 0) {
        if (s->pDecUsed == 0) {
            p->pPrev = p->pNext = p;
            s->pDecUsed = p;
        } else {
            p->pNext = s->pDecUsed;
            p->pPrev = s->pDecUsed->pPrev;
            p->pNext->pPrev = p;
            p->pPrev->pNext = p;
        }
        s->nDecUsed++;
        pCodec = p->pData;
        func_00613790(pCodec);
    }
    return pCodec;
}

/* a codec given back: the encoder is idle again, a decoder goes to the free ring */
void func_0062AE10(VSTREAM *s, void *pCodec)
{
    VNODE *pHead;
    VNODE *p;

    if (pCodec == 0)
        return;
    if (func_006137F0(pCodec) == 1) {
        if (pCodec == s->pEncoder)
            s->bEncoding = 0;
        return;
    }
    p = s->pDecUsed;
    if (p == 0)
        return;
    if (p->pData != pCodec) {
        pHead = p;
        do {
            p = p->pNext;
            if (p == pHead)
                p = 0;
        } while (p != 0 && p->pData != pCodec);
    }
    if (p == 0)
        return;
    p->pNext->pPrev = p->pPrev;
    p->pPrev->pNext = p->pNext;
    if (--s->nDecUsed <= 0)
        s->pDecUsed = 0;
    else if (p == s->pDecUsed)
        s->pDecUsed = p->pNext;
    if (s->pDecFree == 0) {
        p->pNext = p;
        p->pPrev = p;
        s->pDecFree = p;
    } else {
        p->pNext = s->pDecFree;
        p->pPrev = s->pDecFree->pPrev;
        p->pNext->pPrev = p;
        p->pPrev->pNext = p;
    }
    s->nDecFree++;
}

/* a frame buffer: the free ring's tail, or a new node while under the limit, on the used ring */
void *func_0062AF40(VSTREAM *s)
{
    VNODE *p = 0;
    void *pData = 0;

    if (s->pBufFree == 0) {
        if (s->nBufUsed < s->nMax) {
            rt_memory_allocate_init_zero((void **)&p, sizeof(VNODE));
            if (p != 0) {
                p->pData = func_00642968();
                if (p->pData == 0) {
                    rt_memory_free((void **)&p);
                    p = 0;
                }
            }
        }
    } else {
        p = s->pBufFree->pPrev;
        if (p == s->pBufFree)
            s->pBufFree = 0;
        else {
            s->pBufFree->pPrev = p->pPrev;
            s->pBufFree->pPrev->pNext = s->pBufFree;
        }
        s->nBufFree--;
    }
    if (p != 0) {
        if (s->pBufUsed == 0) {
            p->pPrev = p->pNext = p;
            s->pBufUsed = p;
        } else {
            p->pNext = s->pBufUsed;
            p->pPrev = s->pBufUsed->pPrev;
            p->pNext->pPrev = p;
            p->pPrev->pNext = p;
        }
        s->nBufUsed++;
        pData = p->pData;
        func_00642A10(pData);
    }
    return pData;
}

/* a frame buffer given back: off the used ring, onto the free ring */
void func_0062B090(VSTREAM *s, void *pData)
{
    VNODE *p = s->pBufUsed;

    if (pData == 0)
        return;
    if (p == 0)
        return;
    if (p->pData != pData) {
        VNODE *pHead = p;

        do {
            p = p->pNext;
            if (p == pHead)
                p = 0;
        } while (p != 0 && p->pData != pData);
    }
    if (p == 0)
        return;
    p->pNext->pPrev = p->pPrev;
    p->pPrev->pNext = p->pNext;
    if (p == s->pBufUsed)
        s->pBufUsed = (p == p->pNext) ? 0 : p->pNext;
    s->nBufUsed--;
    if (s->pBufFree == 0) {
        p->pNext = p;
        p->pPrev = p;
        s->pBufFree = p;
    } else {
        p->pNext = s->pBufFree;
        p->pPrev = s->pBufFree->pPrev;
        p->pNext->pPrev = p;
        p->pPrev->pNext = p;
    }
    s->nBufFree++;
}
