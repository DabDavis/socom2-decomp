/* fts_net_persona.cpp -- the players' personas: CZPersonaState (SOCOM 1's fts_net_persona.cpp: the
 * network objects of kind "PersonaObject", the persona list, the teams and the ready flags). OVL1
 * from 0x2C2BB0 (ClearPersonaList opens it, as SOCOM 1's does), after the blather state's unit.
 * SOCOM II rewrote it: the list is a table of 24 clients (SOCOM 1's a list<int>), and the teams
 * swap sides (the two team masks trade places when the swap flag is set). Only some of it so far.
 * Defined in reverse of their image order (-inline deferred). */

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

class CZNetwork {
public:
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
    unsigned int m_team;   // 0x0: its team's mask
    int m_4;   // 0x4
    int m_8;   // 0x8
    unsigned char _c[2];
    char m_name[0x2e - 0xe];   // 0xe
    char m_clan[0x43 - 0x2e];   // 0x2e
    signed char m_43;   // 0x43
    unsigned char _44[4];
    unsigned char m_48;   // 0x48
    unsigned char m_49;   // 0x49
    unsigned char m_4a;   // 0x4a: ready
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
void func_002C4500();                       /* (the teams counted) */
bool func_002C2FA0();
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

/* whether the local player is a terrorist, or a SEAL (OVL1 0x2C31A0 and 0x2C3080: the team read
 * ahead of the swap flag -- a local, or the SEALs' test inlined; read in the comparison it comes
 * after the flag) */
extern "C" bool func_002C31A0()
{
    unsigned int team = data_00441470;

    return team == (data_004412D8 ? TEAM_SEALS : TEAM_TERRORISTS);
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
