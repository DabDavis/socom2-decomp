/* fts_net_persona.cpp -- the players' personas: CZPersonaState (SOCOM 1's fts_net_persona.cpp: the
 * network objects of kind "PersonaObject", the persona list, the teams and the ready flags). OVL1
 * from 0x2C2BB0 (ClearPersonaList opens it, as SOCOM 1's does), after the blather state's unit.
 * SOCOM II rewrote it: the list is a table of 24 clients (SOCOM 1's a list<int>), and the teams
 * swap sides (the two team masks trade places when the swap flag is set). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

/* a network object as CZNetwork's table hands it out (only its kind's first letter and its
 * data) */
struct NetObject {
    unsigned char _0[0xc];
    char m_type;   // 0xc: its kind's name's first letter ('B' a blather object)
    unsigned char _d[0x1c - 0xd];
    void *m_data;   // 0x1c
};

/* a blather object's data: a player's five standard-operations messages and five taunts, 46
 * characters each (the MPLOCALSOP and MPTAUNTMSG UI variables'; the names ours) */
struct BlatherData {
    char m_sop[5][46];   // 0x0
    char m_taunt[5][46];   // 0xe6
};

/* a valve (zanim.h's CValve: its value, a 16-bit field, and the callbacks told of a change) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
};

/* CValve::SetValue, inlined in both builds (as fts_mission.cpp spells it) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* a ZRDR value and the editor that builds one (zrdr.h's, zonline.h's layout), and a UI variable
 * (zui.h's: only its set) */
struct _zrdr {
    unsigned int m_0;   // 0x0
    void *m_4;   // 0x4
};

class CRdrEditor : public _zrdr {
public:
    CRdrEditor();
    ~CRdrEditor();
    bool push(const char *tag);
    bool pop();
    bool addstringnode(const char *s);

    unsigned char _8[0x4c - 0x8];
};

class CUIVariable {
public:
    void Set(_zrdr *value);
};

class CZNetwork {
public:
    unsigned int zNetGetTime();
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    int zNetGetMyGameClientIndex();
    NetObject *zNetGetObject(int index);

    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_1;   // 0x1: an online game
};

extern CZNetwork theNetwork;

/* the online game (only this client's minor state) */
class CZNetGame {
public:
    enum MP_MINOR_GAME_STATE {
        MP_MINOR_UNKNOWN = 0,
        MP_MINOR_NOTREADY = 1,
        MP_MINOR_READY = 2
    };

    /* (SOCOM 1's DWARF: only the values this unit names) */
    enum MP_MAJOR_GAME_STATE {
        MP_MAJOR_UNKNOWN = 0,
        MP_MAJOR_GHOST = 14
    };

    MP_MINOR_GAME_STATE MyMinorGameState(MP_MINOR_GAME_STATE state);

    unsigned char _0[0x14];
    CValve *m_pPlayerTeamValve;   // 0x14 (SOCOM 1's at 0x10)
    unsigned char _18[0x48 - 0x18];
    CValve *m_pPlayerReadyCountValve;   // 0x48 (SOCOM 1's at 0x44)
    unsigned char _4c[0x60 - 0x4c];
    CValve *m_pTeamFullValve;   // 0x60: set while either side has more than eight (SOCOM II's; the name ours)
    unsigned char _64[0x84 - 0x64];
    CUIVariable *m_uiv_SealList;   // 0x84 (SOCOM 1's four at 0x50-0x5c)
    CUIVariable *m_uiv_TerrList;   // 0x88
    CUIVariable *m_uiv_NoneList;   // 0x8c
    CUIVariable *m_uiv_ChatList;   // 0x90
    unsigned char _94[0xac - 0x94];
    int m_persona_object;   // 0xac: this client's persona object (the name ours)
    unsigned char _b0[0xd8 - 0xb0];
    bool m_ten_second_launch;   // 0xd8 (SOCOM 1's at 0x9f)
    unsigned char _d9[0xe4 - 0xd9];
    int m_launch_time;   // 0xe4 (SOCOM 1's at 0xa8)
    unsigned char _e8[0x113 - 0xe8];
    MP_MAJOR_GAME_STATE m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
    MP_MAJOR_GAME_STATE m_my_major_game_state;   // 0x114 (SOCOM 1's at 0xdc)
    MP_MINOR_GAME_STATE m_my_minor_game_state;   // 0x115 (SOCOM 1's at 0xdd)
};

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x18f4 - 0x180c];
    unsigned char m_e3_team;   // 0x18f4 (SOCOM 1's E3_TEAMS_ENUM at 0x660, a byte: 1 the terrorists, 2 the SEALs)
};

