/* vis_texfx.cpp -- the texture effects: zdb::CTextureFX, a scrolling texture's state, the manager
 * that runs them and its pool of free ones (SOCOM 1's vis_texfx.cpp, by its static initialiser:
 * the active list and the CVisData zdb::vdata, built in that order, as SOCOM II's 0x404AB0 builds
 * the manager and vdata): OVL1 0x3BFB60-0x3C0CEF, after vis_decal.cpp. SOCOM 1 kept the active
 * list and the pool as CTextureFX's statics and the effect's set-up in CNode::ScrollTexture;
 * SOCOM II moves both into a manager object (theTextureFXManager, 0x4B50D0: CGame::StartEngine
 * calls its Init(20), StopEngine its Uninit, VisualEffectsTick its Tick, CNode::ScrollTexture its
 * Add, the ZAnim SCROLL_MOTION handlers its SetSpeed and Scroll) and keeps a scrolled fraction and
 * a group id per effect. The manager's and the effect's member names past SOCOM 1's are ours.
 * Defined in reverse of their image order (-inline deferred). */

#include "znode.h"
#include "../zvid/zvid.h"

/* a pool of T: the free items listed, new ones made when it runs dry (SOCOM 1's RTPool, outside
 * zdb as RTArray is, no DWARF: RTPool<CTextureFX>'s constructor, destructor, Clear and GetNext; its
 * members out of class like RTArray's, so the unit's own functions call them and its template
 * tail emits them) */
template <class T>
class RTPool {
public:
    RTPool(unsigned int count);
    ~RTPool();
    /* every free item deleted, the list emptied */
    void Clear();
    /* the first free item taken off the list, or, with none, a new one if alloc */
    T *GetNext(bool alloc);
    /* an item handed back */
    void PutBack(T *item) { m_free.push_back(item); }

    std::list<T *> m_free;   // 0x0
};

namespace zdb {

/* a texture scrolled over a visual's vertices: its speed, the fraction of a texel not yet moved,
 * the visual and view, and its group (0x1C bytes; SOCOM 1's was 0x10: the speed, the visual, the
 * view, the type and the frame) */
class CTextureFX {
public:
    CTextureFX() : m_stopped(false), m_head(false), m_tail(false) {}
    /* empty: the pool's delete tests the item first */
    ~CTextureFX() {}
    /* the UVs of the visual's view moved by (du, dv) in texels -- the whole of it from the tail's
     * vertices, the head's or all -- each triangle's kept in range, the fraction too small to move
     * kept for the next tick */
    void ScrollTexture(float du, float dv, bool head, bool tail);
    void SetStopped(bool b) { m_stopped = b; }
    /* the speed set, the effect stopped while it is too small to move anything */
    void SetSpeed(float u, float v)
    {
        m_u = u;
        m_v = v;
        SetStopped(true);
        if (!(fabsf(m_u) <= 1e-5f && fabsf(m_v) <= 1e-5f))
            SetStopped(false);
    }
    /* SOCOM 1's set-up (its InitScrollTexture, out of line there), the fraction cleared besides */
    void InitScrollTexture(float u, float v, bool tick, CVisual *visual, CNode *node)
    {
        SetSpeed(u, v);
        m_fracU = 0.0f;
        m_fracV = 0.0f;
        m_visual = visual;
        if (node)
            m_view = node->m_vid;
        else
            m_view = 0;
        if (tick)
            m_type = 1;
        else
            m_type = 2;
    }

    float m_u;   // 0x0: the speed, texels a second
    float m_v;   // 0x4
    float m_fracU;   // 0x8: what has not moved yet
    float m_fracV;   // 0xc
    CVisual *m_visual;   // 0x10
    unsigned short m_view;   // 0x14: the node's view id
    unsigned char m_type;   // 0x16: 1 runs on the tick, 2 only when scrolled
    unsigned char m_frame;   // 0x17: the frame it last ran on
    unsigned int m_id : 8;   // 0x18: its group (a bitfield: the flags share its word)
    unsigned int m_stopped : 1;   // 0x19
    unsigned int m_head : 1;
    unsigned int m_tail : 1;
};

/* the effects running, the pool of free ones and the next group id (SOCOM II's; 0x14 bytes) */
class CTextureFXManager {
public:
    typedef std::list<CTextureFX *>::iterator iterator;

