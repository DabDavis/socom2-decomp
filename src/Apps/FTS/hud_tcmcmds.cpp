/* hud_tcmcmds.cpp -- the radio command menu's commands (OVL1 0x22EDB0-0x231A2F; the name ours):
 * what each item of the menu runs when it is chosen and the test that shows, hides or relabels it
 * (the caches register them in pairs, 0x2344E0), the item caches' own code and the menu's layout.
 * SOCOM II's own -- no SOCOM 1 counterpart (hud_tcm.h has the cut into three units).
 *
 * Its statics (OVL1 bss 0x412FF0-0x41309F, the initialiser 0x3FF430 sets the layout's) follow
 * the iostream initialisers of st_reboot.cpp's statics. Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include <stdlib.h>
#include <list>

#include "hud_tcm.h"

/* the world's entities (zentity.h's: only the list and the type byte) */
class CEntity {
public:
    static std::list<CEntity *> m_list;

    unsigned char _0[0x4];
    int m_id;   // 0x4: the network id
    unsigned char _8[0x10 - 0x8];
    unsigned char m_type;   // 0x10: 2 a SEAL
    unsigned char _11[0x14 - 0x11];
    char *m_name;   // 0x14

    /* (ours: the vote reads both ids through it -- expanded ahead of the call, the target's
     * id is loaded before ftsGetPlayer is called) */
    int GetId() const { return m_id; }
};

class CNodeAction;

/* the controller (zseal.h's: what the commands call) */
class CSealCtrl {
public:
    virtual void v8();
    virtual void vC();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1C();
    virtual void v20();
    virtual void PlayGesture(int id, float rate, bool now);   // 0x24 (the name ours)

    bool CanPerformAction(CNodeAction *action);
};

/* the network persona a player's votes and mutes are kept on (the network lane's) */
class CZNetPersona;

/* the SEAL as the commands read it */
class CZSealBody : public CEntity {
public:
    unsigned char _18[0xc0 - 0x18];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_teamflags;   // 0xc8: the sides it is on (bit 17 a spectator)
    unsigned char _cc[0xe1 - 0xcc];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4 (the HUD's units test it)
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x538 - 0xe2];
    int m_lastaction;   // 0x538: the action last run
    unsigned char _53c[0xfc8 - 0x53c];
    CZNetPersona *m_persona;   // 0xfc8
};

/* a node's action (zseal.h's) */
class CNodeAction {
public:
    void ExecuteAction(CZSealBody *seal, unsigned int flags);

    unsigned char _0[0x10];
    int m_id;   // 0x10
    unsigned char _14[0x24 - 0x14];
    unsigned char m_kind;   // 0x24: 4 cannot be run from the menu
    unsigned char _25[0x30 - 0x25];
    unsigned int m_teams;   // 0x30: the sides that may run it
    unsigned char _34[0x3f - 0x34];
    unsigned char _3f_0 : 2;
    unsigned char m_enabled : 1;   // 0x3f bit 2
    unsigned char _3f_3 : 5;
};

/* a squad of SEALs (zseal.h's CSealUnit: its orders) */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);
    void *GetDataPtr();

    unsigned char _0[0x4];
    /* the vtable pointer after the first word */
    virtual void Order(CZSealBody *player, int member, int cmd, int flags);   // 0x8 (the names ours)
    virtual bool CanOrder(CZSealBody *player, int member, int cmd);   // 0xc

    unsigned char _8[0xc4 - 0x8];
    int m_channel;   // 0xc4: the radio channel
};

/* the network game (fts_net_game's): whether it is one */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the mission's network game: its type byte (a byte enum: lb, then compared with andi) */
enum NET_GAME_TYPE {
    NGT_0 = 0,
    NGT_1 = 1,
    NGT_2 = 2,
    NGT_3 = 3,
    NGT_4 = 4,
    NGT_LAST = 0x7f
};

class CZNetGame {
public:
    unsigned char _0[0x111];
    NET_GAME_TYPE m_type;   // 0x111
};

extern CZNetGame *data_00437CE8;
/* the mission's vocal queue (theMission + 0x1C0) */
extern unsigned char data_004366A0[];

/* the sound system's */
class CSnd {
public:
    static CSnd *GetSoundByName(const char *name);
};

enum MUSIC_MODE {
    MUSIC_MODE_0 = 0,
    MUSIC_MODE_3 = 3
};

class CSndJukebox {
public:
    static void Play(MUSIC_MODE mode, CSnd *snd);
};

