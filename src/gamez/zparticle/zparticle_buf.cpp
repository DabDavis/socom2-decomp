/* zparticle_buf.cpp -- the particle draw buffer (OVL1 0x324C70-0x326F4F): the frame's particles
 * gathered as buffer elements, sorted back to front and drawn as GS packets. The unit name is
 * SOCOM 1's (its __sinit_zparticle_buf.cpp; SOCOM II's 0x402BB0 builds <iostream>'s pair and the
 * 5000 32-byte elements at 0x45C4D0 with the constructor 0x326F40). SOCOM 1's demo holds DrawRange to the element's
 * constructor in this order. Defined in reverse of their image order (-inline deferred). Written
 * so far: everything (the world lane's rounds 32 and 33; the zqsort instance round 34): the unit is whole.
 * VU0 ASM: a `.set` directive in a block splits the scheduling block there. */

#include <eetypes.h>
#include <iostream>
#include "zparticle.h"

extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float sqrtf(float);

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
 * Byte-exact since round 34 (0x308 of 0x310 since round 20): the median's spill pick -- MW kept pl in s7 and spilled the
 * `lo + hi` CSE (19 over 54 neighbours), the target keeps the sum and spills pl -- was never the numbering: the swap's
 * test is `if (i > j) break;` in the do-while, not `if (i <= j)`. That break makes the block reading the sum a loop exit,
 * and its weight takes the sum's cost from 19 to 131 (`tools/igdump.py`'s costs), past pl's 83 / 37, with no other change.
 * Each arm of the size test steps `sp` itself. The insertion sort's `t = a[j = i]`: `j = i; t = a[j];` is copy-propagated
 * to a[i] ahead of the loop phase, so it took an outer-loop walker of its own, where this read is the inner loop's &a[j] */
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
                if (i > j)
                    break;
                t = a[i];
                a[i] = a[j];
                a[j] = t;
                i++;
                j--;
            } while (i <= j);
            if (lo + hi < i + j) {
                sp[0] = lo;
                sp[1] = j;
                lo = i;
                sp += 2;
            } else {
                sp[0] = i;
                sp[1] = hi;
                hi = j;
                sp += 2;
            }
        }
        if (!(stack < sp))
            break;
        hi = *--sp;
        lo = *--sp;
    }
    for (i = 1; i < n; i++) {
        t = a[j = i];
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

/* whether a point the screen transform gave lies outside what is drawn: nearer than 1000 or
 * within 16000 of the 65536-wide coordinate range's edges (ours, inline: the target materialises
 * the answer, 1 in each early arm's delay slot, before its branch) */
static inline bool OffScreen(const int *v)
{
    if (v[2] < 1000)
        return true;
    if (v[0] < 16000)
        return true;
    if (v[0] > 0xC180)
        return true;
    if (v[1] < 16000)
        return true;
    if (v[1] > 0xC180)
        return true;
    return false;
}

/* SOCOM II's: one particle lying in the ground plane -- its centre through the view to the screen
 * (z kept to 16 bits, nothing drawn nearer than 1000 or near the range's edges), the altitude fog
 * on VU0 as Draw's; its four corners, the scale either way along x and z, each through the view
 * and the same test; the fog into each corner's w; two textured triangles (0, 2, 3) and (0, 1, 2)
 * at the tinted colour -- the fourth vertex at the element's own colour, the original's -- the
/* SOCOM 1's, its tests moved in from the screen's edges: one particle drawn as a streak between its
 * position and its last -- each end through the view to the screen (z kept to 16 bits, nothing
 * drawn nearer than 1000), nothing when both are one point; the direction between them on the screen
 * made unit (straight up when too short), the head pushed out by the size and the tail back by the
 * size times the source's streak length, the half-width the perpendicular's; nothing drawn outside
 * the range on either axis; z halfway between the ends, the altitude fog on VU0 as Draw's; two
 * textured triangles with the tail's colour faded by the source's m_streakFade. Byte-exact (the world
 * lane's round 33, unwritten before; build/scratch/wld/v_st*.py), each by the replay (igreplay --move
 * before any source): `streak * size` WRITTEN TWICE -- a back-end temporary numbered where it first
 * occurs, between the head's temporaries and the tail's (f3; a named local or `streak *= size` is
 * numbered among the locals, 22 off); the sqrt in a statement of its own, one neighbour less for size
 * (a named product at degree 32 makes size deferred, coloured first in f0, 73 off); the camera through
 * a one-line accessor (numbered ahead of the z reads: --move 130=122.5 gave all 112 GPRs); `source`
 * read into a local first (t0), the instance into a local for the equality test (the first
 * declared takes the highest register there); the fade before q */
static inline CPipeCamera *Cam() { return thePipe.m_camera; }

void BufferElement::DrawStreaked(float (&m)[4][4])
{
    sceVu0FVECTOR in;
    sceVu0IVECTOR tail;
    sceVu0IVECTOR head;
    CPnt4D c;
    _word128 *tag;

    m_spr = CSPR::m_sprPacket = (_word128 *)zSysSprGetPacket((u_long128 **)&CSPR::m_sprPacketEnd);
    Instance *inst = m_instance;
    float *last = inst->m_10;
    float *pos = inst->m_pos;

    if (pos[0] == last[0] && pos[1] == last[1] && pos[2] == last[2])
        return;
    in[0] = m_instance->m_pos[0];
    in[1] = m_instance->m_pos[1];
    in[2] = m_instance->m_pos[2];
    in[3] = 1.0f;
    sceVu0RotTransPers(head, m, in, 0);
    head[2] >>= 4;
    if (head[2] < 1000)
        return;
    in[0] = m_instance->m_10[0];
    in[1] = m_instance->m_10[1];
    in[2] = m_instance->m_10[2];
    in[3] = 1.0f;
    sceVu0RotTransPers(tail, m, in, 0);
    tail[2] >>= 4;
    if (tail[2] < 1000)
        return;
    Source *src = m_instance->m_source;
    float scale = m_scale;
    float size = 256.0f * scale / (float)head[3];

    size *= thePipe.m_camera->m_viewToScreen[0][0];
    float dx = (float)(head[0] - tail[0]);
    float dy = (float)(head[1] - tail[1]);
    float streak = src->m_streakLen;
    float len = dx * dx + dy * dy;

    if (len < 0.00001f)
        dy = -1.0f;
    else {
        len = sqrtf(len);
        len = 1.0f / len;
        dx *= len;
        dy *= len;
    }
    head[0] = (int)((float)head[0] + dx * size);
    head[1] = (int)((float)head[1] + dy * size);
    tail[0] = (int)((float)tail[0] - dx * (streak * size));
    tail[1] = (int)((float)tail[1] - dy * (streak * size));
    int ex = (int)(dy * size);
    int ey = (int)(-size * dx);

    if (tail[0] < head[0]) {
        if (tail[0] - ex <= 16000)
            return;
        if (head[0] + ex >= 0xC180)
            return;
    } else {
        if (head[0] - ex <= 16000)
            return;
        if (tail[0] + ex >= 0xC180)
            return;
    }
    if (tail[1] < head[1]) {
        if (tail[1] - ey <= 16000)
            return;
        if (head[1] + ey >= 0xC180)
            return;
    } else {
        if (head[1] - ey <= 16000)
            return;
        if (tail[1] + ey >= 0xC180)
            return;
    }
    int z = (int)((float)tail[2] + 0.5f * (float)(head[2] - tail[2]));
    float *pin = in;

    in[3] = *(float *)&head[3];
    CPnt4D *fogAlt = Cam()->m_vuFogAlt;

#if Z_PORT
    /* the point (its w the projection's word) into vf20, the camera's altitude-fog pair into
     * vf17/vf18, VU0's micro program at 0 started: the fog comes back in vf14 */
    z_vu0_lqc2(20, pin);
    z_vu0_lqc2(17, &fogAlt[0]);
    z_vu0_lqc2(18, &fogAlt[1]);
    z_vu0_callms(0x0);
#else
    asm {
        .set noreorder
        lqc2 vf20, 0(pin)
        lqc2 vf17, 0(fogAlt)
        lqc2 vf18, 16(fogAlt)
        vcallms 0x0
        vnop
        .set reorder
    }
#endif
    _word128 *p = m_spr;

    if (m_texture != m_lastTexture) {
        m_lastTexture = m_texture;
        p->u128 = ((CTexDmaRef *)m_texture)->m_dmaRef;
        p++;
    }
    tag = p++;

    p->u64[0] = 0x3039C00000008006;
    p->u64[1] = 0x412;
    p++;
    int fog;

#if Z_PORT
    fog = ((int *)z_vu0.vf[14])[0];   /* qmfc2.i: waits for the program; the low word, vf14.x */
#else
    asm {
        .set noreorder
        qmfc2.i fog, vf14
        .set reorder
    }
#endif
    CPnt4D tint = func_003B7590(m_instance->m_source->m_tint);

    func_003B59A0((_word128 *)this, (_word128 *)&tint, (_word128 *)&c);
    CPnt4D c2 = c;

    c2.w *= m_instance->m_source->m_streakFade;
    float q = 1.0f / (float)z;

    z <<= 4;
    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c2);
    p++;
    p->u32[0] = tail[0] - ex;
    p->u32[1] = tail[1] - ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u32[0] = head[0] + ex;
    p->u32[1] = head[1] + ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    p->f32[0] = q;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c2);
    p++;
    p->u32[0] = tail[0] + ex;
    p->u32[1] = tail[1] + ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c2);
    p++;
    p->u32[0] = tail[0] - ex;
    p->u32[1] = tail[1] - ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    p->u32[0] = 0;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u32[0] = head[0] - ex;
    p->u32[1] = head[1] - ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u32[0] = head[0] + ex;
    p->u32[1] = head[1] + ey;
    p->u32[2] = z;
    p->u32[3] = fog;
    p++;
    m_spr = p;
    int qwc = m_spr - tag - 1;

    tag->u32[0] = qwc | 0x10000000;
    tag->u32[1] = 0;
    tag->u32[2] = 0x11000000;
    tag->u32[3] = qwc | 0x50000000;
    CSPR::Kick(m_spr, false);
}