extern CMission theMission;

/* the online lobby */
/* the online lobby (zonline's: only a rank's level, the persona's byte at 0x42) */
class CZOnlineLobby {
public:
    unsigned char GetRankLevel(int rank);   /* 0x2EAEE0 */
};

extern CZOnlineLobby theLobby;

/* the SEAL's body (only the client it belongs to) */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_e1_4 : 1;   // 0xe1, bit 4: clear on a player who has not spawned (the name ours)
    unsigned char _e2[0xfc8 - 0xe2];
    int m_fc8;   // 0xfc8 (SOCOM 1's at 0xeb8)
};

/* a persona's network data (only what this unit touches) */
struct PersonaData {
    /* the flag 0x10000 of the team word (the name ours; a bool inline: the test materialises) */
    bool IsFlagged() const { return (m_team & 0x10000) != 0; }

    unsigned int m_team;   // 0x0: its team's mask
    int m_4;   // 0x4
    int m_8;   // 0x8
    unsigned short m_c;   // 0xc: its blather object
    char m_name[32];   // 0xe
    char m_clan[16];   // 0x2e
    signed char m_3e;   // 0x3e
    unsigned char m_3f;   // 0x3f
    CZNetGame::MP_MAJOR_GAME_STATE m_40;   // 0x40: the client's major and minor game states
    CZNetGame::MP_MINOR_GAME_STATE m_41;   // 0x41
    unsigned char m_42;   // 0x42
    signed char m_43;   // 0x43
    unsigned char _44[4];   // (the fields from here on are not sent)
    unsigned char m_48;   // 0x48
    unsigned char m_49;   // 0x49
    unsigned char m_4a;   // 0x4a: ready
    unsigned char _4b;
};

/* a client's persona: its network object and its data */
struct PersonaEntry {
    int m_index;   // 0x0: the network object, -1 for none
    PersonaData *m_data;   // 0x4
};

class CZPersonaState {
public:
    static void ClearPersonaList();
    static bool AllReadyToLoad();
    static void CheckStateSwitch();
    static void ToggleReady();
    static void UpdatePersonaLists();
    static void UpdateLocalPersona();
    static void RemotePersonaDeletionCallback(int client, int index);
    static void RemotePersonaPostUpdateCallback(int client, int index, int field);
    static void RemotePersonaUpdateCallback(int client, int index, int field);
    static void RemotePersonaCreationCallback(int client, int index);
    static void zNetRegisterPersona();

    static int m_NetStructIndexPersonaObject;

    /* a client's persona data (the name ours; inlined everywhere) */
    static PersonaData *GetPersona(int client)
    {
        if (client >= 0 && client < 24)
            return m_PersonaList[client].m_data;
        return 0;
    }

    static PersonaEntry m_PersonaList[24];
};

/* the two teams' masks (the SEALs' and the terrorists'), traded when the sides swap */
enum {
    TEAM_SEALS = 0x80000100,
    TEAM_TERRORISTS = 0x40000001
};

class CZSealBody;

CZSealBody *ftsGetPlayer();

extern "C" {
extern unsigned char data_004412D8;   /* the sides swapped */
extern int data_00441448;             /* (the two teams' counts, func_002C4500's) */
extern int data_00441450;
extern int data_00441458;
extern int data_00441460;             /* (the personas on neither side, unflagged) */
extern int data_00441468;
extern unsigned int data_00441470;    /* (the local player's team) */

bool func_002F6D00(CZOnlineLobby *lobby);   /* (a CZOnlineLobby flag: a valve's, or a byte) */

/* the twelve fields' ids */
extern int data_004413E8;
extern int data_004413F0;
extern int data_004413F8;
extern int data_00441400;
extern int data_00441408;
extern int data_00441410;
extern int data_00441418;
extern int data_00441420;
extern int data_00441428;
extern int data_00441430;
extern int data_00441438;
extern int data_00441440;

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD0A0();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD0E0();
void func_002C4500();                       /* (the teams counted) */
bool func_002C2FA0();
bool func_002C3300(unsigned int team);      /* (whether a team is the SEALs') */
unsigned int func_002C32E0();               /* (the terrorists' team mask) */
unsigned int func_002C32C0();               /* (the SEALs' team mask) */
/* fts_remote.cpp's NetSendE3Index under the map's name: SOCOM II's takes three arguments (its fourth,
 * SOCOM 1's bool, is never read, and the team switch below leaves a3 as it is) */
void NetSendE3Index__FiUii(int who, unsigned int index, int flag);
void func_002C4440();
bool func_002A47B0(CMission *mission, const char *dialog);   /* (fts_mission.cpp's: a dialog over the mission) */
int func_002C4810();
int func_002C4A20();
int strcmp(const char *a, const char *b);
}

