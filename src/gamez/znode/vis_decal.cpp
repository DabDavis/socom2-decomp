/* vis_decal.cpp -- the decals: zdb::CDecal and CMeshDecal, their pools, and the RTArray instances
 * that hold them (SOCOM 1's vis_decal.cpp, by its static initialiser, which builds the four
 * pools): OVL1 0x3BD9A0?-0x3BFB5F (its tail ends with RTArray<CMeshDecal>()'s pieces), as SOCOM
 * 1's 0x312DE0-0x313BBF. Only some of its functions so
 * far; the rest stay generated drafts or placeholders. Defined in reverse of their image order
 * (-inline deferred). */

#include "znode.h"
#include "../zbody/zbody.h"
#include "../zsys/zsys_public.h"

namespace zdb {

class CDecal;
class CMeshDecal;

/* a decal's add-on point (vis_vu.cpp's AddonPoint, its layout: the position, two halfwords, two
 * points with a word after each -- here the words are what AddDecal reads them as, the colour and
 * the normal's halfwords) */
struct AddonPoint {
    CPnt3D m_pos;   // 0x0
    short m_c;   // 0xc
    short m_e;   // 0xe
    CPnt3D m_10;   // 0x10
    unsigned int *m_rgba;   // 0x1c
    CPnt3D m_20;   // 0x20
    short *m_normal;   // 0x2c
};   // 0x30 bytes

/* a decal's pool: the decals in use, the most, the extra allowed past it and how often the most
 * was passed, over an RTArray of them (SOCOM 1's; 0x14 bytes, the statics m_tempPool and
 * m_permPool; the member names ours) */
class CDecalPool {
public:
    /* empty: no array, nothing counted (inline: this unit's initialiser zeroes the four pools'
     * array, count and overflow, in that order) */
    CDecalPool()
    {
        m_pool = 0;
        m_count = 0;
        m_overflow = 0;
    }
    /* the most and the extra kept, the pool's array made for both, recycling (SOCOM 1's) */
    void Init(unsigned int count, unsigned int extra);
    CDecal *Get(bool b);
    /* the overflow worked off: the oldest temporary decals taken back and off their visuals
     * (SOCOM 1's) */
    void RecycleTick();

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CDecal> *m_pool;   // 0x10
};

/* the same over the mesh decals (SOCOM 1's) */
class CMeshDecalPool {
public:
    CMeshDecalPool()
    {
        m_pool = 0;
        m_count = 0;
        m_overflow = 0;
    }
    void Init(unsigned int count, unsigned int extra);
    /* the array deleted and the counts zeroed (SOCOM 1's) */
    void Uninit();
    CMeshDecal *Get(bool b);
    /* the same over the mesh decals, each freed besides (SOCOM 1's) */
    void RecycleTick();

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CMeshDecal> *m_pool;   // 0x10
};

/* a decal on a visual (SOCOM 1's class: no DWARF; only its pools here) */
class CVisual;
class CMesh;

class CDecal {
public:
    /* empty (the pool's array builds and destroys its items with their out-of-line copies,
     * 0x3BF6E0 and 0x3BF690) */
    CDecal() {}
    ~CDecal() {}
    static void PutPermDecal(CDecal *d);
    static void PutTempDecal(CDecal *d);

    CDecal *AddDecal(float size, CPnt3D *pos, AddonPoint **points, unsigned int flags, CTexHandle *tex,
                     CVisual *visual, unsigned int vid);

    CDecal *m_next;   // 0x0
    CVisual *m_visual;   // 0x4
    float m_size;   // 0x8
    unsigned short m_view;   // 0xc
    unsigned short m_kind;   // 0xe: 2 permanent (a negative size), 1 temporary
    u_long128 m_tex;   // 0x10: the texture's VU select reference
    unsigned int m_pkt[0x110 / 4 - 8];   // 0x20: the DMA reference and the VIF packet it sends
    CPnt3D m_pos;   // 0x110 (SOCOM II's)
    unsigned char _11c[0x120 - 0x11c];   // (0x120 bytes: RTArray<CDecal>()'s array stride)

    static CDecalPool m_tempPool;
    static CDecalPool m_permPool;
};

/* a decal on a mesh: its own vertex block (SOCOM 1's class: no DWARF; the names ours) */
class CMeshDecal {
public:
    /* the links and the vertex block cleared (the pool's array builds its items with it, 0x3BFB40;
     * the destructor empty, 0x3BF9B0) */
    CMeshDecal()
    {
        m_life = 0;
        m_next = 0;
        m_mesh = 0;
        m_verts = 0;
        m_34 = 0;
        m_30 = 0;
    }
    ~CMeshDecal() {}
    void FreeMem();
    static void PutTempDecal(CMeshDecal *d);
    static void PutPermDecal(CMeshDecal *d);