    CTextureFXManager();
    void Init(int count);
    void Clear(bool all);
    void Uninit();
    int Add(bool tick, CVisual *visual, CNode *node, int id, float u, float v);
    void SetSpeed(int id, float u, float v);
    void Scroll(int id, float du, float dv, bool head, bool tail);
    CTextureFX *Find(CVisual *visual);
    void Tick(float dt);

    std::list<CTextureFX *> m_texfxpool;   // 0x0: SOCOM 1's static's name
    RTPool<CTextureFX> *m_texfxfreepool;   // 0xc
    int m_nextId;   // 0x10
};

void LimitUVs(short *a, short *b, short *c);

/* the UV wrap (0x5000: five texture widths at 4096 a texel) */
short uvlimit = 0x5000;

/* the initialiser (__sinit_vis_texfx.cpp, 0x404AB0) builds these in order, registering each
 * one's destructor */
CTextureFXManager theTextureFXManager;
CVisData vdata;

/* empty, no pool yet (an own function the initialiser inlines, dead-stripped after: it instantiates
 * the list's out-of-class constructor, which the template tail -- RTPool's constructor, the
 * initialiser -- then inlines) */
CTextureFXManager::CTextureFXManager() : m_texfxfreepool(0), m_nextId(0)
{
}

/* the pool made with count free effects, group ids from 0 */
void CTextureFXManager::Init(int count)
{
    m_texfxfreepool = new RTPool<CTextureFX>(count);
    m_nextId = 0;
}

/* every running effect handed back to the pool, and with all the pool emptied */
void CTextureFXManager::Clear(bool all)
{
    iterator i;
    iterator e = m_texfxpool.end();

    for (i = m_texfxpool.begin(); i != e; ++i)
        m_texfxfreepool->PutBack(*i);
    m_texfxpool.clear();
    if (all)
        m_texfxfreepool->Clear();
}

void CTextureFXManager::Uninit()
{
    delete m_texfxfreepool;
    m_texfxfreepool = 0;
    m_nextId = 0;
}

/* a visual's effect started, or restarted: a new one takes the group asked for (a new group when
 * id is negative), an old one keeps its own; the group is given back (SOCOM 1's
 * InitScrollTexture's arguments, the speed through SetSpeed) */
int CTextureFXManager::Add(bool tick, CVisual *visual, CNode *node, int id, float u, float v)
{
    CTextureFX *fx = Find(visual);

    if (!fx) {
        fx = m_texfxfreepool->GetNext(true);
        m_texfxpool.push_back(fx);
        if (id < 0) {
            m_nextId++;
            id = m_nextId - 1;
        }
        fx->m_id = id;
    } else
        id = fx->m_id;
    if (fx)
        fx->InitScrollTexture(u, v, tick, visual, node);
    return id;
}

/* a group's speed set */
void CTextureFXManager::SetSpeed(int id, float u, float v)
{
    iterator i;
    iterator e = m_texfxpool.end();

    for (i = m_texfxpool.begin(); i != e; ++i) {
        CTextureFX *fx = *i;

        if (id == fx->m_id)
            fx->SetSpeed(u, v);
    }
}

/* a group scrolled by hand */
void CTextureFXManager::Scroll(int id, float du, float dv, bool head, bool tail)
{
    iterator i;
    iterator e = m_texfxpool.end();

    for (i = m_texfxpool.begin(); i != e; ++i) {
        CTextureFX *fx = *i;

        if (id == fx->m_id)
            fx->ScrollTexture(du, dv, head, tail);
    }
}

/* a visual's effect, none if it has none (called: Add's call stays out of line, pinned) */
#pragma auto_inline off
CTextureFX *CTextureFXManager::Find(CVisual *visual)
{
    iterator i;

    for (i = m_texfxpool.begin(); i != m_texfxpool.end(); ++i)
        if ((*i)->m_visual == visual)
            return *i;
    return 0;
}
#pragma auto_inline reset

/* every ticking effect that is moving scrolled by its speed, once a frame */
void CTextureFXManager::Tick(float dt)
{
    iterator i;
    iterator e = m_texfxpool.end();

    for (i = m_texfxpool.begin(); i != e; ++i) {
        CTextureFX *fx = *i;

        if (fx->m_type == 1 && !fx->m_stopped && (unsigned char)zVid.frameNumber != fx->m_frame) {
            fx->m_frame = zVid.frameNumber;
            fx->ScrollTexture(fx->m_u * dt, fx->m_v * dt, fx->m_head, fx->m_tail);
        }
    }
}

void CTextureFX::ScrollTexture(float du, float dv, bool head, bool tail)
{
    CVisData *data = &vdata;
    short u = (int)(4096.0f * (du + m_fracU));
    short v = (int)(4096.0f * (dv + m_fracV));

    if (u != 0 || v != 0) {
        CVisData *d;

        if (tail)
            d = data->SelectVertexTail(m_visual, m_view, 0);
        else if (head)
            d = data->SelectVertexHead(m_visual, m_view, 0);
        else
            d = data->SelectVertex(m_visual, m_view, 0);
        while (d) {
            short *a = (short *)d->m_vertex + 4;

            a[0] += u;
            a[1] += v;
            d = d->NextVertex(1);
            if (!d)
                return;
            short *b = (short *)d->m_vertex + 4;

            b[0] += u;
            b[1] += v;
            d = d->NextVertex(1);
            if (!d)
                return;
            short *c = (short *)d->m_vertex + 4;

            c[0] += u;
            c[1] += v;
            LimitUVs(a, b, c);
            d = d->NextVertex(1);
        }
    }
    if (du != 0.0f && u == 0)
        m_fracU += du;
    else
        m_fracU = 0.0f;
    if (dv != 0.0f && v == 0)
        m_fracV += dv;
    else
        m_fracV = 0.0f;
}

/* a triangle's UVs moved back a whole wrap together, an axis at a time, wherever a vertex has run
 * past it (SOCOM 1's, over the global uvlimit) */
void LimitUVs(short *a, short *b, short *c)
{
    if (a[0] >= uvlimit) {
        a[0] -= uvlimit;
        b[0] -= uvlimit;
        c[0] -= uvlimit;
    } else if (a[0] <= -uvlimit) {
        a[0] += uvlimit;
        b[0] += uvlimit;
        c[0] += uvlimit;
    }
    if (a[1] >= uvlimit) {
        a[1] -= uvlimit;
        b[1] -= uvlimit;
        c[1] -= uvlimit;
    } else if (a[1] <= -uvlimit) {
        a[1] += uvlimit;
        b[1] += uvlimit;
        c[1] += uvlimit;
    }
    if (b[0] >= uvlimit) {
        a[0] -= uvlimit;
        b[0] -= uvlimit;
        c[0] -= uvlimit;
    } else if (b[0] <= -uvlimit) {
        a[0] += uvlimit;
        b[0] += uvlimit;
        c[0] += uvlimit;
    }
    if (b[1] >= uvlimit) {
        a[1] -= uvlimit;
        b[1] -= uvlimit;
        c[1] -= uvlimit;
    } else if (b[1] <= -uvlimit) {
        a[1] += uvlimit;
        b[1] += uvlimit;
        c[1] += uvlimit;
    }
    if (c[0] >= uvlimit) {
        a[0] -= uvlimit;
        b[0] -= uvlimit;
        c[0] -= uvlimit;
    } else if (c[0] <= -uvlimit) {
        a[0] += uvlimit;
        b[0] += uvlimit;
        c[0] += uvlimit;
    }
    if (c[1] >= uvlimit) {
        a[1] -= uvlimit;
        b[1] -= uvlimit;
        c[1] -= uvlimit;
    } else if (c[1] <= -uvlimit) {
        a[1] += uvlimit;
        b[1] += uvlimit;
        c[1] += uvlimit;
    }
}

}

/* the pool's members (SOCOM 1's RTPool<CTextureFX>: the list sized and each slot given a new
 * item; the deleting and taking off) */
template <class T>
RTPool<T>::RTPool(unsigned int count)
{
    typename std::list<T *>::iterator i;

    m_free.resize(count);
    for (i = m_free.begin(); i != m_free.end(); ++i)
        *i = new T;
}

template <class T>
RTPool<T>::~RTPool()
{
    Clear();
}

template <class T>
void RTPool<T>::Clear()
{
    typename std::list<T *>::iterator i;
    typename std::list<T *>::iterator e = m_free.end();

    for (i = m_free.begin(); i != e; ++i)
        delete *i;
    m_free.clear();
}

template <class T>
T *RTPool<T>::GetNext(bool alloc)
{
    T *item = 0;

    if (m_free.size())
        item = m_free.front();
    if (item)
        m_free.remove(item);
    else if (alloc)
        item = new T;
    return item;
}