/* the lobby's account of this player (0xE8 bytes, filled by 0x2F98F0 -- its last copy ends at
 * 0xE1 -- and st_mpexit.cpp's CareerRecord, 0xE8 by two stack frames; UpdateLocalPersona's frame
 * is the same at 0xF0. Only the word at 4 read here; the names ours) */
struct LobbyPlayerInfo {
    int m_0;
    int m_4;
    unsigned char _8[0xe8 - 0x8];
};

extern "C" void func_002F98F0(LobbyPlayerInfo *info);

/* whether a team is the terrorists' (the name ours: inlined only, its value read ahead of the swap
 * flag as func_002C31A0 reads it) */
inline bool IsTerroristTeam(unsigned int team)
{
    unsigned int t = team;

    return t == (data_004412D8 ? TEAM_SEALS : TEAM_TERRORISTS);
}


/* the kind registered: its twelve fields, their offsets taken off a pointer to a local, as
 * fts_net_grenade.cpp's (OVL1 0x2C5A10) */
void CZPersonaState::zNetRegisterPersona()
{
    PersonaData d;
    PersonaData *p = &d;

    NetRegisterObjectStart();
    NetObjectField(&data_00441400, (int)&p->m_team - (int)p, 4, 5, 1, func_002BD0A0());
    NetObjectField(&data_00441440, (int)&p->m_4 - (int)p, 4, 4, 1, func_002BD0A0());
    NetObjectField(&data_00441420, (int)&p->m_8 - (int)p, 4, 4, 1, func_002BD0E0());
    NetObjectField(&data_00441438, (int)&p->m_c - (int)p, 2, 3, 1, func_002BD0E0());
    NetObjectField(&data_00441410, (int)&p->m_name - (int)p, 1, 1, 32, func_002BD0A0());
    NetObjectField(&data_00441418, (int)&p->m_clan - (int)p, 1, 1, 16, func_002BD0E0());
    NetObjectField(&data_00441408, (int)&p->m_3e - (int)p, 1, 0, 1, func_002BD0A0());
    NetObjectField(&data_00441428, (int)&p->m_3f - (int)p, 1, 1, 1, func_002BD0E0());
    NetObjectField(&data_004413E8, (int)&p->m_40 - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004413F0, (int)&p->m_41 - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004413F8, (int)&p->m_42 - (int)p, 1, 1, 1, func_002BD0E0());
    NetObjectField(&data_00441430, (int)&p->m_43 - (int)p, 1, 0, 1, func_002BD0E0());
    NetRegisterStructure(&m_NetStructIndexPersonaObject, "PersonaState", sizeof(PersonaData));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexPersonaObject,
                                                RemotePersonaCreationCallback,
                                                RemotePersonaUpdateCallback,
                                                RemotePersonaPostUpdateCallback,
                                                RemotePersonaDeletionCallback);
}

/* nothing on an update (OVL1 0x2C5800: its data's load kept, as in fts_net_blather.cpp) */
void CZPersonaState::RemotePersonaUpdateCallback(int client, int index, int field)
{
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj)
        void *data = *(void *volatile *)&obj->m_data;
}

/* after an update: the lists rebuilt and our persona sent (OVL1 0x2C57D0, as SOCOM 1's) */
void CZPersonaState::RemotePersonaPostUpdateCallback(int client, int index, int field)
{
    UpdatePersonaLists();
    UpdateLocalPersona();
}

/* a client's persona gone: its entry cleared, and for another client's the lists rebuilt and the
 * state switch checked (OVL1 0x2C5750) */
void CZPersonaState::RemotePersonaDeletionCallback(int client, int index)
{
    if (theNetwork.zNetGetObject(index)) {
        m_PersonaList[client].m_index = -1;
        m_PersonaList[client].m_data = 0;
        if (client != theNetwork.zNetGetMyGameClientIndex()) {
            UpdatePersonaLists();
            CheckStateSwitch();
        }
    }
}

extern "C" int data_00441480[8];
extern "C" int data_004414A0[8];

/* this client's persona brought up to date: the state switch checked, the game's states, the
 * lobby's byte (4 for one name), the team, the slot and the lobby's word copied in, and the lists
 * rebuilt (OVL1 0x2C5360; pinned: the post-update callback calls it, where -inline auto would take
 * it whole) */