/* the element's instance and scale (ours, inline accessors: a corner's coordinates read through
 * them -- see DrawXZ) */
static inline Instance *InstOf(const BufferElement *e) { return e->m_instance; }
static inline float ScaleOf(const BufferElement *e) { return e->m_scale; }

/* SOCOM II's: one particle lying in the ground plane -- its centre through the view to the screen
 * (z kept to 16 bits, nothing drawn nearer than 1000 or near the range's edges), the altitude fog
 * on VU0 as Draw's; its four corners, the scale either way along x and z, each through the view
 * and the same test; the fog into each corner's w; two textured triangles (0, 2, 3) and (0, 1, 2)
 * at the tinted colour -- the fourth vertex at the element's own colour, the original's -- the
 * texture's DMA reference first when it differs from the last, the tags, the cursor kicked
 * (SOCOM 1's DrawXZ built its corners on the screen). Unwritten before the world lane's round 32
 * (build/scratch/wld/v_xz*.py): the corners' coordinates through inline accessors for the instance
 * and the scale -- an inline's temporaries are made ahead of the linearizer's, so the instance
 * loads number before the 1.0f constant (v1 / v0, igreplay --movers: the constant after them) and
 * the scale before the position (f1 / f0); written plainly both pairs swap (47 off).
 * Byte-exact (the world lane's round 33; OPEN at 2 instructions since round 32): the fog's read is
 * bracketed by `.set noreorder` / `.set reorder` -- A `.set` DIRECTIVE SPLITS THE SCHEDULING BLOCK,
 * so the qmfc2.i takes its own cycle and the lui and the first store pair in the next, then the
 * cursor's load (`lui at; sw v0,172(sp); lw s0,m_spr; sw ...`). Without it the lui pairs with the
 * qmfc2.i and the load, ready first, goes ahead of all four stores; the C form, an inline, a
 * chained store, a label, a do / while(0), an else arm and GCC-style asm all keep the one block
 * (build/scratch/wld/v_xz8-11.py; DrawRotated has the same tail) */
