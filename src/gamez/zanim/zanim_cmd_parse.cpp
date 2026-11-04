/* zanim_cmd_parse.cpp -- the ZAnim command table and the parsers' helpers (OVL1 0x25BC20-0x25D22F):
 * InitCommands registers the core commands, zAnimCmdAddData grows a command record, and a few of the
 * commands' handlers sit at the unit's end. The unit name is SOCOM 1's (its __sinit_zanim_cmd_parse.cpp
 * is the <iostream> pair alone, linked right before zanim_cmd_tick.cpp's, as SOCOM II's 0x3FFB00 is --
 * the integrator's initialiser research). Defined in reverse of their image order (-inline deferred);
 * not written yet: zAnimFireWeaponTick 0x25CCB0 and InitCommands 0x25BC20. Between them SOCOM II's
 * key-track evaluators (alpha, colour, four bytes) and the heading's direction (core round 8). */

#include <string.h>
#include <iostream>   /* (its pair is all __sinit_zanim_cmd_parse.cpp builds, 0x3FFB00) */

#include "zanim.h"
#include "../zsys/zsys.h"

/* the weapons' explosives (zweapon.h's CZWeapon: only the static the satchels' command calls) */
class CZWeapon {
public:
    static void RemoveExplosivesInRange(const CPnt3D *pos, float range);   /* 0x3CFFB0 */
};

/* REMOVE_SATCHELS's record (SOCOM II's; the names ours): a point and a range */
struct ZAnimRemoveSatchelsCmd {
    _zanim_cmd_hdr hdr;   // 0x0
    CPnt3D m_pos;   // 0x4
    float m_range;   // 0x10
};

/* REMOVE_SATCHELS's tick: every placed explosive within the range of the point removed (SOCOM II's
 * command, InitCommands' #57; the name ours, by the command's) */
bool zAnimRemoveSatchelsTick(_zanim_cmd_hdr *hdr, float *dT)
{
    ZAnimRemoveSatchelsCmd *cmd = (ZAnimRemoveSatchelsCmd *)hdr;

    CZWeapon::RemoveExplosivesInRange(&cmd->m_pos, cmd->m_range);
    return true;
}

/* DEBUG's tick: nothing (SOCOM 1's) */
bool zAnimDebugTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
}

/* a key of an expression track: a time and the value there (SOCOM II's tracks; the names ours) */
struct ZANIM_EXPR_KEY {
    float t;
    float v;
};

/* a key of a colour track: a time and the colour there (SOCOM II's; the name ours) */
struct ZANIM_COLOR_KEY {
    float t;
    PNT4D c;
};

/* a heading in degrees made a direction on the ground, pitched by the second angle over 90 degrees
 * and shortened by as much (SOCOM II's; the motion and call-anim begins' -- the name ours) */
void zAnimHeadingToDir(CPnt3D *dir, float heading, float pitch)
{
    float a = 0.017453292f * heading;

    dir->x = sinf(a);
    dir->z = cosf(a);
    dir->x = -dir->x;
    dir->y = 0.0f;
    dir->z = -dir->z;
    dir->y = 0.011111111f * pitch;

    float s = dir->y >= 0.0f ? 1.0f - dir->y : 1.0f + dir->y;

    dir->x *= s;
    dir->z *= s;
}

/* a colour track at a time made four bytes, 128 for 1, rounded: nothing at or before its first key,
 * else the colour interpolated between the keys about the time, the last key's past them (SOCOM
 * II's, the BLUR3D tick's amount; the name ours) */
