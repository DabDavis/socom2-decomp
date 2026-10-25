/* fts_action.cpp -- the map's actions: CNodeAction, its list and the EXECUTE_ACTION command
 * (SOCOM 1's fts_action.cpp: SyncActionsMP to GetActionType, between fts_player.cpp's template
 * instances and the 2D messages; its static initialiser registers the list's destructor, as
 * SOCOM II's 0x4003A0 does). OVL1 0x2B4190-0x2B582F. Defined in reverse of their image order
 * (-inline deferred). */

#include "../zentity/zentity.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

class CZSealBody;

/* a map's action: a node that runs an anim when a SEAL uses it, driven by a valve (SOCOM 1's
 * CNodeAction, at SOCOM II's offsets: only what this unit touches) */
class CNodeAction {
public:
    /* every action (SOCOM 1's m_actions, a pointer list) */
    class Container : public std::list<CNodeAction *> {
    };

    static void SyncActionsMP(int who);
    static void Reset();
    static CNodeAction *GetActionById(unsigned int id);
    static CNodeAction *FindActionByValve(const CValve *valve);
    int ExecuteAction(CZSealBody *body, unsigned int index);

    /* whether the action's state goes to the other clients (the name ours) */
    bool IsNetworked() { return m_networked ? true : false; }

    unsigned char _0[0x10];
    CZAnim *m_anim;   // 0x10
    unsigned char _14[4];
    CValve *m_valve;   // 0x18
    unsigned char _1c[0x34 - 0x1c];
    unsigned int m_owner;   // 0x34: the entity using it (SOCOM II's)
    unsigned char _38[4];
    unsigned short m_id;   // 0x3c (SOCOM 1's at 0x2c)
    unsigned char m_3e;   // 0x3e
    unsigned char m_networked : 1;   // 0x3f (SOCOM 1's at 0x2e)
    unsigned char m_network_log : 1;
    unsigned char m_3f_2 : 1;
    unsigned char m_3f_3 : 1;

    static Container m_actions;
};

/* a valve's change of state, its callbacks told (SOCOM 1's CValve::MakeCallbacks; zanim.h's CValve
 * has neither the enum nor the setter, so they are spelled here as fts_remote.cpp spells them) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

extern "C" void func_003519F0(CValve *valve, VALVE_STATE state);   /* CValve::MakeCallbacks */

/* CValve::SetValue, inlined in both builds */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        func_003519F0(valve, state);
}

CZSealBody *ftsGetPlayer();
extern "C" int strcmp(const char *a, const char *b);

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_002BB5B0(int to, unsigned int id, unsigned char value, int arg, int who);   /* NetSendActionAnim,
                                                                         SOCOM II's with one more */
_zanim_cmd_hdr *func_002B4230(_zrdr *rdr);
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

/* the action with this id (OVL1 0x2B4D80) */
CNodeAction *CNodeAction::GetActionById(unsigned int id)
{
    Container::iterator i, e = m_actions.end();

    for (i = m_actions.begin(); i != e; ++i)
        if ((*i)->m_id == id)
            return *i;
    return 0;
}

/* whether the action can start: no anim running, its flag 3 clear or the byte at 0x3E zero, and
 * its valve not at 99 (SOCOM II's; OVL1 0x2B46F0) */
extern "C" bool func_002B46F0(CNodeAction *action)
{
    return action->m_anim == 0 && (!action->m_3f_3 || action->m_3e == 0) &&
           action->m_valve->m_value != 99;
}

/* the action let go by its user (SOCOM II's; OVL1 0x2B49D0) */
extern "C" void func_002B49D0(CNodeAction *action, unsigned int who)
{
    if (action->m_owner == who)
        action->m_owner = CEntity::invalidId;
}

/* the action taken by a user, if no one else has it (SOCOM II's; OVL1 0x2B49A0) */
extern "C" bool func_002B49A0(CNodeAction *action, unsigned int who)
{
    bool taken = false;

    if (action->m_owner == CEntity::invalidId || action->m_owner == who) {
        action->m_owner = who;
        taken = true;
    }
    return taken;
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

/* EXECUTE_ACTION: the action named by the command, run for the player (SOCOM II's; OVL1
 * 0x2B41C0) */
struct _execute_action_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char name;   // 0x4
};

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
