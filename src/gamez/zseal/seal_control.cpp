/* seal_control.cpp -- the SEAL's controller, CSealCtrl: the pad's throttles, the trigger, the
 * rumble, the zoom, the menus (SOCOM 1's unit, __sinit_seal_control.cpp; SOCOM 1's order, the
 * MP bomb's helpers first and the constructor last). OVL2 0x5920A0-0x598840. Defined in reverse
 * of their image order (-inline deferred). */

#include "zseal.h"
#include "../zinput/zinput.h"
#include "../zanim/zanim.h"
#include "../zweapon/zweapon.h"
#include "../zsnd/zsnd.h"

extern "C" int rand(void);

extern "C" {
extern bool data_00650698;   /* the rumble switch (SOCOM 1's file static) */
extern float data_006505E8;  /* (the weapon's raise time a closed menu restarts) */
extern CNetGameState *data_00437CE8;
}

/* a vector's length squared on the ground plane (OVL2 0x5975B0: an inline's copy, before the
 * grenade's statics; PostTick and the kit call it) */
extern "C" float func_005975B0(const CPnt3D *v)
{
    return v->x * v->x + v->z * v->z;
}

/* points turned by a matrix's rotation, in place (OVL2 0x597530: an inline's copy, right after its
 * first caller CSealCtrl::PostTick -- zmath.h's TransformVectors with one array for both, so the
 * input cursor is a copy of the output's; the kit calls it too, 0x5B8920 and 0x5C1970) */
extern "C" void func_00597530(const CMatrix *m, CPnt3D *p, int count)
{
    TransformVectors(m, p, p, count);
}

extern "C" {
/* the controllers' switch (OVL1 data 0x3DF1B0, 1 to begin with: 0x598840 sets it, CTStopAll::Tick and
 * the mission cameras call that; PreTick, Tick and PostTick each do nothing while it is off -- the
 * name unknown) and a flag nothing sets (OVL2 bss 0x66B3E0, read only here: the look's tick below
 * never runs) */
extern bool data_003DF1B0;
extern bool data_0066B3E0;
/* (CSealCtrl::ScanTick -- SOCOM 1's ScanTick__9CSealCtrlFf, which its Tick calls in the same place;
 * OVL2 0x600550, the kit lane's seal_look.cpp range -- is zseal.h's, called by name) */
/* CZSealState::UpdateLocalSeal (SOCOM 1's UpdateLocalSeal__11CZSealStateFP10CZSealBody; OVL1
 * 0x2BD640), SOCOM II's taking a further argument */
void func_002BD640(CEntity *seal, int x);
}

/* the SEAL's state sent on and the bomb's wait run down; first, were the flag ever set, the look:
 * aiming, or neither standing nor crouched, toward -z, else turned with the yaw throttle (SOCOM 1's
 * Tick, the look kinds renumbered: its 5 and 1 are SOCOM II's 3 and 1), and the scan ticked (OVL2
 * 0x596F10, slot 0x10; SOCOM 1's with the switch, the flag, the wait and UpdateLocalSeal's argument
 * added) */
bool CSealCtrl::Tick(float dT)
{
    if (!data_003DF1B0)
        return false;
    bool turning;
    CZSealBody *seal = (CZSealBody *)m_entity;
    float yaw = m_throttle[2];

    if (data_0066B3E0) {
        static float lo = 0.0f;
        static float eps = 0.05f;

        turning = !_close_enough(yaw, lo, eps);
        bool aiming = seal->GetWeaponState() == 1;
        bool upright = seal->m_state == stateStand || seal->m_state == stateCrouch;

        if (aiming || !upright) {
            SetLookDirR(AIL_EVENT, &CPnt3D::unit_nz, true);
        } else {
            ClearLook(AIL_EVENT);
            if (turning) {
                static float scale = -1.5f;
                float a = scale * (m_scan_limits * yaw);
                CPnt3D dir;

                dir.x = sinf(a);
                dir.z = cosf(a);
                dir.z = -dir.z;
                dir.y = 0.0f;
                SetLookDirR(AIL_TURNING, &dir, true);
            } else {
                ClearLook(AIL_TURNING);
            }
        }
        ScanTick(dT);
    }
    func_002BD640(m_entity, 0);
    m_244 -= dT;
    return true;
}

/* the world's camera and its far plane (zdb::CWorld holds it at 0x128, the plane at 0xEC) */
struct CViewCamera {
    unsigned char _0[0xec];
    float m_far;   // 0xec

    /* the far plane (an inline accessor: SetViewRange's product loads it first, into $f1) */
    float GetFar() const { return m_far; }
};

struct CViewWorld {
    unsigned char _0[0x128];
    CViewCamera *m_camera;   // 0x128
};

/* the MP bomb's state machine (SOCOM 1's CZBombState, OVL1's): its status, the state first */
class CZBombState {
public:
    struct STATUS {
        unsigned char m_state;   // 0x0
        unsigned char _1[0x20 - 1];
    };

    static STATUS m_BombStatus;   // OVL1 bss 0x4413B0
};

extern "C" {
/* a pad button double-clicked and its state (OVL1 0x2C6490 -- CPad::GetDoubleClick through the
 * button map --, 0x2C64E0, unowned: the HUD's readers call the second the same way) */
unsigned char func_002C6490(int button, CPad *pad);
unsigned char func_002C64E0(int button, CPad *pad);
/* how long the button is held for the kit's fire-mode cycle, CZKit::CycleFireMode (OVL2 data: 0.4) */
extern float data_00650670;
}

namespace zdb {
/* the camera's collision bit demanded of the polygons a test hits (SOCOM 1's; grid_di.cpp's) */
void SetCameraCollideFlagRequired(bool on);
}

extern "C" {
/* (OVL2 0x5B4340, seal_env.cpp's, unwritten: a SEAL's test with a flag, 0x7FC bytes) */
bool func_005B4340(CZSealBody *seal, int b);
}