void zAnimRGBAExpr(unsigned char *rgba, char *expr, int n, float t)
{
    ZANIM_COLOR_KEY *keys = (ZANIM_COLOR_KEY *)expr;

    if (t <= keys[0].t)
        return;

    PNT4D c;
    bool found = false;

    for (int i = 0; i < n - 1; i++) {
        bool in = t <= keys[i + 1].t;

        if (in) {
            float f = (t - keys[i].t) / (keys[i + 1].t - keys[i].t);

            c.x = keys[i].c.x + f * (keys[i + 1].c.x - keys[i].c.x);
            c.y = keys[i].c.y + f * (keys[i + 1].c.y - keys[i].c.y);
            c.z = keys[i].c.z + f * (keys[i + 1].c.z - keys[i].c.z);
            c.w = keys[i].c.w + f * (keys[i + 1].c.w - keys[i].c.w);
            found = true;
            break;
        }
    }
    if (!found)
        c = keys[n - 1].c;
    rgba[0] = (unsigned int)(0.5f + 128.0f * c.x);
    rgba[1] = (unsigned int)(0.5f + 128.0f * c.y);
    rgba[2] = (unsigned int)(0.5f + 128.0f * c.z);
    rgba[3] = (unsigned int)(0.5f + 128.0f * c.w);
}

/* a colour track at a time: before its first key the colour eased toward that key by the time
 * over the key's, else interpolated between the keys about the time, the last key's past them
 * (SOCOM II's, the TRUECOLOR tick's -- the channel it passes unread; the name ours) */
void zAnimColorExpr(PNT4D *color, char *expr, int n, int channel, float t)
{
    ZANIM_COLOR_KEY *keys = (ZANIM_COLOR_KEY *)expr;

    if (t < keys[0].t) {
        float f = t / keys[0].t;

        color->x = color->x + f * (keys[0].c.x - color->x);
        color->y = color->y + f * (keys[0].c.y - color->y);
        color->z = color->z + f * (keys[0].c.z - color->z);
        color->w = color->w + f * (keys[0].c.w - color->w);
        return;
    }

    bool found = false;

    for (int i = 0; i < n - 1; i++) {
        bool in = t <= keys[i + 1].t;

        if (in) {
            float f = (t - keys[i].t) / (keys[i + 1].t - keys[i].t);

            color->x = keys[i].c.x + f * (keys[i + 1].c.x - keys[i].c.x);
            color->y = keys[i].c.y + f * (keys[i + 1].c.y - keys[i].c.y);
            color->z = keys[i].c.z + f * (keys[i + 1].c.z - keys[i].c.z);
            color->w = keys[i].c.w + f * (keys[i + 1].c.w - keys[i].c.w);
            found = true;
            break;
        }
    }
    if (!found)
        *color = keys[n - 1].c;
}

/* Byte-exact: the key test a bool local (the target materialises it, li/move then xori, before
 * the branch; `if (t <= ...)` branches on the compare), the bound `n - 1` in the loop's test (a
 * hoisted temporary, coloured first: a3) */
/* an alpha track at a time: past its first key, the value interpolated between the keys about the
 * time (the last key's past them) made the FIX alpha, 128 for 1 and clamped to a byte (SOCOM II's,
 * the blend-mode, light and particle ticks' -- the name ours) */
void zAnimAlphaExpr(GsAlpha *alpha, char *expr, int n, float t)
{
    ZANIM_EXPR_KEY *keys = (ZANIM_EXPR_KEY *)expr;

    if (t <= keys[0].t)
        return;

    float v;
    bool found = false;

    for (int i = 0; i < n - 1; i++) {
        bool in = t <= keys[i + 1].t;

        if (in) {
            v = keys[i].v + (t - keys[i].t) / (keys[i + 1].t - keys[i].t) * (keys[i + 1].v - keys[i].v);
            found = true;
            break;
        }
    }
    if (!found)
        v = keys[n - 1].v;

    int a = (int)(128.0f * v);

    if (a < 0)
        a = 0;
    if (a > 255)
        a = 255;
    alpha->FIX = a;
}

/* the record grown by the bytes given, which are copied onto its end, its size updated: the record
 * it now is (SOCOM 1's; the parsers append their keys through it) */
_zanim_cmd_hdr *zAnimCmdAddData(_zanim_cmd_hdr *hdr, void *data, int size)
{
    int newsize = hdr->data_size + size;

    hdr = (_zanim_cmd_hdr *)_realloc(hdr, newsize, 0, 0);
    memcpy((char *)hdr + hdr->data_size, data, size);
    hdr->data_size = newsize;
    return hdr;
}
