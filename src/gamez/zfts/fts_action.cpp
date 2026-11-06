/* fts_action.cpp -- the map's actions: CNodeAction, its list and the EXECUTE_ACTION command
 * (SOCOM 1's fts_action.cpp: SyncActionsMP to GetActionType, between fts_player.cpp's template
 * instances and the 2D messages; its static initialiser registers the list's destructor, as
 * SOCOM II's 0x4003A0 does). OVL1 0x2B4190-0x2B582F. Defined in reverse of their image order
 * (-inline deferred). */

/* the pods' destructors through alloc() and cap() (include/vector's opt-in, with the level at this file's end): CNodeAction::Close */
#define __MSL_POD_DTOR_ARGS
#include <algorithm>

#include "../zentity/zentity.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

extern "C" int strcasecmp(const char *a, const char *b);

/* a SEAL's controller: only the slot this unit calls (fts_remote.cpp's CEntityCtrl, the names
 * ours) */
class CActionCtrl {
public:
    virtual void _slot0();
    virtual void _slot1();
    virtual void _slot2();
    virtual void _slot3();
    virtual void _slot4();
    virtual void _slot5();
    virtual void _slot6();
    virtual void _slot7();
    virtual void _slot8();
    virtual void _slot9();
    virtual bool IsPlayer();   // 0x30
    virtual bool IsRemote();   // 0x34
};

/* a SEAL: only what ExecuteAction reads */
class CZSealBody {
public:
    unsigned char _0[4];
    int m_4;   // 0x4: its network object (SOCOM 1's m_NetObjectIndex)
    unsigned char _8[0xc0 - 0x8];
    CActionCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x3b0 - 0xc4];
    int m_3b0;   // 0x3b0 (SOCOM 1's at 0x318): handed to the action's anim
};

/* the network: only its flag (fts_remote.cpp's CZNetwork, theNetwork at 0x45A0C0) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* a map's action: a node that runs an anim when a SEAL uses it, driven by a valve (SOCOM 1's
 * CNodeAction, at SOCOM II's offsets: only what this unit touches) */
class CNodeAction : public zdb::CNodeEx {
public:
    /* every action (SOCOM 1's m_actions, a pointer list) */
    class Container : public std::list<CNodeAction *> {
    };

    CNodeAction(zdb::CNode *node, CZAnim *anim, CValve *valve, zdb::CTexHandle *tex);
    /* the node's slot, SOCOM 1's OnAction (OVL1 0x2B4750: ExecuteAction for the body passed);
     * declared only, so this unit emits no table */
    virtual int OnWeaponHit(zdb::CNode *node, zdb::IntersectStruct *hit, CZProjectile *projectile);

    /* SOCOM II's, inlined into Close (no out-of-line copy in the image): the valve destroyed, the
     * description freed, the anims' vector after (SOCOM 1's called out of line) */
    ~CNodeAction()
    {
        CValve::Destroy(m_valve);
        m_valve = 0;
        if (m_uidesc)
            _free((void *)m_uidesc, 0, 0);
        m_uidesc = 0;
    }

    /* SOCOM 1's ACTION_TYPE (a byte under -enum min: the type's names by it, 8 the unknown) */
    enum ACTION_TYPE { ACT_DOOR, ACT_WINDOW, ACT_SWITCH, ACT_DEFUSE, ACT_MPBOMB, ACT_BREACH, ACT_SATCHEL,
                       ACT_ITEM, ACT_UNKNOWN };

