/* vis_decal.cpp -- the decals: zdb::CDecal and CMeshDecal, their pools, and the RTArray instances
 * that hold them (SOCOM 1's vis_decal.cpp, by its static initialiser, which builds the four
 * pools): OVL1 0x3BD9A0?-0x3BF9AF?, as SOCOM 1's 0x312DE0-0x313BBF. Only some of its functions so
 * far; the rest stay generated drafts or placeholders. Defined in reverse of their image order
 * (-inline deferred). */

#include "znode.h"
#include "../zbody/zbody.h"
#include "../zsys/zsys_public.h"

namespace zdb {

class CDecal;
class CMeshDecal;

/* a decal's pool: the decals in use, the most, the extra allowed past it and how often the most
 * was passed, over an RTArray of them (SOCOM 1's; 0x14 bytes, the statics m_tempPool and
 * m_permPool; the member names ours) */
class CDecalPool {
public:
    CDecal *Get(bool b);

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CDecal> *m_pool;   // 0x10
};

/* the same over the mesh decals (SOCOM 1's) */
class CMeshDecalPool {
public:
    CMeshDecal *Get(bool b);

    unsigned int m_count;   // 0x0
    unsigned int m_max;   // 0x4
    unsigned int m_extra;   // 0x8
    unsigned int m_overflow;   // 0xc
    RTArray<CMeshDecal> *m_pool;   // 0x10
};

/* a decal on a visual (SOCOM 1's class: no DWARF; only its pools here) */
class CDecal {
public:
    static void PutPermDecal(CDecal *d);
    static void PutTempDecal(CDecal *d);

    static CDecalPool m_tempPool;
    static CDecalPool m_permPool;
};

/* a decal on a mesh: its own vertex block (SOCOM 1's class: no DWARF; the names ours) */
class CMeshDecal {
public:
    void FreeMem();
    static void PutTempDecal(CMeshDecal *d);
    static void PutPermDecal(CMeshDecal *d);

    unsigned char _0[0x10];
    void *m_verts;   // 0x10: allocated, through the uncached mirror
    unsigned char _14[0x30 - 0x14];
    unsigned int m_30;   // 0x30
    unsigned int m_34;   // 0x34

    static CMeshDecalPool m_tempPool;
    static CMeshDecalPool m_permPool;
};

}

using namespace zdb;

/* a cached pointer from its uncached or accelerated mirror */
#define CACHED(p) ((void *)((unsigned int)(p) & 0x0FFFFFFF))

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
