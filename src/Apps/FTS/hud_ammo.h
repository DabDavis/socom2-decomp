/* The HUD's ammunition counts (SOCOM II's; no vtable, so no RTTI name: the class and member names
 * are ours, from its functions' AMMO, MAGS, "%d/%d" and "%d MAG%c"). The header's name is ours. */
#ifndef HUD_AMMO_H
#define HUD_AMMO_H

#include "../../gamez/ztwod/ztwod.h"

class CAmmoDisplay {
public:
    CAmmoDisplay();
    ~CAmmoDisplay();
    void Reset();
    /* the counts and their bitmaps off (0x237DE0) and, in play, on (0x237E50) */
    void Off();
    void On();

    C2DBitmapPoly m_poly;   // 0x0
    C2DBitmap m_bitmap;   // 0x1f0
    C2D m_head;   // 0x270
    C2DString m_ammo;   // 0x288
    C2DString m_mags;   // 0x320
    C2DBitmap m_bitmaps[4];   // 0x3c0
    int m_socom2_5c0;   // 0x5c0
} __attribute__((aligned(16)));   // 0x5d0 bytes

#endif