    /* a type's name in the actions file (SOCOM 1's, out of line there; inlined into Open here) */
    static const char *GetActionType(ACTION_TYPE type)
    {
        switch (type) {
        case ACT_DOOR:
            return "DOOR";
        case ACT_WINDOW:
            return "WINDOW";
        case ACT_DEFUSE:
            return "DEFUSE";
        case ACT_SWITCH:
            return "SWITCH";
        case ACT_ITEM:
            return "ITEM";
        case ACT_MPBOMB:
            return "MPBOMB";
        case ACT_SATCHEL:
            return "SATCHEL";
        case ACT_BREACH:
            return "ACT_BREACH";
        }
        return "UNKNOWN";
    }
    /* an action made on a node and listed, its id its place in the list (ours, inline: Open's
     * arguments are read before the new, and the action Open keeps is the returned copy -- the
     * list's push_back homes only this local) */
    /* a type by its name, unknown when none matches (ours, inline: SOCOM 1's Open walked the names
     * itself) */
    static ACTION_TYPE GetActionType(const char *name)
    {
        for (int i = 0; i < 8; i++)
            if (!strcasecmp(name, GetActionType((ACTION_TYPE)i)))
                return (ACTION_TYPE)i;
        return ACT_UNKNOWN;
    }
    static CNodeAction *Add(zdb::CNode *node, CZAnim *anim, CValve *valve, zdb::CTexHandle *tex)
    {
        CNodeAction *action = new CNodeAction(node, anim, valve, tex);

        action->m_id = m_actions.size();
        m_actions.push_back(action);
        return action;
    }
    static int Open(zdb::CWorld *world, const char *name);
    static void SyncActionsMP(int who);
    static void Close();
    static void Reset();
    static CNodeAction *GetActionById(unsigned int id);
    static CNodeAction *FindActionByValve(const CValve *valve);
    static void GetTCMActions(std::vector<CNodeAction *> *list);
    const char *GetUIString();
    int ExecuteAction(CZSealBody *body, unsigned int index);
    /* the action taken by a user if no one else has it, and let go by its user (SOCOM II's; the AI
     * states' action state takes and lets go of its object through them: the names ours) */
    bool Acquire(unsigned int who);
    void Release(unsigned int who);

    /* whether the action's state goes to the other clients (the name ours) */
    bool IsNetworked() { return m_networked ? true : false; }

    /* the flags set one by one (the constructor's order; the names ours) */
    void SetNetworked(bool b) { m_networked = b; }
    void SetNetworkLog(bool b) { m_network_log = b; }
    void SetTCM(bool b) { m_tcm = b; }
    void SetFlag3(bool b) { m_3f_3 = b; }
    void SetBits4(int n) { m_3f_4 = n; }

    std::vector<CZAnim *> m_anims;   // 0x4: SOCOM 1's m_anim
    CZAnim *m_anim;   // 0x10: the one running (SOCOM 1's m_handle)
    zdb::CNode *m_node;   // 0x14
    CValve *m_valve;   // 0x18
    float m_range;   // 0x1c
    float m_20;   // 0x20 (SOCOM II's)
    unsigned char m_type;   // 0x24: SOCOM 1's ACTION_TYPE (0 a door, 2 a switch ...)
    zdb::CTexHandle *m_tex;   // 0x28
    const char *m_uidesc;   // 0x2c: a description of its own, in place of its type's (SOCOM 1's
                            // m_descriptor)
    int m_30;   // 0x30
    unsigned int m_owner;   // 0x34: the entity using it (SOCOM II's)
    int m_38;   // 0x38
    unsigned short m_id;   // 0x3c (SOCOM 1's at 0x2c)
    unsigned char m_3e;   // 0x3e
    unsigned char m_networked : 1;   // 0x3f (SOCOM 1's at 0x2e)
    unsigned char m_network_log : 1;
    unsigned char m_tcm : 1;   // bit 2: offered in the command menu (Open's `tcm` tag)
    unsigned char m_3f_3 : 1;
    unsigned char m_3f_4 : 4;

    /* whether the command menu offers the action (bit 2 of 0x3f, set by Open's `tcm` tag; the
     * name ours) */
    bool IsTCM() { return m_tcm ? true : false; }

    static Container m_actions;
};

/* CValve::SetValue, inlined in both builds (CValve, VALVE_STATE and MakeCallbacks are zanim.h's) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

CZSealBody *ftsGetPlayer();
extern "C" int strcmp(const char *a, const char *b);

/* the rdr reader (zrdr.h's) */
class CRdrFile;
_zrdr *zrdr_read(const char *name, const char *path, int flags);
int zrdr_free(CRdrFile *file);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
/* a team mask read (vehicle_rdr.cpp's) */
unsigned int ParseTeams(_zrdr *rdr);
/* whether the network game swaps its sides (fts_net_game.cpp's) */
extern "C" unsigned char data_004412D8;

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_002BB5B0(int to, unsigned int id, unsigned char value, int arg, int who);   /* NetSendActionAnim,
                                                                         SOCOM II's with one more */
