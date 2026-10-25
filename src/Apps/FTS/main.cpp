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
void func_003566C0(TexBufferVector *texmanager, const char *name, float a, float b);   /* zdb::CTexManager::doAddBuffer */
void func_0038A7A0();                            /* InitSystemTuners */
void func_002ACC10(CMission *mission, const char *database);   /* CMission::PreOpen */
zdb::CAssetLib *func_00320D70(const char *name); /* zdb::CAssetMgr::GetLoadedLibRef */
void func_002E0450(void *lobby);                 /* CZOnlineLobby::Init */
void UnInit__13CZOnlineLobbyFv(void *lobby);

void func_001EABD0(int argc, char **argv);       /* (the command line taken up) */
void func_0034F960();                            /* (zSysInit's place) */
void func_0023B540(void *arg);
void func_001EAC80();                            /* (the command line let go of) */
void func_0039C700(int arg);
void *func_003857C0(void *vars, const char *name, int a, int b);   /* a UI variable made */
void func_003862A0(void *var, const char *text); /* and its text set */

extern char data_00414680[];
}

const char *GetDatabase();
extern char theUIVarManager[];

/* a valve: a named value (only its creation, SOCOM 1's CValve::Create with SOCOM II's value; the
 * type's values are not in SOCOM 1's DWARF, as zanim.h says) */
enum VALVE_TYPE {};

class CValve {
public:
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);
};

/* the online lobby (SOCOM 1's theLobby) */
extern char theLobby[];

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
        func_003566C0(&zdb::CTexManager::m_texmanager, "default", 0.0f, 1.0f);
        theMission.Init();
        func_0038A7A0();
        CValve::Create("R_NAME_SIZE", 31, (VALVE_TYPE)1);
        CValve::Create("MAX_CHAT_TEXT_LENGTH", 99, (VALVE_TYPE)1);
        if (GetDatabase() && strlen(GetDatabase()))
            func_002ACC10(&theMission, GetDatabase());
        ((unsigned char *)func_00320D70("common/assetlib/font"))[0x2c] = 0;
        func_002E0450(theLobby);
        started = true;
    }
    return started;
}

#pragma auto_inline reset

/* the engine stopped, the lobby with it */
void COurGame::StopEngine()
{
    CGame::StopEngine();
    UnInit__13CZOnlineLobbyFv(theLobby);
}

/* the overlay's entry (the boot loader's main runs it): the command line, the thread's priority,
 * the systems, the engine and play started, the build's version string set for the UI, then the
 * game ticked forever (SOCOM 1's main; the version is "SOCOM 2 r0001 " __TIME__ " " __DATE__) */
int main(int argc, char **argv)
{
    func_001EABD0(argc, argv);
    ChangeThreadPriority(GetThreadId(), 5);
    func_0034F960();
    func_0023B540(data_00414680);
    func_001EAC80();
    func_0039C700(0);
    theGame.StartEngine();
    theGame.StartPlay();
    func_003862A0(func_003857C0(theUIVarManager, "VERSION_STRING", 0, 2),
                  "SOCOM 2 r0001 17:22:21 Oct 11 2003");
    for (;;) {
        CGame *game = &theGame;

        game->Tick(zVid.frameTime);
    }
}