/* a polygon's camera type (DI_PARAMS's two bits at 18, as a byte enum: seal_body.cpp's) */
enum CAMERA_TYPE { CAMERA_TYPE_0, CAMERA_TYPE_1, CAMERA_TYPE_2, CAMERA_TYPE_3 };

static inline CAMERA_TYPE CameraType(zdb::IntersectStruct *hit)
{
    return (CAMERA_TYPE)hit->m_poly->m_flags.m_cameratype;
}

/* whether there is room to the side: a ray across the SEAL's frame at the root part's height (8 at
 * most), d to the left (-1) or right (1), with the SEAL's node switched off and camera-collision
 * polygons asked for -- clear unless it hits a polygon of a camera type other than 1; none when
 * 0x5B4340 refuses (OVL2 0x596D60, new in SOCOM II: PreTick asks it twice; the name unknown. The
 * shape is seal_body.cpp's head-room test 0x57F150) */
extern "C" bool func_00596D60(CSealCtrl *ctrl, signed char side, float d)
{
    zdb::IntersectStruct *hit;
    bool was;
    CZSealBody *seal = (CZSealBody *)ctrl->m_entity;

    if (!func_005B4340(seal, 1))
        return false;
    float h = seal->GetRootPart()->m_pos.y;

    if (h > 8.0f)
        h = 8.0f;
    CPnt3D from;

    /* (z zeroed first: `a = b = 0`, or the fields set z first -- the three-float constructor zeroes x
     * first) */
    from.x = from.z = 0.0f;
    from.y = h;
    CPnt3D to = from;

    if (side == -1)
        to.x -= d;
    else if (side == 1)
        to.x += d;
    seal->m_node->m_matrix.Transform(&from, 1);
    seal->m_node->m_matrix.Transform(&to, 1);
    was = seal->GetNode()->SetActive(false);
    zdb::SetCameraCollideFlagRequired(true);
    hit = zdb::CWorld::m_world->GetIntersect(&from, &to);
    seal->GetNode()->SetActive(was);
    if (hit && CameraType(hit) != CAMERA_TYPE_1) {
        zdb::SetCameraCollideFlagRequired(false);
        return false;
    }
    zdb::SetCameraCollideFlagRequired(false);
    return true;
}

/* the reload button: double-clicked, or held past the limit, reloads the SEAL's kit; while held its
 * time runs, else it is cleared (OVL2 0x594B70, new in SOCOM II: PreTick calls it; the name unknown).
 * The double-click's result taken into a byte local first (the flag's zero after the call) and the
 * held test stored `!= 0` (the target's second sltu) */
extern "C" void func_00594B70(CSealCtrl *ctrl, float dT)
{
    CPad *pad = ctrl->m_padid < CInput::NUM_PADS ? CInput::m_pads[ctrl->m_padid] : 0;
    unsigned char dbl = func_002C6490(3, pad);
    bool reload = false;

    if (dbl)
        reload = true;
    else {
        ctrl->m_bit18 = (func_002C64E0(3, pad) == 2) != 0;
        if (ctrl->m_bit18)
            ctrl->m_240 += dT;
        else
            ctrl->m_240 = 0.0f;
        if (!(ctrl->m_240 <= data_00650670)) {
            ctrl->m_240 = 0.0f;
            reload = true;
        }
    }
    if (reload)
        ((CZSealBody *)ctrl->m_entity)->m_kit.CycleFireMode();
}

extern "C" {
/* (OVL1, unowned: a SEAL's side in the game, both builds' net code; seal.cpp's too) */
bool func_002C30B0(const CZSealBody *seal);
/* the two teams' bomb sites (OVL1 bss, past theMission's mapped size; the names ours) */
extern zdb::CNode *data_00437DCC;
extern zdb::CNode *data_00437DD0;
}

/* the mission, only the time it has left (CMission::GetTimeLeft, OVL1 0x2A6AD0, on theMission, OVL1
 * bss 0x4364E0: fts_state.h's class, which this unit does not include) */
class CMission {
public:
    int GetTimeLeft();
};

extern CMission theMission;

extern "C" {
/* (OVL2 0x5A7DE0 is CZSealBody::CanFire, zseal.h's) the kit's wait before the next use (OVL2
 * 0x5C0AD0, the kit lane's) */
float func_005C0AD0(CZKit *kit);
}

/* whether the bomb can be planted at an action: the site the SEAL's side may use, time left online,
 * the controller's wait run out, the charge held with ammunition, room to kneel, the kit ready --
 * else the wait restarted (OVL2 0x594480; SOCOM 1's by order, CanPerformAction calls it). The flag
 * declared ahead of the SEAL (the SEAL takes s0, the flag s1) and cleared after the side's call */
bool CSealCtrl::CanPlaceBomb(CNodeAction *action)
{
    bool site;
    CZSealBody *seal = (CZSealBody *)m_entity;
    bool side = func_002C30B0(seal);

    site = false;

    if (side && action->m_node == data_00437DCC)
        site = true;
    if (!side && action->m_node == data_00437DD0)
        site = true;
    bool time = true;

    if (theNetwork.m_9 && (float)theMission.GetTimeLeft() <= 0.0f)
        time = false;
    if (m_244 <= 0.0f && seal && time && seal->m_kit.HasItem((EQUIP_ITEM)154) &&
        seal->m_kit.ItemHasAmmo((EQUIP_ITEM)154) && seal->CanFire(true, false) &&
        func_005C0AD0(&seal->m_kit) <= 0.0f && site)
        return true;
    if (m_244 <= 0.0f)
        m_244 = 1.5f;
    return false;
}

/* whether an action can be taken: none by the dead; the bomb's by its state (planting asks
 * CanPlaceBomb), a breach with a charge and a closed valve, a kind 5 never, any other always
 * (OVL2 0x594350; SOCOM 1's, the life test and kind 5 new). Byte-exact since round 50 (open since
 * round 13, 13 off): the target reads the bomb's state again for the 1-or-3 test, through its address
 * built in the first test's delay slot (lbu 0(a3)) -- a reference to the status taken at the case's
 * top (a reference or pointer to the state alike); taken in the else, the same reads are 0x134, and
 * read straight MW reuses the first test's load. */
