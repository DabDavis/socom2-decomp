/* zanim_cmd_parse.cpp -- the ZAnim command table and the parsers' helpers (OVL1 0x25BC20-0x25D22F):
 * InitCommands registers the core commands, zAnimCmdAddData grows a command record, and a few of the
 * commands' handlers sit at the unit's end. The unit name is SOCOM 1's (its __sinit_zanim_cmd_parse.cpp
 * is the <iostream> pair alone, linked right before zanim_cmd_tick.cpp's, as SOCOM II's 0x3FFB00 is --
 * the integrator's initialiser research). Defined in reverse of their image order (-inline deferred);
 * not written yet: zAnimFireWeaponTick 0x25CCB0, 0x25CB00, 0x25C880, 0x25C6E0, the alpha expression
 * 0x25C5F0 (zanim_cmd_tick.cpp calls it) and InitCommands 0x25BC20. */

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
