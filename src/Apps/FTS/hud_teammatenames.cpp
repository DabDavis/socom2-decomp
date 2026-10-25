/* hud_teammatenames.cpp -- the teammates' names (OVL1 0x226960-0x227337): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_teammatenames.cpp), SOCOM II's CMPTeammateNames: On, Tick,
 * WordIntersectionTest, UnInit and Init in the image. Not written yet: Tick and
 * WordIntersectionTest. Defined in reverse of their image order (-inline deferred). */

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