void BufferElement::DrawXZ(float (&m)[4][4])
{
    sceVu0FVECTOR in;
    sceVu0FVECTOR corner[4];
    sceVu0IVECTOR out;
    sceVu0IVECTOR p0;
    sceVu0IVECTOR p1;
    sceVu0IVECTOR p2;
    sceVu0IVECTOR p3;
    CPnt4D c;
    _word128 *tag;

    m_spr = CSPR::m_sprPacket = (_word128 *)zSysSprGetPacket((u_long128 **)&CSPR::m_sprPacketEnd);
    in[0] = m_instance->m_pos[0];
    in[1] = m_instance->m_pos[1];
    in[2] = m_instance->m_pos[2];
    in[3] = 1.0f;
    sceVu0RotTransPers(out, m, in, 0);
    out[2] >>= 4;
    if (out[2] < 1000)
        return;
    if (out[0] < 16000)
        return;
    if (out[0] > 0xC180)
        return;
    if (out[1] < 16000)
        return;
    if (out[1] > 0xC180)
        return;
    float *pin = in;

    in[3] = *(float *)&out[3];
    CPnt4D *fogAlt = thePipe.m_camera->m_vuFogAlt;

#if Z_PORT
    /* the point (its w the projection's word) into vf20, the camera's altitude-fog pair into
     * vf17/vf18, VU0's micro program at 0 started: the fog comes back in vf14 */
    z_vu0_lqc2(20, pin);
    z_vu0_lqc2(17, &fogAlt[0]);
    z_vu0_lqc2(18, &fogAlt[1]);
    z_vu0_callms(0x0);
#else
    asm {
        lqc2 vf20, 0(pin)
        lqc2 vf17, 0(fogAlt)
        lqc2 vf18, 16(fogAlt)
        vcallms 0x0
        vnop
    }
#endif
    corner[0][0] = InstOf(this)->m_pos[0] - ScaleOf(this);
    corner[0][1] = InstOf(this)->m_pos[1];
    corner[0][2] = InstOf(this)->m_pos[2] - ScaleOf(this);
    corner[0][3] = 1.0f;
    corner[1][0] = InstOf(this)->m_pos[0] + ScaleOf(this);
    corner[1][1] = InstOf(this)->m_pos[1];
    corner[1][2] = InstOf(this)->m_pos[2] - ScaleOf(this);
    corner[1][3] = 1.0f;
    corner[2][0] = InstOf(this)->m_pos[0] + ScaleOf(this);
    corner[2][1] = InstOf(this)->m_pos[1];
    corner[2][2] = InstOf(this)->m_pos[2] + ScaleOf(this);
    corner[2][3] = 1.0f;
    corner[3][0] = InstOf(this)->m_pos[0] - ScaleOf(this);
    corner[3][1] = InstOf(this)->m_pos[1];
    corner[3][2] = InstOf(this)->m_pos[2] + ScaleOf(this);
    corner[3][3] = 1.0f;
    sceVu0RotTransPers(p0, m, corner[0], 0);
    if (OffScreen(p0))
        return;
    sceVu0RotTransPers(p1, m, corner[1], 0);
    if (OffScreen(p1))
        return;
    sceVu0RotTransPers(p2, m, corner[2], 0);
    if (OffScreen(p2))
        return;
    sceVu0RotTransPers(p3, m, corner[3], 0);
    if (OffScreen(p3))
        return;
    int fog;

#if Z_PORT
    fog = ((int *)z_vu0.vf[14])[0];   /* qmfc2.i: waits for the program; the low word, vf14.x */
#else
    asm {
        .set noreorder
        qmfc2.i fog, vf14
        .set reorder
    }
#endif
    p0[3] = fog;
    p1[3] = fog;
    p2[3] = fog;
    p3[3] = fog;
    _word128 *p = m_spr;

    if (m_texture != m_lastTexture) {
        m_lastTexture = m_texture;
        p->u128 = ((CTexDmaRef *)m_texture)->m_dmaRef;
        p++;
    }
    tag = p++;

    p->u64[0] = 0x3039C00000008006;
    p->u64[1] = 0x412;
    p++;
    CPnt4D tint = func_003B7590(m_instance->m_source->m_tint);

    func_003B59A0((_word128 *)this, (_word128 *)&tint, (_word128 *)&c);
    float q = 1.0f / (float)out[2];

    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u128 = *(u_long128 *)p0;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u128 = *(u_long128 *)p2;
    p++;
    p->f32[0] = q;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u128 = *(u_long128 *)p3;
    p++;
    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)this);
    p++;
    p->u128 = *(u_long128 *)p0;
    p++;
    p->u32[0] = 0;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u128 = *(u_long128 *)p1;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    p++;
    p->u128 = *(u_long128 *)p2;
    p++;
    m_spr = p;
    int qwc = m_spr - tag - 1;

    tag->u32[0] = qwc | 0x10000000;
    tag->u32[1] = 0;
    tag->u32[2] = 0x11000000;
    tag->u32[3] = qwc | 0x50000000;
    CSPR::Kick(m_spr, false);
}