/* the voice chat (OVL1 bss 0x45B718: a pointer; whether the radio is on, a player muted) */
extern void *data_0045B718;

/* the network senders and the remote messages (fts_remote.cpp's, fts_net_blather.cpp's) */
enum R_SEND_TO {
    RST_ALL = -1,
    RST_SIDE1 = -2,
    RST_SIDE2 = -3
};

void SendVoteMessage(int from, int to, bool remove);
void SendMessageMessage(int from, R_SEND_TO to, int msg);
void PlayRemoteMsg(CEntity *from, int msg, int flags);
CZSealBody *ftsGetPlayer();
const char *TranslateString(const char *s);

extern "C" {
/* the SEAL lane's, no names yet */
void func_005EAF20(CSealUnit *unit, int msg, int flags, float rate);
void func_005EAEB0(CSealUnit *unit);
void func_005EAD70(CSealUnit *unit, int member, int cmd, int flags);
void func_005EABA0(CSealUnit *unit, int member, int cmd, int flags);
CNodeAction *func_005AA240(CZSealBody *seal, int which);
CEntity *func_005E8A90(CSealUnit *unit);
bool func_0058DFA0(unsigned char member);
/* the mission's */
void func_002B6530(void *queue, const char *name, int a, int b, int c, float delay);
int func_002B46F0(CNodeAction *action);
void *func_002B48E0(void *data);
void *func_002B4760(void *data);
void *func_002B4820(void *data);
/* the network game's: whether a SEAL is alive, on each side; the vote and the persona's flag */
bool func_002C2FD0(CZSealBody *seal);
bool func_002C31D0(const CZSealBody *seal);
bool func_002C30B0(const CZSealBody *seal);
bool func_002C3980(CZNetPersona *persona);
void func_002C39D0(CZNetPersona *persona, bool remove);
void func_002B9B60(int from, bool remove);
int func_002C2FA0();
int func_002C2C20(int channel);
const char *func_002C3B60(CZSealBody *seal, int i);
const char *func_002C3C30(CZSealBody *seal, int i);
/* the voice chat's */
bool func_0030E540(void *chat);
void func_0030E490(void *chat);
bool func_0030E550(void *chat, CZNetPersona *persona);
void func_0030E580(void *chat, CZNetPersona *persona, bool muted);
/* the replay of the last message (a sender and its argument) */
struct CTCMRepeat;
CTCMRepeat *func_0034A620();
}

class CTCMRepeatTarget {
public:
    virtual void v8();
    virtual void Repeat(int a, int b);   // 0xc
};

struct CTCMRepeat {
    CTCMRepeatTarget *m_target;   // 0x0
    int m_arg;   // 0x4
};

/* the unit's globals (not file statics: a call reloads them, as the target does) */
CTCMNode *s_tcmCurNode;   // 0x412FF0: the node whose command or test runs
bool s_tcmVoteAll;   // 0x412FF8
bool s_tcmRecoMode;   // 0x413000: orders come from the voice recogniser
unsigned char s_tcmTeam;   // 0x413008
signed char s_tcmMember;   // 0x413010
CZSealBody *s_tcmTarget;   // 0x413018: the player a vote or a mute is for

/* the world's entities, read-only (the walks copy begin() and end() out of it twice each, then
 * keep them in registers: const iterators initialised from a const view) */
static inline const std::list<CEntity *> &tcmEntities()
{
    return CEntity::m_list;
}

/* -------------------------------------------------------------------------------------------- */

void tcmSetCurNode(CTCMNode *node)
{
    s_tcmCurNode = node;
}

void tcmClearCurNode()
{
    s_tcmCurNode = 0;
}

int tcmUpdateTeam(CZSealBody *player, int team)
{
    int r = 1;
    CSealUnit *unit = CSealUnit::GetUnitByTeam(team);

    if (unit && player && player->m_socom2_e1_4) {
        s_tcmTeam = team;
        r = 0;
        if (s_tcmTeam == 18) {
            CEntity *leader = func_005E8A90(unit);

            if (leader) {
                const char *name = leader->m_name;

                if (name && strstr(name, "CHA SOLDIER"))
                    name = "SOLDIER";
                func_00232170(s_tcmCurNode, name, 0);
                func_00232100(s_tcmCurNode, name);
            }
        }
    }
    return r;
}

int tcmSelectTeam(CZSealBody *player, int team)
{
    s_tcmTeam = team;
    return 1;
}

