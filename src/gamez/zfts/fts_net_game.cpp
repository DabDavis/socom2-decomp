/* fts_net_game.cpp -- the online game's network object: CZGameState (SOCOM 1's fts_net_game.cpp:
 * the session master's game state, "GameState", that every client reads its rounds, scores and
 * flags from). OVL1 0x2C0040-0x2C141F, after fts_net_seal.cpp and before fts_net_grenade.cpp; SOCOM
 * II adds the sides' swap (0x2C0040) and a reader of the object's halfword at 0x16 (0x2C0090) after
 * UpdateLocalGame. Written but RemoteGameUpdateCallback (0x2C0850). Defined in reverse of their
 * image order (-inline deferred). */

#include "../zanim/zanim.h"

/* a network object as CZNetwork's table hands it out (only its data) */
struct NetObject {
    unsigned char _0[0x1c];
    void *m_data;   // 0x1c
};

/* the DME's ownership and lifespan of an object (their values are not in SOCOM 1's DWARF, only
 * their names in zNetCreateObject's) */
enum NetOwnershipStatus {};
enum NetObjectLifespan {};

class CZNetwork {
public:
    void zNetRegisterRemoteObjectCallback(int index, void (*creation)(int, int),
                                          void (*update)(int, int, int),
                                          void (*postupdate)(int, int, int),
                                          void (*deletion)(int, int));
    int zNetCreateObject(int *index, NetOwnershipStatus own, NetObjectLifespan life, unsigned int a,
                         int b, unsigned int c, int d, void *e, char *name, int structure);
    NetObject *zNetGetObject(int index);

    unsigned char m_bSessionMaster;   // 0x0 (znet.h's)
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

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

/* the game's clock (SOCOM 1's CNetClock: only its first two words) */
class CNetClock {
public:
    unsigned int m_max_game_time;   // 0x0: in ms (sent in seconds)
    unsigned int m_4;   // 0x4 (SOCOM 1's m_save_max_game_time)
};

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
    unsigned int DecrementRoundClockSec(int sec);   /* 0x2A6960 */

    CValve *m_0;   // 0x0
    unsigned char _4[0x8 - 0x4];
    CValve *m_8;   // 0x8
    unsigned char _c[0x40 - 0xc];
    CValve *m_pPenaltyValve;   // 0x40 (SOCOM 1's at 0x3c: UpdateLocalGame's penalty test)
    unsigned char _44[0x4c - 0x44];
    CValve *m_4c;   // 0x4c
    unsigned char _50[0x58 - 0x50];
    CValve *m_58;   // 0x58
    CValve *m_5c;   // 0x5c
    unsigned char _60[0x64 - 0x60];
    CValve *m_64;   // 0x64
    unsigned char _68[0xb0 - 0x68];
    int m_game_object_index;   // 0xb0 (SOCOM 1's at 0x74)
    unsigned char _b4[0xc0 - 0xb4];
    unsigned int m_last_penalty;   // 0xc0 (SOCOM 1's int at 0x84: SOCOM II's compares unsigned)
    unsigned char _c4[0xcd - 0xc4];
    unsigned char m_cd;   // 0xcd
    unsigned char _ce[0xd4 - 0xce];
    bool m_d4;   // 0xd4
    unsigned char _d5[0xd8 - 0xd5];
    bool m_d8;   // 0xd8
    unsigned char _d9[0xe0 - 0xd9];
    CNetClock m_clock;   // 0xe0 (SOCOM 1's at 0xa0)
    unsigned char _e8[0x111 - 0xe8];
    unsigned char m_game_type;   // 0x111 (SOCOM 1's at 0xd9; packed after the start flag there)
    unsigned char m_net_start_flag;   // 0x112 (SOCOM 1's at 0xda)
    unsigned char m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
    unsigned char _114[0x11c - 0x114];
    int m_11c;   // 0x11c
    int m_120;   // 0x120
    int m_124;   // 0x124
};

