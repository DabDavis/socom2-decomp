/* seal_look.cpp -- where an AI SEAL looks: CSealCtrl's look and scan state (SOCOM 1 keeps these
 * functions -- ScanTick, ClearLook, SetLookDirR/W, SetLookPosW, SetDefaultLookParams and the scan
 * setters -- right after the hand signals, before seal_airesponse.cpp's states; this unit's name
 * is ours). OVL2 0x5FF200-0x60187F, so far only ClearLook. zseal.h's CSealCtrl (the SEAL lane's)
 * neither names the look members nor declares these functions, so they are spelled as
 * placeholders on a view of the controller. */

#include "zkit.h"

/* where a look comes from (SOCOM 1's DWARF, a signed byte; SOCOM II's SetDefaultLookParams takes
 * twelve kinds) */
enum AI_LOOK {
    AIL_ALL = -1, AIL_NORMAL = 0, AIL_TURNING = 1, AIL_CURIOUS = 2, AIL_EVENT = 3, AIL_CORPSE = 4,
    AIL_TASK = 5, AIL_FRANTIC = 6, AIL_TARGET = 7, AIL_BUDDY = 8, AIL_DEPLOY = 9, AIL_10 = 10,
    AIL_11 = 11
};

/* what the look's point is (SOCOM 1's DWARF, a byte) */
enum LOOK_DATA_TYPE { LOOK_GARBAGE = 0, LOOK_POSW = 1, LOOK_DIRW = 2, LOOK_DIRR = 3 };

/* the controller's look members (SOCOM 1's DWARF: CSealCtrl's m_look_timer at 0x14 on; SOCOM
 * II's sit 4 further on) */
struct CSealLook {
    unsigned char _0[0x18];
    float m_look_timer;   // 0x18
    float m_look_rate[2];   // 0x1c: an Rfloat
    AI_LOOK m_look_mode;   // 0x24
    LOOK_DATA_TYPE m_look_type;   // 0x25
    unsigned char _26[0x28 - 0x26];
    CPnt3D m_look_data;   // 0x28
    CPnt3D m_look_data_mdir;   // 0x34
    CPnt3D m_look_bias;   // 0x40
    unsigned int m_look_flags;   // 0x4c
};

extern "C" {
/* CSealCtrl::SetDefaultLookParams (SOCOM 1's SetDefaultLookParams__9CSealCtrlF7AI_LOOK, called
 * from the same ClearLook; SOCOM II's takes whether the look's point was set: OVL2 0x601400) */
void func_00601400(CSealCtrl *ctrl, AI_LOOK look, bool changed);
}

/* a look dropped -- this kind's, or any (AIL_ALL): straight ahead, no timer, no flags, the
 * defaults back (OVL2 0x600D40; SOCOM 1's CSealCtrl::ClearLook(AI_LOOK), the same stores 4
 * bytes on) */
extern "C" void func_00600D40(CSealCtrl *ctrl, AI_LOOK look)
{
    CSealLook *l = (CSealLook *)ctrl;

    if (look == AIL_ALL || look == l->m_look_mode) {
        bool changed = l->m_look_type != LOOK_DIRR;

        l->m_look_data = CPnt3D::unit_nz;
        l->m_look_type = LOOK_DIRR;
        l->m_look_mode = AIL_NORMAL;
        l->m_look_timer = CAiTimer::infinite;
        l->m_look_flags = 0;
        func_00601400(ctrl, AIL_NORMAL, changed);
    }
}
