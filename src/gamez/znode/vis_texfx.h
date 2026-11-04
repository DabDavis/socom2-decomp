/* vis_texfx.h -- the texture effects (vis_texfx.cpp, OVL1 0x3BFB60-0x3C0CEF): zdb::CTextureFX, a
 * scrolling texture's state, the pool of free ones and the manager, theTextureFXManager (0x4B50D0),
 * that CGame::StartEngine/StopEngine, VisualEffectsTick, CNode::ScrollTexture and the ZAnim
 * SCROLL_MOTION handlers (zanim_cmd_tick.cpp) call as members. The manager's and the effect's
 * member names past SOCOM 1's are ours (vis_texfx.cpp's head). */

#ifndef VIS_TEXFX_H
#define VIS_TEXFX_H

#include "znode.h"

/* a pool of T: the free items listed, new ones made when it runs dry (SOCOM 1's RTPool, outside
 * zdb as RTArray is, no DWARF: RTPool<CTextureFX>'s constructor, destructor, Clear and GetNext; its
 * members out of class like RTArray's, so the unit's own functions call them and its template
 * tail emits them -- defined in vis_texfx.cpp) */
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

/* the manager (0x4B50D0; vis_texfx.cpp's initialiser builds it) */
extern CTextureFXManager theTextureFXManager;

}   // namespace zdb

#endif