class CMission {
public:
    /* (the name ours: the creation callback's game comes through an inline -- read as the member,
     * it takes s1 and the index s0, the target's the other way round) */
    CZNetGame *GetNetGame() { return m_pNetGame; }

    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

/* the game's network data: 92 bytes */
struct TypeGameObject {
    unsigned char m_0[5];   // 0x0
    unsigned char _5[3];
    int m_8;   // 0x8
    unsigned int m_c;   // 0xc: the game's clock in seconds
    int m_10;   // 0x10
    short m_14;   // 0x14: 71, 2071 for the 1 and 2 of the byte at 0x437DD4
    unsigned short m_16;   // 0x16: the node universe's count
    /* the flags (UpdateLocalGame's, bit by bit; a word of their own, its address registered) */
    struct Flags {
        unsigned int m_b0 : 1;   // 0x18, bit 0: the game's 0xd8
        unsigned int m_b1 : 2;   // bits 1-2: the start flag
        unsigned int m_b3 : 4;   // bits 3-6: the major game state
        unsigned int m_b7 : 1;   // bit 7: the game's 0xd4
        unsigned int m_b8 : 3;   // bits 8-10: the game type
        unsigned int m_b11 : 2;
        unsigned int m_b13 : 1;   // bit 13: "mp_friendly_fire"
        unsigned int m_b14 : 1;   // bit 14: "mp_locked_game"
        unsigned int m_b15 : 1;   // bit 15: the game's valve at 0x4c
        unsigned int m_b16 : 1;   // bit 16: the game's valve at 0x64
        unsigned int m_b17 : 4;   // bits 17-20: the game's valve at 0x58
        unsigned int m_b21 : 4;   // bits 21-24: the game's valve at 0x5c
        unsigned int m_b25 : 3;   // bits 25-27: "EQlimit"
        unsigned int m_b28 : 1;   // bit 28: the sides swapped
        unsigned int m_b29 : 3;
    } m_18;   // 0x18
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

void NetRegisterObjectStart();   /* (the DME's) */
void NetObjectField(int *field, int offset, int a, int b, int c, void *type);
void NetRegisterStructure(int *index, const char *name, int size);
void *func_002BD0A0();   /* (fts_remote.cpp's broadcast schedules) */
void *func_002BD120();
void *memset(void *dst, int c, unsigned int n);
void func_002A7560(CZNetGame *game, bool on);
void func_002C2D50(int value);   /* (fts_net_persona.cpp's: the local persona's byte 0x43) */
void func_002C3330(unsigned int team, int swapped);

extern unsigned char data_00437DD4;   /* (a byte the version adds 2000 for at 1 and 2) */
extern unsigned char data_00441580[24];   /* (the mission picks the game sends) */
void *memcpy(void *dst, const void *src, unsigned int n);
char *strncpy(char *dst, const char *src, unsigned int n);
}

const char *GetDatabase();   /* 0x27F800 */

/* the SEALs' friendly-fire switch (zseal.h's) */
class CZSealBody {
public:
    static bool m_allow_friendly_fire;
};

/* the online lobby (zonline.h's: only what is called) */
class CZOnlineLobby {
public:
    void GetMissionSequence(unsigned char *idx, unsigned char *seq);   /* 0x2F82E0 */
};

extern CZOnlineLobby theLobby;

/* the kind registered: its fifteen fields, their offsets taken off a pointer to a local, as
 * fts_net_grenade.cpp's (OVL1 0x2C1120) */
void CZGameState::zNetRegisterGame()
{
    TypeGameObject g;
    TypeGameObject *p = &g;

    NetRegisterObjectStart();
    NetObjectField(&data_00441258, (int)&p->m_0 - (int)p, 1, 1, 5, func_002BD120());
    NetObjectField(&data_00441260, (int)&p->m_8 - (int)p, 4, 5, 1, func_002BD0A0());
    NetObjectField(&data_00441268, (int)&p->m_c - (int)p, 4, 5, 1, func_002BD120());
    NetObjectField(&data_00441270, (int)&p->m_10 - (int)p, 4, 5, 1, func_002BD120());
    NetObjectField(&data_00441288, (int)&p->m_18 - (int)p, 4, 5, 1, func_002BD0A0());
    NetObjectField(&data_00441278, (int)&p->m_14 - (int)p, 2, 3, 1, func_002BD0A0());
    NetObjectField(&data_00441280, (int)&p->m_16 - (int)p, 2, 3, 1, func_002BD0A0());
    NetObjectField(&data_00441290, (int)&p->m_1c - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_00441298, (int)&p->m_1d - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004412A0, (int)&p->m_1e - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004412A8, (int)&p->m_1f - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004412B0, (int)&p->m_20 - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004412B8, (int)&p->m_21 - (int)p, 1, 1, 1, func_002BD0A0());
    NetObjectField(&data_004412C0, (int)&p->m_22 - (int)p, 1, 1, 32, func_002BD0A0());
    NetObjectField(&data_004412C8, (int)&p->m_42 - (int)p, 1, 1, 24, func_002BD0A0());
    NetRegisterStructure(&m_NetStructIndexGameObject, "GameState", sizeof(TypeGameObject));
    theNetwork.zNetRegisterRemoteObjectCallback(m_NetStructIndexGameObject,
                                                RemoteGameCreationCallback,
                                                RemoteGameUpdateCallback,
                                                RemoteGamePostUpdateCallback,
                                                RemoteGameDeletionCallback);
}

/* after an update: the valve at 0x64 set from the flag the object carries (OVL1 0x2C07B0) */
void CZGameState::RemoteGamePostUpdateCallback(int client, int index, int field)
{
    TypeGameObject *data = (TypeGameObject *)theNetwork.zNetGetObject(index)->m_data;

    if (data) {
        CZNetGame *game = theMission.m_pNetGame;

        if (game) {
            SetValveValue(game->m_64, data->m_18.m_b16);
            func_002A7560(game, game->m_64->m_value != 0);
        }
    }
}

/* the game's object appears: the game's state unknown, its object kept, its fields taken up, and
 * the local persona told (OVL1 0x2C06A0; the post-update callback inlined, as the target has it;
 * the game through the mission's inline accessor, which gives it s0 and the index s1 -- a value an
 * inline returns takes its saved register after the named ones) */
void CZGameState::RemoteGameCreationCallback(int client, int index)
{
    CZNetGame *game = theMission.GetNetGame();

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

    if (theNetwork.zNetCreateObject(&index, (NetOwnershipStatus)1, (NetObjectLifespan)2, 0, 0, 0,
                                    0, 0, "GameObject", m_NetStructIndexGameObject))
        return -1;
    NetObject *obj = theNetwork.zNetGetObject(index);

    if (!obj)
        return -1;
    memset(obj->m_data, 0, sizeof(TypeGameObject));
    return index;
}

/* the session master's game object written: the penalty clock caught up, then the object built
 * on the stack -- the database, the clock in seconds, the round, the version, the node count, the
 * flags bit by bit, the counts, the winner, the mission sequence and the pick list -- and copied
 * over the object's data (SOCOM 1's, the struct grown from 64 bytes to 92; OVL1 0x2C00F0).
 * Byte-exact (the states' round 45). Its tells: a value masked to its field's width before the
 * insert (`x & 3` into a 2-bit field) keeps both andi's, a bool or a byte into a 1-bit field one;
 * the penalty count compares unsigned (sltu: SOCOM 1's int is unsigned here); the version an int
 * (a short sign-extends the sum). The clock word read twice through addresses held in registers
 * (SOCOM 1 does the same) comes only through references -- plain, a pointer or an array index
 * fold the offset into each load -- and the destination's address takes a2 and the clock's a3
 * only with the destination's reference declared first (igreplay: the field's address numbered
 * before the clock's; a marked numbering device) */
void CZGameState::UpdateLocalGame(CZNetGame *game)
{
    if (theNetwork.m_bNetwork && theNetwork.m_bSessionMaster) {
        NetObject *obj = theNetwork.zNetGetObject(game->m_game_object_index);

        if (obj) {
            TypeGameObject *data = (TypeGameObject *)obj->m_data;

            if (data) {
                TypeGameObject g;

                GetDatabase();
                if (game->m_last_penalty < game->m_pPenaltyValve->m_value)
                    game->DecrementRoundClockSec(60);
                game->m_last_penalty = game->m_pPenaltyValve->m_value;
                memset(&g, 0, sizeof(g));
                strncpy((char *)g.m_0, GetDatabase(), 5);
                g.m_c = (unsigned int)(game->m_clock.m_max_game_time * 0.001f);
                g.m_8 = game->m_8->m_value;
                int &dst = g.m_10;
                const unsigned int &src = game->m_clock.m_4;

                dst = src;
                int version = 71;

                if (data_00437DD4 == 1 || data_00437DD4 == 2)
                    version += 2000;
                g.m_14 = version;
                g.m_16 = zdb::NodeUniverse ? zdb::NodeUniverse->size() : 0;
                g.m_18.m_b0 = game->m_d8;
                if (g.m_18.m_b0)
                    dst = src;
                g.m_18.m_b1 = game->m_net_start_flag & 3;
                g.m_18.m_b3 = game->m_mp_major_game_state & 0xf;
                g.m_18.m_b7 = game->m_d4;
                g.m_18.m_b8 = game->m_game_type & 7;
                g.m_18.m_b17 = game->m_58->m_value & 0xf;
                g.m_18.m_b21 = game->m_5c->m_value & 0xf;
                g.m_18.m_b16 = game->m_64->m_value & 1;
                g.m_18.m_b28 = data_004412D8;
                CValve *valve = CValve::GetByName("mp_friendly_fire");

                if (valve && valve->m_value) {
                    g.m_18.m_b13 = 1;
                    CZSealBody::m_allow_friendly_fire = true;
                } else
                    CZSealBody::m_allow_friendly_fire = false;
                valve = CValve::GetByName("EQlimit");
                if (valve)
                    g.m_18.m_b25 = valve->m_value & 7;
                valve = CValve::GetByName("mp_locked_game");
                if (valve && valve->m_value)
                    g.m_18.m_b14 = 1;
                if (game->m_4c)
                    g.m_18.m_b15 = game->m_4c->m_value & 1;
                g.m_1c = game->m_11c;
                g.m_1d = game->m_0->m_value;
                g.m_1f = game->m_120;
                g.m_20 = game->m_124;
                valve = CValve::GetByName("mp_winner");
                if (valve)
                    g.m_1e = valve->m_value;
                theLobby.GetMissionSequence(&g.m_21, g.m_22);
                for (int i = 0; i < 24; i++)
                    g.m_42[i] = data_00441580[i];
                memcpy(data, &g, sizeof(g));
            }
        }
    }
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