#pragma auto_inline off
void CZPersonaState::UpdateLocalPersona()
{
    if (!theNetwork.m_1)
        return;
    CZNetGame *game = theMission.m_pNetGame;

    if (!game)
        return;
    NetObject *obj = theNetwork.zNetGetObject(game->m_persona_object);

    if (!obj)
        return;
    PersonaData *data = (PersonaData *)obj->m_data;

    if (!data)
        return;
    CheckStateSwitch();
    data->m_40 = game->m_my_major_game_state;
    data->m_41 = game->m_my_minor_game_state;
    data->m_42 = theLobby.GetRankLevel(-1);
    if (!strcmp(data->m_name, "oop9"))
        data->m_42 = 4;
    data->m_team = data_00441470;
    data->m_3e = data_00441468;

    LobbyPlayerInfo info;

    func_002F98F0(&info);
    data->m_8 = info.m_4;
    UpdatePersonaLists();
}
#pragma auto_inline reset

/* a client's slot taken ahead of its persona (24 of them at 0x441580: the slot, the side -- 1 the
 * terrorists, 2 the SEALs -- and whether it is held; the names ours) */
enum PendingSide {
    SIDE_NONE,
    SIDE_TERRORISTS,
    SIDE_SEALS,
    SIDE_3
};

struct PendingSlot {
    unsigned char slot : 3;
    unsigned char side : 3;   // a PendingSide: compared as the byte enum (the target's andi 0xff)
    unsigned char held : 2;
};

extern "C" PendingSlot data_00441580[24];

/* a client moved to the other side when that side has a slot free: its slot and team taken there,
 * the switch sent and the counts moved (OVL1 0x2C4290; the session master's answer to a team
 * switch, fts_remote.cpp's NetSendRequestTeamSwitch) */
extern "C" void func_002C4290(int client)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (!p)
        return;
    func_002C4440();
    if (IsTerroristTeam(p->m_team) && data_00441450 < 8) {
        signed char slot = func_002C4810();

        p->m_3e = slot;
        p->m_team = func_002C32C0();
        NetSendE3Index__FiUii(client, func_002C32C0(), slot);
        data_00441450++;
        data_00441448--;
    } else if (func_002C3300(p->m_team) && data_00441448 < 8) {
        signed char slot = func_002C4A20();

        p->m_3e = slot;
        p->m_team = func_002C32E0();
        NetSendE3Index__FiUii(client, func_002C32E0(), slot);
        data_00441450--;
        data_00441448++;
    }
}

/* whether a team is the SEALs' (func_002C3300 inline: the value read ahead of the swap flag) */
inline bool IsSealTeam(unsigned int team)
{
    unsigned int t = team;

    return t == (data_004412D8 ? TEAM_TERRORISTS : TEAM_SEALS);
}

/* a team word's flag 0x10000 (ours: PersonaData::IsFlagged on the word, which the three tests then
 * read once -- through the member each reloads it -- and compared `!= false`, so materialised) */
inline bool IsFlaggedTeam(unsigned int team)
{
    return (team & 0x10000) != 0;
}

extern "C" unsigned int strlen(const char *s);

/* the online game's three player lists from the host's packed names -- the SEALs', the terrorists'
 * and the rest's, each name null-terminated after the last -- and an empty chat list, into their
 * UI variables (SOCOM II's; the packet handler 0x2B9B20 hands it the counts and the names; OVL1
 * 0x2C4DF0) */
extern "C" void func_002C4DF0(int nseals, int nterrs, int nnone, const char *names)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game) {
        CRdrEditor seals;
        CRdrEditor terrs;
        CRdrEditor none;
        CRdrEditor chat;
        int i;

        seals.push(0);
        terrs.push(0);
        none.push(0);
        chat.push(0);
        for (i = 0; i < nseals; i++) {
            seals.push(0);
            seals.addstringnode(names);
            seals.pop();
            names += strlen(names) + 1;
        }
        for (i = 0; i < nterrs; i++) {
            terrs.push(0);
            terrs.addstringnode(names);
            terrs.pop();
            names += strlen(names) + 1;
        }
        for (i = 0; i < nnone; i++) {
            none.push(0);
            none.addstringnode(names);
            none.pop();
            names += strlen(names) + 1;
        }
        seals.pop();
        terrs.pop();
        none.pop();
        chat.pop();
        game->m_uiv_SealList->Set(&seals);
        game->m_uiv_TerrList->Set(&terrs);
        game->m_uiv_NoneList->Set(&none);
        game->m_uiv_ChatList->Set(&chat);
    }
}

/* whether a persona is the terrorists', or the SEALs' (ours: a null persona neither -- the test
 * materialised, the team read after the swap flag) */
inline bool IsPersonaTerrorist(PersonaData *p)
{
    return p ? IsTerroristTeam(p->m_team) : false;
}

