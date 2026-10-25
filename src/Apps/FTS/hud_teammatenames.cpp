/* hud_teammatenames.cpp -- the teammates' names (OVL1 0x226960-0x22755F): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_teammatenames.cpp), SOCOM II's CMPTeammateNames: On, Tick,
 * WordIntersectionTest, UnInit, Init and the font Init (CHUD::Init's) in the image. Not written yet: Tick. Defined in reverse of their image order (-inline deferred). */

#include "hud_teammatenames.h"

/* C2DBitmap's placement by ints (0x364660; no row yet) */
extern "C" void func_00364660(C2DBitmap *bitmap, int x, int y, int w, int h);

/* the two bitmaps' sizes (variables: read with lwc1), and their halves, which the unit's
 * initialiser (0x3FE940) computes and Tick reads (the names ours) */
static float s_bitmap1Size = 32.0f;
static float s_bitmap2Size = 32.0f;
static float s_bitmap1Half = 0.5f * s_bitmap1Size;
static float s_bitmap2Half = 0.5f * s_bitmap2Size;

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the HUD's phases (CHUD::Add2D) */
class CHUD {
public:
    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

/* a valve as the names read it: its value (zanim.h's CValve) */
enum VALVE_TYPE {};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* the names made: eight strings in the font, listed, and the two bitmaps (the ok-to-talk and the
 * off arrow textures) added to the HUD's phases off, grey and teal, half transparent; shown while
 * the DISPLAY_MP_TEAM_NAMES valve is on (CHUD::Init's; SOCOM 1's Init took the font too) */
void CMPTeammateNames::Init(C2DFont *font)
{
    m_socom2_5e0 = 0;
    for (unsigned int i = 0; i < 8; i++) {
        m_names[i].SetFont(font);
        m_names[i].m_scale = 0.9f;
        push_back(&m_names[i]);
    }
    m_bitmap1.Off();
    m_bitmap1.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap1.SetTrans(64.0f);
    m_bitmap1.m_pTexHandle = zdb::CAssetMgr::m_assets.GetTexHandle("action_ok_talk2.tif");
    func_00364660(&m_bitmap1, 0, 0, (int)s_bitmap1Size, (int)s_bitmap1Size);
    theHUD->Add2D(&m_bitmap1, -1, true);
    m_bitmap2.Off();
    m_bitmap2.SetColor(40.0f, 96.0f, 110.0f);
    m_bitmap2.SetTrans(64.0f);
    m_bitmap2.m_pTexHandle = zdb::CAssetMgr::m_assets.GetTexHandle("hud_arrow_off2.tif");
    func_00364660(&m_bitmap2, 0, 0, (int)s_bitmap2Size, (int)s_bitmap2Size);
    theHUD->Add2D(&m_bitmap2, -1, true);
    m_socom2_5e4 = CValve::Create("DISPLAY_MP_TEAM_NAMES", (VALVE_TYPE)1)->m_value != 0;
    theHUD->Add2D(this, 1, true);
}

/* both bitmaps off, grey and teal, half transparent, placed at the origin */
void CMPTeammateNames::Init()
{
    m_socom2_5e0 = 0;
    m_bitmap1.Off();
    m_bitmap1.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap1.SetTrans(64.0f);
    func_00364660(&m_bitmap1, 0, 0, (int)s_bitmap1Size, (int)s_bitmap1Size);
    m_bitmap2.Off();
    m_bitmap2.SetColor(40.0f, 96.0f, 110.0f);
    m_bitmap2.SetTrans(64.0f);
    func_00364660(&m_bitmap2, 0, 0, (int)s_bitmap2Size, (int)s_bitmap2Size);
}

/* the font as the test reads it: the glyphs' top and bottom (SOCOM 1's, 8 bytes on in SOCOM II) */
class C2DFont {
public:
    unsigned char _0[0x20];
    int m_displaytop;   // 0x20
    int m_displaybottom;   // 0x24
};

/* a string's width (the float at 0x3C, where SOCOM 1 read m_length): an inline accessor, which makes
 * the width the sum's right operand while it is loaded first, as the target has it */
static inline float StringWidth(const C2DString *s)
{
    return s->m_charheight;
}

/* whether a rectangle overlaps a string's: its corner the position (12.4 fixed point) moved by
 * its offset, as wide as StringWidth and as tall as its font. The last test is written
 * `!(ty2 <= y1)`: as `y1 < ty2` MW materialises it into v0 where the target branches on at (the
 * negated test's early return is the only form that gives the target's block order) */
unsigned char WordIntersectionTest(int tx1, int ty1, int tx2, int ty2, C2DString *target)
{
    int x1 = (int)(target->m_posx + target->m_socom2_7c) >> 4;
    int y1 = (int)(target->m_posy + target->m_socom2_80) >> 4;
    int x2 = (int)(x1 + StringWidth(target));
    int y2 = y1 + (target->m_font->m_displaybottom - target->m_font->m_displaytop);

    if (!(tx1 < x2 && x1 < tx2 && ty1 < y2 && !(ty2 <= y1)))
        return false;
    return true;
}

/* every name off and emptied */
void CMPTeammateNames::UnInit()
{
    for (unsigned int i = 0; i < 8; i++) {
        m_names[i].Off();
        m_names[i].SetString(0, true);
    }
}

void CMPTeammateNames::On()
{
    if (m_socom2_5e4)
        C2D::On();
    else
        C2D::Off();
}
