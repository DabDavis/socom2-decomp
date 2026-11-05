/* zparticle_buf.cpp -- the particle draw buffer (OVL1 0x324C70-0x326F4F): the frame's particles
 * gathered as buffer elements, sorted back to front and drawn as GS packets. The unit name is
 * SOCOM 1's (its __sinit_zparticle_buf.cpp; SOCOM II's 0x402BB0 builds <iostream>'s pair and the
 * 5000 32-byte elements at 0x45C4D0 with the constructor 0x326F40). SOCOM 1's demo holds DrawRange to the element's
 * constructor in this order. Defined in reverse of their image order (-inline deferred). Written
 * so far: DrawRange, DrawFlush, the sort's comparison and DrawPrep (its zqsort instance, 0x326C30,
 * declared only). */

#include <eetypes.h>
#include <iostream>
#include "zparticle.h"

/* the renderer's pipe, as much as the buffer reads: its camera (zrndr_pipe.cpp's class; the
 * camera's matrix set's view at 0x3F0 -- SOCOM 1's 0x3C0, in its m_mtxSet -- the names ours) */
struct ZCAM_VIEW {
    unsigned char _0[0x20];   // 0x0
    float m[4][4];   // 0x20
};
class CPipeCamera {
public:
    unsigned char _0[0x3f0];   // 0x0
    ZCAM_VIEW *m_mtx;   // 0x3f0
};
class CPipe {
public:
    unsigned char _0[0x628];   // 0x0
    CPipeCamera *m_camera;   // 0x628
};
extern CPipe thePipe;   // 0x4887C0

namespace Particle {

/* one particle as drawn: zparticle.h's BufferElement */

/* the frame's elements (SOCOM 1's buffer, 5000 of them: the unit's initialiser 0x402BB0 constructs
 * them in place, 0x45C4D0) */
BufferElement buffer[5000];

/* the sorted elements, drawn from the last back to the first (SOCOM 1's) */
extern BufferElement *bufferPtr[5000];   // 0x4835F0
extern BufferElement **pParticleBuf;   // 0x4835D8
extern zdb::CTexHandle *m_lastTexture;   // 0x4835D0

int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b);

/* the elements buffered this frame (SOCOM 1's numParticles, 0x45C4C0) */
extern int numParticles;

}

/* the system's packet ring (zsys_main.cpp) */
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoKick(u_long128 *packet, unsigned int qwc);

/* SOCOM 1's sort of an array by a comparison: an explicit-stack quicksort -- the median of three
 * as the pivot, the larger side's range pushed, ranges of nine or fewer left -- finished by an
 * insertion sort (its instance for the elements is this unit's tail, 0x326C30; SOCOM 1's 0x2360A0
 * has the same blocks). The pivot and the swaps' value live in memory: the comparison takes them by
 * reference. Written in the world lane's round 20 from the target: the frame (1424: three spill
 * slots, the 300-int stack at sp+208, t and the pivot at 1416/1420), the prologue, lo and hi
 * spilled, the loop test's reloads into i and j (`(j = hi) > (i = lo) + 8`: the assignments'
 * values copied, the compare fused into at -- `(i = lo) + 8 < (j = hi)` materialises it) and the
 * scans and pushes are the target's.
 * OPEN, 47 of 196 instructions unmatched (0x308 of 0x310): (1) the median's spill pick -- MW spills
 * the `lo + hi` CSE (cost 19 over 54 neighbours, the cheapest node: tools/igdump.py) and keeps pl in
 * s7, where the target keeps the sum in s7 and spills pl (reloaded from sp+160 at each use; SOCOM
 * 1's kept the sum too and spilled n): the original's sum costs more -- tried the sum as a named
 * local (first, last, at the top), the median on array elements, pm alone or with ph named, the size
 * test as `j - lo > hi - i` (73), an extra use of the sum; no numbering of pl, hi and the sum gives
 * it (igreplay --want); (2) the insertion sort loads t through an outer-loop walker of its own
 * (`addiu s6,s5,4`), the target through the inner loop's &a[j] (s2): `t = a[j]` after `j = i`
 * and a pointer for a[j - 1] brought the inner loop to the target's shape (build/scratch/wld/
 * v_qs.py, vscore.py) */
