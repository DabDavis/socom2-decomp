/* fts_net_game.cpp -- the online game's network object: CZGameState (SOCOM 1's fts_net_game.cpp:
 * the session master's game state, "GameState", that every client reads its rounds, scores and
 * flags from). OVL1 0x2C0040-0x2C141F, after fts_net_seal.cpp and before fts_net_grenade.cpp; SOCOM
 * II adds the sides' swap (0x2C0040) and a reader of the object's halfword at 0x16 (0x2C0090) after
 * UpdateLocalGame. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

class CZNetwork {
public:
    NetObject *zNetGetObject(int index);
};

extern CZNetwork theNetwork;

/* a valve (only its value) */
class CValve {
public:
    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_name_pooled : 1;   // 0x4
    unsigned int m_pooled : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
};

enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

extern "C" void func_003519F0(CValve *valve, VALVE_STATE state);   /* CValve::MakeCallbacks */

/* CValve::SetValue, inlined in both builds (as fts_mission.cpp spells it) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        func_003519F0(valve, state);
}

/* the online game (SOCOM 1's CZNetGame, at SOCOM II's offsets: only what this unit touches) */
class CZNetGame {
public:
    enum MP_MAJOR_GAME_STATE {
        MP_MAJOR_UNKNOWN = 0,
        MP_MAJOR_MENUS = 1,
        MP_MAJOR_LOADING = 2,
        MP_MAJOR_INPLAY = 3,
        MP_MAJOR_ROUNDOVER = 4,
        MP_MAJOR_NEWROUND = 5,
        MP_MAJOR_GAMEOVER = 6,
        MP_MAJOR_PLAY = 12,
        MP_MAJOR_GAMELOBBY = 13,
        MP_MAJOR_GHOST = 14
    };

    MP_MAJOR_GAME_STATE MPMajorGameState(MP_MAJOR_GAME_STATE state);

    unsigned char _0[0x64];
    CValve *m_64;   // 0x64
    unsigned char _68[0xb0 - 0x68];
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xcd - 0xb4];
    unsigned char m_cd;   // 0xcd
    unsigned char _ce[0x11c - 0xce];
    int m_11c;   // 0x11c
};

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

/* the game's network data: 92 bytes */
struct TypeGameObject {
    unsigned char m_0[5];   // 0x0
    unsigned char _5[3];
    int m_8;   // 0x8
    int m_c;   // 0xc
    int m_10;   // 0x10
    short m_14;   // 0x14
    unsigned short m_16;   // 0x16
    unsigned int m_18_0 : 16;   // 0x18
    unsigned int m_18_16 : 1;   // 0x18, bit 16
    unsigned int m_18_17 : 15;
    unsigned char m_1c;   // 0x1c
    unsigned char m_1d;   // 0x1d
    unsigned char m_1e;   // 0x1e
    unsigned char m_1f;   // 0x1f
    unsigned char m_20;   // 0x20
    unsigned char m_21;   // 0x21
    unsigned char m_22[32];   // 0x22
    unsigned char m_42[24];   // 0x42
};

class CZGameState {
public:
    static void UpdateLocalGame(CZNetGame *game);
    static int CreateLocalGame(CZNetGame *game);
    static void RemoteGameDeletionCallback(int client, int index);
    static void RemoteGameCreationCallback(int client, int index);
    static void RemoteGamePostUpdateCallback(int client, int index, int field);
    static void RemoteGameUpdateCallback(int client, int index, int field);
    static void zNetRegisterGame();

    static int m_NetStructIndexGameObject;
};

class CZPersonaState {
public:
    static void UpdateLocalPersona();
};

extern "C" {
/* the fifteen fields' ids */
extern int data_00441258;
extern int data_00441260;
extern int data_00441268;
extern int data_00441270;
extern int data_00441278;
extern int data_00441280;
extern int data_00441288;
extern int data_00441290;
extern int data_00441298;
extern int data_004412A0;
extern int data_004412A8;
extern int data_004412B0;
extern int data_004412B8;
extern int data_004412C0;
extern int data_004412C8;

extern unsigned char data_004412D8;   /* the sides swapped */
extern unsigned int data_00441470;    /* (the local player's team) */

void func_00620D28();   /* (the DME's: SOCOM 1's NetRegisterObjectStart) */
void func_00620DB0(int *field, int offset, int a, int b, int c, void *type);   /* (NetObjectField) */
void func_00620E98(int *index, const char *name, int size);   /* (NetRegisterStructure) */
void func_0030CE80(CZNetwork *net, int index, void (*creation)(int, int),
                   void (*update)(int, int, int), void (*postupdate)(int, int, int),
                   void (*deletion)(int, int));   /* (CZNetwork::zNetRegisterRemoteObjectCallback) */
void *func_002BD0A0();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD120();
int func_0030D670(CZNetwork *net, int *index, int own, int life, unsigned int a, int b,
                  unsigned int c, int d, void *e, const char *name, int structure);   /* (CZNetwork::zNetCreateObject) */
void *memset(void *dst, int c, unsigned int n);
void func_002A7560(CZNetGame *game, bool on);
void func_002C2D50(int value);   /* (fts_net_persona.cpp's: the local persona's byte 0x43) */
void func_002C3330(unsigned int team, int swapped);
}

