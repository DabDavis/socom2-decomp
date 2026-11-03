/* main.cpp -- the game itself: COurGame, its engine's start and stop, and the overlay's main loop
 * (SOCOM 1's Apps/FTS/main.cpp, without the allocator, which SOCOM II keeps in MAIN; OVL1
 * 0x1E7040-0x1E7F57, the first unit of FTSCore). Its out-of-line copies -- the states' implicit
 * destructors, CGameState's constructor and virtuals, CSaferStr's pieces -- and MSL's string
 * instances follow. Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

extern "C" {
int GetThreadId(void);
int ChangeThreadPriority(int tid, int prio);
unsigned int strlen(const char *s);

/* unnamed yet (SOCOM 1's names in the comments, where it had them) */
void func_003AFF30(bool wait, bool swap);        /* zVid_Swap */
void func_002ACC10(CMission *mission, const char *database);   /* CMission::PreOpen */

void func_001EABD0(int argc, char **argv);       /* (the command line taken up) */
void func_001EAC80();                            /* (the command line let go of) */
void func_0039C700(int arg);
}

void InitSystemTuners();   /* 0x38A7A0 */
void zSysInit();   /* 0x34F960 */

/* the voice recogniser (reco.cpp's CReco; only what main calls): its files loaded */
class CReco {
public:
    bool Load();
};

extern CReco theReco;

const char *GetDatabase();

/* the UI's variables (zui.h's: only the version string's) */
struct _zrdr;

enum UIVAR_LONGEVITY {
    UIVL_DORMANT = 0,
    UIVL_TEMPORARY = 1,
    UIVL_PERMANENT = 2,
    UIVL_PERSISTENT = 3
};

class CUIVariable {
public:
    CUIVariable &operator=(const char *str);   /* 0x3862A0 */
};

class CUIVarManager {
public:
    CUIVariable *Add(const char *name, _zrdr *value, UIVAR_LONGEVITY longevity);   /* 0x3857C0 */
};
extern CUIVarManager theUIVarManager;

/* a valve: a named value (only its creation, SOCOM 1's CValve::Create with SOCOM II's value; the
 * type's values are not in SOCOM 1's DWARF, as zanim.h says) */
enum VALVE_TYPE {};

class CValve {
public:
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);
};

/* the online lobby (SOCOM 1's theLobby: only its set-up and teardown) */
class CZOnlineLobby {
public:
    void Init();   /* 0x2E0450 */
    void UnInit();
};
extern CZOnlineLobby theLobby;

/* the menus first: every other state made, and the film's */
COurGame::COurGame()
{
    m_InitialState = &m_menuState;
}

/* the engine started: the screen swapped, the default texture buffer, the mission's set-up, the
 * system tuners, two config values, the database's mission opened ahead (when there is one), the
 * font library kept, the lobby. Pinned: main calls it */
#pragma auto_inline off
bool COurGame::StartEngine()
{
    bool started = false;

    if (CGame::StartEngine()) {
        func_003AFF30(false, true);
        zdb::CTexManager::m_texmanager.doAddBuffer("default", 0.0f, 1.0f);
        theMission.Init();
        InitSystemTuners();
        CValve::Create("R_NAME_SIZE", 31, (VALVE_TYPE)1);
        CValve::Create("MAX_CHAT_TEXT_LENGTH", 99, (VALVE_TYPE)1);
        if (GetDatabase() && strlen(GetDatabase()))
            func_002ACC10(&theMission, GetDatabase());
        ((unsigned char *)zdb::CAssetMgr::GetLoadedLibRef("common/assetlib/font"))[0x2c] = 0;
        theLobby.Init();
        started = true;
    }
    return started;
}

#pragma auto_inline reset

/* the engine stopped, the lobby with it */
void COurGame::StopEngine()
{
    CGame::StopEngine();
    theLobby.UnInit();
}

/* the overlay's entry (the boot loader's main runs it): the command line, the thread's priority,
 * the systems, the recogniser's files, the engine and play started, the build's version string set for the UI, then the
 * game ticked forever (SOCOM 1's main; the version is "SOCOM 2 r0001 " __TIME__ " " __DATE__) */
int main(int argc, char **argv)
{
    func_001EABD0(argc, argv);
    ChangeThreadPriority(GetThreadId(), 5);
    zSysInit();
    theReco.Load();
    func_001EAC80();
    func_0039C700(0);
    theGame.StartEngine();
    theGame.StartPlay();
    *theUIVarManager.Add("VERSION_STRING", 0, UIVL_PERMANENT) = "SOCOM 2 r0001 17:22:21 Oct 11 2003";
    for (;;) {
        CGame *game = &theGame;

        game->Tick(zVid.frameTime);
    }
}
