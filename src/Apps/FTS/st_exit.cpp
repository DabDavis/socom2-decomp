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

extern unsigned char data_004A4511;
extern CZAnimZAR *data_00408D80;                 /* animfile (the unit's, in an unnamed namespace) */
}

extern char theUIVarManager[];

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

    std::list<CEntity *>::iterator _it;   // 0x0
    int _iIdx;   // 0x4
};

/* on to the next SEAL, counting every entity passed.
 * OPEN (0x68 of 0x78): the target builds the list's end in a temporary and copies it bitwise
 * (lwc1/swc1) into a second before comparing -- SOCOM 1 called the const end(), then
 * list<CEntity *>'s const_iterator built from the base's by value (0x321240 in the demo);
 * include/list has no such wrapper iterator yet */
void CEntityItr::Advance()
{
    do {
        ++_it;
        _iIdx++;
    } while (_it != CEntity::m_list.end() && !&GetSeal());
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
    data_004A4511 = 0;
    func_003A3740(func_00361BC0(), "UIX");
    if (data_00408D80) {
        ZAnim.CloseZAR(data_00408D80);
        data_00408D80 = 0;
    }
    if (m_menu)
        delete m_menu;
    m_menu = 0;
    func_00321D30();
    func_00385C30(theUIVarManager);
    func_0032C2B0("readerx.zar", "run/ui");
    func_0027EDF0(func_00274930(), true);
}
