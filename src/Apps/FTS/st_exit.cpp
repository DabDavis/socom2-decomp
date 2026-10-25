/* st_exit.cpp -- the end of a mission: the stats screen (SOCOM 1's Apps/FTS/st_exit.cpp; OVL1
 * 0x1F1A00-0x1F3B1F). Defined in reverse of their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void *func_00361BC0();                           /* (the sound system; SOCOM 1 called CSnd::Close here) */
void func_003A3740(void *snd, const char *bank);
void func_00321D30();                            /* zdb::EmptyAssetRecycleBin */
void func_00385C30(void *vars);                  /* CUIVarManager::Reset */
void func_0032C2B0(const char *name, const char *path);   /* CRdrArchive::RemoveArchive */
void *func_00274930();                           /* GetMenuStack */
void func_0027EDF0(void *stack, bool all);       /* CMenuStack::FlushHistory */
void func_00341A60(void *data, float dT);        /* CSndInstance::PostTick */
void func_002D9E40(float dT);                    /* CInput::Tick */
void func_003654C0(CGameMenu *menu, float dT);   /* CGameMenu::Tick */
void func_003AFF30(bool a, bool b);              /* zVid_Swap */
void func_001EEB40(CStateString *s, const char *text);   /* std::string::assign */
void func_0032C1E0(const char *name, const char *path);   /* CRdrArchive::AddArchive */
void func_0032C570();                            /* CRdrArchive::OpenAll */
void *func_003A4460();
void func_003A3900(void *snd, const char *bank, void *arg);
void func_00368570(CGameMenu *menu, void *reloc, CZAnimZAR *zar);   /* CGameMenu::Init */
void func_003AFD50(int r, int g, int b, int a, int z);   /* CVideo::Clear */

}

extern char theUIVarManager[];

namespace {
/* the stats screen's anims */
CZAnimZAR *animfile;
}

const char *CExitMenu::GetReaderFileName()
{
    return "dlgExitState.rdr";
}

#include <list>

class CZSealBody;

/* a thing in the world: only its type (2, a SEAL) at 0x10 (SOCOM 1's DWARF) */
class CEntity {
public:
    unsigned int _0;
    unsigned int m_id;   // 0x4
    unsigned int m_index;   // 0x8
    unsigned int m_vehicle_index;   // 0xc
    unsigned char m_type;   // 0x10

    static std::list<CEntity *> m_list;
};

extern "C" bool func_002C2FD0(CZSealBody *seal);

/* a walk over the world's entities, counting (SOCOM 1's DWARF: the list's iterator, an index) */
class CEntityItr {
public:
    CZSealBody &GetSeal() const;
    bool IsSeal() const;
    bool IsLiveSeal() const;
    void Advance();

    /* the world's entities, const (CEntity::GetEntityList() returning a const reference -- SOCOM 1
     * links its out-of-line copy, GetEntityList__7CEntityFv, 0x1EA1C0; SOCOM II's is 0x299140 --
     * here until this unit's CEntity stub gives way to zentity.h's; the name is ours) */
    static const std::list<CEntity *> &GetEntityList() { return CEntity::m_list; }

    std::list<CEntity *>::const_iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* on to the next SEAL, counting every entity passed. The walk is a const_iterator and the end the
 * const list's (SOCOM 1's called list<CEntity *>::end() const, whose const_iterator is built from
 * the base's by value): a temporary copied bitwise into a second, rebuilt at every step; through
 * the accessor, not a cast of the list (that builds it once, before the loop) */
void CEntityItr::Advance()
{
    do {
        ++_it;
        _iIdx++;
    } while (_it != GetEntityList().end() && &GetSeal() == 0);
}

/* whether the walk is on a SEAL (the name ours) */
bool CEntityItr::IsSeal() const
{
    if (!&GetSeal())
        return false;
    return true;
}

/* whether the walk is on a SEAL for which func_002C2FD0 says no (the name ours) */
bool CEntityItr::IsLiveSeal() const
{
    if (!&GetSeal() || func_002C2FD0(&GetSeal()))
        return false;
    return true;
}

/* the entity the walk is on, as a SEAL -- or none (a null reference) */
CZSealBody &CEntityItr::GetSeal() const
{
    CEntity *e = (CEntity *)*_it;

    return *(CZSealBody *)(e && e->m_type == 2 ? e : 0);
}

/* the mission's objectives counted: complete and total, and the fraction complete, for the
 * primary, the secondary and the third kind (SOCOM 1 counted two in Init itself) */
struct CObjectiveCount {
    CObjectiveCount()
    {
        complete = 0;
        total = 0;
    }

    float fraction;
    int complete;
    int total;
};

struct CObjectiveCounts {
    CObjectiveCount primary;
    CObjectiveCount secondary;
    CObjectiveCount third;
};

extern "C" {
void func_001F38F0(CObjectiveCounts *counts, void *objectives);
void func_001F21B0(CObjectiveCounts *counts);   /* SetRoundStatUiVars */
}


/* the stats screen up: the objectives counted, the mission unloaded around the stats being set,
 * and -- unless the mission ends the game (popped) or the menu is skipped (popped and the load
 * state switched to) -- the reader archive, the menu, its anims, the UI's sound bank */
bool CExitState::Init()
{
    CObjectiveCounts counts;

    func_001F38F0(&counts, theMission.m_objectives);
    theMission.ReOpen_Unload1();
    func_001F21B0(&counts);
    theMission.ReOpen_Unload2();
    if (theMission.m_1ac->m_4 == 2) {
        func_002CF2F0(&theGame, 1);
        return true;
    }
    if (theGame.m_menuState.m_skip) {
        func_002CF2F0(&theGame, 0);
        func_002CF410(&theGame, &theGame.m_loadState, 0);
        return true;
    }
    func_0032C1E0("readerx.zar", "run/ui");
    func_0032C570();
    if (!m_menu)
        m_menu = new CExitMenu;
    animfile = ZAnim.OpenZAR("run/extzanim.zar");
    func_003A3900(func_00361BC0(), "UIX", func_003A4460());
    func_00368570(m_menu, 0, animfile);
    func_003AFD50(0, 0, 0, 0, 0);
    zVid.m_91 = 1;
    zVid_Sync0();
    return true;
}

/* the splash screen's asset library named; no menu yet */
CExitState::CExitState() : CGameState("ExitState")
{
    func_001EEB40(&m_libname, "common/assetlib/splash");
    m_menu = 0;
}

/* a frame of the stats screen: sound, input, the menu ticked and drawn, the frame swapped */
bool CExitState::Tick(float dT)
{
    func_00341A60(0, dT);
    func_002D9E40(dT);
    func_003654C0(m_menu, dT);
    m_menu->Draw2dOnly();
    zSysFifoEnd();
    func_003AFF30(true, true);
    return false;
}

/* torn down: the UI's sound bank, the screen's anims, the menu, the recycled assets, the UI's
 * variables, the reader archive and the menu history */
void CExitState::UnInit()
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    zVid.m_91 = 0;
    func_003A3740(func_00361BC0(), "UIX");
    if (animfile) {
        ZAnim.CloseZAR(animfile);
        animfile = 0;
    }
    if (m_menu)
        delete m_menu;
    m_menu = 0;
    func_00321D30();
    func_00385C30(theUIVarManager);
    func_0032C2B0("readerx.zar", "run/ui");
    func_0027EDF0(func_00274930(), true);
}