_zanim_cmd_hdr *func_002B4230(_zrdr *rdr);
}

/* an AI map's action (SOCOM 1's CAiMap::action: only its state, three bits of byte 6) */
class CAiMap {
public:
    struct action {
        unsigned char _0[6];
        unsigned char m_6_0 : 4;
        unsigned char m_state : 3;   // 0x6, bits 4-6
    };
};

/* the AI maps (OVL2's: only the action lookup) */
class CAiMaps {
public:
    bool GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **action);   /* OVL2 0x52DAA0 */
};

extern "C" {
extern CAiMaps AiMaps;   /* OVL2 0x66A1B0 (SOCOM 1's theAiMaps) */
}

/* every action (its initialiser __sinit_fts_action.cpp 0x4003A0 builds the list through the
 * deleter's constructor, 0x1EABB0, and registers ~Container 0x2B57D0, as SOCOM 1's does) */
CNodeAction::Container CNodeAction::m_actions;

/* an action's valve changed: the AI map's action of the same id takes the valve's value (SOCOM 1's,
 * the id at SOCOM II's offset; OVL1 0x2B5760) */
void _actionValveCallback(CValve *valve, void *data)
{
    CNodeAction *action = (CNodeAction *)data;

    if (action) {
        CAiMap::action *a = 0;
        CAiMapLoc loc;

        if (AiMaps.GetActionById(action->m_id, loc, &a))
            a->m_state = valve->m_value;
    }
}

/* an action let go of: its node's extension cleared when it is the action, the action deleted
 * and its slot left null (std::transform's operation, inlined; the name ours) */
static inline CNodeAction *_close_action(CNodeAction *action)
{
    if (action && action->m_node && action == action->m_node->m_nodeEx)
        action->m_node->SetNodeEx(0, false);
    delete action;
    return 0;
}

/* every action deleted and the list emptied (SOCOM 1's CNodeAction::Close, the walk a
 * std::transform as SOCOM 1's two iterators show; OVL1 0x2B55E0, CMission::Close's call). It is
 * also what makes the unit's tail call ~__list_imp from ~Container, as the target does: the clear
 * instantiates the range erase (README "Out-of-class template members"). Byte-exact since the MSL
 * headers' round 40, under `#pragma inline_depth(7)` at the file's end and `__MSL_POD_DTOR_ARGS`
 * (a modelling device, a "Level 6, or 8?" witness): transform (pass 0), the op (1), ~CNodeAction
 * (2), the vector's three this-tests (3, 4, 5), deallocate (6), and its two accessor arguments would
 * be expanded at pass 7, so they stay calls, cap() 0x2B5750 then alloc() 0x2B5740, the data word held
 * across them. The height check does not count an argument's wait, so all of it is admitted */
void CNodeAction::Close()
{
    std::transform(m_actions.begin(), m_actions.end(), m_actions.begin(), _close_action);
    m_actions.clear();
}

/* every action back to its start: its valve off, no anim, no user (OVL1 0x2B5510) */
void CNodeAction::Reset()
{
    Container::iterator e = m_actions.end();

    for (Container::iterator i = m_actions.begin(); i != e; ++i) {
        CNodeAction *action = *i;

        if (action) {
            if (action->m_valve)
                SetValveValue(action->m_valve, 0);
            action->m_anim = 0;
            action->m_3e = 0;
            action->m_network_log = false;
            action->m_owner = CEntity::invalidId;
        }
    }
}

/* the map's actions read from its rdr (OVL1 0x2B4F40: SOCOM 1's CNodeAction::Open, grown): for each
 * entry of `actions`, its node found in the world, its valve made, an action put on the node unless
 * it already has an extension -- its anim, bitmap, range and elevation, then SOCOM II's settings:
 * the descriptor, no_netlog, one_shot, points, the type by name, the extra anims, the team (its two
 * sides swapped when the network game swaps them) and tcm. Byte-exact (round 35): the action made
 * and listed through the inline Add, the type by name through the inline GetActionType(const char *)
 * returning early, and the anims tag and each extra anim assigned in their tests (the value tested
 * in v0, the variable set in the delay slot) */
