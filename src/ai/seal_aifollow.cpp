/* seal_aifollow.cpp -- the follow state, CAiSFollow (SOCOM 1's unit, __sinit_seal_aifollow.cpp).
 * OVL2 0x5FADF0-0x5FE1EF: the destructor first, as SOCOM 1's. So far only the destructor and the
 * new slot +0x3C. Defined in reverse of their image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"

extern "C" float data_00650C38;   /* the follow's close range (a variable: read with lwc1) */

/* a cell's size in the world (OVL2 data 0x64FC60, 10.0: read as a variable) */
extern "C" float data_0064FC60;

/* a formation's slot as an offset in cells (OVL2 0x5EB910, seal_unit.cpp's range: the formation, the
 * slot, the offset written) */
extern "C" void func_005EB910(signed char formation, int slot, int *cells);

/* (field by field: the three-float constructor interleaves the two terms' products, the target
 * computes one after the other) */
inline void CAiSFollow::UpdateOffsetLength()
{
    CPnt3D v;

    v.x = m_5c[1] * data_0064FC60;
    v.y = 0.0f;
    v.z = -m_5c[0] * data_0064FC60;
    m_64 = v.Length();
}

/* the remote console's word at 0xC of its +4 set (OVL2 0x5E6AA0, a named draft in seal_unit.cpp's
 * range; the name ours) */
extern "C" bool func_005E6AA0(CEntityCtrl *rc);

/* (OVL2 0x5FCC40; SOCOM 1's, the followers counted where SOCOM 1 tested the current flag, and a
 * remote SEAL's console asked) */
void CZSealBody::InvalidateFollowParams()
{
    if (m_followers <= 0) {
        m_saved_formation_dir_is_valid = 0;
        if (!IsPlayer()) {
            bool keep = false;

            if (IsRemote()) {
                CEntityCtrl *rc = m_control->GetRemoteControl();

                keep = rc ? !func_005E6AA0(rc) : false;
            }
            if (!keep)
                SetSnakePathUpdate(false, 50);
        }
    }
    m_follow_params_are_current = 0;
}

extern "C" {
/* the follow's hide update (OVL2 0x5FDAD0, unwritten) */
void func_005FDAD0(CAiSFollow *f, CSealCtrlAi *ai);
}