int tcmUpdateTeamMember(CZSealBody *player, int member)
{
    int r = 1;

    s_tcmMember = member;
    if (func_0058DFA0(member)) {
        r = 0;
    } else {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

        if (unit && player && player->m_socom2_e1_4 && unit->CanOrder(player, (unsigned char)member, 0)) {
            r = 0;
            if ((unsigned char)member == 1 && s_tcmCurNode) {
                void *data = unit->GetDataPtr();

                if (data) {
                    void *caption = func_002B48E0(data);
                    void *text2 = func_002B4760(data);
                    void *name = func_002B4820(data);

                    if (caption && text2) {
                        func_00232170(s_tcmCurNode, (const char *)caption, 0);
                        func_002320E0(s_tcmCurNode, (const char *)text2);
                    } else {
                        r = 1;
                    }
                    if (name)
                        func_00232100(s_tcmCurNode, (const char *)name);
                }
            }
        }
    }
    return r;
}

int tcmTeamMember(CZSealBody *player, int member)
{
    int r = 1;

    s_tcmMember = member;
    if (s_tcmRecoMode) {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

        if (unit && player && player->m_socom2_e1_4)
            func_005EAD70(unit, s_tcmMember, 0, 0);
        return 1;
    }
    if (!func_0058DFA0(member)) {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

        if (unit && player && player->m_socom2_e1_4) {
            func_005EABA0(unit, s_tcmMember, 0, 0);
            unit->Order(player, s_tcmMember, 0, 0);
            r = 0;
        }
    }
    return r;
}

int tcmUpdateTeamCommand(CZSealBody *player, int cmd)
{
    int r = 1;
    CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

    if (unit && player && player->m_socom2_e1_4 && unit->CanOrder(player, s_tcmMember, (unsigned char)cmd))
        r = 0;
    return r;
}

int tcmTeamCommand(CZSealBody *player, int cmd)
{
    int r = 1;

    if (s_tcmRecoMode) {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

        if (unit && player && player->m_socom2_e1_4)
            func_005EAD70(unit, s_tcmMember, (unsigned char)cmd, 0);
        return 1;
    }
    CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

    if (unit && player && player->m_socom2_e1_4) {
        func_005EABA0(unit, s_tcmMember, (unsigned char)cmd, 0);
        unit->Order(player, s_tcmMember, (unsigned char)cmd, 0);
        r = 0;
    }
    return r;
}

int tcmUpdateIntimidate(CZSealBody *player, int arg)
{
    return 0;
}

int tcmIntimidate(CZSealBody *player, int arg)
{
    if (player && player->m_socom2_e1_4)
        player->m_ctrl->PlayGesture(5, 1.0f, true);
    return 0;
}

int tcmSendMessage(CZSealBody *player, int msg)
{
    if (!s_tcmCurNode->IsWaiting()) {
        R_SEND_TO to = RST_ALL;

        if (msg >= 54 && msg < 59) {
            if (func_002C31D0(player))
                to = RST_SIDE1;
            else if (func_002C30B0(player))
                to = RST_SIDE2;
        } else {
            if (func_002C31D0(player))
                to = RST_SIDE2;
            else if (func_002C30B0(player))
                to = RST_SIDE1;
        }
        SendMessageMessage(player->m_id, to, (unsigned char)msg);
        PlayRemoteMsg(player, (unsigned char)msg, 0);
        func_00232240(s_tcmCurNode, 5.0f);
    }
    return 0;
}

int tcmUpdateMessage(CZSealBody *player, int msg)
{
    int r = 0;
    NET_GAME_TYPE type = data_00437CE8 ? data_00437CE8->m_type : NGT_0;

    switch (msg) {
    case 46:
        if (type != NGT_1 && type != NGT_2)
            r = 1;
        break;
    case 47:
        if (type != NGT_3 && type != NGT_4)
            r = 1;
        break;
    case 48:
        break;
    case 49:
    case 50:
    case 51:
    case 52:
    case 53: {
        const char *s = func_002C3C30(ftsGetPlayer(), msg - 49);

        func_00232170(s_tcmCurNode, s && *s ? TranslateString(s) : "<NONE>", 32);
        break;
    }
    case 54:
    case 55:
    case 56:
    case 57:
    case 58: {
        const char *s = func_002C3B60(ftsGetPlayer(), msg - 54);

        func_00232170(s_tcmCurNode, s && *s ? TranslateString(s) : "<NONE>", 32);
        break;
    }
    }
    return r;
}