inline bool IsPersonaSeal(PersonaData *p)
{
    return p ? IsSealTeam(p->m_team) : false;
}

inline bool IsPersonaFlagged(PersonaData *p);

extern "C" int sprintf(char *buf, const char *format, ...);

/* the fts_remote.cpp sender of the packed lists (OVL1 0x2B9A40) */
extern "C" void func_002B9A40(int to, int a, int b, int c, int size, const char *names);

/* the host's side of func_002C4DF0: the terrorists', the SEALs' and the flagged personas' names
 * packed one after another, each null-terminated, and sent to a client with the three counts
 * (SOCOM II's; 0x2BC620 calls it; OVL1 0x2C4B50) */
extern "C" void func_002C4B50(int client)
{
    char names[24 * 32 + 1];
    char *dst = names;
    int size = 0;
    int nterrs = 0;
    int nseals = 0;
    int i;
    int nflagged = 0;

    for (i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && IsPersonaTerrorist(p)) {
            sprintf(dst, "%s", p->m_name);
            nterrs++;
            int len = strlen(p->m_name);
            dst[len] = 0;
            size += len + 1;
            dst += len + 1;
        }
    }
    for (i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && IsPersonaSeal(p)) {
            sprintf(dst, "%s", p->m_name);
            nseals++;
            int len = strlen(p->m_name);
            dst[len] = 0;
            size += len + 1;
            dst += len + 1;
        }
    }
    for (i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && IsPersonaFlagged(p)) {
            sprintf(dst, "%s", p->m_name);
            nflagged++;
            int len = strlen(p->m_name);
            dst[len] = 0;
            size += len + 1;
            dst += len + 1;
        }
    }
    func_002B9A40(client, nterrs, nseals, nflagged, size, names);
}

/* the sides counted -- the terrorists, the SEALs, the flagged personas and the rest -- the
 * team-full valve set while either side has more than eight, and a game the host is left alone in
 * with only flagged players abandoned (SOCOM II's; OVL1 0x2C4500) */
extern "C" void func_002C4500()
{
    data_00441448 = data_00441450 = data_00441458 = data_00441460 = 0;
    for (int i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p) {
            if (IsTerroristTeam(p->m_team))
                data_00441448++;
            else if (IsSealTeam(p->m_team))
                data_00441450++;
            else if (IsFlaggedTeam(p->m_team) != false)
                data_00441458++;
            else
                data_00441460++;
        }
    }
    CZNetGame *game = theMission.m_pNetGame;

    if (data_00441448 > 8 || data_00441450 > 8)
        SetValveValue(game->m_pTeamFullValve, 1);
    else
        SetValveValue(game->m_pTeamFullValve, 0);
    if (data_00441448 + data_00441450 == 0 && game->m_mp_major_game_state >= 2 &&
        game->m_mp_major_game_state < 6 && theNetwork.m_bSessionMaster && data_00441458)
        func_002A47B0(&theMission, "dlgNetAbandoned.rdr");
}

/* the sides counted, the slots held ahead of their personas added (OVL1 0x2C4440) */
extern "C" void func_002C4440()
{
    func_002C4500();
    for (int i = 0; i < 24; i++) {
        PendingSlot &s = data_00441580[i];
        PendingSide side = (PendingSide)s.side;

        if (s.held) {
            if (side == SIDE_TERRORISTS)
                data_00441448++;
            else if (side == SIDE_SEALS)
                data_00441450++;
            else if (side == SIDE_3)
                data_00441458++;
        }
    }
}

/* each side's first empty slot, the slots held ahead of their personas counted too -- the
 * terrorists' and the SEALs' (OVL1 0x2C4940 and 0x2C4730) */
extern "C" int func_002C4940()
{
    int i;

    func_002C4A20();
    for (i = 0; i < 24; i++) {
        PendingSlot &s = data_00441580[i];

        if (s.held && (PendingSide)s.side == SIDE_TERRORISTS && s.slot > -1 && s.slot < 8)
            data_00441480[s.slot]++;
    }
    for (i = 0; i < 8; i++)
        if (!data_00441480[i])
            return i;
    return -1;
}

extern "C" int func_002C4730()
{
    int i;

    func_002C4810();
    for (i = 0; i < 24; i++) {
        PendingSlot &s = data_00441580[i];

        if (s.held && (PendingSide)s.side == SIDE_SEALS && s.slot > -1 && s.slot < 8)
            data_004414A0[s.slot]++;
    }
    for (i = 0; i < 8; i++)
        if (!data_004414A0[i])
            return i;
    return -1;
}