/* SOCOM 1's, its corners' tests SOCOM II's: one spinning particle -- its centre through the view to
 * the screen (z kept to 16 bits, nothing drawn nearer than 10), the half-size the scale over the
 * depth through the screen matrix's diagonal, turned by the instance's angle into its two screen
 * offsets; the altitude fog on VU0 as Draw's; the four corners the centre plus the offsets a
 * quarter-turn apart, each tested as DrawXZ's; the fog into each corner's w; two textured triangles
 * (0, 2, 3) and (0, 1, 2), the colour converted once and copied for the others, the tags, the cursor
 * kicked. The larger offset, SOCOM 1's range test's margin, is still worked out and unused: the
 * target keeps its branches with nothing in them (the tests are now each corner's) */
void BufferElement::DrawRotated(float (&m)[4][4])
{
    sceVu0FVECTOR in;
    sceVu0IVECTOR out;
    sceVu0IVECTOR p0;
    sceVu0IVECTOR p1;
    sceVu0IVECTOR p2;
    sceVu0IVECTOR p3;
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
    float aspect = 1.0f;
    CPipeCamera *cam = thePipe.m_camera;
    float angle = m_instance->m_10[0];
    float r = s * cam->m_viewToScreen[0][0];
    float cs = cosf(angle);
    float sn = sinf(angle);
    int dx = (int)(r * cs);
    int dy = (int)(r * sn);
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    int margin = ady < adx ? adx : ady;
    float *pin = in;

    in[3] = *(float *)&out[3];
    CPnt4D *fogAlt = thePipe.m_camera->m_vuFogAlt;

#if Z_PORT
    /* the point (its w the projection's word) into vf20, the camera's altitude-fog pair into
     * vf17/vf18, VU0's micro program at 0 started: the fog comes back in vf14 */
    z_vu0_lqc2(20, pin);
    z_vu0_lqc2(17, &fogAlt[0]);
    z_vu0_lqc2(18, &fogAlt[1]);
    z_vu0_callms(0x0);
#else
    asm {
        .set noreorder
        lqc2 vf20, 0(pin)
        lqc2 vf17, 0(fogAlt)
        lqc2 vf18, 16(fogAlt)
        vcallms 0x0
        vnop
        .set reorder
    }
#endif
    p0[0] = (int)((float)out[0] + (float)dx * aspect);
    p0[1] = (int)((float)out[1] + (float)dy * aspect);
    p0[2] = out[2] << 4;
    if (OffScreen(p0))
        return;
    p1[0] = (int)((float)out[0] - (float)dy * aspect);
    p1[1] = (int)((float)out[1] + (float)dx * aspect);
    p1[2] = out[2] << 4;
    if (OffScreen(p1))
        return;
    p2[0] = (int)((float)out[0] - (float)dx * aspect);
    p2[1] = (int)((float)out[1] - (float)dy * aspect);
    p2[2] = out[2] << 4;
    if (OffScreen(p2))
        return;
    p3[0] = (int)((float)out[0] + (float)dy * aspect);
    p3[1] = (int)((float)out[1] - (float)dx * aspect);
    p3[2] = out[2] << 4;
    if (OffScreen(p3))
        return;
    int fog;

#if Z_PORT
    fog = ((int *)z_vu0.vf[14])[0];   /* qmfc2.i: waits for the program; the low word, vf14.x */
#else
    asm {
        .set noreorder
        qmfc2.i fog, vf14
        .set reorder
    }
#endif
    p1[3] = fog;
    p0[3] = fog;
    p2[3] = fog;
    p3[3] = fog;
    _word128 *p = m_spr;

    if (m_texture != m_lastTexture) {
        m_lastTexture = m_texture;
        p->u128 = ((CTexDmaRef *)m_texture)->m_dmaRef;
        p++;
    }
    tag = p++;

    p->u64[0] = 0x3039C00000008006;
    p->u64[1] = 0x412;
    p++;
    CPnt4D tint = func_003B7590(m_instance->m_source->m_tint);

    func_003B59A0((_word128 *)this, (_word128 *)&tint, (_word128 *)&c);
    float q = 1.0f / (float)out[2];

    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    sceVu0FTOI0Vector((int *)p, (float *)&c);
    _word128 *col = p;

    p++;
    p->u128 = *(u_long128 *)p0;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    *p++ = *col;
    p->u128 = *(u_long128 *)p2;
    p++;
    p->f32[0] = q;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    *p++ = *col;
    p->u128 = *(u_long128 *)p3;
    p++;
    p->u32[0] = 0;
    p->u32[1] = 0;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    *p++ = *col;
    p->u128 = *(u_long128 *)p0;
    p++;
    p->u32[0] = 0;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    *p++ = *col;
    p->u128 = *(u_long128 *)p1;
    p++;
    p->f32[0] = q;
    p->f32[1] = q;
    p->f32[2] = q;
    p->u32[3] = 0;
    p++;
    *p++ = *col;
    p->u128 = *(u_long128 *)p2;
    p++;
    m_spr = p;
    int qwc = m_spr - tag - 1;

    tag->u32[0] = qwc | 0x10000000;
    tag->u32[1] = 0;
    tag->u32[2] = 0x11000000;
    tag->u32[3] = qwc | 0x50000000;
    CSPR::Kick(m_spr, false);
}

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
#if Z_PORT
    /* the point (its w the projection's word) into vf20, the camera's altitude-fog pair into
     * vf17/vf18, VU0's micro program at 0 started: the fog comes back in vf14 */
    z_vu0_lqc2(20, pin);
    z_vu0_lqc2(17, &fogAlt[0]);
    z_vu0_lqc2(18, &fogAlt[1]);
    z_vu0_callms(0x0);
#else
    asm {
        lqc2 vf20, 0(pin)
        lqc2 vf17, 0(fogAlt)
        lqc2 vf18, 16(fogAlt)
        vcallms 0x0
        vnop
    }
#endif
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

#if Z_PORT
        fog = ((int *)z_vu0.vf[14])[0];   /* qmfc2.i: waits for the program; the low word, vf14.x */
#else
        asm {
            qmfc2.i fog, vf14
        }
#endif
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
#if Z_PORT
        fog = ((int *)z_vu0.vf[14])[0];   /* qmfc2.i: waits for the program; the low word, vf14.x */
#else
        asm {
            qmfc2.i fog, vf14
        }
#endif
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
