/* st_load.cpp -- the mission's load screen (SOCOM 1's Apps/FTS/st_load.cpp, without the E3
 * build's mission config; OVL1 0x1F5D00-0x1F6A8F). Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"

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

/* the online game's state (SOCOM 1's CZNetGame: only its flags here and the two states it sets) */
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

    unsigned char _0[0xcc];
    unsigned char m_cc;   // 0xcc
    unsigned char m_cd;   // 0xcd
    unsigned char _ce[2];
    unsigned char m_d0;   // 0xd0
    unsigned char m_d1;   // 0xd1
    unsigned char m_d2;   // 0xd2
    unsigned char _d3[0xdd - 0xd3];
    unsigned char m_dd;   // 0xdd
};

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
};

extern "C" {
void func_002AF360(const char *name);
void func_002C2C20(int arg);
void func_0030E6E0(void *net, int arg);
void func_0030E7F0(void *net);
int func_002C2FA0();
void func_0030E5A0(void *net, bool arg);
void ClearRemoteGuyList__11CZSealStateFv();   /* CZSealState::ClearRemoteGuyList */
void func_002BB1D0();

extern void (*data_0044F3CC)(int);               /* the net error handler */
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
