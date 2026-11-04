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
