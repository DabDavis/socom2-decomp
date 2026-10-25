/* hud_pausemenu.cpp -- the HUD's pause menu (CZPauseTest, OVL1 0x201580-0x2057FF): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_pausemenu.cpp), after the lens flares and before the
 * objectives as there; its first functions in the image are the vtable's On, Off and Draw, its
 * last the destructor and the constructor. SOCOM II's CZPauseTest is 0x13C0 bytes (SOCOM 1's
 * 0x380), laid out here only as far as these reach. Only On, Off and Draw so far. Defined in
 * reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

/* a menu line (0xA4 bytes: the menu's array of them is new'ed; only Draw is called) */
class CPauseItem : public C2D {
public:
    unsigned char _18[0xa4 - 0x18];
};

/* the pause menu as its first functions see it: C2D's vtable at 0xC, the lines through a pointer
 * at 0x18, the background and two panels (only their C2D Draw slot called), the open flag that
 * On and Off keep, and a flag that swaps the lines for the third panel */
class CZPauseTest : public C2D {
public:
    CZPauseTest();
    virtual ~CZPauseTest();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Off();
    virtual void On();

    CPauseItem *m_items;   // 0x18
    unsigned char _1c[0x30 - 0x1c];
    C2D m_background;   // 0x30
    unsigned char _48[0x220 - 0x48];
    C2D m_panel;   // 0x220
    unsigned char _238[0xbc0 - 0x238];
    unsigned char m_open;   // 0xbc0
    unsigned char _bc1[0xbdc - 0xbc1];
    unsigned char m_socom2_bdc;   // 0xbdc: the third panel drawn instead of the lines
    unsigned char _bdd[0xc70 - 0xbdd];
    C2D m_panel2;   // 0xc70
    unsigned char _c88[0x13c0 - 0xc88];
} __attribute__((aligned(16)));   // 0x13c0 bytes (CHUD's member at 0x18900)

/* the menu drawn while open, or while 0x409080 holds (a byte; no row named yet): the background,
 * then the third panel or the second and the twelve lines */
extern unsigned char data_00409080;

void CZPauseTest::Draw(zdb::CCamera *cam)
{
    if (!m_open && !data_00409080)
        return;
    m_background.Draw(cam);
    if (m_socom2_bdc) {
        m_panel2.Draw(cam);
        return;
    }
    m_panel.Draw(cam);
    /* an int counter, the index narrowed to a signed char each pass (sign-extended, never
     * strength-reduced, as the target; a signed char counter is reduced to a walking pointer) --
     * likely an accessor taking a char */
    for (int i = 0; i < 12; i++)
        m_items[(signed char)i].Draw(cam);
}

void CZPauseTest::Off()
{
    m_open = 0;
    C2D::Off();
}

void CZPauseTest::On()
{
    m_open = 1;
    C2D::On();
}