static inline bool IsBombState(unsigned char state)
{
    return CZBombState::m_BombStatus.m_state == state;
}

/* a breach: a charge with ammunition in the kit and the door's valve closed */
static inline bool CanBreach(CZSealBody *seal, CValve *valve)
{
    if (seal && seal->m_kit.HasItem((EQUIP_ITEM)152) && seal->m_kit.ItemHasAmmo((EQUIP_ITEM)152) && valve &&
        valve->m_value == 0)
        return true;
    return false;
}

bool CSealCtrl::CanPerformAction(CNodeAction *action)
{
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (!seal->m_isAlive)
        return false;
    bool can = false;

    switch (action->m_type) {
    case 4: {
        const CZBombState::STATUS &st = CZBombState::m_BombStatus;

        if (IsBombState(2))
            can = CanPlaceBomb(action);
        else if (st.m_state == 1 || st.m_state == 3)
            can = false;
        break;
    }
    case 6:
        can = CanBreach(seal, action->m_valve);
        break;
    case 5:
        can = false;
        break;
    default:
        can = true;
        break;
    }
    return can;
}

/* a SEAL, or none (seal_ai.cpp's inlines alike) */
static inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

static inline bool AiDriven(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrl *)seal->m_control)->IsAi() : false;
}

static inline bool AiAwakeNow(const CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

/* online, a SEAL the game will not let another take (OVL1 0x2C2FD0; the HUD's commands and the
 * exit state ask it too) */
extern "C" bool func_002C2FD0(CZSealBody *seal);

/* the SEALs and the turret in reach (SOCOM 1's TickVictims, by its order and body; OVL2 0x5940D0):
 * among the targets within 30 the nearest living SEAL with the target's bit 9 set (the victim, while
 * none is held), the nearest body that can be carried, and the first turret -- an AI asleep passed
 * over, and online one the game keeps (SOCOM 1's took the first living SEAL and stopped, and the
 * first body; its reach a guarded static of 400) */
void CSealCtrl::TickVictims()
{
    static float range = 900.0f;
    CZSealBody *seal = (CZSealBody *)m_entity;
    CTargetList &targets = seal->m_targets;
    float nearest = -1.0f;
    float nearest_body = nearest;
    CTargetList::iterator i = targets.begin(), end = targets.end();

    m_victimbody = 0;
    m_corpsebody = 0;
    m_16c = 0;
    for (; i != end; ++i) {
        float d = i->m_distSq;

        if (!(d <= range))
            continue;
        CEntity *e = i->m_ent;
        CZSealBody *s = m_victimbody ? 0 : AsSeal(e);

        if (s) {
            if (AiDriven(s) && !AiAwakeNow(s->GetSealCtrlAi()))
                continue;
            if (theNetwork.m_9 && func_002C2FD0(s))
                continue;
            if (s->m_isAlive) {
                if (i->m_flags & 0x200) {
                    if (nearest == -1.0f || d < nearest) {
                        m_victimbody = s;
                        nearest = d;
                    }
                }
            } else if (!m_corpsebody && s->m_deathType != 6 && !s->m_fd1) {
                if (nearest_body == -1.0f || d < nearest_body) {
                    m_corpsebody = s;
                    nearest_body = d;
                }
            }
        } else if (!m_16c && e && e->m_type == CEntity::ENT_TYPE_TURRET)
            m_16c = e;
    }
}

extern "C" {
/* whether the SEAL is free to take a weapon (OVL2 0x5A81D0, seal_env.cpp's) */
bool func_005A81D0(CZSealBody *seal);
}

/* the pickups near (SOCOM 1's TickPickups, by its order and body; OVL2 0x593DA0): each ammunition
 * pickup within 20 picked up once (the last one kept in a function-local static), the weapon under
 * the reticule or near and its secondary's net ids while the weapon timer is out, and online the MP
 * bomb's node kept while within 15 across and 20 up, the bomb picked up when it is near and the kit
 * holds no charge */
void CSealCtrl::TickPickups(float dT)
{
    static CPickup *last = 0;
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (!seal)
        return;
    CPickup::iterator it = CPickup::GetFirst();
    CPickup *p;
    bool found = false;

    while ((p = CPickup::GetNext(it, &seal->m_posW, 20.0f, CPickup::PU_AMMO)) != 0) {
        if (p != last)
            seal->PickupObject(p);
        last = p;
        found = true;
    }
    if (!found)
        last = 0;

    CPickup *secondary = 0;
    CPickup *w = 0;

    if (func_005A81D0(seal)) {
        w = seal->GetReticuleWeapon(&secondary);
        if (!w)
            w = seal->GetProximityWeapon(&secondary);
    }
    if (!w)
        m_curpickup_ID = -1;
    else if (!(m_weapon_pickup_timer <= 0.0f)) {
        m_weapon_pickup_timer -= dT;
        m_curpickup_ID = -1;
    } else if (w->m_type == CPickup::PU_WEAPON) {
        if (!w->m_data)
            m_curpickup_ID = -1;
        else
            m_curpickup_ID = w->GetPickupNetEntityID();
    }
    if (!secondary)
        m_curpickup_secondary_ID = -1;
    else if (!(m_weapon_pickup_timer <= 0.0f)) {
        m_weapon_pickup_timer -= dT;
        m_curpickup_secondary_ID = -1;
    } else if (secondary->m_type == CPickup::PU_WEAPON) {
        if (!secondary->m_data)
            m_curpickup_secondary_ID = -1;
        else
            m_curpickup_secondary_ID = secondary->GetPickupNetEntityID();
    }
    if (!theNetwork.m_9)
        return;
    if (!(m_bombpickuptimer <= 0.0f)) {
        m_bombpickuptimer -= dT;
        m_curMPBOMBpickup_ID = -1;
        return;
    }
    zdb::CNode *bomb = CPickup::m_mpbombnode;

    if (bomb) {
        m_bomb_defusal_node = bomb;
        if (m_bomb_defusal_node) {
            static float across = 15.0f;
            static float up = 20.0f;
            CPnt3D d;

            ((const CPnt3D *)bomb->m_matrix.m_matrix[3])->SubXZ(&seal->m_posW, &d);
            if (d.x * d.x + d.z * d.z < across * across) {
                float dy = bomb->m_matrix.m_matrix[3][1] - seal->m_posW.y;

                if (!(dy * dy <= up * up))
                    m_bomb_defusal_node = 0;
            } else
                m_bomb_defusal_node = 0;
        }
    } else
        m_bomb_defusal_node = 0;
    CPickup *b = seal->GetProximityMPBomb();

    if (!b) {
        m_curMPBOMBpickup_ID = -1;
        return;
    }
    if (b->m_type == CPickup::PU_MPBOMB && m_curMPBOMBpickup_ID == -1) {
        if (seal->m_kit.HasItem((EQUIP_ITEM)154)) {
            m_curMPBOMBpickup_ID = -1;
            return;
        }
        seal->PickupObject(b);
        m_bombpickuptimer = 4.0f;
    }
}

/* the state a SEAL's controller sends over the network (seal.cpp's SEAL_NET_STATE: a word packed
 * through memcpy, its fourth byte the node's fade in fifteenths) */
struct SEAL_NET_STATE_CTRL {
    bool m_jump;   // 0x0
    unsigned char _1[2];
    unsigned char m_fade : 4;   // 0x3
    unsigned char m_bit4 : 1;
    unsigned char m_friend : 1;
    unsigned char m_bit6 : 1;
    unsigned char m_bit7 : 1;
};

extern "C" {
extern float floorf(float);
/* the game clock (OVL1 data 0x4365C0, the AI's: seconds), whether the spectators' menu is up (OVL1
 * 0x2C2FA0, the HUD's) and the kit's thrown items cleared (OVL2 0x5B88C0, the kit lane's) */
extern float data_004365C0;
bool func_002C2FA0();
void func_005B88C0(CZKit *kit);
}

/* a node's fade in fifteenths (seal.cpp's FadeLevel: 15 times the faded part when it is shown) */
static inline unsigned int NetFadeLevel(bool active, float opacity)
{
    return active ? (unsigned int)floorf(15.0f * (1.0f - opacity)) : 0;
}

/* the controller's SEAL (an inline: its result a temporary, numbered after the named locals -- the
 * SEAL in s2 under the menu byte and the node, PostMortemPreTick's registers by tools/igreplay.py) */
static inline CZSealBody *SealOf(CSealCtrl *c)
{
    return (CZSealBody *)c->m_entity;
}

/* the dead SEAL's controller tick: the throttles' tick, then online -- unless a multiplayer death is
 * pending a respawn (0xDC) out of the ghost and spectator modes -- the spectator's next and previous
 * asked by two buttons out of the menu; five seconds after the death the kit's thrown items cleared;
 * and, the action button pressed out of the menus, the node shown or hidden in turn, the fade sent
 * as the network state and a respawn asked once it was hidden; the SEAL's state sent (OVL2 0x592560,
 * slot 0x14; SOCOM 1's PostMortemPreTick ticked the throttles and read two buttons). Byte-exact (round
 * 49): the menu byte an int local (lbu, compared unmasked), the SEAL through SealOf */
bool CSealCtrl::PostMortemPreTick(float dT)
{
    CZSealBody *seal = SealOf(this);
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;
    zdb::CNode *node = seal->GetNode();
    bool action = false;

    ThrottlesPreTick(pad, dT);

    int menu = m_menustate;

    if (!menu)
        action = func_002C64E0(0, pad) == 1;
    if (theNetwork.m_9) {
        CNetGameState *game = data_00437CE8;
        float t = data_004365C0 - seal->m_time_of_death;

        if (!game->m_dc || game->m_ghost || func_002C2FA0()) {
            if (pad->m_state[2] == 3 && menu != 1)
                seal->m_spectate_next = 1;
            else
                seal->m_spectate_next = 0;
            if (pad->m_state[3] == 3 && menu != 1)
                seal->m_spectate_prev = 1;
            else
                seal->m_spectate_prev = 0;
        }
        if (t > 5.0f && game->m_dc)
            func_005B88C0(&seal->m_kit);
        if (action && !game->m_ghost && !func_002C2FA0() && game->m_dc && t > 5.0f) {
            float opacity = node->m_Opacity;
            bool hidden = opacity == 0.0f;
            bool show = !hidden;

            node->SetActive(show);

            SEAL_NET_STATE_CTRL st;
            unsigned int zero = 0;
            unsigned int state;

            memcpy(&st, &zero, sizeof(st));
            if (show && opacity > 0.0f)
                st.m_fade = NetFadeLevel(show, opacity);
            else
                st.m_fade = 15;
            memcpy(&state, &st, sizeof(st));
            SetAiNetState(state);
            if (hidden)
                seal->m_needs_respawn = 1;
        }
        func_002BD640(seal, 0);
    }
    return true;
}

/* the view range's floor (OVL2 data 0x6506A0, read as a variable) */
extern "C" float data_006506A0;

/* the view range: a given one held between the floor and the camera's reach (95 % of its far
 * plane, or 1000), none taking that reach (OVL2 0x5924B0; SOCOM 1's took any positive range).
 * The far plane through an inline accessor: read as a member, MW loads the constant first and
 * the registers swap (the factor on either side, casts, a local, `*=` all missed by 3) */
void CSealCtrl::SetViewRange(float range)
{
    float far = 1000.0f;
    CViewCamera *cam = ((CViewWorld *)zdb::CWorld::m_world)->m_camera;

    if (cam)
        far = 0.95f * cam->GetFar();
    if (range <= 0.0f)
        range = far;
    else
        _saturate(&range, data_006506A0, far);
    m_visrange.m_goal = m_visrange.m_value = range;
    m_aiParams.visual_range = range;
}

/* the cross product through VU0, normalised when asked (OVL2 0x592400, SOCOM 1's: after its
 * first caller, PerturbAimVector, which calls it out of line -- pinned; zmath.h declares it) */
#pragma auto_inline off
void CPnt3D::Cross(const CPnt3D *b, CPnt3D *out, bool normalize) const
{
    sceVu0FVECTOR r;
    sceVu0FVECTOR va;
    sceVu0FVECTOR vb;

    va[0] = x;
    va[1] = y;
    va[2] = z;
    va[3] = 1.0f;
    vb[0] = b->x;
    vb[1] = b->y;
    vb[2] = b->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    if (normalize)
        sceVu0Normalize(r, r);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
}
#pragma auto_inline reset

extern "C" {
/* OVL2's out-of-line copies of CPnt3D's constructors, its assignment and its operators (0x5414F0,
 * 0x516900, 0x539E90; seal_ai.cpp's 0x557E10 and 0x557DC0): the target calls each of them here,
 * the products and sums through their return slots */
CPnt3D *func_005414F0(CPnt3D *p, float x, float y, float z);
CPnt3D *func_00539E90(CPnt3D *p, const CPnt3D *src);
void __ml__6CPnt3DCFf(CPnt3D *ret, const CPnt3D *p, float s);
void __pl__6CPnt3DCFRC6CPnt3D(CPnt3D *ret, const CPnt3D *p, const CPnt3D *b);
}

/* a random value in [-1, 1], its square keeping the sign (an inline: the two draws expand alike;
 * the square assigned back in each arm -- a ternary or a result local of its own if-converts) */
static inline float SignedSquareRand()
{
    float r = 9.313226e-10f * (float)(rand() - 0x3fffffff);

    if (r < 0.0f)
        r = -(r * r);
    else
        r = r * r;
    return r;
}

/* the aim thrown off by the SEAL's accuracy: moved sideways and up by its factors, each scaled by a
 * random amount (OVL2 0x592260, slot 0x74; SOCOM 1's, the same arithmetic) */
void CSealCtrl::PerturbAimVector(CPnt3D *v)
{
    CAccuracy *acc = &((CZSealBody *)m_entity)->m_accuracy;
    CPnt3D up;
    CPnt3D side;
    CPnt3D vert;
    CPnt3D t1;
    CPnt3D t2;
    CPnt3D t4;
    CPnt3D t3;

    func_005414F0(&up, 0.0f, 1.0f, 0.0f);
    func_00516900(&side);
    func_00516900(&vert);
    v->Cross(&up, &side, false);
    side.Cross(v, &vert, false);
    float r = SignedSquareRand();

    __ml__6CPnt3DCFf(&t1, &side, acc->m_factorX + acc->m_factor * r);
    func_00539E90(&side, &t1);
    r = SignedSquareRand();
    __ml__6CPnt3DCFf(&t2, &vert, acc->m_factorY + acc->m_factor * r);
    func_00539E90(&vert, &t2);
    __pl__6CPnt3DCFRC6CPnt3D(&t3, v, &side);
    __pl__6CPnt3DCFRC6CPnt3D(&t4, &t3, &vert);
    func_00539E90(v, &t4);
}

/* a grenade's flight time and launch speed; SOCOM 1's function-local statics, which MW 3.0 lays
 * out initialised, with no guard (OVL2 0x5976E0, 0x5976A0, 0x5975D0) */
float CDynGrenade::ComputeMaxVel(float dist, float height, float *vel)
{
    static float k_rise = 1.0f;
    static float k_cos = 0.707107f;
    float g = -98.0f;
    float v = sqrtf(2.0f * (height + k_rise * dist) / -g);
    float t = dist / (k_cos * v);

    if (vel)
        *vel = v;
    return t;
}

/* the elevation offset: a blend of the two statics, made radians by a scaling statement of its
 * own -- `d *= k;` is the only form that madds the blend into $f0 and loads pi/180 into $f1 after
 * the mula; `return blend * k`, an inline DegToRad, a lerp inline or out of line, a local factor
 * all give SOCOM 1's registers (probes: build/scratch/seal/elev.spec, v7). */
float CDynGrenade::ComputeElevOfs(float t)
{
    static float lo = 10.0f;
    static float hi = 12.0f;
    float d = lo * (1.0f - t) + hi * t;

    d *= 0.017453292f;
    return d;
}

float CDynGrenade::ComputeTimeToImpact(float scale, float b, float c)
{
    float t0, t1;

    return scale * (ai::quadratic(-49.0f, b, c, t0, t1) ? ai::_max(t0, t1) : 0.0f);
}

/* (OVL2 0x598220) */
CSealCtrl::~CSealCtrl()
{
}

/* a projectile's event: a shot of direct fire or a rocket (item 185) -- not one flown out (flag 3) --
 * passing the SEAL: within 70 an AI event 9 for its shooter; a rocket within 100 its pass sound at the
 * nearest point, once (the projectile's flag 20), any other shot within 20 the bullet's (OVL2 0x598000;
 * SOCOM 1's with the rocket, the position read straight where SOCOM 1's asked a virtual, event 9 for 12).
 * The hit an unsigned char: the image narrows DistToVector's answer with `andi 0xff`, a bool's, where
 * zmath.h declares it int (a bool local converts with sltu) */
void CSealCtrl::OnWeaponEvent(CZProjectile *proj, unsigned int event, void *data)
{
    CZWeapon *weapon = proj->m_weapon;
    bool rocket = weapon ? weapon->GetIDType() == (EQUIP_ITEM)185 : false;

    if (weapon && (weapon->IsDirectFire() || rocket) && !proj->m_useflyout) {
        const CPnt3D *pos = &m_entity->m_posW;
        CPnt3D closest;
        float dist = 999999.0f;
        unsigned char hit = pos->DistToVector(&proj->m_framestartpos, &proj->m_frameendpos, &closest, &dist);

        if (hit && dist < 70.0f)
            CreateAiEvent((CAiEvent::EVENT)9, 0.5f, 1500.0f, false, pos, proj->m_shooterID);
        if (rocket) {
            if (hit && dist < 100.0f && !proj->m_b20) {
                CSnd *snd = CZWeapon::GetRocketPassSound();

                if (snd)
                    snd->PlaySound3D(&closest, 1.0f, 0, 0, 255);
                proj->SetFlag20(true);
            }
        } else if (hit && dist < 20.0f) {
            CSnd *snd = CZWeapon::GetBulletPassSound();

            if (snd)
                snd->PlaySound3D(&closest, 1.0f, 0, 0, 255);
        }
    }
}

/* the mission begun: online, the squad told to take up its places (unless the player is a
 * ghost); the body crouched and the controls given (OVL2 0x597F50; SOCOM 1's did nothing) */
void CSealCtrl::OnMissionStart()
{
    if (theNetwork.m_9) {
        ClearGoal(GOAL_NONE);
        CSealUnit *unit = CSealUnit::GetUnitByTeam(24);

        if (unit && !data_00437CE8->m_ghost)
            unit->IssueCommand((CZSealBody *)m_entity, (FT_COMMAND)35, 0, false);
    }
    ((CZSealBody *)m_entity)->m_desiredState = stateCrouch;
    m_input_enabled = true;
}

/* the mission over: no goal, no controls, the motors stopped (OVL2 0x597EB0; SOCOM 1's did
 * nothing) */
void CSealCtrl::OnMissionEnd()
{
    ClearGoal(GOAL_NONE);
    m_input_enabled = false;
    StopRumbler();
}

/* how far the SEAL may face a flash and not be blinded through its eyes (SOCOM 1's FLASHBANG_DOT, OVL2
 * data 0x65E640: 0 here, a variable read with lwc1) */
float FLASHBANG_DOT = 0.0f;

/* the square of the distance between two points (an inline: its difference a local of its own, below
 * the direction's temporary on the stack) */
static inline float DistSq(const CPnt3D *a, const CPnt3D *b)
{
    CPnt3D v;

    a->Sub(b, &v);
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

/* a stun taken: a flash (stun type 0) from a point within 150 blinds the SEAL by how squarely it faced
 * it and how near it was -- full past 80, falling off to 150 -- the blindplayer01-03 anim played (OVL2
 * 0x597C00; SOCOM 1's with the HUD's flash and the sounds' dip replaced by the anims, the position read
 * straight) */
void CSealCtrl::OnStunDamage(STUN_TYPE type, float amount, const CPnt3D *from)
{
    if (!from)
        return;
    if (type != STUN_NONE)
        return;
    const CPnt3D *pos = &m_entity->m_posW;
    const CPnt3D *fwd = (const CPnt3D *)m_entity->m_node->m_matrix.m_matrix[2];
    CPnt3D dir = *pos - *from;

    dir.Normalize();
    float dot = fwd->x * dir.x + fwd->y * dir.y + fwd->z * dir.z;
    float d2 = DistSq(pos, from);
    bool facing = dot > FLASHBANG_DOT;

    if (d2 < 22500.0f) {
        float f = 1.0f;

        if (!(d2 <= 6400.0f))
            f = 1.0f - 6.2e-5f * (d2 - 6400.0f);
        float s = 1.0f - dot * f;
        int level;
        bool full = false;

        /* (full: facing away, or hit by a shot flagged 19 -- written here, not an inline helper: as one
         * its result and the entity swap their scratch registers) */
        if (m_entity) {
            CZProjectile *p = ((CZSealBody *)m_entity)->m_tempprojectile;

            if (!facing || (p && p->m_b19))
                full = true;
        }
        if (full)
            level = 1;
        else if (s < 0.3f)
            level = 3;
        else if (s < 0.6f)
            level = 2;
        else
            level = 1;
        char name[16];

        sprintf(name, "blindplayer0%d", level);
        CZAnim *anim = ZAnim.GetAnim(name, 0);

        if (anim)
            anim->Activate(0);
    }
}

extern "C" {
/* (OVL1 0x30E6E0, unowned: told of a death online, with the global at 0x45B718) */
void func_0030E6E0(void *obj, int a);
extern void *data_0045B718;
/* (OVL2 0x552780, seal.cpp's: the body told something with two times) */
void func_00552780(CZSealBody *seal, int a, float b, float c);
}

/* dead: the squad told, the death cry (none for a ghost), the restraint undone; online, the squad
 * ordered on unless the game holds the body back, which then waits (OVL2 0x5979A0; SOCOM 1's with
 * SOCOM II's network ending) */
void CSealCtrl::OnDeath()
{
    m_unit->Notify((CZSealBody *)m_entity, UNIT_MESSAGE_DEATH, 0);
    switch (((CZSealBody *)m_entity)->m_deathType) {
    case DEATH_KNIFE_KILL:
    case DEATH_CQB:
    case DEATH_HEADSHOT:
        PlayAiSound((CHARACTER_SOUND)61, 1.0f, CAiEvent::AIE_NONE);
        break;
    case DEATH_GHOST:
        break;
    default:
        PlayAiSound((CHARACTER_SOUND)60, 1.0f, CAiEvent::AIE_NONE);
        break;
    }
    CZSealBody *seal = (CZSealBody *)m_entity;

    seal->SetRestrained(false);
    if (theNetwork.m_9) {
        if (!data_00437CE8->m_dc || data_00437CE8->m_ghost) {
            func_0030E6E0(data_0045B718, 0);
            CSealUnit::GetUnitByTeam(24)->IssueCommand(seal, (FT_COMMAND)34, 0, false);
        } else
            func_00552780(seal, 2, 0.0f, 0.1f);
    }
}

/* a random value in a range (SOCOM 1's Rfloat; seal_ai.cpp holds its operator's out-of-line copy) */
class Rfloat {
public:
    Rfloat(float min, float range) : m_min(min), m_range(range) {}
    operator float() const
    {
        if (m_range == 0.0f)
            return m_min;
        float r = 4.656613e-10f * (float)rand();

        return m_min + m_range * r;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* whether a chance comes up (an inline: seal_ai.cpp holds its out-of-line copy, 0x5608B0) */
static inline bool Chance(float chance)
{
    const float &c = chance;

    return Rfloat(0.0f, 1.0f) <= c;
}

extern "C" {
/* the voice's AI event made where the SEAL stands (OVL2 0x55FEE0, seal_ai.cpp's) */
CAiEvent *func_0055FEE0(CSealCtrl *ctrl, CAiEvent::EVENT event);
}

/* a sound of the SEAL's character played where it stands, if the chance comes up, and its AI
 * event made; whether it played (OVL2 0x5978A0; SOCOM 1's played it always) */
bool CSealCtrl::PlayAiSound(CHARACTER_SOUND sound, float chance, CAiEvent::EVENT event)
{
    bool played;
    CZSealBody *seal = (CZSealBody *)m_entity;
    bool go = true;

    played = false;
    if (chance < 1.0f)
        go = Chance(chance);
    if (go) {
        played = seal->GetCharacter()->PlayAt(sound, seal->m_id, &seal->m_posW);
        func_0055FEE0(this, event);
    }
    return played;
}

/* hurt: the pain cry, and the pad shaken unless the SEAL is a remote one (OVL2 0x597B00) */
void CSealCtrl::OnSetHealth(float health, float old)
{
    PlayAiSound((CHARACTER_SOUND)59, 1.0f, CAiEvent::AIE_NONE);
    if (IsRemote())
        return;
    SetLowRumbleTime(0.18f, 175.0f);
    SetHighRumbleTime(0.04f);
}

/* the trigger stepped with its pressure (OVL2 0x597810) */
void CSealCtrl::SetTrigger(bool down, float power)
{
    m_trigger_power = power;
    SetTrigger(down);
}

/* the trigger's edge state stepped by the button (OVL2 0x597790) */
void CSealCtrl::SetTrigger(bool down)
{
    TRIGGER t = m_trigger;

    switch (m_trigger) {
    case TRIGGER_UP:
        t = down ? TRIGGER_FALLING : TRIGGER_UP;
        break;
    case TRIGGER_DOWN:
        t = down ? TRIGGER_DOWN : TRIGGER_RISING;
        break;
    case TRIGGER_FALLING:
        t = down ? TRIGGER_DOWN : TRIGGER_RISING;
        break;
    case TRIGGER_RISING:
        t = down ? TRIGGER_FALLING : TRIGGER_UP;
        break;
    }
    m_trigger = t;
}

/* a menu opened or closed; closing them all restarts the weapon's raise unless told to keep it
 * (OVL2 0x597740: SOCOM II's second parameter) */
void CSealCtrl::SetMenuState(MENU_STATE state, bool keep)
{
    if (state < stateLast) {
        if (state != m_menustate && state == stateNone && !keep)
            ((CZSealBody *)m_entity)->m_wep_upT = data_006505E8;
        m_menustate = state;
    }
}

/* the reticle's actions: the squad asked whether orders 4 and 5 are allowed now, actions 14 and
 * 15 offered if so (OVL2 0x593D00; SOCOM 1's without its flag and the reticle seal's tail). The
 * squad declared first: the SEAL takes s0, the squad s1, as the target's. */
void CSealCtrl::PerformReticleActions()
{
    CSealUnit *unit;
    CZSealBody *seal = (CZSealBody *)m_entity;

    unit = CSealUnit::GetUnitByTeam(17);

    if (unit) {
        if (unit->AllowCommand(seal, (FT_COMMAND)4, 0))
            seal->ActionAvailableMask(14, true);
        if (unit->AllowCommand(seal, (FT_COMMAND)5, 0))
            seal->ActionAvailableMask(15, true);
    }
}

extern "C" {
/* the body's top blend (seal.cpp's tail copy, OVL2 0x555170), and two tests of the SEAL's anim
 * (seal_body.cpp's OVL2 0x587F50 and 0x587F10: anims 8 or 9, 10 or 11) */
void *func_00555170(void *stack);
bool func_00587F50(CZSealBody *seal);
bool func_00587F10(CZSealBody *seal);
}

CZSealBody *ftsGetPlayer();

/* the body's top blend (an inline: the target makes the call ahead of the static's load and
 * keeps the blend's anim in v0; the name ours) */
/* whether the SEAL is in the air (an inline bool: the target materialises the bit before its
 * branch; the name ours) */
static inline bool Airborne(CZSealBody *seal)
{
    return seal->m_airborne ? true : false;
}

static inline CZBodyAnimBlend *TopBlend(CZSealBody *seal)
{
    return (CZBodyAnimBlend *)func_00555170(&seal->m_blends);
}

/* the actions the SEAL can take now (SOCOM 1's SampleActionAvailable, by its order and body; OVL2
 * 0x593500): none while the weapon comes up or a menu other than 3 is open, online none for a player
 * the game keeps; through a scope or binoculars none; an action pending or shown, a carried body set
 * down (state 9) unless the carry is being picked up, else the reticle's and the proximity's actions
 * and the climbs when the SEAL can fire, or the climbs alone in the air or the four anims -- and in
 * state 5 action 16 (SOCOM 1's ActionDisplayOn is the member read, its third action slot gone) */
void CSealCtrl::SampleActionAvailable()
{
    CZSealBody *seal = (CZSealBody *)m_entity;

    seal->m_action_available = 0;
    seal->m_action_selection = 0;
    if (!(seal->m_wep_upT <= 0.0f) || (m_menustate != 0 && m_menustate != 3))
        return;
    if (theNetwork.m_9 && func_002C2FD0(ftsGetPlayer()))
        return;
    bool goal = (bool)m_goal_dir_valid || (bool)m_goal_pos_valid;

    if (goal)
        m_action = false;
    bool action = m_action;
    CZKit *kit = &seal->m_kit;

    if (kit && kit->IsUsingScope() || kit->IsUsingBinocs())
        return;
    if (!action && !seal->m_action_display_on)
        return;
    if (seal->m_state == 9 && TopBlend(seal)->m_animType != AnimTypes::BuddyCarriedPickup &&
        TopBlend(seal)->m_animType != AnimTypes::BuddyCarryPickup)
        seal->ActionAvailableMask(9, true);
    else if (seal->CanFire(true, false)) {
        PerformReticleActions();
        PerformProximityActions();
        if (seal->CanClimbWall())
            seal->ActionAvailableMask(2, true);
        if (seal->CanClimbLadder())
            seal->ActionAvailableMask(2, true);
    } else if (Airborne(seal) || func_00587F50(seal) || func_00587F10(seal)) {
        if (seal->CanClimbWall())
            seal->ActionAvailableMask(2, true);
        if (seal->CanClimbLadder())
            seal->ActionAvailableMask(2, true);
        return;
    }
    if (seal->m_state == 5)
        seal->ActionAvailableMask(16, true);
}

/* the scope's zoom (OVL2 0x592C70, 0x592C30, 0x592C10, 0x592BF0) */
void CSealCtrl::SetCameraZoom(float level, float increment)
{
    m_zoomlevel = level;
    m_zoomincrement = increment;
    m_update_zoom = true;
}

bool CSealCtrl::UpdateHudWeapon()
{
    if (m_HudUpdateWeapon) {
        m_HudUpdateWeapon = false;
        return true;
    }
    return false;
}

float CSealCtrl::GetZoomLevel()
{
    m_update_zoom = false;
    return m_zoomlevel;
}

float CSealCtrl::GetZoomIncrement()
{
    m_update_zoom = false;
    return m_zoomincrement;
}

/* the rumble switch (OVL2 0x592BE0, 0x592BD0) */
void SetRumbleOn(bool on)
{
    data_00650698 = on;
}

bool IsRumbleOn()
{
    return data_00650698;
}

/* the motors: the strong one's time, the weak one's time and strength, kept at their longest
 * (OVL2 0x592BA0, 0x592B40) */
void CSealCtrl::SetHighRumbleTime(float t)
{
    if (data_00650698) {
        if (m_highrumbletime < t)
            m_highrumbletime = t;
    }
}

void CSealCtrl::SetLowRumbleTime(float t, float strength)
{
    if (data_00650698) {
        if (m_rumblestr < strength || m_rumbletime <= 0.0f)
            m_rumblestr = strength;
        if (m_rumbletime < t)
            m_rumbletime = t;
    }
}

/* the motors run down: the weak one at its strength while its time lasts, the strong one on or
 * off (OVL2 0x5929A0) */
void CSealCtrl::TickRumbler(float dT)
{
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;

    if (pad) {
        if (m_rumbletime > 0.0f) {
            pad->m_ActData[1] = m_rumblestr;
            m_rumbletime -= dT;
        } else {
            m_rumblestr = 0.0f;
            pad->m_ActData[1] = m_rumblestr;
        }
        if (m_highrumbletime > 0.0f) {
            pad->m_ActData[0] = 1;
            m_highrumbletime -= dT;
        } else
            pad->m_ActData[0] = 0;
    }
}

/* the motors stopped, the pad told at once, the input flushed and the heartbeat silenced
 * (OVL2 0x592AC0) */
void CSealCtrl::StopRumbler()
{
    CPad *pad = m_padid < CInput::NUM_PADS ? CInput::m_pads[m_padid] : 0;

    if (pad) {
        pad->m_ActData[1] = 0;
        pad->m_ActData[0] = 0;
        pad->Tick(0.0f);
    }
    m_highrumbletime = 0.0f;
    m_rumbletime = 0.0f;
    m_rumblestr = 0.0f;
    CInput::Flush();
    CZSealBody::StopAllHeartbeatSounds();
}

extern "C" {
/* the planting's second time (OVL2 data, read as a variable; SOCOM 1's was the constant 0.2) */
extern float data_00650678;
}

/* the timer's flag cleared (an inline, the name ours: the constant left unfolded before the store) */
inline void MPBomb_TimerEngaged(CSealCtrl *ctrl, bool on)
{
    ctrl->m_mpbomb_bTimerEngaged = on;
}

/* the MP bomb worked at: its timer runs while the reticule's action, the reticule's hit or the MP bomb
 * is the bomb's node; run out, a defusal under way is attempted once the bomb is planted (state 3),
 * else the charge is planted at a bomb action the controller can take -- not over a planted bomb --
 * and the bomb let go (OVL2 0x5920C0; SOCOM 1's TickMPBomb, its proximity search gone) */
void CSealCtrl::TickMPBomb(float dT)
{
    if (m_mpbomb_node) {
        CNodeAction *action = 0;
        CZSealBody *seal = (CZSealBody *)m_entity;

        action = seal->GetReticuleAction(true);
        zdb::CNode *bomb = CPickup::m_mpbombnode;

        if (action && m_mpbomb_node == action->m_node)
            m_mpbomb_timer -= dT;
        else if (m_mpbomb_node == seal->m_last_reticule.m_node)
            m_mpbomb_timer -= dT;
        else if (m_mpbomb_node == bomb)
            m_mpbomb_timer -= dT;
        if (m_mpbomb_timer < 0.0f) {
            if (m_bomb_defusal_node) {
                if (CZBombState::m_BombStatus.m_state == 3) {
                    m_bombpickuptimer = 1.0f;
                    seal->AttemptBombDefuse();
                    m_bomb_defusal_node = 0;
                }
            } else if (action && CanPerformAction(action) && action->m_type == NODEACTION_BOMB) {
                CZSealBody *body = (CZSealBody *)m_entity;

                if (CZBombState::m_BombStatus.m_state != 3) {
                    action->ExecuteAction(body, 0);
                    body->m_kit.HandlePlacingExlosive((EQUIP_ITEM)154, 1.0f, data_00650678);
                }
                MPBomb_TimerEngaged(this, false);
            }
            m_mpbomb_node = 0;
        }
    }
}

/* the MP bomb's timer offered (OVL2 0x5920A0) */
void CSealCtrl::MPBomb_TimerAvailable(bool on)
{
    m_mpbomb_bTimerAvailable = on;
}
