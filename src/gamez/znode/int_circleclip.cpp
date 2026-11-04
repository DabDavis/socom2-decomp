/* int_circleclip.cpp -- zdb::CCircleClip, a circle in the xy plane clipping a segment: SOCOM 1's
 * class and function (its symbol table: CircleSegmentClip__Q23zdb11CCircleClipFP6CPnt4D x12, 0x420
 * bytes at the demo's 0x220170, right after int_material.cpp's tree instances, as here; the DWARF
 * gives the class's five floats, no file). One function, OVL1 0x2DF160-0x2DF553: a unit of its own
 * between int_material.cpp (its last row the string compare 0x2DF030) and the lobby's
 * SyncGameClock (0x2DF560). The unit's name is ours. The only caller is the HUD map's line clip
 * (CZLineMap::Clip, 0x20D5F0), whose header (hud_newhudmap.h) keeps a global stand-in of the class
 * with the same five members. */

#include "../zmath/zmath.h"

extern "C" void sceVu0InterVector(float *v0, float *v1, float *v2, float r);

namespace zdb {

/* the clipping circle: its centre, the centre in the segment's frame (along it, and its distance
 * off it), the radius (SOCOM 1's DWARF, 0x14 bytes) */
class CCircleClip {
public:
    int CircleSegmentClip(CPnt4D *in, CPnt4D *out, CPnt4D *a, CPnt4D *b, CPnt4D *in_c, CPnt4D *out_c,
                          CPnt4D *a_c, CPnt4D *b_c, CPnt4D *in_t, CPnt4D *out_t, CPnt4D *a_t,
                          CPnt4D *b_t);

    float m_Cx;   // 0x0
    float m_Cy;   // 0x4
    float m_rCx;   // 0x8
    float m_rCy;   // 0xc
    float m_Radius;   // 0x10
};

/* the segment a-b clipped to the circle (only x and y read): 0 when it misses, 3 when both ends lie
 * inside, else the points where it crosses -- `in` (entering, 1), `out` (leaving, 2) or both (4) --
 * each moved onto the segment, with the two attributes interpolated there between the ends' (a
 * colour and a texture point, the HUD's; sceVu0InterVector gives b's at 1, a's at 0). Each crossing
 * is first kept as its distance along the segment in its x (SOCOM 1's). Byte-exact; what settled it:
 * each fraction a ternary (`len != 0 ? x / len : 0`: three nops after the div.s, where `t = 0; if
 * (len != 0) t = x / len;` pads only to the label's alignment), the tests written `0.0f < in->x` and
 * `len > out->x` (MW branches `c.lt.s 0,x`/`c.le.s len,x` as the target), and uy declared ahead of
 * ux but computed after it (ux f21, uy f22, the x difference loaded first) */
int CCircleClip::CircleSegmentClip(CPnt4D *in, CPnt4D *out, CPnt4D *a, CPnt4D *b, CPnt4D *in_c,
                                   CPnt4D *out_c, CPnt4D *a_c, CPnt4D *b_c, CPnt4D *in_t,
                                   CPnt4D *out_t, CPnt4D *a_t, CPnt4D *b_t)
{
    float uy;
    float ux = b->x - a->x;
    float dx;
    float dy;
    float l;
    float len;
    float rx;
    float ry;
    float h;
    float t;

    uy = b->y - a->y;
    dx = ux;
    dy = uy;
    l = sqrtf(ux * ux + uy * uy);
    if (l != 0.0f)
        l = 1.0f / l;
    ux *= l;
    uy *= l;
    len = dx * ux + dy * uy;
    rx = m_Cx - a->x;
    ry = m_Cy - a->y;
    m_rCx = rx * ux + ry * uy;
    m_rCy = ry * ux - rx * uy;
    if (m_rCy < 0.0f)
        m_rCy = -m_rCy;
    if (!(m_rCy <= m_Radius))
        return 0;
    h = sqrtf(m_Radius * m_Radius - m_rCy * m_rCy);
    out->x = m_rCx + h;
    if (out->x <= 0.0f)
        return 0;
    in->x = m_rCx - h;
    if (!(in->x < len))
        return 0;
    if (0.0f < in->x && len > out->x) {
        t = len != 0.0f ? in->x / len : 0.0f;
        sceVu0InterVector((float *)in_c, (float *)b_c, (float *)a_c, t);
        sceVu0InterVector((float *)in_t, (float *)b_t, (float *)a_t, t);
        in->y = a->y + in->x * uy;
        in->x = a->x + in->x * ux;
        t = len != 0.0f ? out->x / len : 0.0f;
        sceVu0InterVector((float *)out_c, (float *)b_c, (float *)a_c, t);
        sceVu0InterVector((float *)out_t, (float *)b_t, (float *)a_t, t);
        out->y = a->y + out->x * uy;
        out->x = a->x + out->x * ux;
        return 4;
    }
    if (!(0.0f < in->x) && !(len > out->x))
        return 3;
    if (0.0f < in->x) {
        t = len != 0.0f ? in->x / len : 0.0f;
        sceVu0InterVector((float *)in_c, (float *)b_c, (float *)a_c, t);
        sceVu0InterVector((float *)in_t, (float *)b_t, (float *)a_t, t);
        in->y = a->y + in->x * uy;
        in->x = a->x + in->x * ux;
        return 1;
    }
    t = len != 0.0f ? out->x / len : 0.0f;
    sceVu0InterVector((float *)out_c, (float *)b_c, (float *)a_c, t);
    sceVu0InterVector((float *)out_t, (float *)b_t, (float *)a_t, t);
    out->y = a->y + out->x * uy;
    out->x = a->x + out->x * ux;
    return 2;
}

}   // namespace zdb
