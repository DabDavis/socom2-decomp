/* zparticle_buf.cpp -- the particle draw buffer (OVL1 0x324C70-0x326F4F): the frame's particles
 * gathered as buffer elements, sorted back to front and drawn as GS packets. The unit name is
 * SOCOM 1's (its __sinit_zparticle_buf.cpp; SOCOM II's 0x402BB0 builds <iostream>'s pair and the
 * 5000 32-byte elements at 0x45C4D0 with the constructor 0x326F40). SOCOM 1's demo holds DrawRange to the element's
 * constructor in this order. Defined in reverse of their image order (-inline deferred). Written
 * so far: DrawRange, DrawFlush, the sort's comparison, DrawPrep (its zqsort instance, 0x326C30,
 * declared only), Draw and BufferElement::Draw (the world lane's round 32). */

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
    unsigned char _0[0x3b0];   // 0x0
    /* zcamera.h's m_mtxSet.mtxViewToScreen (SOCOM 1's at 0x380): the screen scale on its diagonal */
    float m_viewToScreen[4][4];   // 0x3b0
    ZCAM_VIEW *m_mtx;   // 0x3f0
    unsigned char _3f4[0x520 - 0x3f4];   // 0x3f4
    /* zcamera.h's m_vuFogAlt: the altitude fog's two vectors as the VU takes them */
    CPnt4D m_vuFogAlt[2];   // 0x520
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

/* the sorted elements, drawn from the last back to the first (SOCOM 1's); the texture the packets
 * last selected (SOCOM 1's m_lastTexture: an element's texture is compared with it) */
extern BufferElement *bufferPtr[5000];   // 0x4835F0
extern BufferElement **pParticleBuf;   // 0x4835D8
extern zdb::CTexture *m_lastTexture;   // 0x4835D0

int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b);

/* the elements buffered this frame (SOCOM 1's numParticles, 0x45C4C0) */
extern int numParticles;

}

/* the system's packet ring (zsys_main.cpp) */
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoKick(u_long128 *packet, unsigned int qwc);

/* a quadword seen every way and the scratchpad's packet (zsys_public.h's) */
union _word128 {
    u_long128 u128;
    float f32[4];
    unsigned long u64[2];
    unsigned int u32[4];
    unsigned short u16[8];
    unsigned char u8[16];
};
class CSPR {
public:
    static _word128 *Kick(_word128 *end, bool getNew);

    static _word128 *m_sprPacket;   // 0x49E330
    static _word128 *m_sprPacketEnd;   // 0x49E338
};

/* Sony's libvu0 (MAIN): a point through a matrix to the screen, and a colour to integers */
typedef int sceVu0IVECTOR[4] __attribute__((aligned(16)));
typedef float sceVu0FVECTOR[4] __attribute__((aligned(16)));
extern "C" void sceVu0RotTransPers(sceVu0IVECTOR v0, float m0[4][4], sceVu0FVECTOR v1, int mode);
extern "C" void sceVu0FTOI0Vector(int *v0, float *v1);

/* vis_vu.cpp's: a light tint by value (CVisual::m_lightTint[i]), and a colour through it (the
 * placeholders' rows, not named) */
extern "C" CPnt4D func_003B7590(int i);
extern "C" void func_003B59A0(_word128 *in, _word128 *m, _word128 *out);

/* the texture's DMA reference (znode.h's CTexture::m_dmaRef, a quadword at 0x30; zparticle.h's
 * stand-in stops at the palette) */
struct CTexDmaRef {
    unsigned char _0[0x30];   // 0x0
    u_long128 m_dmaRef;   // 0x30
};

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

/* SOCOM 1's: one billboard -- its instance's point through the view to the screen (z kept to 16
 * bits and nothing nearer than 10), the sprite's half-size the scale over the depth through the
 * screen matrix's diagonal, nothing drawn that leaves the 4096-pixel square; the colour through
 * the source's light tint, the altitude fog worked out on VU0 (its microprogram at 0, the fog read
 * back from vf14 -- new in SOCOM II); then a two-vertex sprite with or without its texture (the
 * texture's DMA reference first when it differs from the last), the packet's DMA and VIF tags and
 * the cursor kicked. Byte-exact (the world lane's round 32, unwritten before; build/scratch/wld/
 * v_bed*.py): the VU0 blocks PLAIN `asm { }` -- `asm volatile` turns the IR optimiser off for the
 * whole function (0x3D4: the int-to-float loads through GPRs; plain asm restored the size, 75 off
 * on registers); the tint initialised from its call (assigned, its by-value result is copied
 * through four FPRs from a fifth stack slot); `tag` declared at the top (numbered ahead of the
 * half-sizes: s4, then w s3, h s2, the cursor s1); the four range tests one `if` each (an `||`
 * materialises the compares); out.w read before the fog vectors' address (the target's load
 * order); the scale read into a named local, numbered ahead of the 256.0f constant (f1 / f0:
 * igreplay --want 38=f0,39=f1 --movers 38,39; a `(float)` cast on m_scale does the same) */
