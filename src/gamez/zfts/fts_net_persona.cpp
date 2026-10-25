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

class CZNetwork {
public:
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

    unsigned char _0[0xac];
    int m_persona_object;   // 0xac: this client's persona object (the name ours)
    unsigned char _b0[0x114 - 0xb0];
    MP_MAJOR_GAME_STATE m_my_major_game_state;   // 0x114 (SOCOM 1's at 0xdc)
    MP_MINOR_GAME_STATE m_my_minor_game_state;   // 0x115 (SOCOM 1's at 0xdd)
};

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

/* the online lobby */
class CZOnlineLobby;

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
int func_002C4810();
int func_002C4A20();
int strcmp(const char *a, const char *b);
unsigned char func_002EAEE0(CZOnlineLobby *lobby, int a);   /* (the lobby's: the persona's byte at 0x42) */
}

/* the lobby's account of this player (0xF0 bytes, filled by 0x2F98F0: only the word at 4 read
 * here; the names ours) */
struct LobbyPlayerInfo {
    int m_0;
    int m_4;
    unsigned char _8[0xf0 - 0x8];
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
    data->m_42 = func_002EAEE0(&theLobby, -1);
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