    float m_life;   // 0x0
    CMeshDecal *m_next;   // 0x4
    CMesh *m_mesh;   // 0x8
    unsigned char _c[0x10 - 0xc];
    void *m_verts;   // 0x10: allocated, through the uncached mirror
    unsigned char _14[0x30 - 0x14];
    unsigned int m_30;   // 0x30
    unsigned int m_34;   // 0x34
    unsigned char _38[0x40 - 0x38];   // (0x40 bytes: RTArray<CMeshDecal>()'s array stride)

    static CMeshDecalPool m_tempPool;
    static CMeshDecalPool m_permPool;
};

}

using namespace zdb;

/* a cached pointer from its uncached or accelerated mirror */
#define CACHED(p) ((void *)((unsigned int)(p) & 0x0FFFFFFF))

namespace zdb {

/* the four pools (SOCOM 1's statics) and the square a decal's UVs must reach into, -1.2 to 1.2
 * (new in SOCOM II, where SOCOM 1 tested 0 to 1: 1 and -1 widened by uvSlop, the float at 0x3E14C8)
 * -- defined in the order this unit's initialiser (0x404A10) builds them; the names ours */
CDecalPool CDecal::m_tempPool;
CDecalPool CDecal::m_permPool;
CMeshDecalPool CMeshDecal::m_tempPool;
CMeshDecalPool CMeshDecal::m_permPool;
float uvSlop = 0.2f;
float uvMax = 1.0f + uvSlop;
float uvMin = -1.0f - uvSlop;

/* whether a triangle's UVs, by their bounding box, reach into the square (SOCOM 1's UVCompare,
 * after CDecalPool::Init in both units; the VU0 decal builder 0x3B8680 and 0x3B3AB0 call it) */
bool UVCompare(CPnt3D *a, CPnt3D *b, CPnt3D *c)
{
    float m;

    m = a->x;
    m = m < b->x ? m : b->x;
    m = m < c->x ? m : c->x;
    if (m > uvMax)
        return false;
    m = a->x > b->x ? a->x : b->x;
    m = m > c->x ? m : c->x;
    if (m < uvMin)
        return false;
    m = a->y;
    m = m < b->y ? m : b->y;
    m = m < c->y ? m : c->y;
    if (m > uvMax)
        return false;
    m = a->y > b->y ? a->y : b->y;
    m = m > c->y ? m : c->y;
    if (m < uvMin)
        return false;
    return true;
}

}

void CDecalPool::Init(unsigned int count, unsigned int extra)
{
    m_max = count;
    m_extra = extra;
    m_pool = new RTArray<CDecal>(count + extra, true, true);
}

/* a temporary decal given back: the overflow first, then the count (SOCOM 1's) */
void CDecal::PutTempDecal(CDecal *d)
{
    if (m_tempPool.m_overflow)
        m_tempPool.m_overflow--;
    m_tempPool.m_count--;
    m_tempPool.m_pool->PutBack(d);
}

void CDecal::PutPermDecal(CDecal *d)
{
    m_permPool.m_count--;
    m_permPool.m_pool->PutBack(d);
}

/* the overflow of temporary decals worked off: each oldest one taken back and off its visual
 * (SOCOM 1's) */
void CDecalPool::RecycleTick()
{
    while (m_overflow) {
        CDecal *d = CDecal::m_tempPool.m_pool->GetOldestUsed();

        CDecal::m_tempPool.m_count--;
        CDecal::m_tempPool.m_pool->PutBack(d);
        d->m_visual->RemoveDecal(d);
        m_overflow--;
    }
}

/* a decal handed out while the pool has room, counted, the overflow counted past the most (SOCOM
 * 1's passed its flag on to GetNext; SOCOM II's always asks for a recycled one) */
CDecal *CDecalPool::Get(bool b)
{
    CDecal *d = 0;

    if (m_count < m_max + m_extra) {
        d = m_pool->GetNext(true);
        if (d) {
            m_count++;
            if (m_count > m_max)
                m_overflow++;
        }
    }
    return d;
}

/* a decal set up on a visual's view: its size, kind, visual and view; the texture's VU select; the
 * DMA reference to its VIF packet -- the three points' colours, the fan's and the strip's GIF tags
 * (textured, gouraud, fogged triangles, alpha-blended for a translucent texture that is not a bump
 * map), each point's two quadwords and the last point's normal (SOCOM 1's CDecal::AddDecal, 0x312E80
 * in the demo: the same packet; SOCOM II's takes the point after the size and keeps it at 0x110 --
 * the flags are read by neither). The alpha test a materialised bool (the target's sltu / xori
 * before its branch), the points unrolled, the reference's address an integer sum (a pointer sum
 * folds this+16+32 into one addiu; the target adds twice), the decal returned (the target's mid-body
 * `move v0,a0`). OPEN, 50 instructions at the target's size: the colouring -- `tools/igreplay.py
 * --move prim=100.5` (prim numbered after the code generator's temporaries) gives the target's t0
 * for t, v1 for prim and a3 for the flag byte, leaving 8 code-generation temporaries off (the
 * constants' v0/a3 and t1-t3, which then set the schedule); no source shape tried numbers prim so
 * late -- one variable for the set-up and strip tags, prim declared after strip, blend first, a
 * ternary, a one-element local array, a one-word local struct, a bitfield struct and an inline
 * returning prim give 50-59 (build/scratch/wep/v_add*.py) */
