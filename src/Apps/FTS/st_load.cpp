/* st_load.cpp -- the mission's load screen (SOCOM 1's Apps/FTS/st_load.cpp, without the E3
 * build's mission config; OVL1 0x1F5D00-0x1F6A8F). Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"
#include "../../gamez/ztwod/ztwod.h"

#include <iostream>

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
int InitThread(void);

}

/* the localised string tables (util_loc.cpp's; only what is called), the language a word as
 * st_mpexit.cpp has it; the asset recycler and the UI's variables (their units') */
enum LANGUAGE {
    LANGUAGE_ALIGNMENT_HACK = 0x7FFFFFFF
};
LANGUAGE GetLanguage();   /* 0x3A4460 */

class CStringTables {
public:
    void Load(const char *name, LANGUAGE language);   /* 0x3A3900 */
    void Unload(const char *name);   /* 0x3A3740 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

namespace zdb {
void EmptyAssetRecycleBin();   /* 0x321D30 */
}

class CUIVarManager {
public:
    void Reset();   /* 0x385C30 */
};
extern CUIVarManager theUIVarManager;

namespace {
/* the load screen's asset library */
zdb::CAssetLib *g_pMissionLoadingLib;
}

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);
extern "C" int strcmp(const char *a, const char *b);
const char *GetDatabase();

/* a valve (zanim.h's CValve: only its value and its callbacks) */
enum VALVE_STATE { VALVE_SAME = 0, VALVE_ON = 1, VALVE_OFF = 2 };

class CValve {
public:
    void MakeCallbacks(VALVE_STATE state);   /* 0x3519F0 */
    /* the value (an accessor: the test of it is not merged with the setter's read) */
    unsigned int GetValue() const { return m_value; }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* CValve::SetValue, inlined (fts_net_game.cpp's spelling: a free inline, which reloads the value
 * the caller just tested) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the online game's state (SOCOM 1's CZNetGame: only its flags here and the states it sets;
 * fts_net_game.cpp's names) */
class CZNetGame {
public:
    enum MP_MINOR_GAME_STATE {
        MP_MINOR_UNKNOWN = 0,
        MP_MINOR_NOTREADY = 1,
        MP_MINOR_READY = 2
    };
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
    MP_MINOR_GAME_STATE MyMinorGameState(MP_MINOR_GAME_STATE state);
    MP_MAJOR_GAME_STATE MyMajorGameState(MP_MAJOR_GAME_STATE state);   /* 0x2A7420 */
    void ResetScores();   /* 0x2A72D0 */
    bool LateJoiner(bool late);   /* 0x2A7340 */
    void SetE3RoundCount(int count);   /* 0x2A7510 */
    /* the custom character type taken (SOCOM 1's was a global, written the same way; the name
     * ours -- a member: `this` loads after the argument's call) */
    void SetCharType(const char *type)
    {
        strncpy(m_charType, type, sizeof(m_charType));
        m_charType[sizeof(m_charType) - 1] = 0;
    }