/* two cells' x and y distances summed (CAiPlanner::dist's shape, unscaled; the name ours) */
static inline int FollowCellDist(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* (OVL2 0x5FC7D0: the SEAL's offset from the follow's point computed and never read) */
bool CAiSFollow::HideCheck(CSealCtrlAi *ai, bool b, bool hide, char *out)
{
    bool r = hide;
    CPnt3D d;

    ((const CPnt3D *)m_88)->Sub(&ai->m_entity->m_posW, &d);
    if (m_79 && !hide)
        m_79 = 0;
    if (!m_79 && hide)
        m_79 = 1;
    if (hide && !b) {
        if (FollowCellDist(*(const CAiMapLoc *)&m_c0, *(const CAiMapLoc *)&m_c8) > m_c4) {
            *out = 0;
            r = false;
        }
    }
    if (r)
        func_005FDAD0(this, ai);
    return r;
}

/* (OVL2 0x5FCD30) */
CZSealBody *CSealCtrlAi::GetFollowMaster(bool recurse)
{
    CZSealBody *master = m_follow_master;

    if (master && recurse) {
        CSealCtrlAi *ai = master->GetSealCtrlAi();

        if (ai && ai != this) {
            CZSealBody *top = ai->GetFollowMaster(recurse);

            if (top)
                master = top;
        }
    }
    return master;
}

/* (OVL2 0x5FCF10) */
bool CSealCtrlAi::SetFollowMaster(CZSealBody *master)
{
    bool ok = false;
    CZSealBody *self = (CZSealBody *)m_entity;

    if (m_follow_master) {
        m_follow_master->RemoveFollower();
        m_follow_master = 0;
    }
    if (!master || self != master && master->m_isAlive) {
        m_follow_master = master;
        if (m_follow_master) {
            m_follow_master->AddFollower();
            m_follow_master_id = m_follow_master->m_id;
        }
        ok = true;
    }
    if (!m_follow_master) {
        CAiState *s = FindAiState(AI_STATE_FOLLOW);
        CAiSFollow *f = s && s->m_state == AI_STATE_FOLLOW ? (CAiSFollow *)s : 0;

        if (f) {
            f->m_active = 0;
            f->m_master = 0;
        }
    }
    return ok;
}

/* (OVL2 0x5FD0E0; CTState::Tick calls it) */
void CAiSFollow::SetOffset(const int *cells)
{
    m_5c[0] = cells[0];
    m_5c[1] = cells[1];
    UpdateOffsetLength();
}

/* (OVL2 0x5FD050; CSealCtrlAi::OnCommand and the squad's formation, 0x5EB6D0, call it) */
void CAiSFollow::SetFormation(signed char formation, int slot)
{
    m_70 = slot;
    m_74 = formation;
    func_005EB910(m_74, m_70, m_5c);
    UpdateOffsetLength();
}

extern "C" {
/* the AI's check of an entity at a distance (OVL2 0x5FD150, this unit's, not written: the AI, the
 * entity, a flag, the distance; a bool) */
bool func_005FD150(CSealCtrlAi *ai, CEntity *e, bool b, float dist);
}

/* the AI's current state if it is a follow, else 0 (inline; the name ours) */
static inline const CAiSFollow *FollowState(const CSealCtrlAi *ai)
{
    const CAiState *s = ai->GetAiState();

    return s && s->m_state == AI_STATE_FOLLOW ? (const CAiSFollow *)s : 0;
}

/* the AI's mode, the follow's own when following (mode 6): 4, 3 or 2 by its two flags at 0x7C and
 * 0x7D (inline in 0x5FD620; the name ours) */
static inline AI_MODE FollowMode(const CSealCtrlAi *ai)
{
    AI_MODE m = ai->m_3f4;

    if (m == 6) {
        const CAiSFollow *f = FollowState(ai);

        if (f) {
            if (f->m_7c)
                m = (AI_MODE)4;
            else if (f->m_7d)
                m = (AI_MODE)3;
            else
                m = (AI_MODE)2;
        }
    }
    return m;
}

extern "C" {
/* the AI's follow step by mode (OVL2 0x5FD460, this unit's, not written: the AI, two values passed
 * through, the mode) */
void func_005FD460(CSealCtrlAi *ai, void *a, int b, AI_MODE mode);
}

/* (OVL2 0x5FD6F0; 0x556370 calls it -- the name unknown) */
extern "C" AI_MODE func_005FD6F0(const CSealCtrlAi *ai)
{
    return FollowMode(ai);
}

/* the follow step in the AI's mode (OVL2 0x5FD620; 0x556370 calls it -- the name unknown) */
extern "C" void func_005FD620(CSealCtrlAi *ai, void *a, int b)
{
    func_005FD460(ai, a, b, FollowMode(ai));
}

/* the AI's check of an entity (0x5FD150) at the entity's distance from the SEAL; true when there is
 * none (OVL2 0x5FD3E0: CSealCtrlAi::PreTick hands it the player; the name unknown) -- the SEAL taken
 * into a local first: the target loads it ahead of the entity's null test */
extern "C" bool func_005FD3E0(CSealCtrlAi *ai, CEntity *e)
{
    bool r = true;
    CEntity *me = ((CEntityCtrl *)ai)->m_entity;

    if (e) {
        CPnt3D d = me->m_posW - e->m_posW;

        r = func_005FD150(ai, e, true, d.Normalize());
    }
    return r;
}

/* (OVL2 0x5FDD00) */
bool CAiSFollow::Virtual3C()
{
    return m_f4 < data_00650C38 && (m_6c & 8) != 0;
}

/* a record whose first byte's low five bits are a kind (the follow's Tick and 0x5FD790 test it;
 * the names ours) */
struct CAiFollowKind {
    unsigned char m_kind : 5;
};

/* whether the kind is 2, 4, 5 or 12 (OVL2 0x5FC550; the name ours): a bit test written out -- MW
 * lowers a switch over those cases to a compare chain -- true through a bool local (`andi v0,a0,
 * 0xff` from the shift's 1) */
extern "C" bool func_005FC550(const CAiFollowKind *k)
{
    bool r = false;
    unsigned int t = k->m_kind - 2;

    if (t < 11 && ((1 << t) & 0x40d))
        r = true;
    return r;
}

/* (OVL2 0x5FADF0) */
CAiSFollow::~CAiSFollow()
{
}