CDecal *CDecal::AddDecal(float size, CPnt3D *pos, AddonPoint **points, unsigned int flags, CTexHandle *tex,
                         CVisual *visual, unsigned int vid)
{
    CTexture *t = tex->m_texture;

    m_size = size;
    m_view = vid;
    m_visual = visual;
    if (size < 0.0f)
        m_kind = 2;
    else
        m_kind = 1;

    unsigned int prim = 0x301DC000;
    bool blend = t->m_transparent && !t->m_bumpmap;

    if (blend)
        prim |= 0x200000;
    m_tex = t->m_dmaRefVu;
    m_pkt[0] = 0x3000000E;
    u_long128 *q = &m_tex;

    m_pkt[1] = (unsigned int)CACHED((unsigned int)q + 0x20);
    m_pkt[2] = 0x01000103;
    m_pkt[3] = 0x6E038006;
    unsigned int strip = (prim & 0xFFFC7FFF) | 0x18000;
    m_pkt[3] |= 0x4000;
    m_pkt[4] = *points[0]->m_rgba;
    m_pkt[5] = *points[1]->m_rgba;
    m_pkt[6] = *points[2]->m_rgba;
    m_pkt[7] = 0x01000101;
    m_pkt[8] = 0x6C048000;
    m_pkt[9] = 0x8000;
    m_pkt[10] = (strip & 0xFFFC7FFF) | 0x28000;
    m_pkt[11] = 0x412;
    m_pkt[12] = 0;
    m_pkt[13] = 0x8003;
    m_pkt[14] = strip;
    m_pkt[15] = 0x412;
    m_pkt[16] = 0;
    m_pkt[17] = 13;
    m_pkt[18] = 0;
    m_pkt[19] = 3;
    m_pkt[20] = 1;
    m_pkt[21] = 0;
    m_pkt[22] = 0;
    m_pkt[23] = 0;
    m_pkt[24] = 0;
    m_pkt[25] = 0x01000203;
    m_pkt[26] = 0x6C068004;

    AddonPoint *p;
    float *f = (float *)&m_pkt[27];

    p = points[0];
    f[0] = p->m_10.x;
    f[1] = p->m_10.y;
    f[2] = p->m_10.z;
    f[3] = p->m_20.x;
    f[4] = p->m_pos.x;
    f[5] = p->m_pos.y;
    f[6] = p->m_20.y;
    f[7] = p->m_20.z;
    p = points[1];
    f[8] = p->m_10.x;
    f[9] = p->m_10.y;
    f[10] = p->m_10.z;
    f[11] = p->m_20.x;
    f[12] = p->m_pos.x;
    f[13] = p->m_pos.y;
    f[14] = p->m_20.y;
    f[15] = p->m_20.z;
    p = points[2];
    f[16] = p->m_10.x;
    f[17] = p->m_10.y;
    f[18] = p->m_10.z;
    f[19] = p->m_20.x;
    f[20] = p->m_pos.x;
    f[21] = p->m_pos.y;
    f[22] = p->m_20.y;
    f[23] = p->m_20.z;
    m_pkt[51] = 0x01000102;
    m_pkt[52] = 0x6E01800D;
    m_pkt[52] |= 0x4000;
    ((unsigned char *)&m_pkt[53])[0] = 0;
    ((unsigned char *)&m_pkt[53])[1] = 3;
    ((unsigned char *)&m_pkt[53])[2] = 6;
    ((unsigned char *)&m_pkt[53])[3] = 1;
    m_pkt[54] = 0x01000102;
    m_pkt[55] = 0x6901800E;
    ((short *)&m_pkt[56])[0] = p->m_normal[0];
    ((short *)&m_pkt[56])[1] = p->m_normal[1];
    ((short *)&m_pkt[56])[2] = p->m_normal[2];
    ((short *)&m_pkt[56])[3] = 0;
    m_pkt[58] = 0x17000000;
    m_pkt[59] = 0;
    m_pos = *pos;
    return this;
}

/* the VU0 decal builder's batch (0x3B8680 makes it on first use: new in SOCOM II, no SOCOM 1
 * counterpart -- the names ours): a list of indices, 296 items and a count (0x2E50 bytes) */