int tcmCharacterTaunt(CZSealBody *player, int arg)
{
    if (!s_tcmCurNode->IsWaiting()) {
        R_SEND_TO to = RST_ALL;
        int msg;

        if (func_002C31D0(player))
            to = RST_SIDE1;
        else if (func_002C30B0(player))
            to = RST_SIDE2;
        msg = rand() % 7 + 59;
        SendMessageMessage(player->m_id, to, msg);
        PlayRemoteMsg(player, msg, 0);
        func_00232240(s_tcmCurNode, 5.0f);
    }
    return 0;
}

int tcmSetChannel(CZSealBody *player, int channel)
{
    if (theNetwork.m_bNetwork && channel >= 0 && channel < 3) {
        int set = func_002C2C20(channel);

        if (set >= 0 && set < 3)
            CSealUnit::GetUnitByTeam(24)->m_channel = set;
    }
    return 0;
}

int tcmUpdateRadioOnOff(CZSealBody *player, int arg)
{
    s_tcmCurNode->m_text.SetString(func_0030E540(data_0045B718) ? "RADIO OFF" : "RADIO ON", true);
    return 0;
}

int tcmRadioOnOff(CZSealBody *player, int arg)
{
    if (func_0030E540(data_0045B718))
        func_0030E490(data_0045B718);
    else if (func_002C2FA0())
        func_002C2C20(10);
    else
        func_002C2C20(0);
    return 0;
}

int tcmUpdateRadio(CZSealBody *player, int arg)
{
    int r = 1;

    if (player && player->m_socom2_e1_4)
        r = 0;
    else if (theNetwork.m_bNetwork && func_002C2FA0())
        r = 0;
    return r;
}

int tcmUpdatePlayer(CZSealBody *player, int index)
{
    int r = 1;
    int n = 0;
    std::list<CEntity *>::const_iterator it = tcmEntities().begin();
    std::list<CEntity *>::const_iterator e = tcmEntities().end();

    s_tcmTarget = 0;
    for (; it != e && !s_tcmTarget; ++it) {
        CZSealBody *seal = (CZSealBody *)*it;

        if (seal && seal->m_type == 2 && seal != player && (seal->m_teamflags & player->m_teamflags) &&
            !(seal->m_teamflags & 0x20000)) {
            if (n == index)
                s_tcmTarget = seal;
            else
                n++;
        }
    }
    if (s_tcmTarget) {
        r = 0;
        s_tcmCurNode->m_text.SetString(s_tcmTarget->m_name, true);
    }
    return r;
}

int tcmSelectPlayer(CZSealBody *player, int index)
{
    int n = 0;
    std::list<CEntity *>::const_iterator it = tcmEntities().begin();
    std::list<CEntity *>::const_iterator e = tcmEntities().end();

    s_tcmTarget = 0;
    for (; it != e && !s_tcmTarget; ++it) {
        CZSealBody *seal = (CZSealBody *)*it;

        if (seal && seal->m_type == 2 && seal != player && (seal->m_teamflags & player->m_teamflags) &&
            !(seal->m_teamflags & 0x20000) && !func_002C2FD0(seal)) {
            if (n == index)
                s_tcmTarget = seal;
            else
                n++;
        }
    }
    return 1;
}

int tcmUpdateMute(CZSealBody *player, int arg)
{
    int r = 1;

    if (s_tcmTarget) {
        r = 0;
        bool muted = func_0030E550(data_0045B718, s_tcmTarget->m_persona);

        s_tcmCurNode->m_text.SetString(muted ? "MUTE ON" : "MUTE OFF", true);
    }
    return r;
}

int tcmMute(CZSealBody *player, int arg)
{
    if (s_tcmTarget) {
        CZNetPersona *persona = s_tcmTarget->m_persona;
        bool muted = func_0030E550(data_0045B718, persona);

        func_0030E580(data_0045B718, persona, !muted);
    }
    return 0;
}

int tcmUpdateVote(CZSealBody *player, int arg)
{
    int r = 1;

    if (s_tcmTarget) {
        r = 0;
        bool retain = func_002C3980(s_tcmTarget->m_persona);

        s_tcmCurNode->m_text.SetString(retain ? "VOTE RETAIN" : "VOTE REMOVE", true);
    }
    return r;
}

int tcmVote(CZSealBody *player, int arg)
{
    if (s_tcmTarget) {
        CZNetPersona *persona = s_tcmTarget->m_persona;
        bool cur = func_002C3980(persona);
        bool remove = !cur;

        func_002C39D0(persona, remove);
        SendVoteMessage(ftsGetPlayer()->GetId(), s_tcmTarget->GetId(), remove);
    }
    return 0;
}

