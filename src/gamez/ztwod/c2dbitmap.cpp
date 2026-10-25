/* c2dbitmap.cpp -- C2DBitmap (OVL1 0x3641F0-0x364D5F): SOCOM 1's functions in SOCOM 1's order
 * (GetAssetLibs, TickFade, Reset, MakePacket, the two SetPos, the two Load, SetUV, GetTrans,
 * SetTrans, SetColor, the two Draw, the constructor), with SOCOM II's own among them: a colour
 * from a point and its getter, and the four flag setters that keep the GS PRIM bits (vtable slots
 * 6-9). The unit's name is ours; it ends where C2D's begins (ztwod.cpp). Written so far: the
 * small ones; defined in reverse of their image order (-inline deferred). */

#include "ztwod.h"

/* the texture a handle holds, as the bitmap reads it: its size two halfwords at 0 */
namespace zdb {
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};
}

/* slot 6: with a texture to draw, TME */
void C2DBitmap::SetHasTexture(bool on)
{
    m_hasTexture = on;
    if (on && m_pTexHandle)
        m_prim |= 0x10;
    else if (m_prim & 0x10)
        m_prim ^= 0x10;
}

/* slot 7: AA1 */
void C2DBitmap::SetAA(bool on)
{
    m_isAA = on;
    if (on)
        m_prim |= 0x80;
    else if (m_prim & 0x80)
        m_prim ^= 0x80;
}

/* slot 8: ABE */
void C2DBitmap::SetIsTrans(bool on)
{
    m_isTrans = on;
    if (on)
        m_prim |= 0x40;
    else if (m_prim & 0x40)
        m_prim ^= 0x40;
}

/* slot 9: IIP */
void C2DBitmap::SetGouraud(bool on)
{
    m_socom2_bit5 = on;
    if (on)
        m_prim |= 0x8;
    else if (m_prim & 0x8)
        m_prim ^= 0x8;
}

/* both rows' colour (SOCOM 1's) */
void C2DBitmap::SetColor(float r, float g, float b)
{
    m_RGB[0][0] = r;
    m_RGB[0][1] = g;
    m_RGB[0][2] = b;
    m_RGB[1][0] = r;
    m_RGB[1][1] = g;
    m_RGB[1][2] = b;
}

/* both rows' colour from a point, through the floats' (inlined: the components loaded first,
 * last to first; SOCOM II's, the name ours) */
void C2DBitmap::SetColor(const CPnt3D &color)
{
    SetColor(color.x, color.y, color.z);
}

/* the first row's colour (SOCOM II's; the name ours, as C2DPoly's) */
float *C2DBitmap::GetColor()
{
    return m_RGB[0];
}

/* slot 11: both rows' alpha; opaque (128) drops the blending, anything else keeps it */
void C2DBitmap::SetTrans(float trans)
{
    m_RGB[0][3] = trans;
    m_RGB[1][3] = trans;
    if (m_isTrans && trans == 128.0f) {
        m_isTrans = false;
        if (m_prim & 0x40)
            m_prim ^= 0x40;
    }
    if (!m_isTrans) {
        m_isTrans = true;
        m_prim |= 0x40;
    }
}

float C2DBitmap::GetTrans()
{
    return m_RGB[0][3];
}

/* placed at the point, as big as its texture (none: not moved) */
void C2DBitmap::SetPos(int x, int y)
{
    if (m_pTexHandle ? m_pTexHandle->m_texture : 0) {
        m_x = x;
        m_y = y;
        m_iWidth = m_pTexHandle->m_texture->m_width;
        m_iHeight = m_pTexHandle->m_texture->m_height;
    }
}

/* placed by its corners */
void C2DBitmap::SetPos(int x1, int y1, int x2, int y2)
{
    m_x = x1;
    m_y = y1;
    m_iWidth = x2 - x1;
    m_iHeight = y2 - y1;
}
