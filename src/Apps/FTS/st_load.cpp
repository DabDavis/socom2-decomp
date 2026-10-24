/* st_load.cpp -- the mission's load screen (SOCOM 1's Apps/FTS/st_load.cpp, without the E3
 * build's mission config; OVL1 0x1F5D00-0x1F6A8F). Defined in reverse of their image order
 * (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
int InitThread(void);
void *func_00361BC0();
void func_003A3740(void *snd, const char *bank);
void func_00321D30();                            /* zdb::EmptyAssetRecycleBin */
void func_00385C30(void *vars);                  /* CUIVarManager::Reset */

extern zdb::CAssetLib *data_00408E28;            /* the load screen's asset library */
}

extern char theUIVarManager[];

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);
extern "C" int strcmp(const char *a, const char *b);
const char *GetDatabase();
void zVid_Sync0();

/* the online game's state (unnamed: only its flags here) */
class CNetGame {
public:
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

extern "C" {
void func_002AF360(const char *name);
zdb::CAssetLib *func_00320D70(const char *name); /* zdb::CAssetMgr's library load */
void func_003A3900(void *snd, const char *bank, void *arg);
void *func_003A4460();
void func_00368570(CGameMenu *menu, void *reloc, CZAnimZAR *zar);   /* CGameMenu::Init */
void func_002A73B0(CNetGame *game, int arg);
void func_002C5360();
void func_002C2C20(int arg);
void func_0030E6E0(void *net, int arg);
void func_0030E7F0(void *net);
int func_002C2FA0();
void func_0030E5A0(void *net, bool arg);
void func_002BD630();
void func_002C79E0();
void func_002BB1D0();
void func_002A7490(CNetGame *game, int arg);

extern int data_00408534;                        /* theGame's word at 0x74 */
extern CStateString data_00437D1C;               /* the load screen's asset library's name */
extern CNetGame *data_00437CE8;                  /* the online game */
extern void (*data_0044F3CC)(int);               /* the net error handler */
extern void *data_0045B718;
extern unsigned char data_003DF1A8;
extern unsigned char data_0045A0C0;              /* theNetwork's +8 */
extern unsigned char data_0045A0C1;              /* theNetwork's +9 (an online game) */
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
    data_00408534 = 0;
    func_002AF360(theMission.m_1c);
    const char *libname = data_00437D1C.c_str();

    if (libname && *libname) {
        data_00408E28 = func_00320D70(data_00437D1C.c_str());
        ((unsigned char *)data_00408E28)[0x2c] = 0;
    }
    m_menu = new CLoadMenu;
    CZAnimZAR *zar = ZAnim.OpenZAR("run/ldzanim.zar");

    func_003A3900(func_00361BC0(), "UILd", func_003A4460());
    func_00368570(m_menu, 0, zar);
    if (zar)
        ZAnim.CloseZAR(zar);
    m_fadeup = true;
    m_fadedown = false;
    m_time_elapsed = 0.0f;
    m_18 = false;
    m_19 = true;
    if (data_0045A0C1) {
        CNetGame *game = data_00437CE8;

        data_0044F3CC = CMission::_RecoverFromNetError;
        CZBombState::UninitBomb();
        game->m_d0 = 0;
        game->m_cd = 0;
        func_002A73B0(game, 1);
        if (!game->m_cc) {
            func_002C5360();
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
        func_002BD630();
        func_002C79E0();
        func_002BB1D0();
    }
    if (data_0045A0C1 && !data_0045A0C0 && data_00437CE8->m_cc)
        func_002A7490(data_00437CE8, 2);
    zVid_Sync0();
    if (data_0045A0C1) {
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
    func_003A3740(func_00361BC0(), "UILd");
    zdb::CAssetMgr::ReleaseLibRef(data_00408E28);
    data_00408E28 = 0;
    func_00321D30();
    func_00385C30(theUIVarManager);
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