/* the kind registered: its fifteen fields, their offsets taken off a pointer to a local, as
 * fts_net_grenade.cpp's (OVL1 0x2C1120) */
void CZGameState::zNetRegisterGame()
{
    TypeGameObject g;
    TypeGameObject *p = &g;

    func_00620D28();
    func_00620DB0(&data_00441258, (int)&p->m_0 - (int)p, 1, 1, 5, func_002BD120());
    func_00620DB0(&data_00441260, (int)&p->m_8 - (int)p, 4, 5, 1, func_002BD0A0());
    func_00620DB0(&data_00441268, (int)&p->m_c - (int)p, 4, 5, 1, func_002BD120());
    func_00620DB0(&data_00441270, (int)&p->m_10 - (int)p, 4, 5, 1, func_002BD120());
    func_00620DB0(&data_00441288, (int)&p->m_18_0 - (int)p, 4, 5, 1, func_002BD0A0());
    func_00620DB0(&data_00441278, (int)&p->m_14 - (int)p, 2, 3, 1, func_002BD0A0());
    func_00620DB0(&data_00441280, (int)&p->m_16 - (int)p, 2, 3, 1, func_002BD0A0());
    func_00620DB0(&data_00441290, (int)&p->m_1c - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_00441298, (int)&p->m_1d - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_004412A0, (int)&p->m_1e - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_004412A8, (int)&p->m_1f - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_004412B0, (int)&p->m_20 - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_004412B8, (int)&p->m_21 - (int)p, 1, 1, 1, func_002BD0A0());
    func_00620DB0(&data_004412C0, (int)&p->m_22 - (int)p, 1, 1, 32, func_002BD0A0());
    func_00620DB0(&data_004412C8, (int)&p->m_42 - (int)p, 1, 1, 24, func_002BD0A0());
    func_00620E98(&m_NetStructIndexGameObject, "GameState", sizeof(TypeGameObject));
    func_0030CE80(&theNetwork, m_NetStructIndexGameObject, RemoteGameCreationCallback,
                  RemoteGameUpdateCallback, RemoteGamePostUpdateCallback,
                  RemoteGameDeletionCallback);
}

/* after an update: the valve at 0x64 set from the flag the object carries (OVL1 0x2C07B0) */
void CZGameState::RemoteGamePostUpdateCallback(int client, int index, int field)
{
    TypeGameObject *data = (TypeGameObject *)theNetwork.zNetGetObject(index)->m_data;

    if (data) {
        CZNetGame *game = theMission.m_pNetGame;

        if (game) {
            SetValveValue(game->m_64, data->m_18_16);
            func_002A7560(game, game->m_64->m_value != 0);
        }
    }
}

/* the game's object appears: the game's state unknown, its object kept, its fields taken up, and
 * the local persona told (OVL1 0x2C06A0; the post-update callback inlined, as the target has it).
 * OPEN, 14 instructions: s0 and s1 trade places -- the target keeps the game in s0 and the index in
 * s1 (the inlined callback's game then takes s1), this the other way round. Tried: the game
 * declared in the condition or assigned in it, an early return, the object index stored before
 * the state call, the index re-read from the game, the inlined callback's tests joined. */
void CZGameState::RemoteGameCreationCallback(int client, int index)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (game) {
        game->MPMajorGameState(CZNetGame::MP_MAJOR_UNKNOWN);
        game->m_game_object_index = index;
        RemoteGameUpdateCallback(client, index, -1);
        RemoteGamePostUpdateCallback(client, index, -1);
        if (game->m_cd)
            func_002C2D50(game->m_11c);
        else
            func_002C2D50(-1);
    }
}

/* nothing when it goes (OVL1 0x2C0690, as SOCOM 1's) */
void CZGameState::RemoteGameDeletionCallback(int client, int index)
{
}

/* the session master's game object made (OVL1 0x2C05E0) */
int CZGameState::CreateLocalGame(CZNetGame *game)
{
    int index;

    if (func_0030D670(&theNetwork, &index, 1, 2, 0, 0, 0, 0, 0, "GameObject",
                      m_NetStructIndexGameObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return -1;
    memset(obj->m_data, 0, sizeof(TypeGameObject));
    return index;
}

/* the game object's halfword at 0x16 (SOCOM II's; OVL1 0x2C0090) */
extern "C" unsigned short func_002C0090(CZNetGame *game)
{
    if (!game)
        return 0;
    NetObject *obj = theNetwork.zNetGetObject(game->m_game_object_index);

    if (!obj)
        return 0;
    TypeGameObject *data = (TypeGameObject *)obj->m_data;

    if (data)
        return data->m_16;
    return 0;
}

/* the sides swapped, or back: the team re-picked, the game's object and the persona's sent (SOCOM
 * II's; OVL1 0x2C0040) */
extern "C" void func_002C0040(bool swapped)
{
    if (swapped != data_004412D8) {
        data_004412D8 = swapped;
        func_002C3330(data_00441470, swapped);
        CZGameState::UpdateLocalGame(theMission.m_pNetGame);
        CZPersonaState::UpdateLocalPersona();
    }
}