int CNodeAction::Open(zdb::CWorld *world, const char *name)
{
    _zrdr *rdr = zrdr_read(name, 0, 0);
    _zrdr *actions = zrdr_findtag(rdr, "actions");

    if (actions && actions->type == 4) {
        int n = actions->Count();

        for (unsigned int i = 0; (int)i < n; i++) {
            _zrdr *entry = actions->Item(i);
            float range = 32.0f;
            float elevation = -1.0f;
            const char *nodeName = zrdr_findstring(entry, "node");
            const char *animName = zrdr_findstring(entry, "anim");
            const char *valveName = zrdr_findstring(entry, "valve");
            const char *bitmap = zrdr_findstring(entry, "bitmap");
            const char *descriptor = zrdr_findstring(entry, "descriptor");
            zdb::CTexHandle *tex = 0;

            if (bitmap)
                tex = zdb::CWorld::GetTexHandle(bitmap);
            zrdr_findreal(entry, "range", &range, 1);
            zrdr_findreal(entry, "elevation", &elevation, 1);

            zdb::CNode *node = world->FindChild(nodeName, true);
            CZAnim *anim = animName ? ZAnim.GetAnim(animName, 0) : 0;

            if (node) {
                CValve::GetByName(valveName);
                CValve *valve = CValve::Create(valveName, VALVE_PERM);

                if (!node->m_nodeEx) {
                    CNodeAction *action = Add(node, anim, valve, tex);

                    action->m_range = range;
                    action->m_20 = elevation;
                    node->SetNodeEx(action, false);
                    valve->AddCallback(3, _actionValveCallback, action);
                    if (descriptor)
                        action->m_uidesc = _strdup(descriptor, 0, 0);
                    if (zrdr_findtag(entry, "no_netlog"))
                        action->SetNetworked(false);
                    if (zrdr_findtag(entry, "one_shot"))
                        action->SetFlag3(true);
                    zrdr_findint(entry, "points", &action->m_38, 1);

                    const char *type = zrdr_findstring(entry, "type");

                    if (type)
                        action->m_type = GetActionType(type);

                    _zrdr *anims;

                    if ((anims = zrdr_findtag(entry, "anim")) != 0) {
                        unsigned int count = anims->Count();

                        for (unsigned int k = 1; k < count; k++) {
                            if ((anim = ZAnim.GetAnim(anims->Item(k)->String(), 0)) != 0)
                                action->m_anims.push_back(anim);
                        }
                    }

                    _zrdr *team = zrdr_findtag(entry, "team");

                    if (team) {
                        unsigned int teams = ParseTeams(team);

                        if (theNetwork.m_bNetwork && data_004412D8) {
                            unsigned int swapped = teams & ~0x101;

                            if (teams & 1)
                                swapped |= 0x100;
                            if (teams & 0x100)
                                swapped |= 1;
                            teams = swapped;
                        }
                        action->m_30 = teams;
                    }
                    if (zrdr_findtag(entry, "tcm"))
                        action->SetTCM(true);
                }
            }
        }
    }
    zrdr_free((CRdrFile *)rdr);
    return 0;
}

/* the action whose valve has this name (SOCOM II's; OVL1 0x2B4E80). Out of line: the
 * EXECUTE_ACTION tick calls it */
#pragma auto_inline off
extern "C" CNodeAction *func_002B4E80(const char *name)
{
    CNodeAction *found = 0;

    if (name) {
        CNodeAction::Container::iterator i, e = CNodeAction::m_actions.end();

        for (i = CNodeAction::m_actions.begin(); i != e; ++i) {
            CNodeAction *action = *i;

            if (action->m_valve && !strcmp(action->m_valve->m_name, name)) {
                found = action;
                break;
            }
        }
    }
    return found;
}

#pragma auto_inline reset

/* the action a valve drives (OVL1 0x2B4E00) */
CNodeAction *CNodeAction::FindActionByValve(const CValve *valve)
{
    CNodeAction *found = 0;

    if (valve) {
        Container::iterator i, e = m_actions.end();

        for (i = m_actions.begin(); i != e; ++i) {
            CNodeAction *action = *i;

            if (action->m_valve == valve) {
                found = action;
                break;
            }
        }
    }
    return found;
}

