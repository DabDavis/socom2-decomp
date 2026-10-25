/* hud_teammatenames.cpp -- the teammates' names (OVL1 0x226960-0x227337): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_teammatenames.cpp), SOCOM II's CMPTeammateNames: On, Tick,
 * WordIntersectionTest, UnInit and Init in the image. Not written yet: Tick; WordIntersectionTest
 * OPEN. Defined in reverse of their image order (-inline deferred). */

#include "hud_teammatenames.h"

/* C2DBitmap's placement by ints (0x364660; no row yet) */
extern "C" void func_00364660(C2DBitmap *bitmap, int x, int y, int w, int h);

/* the two bitmaps' sizes (variables: read with lwc1) */
static float s_bitmap1Size = 32.0f;
static float s_bitmap2Size = 32.0f;

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

/* whether a rectangle overlaps a string's: its corner the position (12.4 fixed point) moved by
 * its offset, as wide as the float at 0x3C (SOCOM 1 read m_length there) and as tall as its font.
 * OPEN, 3 instructions: the width's sum is (float)x1 + width in the target with the width loaded
 * first (f1); written `width + x1` the registers come right and the operands reversed, written
 * `x1 + width` the operands right and the registers swapped (the C2DString::Load pattern); and
 * the last test materialises into v0 where the target's goes through at (the negated test's
 * early return is the only form that gives the target's block order; build/scratch/hud/v_wit*.py) */
unsigned char WordIntersectionTest(int tx1, int ty1, int tx2, int ty2, C2DString *target)
{
    int x1 = (int)(target->m_posx + target->m_socom2_7c) >> 4;
    int y1 = (int)(target->m_posy + target->m_socom2_80) >> 4;
    int x2 = (int)(target->m_charheight + x1);
    int y2 = y1 + (target->m_font->m_displaybottom - target->m_font->m_displaytop);

    if (!(tx1 < x2 && x1 < tx2 && ty1 < y2 && y1 < ty2))
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
