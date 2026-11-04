/* node_wave.cpp -- the lens flare node and SOCOM 1's decal ripples (OVL1 0x321DB0-0x3223CF, after
 * node_assetlib.cpp; zparticle.cpp follows at 0x3223D0). The unit name is SOCOM 1's: its
 * __sinit_node_wave.cpp builds CRipple::m_ripplepool, as SOCOM II's 0x402A90 builds the list at
 * 0x45C3E0 (the integrator's initialiser research), and SOCOM 1's node_wave.cpp holds the same
 * functions in the same order -- CLensFlare's constructor and destructor, ~CVisData, CRipple's Tick,
 * Uninit and Init, then RTPool<CRipple>'s and the ripple list's template tail. SOCOM II keeps the
 * ripples' tick and teardown but makes none (its Init is empty, so RTPool<CRipple>'s constructor is
 * never instantiated); its Tick calls the flattening SOCOM 1 inlined. No DWARF for either class: the
 * member names are ours. Defined in reverse of their image order (-inline deferred).
 *
 * ~CVisData 0x321EA0 and std::stack<void *>'s top() 0x321EF0 and empty() 0x321F40 sit between the
 * lens flare and the tick: copies this unit's object carried (the linker keeps the first), their
 * caller here stripped -- the vertex walk's CVisData and a GetVid() inlined one level. Which function
 * that was is not known; the copies this file does not emit match from vis_texfx.cpp's and
 * vis_vu.cpp's. */

#include "znode.h"
#include "vis_texfx.h"

namespace zdb {

/* a lens flare in the scene graph (node type 9; SOCOM 1's class, same slot of the same vtable in
 * both builds: only the destructor its own) */
class CLensFlare : public CNode {
public:
    CLensFlare();
    virtual ~CLensFlare();

    float _c0[3];   // 0xc0 (not set by the constructor)
    std::vector<unsigned int> m_flares;   // 0xcc (SOCOM 1's destructor frees a vector<unsigned int>)
};   // 0xd8 bytes

/* a ripple run over a visual's vertices for a time, its vertices flattened when the last ripple on
 * the visual ends (SOCOM 1's zdb::CRipple, no DWARF: the layout read off Tick, the names ours) */
class CRipple {
public:
    /* empty: the pool's delete tests the item first (as CTextureFX's) */
    ~CRipple() {}
    static void Init();
    static void Uninit();
    static void Tick(float dt);
    /* the visual's vertices set flat, the ripple done (SOCOM 1 inlined it into Tick) */
    void Flatten();

    unsigned char _0[0xc];
    float m_time;   // 0xc: how long it has run
    unsigned char _10[0x1c - 0x10];
    float m_life;   // 0x1c: how long it runs
    unsigned char _20[0x2c - 0x20];
    bool m_active;   // 0x2c
    CVisual *m_visual;   // 0x30
    CNode *m_node;   // 0x34: the node it is on (its view picks the vertices)

    /* the ripples running (0x45C3E0) and the free ones (0x45C3F0) */
    static std::list<CRipple *> m_ripplepool;
    static RTPool<CRipple> *m_ripplefreepool;
};

std::list<CRipple *> CRipple::m_ripplepool;
RTPool<CRipple> *CRipple::m_ripplefreepool;

/* SOCOM 1's pool made the free ripples; SOCOM II's makes none */
void CRipple::Init()
{
}

void CRipple::Uninit()
{
    delete m_ripplefreepool;
    m_ripplefreepool = 0;
}

/* OPEN, 0xB8 against 0xF0: the up normal -- the target multiplies 32767.5 by 0.0 and by 1.0 at run
 * time and hoists the products, converted to shorts, out of the walk (s2 for the two zeros, s1 for
 * the one), where MW here folds them to constants: vis_ripple.cpp's CWaterRipple::Flatten has the
 * same loop and the same miss. Tried: the point outside the walk, float locals, a struct
 * copy, an assignment, a brace-initialised PNT3D, a packing helper or a normal setter taking the
 * floats (0x90-0x98), a const reference to a temporary (0xF0, the products in the walk), the shorts
 * computed ahead of the walk; a function-scoped `#pragma opt_propagation off` comes nearest (0xF4:
 * the products hoisted into f20/f21, their conversions left in the walk) */
#pragma auto_inline off
void CRipple::Flatten()
{
    unsigned int view = m_node ? m_node->m_vid : 0;
    CVisData data;
    CVisData *v;

    for (v = data.SelectVertex(m_visual, view, 0); v; v = v->NextVertex(1)) {
        CPnt3D up(0.0f, 1.0f, 0.0f);

        ((short *)v->m_vertex)[3] = (short)(32767.5f * up.x);
        ((short *)v->m_vertex)[6] = (short)(32767.5f * up.y);
        ((short *)v->m_vertex)[7] = (short)(32767.5f * up.z);
    }
    m_active = false;
}
#pragma auto_inline reset

/* the ripple at i taken off the running list: a stand-in for list<T *>'s own erase(iterator), which
 * include/list does not have (SOCOM 1's Tick converts the wrapper's iterator into the base's for the
 * call) -- the erase's temporaries take the frame's lowest slots after PutBack's argument, as a call
 * one inline level down does; called straight, they sit above it (the round's measurement) */
inline std::list<CRipple *>::iterator Erase(std::list<CRipple *>::iterator i)
{
    return CRipple::m_ripplepool.erase(i);
}

/* each ripple aged; one past its life handed back to the pool, its visual flattened unless another
 * ripple still runs on it (the outer end kept in a saved register, the inner one taken into a local
 * after the iterator: so the target numbers the walk's temporaries) */
void CRipple::Tick(float dt)
{
    std::list<CRipple *>::iterator e = m_ripplepool.end();
    std::list<CRipple *>::iterator i;

    i = m_ripplepool.begin();
    while (i != e) {
        CRipple *r = *i;

        r->m_time += dt;
        if (r->m_time >= r->m_life) {
            i = Erase(i);
            m_ripplefreepool->PutBack(r);

            bool last = true;
            CVisual *vis = r->m_visual;
            std::list<CRipple *>::iterator j;
            std::list<CRipple *>::iterator je = m_ripplepool.end();

            for (j = m_ripplepool.begin(); j != je; ++j)
                if ((*j)->m_visual == vis) {
                    last = false;
                    break;
                }
            if (last)
                r->Flatten();
        } else
            ++i;
    }
}

CLensFlare::~CLensFlare()
{
}

CLensFlare::CLensFlare()
{
    m_type = 9;
}

}

/* the pool's members (vis_texfx.cpp's, for RTPool<CRipple>: only the teardown is instantiated --
 * Clear inlines the range erase here, where RTPool<CTextureFX>'s calls it).
 *
 * The ripple list's destructor ~list<CRipple *> 0x322370 (__sinit_node_wave.cpp registers it) is
 * OPEN, 0xD4 against 0x60: the target inlines ~__list_imp and calls ~__list_deleter (0x1EA2D0), as
 * node_main.cpp's ~list<CVisual *> 0x3177C0 does; here MW inlines the whole chain with the range
 * erase. A deleter destructor one level taller (`clear()` for its erase) gives the target's 0x60
 * but costs this unit's ~RTPool and Clear (0x68, 0xAC) -- an include/list question (the drafts'
 * matches stand: 00322370.c carries an `sc: as=` line) */
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