void tcmResetVoteAll()
{
    s_tcmVoteAll = false;
}

void tcmSetRecoMode(bool on)
{
    s_tcmRecoMode = on;
}

void tcmRecoCancel()
{
    CZSealBody *player = ftsGetPlayer();
    CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

    if (unit && player && player->m_socom2_e1_4)
        func_005EAD70(unit, 0, 0, 0);
}

int tcmUpdateVoteAll(CZSealBody *player, int arg)
{
    int r = 1;
    int n = 0;
    std::list<CEntity *>::const_iterator it = tcmEntities().begin();
    std::list<CEntity *>::const_iterator e = tcmEntities().end();

    for (; it != e && !s_tcmTarget; ++it) {
        if ((*it)->m_type == 2 && func_002C2FD0((CZSealBody *)*it))
            n++;
    }
    if (n) {
        r = 0;
        s_tcmCurNode->m_text.SetString(s_tcmVoteAll ? "VOTE ALL RETAIN" : "VOTE ALL REMOVE", true);
    }
    return r;
}

int tcmVoteAll(CZSealBody *player, int arg)
{
    std::list<CEntity *>::const_iterator it = tcmEntities().begin();
    std::list<CEntity *>::const_iterator e = tcmEntities().end();

    s_tcmVoteAll = !s_tcmVoteAll;
    for (; it != e && !s_tcmTarget; ++it) {
        CZSealBody *seal = (CZSealBody *)*it;

        if (seal->m_type == 2 && func_002C2FD0(seal)) {
            CZNetPersona *persona = seal->m_persona;
            bool remove = func_002C3980(persona);

            if (remove != s_tcmVoteAll) {
                func_002C39D0(persona, s_tcmVoteAll);
                func_002B9B60(ftsGetPlayer()->m_id, s_tcmVoteAll);
            }
        }
    }
    return 0;
}

int tcmNodeAction(CZSealBody *player, int arg)
{
    if (s_tcmRecoMode) {
        CSealUnit *unit = CSealUnit::GetUnitByTeam(s_tcmTeam);

        if (unit && player && player->m_socom2_e1_4)
            func_005EAD70(unit, 1, 0, 0);
        return 1;
    }
    CNodeAction *action = s_tcmCurNode->m_action;
    CSealCtrl *ctrl = player ? player->m_ctrl : 0;

    if (ctrl && action && ctrl->CanPerformAction(action) && action->m_kind != 4) {
        action->ExecuteAction(player, 0);
        player->m_lastaction = action->m_id;
    }
    return 0;
}

int tcmUpdateNodeAction(CZSealBody *player, int arg)
{
    CNodeAction *action = s_tcmCurNode->m_action;
    int r = 1;
    CNodeAction *avail = func_005AA240(player, 0);

    if (avail && action && (bool)avail->m_enabled && (bool)action->m_enabled) {
        CSealCtrl *ctrl = player ? player->m_ctrl : 0;

        if (ctrl && action && ctrl->CanPerformAction(action)) {
            bool ok = (action->m_teams & player->m_teamflags) != 0;

            if (ok)
                ok = func_002B46F0(action) != 0;
            if (ok)
                r = 0;
        }
    }
    return r;
}

int tcmNiceOne(CZSealBody *player, int arg)
{
    func_002B6530(data_004366A0, "COMPLIMENT", 0, 0, 0, 0.0f);
    CSealUnit *unit = CSealUnit::GetUnitByTeam(1);

    if (unit)
        func_005EAF20(unit, 76, 0, 1.0f);
    return 0;
}

int tcmSayAgain(CZSealBody *player, int arg)
{
    CTCMRepeat *last = func_0034A620();

    if (last && last->m_target)
        last->m_target->Repeat(0, last->m_arg);
    return 0;
}

int tcmWhassup(CZSealBody *player, int arg)
{
    CSealUnit *unit = CSealUnit::GetUnitByTeam(1);

    if (unit)
        func_005EAF20(unit, 77, 0, 1.0f);
    return 0;
}

int tcmReportStatus(CZSealBody *player, int arg)
{
    CSealUnit *unit = CSealUnit::GetUnitByTeam(1);

    if (unit)
        func_005EAEB0(unit);
    return 0;
}

int tcmCueScaryMusic(CZSealBody *player, int arg)
{
    CSnd *snd = CSnd::GetSoundByName("MUS_SCARY");

    if (snd)
        CSndJukebox::Play(MUSIC_MODE_3, snd);
    return 0;
}
