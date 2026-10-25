/* The teammates' names over their heads in a network game (CMPTeammateNames, SOCOM 1's
 * CTeamMateNames): eight strings now, and two bitmaps. Its constructor is inline (CHUD's
 * inlines it); its vtable 0x4057D0 overrides On and Tick. The header's name is ours. */
#ifndef HUD_TEAMMATENAMES_H
#define HUD_TEAMMATENAMES_H

#include "../../gamez/ztwod/ztwod.h"

class CMPTeammateNames : public C2D {
public:
    virtual void On();
    virtual void Tick(float dT);
    void UnInit();
    void Init();

    C2DString m_names[8];   // 0x18
    C2DBitmap m_bitmap1;   // 0x4e0
    C2DBitmap m_bitmap2;   // 0x560
    int m_socom2_5e0;   // 0x5e0: zeroed by Init
    bool m_socom2_5e4;   // 0x5e4: On shows the names only while it is set
};   // 0x5f0 bytes

#endif