    CValve *m_0;   // 0x0
    unsigned char _4[0x8 - 0x4];
    CValve *m_8;   // 0x8: the round's time in seconds
    unsigned char _c[0x64 - 0xc];
    CValve *m_64;   // 0x64
    unsigned char _68[0xb0 - 0x68];
    int m_game_object_index;   // 0xb0
    unsigned char _b4[0xcc - 0xb4];
    unsigned char m_cc;   // 0xcc
    unsigned char m_cd;   // 0xcd
    unsigned char _ce[2];
    unsigned char m_d0;   // 0xd0
    unsigned char m_d1;   // 0xd1
    unsigned char m_d2;   // 0xd2
    unsigned char _d3[0xdd - 0xd3];
    unsigned char m_dd;   // 0xdd
    unsigned char _de[0xe0 - 0xde];
    int m_round_ms;   // 0xe0 (fts_net_game.cpp's CNetClock m_clock)
    unsigned char _e4[0x111 - 0xe4];
    unsigned char m_game_type;   // 0x111
    unsigned char m_net_start_flag;   // 0x112
    unsigned char m_mp_major_game_state;   // 0x113
    unsigned char _114;
    unsigned char m_115;   // 0x115
    unsigned char _116[0x12c - 0x116];
    char m_charType[31];   // 0x12c
};

/* the multiplayer missions (zNetMediusRoom.cpp's: a mission's game type by its database name) */
class CMultiMissionList {
public:
    signed char GetMultiValue(const char *name);   /* 0x2C8A00 */
};
extern "C" CMultiMissionList data_00441650;

class CZGameState {
public:
    static void UpdateLocalGame(CZNetGame *game);   /* 0x2C00F0 */
};

class CZSealState {
public:
    static bool UnloadRemoteGuyList();   /* 0x2BD560 */
};

const char *GetCustomCharTypeName(int index);   /* 0x2AF780 */
bool DMEJoinGameServer();   /* 0x2B83E0 */
void ftsNetworkGameInit();   /* 0x2B7A20 */
void ftsNetworkSealInit();   /* 0x2B7A90 */

/* the system's public state (zsys_public.h's _zsys_public: only its timer, as st_core.cpp's) */
struct _zsys_public {
    unsigned char _0[0x24];
    unsigned int timerTicksPerSecond;   // 0x24
};
extern _zsys_public zSys;

extern "C" {
int SuspendThread(int thid);
int ResumeThread(int thid);
}
void zSysVu0Save(int mode);   /* 0x351260 */
void zSysVu0Restore(int mode);   /* 0x351140 */
void zSysSleep(int ticks);   /* 0x34F0E0 */
void zVid_Swap(bool flip, bool restore);   /* 0x3AFF30 */

class CZBombState {
public:
    static void UninitBomb();
};

class CZPersonaState {
public:
    static void UpdateLocalPersona();
};

/* the turrets' network state (fts_net_platform.cpp's) */
class CZPlatformState {
public:
    static void ClearPendingPlatforms();   /* 0x2C79E0 */
    static void CreatePendingPlatforms();   /* 0x2C7910 */
};

extern "C" {
bool func_002C31A0();   /* we play on the game's first side (zNetMediusStates.cpp's) */
int func_002C4020();
void func_002A7560(CZNetGame *game, bool on);
void func_002AF360(const char *name);
void func_002C2C20(int arg);
void func_0030E6E0(void *net, int arg);
void func_0030E7F0(void *net);
int func_002C2FA0();
void func_0030E5A0(void *net, bool arg);
void ClearRemoteGuyList__11CZSealStateFv();   /* CZSealState::ClearRemoteGuyList */
void func_002BB1D0();

class CZOnlineLobby;
extern CZOnlineLobby theLobby;
#define data_0044F3CC (*(void (*(*))(int))((char *)&::theLobby + 0x5C))   /* theLobby+0x5C */ /* the net error handler */
extern void *data_0045B718;
extern unsigned char data_003DF1A8;
}

const char *CLoadMenu::GetReaderFileName()
{
    return "dlgLoad.rdr";
}

/* the load thread's kernel set up */
CLoadState::CLoadState() : CGameState("LoadState")
{
    m_menu = 0;
    InitThread();
}

CLoadState::~CLoadState()
{
    if (m_menu)
        delete m_menu;
    m_menu = 0;
}

/* the load screen up: the mission's script, its asset library (when it names one), the menu and
 * its anims, the UI's sound bank, the fades; online, the game's state reset for the round (the bomb,
 * the respawn, the voice), and a server whose database is not the mission's aborts the load */
bool CLoadState::Init()
{
    m_abort = false;
    theGame.m_pad = 0;
    func_002AF360(theMission.m_1c);
    const char *libname = theMission.m_libname.c_str();

    if (libname && *libname) {
        g_pMissionLoadingLib = zdb::CAssetMgr::GetLoadedLibRef(theMission.m_libname.c_str());
        ((unsigned char *)g_pMissionLoadingLib)[0x2c] = 0;
    }
    m_menu = new CLoadMenu;
    CZAnimZAR *zar = ZAnim.OpenZAR("run/ldzanim.zar");

    CStringTables *tables = GetStringTables();

    tables->Load("UILd", GetLanguage());
    m_menu->Init(0, zar);
    if (zar)
        ZAnim.CloseZAR(zar);
    m_fadeup = true;
    m_fadedown = false;
    m_time_elapsed = 0.0f;
    m_18 = false;
    m_19 = true;
    if (theNetwork.m_9) {
        CZNetGame *game = theMission.m_netGame;

        data_0044F3CC = CMission::RecoverFromNetError;
        CZBombState::UninitBomb();
        game->m_d0 = 0;
        game->m_cd = 0;
        game->MyMinorGameState(CZNetGame::MP_MINOR_NOTREADY);
        if (!game->m_cc) {
            CZPersonaState::UpdateLocalPersona();
            func_002C2C20(0);
            func_0030E6E0(data_0045B718, 0);
            func_0030E7F0(data_0045B718);
        } else {
            func_002C2C20(3);
        }
        if (!func_002C2FA0())
            func_0030E5A0(data_0045B718, !data_003DF1A8);
        game->m_d2 = 0;
        game->m_d1 = 0;
        game->m_dd = 0;
        ClearRemoteGuyList__11CZSealStateFv();
        CZPlatformState::ClearPendingPlatforms();
        func_002BB1D0();
    }
    if (theNetwork.m_9 && !theNetwork.m_8 && theMission.m_netGame->m_cc)
        theMission.m_netGame->MPMajorGameState(CZNetGame::MP_MAJOR_LOADING);
    zVid_Sync0();
    if (theNetwork.m_9) {
        const char *mission = theMission.m_1c;

        if (strcmp(mission, GetDatabase()))
            AbortLoad("dlgNetError.rdr");
    }
    return true;
}

/* the menu's fade (SOCOM 1's DWARF: CGameMenu::_bmpFade, a CPlainBmp at 0x170 -- C2D's SetTrans,
 * its slot 0x34) */
static inline C2D &MenuFade(CGameMenu *menu)
{
    return *(C2D *)((unsigned char *)menu + 0x170);
}

/* the menu faded to a level, held within 0..1 (the name ours) */
static inline void SetFade(CGameMenu *menu, float t)
{
    if (t > 1.0f)
        t = 1.0f;
    else if (t < 0.0f)
        t = 0.0f;
    MenuFade(menu).SetTrans(128.0f * (1.0f - t));
}

/* a frame of the load: the input ticked; an aborted load back to the menus once the core state has
 * stopped loading. Online, once the load has not started: the session master sets the game up for
 * the round (its type, its clock from the time valve -- 300 seconds when unset --, the rounds valve
 * 9 when unset, the major state loading, the scores reset, the game sent), and a game in its loading
 * state gives a custom character "0" the side's type (0 the first side's, 4 the other's) and starts
 * the load; once it has, the fade down begins when the load is done. Offline the fade down begins
 * when the load is done, else the load starts once the fade up is over. The fade up stepped; on the
 * fade down, online, the network SEALs and the waiting remote ones made, the turrets, the master's
 * start flag moved on, the persona ready, a late joiner joined (or the load aborted on a lost
 * server or an abandoned game); the fade down stepped once the game is ready, the core state
 * switched to at its end. The menu ticked and drawn with the load thread suspended, the frame
 * swapped, and a pause while nothing fades (OVL1 0x1F5E70; SOCOM 1's, without its E3 build's
 * mission players). Byte-exact on its seventh build (the game states' round 48, unwritten before):
 * the valves tested through GetValue() (a direct m_value test merges with the setter's read; the
 * target reads it twice), the custom type through a member inline (`this` re-read after the name's
 * call, kept for both stores), the fades through a free inline taking the menu (loaded before the
 * clamp, as the target), the clamp an if / else-if (a ternary chain leaves a `b` to the join), and
 * the second fade-down test `m_fadedown == 0` (the target's andi on the reused byte; `!` branches on
 * the register) */
bool CLoadState::Tick(float dT)
{
    CInput::Tick(dT);
    if (m_abort && (!theGame.m_coreState.m_loadstarted || theGame.m_coreState.IsLoaded())) {
        theGame.Switch(&theGame.m_menuState, 0);
        return false;
    }
    if (theNetwork.m_9) {
        CZNetGame *game = theMission.m_netGame;
        const char *db = GetDatabase();

        if (theGame.m_coreState.m_loadstarted) {
            if (!m_fadedown && theGame.m_coreState.IsLoaded()) {
                m_fadedown = true;
                m_time_elapsed = 0.0f;
            }
        } else {
            if (theNetwork.m_8) {
                game->m_game_type = data_00441650.GetMultiValue(db);
                func_002A7560(game, game->m_64->m_value != 0);
                if (game->m_8->GetValue() == 0)
                    SetValveValue(game->m_8, 300);
                game->m_round_ms = game->m_8->m_value * 1000;
                if (game->m_0->GetValue() == 0)
                    SetValveValue(game->m_0, 9);
                game->MPMajorGameState(CZNetGame::MP_MAJOR_LOADING);
                game->ResetScores();
                CZGameState::UpdateLocalGame(game);
            }
            if (game->m_mp_major_game_state == CZNetGame::MP_MAJOR_LOADING) {
                if (!strcmp(theMission.m_netGame->m_charType, "0")) {
                    if (func_002C31A0())
                        theMission.m_netGame->SetCharType(GetCustomCharTypeName(0));
                    else
                        theMission.m_netGame->SetCharType(GetCustomCharTypeName(4));
                }
                theGame.m_coreState.Load(true);
            }
        }
    } else if (!m_fadedown && theGame.m_coreState.IsLoaded() && !m_19) {
        m_fadedown = true;
        m_time_elapsed = 0.0f;
    } else if (!m_fadeup && !m_18 && m_fadedown == 0) {
        m_18 = true;
        m_19 = false;
        CSnd::UnloadAllSounds();
        theGame.m_coreState.Load(true);
    }
    if (m_fadeup) {
        m_time_elapsed += dT;
        if (m_time_elapsed > 0.5f) {
            m_time_elapsed = 0.5f;
            m_fadeup = false;
        }
        SetFade(m_menu, m_time_elapsed / 0.5f);
    }
    if (m_fadedown && theNetwork.m_9) {
        CZNetGame *game = theMission.m_netGame;

        if (theNetwork.m_8) {
            ftsNetworkSealInit();
            CZSealState::UnloadRemoteGuyList();
            CZPlatformState::CreatePendingPlatforms();
            if (game->m_net_start_flag == 1 && func_002C4020()) {
                game->m_net_start_flag = 2;
                game->SetE3RoundCount(0);
                CZGameState::UpdateLocalGame(game);
            }
        } else if (!game->m_cc && game->m_game_object_index >= 0) {
            ftsNetworkSealInit();
            CZSealState::UnloadRemoteGuyList();
            CZPlatformState::CreatePendingPlatforms();
            if (game->m_cd)
                game->m_d0 = 1;
        }
        if (game->m_115 != 2 && game->m_game_object_index >= 0) {
            game->MyMajorGameState(CZNetGame::MP_MAJOR_LOADING);
            game->MyMinorGameState(CZNetGame::MP_MINOR_READY);
            CZPersonaState::UpdateLocalPersona();
        }
        if (game->m_net_start_flag == 2 || !game->m_cc && game->m_mp_major_game_state == CZNetGame::MP_MAJOR_INPLAY) {
            if (game->m_game_object_index >= 0)
                game->m_d0 = 1;
        } else if (game->m_net_start_flag == 3 && game->m_cc) {
            game->LateJoiner(false);
            AbortLoad("dlgNetError.rdr");
        }
        if (game->m_cc) {
            game->LateJoiner(false);
            game->m_cd = 1;
            if (DMEJoinGameServer()) {
                ftsNetworkGameInit();
                if (!func_002C2FA0())
                    game->m_d2 = 1;
            } else {
                AbortLoad("dlgNetError.rdr");
            }
        }
        if (game->m_cd) {
            if (game->m_game_object_index >= 0) {
                unsigned char state = game->m_mp_major_game_state;

                if (state == CZNetGame::MP_MAJOR_GAMEOVER || state == CZNetGame::MP_MAJOR_MENUS) {
                    AbortLoad("dlgNetAbandoned.rdr");
                    game->m_cd = 0;
                }
            } else {
                game->m_d0 = 0;
            }
            if (theNetwork.m_8) {
                AbortLoad("dlgNetAbandoned.rdr");
                game->m_cd = 0;
            }
        }
    }
    if (m_fadedown && (!theNetwork.m_9 || theMission.m_netGame->m_d0)) {
        m_time_elapsed += dT;
        if (m_time_elapsed > 0.5f) {
            theGame.Switch(&theGame.m_coreState, 0);
            return false;
        }
        SetFade(m_menu, 1.0f - m_time_elapsed / 0.5f);
    }
    if (theGame.m_coreState.m_LoadThreadId != -1) {
        SuspendThread(theGame.m_coreState.m_LoadThreadId);
        zSysVu0Save(0);
    }
    m_menu->Tick(dT);
    m_menu->Draw2dOnly();
    if (theGame.m_coreState.m_LoadThreadId != -1) {
        zSysVu0Restore(0);
        ResumeThread(theGame.m_coreState.m_LoadThreadId);
    }
    zSysFifoEnd();
    zVid_Swap(true, true);
    if (!m_fadeup && !m_fadedown)
        zSysSleep(zSys.timerTicksPerSecond / 30);
    return false;
}

/* torn down: the menu, the UI's sound bank, the screen's library, the recycled assets, the UI's
 * variables -- and an aborted load's mission unloaded (if it is not still loading), the load
 * thread's stack freed */
void CLoadState::UnInit()
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    if (m_menu)
        delete m_menu;
    m_menu = 0;
    GetStringTables()->Unload("UILd");
    zdb::CAssetMgr::ReleaseLibRef(g_pMissionLoadingLib);
    g_pMissionLoadingLib = 0;
    zdb::EmptyAssetRecycleBin();
    theUIVarManager.Reset();
    if (m_abort && theGame.m_coreState.IsLoaded())
        theGame.m_coreState.Unload();
    theGame.m_coreState.FreeLoadThread();
}

/* the load given up: the menus to come back to the named dialog */
void CLoadState::AbortLoad(const char *dlgname)
{
    strncpy(theGame.m_menuState.m_DefaultMenu, dlgname, 64);
    m_abort = true;
}