/* each side's eight slots: the personas in each counted, the first empty one returned, -1 when all
 * are taken -- the terrorists' and the SEALs' (OVL1 0x2C4A20 and 0x2C4810; the counts at 0x441480 and
 * 0x4414A0) */
extern "C" int func_002C4A20()
{
    int i;

    for (i = 0; i < 8; i++)
        data_00441480[i] = 0;
    for (i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && IsTerroristTeam(p->m_team) && p->m_3e > -1 && p->m_3e < 8)
            data_00441480[p->m_3e]++;
    }
    for (i = 0; i < 8; i++)
        if (!data_00441480[i])
            return i;
    return -1;
}

extern "C" int func_002C4810()
{
    int i;

    for (i = 0; i < 8; i++)
        data_004414A0[i] = 0;
    for (i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && func_002C3300(p->m_team) && p->m_3e > -1 && p->m_3e < 8)
            data_004414A0[p->m_3e]++;
    }
    for (i = 0; i < 8; i++)
        if (!data_004414A0[i])
            return i;
    return -1;
}

/* the menu's event handler (the lobby's: an event's name and its argument) and whether the eighty
 * per cent warning was given (a byte of the lobby's; OVL1 0x2E99E0) */
extern "C" bool (*data_0044F3C8)(const char *event, const char *arg);
extern "C" bool func_002E99E0();

/* a persona flagged 0x10000, a null one not (ours) */
inline bool IsPersonaFlagged(PersonaData *p)
{
    return p ? p->IsFlagged() : false;
}

/* whether every persona is ready to load -- at once when the ten-second launch has run out; the
 * ready count given to its valve, and the menu told when eighty per cent of the teams are ready
 * (SOCOM 1's AllReadyToLoad, CheckStateSwitch's call: SOCOM II skips the flagged personas and adds
 * the warning; OVL1 0x2C40C0) */
/* the ten-second launch still counting down (ours: through an inline, its result materialised --
 * `sltu v0` where the test written in the `if` compares into at -- and the launch time read ahead of
 * the clock, as SOCOM 1's AllReadyToLoad has it) */
inline bool LaunchPending(CZNetGame *game)
{
    int t = game->m_launch_time;

    return theNetwork.zNetGetTime() < t;
}

bool CZPersonaState::AllReadyToLoad()
{
    bool ready;
    int count;
    CZNetGame *game = theMission.m_pNetGame;

    if (game->m_ten_second_launch && !LaunchPending(game))
        return true;
    ready = true;
    count = 0;

    for (int i = 0; i < 24; i++) {
        PersonaData *p = GetPersona(i);

        if (p && !IsPersonaFlagged(p)) {
            if (p->m_41 != CZNetGame::MP_MINOR_READY)
                ready = false;
            else
                count++;
        }
    }
    SetValveValue(game->m_pPlayerReadyCountValve, count);
    if (!game->m_ten_second_launch && !ready && data_00441448 && data_00441450 &&
        !(count < 0.8f * (data_00441448 + data_00441450)) && func_002E99E0() == false && data_0044F3C8)
        data_0044F3C8("MP_EIGHTY_READY", 0);
    return ready;
}

/* whether no persona without the flag 0x10000 waits in state 2 at 0x40 with 0x41 not yet 2
 * (OVL1 0x2C4020; after ToggleReady, where SOCOM 1 has AllReadyToPlay) */
extern "C" bool func_002C4020()
{
    for (int i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && p->IsFlagged() == false && p->m_40 == 2 && p->m_41 != 2)
            return false;
    }
    return true;
}

/* this client ready, or not (OVL1 0x2C3FC0; SOCOM 1's ToggleReady, the same code) */
void CZPersonaState::ToggleReady()
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game) {
        if (game->m_my_minor_game_state == CZNetGame::MP_MINOR_READY)
            game->MyMinorGameState(CZNetGame::MP_MINOR_NOTREADY);
        else
            game->MyMinorGameState(CZNetGame::MP_MINOR_READY);
        UpdateLocalPersona();
    }
}

/* one of a blather object's messages, if the object is one (the name ours: inlined in both
 * readers below, its result through a local) */
inline char *GetBlatherMessage(int index, int i, bool taunt)
{
    char *msg = 0;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (obj && obj->m_type == 'B')
        msg = taunt ? ((BlatherData *)obj->m_data)->m_taunt[i] : ((BlatherData *)obj->m_data)->m_sop[i];
    return msg;
}

/* a SEAL's player's standard-operations message i, and its taunt i (OVL1 0x2C3C30 and 0x2C3B60) */
extern "C" char *func_002C3C30(CZSealBody *body, int i)
{
    char *msg = 0;

    if (body && i < 5) {
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        if (p)
            msg = GetBlatherMessage(p->m_c, i, false);
    }
    return msg;
}

