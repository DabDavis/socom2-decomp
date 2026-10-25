/* The HUD's lens flares: SOCOM II's manager and entries, where SOCOM 1's hud_lensflare.cpp had
 * CLensFlareSaturate. The header's name is ours. */
#ifndef HUD_LENSFLARE_H
#define HUD_LENSFLARE_H

#include <list>

#include "../../gamez/ztwod/ztwod.h"

/* a flare: a bitmap (its vtable 0x4052B0 has C2DBitmap's Draw) with a handle in the flare system
 * (-1 for none) */
class CLensFlareEntry : public C2DBitmap {
public:
    virtual ~CLensFlareEntry();

    unsigned char _80[0x8c - 0x80];
    int m_handle;   // 0x8c (the name ours)
};

/* the flares, a list as the base (built before the vtable pointer is stored, which follows it at
 * 0xc). The vtable is three words: Tick its only slot, no virtual destructor */
class CLensFlareManager : public std::list<CLensFlareEntry *> {
public:
    virtual void Tick(float dT);
    void Reset();
    void LoadRdr(const char *name);
    void Init();
    void Clear();

    bool m_active;   // 0x10
};   // 0x14 bytes

#endif
