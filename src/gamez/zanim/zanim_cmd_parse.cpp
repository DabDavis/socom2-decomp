/* zanim_cmd_parse.cpp -- the ZAnim command table and the parsers' helpers (OVL1 0x25BC20-0x25D22F):
 * InitCommands registers the core commands, zAnimCmdAddData grows a command record, and a few of the
 * commands' handlers sit at the unit's end. The unit name is SOCOM 1's (its __sinit_zanim_cmd_parse.cpp
 * is the <iostream> pair alone, linked right before zanim_cmd_tick.cpp's, as SOCOM II's 0x3FFB00 is --
 * the integrator's initialiser research). Defined in reverse of their image order (-inline deferred);
 * not written yet: zAnimFireWeaponTick 0x25CCB0. After InitCommands SOCOM II's
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

/* the core commands' handlers (zanim_cmd_tick.cpp's, zanim_menu.cpp's and this unit's; the
 * placeholders the ticks not named yet) */
bool zAnimQuadAlignTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimIfTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimGoToEndIf(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimEndIfTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00263B30(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimNodeActiveTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimNodeRenderedTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimRangeTestTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimRandomWeightTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimFailTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimHealthTestTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimLODTestTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimLoopTick(_zanim_cmd_hdr *hdr, float *dT);
void zAnimWaitBegin(_zanim_cmd_hdr *hdr);
bool zAnimWaitTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectBlendModeTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectActiveStateTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectTranslateStateTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectRotateStateTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" void func_002634A0(_zanim_cmd_hdr *hdr);
extern "C" bool func_002633A0(_zanim_cmd_hdr *hdr, float *dT);
void zAnimObjectMotionBegin(_zanim_cmd_hdr *hdr);
bool zAnimObjectMotionTick(_zanim_cmd_hdr *hdr, float *dT);
void zAnimObjectMotionFromToBegin(_zanim_cmd_hdr *hdr);
bool zAnimObjectMotionFromToTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" void func_00261300(_zanim_cmd_hdr *hdr);
extern "C" bool func_00261080(_zanim_cmd_hdr *hdr, float *dT);
extern "C" void func_00260FB0(_zanim_cmd_hdr *hdr);
extern "C" bool func_00260B40(_zanim_cmd_hdr *hdr, float *dT);
void zAnimObjectOpacityFromToBegin(_zanim_cmd_hdr *hdr);
bool zAnimObjectOpacityFromToTick(_zanim_cmd_hdr *hdr, float *dT);
void zAnimMultiObjectMotionScriptBegin(_zanim_cmd_hdr *hdr);
bool zAnimMultiObjectMotionScriptTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimParticleSourceTick(_zanim_cmd_hdr *hdr, float *dT);
void zAnimCameraBegin(_zanim_cmd_hdr *hdr);
bool zAnimCameraTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimDestructorTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimSoundTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_002656F0(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimLightTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00264A40(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00264610(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00264580(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_002642F0(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00263F90(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_00263E20(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimWhileTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimEndWhileTick(_zanim_cmd_hdr *hdr, float *dT);
extern "C" void func_0025E3D0(_zanim_cmd_hdr *hdr);
extern "C" bool func_0025E360(_zanim_cmd_hdr *hdr, float *dT);
extern "C" bool func_0025E2D0(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimBreakTick(_zanim_cmd_hdr *hdr, float *dT);
void zAnimCallAnimBegin(_zanim_cmd_hdr *hdr);
bool zAnimCallAnimTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimStopAnimTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimPauseAnimTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimResumeAnimTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimInvalidateAnimTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimCallSequenceTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimStopSequenceTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimDebugTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectAddChildTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimObjectDeleteChildTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimMessageTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimTimerTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimFireWeaponTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimRemoveSatchelsTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimUIMenuTick(_zanim_cmd_hdr *hdr, float *dT);
bool zAnimUIAppMenuTick(_zanim_cmd_hdr *hdr, float *dT);

/* the core commands registered, the ids of the flow commands kept in ZAnim (SOCOM 1's, eleven
 * more: OBJECT_MAKE_WORLDCHILD, the ripples, the colour, blur and iris effects, the shadow map's
 * lighting, FIRE_WEAPON, REMOVE_SATCHELS, the UI's two) */
bool CZAnimMain::InitCommands()
{
    ZAnim.AddCmd("QUAD_ALIGN", 0, 0, zAnimQuadAlignTick, 0);
    ZAnim.m_cmd_if.data_type = ZAnim.AddCmd("IF", 0, 0, zAnimIfTick, 0);
    ZAnim.m_cmd_elseif.data_type = ZAnim.AddCmd("ELSEIF", 0, 0, zAnimGoToEndIf, 0);
    ZAnim.m_cmd_else.data_type = ZAnim.AddCmd("ELSE", 0, 0, zAnimGoToEndIf, 0);
    ZAnim.m_cmd_endif.data_type = ZAnim.AddCmd("ENDIF", 0, 0, zAnimEndIfTick, 0);
    ZAnim.AddCmd("OBJECT_MAKE_WORLDCHILD", 0, 0, func_00263B30, 0);
    ZAnim.AddCmd("NODE_ACTIVE", 0, 0, zAnimNodeActiveTick, 0);
    ZAnim.AddCmd("NODE_RENDERED", 0, 0, zAnimNodeRenderedTick, 0);
    ZAnim.AddCmd("RANGE_TEST", 0, 0, zAnimRangeTestTick, 0);
    ZAnim.AddCmd("RANDOM_WEIGHT", 0, 0, zAnimRandomWeightTick, 0);
    ZAnim.AddCmd("FAIL", 0, 0, zAnimFailTick, 0);
    ZAnim.AddCmd("ANIM_HEALTH", 0, 0, zAnimHealthTestTick, 0);
    ZAnim.AddCmd("ANIM_LOD", 0, 0, zAnimLODTestTick, 0);
    ZAnim.AddCmd("LOOP", 0, 0, zAnimLoopTick, 0);
    ZAnim.AddCmd("WAIT", 0, zAnimWaitBegin, zAnimWaitTick, 0);
    ZAnim.AddCmd("OBJECT_BLENDMODE", 0, 0, zAnimObjectBlendModeTick, 0);
    ZAnim.AddCmd("OBJECT_ACTIVE_STATE", 0, 0, zAnimObjectActiveStateTick, 0);
    ZAnim.AddCmd("OBJECT_TRANSLATE_STATE", 0, 0, zAnimObjectTranslateStateTick, 0);
    ZAnim.AddCmd("OBJECT_ROTATE_STATE", 0, 0, zAnimObjectRotateStateTick, 0);
    ZAnim.AddCmd("OBJECT_AIM", 0, func_002634A0, func_002633A0, 0);
    ZAnim.AddCmd("OBJECT_MOTION", 0, zAnimObjectMotionBegin, zAnimObjectMotionTick, 0);
    ZAnim.AddCmd("OBJECT_MOTION_FROM_TO", 0, zAnimObjectMotionFromToBegin, zAnimObjectMotionFromToTick, 0);
    ZAnim.AddCmd("WHEEL_MOTION", 0, func_00261300, func_00261080, 0);
    ZAnim.AddCmd("SCROLL_MOTION", 0, func_00260FB0, func_00260B40, 0);
    ZAnim.AddCmd("OBJECT_OPACITY_FROM_TO", 0, zAnimObjectOpacityFromToBegin, zAnimObjectOpacityFromToTick, 0);
    ZAnim.AddCmd("OBJECT_MOTION_SI_SCRIPT", 0, zAnimMultiObjectMotionScriptBegin, zAnimMultiObjectMotionScriptTick, 0);
    ZAnim.AddCmd("PARTICLE_SOURCE", 0, 0, zAnimParticleSourceTick, 0);
    ZAnim.AddCmd("CAMERA", 0, zAnimCameraBegin, zAnimCameraTick, 0);
    ZAnim.AddCmd("DESTRUCTION_SOURCE", 0, 0, zAnimDestructorTick, 0);
    ZAnim.AddCmd("SOUND", 0, 0, zAnimSoundTick, 0);
    ZAnim.AddCmd("UPDATE_SHADOW_MAP_LIGHTING", 0, 0, func_002656F0, 0);
    ZAnim.AddCmd("LIGHT", 0, 0, zAnimLightTick, 0);
    ZAnim.AddCmd("BLUR3D", 0, 0, func_00264A40, 0);
    ZAnim.AddCmd("IRIS_EFFECT", 0, 0, func_00264610, 0);
    ZAnim.AddCmd("SCALE_COLOR", 0, 0, func_00264580, 0);
    ZAnim.AddCmd("TRUE_COLOR_SCALE", 0, 0, func_002642F0, 0);
    ZAnim.AddCmd("RIPPLE", 0, 0, func_00263F90, 0);
    ZAnim.AddCmd("RIPPLE_NODE_GROUP", 0, 0, func_00263E20, 0);
    ZAnim.AddCmd("WHILE", 0, 0, zAnimWhileTick, 0);
    ZAnim.m_cmd_endwhile.data_type = ZAnim.AddCmd("END_WHILE", 0, 0, zAnimEndWhileTick, 0);
    ZAnim.AddCmd("FOR_CHILDREN", 0, func_0025E3D0, func_0025E360, 0);
    ZAnim.AddCmd("END_FOR_CHILDREN", 0, 0, func_0025E2D0, 0);
    ZAnim.m_cmd_ExprOperator.data_type = ZAnim.AddCmd("EXPRESSION", 0, 0, 0, 0);
    ZAnim.AddCmd("BREAK", 0, 0, zAnimBreakTick, 0);
    ZAnim.AddCmd("CALL_ANIMATION", 0, zAnimCallAnimBegin, zAnimCallAnimTick, 0);
    ZAnim.AddCmd("STOP_ANIMATION", 0, 0, zAnimStopAnimTick, 0);
    ZAnim.AddCmd("PAUSE_ANIMATION", 0, 0, zAnimPauseAnimTick, 0);
    ZAnim.AddCmd("RESUME_ANIMATION", 0, 0, zAnimResumeAnimTick, 0);
    ZAnim.AddCmd("INVALIDATE_ANIMATION", 0, 0, zAnimInvalidateAnimTick, 0);
    ZAnim.AddCmd("CALL_SEQUENCE", 0, 0, zAnimCallSequenceTick, 0);
    ZAnim.AddCmd("STOP_SEQUENCE", 0, 0, zAnimStopSequenceTick, 0);
    ZAnim.AddCmd("DEBUG", 0, 0, zAnimDebugTick, 0);
    ZAnim.AddCmd("OBJECT_ADD_CHILD", 0, 0, zAnimObjectAddChildTick, 0);
    ZAnim.AddCmd("OBJECT_DELETE_CHILD", 0, 0, zAnimObjectDeleteChildTick, 0);
    ZAnim.AddCmd("MESSAGE", 0, 0, zAnimMessageTick, 0);
    ZAnim.AddCmd("TIMER", 0, 0, zAnimTimerTick, 0);
    ZAnim.AddCmd("FIRE_WEAPON", 0, 0, zAnimFireWeaponTick, 0);
    ZAnim.AddCmd("REMOVE_SATCHELS", 0, 0, zAnimRemoveSatchelsTick, 0);
    ZAnim.AddCmd("ui::UI_COMMAND", 0, 0, zAnimUIMenuTick, 0);
    ZAnim.AddCmd("ui::UI_APP_COMMAND", 0, 0, zAnimUIAppMenuTick, 0);
    return true;
}