namespace zdb {

class CDecalBatchItem {
public:
    /* (out of line at 0x3BEC40: the two lists, then the point and the two halfwords) */
    CDecalBatchItem()
    {
        m_pos = CPnt3D::zero;
        m_c = 0;
        m_e = -1;
    }
    /* (out of line at 0x3BEB00: the second list cleared, then both destroyed) */
    ~CDecalBatchItem() { m_1c.clear(); }

    CPnt3D m_pos;   // 0x0
    short m_c;   // 0xc
    short m_e;   // 0xe
    std::vector<unsigned int> m_10;   // 0x10
    std::vector<unsigned int> m_1c;   // 0x1c
};   // 0x28 bytes

class CDecalBatch {
public:
    CDecalBatch()
    {
        m_list.reserve(296);
        m_count = 0;
    }

    std::vector<unsigned int> m_list;   // 0x0
    CDecalBatchItem m_items[296];   // 0xc
    int m_count;   // 0x2e4c
};

}

extern "C" CDecalBatch *data_004B5078;

/* the batch made (0x3B8680's first use) */
extern "C" void func_003BEBD0()
{
    data_004B5078 = new CDecalBatch;
}

void CMeshDecalPool::Init(unsigned int count, unsigned int extra)
{
    m_max = count;
    m_extra = extra;
    m_pool = new RTArray<CMeshDecal>(count + extra, true, true);
}

CMeshDecal *CMeshDecalPool::Get(bool b)
{
    CMeshDecal *d = 0;

    if (m_count < m_max + m_extra) {
        d = m_pool->GetNext(true);
        if (d) {
            m_count++;
            if (m_count > m_max)
                m_overflow++;
        }
    }
    return d;
}

void CMeshDecal::PutTempDecal(CMeshDecal *d)
{
    if (m_tempPool.m_overflow)
        m_tempPool.m_overflow--;
    m_tempPool.m_count--;
    m_tempPool.m_pool->PutBack(d);
}

/* the overflow of temporary mesh decals worked off: each oldest one taken back, off its mesh and
 * freed (SOCOM 1's) */
void CMeshDecalPool::RecycleTick()
{
    while (m_overflow) {
        CMeshDecal *d = CMeshDecal::m_tempPool.m_pool->GetOldestUsed();

        CMeshDecal::m_tempPool.m_count--;
        CMeshDecal::m_tempPool.m_pool->PutBack(d);
        d->m_mesh->RemoveMeshDecal(d);
        d->FreeMem();
        m_overflow--;
    }
}

/* a permanent mesh decal given back: CDecal::PutPermDecal's twin over the mesh decals' permanent
 * pool, which SOCOM 1 did not have (the same 0x24 bytes; 0x3BE8C0) */
void CMeshDecal::PutPermDecal(CMeshDecal *d)
{
    m_permPool.m_count--;
    m_permPool.m_pool->PutBack(d);
}

/* the vertex block freed (SOCOM 1's, without its file and line) */
void CMeshDecal::FreeMem()
{
    if (m_verts)
        _free(CACHED(m_verts), 0, 0);
    m_verts = 0;
    m_34 = 0;
    m_30 = 0;
}

/* an item added to the VU0 decal builder's batch (0x3B8680's, its one caller; new in SOCOM II, not
 * named): the batch's next item, its point and halfwords reset and each list restarted with the
 * word given, then the point and the halfwords set; the first word is a strip whose id (its
 * halfword at 6) is searched for from the view data resumed, each strip found adding its vertex
 * and its id to the lists. The cursor declared first (numbered ahead of the item, so the item takes
 * s1 and the cursor the last halfword's s2, as the target), the id an unsigned int (an unsigned
 * short adds the narrowing `andi`) */
extern "C" void func_003BE5F0(unsigned int a, unsigned int b, const CPnt3D *pos, short c, short e,
                              CVisData *from)
{
    CVisData *v;
    CVisData vd;
    CDecalBatchItem *item = &data_004B5078->m_items[data_004B5078->m_count++];

    item->m_pos = CPnt3D::zero;
    item->m_c = 0;
    item->m_e = -1;
    item->m_10.clear();
    item->m_10.push_back(a);
    item->m_1c.clear();
    item->m_1c.push_back(b);
    item->m_pos = *pos;
    item->m_c = c;
    item->m_e = e;
    unsigned int id = ((unsigned short *)a)[3];
    v = vd.Resume(from);

    while (v) {
        v = v->SearchStrip(id);
        if (v) {
            unsigned int sid = v->m_id;
            unsigned int strip = (unsigned int)v->m_strip;

            item->m_10.push_back(strip);
            item->m_1c.push_back(sid);
        }
    }
}

void CMeshDecalPool::Uninit()
{
    delete m_pool;
    m_pool = 0;
    m_count = 0;
    m_overflow = 0;
}