/* every action the command menu offers appended to a vector (SOCOM II's; OVL1 0x2B4CD0, the HUD's
 * tcmSetNodeActions 0x236260 calls it: the actions Open tagged `tcm`; the name ours) */
void CNodeAction::GetTCMActions(std::vector<CNodeAction *> *list)
{
    if (list) {
        Container::iterator e = m_actions.end();

        for (Container::iterator i = m_actions.begin(); i != e; ++i) {
            CNodeAction *action = *i;

            if (action && action->IsTCM())
                list->push_back(action);
        }
    }
}

/* the action with this id (OVL1 0x2B4D80) */
CNodeAction *CNodeAction::GetActionById(unsigned int id)
{
    Container::iterator i, e = m_actions.end();

    for (i = m_actions.begin(); i != e; ++i)
        if ((*i)->m_id == id)
            return *i;
    return 0;
}

/* the action's types described for the UI (SOCOM 1's UISTR_ACT_* pointers; SOCOM II's second
 * set: the door's pair by its valve, then one each) */
extern "C" {
extern const char *data_003DE698[2];   /* "Close target door under reticule", "Open ..." */
extern const char *data_003DE6A0;      /* "Activate switch" */
extern const char *data_003DE6A8;
extern const char *data_003DE6B0;
extern const char *data_003DE6B8;
extern const char *data_003DE6C0;
extern const char *data_003DE6C8;      /* "Breach doorway under reticule" */
}

/* the action described for the UI: its own description, else its type's (SOCOM II's; OVL1
 * 0x2B4760 -- 0x2B4820 and GetUIString 0x2B48E0 beside it are the same switch over two more name tables) */
extern "C" const char *func_002B4760(CNodeAction *action)
{
    const char *s = action->m_uidesc;

    if (!s) {
        switch (action->m_type) {
        case 0:
            s = action->m_valve->m_value ? data_003DE698[0] : data_003DE698[1];
            break;
        case 2:
            s = data_003DE6A0;
            break;
        case 7:
            s = data_003DE6A8;
            break;
        case 3:
            s = data_003DE6B0;
            break;
        case 4:
            s = data_003DE6B8;
            break;
        case 6:
            s = data_003DE6C0;
            break;
        case 5:
            s = data_003DE6C8;
            break;
        }
    }
    return s;
}

/* whether the action can start: no anim running, its flag 3 clear or the byte at 0x3E zero, and
 * its valve not at 99 (SOCOM II's; OVL1 0x2B46F0) */
extern "C" bool func_002B46F0(CNodeAction *action)
{
    return action->m_anim == 0 && (!action->m_3f_3 || action->m_3e == 0) &&
           action->m_valve->m_value != 99;
}

/* the action whose node is nearest a point across the ground, within a height of it unless the
 * height is -1, and its distance; none when no node is placed (SOCOM 1's GetClosestAction, which
 * returned the id; OVL1 0x2B4B40) */
extern "C" CNodeAction *func_002B4B40(const CPnt3D *pos, float *dist, float height)
{
    float best = 9999999.0f * 9999999.0f;
    CNodeAction *closest = 0;
    CMatrix m;
    CNodeAction::Container::iterator e = CNodeAction::m_actions.end();

    for (CNodeAction::Container::iterator i = CNodeAction::m_actions.begin(); i != e; ++i) {
        CNodeAction *action = *i;
        zdb::CNode *node = action->m_node;

        if (node) {
            m.SetIdentity();
            if (node->BuildMTW(m)) {
                CPnt3D d = *(CPnt3D *)m.m_matrix[3] - *pos;

                if (height == -1.0f || fabsf(d.y) < height) {
                    float r = d.x * d.x + d.z * d.z;

                    if (r < best) {
                        closest = action;
                        best = r;
                    }
                }
            }
        }
    }
    if (closest)
        *dist = sqrtf(best);
    return closest;
}

/* an action on a node, its anim (if any) kept in its list: a range of 32, networked, no user
 * (OVL1 0x2B49F0: SOCOM 1's constructor, the type 8 where SOCOM 1 had 7) */
