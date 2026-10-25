/* hud_pausemenu.cpp -- the HUD's pause menu (CZPauseTest, OVL1 0x201580-0x2057FF): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_pausemenu.cpp), after the lens flares and before the
 * objectives as there; its first functions in the image are the vtable's On, Off and Draw, its
 * last the destructor and the constructor. SOCOM II's CZPauseTest is 0x13C0 bytes (SOCOM 1's
 * 0x380), laid out from its destructor. Written: On, Off, Draw, Clear and ReInit. Defined in
 * reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

/* a menu line (0xA4 bytes: the menu's array of twelve is new'ed): a string first and three words
 * after it -- a member, not a base: the line's implicit destructor (0x2057A0, which delete[] hands
 * to __destroy_new_array) destroys the string with -1 */
struct CPauseItem {
    C2DString m_text;   // 0x0
    unsigned char _98[0xa4 - 0x98];
};

/* the pause menu, laid out from its destructor (0x205480): the lines through a pointer at 0x18, the
 * background (a bitmap of two triangles), bitmaps, strings and three more two-triangle bitmaps, the
 * open flag that On and Off keep and a flag that swaps the lines for the third panel; the members'
 * names are ours. Each of the three bitmap-polys and seven strings at 0xCF0 is destroyed on its
 * own, so they are members, not arrays */
class CZPauseTest : public C2D {
public:
    CZPauseTest();
    virtual ~CZPauseTest();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Off();
    virtual void On();
    void ReInit();
    void Clear();
    void ClosePauseMenu();

    CPauseItem *m_items;   // 0x18
    signed char m_sel;   // 0x1c: the line selected
    unsigned char _1d[0x30 - 0x1d];
    C2DBitmapPoly m_background;   // 0x30
    C2DBitmap m_panel;   // 0x220
    C2DBitmap m_bitmap2a0;   // 0x2a0
    C2DBitmap m_bitmap320;   // 0x320
    C2DBitmap m_bitmap3a0;   // 0x3a0
    C2DString m_text420;   // 0x420
    C2DBitmapPoly m_poly4c0;   // 0x4c0
    C2DBitmapPoly m_poly6b0;   // 0x6b0
    C2DBitmapPoly m_poly8a0;   // 0x8a0
    C2DString m_texta90;   // 0xa90
    C2DString m_textb28;   // 0xb28
    unsigned char m_open;   // 0xbc0
    unsigned char m_socom2_bc1;   // 0xbc1: cleared by ReInit
    unsigned char _bc2[0xbdc - 0xbc2];
    unsigned char m_socom2_bdc;   // 0xbdc: the third panel drawn instead of the lines
    unsigned char _bdd[0xbf0 - 0xbdd];
    C2DBitmap m_bitmapbf0;   // 0xbf0
    C2DBitmap m_panel2;   // 0xc70
    C2DString m_textcf0;   // 0xcf0
    C2DString m_textd88;   // 0xd88
    C2DString m_texte20;   // 0xe20
    C2DString m_texteb8;   // 0xeb8
    C2DString m_textf50;   // 0xf50
    C2DString m_textfe8;   // 0xfe8
    C2DString m_text1080;   // 0x1080
    C2DBitmap m_bitmap1120;   // 0x1120
    C2DBitmap m_bitmap11a0;   // 0x11a0
    C2DString m_text1220;   // 0x1220
    C2DBitmap m_bitmap12c0;   // 0x12c0
    C2DBitmap m_bitmap1340;   // 0x1340
} __attribute__((aligned(16)));   // 0x13c0 bytes (CHUD's member at 0x18900)

/* the menu drawn while open, or while 0x409080 holds (a byte; no row named yet): the background,
 * then the third panel or the second and the twelve lines */
extern unsigned char data_00409080;
/* a second byte the menu keeps (OVL1 bss 0x409070; no row named yet) */
extern unsigned char data_00409070;

/* a word the menu keeps (OVL1 bss 0x409088; no row named yet) and the lines' test, whether one can
 * be selected (0x205A70, an inline's copy after the constructor; no row named yet) */
extern int data_00409088;
extern "C" bool func_00205A70(CZPauseTest *menu, signed char line);

/* the menu set up for a mission: the first line that can be selected (wrapping after the twelfth),
 * the accept and back prompts, the two questions, the answers (CHUD::ReInit_Init's call; the name
 * ours) */
void CZPauseTest::ReInit()
{
    signed char sel = -1;
    bool found = false;

    /* the step cast back to a signed char: MW re-extends the line before adding, as the target
     * (an increment or `++sel` adds to the register as it stands) */
    do {
        sel = (signed char)(sel + 1);
        if (sel == 12)
            sel = 0;
        if (func_00205A70(this, sel))
            found = true;
    } while (!found);
    m_sel = sel;
    m_socom2_bc1 = 0;
    m_texta90.SetString("\xa6 ACCEPT", true);
    m_textb28.SetString("\xac BACK", true);
    m_textcf0.SetString("Are you sure you want", true);
    m_textd88.SetString("to abort the game?", true);
    m_texte20.SetString(" ", true);
    m_texteb8.SetString("Are you sure you want", true);
    m_textf50.SetString("to replay the mission?", true);
    m_textfe8.SetString(" ", true);
    m_text1080.SetString("YES", true);
    m_text1220.SetString("NO", true);
    data_00409088 = 0;
}

/* both flags down, every line and string wiped, the children let go (the line index narrowed to a
 * signed char, as Draw's) */
void CZPauseTest::Clear()
{
    data_00409080 = 0;
    data_00409070 = 0;
    for (unsigned int i = 0; i < 12; i++)
        m_items[(signed char)i].m_text.ClearAndWipe();
    m_texta90.ClearAndWipe();
    m_textb28.ClearAndWipe();
    m_text420.ClearAndWipe();
    m_textcf0.ClearAndWipe();
    m_textd88.ClearAndWipe();
    m_texte20.ClearAndWipe();
    m_texteb8.ClearAndWipe();
    m_textf50.ClearAndWipe();
    m_textfe8.ClearAndWipe();
    m_text1080.ClearAndWipe();
    m_text1220.ClearAndWipe();
    clear();
}

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
        m_items[(signed char)i].m_text.Draw(cam);
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
