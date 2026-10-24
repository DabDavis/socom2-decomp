/* hud_bitmapreticule.cpp -- the reticle (OVL1 ...-0x21938F): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_bitmapreticule.cpp). Only UnInit and Draw so far, in SOCOM 1's order;
 * BitmapReticule is 0x1000 bytes longer in SOCOM II, and only the members those two touch are
 * placed (SOCOM 1's names, SOCOM II's offsets; the reticle type is a byte now). Defined in reverse
 * of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

class BitmapReticule : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
    void UnInit();

    unsigned char _18[0x4108 - 0x18];
    C2DString m_lase_update;   // 0x4108 (SOCOM 1: 0x3124)
    unsigned char _41a0[0x41c8 - 0x41a0];
    C2DString m_detonation_update;   // 0x41c8 (SOCOM 1: 0x31d4)
    unsigned char _4260[0x44b8 - 0x4260];
    signed char m_rettype;   // 0x44b8 (SOCOM 1: an int at 0x33ac)
    zdb::CTexHandle *m_reticuleTex[10];   // 0x44bc (SOCOM 1: 0x33b0)
    unsigned char _44e4[0x4544 - 0x44e4];
    C2DString m_enemy_name;   // 0x4544 (SOCOM 1: 0x3430)
};

/* the reticle drawn if its type has a texture */
void BitmapReticule::Draw(zdb::CCamera *cam)
{
    if (m_reticuleTex[m_rettype])
        C2D::Draw(cam);
}

void BitmapReticule::UnInit()
{
    m_lase_update.ClearAndWipe();
    m_detonation_update.ClearAndWipe();
    m_enemy_name.ClearAndWipe();
}