void BufferElement::Draw(float (&m)[4][4])
{
    sceVu0FVECTOR in;
    sceVu0IVECTOR out;
    CPnt4D c;
    _word128 *tag;

    m_spr = CSPR::m_sprPacket = (_word128 *)zSysSprGetPacket((u_long128 **)&CSPR::m_sprPacketEnd);
    in[0] = m_instance->m_pos[0];
    in[1] = m_instance->m_pos[1];
    in[2] = m_instance->m_pos[2];
    in[3] = 1.0f;
    sceVu0RotTransPers(out, m, in, 0);
    out[2] >>= 4;
    if (out[2] < 10)
        return;
    float scale = m_scale;
    float s = 256.0f * scale / (float)out[3];
    CPipeCamera *cam = thePipe.m_camera;
    int w = (int)(0.5f + s * cam->m_viewToScreen[0][0]);
    int h = (int)(0.5f + s * cam->m_viewToScreen[1][1]);

    if (out[0] - w < 0)
        return;
    if (out[0] + w >= 0x10000)
        return;
    if (out[1] - h < 0)
        return;
    if (out[1] + h >= 0x10000)
        return;
    CPnt4D tint = func_003B7590(m_instance->m_source->m_tint);

    func_003B59A0((_word128 *)this, (_word128 *)&tint, (_word128 *)&c);
    float *pin = in;

    in[3] = *(float *)&out[3];
    CPnt4D *fogAlt = thePipe.m_camera->m_vuFogAlt;
    asm {
        lqc2 vf20, 0(pin)
        lqc2 vf17, 0(fogAlt)
        lqc2 vf18, 16(fogAlt)
        vcallms 0x0
        vnop
    }
    _word128 *p = m_spr;
    int fog;

    if (m_texture) {
        if (m_texture != m_lastTexture) {
            m_lastTexture = m_texture;
            p->u128 = ((CTexDmaRef *)m_texture)->m_dmaRef;
            p++;
        }
        tag = p++;
        p->u64[0] = 0x307B400000008002;
        p->u64[1] = 0x412;
        p++;
        float q = 1.0f / (float)out[2];

        asm {
            qmfc2.i fog, vf14
        }
        p->u128 = *(const u_long128 *)&CPnt4D::zero;
        p++;
        sceVu0FTOI0Vector((int *)p, (float *)&c);
        p++;
        p->u32[0] = out[0] - w;
        p->u32[1] = out[1] - h;
        p->u32[2] = out[2] << 4;
        p->u32[3] = fog;
        p++;
        p->f32[2] = q;
        p->f32[1] = q;
        p->f32[0] = q;
        p->u32[3] = 0;
        p++;
        sceVu0FTOI0Vector((int *)p, (float *)&c);
        p++;
        p->u32[0] = out[0] + w;
        p->u32[1] = out[1] + h;
        p->u32[2] = out[2] << 4;
        p->u32[3] = fog;
        p++;
    } else {
        tag = p++;
        p->u64[0] = 0x2073400000008002;
        p->u64[1] = 0x51;
        p++;
        asm {
            qmfc2.i fog, vf14
        }
        sceVu0FTOI0Vector((int *)p, (float *)&c);
        p++;
        p->u32[0] = out[0] - w;
        p->u32[1] = out[1] - h;
        p->u32[2] = out[2] << 4;
        p->u32[3] = fog;
        p++;
        sceVu0FTOI0Vector((int *)p, (float *)&c);
        p++;
        p->u32[0] = out[0] + w;
        p->u32[1] = out[1] + h;
        p->u32[2] = out[2] << 4;
        p->u32[3] = 0;
        p++;
    }
    int qwc = p - tag - 1;

    tag->u32[0] = qwc | 0x10000000;
    tag->u32[1] = 0;
    tag->u32[2] = 0x11000000;
    tag->u32[3] = qwc | 0x50000000;
    m_spr = p;
    CSPR::Kick(m_spr, false);
}

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

/* SOCOM 1's: the frame's particles drawn whole (CPipe::CAlpha::Render's call when its draw list is
 * empty): DrawPrep's body without its depth, then DrawFlush's -- written out, as the target holds
 * both (called, neither inlines: 0x24). Byte-exact (the world lane round 23) */
void Draw()
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
    }
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

}