extern "C" char *func_002C3B60(CZSealBody *body, int i)
{
    char *msg = 0;

    if (body && i < 5) {
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        if (p)
            msg = GetBlatherMessage(p->m_c, i, true);
    }
    return msg;
}

/* a client's persona's name (OVL1 0x2C3B10 .. 0x2C3A70) */
extern "C" int func_002C3B10(int client)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        return p->m_4;
    return 0;
}

extern "C" const char *func_002C3AC0(int client)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        return p->m_clan;
    return "";
}

extern "C" const char *func_002C3A70(int client)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (!p)
        return 0;
    return p->m_name;
}

extern "C" void func_002C3A20(int client, unsigned char on)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        p->m_48 = on;
}

extern "C" void func_002C39D0(int client, unsigned char on)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        p->m_49 = on;
}

extern "C" unsigned char func_002C3980(int client)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        return p->m_49;
    return 0;
}

/* the personas with the flag at 0x48 set (OVL1 0x2C3920) */
extern "C" int func_002C3920()
{
    int n = 0;

    for (int i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && p->m_48)
            n++;
    }
    return n;
}

/* a SEAL's persona's word at 8 (OVL1 0x2C3500) */
extern "C" int func_002C3500(CZSealBody *body)
{
    PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

    return p ? p->m_8 : 0;
}

/* (OVL1 0x2C34F0) */
extern "C" void func_002C34F0(int value)
{
    data_00441468 = value;
}

/* the player's team taken up (OVL1 0x2C3330; the name ours): the E3 game's team over the one asked
 * for (theMission's m_e3_team: 1 the terrorists', 2 the SEALs'), kept, the net game's player team
 * valve set to it -- 0 the terrorists, 8 the SEALs, 16 a spectator (the 0x10000 bit), else 0 --
 * and the character type for it chosen (fts_mission.cpp's 0x2A74A0). Every team test is the
 * masks' inline, the valve read in each arm through the net game read once */
extern "C" void func_002A74A0(unsigned int team);

/* whether a team is the spectators' (the name ours: a bool inline, compared with false -- the
 * conversion kept, sltu before the branch; tested plainly it folds into the and) */
inline bool IsSpectatorTeam(unsigned int team) { return team & 0x10000; }

extern "C" void func_002C3330(unsigned int team)
{
    if (theMission.m_e3_team == 1)
        team = func_002C32E0();
    else if (theMission.m_e3_team == 2)
        team = func_002C32C0();
    data_00441470 = team;
    CZNetGame *game = theMission.m_pNetGame;

    if (team == func_002C32E0())
        SetValveValue(game->m_pPlayerTeamValve, 0);
    else if (team == func_002C32C0())
        SetValveValue(game->m_pPlayerTeamValve, 8);
    else if (IsSpectatorTeam(team) != false)
        SetValveValue(game->m_pPlayerTeamValve, 16);
    else
        SetValveValue(game->m_pPlayerTeamValve, 0);
    func_002A74A0(team);
}

/* whether a team is the SEALs' (with the sides swapped, the terrorists'; OVL1 0x2C3300) */
extern "C" bool func_002C3300(unsigned int team)
{
    return team == (data_004412D8 ? TEAM_TERRORISTS : TEAM_SEALS);
}

/* the terrorists' team mask, and the SEALs' (OVL1 0x2C32E0 and 0x2C32C0) */
extern "C" unsigned int func_002C32E0()
{
    return data_004412D8 ? TEAM_SEALS : TEAM_TERRORISTS;
}

extern "C" unsigned int func_002C32C0()
{
    return data_004412D8 ? TEAM_TERRORISTS : TEAM_SEALS;
}

/* whether a SEAL is on the terrorists' side: the player's own team, another's persona's (OVL1
 * 0x2C31D0) */
extern "C" bool func_002C31D0(CZSealBody *body)
{
    if (body) {
        if (ftsGetPlayer() == body)
            return IsTerroristTeam(data_00441470);
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        return p ? IsTerroristTeam(p->m_team) : false;
    }
    return false;
}

/* whether the local player is a terrorist, or a SEAL (OVL1 0x2C31A0 and 0x2C3080: the team read
 * ahead of the swap flag -- a local, or the SEALs' test inlined; read in the comparison it comes
 * after the flag) */
extern "C" bool func_002C31A0()
{
    unsigned int team = data_00441470;

    return team == (data_004412D8 ? TEAM_SEALS : TEAM_TERRORISTS);
}

/* whether a SEAL is on the SEALs' side (OVL1 0x2C30B0; ChangeBombState sets the bomb owner's valve
 * from it) */