CNodeAction::CNodeAction(zdb::CNode *node, CZAnim *anim, CValve *valve, zdb::CTexHandle *tex)
    : m_anim(0), m_node(node), m_valve(valve), m_type(8), m_tex(tex)
{
    if (anim)
        m_anims.push_back(anim);
    m_uidesc = 0;
    m_38 = 0;
    m_30 = -1;
    m_id = 0;
    m_3e = 0;
    SetNetworked(true);
    SetNetworkLog(false);
    SetTCM(false);
    SetFlag3(false);
    SetBits4(0);
    m_range = 32.0f;
    m_20 = -1.0f;
    m_owner = CEntity::invalidId;
}

/* the action let go by its user: the word at 0x34 back to the invalid id when it is the one passed
 * (SOCOM II's; OVL1 0x2B49D0 -- CAiSAction::OnSignal 0x5F0470 calls it twice for its object) */
void CNodeAction::Release(unsigned int who)
{
    if (m_owner == who)
        m_owner = CEntity::invalidId;
}

/* the action taken by a user, if no one else has it (SOCOM II's; OVL1 0x2B49A0 -- OVL2's 0x5F0180
 * calls it) */
bool CNodeAction::Acquire(unsigned int who)
{
    bool taken = false;

    if (m_owner == CEntity::invalidId || m_owner == who) {
        m_owner = who;
        taken = true;
    }
    return taken;
}

/* the action types' names (SOCOM 1's UISTR_ACT_* pointers, a set of SOCOM II's own for each of the
 * three describers: the door's pair by its valve, then one each) */
extern "C" {
extern const char *data_003DE628[2];   /* "CLOSE DOOR", "OPEN DOOR" */
extern const char *data_003DE630;      /* "FLIP SWITCH" */
extern const char *data_003DE638;      /* "TAKE ITEM" */
extern const char *data_003DE640;      /* "DEFUSE BOMB" */
extern const char *data_003DE648;      /* "MP BOMB" */
extern const char *data_003DE650;      /* "PLACE SATCHEL" */
extern const char *data_003DE658;      /* "BREACH" */
}

/* the action's type named for the UI (SOCOM 1's GetUIString(), returning the name where SOCOM 1's
 * string overload filled one; OVL1 0x2B48E0: CActionTxtrMachine::Add 0x2209C0 calls it where SOCOM
 * 1's Add calls GetUIString) */
const char *CNodeAction::GetUIString()
{
    const char *s = 0;

    switch (m_type) {
    case 0:
        s = m_valve->m_value ? data_003DE628[0] : data_003DE628[1];
        break;
    case 2:
        s = data_003DE630;
        break;
    case 7:
        s = data_003DE638;
        break;
    case 3:
        s = data_003DE640;
        break;
    case 4:
        s = data_003DE648;
        break;
    case 6:
        s = data_003DE650;
        break;
    case 5:
        s = data_003DE658;
        break;
    }
    return s;
}

/* an anim's done callback set (CZAnim's fields at 0x1c/0x20; the anim a copy, so both stores go
 * through one load) */
inline void SetDoneCallback(CZAnim *anim, void (*cb)(CZAnim *anim, void *arg), void *arg)
{
    anim->m_done_callback = cb;
    anim->m_done_arg = arg;
}

/* the action's anim done: the action free again, its uses counted (SOCOM II's; OVL1 0x2B46D0,
 * ExecuteAction's done callback) */
extern "C" void func_002B46D0(CZAnim *anim, void *arg)
{
    CNodeAction *action = (CNodeAction *)arg;

    action->m_anim = 0;
    action->m_3e++;
}

/* one of the action's anims run (the index's, else the first) on its node and valve for a SEAL,
 * told to the other clients when networked; whether it runs (OVL1 0x2B44E0: SOCOM 1's, with the
 * use limit, the SEAL's object index passed on when it has one, and the done callback SOCOM II's) */
