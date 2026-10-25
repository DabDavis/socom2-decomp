/* hud_bitmapreticule.cpp -- the reticle (OVL1 ...-0x21938F): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_bitmapreticule.cpp). Only UnInit, Draw, On, Off and the pause test's
 * OnPauseTest (0x215180-0x215243) so far, in SOCOM 1's order;
 * BitmapReticule is 0x1000 bytes longer in SOCOM II, and only the members those two touch are
 * placed (SOCOM 1's names, SOCOM II's offsets; the reticle type is a byte now). Defined in reverse
 * of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

class CHUD;

class BitmapReticule : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
    virtual void Off();
    virtual void On();
    void OnPauseTest();
    void UnInit();

    CHUD *m_parent;   // 0x18 (SOCOM 1: 0x1c)
    /* three groups of the reticle's pieces (SOCOM II's: plain C2Ds, the constructor builds them
     * first; the names ours) */
    C2D m_group0;   // 0x1c
    C2D m_group1;   // 0x34
    C2D m_group2;   // 0x4c
    unsigned char _64[0x4108 - 0x64];
    C2DString m_lase_update;   // 0x4108 (SOCOM 1: 0x3124)
    unsigned char _41a0[0x41c8 - 0x41a0];
    C2DString m_detonation_update;   // 0x41c8 (SOCOM 1: 0x31d4)
    unsigned char _4260[0x44b8 - 0x4260];
    signed char m_rettype;   // 0x44b8 (SOCOM 1: an int at 0x33ac)
    zdb::CTexHandle *m_reticuleTex[10];   // 0x44bc (SOCOM 1: 0x33b0)
    unsigned char _44e4[0x4544 - 0x44e4];
    C2DString m_enemy_name;   // 0x4544 (SOCOM 1: 0x3430)
};

/* the reticle on: itself and its three groups (SOCOM 1's On, which only turned itself on) */
void BitmapReticule::On()
{
    C2D::On();
    m_group0.On();
    m_group1.On();
    m_group2.On();
}

/* the reticle's groups off (SOCOM 1's Off; the reticle itself is left on) */
void BitmapReticule::Off()
{
    m_group0.Off();
    m_group1.Off();
    m_group2.Off();
}

/* the second group shown alone: CHUD::Off calls it in the pause test (SOCOM II's; the name ours) */
void BitmapReticule::OnPauseTest()
{
    m_group1.On();
}

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
