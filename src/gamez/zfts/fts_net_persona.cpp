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

    MP_MINOR_GAME_STATE MyMinorGameState(MP_MINOR_GAME_STATE state);

    unsigned char _0[0x115];
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
    unsigned char _0[0xfc8];
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
    unsigned char m_40;   // 0x40
    unsigned char m_41;   // 0x41
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

/* whether a team is the terrorists' (the name ours: inlined only, its value read ahead of the swap
 * flag as func_002C31A0 reads it) */
inline bool IsTerroristTeam(unsigned int team)
{
    unsigned int t = team;

    return t == (data_004412D8 ? TEAM_SEALS : TEAM_TERRORISTS);
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

/* every client's persona forgotten (OVL1 0x2C2BB0) */
void CZPersonaState::ClearPersonaList()
{
    for (int i = 0; i < 24; i++) {
        m_PersonaList[i].m_index = -1;
        m_PersonaList[i].m_data = 0;
    }
}