template <class T>
void zqsort(T *a, int n, int (*cmp)(T const &, T const &))
{
    int stack[300];
    int *sp = stack;
    int lo = 0;
    int hi = n - 1;
    int i, j;
    T pivot;
    T t;

    for (;;) {
        while ((j = hi) > (i = lo) + 8) {
            T *pm = &a[(lo + hi) >> 1];
            T *pl = &a[lo];
            T *ph;

            if (cmp(*pl, *pm) > 0) {
                pivot = *pl;
                *pl = *pm;
                *pm = pivot;
            }
            ph = &a[hi];
            if (cmp(*pl, *ph) > 0) {
                pivot = *pl;
                *pl = *ph;
                *ph = pivot;
            }
            if (cmp(*pm, *ph) > 0) {
                pivot = *pm;
                *pm = *ph;
                *ph = pivot;
            }
            pivot = *pm;
            i++;
            j--;
            do {
                while (cmp(a[i], pivot) < 0)
                    i++;
                while (cmp(a[j], pivot) > 0)
                    j--;
                if (i <= j) {
                    t = a[i];
                    a[i] = a[j];
                    a[j] = t;
                    i++;
                    j--;
                }
            } while (i <= j);
            if (lo + hi < i + j) {
                sp[0] = lo;
                sp[1] = j;
                lo = i;
            } else {
                sp[0] = i;
                sp[1] = hi;
                hi = j;
            }
            sp += 2;
        }
        if (!(stack < sp))
            break;
        hi = *--sp;
        lo = *--sp;
    }
    for (i = 1; i < n; i++) {
        j = i;
        t = a[j];
        do {
            T *p = &a[j - 1];

            if (cmp(*p, t) <= 0)
                break;
            a[j] = *p;
        } while (--j);
        a[j] = t;
    }
}

namespace Particle {

/* whether a depth is still inside the range (ours: the target materialises the compare as a bool
 * before the loop's other test) */
static inline bool InRange(float depth, float end)
{
    return depth > end;
}

/* the next element's depth, -1 past the first buffered (ours: the select's join inside an inline
 * is left unpadded, as the target's two labels are -- the same if/else written in DrawRange pads
 * both to 8 bytes, 2 words long) */
static inline float ElementDepth(BufferElement *e)
{
    if (e == 0)
        return -1.0f;
    else
        return e->m_depth;
}

/* SOCOM 1's: the buffered elements drawn, last first, while their depth stays beyond `end`; the
 * next element's depth returned (-1 past the first). SOCOM II compares the depth itself where
 * SOCOM 1 squared it. Byte-exact since the world lane's round 14: the next depth's select through
 * ElementDepth (it was 2 words long written here: the ternary both ways round, `!= 0`, a pointer
 * local, an if-assign and setters through a reference or a pointer all pad the labels or put the
 * non-null arm first) */
float DrawRange(float depth, float end)
{
    m_lastTexture = 0;
    while (InRange(depth, end) && pParticleBuf >= bufferPtr) {
        BufferElement *e = *pParticleBuf;
        float (&m)[4][4] = thePipe.m_camera->m_mtx->m;

        switch (e->m_instance->m_source->m_drawType) {
        case PT_BILLBOARD:
            e->Draw(m);
            break;
        case PT_ROTATED:
            e->DrawRotated(m);
            break;
        case PT_STREAKED:
            e->DrawStreaked(m);
            break;
        case PT_XZ:
            e->DrawXZ(m);
            break;
        }
        depth = ElementDepth(*pParticleBuf);
        pParticleBuf--;
    }
    m_lastTexture = 0;
    return depth;
}

/* SOCOM 1's: every buffered element drawn by its source's type, last first, through the pipe
 * camera's view matrix */
void DrawFlush()
{
    m_lastTexture = 0;
    while (pParticleBuf >= bufferPtr) {
        BufferElement *e = *pParticleBuf;
        float (&m)[4][4] = thePipe.m_camera->m_mtx->m;

        switch (e->m_instance->m_source->m_drawType) {
        case PT_BILLBOARD:
            e->Draw(m);
            break;
        case PT_ROTATED:
            e->DrawRotated(m);
            break;
        case PT_STREAKED:
            e->DrawStreaked(m);
            break;
        case PT_XZ:
            e->DrawXZ(m);
            break;
        }
        pParticleBuf--;
    }
    m_lastTexture = 0;
}

/* SOCOM 1's: two elements ordered by depth, the difference's bits returned as an int (its sign
 * the float's) */
int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b)
{
    float d = a->m_depth - b->m_depth;

    return *(int *)&d;
}

/* SOCOM 1's: the source boxes drawn; with any particles buffered, a packet of ten VIF flushes
 * kicked, the elements' pointers sorted by depth and the cursor set on the last, whose depth is
 * returned (-1 with none there; 0 with nothing buffered). SOCOM II kicks the flushes where SOCOM 1
 * did not. The flushes' counter unsigned and bounded `<= 10`: the target's fused sltiu at / bnez
 * at (`< 11` computes the compare into v1, which re-colours the loop) */
float DrawPrep()
{
    Source::RenderBBox();
    if (numParticles) {
        unsigned int *p = (unsigned int *)zSysSprGetPacket(0);

        p[0] = 0x1000000A;
        p[1] = 0x19191919;
        p[2] = 0;
        p[3] = 0;
        for (unsigned int i = 1; i <= 10; i++) {
            p[i * 4] = 0x11000000;
            p[i * 4 + 1] = 0;
            p[i * 4 + 2] = 0;
            p[i * 4 + 3] = 0;
        }
        zSysFifoKick((u_long128 *)p, 11);
        for (int j = 0; j < numParticles; j++)
            bufferPtr[j] = &buffer[j];
        zqsort(bufferPtr, numParticles, bufferPtrCompare);
        pParticleBuf = &bufferPtr[numParticles - 1];
        return ElementDepth(*pParticleBuf);
    }
    return 0.0f;
}

}
