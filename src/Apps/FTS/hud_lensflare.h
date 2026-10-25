/* The HUD's lens flares: SOCOM II's manager and entries, where SOCOM 1's hud_lensflare.cpp had
 * CLensFlareSaturate. The header's name is ours. */
#ifndef HUD_LENSFLARE_H
#define HUD_LENSFLARE_H

#include <list>

#include "../../gamez/ztwod/ztwod.h"

namespace zdb {
class CNode;
}

/* a flare: a bitmap (its vtable 0x4052B0 has C2DBitmap's Draw) with a handle in the flare system
 * (-1 for none), placed on a node of the world when it tracks one (the manager's PreDITick and
 * ReInit; the names ours) */
class CLensFlareEntry : public C2DBitmap {
public:
    virtual ~CLensFlareEntry();

    unsigned char m_on : 1;   // 0x80 bit 0: PreDITick asks the flare system only about these
    unsigned char _80_1 : 1;
    unsigned char m_visible : 1;   // bit 2: the flare system's answer
    unsigned char _80_3 : 2;
    unsigned char m_tracking : 1;   // bit 5: the position follows the node each frame
    unsigned char _80_6 : 2;
    unsigned char _81[0x8c - 0x81];
    int m_handle;   // 0x8c
    unsigned char m_90[0xb4 - 0x90];   // what PreDITick hands the flare system's query
    zdb::CNode *m_node;   // 0xb4
    CPnt3D m_pos;   // 0xb8: the node's origin in the world
    float m_size;   // 0xc4: given to the flare system with the position
};

/* the flares, a list as the base (built before the vtable pointer is stored, which follows it at
 * 0xc). The vtable is three words: Tick its only slot, no virtual destructor */
class CLensFlareManager : public std::list<CLensFlareEntry *> {
public:
    virtual void Tick(float dT);
    void PreDITick(float dT);
    void ReInit();
    void Reset();
    void LoadRdr(const char *name);
    void Init();
    void Clear();

    bool m_active;   // 0x10
};   // 0x14 bytes

#endif