int CNodeAction::ExecuteAction(CZSealBody *body, unsigned int index)
{
    if (m_anim && m_anim->Running())
        return 0;
    if (m_3f_3 && m_3e > 0)
        return 0;
    if (m_anims.size() == 0)
        return 0;
    CZAnim *anim = index < m_anims.size() ? m_anims[index] : m_anims[0];

    if (!anim)
        return 0;
    m_anim = 0;
    int who = -1;

    if (body)
        who = body->m_4;

    if (who >= 0)
        m_anim = anim->ActivateArg(1, 3, m_node, 4, m_valve, 1, body ? body->m_3b0 : 0, 9, who, 0);
    else
        m_anim = anim->ActivateArg(1, 3, m_node, 4, m_valve, 1, body ? body->m_3b0 : 0, 0);
    if (m_anim) {
        SetDoneCallback(m_anim, func_002B46D0, this);
        if (IsNetworked() && theNetwork.m_bNetwork && body &&
            !(body->m_ctrl ? body->m_ctrl->IsRemote() : false))
            func_002BB5B0(body->m_4, m_id, m_valve->m_value, index, -1);
    }
    return m_anim != 0;
}

/* every networked action's valve sent to a client that joins (OVL1 0x2B4440) */
void CNodeAction::SyncActionsMP(int who)
{
    Container::iterator e = m_actions.end();

    for (Container::iterator i = m_actions.begin(); i != e; ++i) {
        CNodeAction *action = *i;

        if (action->IsNetworked() && action->m_valve->m_value)
            func_002BB5B0(-1, action->m_id, action->m_valve->m_value, 0, who);
    }
}

/* every action's anim paused, or resumed (SOCOM II's; OVL1 0x2B4380: the paused byte converted to
 * bool before the test, as an accessor of CZAnim's would) */
extern "C" void func_002B4380(bool pause)
{
    for (CNodeAction::Container::iterator i = CNodeAction::m_actions.begin(),
         e = CNodeAction::m_actions.end(); i != e; ++i) {
        CNodeAction *action = *i;

        if (pause) {
            if (action->m_anim && action->m_anim->Running())
                func_00270BE0(action->m_anim, 0);
        } else if (action->m_anim && (action->m_anim->m_paused ? true : false))
            func_002709B0(action->m_anim, 0);
    }
}

/* EXECUTE_ACTION's command: its header and the action's name, an index into the anim's names */
struct _execute_action_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char name;   // 0x4
};

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
extern "C" int func_0026ED00(CZAnim *anim, const char *name);   /* CZAnim::AddName */

/* EXECUTE_ACTION parsed: the NAME tag's (or the block's own) first string kept as a name of the
 * current anim (SOCOM II's; OVL1 0x2B4230, registered by 0x2B4190) */
extern "C" _zanim_cmd_hdr *func_002B4230(_zrdr *rdr)
{
    _execute_action_cmd *cmd = 0;

    if (rdr) {
        _zrdr *tag = zrdr_findtag(rdr, "NAME");

        if (!tag)
            tag = rdr;
        if (tag->type == 4 && tag->Count() && tag->Item(0)->type == 3) {
            CZAnim *anim = ZAnim.m_CurAnim;

            cmd = (_execute_action_cmd *)_calloc(1, sizeof(_execute_action_cmd), 0, 0);
            cmd->hdr.data_size = sizeof(_execute_action_cmd);
            cmd->name = func_0026ED00(anim, tag->Item(0)->String());
        }
    }
    return &cmd->hdr;
}

/* EXECUTE_ACTION: the action named by the command, run for the player (SOCOM II's; OVL1
 * 0x2B41C0) */
extern "C" bool func_002B41C0(_zanim_cmd_hdr *hdr, float *dT)
{
    const char *name = ZAnim.m_CurAnim->GetName(((_execute_action_cmd *)hdr)->name);

    if (name) {
        CNodeAction *action = func_002B4E80(name);

        if (action)
            action->ExecuteAction(ftsGetPlayer(), 0);
    }
    return true;
}

/* the actions' ZAnim command (SOCOM II's; OVL1 0x2B4190) */
extern "C" void func_002B4190()
{
    ZAnim.AddCmd("EXECUTE_ACTION", func_002B4230, 0, func_002B41C0, 0);
}

/* a modelling device, a "Level 6, or 8?" witness: CNodeAction::Close's note */
#pragma inline_depth(7)