extern "C" bool func_002C30B0(CZSealBody *body)
{
    if (body) {
        if (ftsGetPlayer() == body)
            return func_002C3300(data_00441470);
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        return p ? func_002C3300(p->m_team) : false;
    }
    return false;
}

extern "C" bool func_002C3080()
{
    return func_002C3300(data_00441470);
}

/* a SEAL's persona's flag 0x10000: the player's is the lobby's (OVL1 0x2C2FD0) */
extern "C" bool func_002C2FD0(CZSealBody *body)
{
    if (body) {
        if (ftsGetPlayer() == body)
            return func_002C2FA0();
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        return p ? (p->m_team & 0x10000) != 0 : false;
    }
    return false;
}


/* the lobby's flag for the local player (OVL1 0x2C2FA0: kept in a local, the bool is re-extended
 * after the call -- returned straight, MW tail-calls) */
extern "C" bool func_002C2FA0()
{
    bool on = func_002F6D00(&theLobby);

    return on;
}

/* a client's ready flag (OVL1 0x2C2F50) */
extern "C" void func_002C2F50(int client, unsigned char ready)
{
    PersonaData *p = CZPersonaState::GetPersona(client);

    if (p)
        p->m_4a = ready;
}

/* the personas ready (OVL1 0x2C2EF0) */
extern "C" int func_002C2EF0()
{
    int n = 0;

    for (int i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && p->m_4a)
            n++;
    }
    return n;
}

/* whether more than 60 % of the two teams are ready (OVL1 0x2C2E30: the counts read into locals
 * ahead of the loop, as the target loads them) */
extern "C" bool func_002C2E30()
{
    func_002C4500();
    int a = data_00441448;
    int b = data_00441450;
    int n = 0;

    for (int i = 0; i < 24; i++) {
        PersonaData *p = CZPersonaState::GetPersona(i);

        if (p && p->m_4a)
            n++;
    }
    return n > 0.6f * (a + b);
}

/* a SEAL's persona's byte at 0x43, -1 for none (OVL1 0x2C2DC0) */
extern "C" int func_002C2DC0(CZSealBody *body)
{
    if (body) {
        PersonaData *p = CZPersonaState::GetPersona(body->m_fc8);

        return p ? p->m_43 : -1;
    }
    return -1;
}

/* the local persona's byte at 0x43 (OVL1 0x2C2D50) */
extern "C" void func_002C2D50(signed char value)
{
    PersonaData *p = CZPersonaState::GetPersona(theNetwork.zNetGetMyGameClientIndex());

    if (p)
        p->m_43 = value;
}

/* the voice chat's object (0x45B718: its mode set, its word at 0x2C read back; the names ours) */
struct VoiceChat {
    unsigned char _0[0x2c];
    int m_2c;   // 0x2c
    unsigned char _30[0x4d - 0x30];
    unsigned char m_4d;   // 0x4d
};

extern "C" VoiceChat *data_0045B718;
extern "C" void func_0030FCD0(VoiceChat *chat, int mode, int extra);

/* the voice chat put in a mode: 3 for a player not yet in, 10 for the lobby's flag, with 4 added
 * for a SEAL on the SEALs' side when the chat's byte 0x4D is clear (OVL1 0x2C2C20; the load state
 * calls it with 0 and 3) */
extern "C" int func_002C2C20(int mode)
{
    int extra = 0;

    if (ftsGetPlayer() && !ftsGetPlayer()->m_e1_4)
        mode = 3;
    if (func_002F6D00(&theLobby) != false)
        mode = 10;
    PersonaData *p = CZPersonaState::GetPersona(theNetwork.zNetGetMyGameClientIndex());

    if (p) {
        if (mode != 9 && func_002C3300(p->m_team) && !data_0045B718->m_4d)
            extra = 4;
        func_0030FCD0(data_0045B718, mode, extra);
    }
    return data_0045B718->m_2c;
}

/* every client's persona forgotten (OVL1 0x2C2BB0) */
void CZPersonaState::ClearPersonaList()
{
    for (int i = 0; i < 24; i++) {
        m_PersonaList[i].m_index = -1;
        m_PersonaList[i].m_data = 0;
    }
}

/* the list cleared at start-up: the unit's initialiser __sinit_fts_net_persona.cpp 0x4006C0 is
 * ClearPersonaList's loop inlined, with no object of its own stored -- an empty static whose
 * constructor clears it (the class and the static ours) */
class CPersonaListInit {
public:
    CPersonaListInit() { CZPersonaState::ClearPersonaList(); }
};

static CPersonaListInit s_personaListInit;
