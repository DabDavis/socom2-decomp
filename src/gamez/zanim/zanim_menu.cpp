/* zanim_menu.cpp -- the menu's ZAnim commands and the scene and database they work on (OVL1
 * 0x2744F0-...). The unit name is SOCOM 1's: its anonymous-namespace handlers mangle as
 * @unnamed@zanim_menu_cpp@. Defined in reverse of their image order (-inline deferred).
 *
 * The UI commands: a menu's sequence names a command (C2DAnimCmdHdr), zAnimUIMenuTick looks its
 * name up in UICommandTable (0x3DD4D0, 208 records in SOCOM II, 149 in SOCOM 1's) and calls the
 * record's handler with the command and the frame time; the handler returns whether the command is
 * done. Both builds register their handlers in that table by name, so each handler's row is named
 * by the command it is registered under: SOCOM 1's handler name for a command SOCOM 1 registers
 * (UI<command> mostly, some in the anonymous namespace), UI<command> after SOCOM 1's convention for
 * SOCOM II's new ones. The functions above 0x280000 (the equipment and saved-game handlers, the
 * template instances that close the unit) are not written yet. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zanim_menu.h"
#include "../zsys/zsys.h"

/* MAIN's strncasecmp (0x1993C0, newlib's; include/string.h has none: declared as ai.h does) */
extern "C" int strncasecmp(const char *a, const char *b, unsigned int n);

/* the lobby objects the handlers also reach (the LAN lobby: zonline.h's CZLANLobby, its object not
 * named yet) */
class CZLANLobby {
public:
    bool GameHasPassword(const char *game);   /* 0x303730 */
    void CreateGame();                        /* 0x303F40 */
    bool RefreshGameList(C2D *list);          /* 0x3042B0 */
    void Disconnect();                        /* 0x3043E0 */
    bool IsActive();                          /* 0x3045B0: whether the LAN lobby is the one in use */
    bool JoinGame(const char *game);          /* 0x3039B0 */
};
extern "C" {
extern CZLANLobby data_0044FE10;           /* a second lobby: the LAN game's */
void func_0030D770(CZNetwork *net);
void func_0030E3A0(CZNetwork *net, int arg);
extern int data_00441470;
extern char data_00437E30[];               /* the career stats */
}

/* the online armory (an object at 0x45B718; only the flag VCButton tests) */
struct COnlineArmoryView {
    unsigned char _0[0x4a];
    unsigned char m_4a;   // 0x4a
    unsigned char _4b[0x4f - 0x4b];
    unsigned char m_4f;   // 0x4f: the scale defaulted
    unsigned char m_50;   // 0x50: the range defaulted
};

/* a character type's figures the armory is told (six floats: the last three a range and a scale,
 * defaulted when negative; ours) */
struct CharTypeFigures {
    float m_0;
    float m_4;
    float m_8;
    float m_min;   // 0xc
    float m_max;   // 0x10
    float m_scale;   // 0x14
};

/* a character type's voice (zchar.h's CVoiceFont: only its six figures) */
class CVoiceFont {
public:
    float GetWhisperFactor() const;    /* 0x53B010 */
    float GetRoboSquash() const;       /* 0x53B020 */
    float GetRoboOffset() const;       /* 0x53B030 */
    float GetSpectralLedge() const;    /* 0x53B040 */
    float GetSpectralFactor() const;   /* 0x53B050 */
    float GetPitchOffset() const;      /* 0x53B060 */
};

extern "C" {
extern COnlineArmoryView *data_0045B718;
void func_0030E6E0(COnlineArmoryView *p, int arg);
void func_0030E490(COnlineArmoryView *p);
void func_0030E6A0(COnlineArmoryView *p, CharTypeFigures *figures);
}

class CZGameState {
public:
    static void UpdateLocalGame(CZNetGame *game);
};
void ftsNetworkGameInit();
void ftsNetworkGameUninit();
bool DMEConnectToGameServer();
void DMEDisconnectFromGameServer();

/* the 2D object a command's node draws, if it has one (ours). The visual is znode.h's zdb::CVisual:
 * its virtual call to the inline Get2DVisual (+0x1C, `return 0;`) makes this unit emit the
 * function's out-of-line copy, 0x274860 -- this unit's in both builds */
inline C2D *GetCommandC2D(unsigned short node_ref)
{
    C2D *c2d = 0;

    if (node_ref) {
        ZAnim.m_CurAnim->Refresh(false);
        CNode *node = ZAnim.m_CurAnim->GetNode(node_ref, false);

        if (node) {
            zdb::CVisual *visual = node->GetFirstVisual();

            if (visual) {
                C2DVisual *v2d = visual->Get2DVisual();

                if (v2d)
                    c2d = v2d->m_c2d;
            }
        }
    }
    return c2d;
}

char database[64];
zdb::CNode *g_menuRootNode;
/* the saved game's name the memory card handlers build (0x414E50) */
static char s_savename[64];

/* the video's still images (zvid's CVideo: only the restore), and what follows it (not named yet) */
class CVideo {
public:
    static void RestoreImage(char *name);   /* 0x3B1A70 (SOCOM 1's) */
};
extern "C" void func_003B0C00(int on);

namespace {

/* the selected characters one by one: a bit for each of the 32 in a mask, the weapons list of the
 * character the index is on (SOCOM 1's class; the members' names ours) */
class CSelectedCharsItr {
public:
    /* the first selected character of the valve's mask (SOCOM 1's, inlined in every handler) */
    CSelectedCharsItr(const char *valve)
    {
        m_index = -1;
        m_weapons = 0;
        CValve *v = CValve::GetByName(valve);

        m_mask = v ? v->m_value : 0;
        Advance();
    }
    void Advance();
    /* (an accessor: the loop's test then reads the index apart from the body, as the target's
     * does -- written out, MW keeps the test's load for the shift) */
    bool Valid() const { return m_index < 32; }

    unsigned int m_mask;   // 0x0
    int m_index;   // 0x4
    void *m_weapons;   // 0x8
};

}

/* ---- the handlers from 0x280000 to the deque's instances that close the unit (SOCOM 1 keeps the
 * ones it has -- RemoveEquipment, ListSavedGames, IsEquipmentSelected, CloseAndSwitch, the equipment
 * choices -- in zanim_menu.cpp's anonymous namespace, so the stretch is this unit's) ---- */

/* the weapons as the equipment handlers see them (zweapon.h's: SOCOM 1's EQUIP_ITEM, an UNSIGNED byte
 * -- EQUIP_NONE is 255: the weapon walks compare it zero-extended, andi 0xff against lbu -- and the
 * four lookups they make) */
enum EQUIP_ITEM { EQUIP_PISTOL = 0, EQUIP_END = 254, EQUIP_NONE = 255 };
class CZWeapon {
public:
    static EQUIP_ITEM GetWeaponId(const char *name);   /* 0x3D19E0 */
    static bool IsPrimary(EQUIP_ITEM item);            /* 0x3D1D10 */
    static bool IsSecondary(EQUIP_ITEM item);          /* 0x3D1CE0 */
    static EQUIP_ITEM GetIDType(EQUIP_ITEM item);      /* 0x3D1A60: the item's class */
    static CValve *GetEnableValve(EQUIP_ITEM item);    /* 0x3CF1F0: "Enable_<item>", none for the rest */
    static int GetMenuValue(EQUIP_ITEM item);          /* 0x3D3230: the value UiWeapon holds */
};
class CZFTSWeapon {
public:
    static bool IsM203Weapon(EQUIP_ITEM item);         /* 0x3C5E10 */
    bool IsM203Weapon();                               /* 0x3C5E40 */
};
/* the weapons' list (zweapon.h's: its lookup by item) and the list itself */
class CZWeaponList {
public:
    CZFTSWeapon *GetWeapon(EQUIP_ITEM item);           /* 0x3C4B40 */
};
extern CZWeaponList m_listWeapons__8CZWeapon;
/* a character's weapon (zchar.h's CCharacterWeap, SOCOM 1's DWARF: 12 bytes, the item first). Its
 * constructors and destructor are char_type.cpp's (OVL2 0x53EFD0, 0x53EFA0 member by member, and
 * 0x53EF50), out of line here: the tail's clear 0x285230 calls the destructor, its range assign
 * 0x284720 the copy and the destructor, the selection valves' update 0x282140 the default one. The
 * tail's two copies are byte-exact from this unit once those three OVL2 rows carry the names
 * (measured with them renamed; the rows are the AI core's: the integrator's) -- until then they
 * miss on the relocations alone, and 0x285230 keeps its draft's match */
enum EQUIP_AMMO { AMMO_END = 254, AMMO_NONE = 255 };
class CCharacterWeap {
public:
    CCharacterWeap();                             /* 0x53EFD0 (OVL2: char_type.cpp's) */
    CCharacterWeap(const CCharacterWeap &w);      /* 0x53EFA0 (OVL2: member by member) */
    ~CCharacterWeap();                            /* 0x53EF50 (OVL2: char_type.cpp's) */
    void SetupCharacterWeapon(EQUIP_ITEM item);   /* 0x53EEE0 */

    EQUIP_ITEM m_wepId;   // 0x0
    EQUIP_AMMO m_ammoId;   // 0x1
    int m_ammoCount;   // 0x4
    int m_magCount;   // 0x8
};
/* a type's weapons (zchar.h's: a class vector -- its size at 4, its elements at 8) */
class CCharacterWeapList : public std::vector<CCharacterWeap> {
public:
    bool RemoveWeapon(EQUIP_ITEM item);   /* 0x53B300 */
    void PadWeaponSlots();                 /* 0x53B070 */
};
void UpdateCharWeaponUiVars(int index);   /* 0x2AF190 */
const char *GetDefCharTypeName(int index);   /* 0x2AF760 */

namespace {

/* whether any selected character carries the item the argument names (SOCOM 1's). OPEN, 6
 * instructions short (0x120 against 0x138): the target re-reads the list's size and data at every
 * step of the walk (`lw 4(a2); lw 8(a2)` before each test: the end recomputed, the list kept in
 * a2), where MW hoists them out of the loop here -- SOCOM 1's computed the end once, as this does.
 * Tried, all 0x120 or shorter: the walk through a reference, a const reference and const
 * iterators, the iterator assigned, postfix/`+= 1`/`i = i + 1` steps, a while, a do-while behind
 * an if, a goto loop, a found flag with break (0x130), the walk as an inline member or free function
 * (21 off), the end spelled `begin() + size()` or through raw pointers, an index loop (0x100); the
 * item's unsigned compare (andi/lbu) is EQUIP_ITEM's (build/scratch/core/v_iseq.py). Round 12: the
 * found flag in either loop's condition with a break (SequenceRunning's shape: 0x118-0x134, the
 * blocks moved), the end through reference accessors on the list (the size `unsigned int &`, the
 * data's reference, a const local) -- MW hoists the end whatever its spelling, so the target's
 * loop holds something that blocks the hoist (build/scratch/core/v_iseq2.py, v_iseq3.py) */
bool OnIsEquipmentSelected(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance()) {
        CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;

        for (CCharacterWeapList::iterator i = weapons->begin(); i != weapons->end(); ++i)
            if ((*i).m_wepId == item)
                return true;
    }
    return false;
}

}

namespace {

/* whether the mission was won (SOCOM 1's: the UI command table's IsMissionSuccessful, id 2) */
bool OnIsMissionSuccessful(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMission.m_state == MISSION_SUCCESS;
}

/* a single-player mission's title by its code's first three characters, "UU" for none (SOCOM II's;
 * the name ours -- 0x2801C0 asks it of the mission it lists; the first four codes carry a space the
 * three-character compare never reaches) */
const char *GetMissionTitle(const char *code)
{
    if (!strncasecmp(code, "m51 ", 3))
        return "Seeding Chaos";
    if (!strncasecmp(code, "m52 ", 3))
        return "Terminal Transaction";
    if (!strncasecmp(code, "m53 ", 3))
        return "Upland Assault";
    if (!strncasecmp(code, "m61 ", 3))
        return "Urban Sweep";
    if (!strncasecmp(code, "m62", 3))
        return "Stranglehold";
    if (!strncasecmp(code, "m63", 3))
        return "Hydroelectric";
    if (!strncasecmp(code, "m71", 3))
        return "Guardian Angels";
    if (!strncasecmp(code, "m72", 3))
        return "Protect and Serve";
    if (!strncasecmp(code, "m73", 3))
        return "Against the Tide";
    if (!strncasecmp(code, "m81", 3))
        return "Lockdown";
    if (!strncasecmp(code, "m82", 3))
        return "Guided Tour";
    if (!strncasecmp(code, "m83", 3))
        return "Doomsday Delivery";
    return "UU";
}

/* the item the argument names taken from every selected character -- not a primary or a secondary,
 * which stay (SOCOM 1's) */
bool OnRemoveEquipment(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));

    if (CZWeapon::IsPrimary(item) || CZWeapon::IsSecondary(item))
        return true;
    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (((CCharacterWeapList *)it.m_weapons)->RemoveWeapon(item))
            UpdateCharWeaponUiVars(it.m_index);
    return true;
}

}

/* the per-character picks the new choosers make (SOCOM II's, not named yet): a primary weapon, an
 * item into one of the three equipment slots (2 to 4) */
bool ChoosePrimaryForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item);
namespace {
bool ChooseSlotForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item, int slot);
}
/* the selection's item valves brought up to date (SOCOM II's, not named yet: UpdateSelItemValves
 * 0x282410 is a jump to it) */
extern "C" void func_00282140();

/* the secondary weapon given to one selected character: its second weapon set up as the item unless
 * it is that already, its UI variables and the selection's valves updated (SOCOM II's; the name ours,
 * after ChooseEquipmentForCharacter) -- true either way */
inline bool ChooseSecondaryForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item)
{
    CCharacterWeap &w = (*(CCharacterWeapList *)it.m_weapons)[1];

    if (item == w.m_wepId)
        return true;
    w.SetupCharacterWeapon(item);
    UpdateCharWeaponUiVars(it.m_index);
    func_00282140();
    return true;
}

/* the item set into a slot after its old one is let go (SOCOM II's, 0x283170: not named yet) */
extern "C" void func_00283170(const CSelectedCharsItr &it, int slot, EQUIP_ITEM item);
/* the equipment slots a weapon takes in a loadout, 1 for an unknown item (zwep_weapon.cpp's 0x3D3750,
 * not named yet: char_type.cpp declares it alike) */
extern "C" int func_003D3750(EQUIP_ITEM item);
/* what each selected character's equipment slots held before a primary's own items went in (SOCOM
 * II's, two items a character; not named yet) */
struct CHAR_SLOT_PICKS {
    EQUIP_ITEM a;
    EQUIP_ITEM b;
};
extern CHAR_SLOT_PICKS data_003DD060[];

/* (the slot tests read an item's menu valve, CZWeapon::GetEnableValve: a value with 0x30 set keeps
 * the item in its slot -- the weapons' round 22 named 0x3CF1F0; read here as `lhu 4(v)`, CValve's
 * 16-bit value) */

/* the new choosers' slot helpers (SOCOM II's, the names ours), each called out of line: pinned. They
 * are defined in the reverse of their image order (IsSlotLocked 0x281950, FindItemSlot 0x282EB0,
 * HasSlotsForItem 0x284120, ChoosePrimaryForCharacter 0x284360, NextFreeSlot 0x284640), which is
 * their generation order -- load-bearing: a caller generated after its callee knows the registers it
 * writes, so NextFreeSlot keeps `slot` in a2 across FindItemSlot (defined ahead of its callees here,
 * it saved the slot in s1: 12 bytes long) */
namespace {

int FindItemSlot(const CSelectedCharsItr &it, EQUIP_ITEM item, int skip);
bool IsSlotLocked(CSelectedCharsItr it, int slot, EQUIP_ITEM item);
bool HasSlotsForItem(const CSelectedCharsItr &it, EQUIP_ITEM item, int slot);

#pragma auto_inline off
/* the slot an item goes into: the first empty one past the given, else the next of the equipment
 * slots (2 to 4, round) that is not locked (0x284640) */
int NextFreeSlot(const CSelectedCharsItr &it, int slot)
{
    int s = FindItemSlot(it, EQUIP_NONE, slot);

    if (s == -1) {
        s = slot + 1;
        if (s >= 5)
            s = 2;
        if (s < 2)
            s = 2;
        while (s == -1 || IsSlotLocked(it, s, EQUIP_NONE)) {
            s++;
            if (s >= 5)
                s = 2;
        }
    }
    return s;
}
#pragma auto_inline reset

}

/* the primary weapon given to one selected character, when it may take it: its first weapon set up
 * as the item unless it is that already, then the primary's own items into free equipment slots --
 * an M203 rifle's two grenades (141, 175), 143's 179, 142's 175, 63's 183 -- what each slot held
 * kept, the UI variables and the selection's valves updated (SOCOM II's, 0x284360; the name ours,
 * after ChooseEquipmentForCharacter). The first weapon's item a const local: the narrowed item's
 * temporary has 26 neighbours, one over K (25) once the item is pushed, so MW deferred it and
 * coloured it first (s0, the list s1, the character s2: 36 off); the const local, numbered just
 * below it and pushed first, drops it under K and gives the target's list s0, character s1,
 * narrowed item s2 and item s3 (the UI's round 18 rule; a const weapon pointer, the list through a
 * reference, the compare's operands swapped, a switch and dead first definitions did not) */
bool ChoosePrimaryForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item)
{
    if (!HasSlotsForItem(it, item, 0))
        return false;

    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;

    const EQUIP_ITEM first = (*weapons)[0].m_wepId;

    if (item == first)
        return true;
    func_00283170(it, 0, item);
    (*weapons)[0].SetupCharacterWeapon(item);

    CZFTSWeapon *w = m_listWeapons__8CZWeapon.GetWeapon(item);
    unsigned int slot;

    if (w && w->IsM203Weapon()) {
        slot = NextFreeSlot(it, -1);
        if (slot < weapons->size())
            data_003DD060[it.m_index].a = (*weapons)[slot].m_wepId;
        ChooseSlotForCharacter(it, (EQUIP_ITEM)141, slot);
        slot = NextFreeSlot(it, -1);
        if (slot < weapons->size())
            data_003DD060[it.m_index].b = (*weapons)[slot].m_wepId;
        ChooseSlotForCharacter(it, (EQUIP_ITEM)175, slot);
    } else if (item == 143) {
        slot = NextFreeSlot(it, -1);
        if (slot < weapons->size())
            data_003DD060[it.m_index].a = (*weapons)[slot].m_wepId;
        ChooseSlotForCharacter(it, (EQUIP_ITEM)179, slot);
    } else if (item == 142) {
        slot = NextFreeSlot(it, -1);
        if (slot < weapons->size())
            data_003DD060[it.m_index].a = (*weapons)[slot].m_wepId;
        ChooseSlotForCharacter(it, (EQUIP_ITEM)175, slot);
    } else if (item == 63) {
        slot = NextFreeSlot(it, -1);
        if (slot < weapons->size())
            data_003DD060[it.m_index].a = (*weapons)[slot].m_wepId;
        ChooseSlotForCharacter(it, (EQUIP_ITEM)183, slot);
    }
    UpdateCharWeaponUiVars(it.m_index);
    func_00282140();
    return true;
}

namespace {

/* whether a slot is locked against the item for any selected character (SOCOM II's; the name ours):
 * the lock's negation materialised (sltu, xori) before the branch, as `!(bool)` with a continue */
inline bool AnySlotLocked(int slot, EQUIP_ITEM item)
{
    for (CSelectedCharsItr c("UiCharSet"); c.Valid(); c.Advance()) {
        if (!(bool)IsSlotLocked(c, slot, item))
            continue;
        return true;
    }
    return false;
}

#pragma auto_inline off
/* whether a selected character has room for the item in its equipment slots (SOCOM II's, 0x284120;
 * the name ours): the three slots, more what the slot it would replace takes up, less every slot
 * locked against what the item keeps, against the slots the item takes -- two for 145, 146 and 152
 * and for 52 and 61 (whose own 141 a slot then keeps), one for 63, 142 and 143, none for anything
 * else. The cases are in reverse of the target's compare chains; the two switches written out (an
 * inline for the second takes the item by const reference to match, by value it colours the index
 * into v0 ahead of the switch's constant) */
bool HasSlotsForItem(const CSelectedCharsItr &it, EQUIP_ITEM item, int slot)
{
    int need = 0;
    EQUIP_ITEM keep = EQUIP_NONE;

    switch (item) {
    case 61:
    case 52:
        need = 2;
        keep = (EQUIP_ITEM)141;
        break;
    case 63:
    case 142:
    case 143:
        need = 1;
        break;
    case 145:
    case 146:
    case 152:
        need = 2;
        break;
    }
    int free = 3;
    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;

    if (slot < weapons->size()) {
        int n = 0;

        switch ((*weapons)[slot].m_wepId) {
        case 61:
        case 52:
            n = 2;
            break;
        case 63:
        case 142:
        case 143:
            n = 1;
            break;
        case 145:
        case 146:
        case 152:
            n = 2;
            break;
        }
        free += n;
    }
    for (int s = 2; s < 5; s++)
        if (AnySlotLocked(s, keep))
            free--;
    return free >= need;
}

/* a weapon put into a selected character's slot (SOCOM II's, 0x282F20; the name ours): an item taking
 * two slots first takes the next free one past it (round the equipment slots, past any locked) for
 * its second -- 185 for a 145, 186 for a 146, else EQUIP_END: what the slot held let go and kept in
 * the character's picks, or the second pushed on with the pick EQUIP_NONE -- then the UI variables
 * and the selection's valves updated. The slot and the second declared at the top, the weapon's item
 * read once into a local (lb, narrowed for the tests and passed on): the target's saved registers */
void PutInSlot(const CSelectedCharsItr &it, const CCharacterWeap &w, int slot)
{
    int s;
    EQUIP_ITEM second;
    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;

    if (func_003D3750(w.m_wepId) == 2) {
        s = FindItemSlot(it, EQUIP_NONE, slot);

        if (s == -1) {
            s = slot + 1;
            if (s >= 5)
                s = 2;
            while (s == -1 || IsSlotLocked(it, s, EQUIP_NONE)) {
                s++;
                if (s >= 5)
                    s = 2;
            }
        }

        EQUIP_ITEM id = w.m_wepId;

        second = EQUIP_END;
        if (id == 145)
            second = (EQUIP_ITEM)185;
        else if (id == 146)
            second = (EQUIP_ITEM)186;
        if (s < weapons->size()) {
            func_00283170(it, s, id);
            data_003DD060[it.m_index].a = (*weapons)[s].m_wepId;

            CCharacterWeap tmp;

            tmp.SetupCharacterWeapon(second);
            (*weapons)[s] = tmp;
        } else {
            data_003DD060[it.m_index].a = EQUIP_NONE;

            CCharacterWeap tmp;

            tmp.SetupCharacterWeapon(second);
            weapons->push_back(tmp);
        }
    }
    (*weapons)[slot] = w;
    UpdateCharWeaponUiVars(it.m_index);
    func_00282140();
}

/* the slot of a character's first weapon of the item past the one to skip, among the first five,
 * or -1 (0x282EB0) */
int FindItemSlot(const CSelectedCharsItr &it, EQUIP_ITEM item, int skip)
{
    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;
    int found = -1;
    int i = 0;

    for (CCharacterWeapList::iterator w = weapons->begin(), e = weapons->end();
         w != e && found == -1 && i < 5; ++w, ++i)
        if ((*w).m_wepId == item && i != skip)
            found = i;
    return found;
}

/* an item put into a selected character's equipment slot, or the first free one (-1: past the weapons
 * the slots hold, each its own count of slots, within the five) when it has room for it (SOCOM II's,
 * 0x282C20; the name ours): true when it is there already, else what the slot held let go, the
 * weapons padded with EQUIP_NONE up to an empty slot, the item set in through PutInSlot when the old
 * one took one or two slots, the UI variables updated */
bool ChooseSlotForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item, int slot)
{
    if (!HasSlotsForItem(it, item, slot))
        return false;

    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;

    if (item == (*weapons)[slot].m_wepId)
        return true;
    if (slot == -1) {
        for (slot = 2; slot < weapons->size(); slot += func_003D3750((*weapons)[slot].m_wepId))
            ;
        if (slot >= 5)
            return false;
    }

    CCharacterWeap w;

    w.SetupCharacterWeapon(item);

    EQUIP_ITEM cur = EQUIP_NONE;

    if (slot < weapons->size())
        cur = (*weapons)[slot].m_wepId;
    if (cur == item) {
        UpdateCharWeaponUiVars(it.m_index);
        return true;
    }
    func_00283170(it, slot, item);
    if (cur == EQUIP_NONE) {
        if (slot < weapons->size())
            PutInSlot(it, w, slot);
        else {
            while (slot >= weapons->size()) {
                CCharacterWeap pad;

                pad.SetupCharacterWeapon(EQUIP_NONE);
                weapons->push_back(pad);
            }
            PutInSlot(it, w, slot);
        }
    } else if (func_003D3750(cur) == 1)
        PutInSlot(it, w, slot);
    else if (func_003D3750(cur) == 2)
        PutInSlot(it, w, slot);
    UpdateCharWeaponUiVars(it.m_index);
    return true;
}

/* whether a slot keeps what it holds against the item: not when it holds the item itself; yes for
 * items 141 and 185, for 186 when no other slot holds one, for a weapon's padding (EQUIP_END) and for
 * an item whose definition is flagged 0x30 (0x281950; the true cases one ||'d test, their shared
 * `return true` block as the target's). The character by value: the callee's copy (12 bytes
 * through the FPRs) */
bool IsSlotLocked(CSelectedCharsItr it, int slot, EQUIP_ITEM item)
{
    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;
    EQUIP_ITEM cur = EQUIP_NONE;

    if (slot < weapons->size())
        cur = (*weapons)[slot].m_wepId;
    CValve *def = CZWeapon::GetEnableValve(cur);

    if (cur == item)
        return false;
    if (cur == 141 || cur == 185 || (cur == 186 && FindItemSlot(it, cur, slot) == -1) ||
        cur == EQUIP_END || (def && (def->m_value & 0x30)))
        return true;
    return false;
}
#pragma auto_inline reset

/* whether an equipment slot (0 to 2: slots 2 to 4) is locked for a selected character, with
 * UiCharLocked made the mask of every character whose slot is -- the selection widened to all of
 * them (eight on the network, else four) for the walk, then put back (SOCOM II's, 0x2815D0; the name
 * ours, after the three DoAllSelCharsHaveLockedEquip commands that jump to it). Its lock test is
 * IsSlotLocked's in another order, written out; a vector of items is made and never used (the
 * have-equipment test's, 0x281160, parses the command's argument into one -- a one-byte pod: the class
 * vector's teardown is 12 bytes shorter). The valve's value is read through CValve's const GetValue:
 * read so, MW keeps it apart from SetValue's own read of the same field -- the target reads m_value
 * twice for the saved selection and the lock count, where a plain read merges with SetValue's */
bool SelCharsHaveLockedEquip(int equip, C2DAnimCmdHdr *hdr)
{
    int slot = equip + 2;
    std::vector<char> items;
    CValve *set = CValve::GetByName("UiCharSet");
    unsigned int mask = set->GetValue();

    if (theNetwork.m_bNetwork)
        set->SetValue(255);
    else
        set->SetValue(15);

    CValve *locked = CValve::Create("UiCharLocked", VALVE_PERM);

    locked->SetValue(0);

    int bit = 1;

    for (CSelectedCharsItr c("UiCharSet"); c.Valid(); c.Advance()) {
        CCharacterWeapList *weapons = (CCharacterWeapList *)c.m_weapons;
        EQUIP_ITEM cur = EQUIP_NONE;

        if (slot < weapons->size())
            cur = (*weapons)[slot].m_wepId;
        CValve *def = CZWeapon::GetEnableValve(cur);

        if ((cur == 186 && FindItemSlot(c, cur, slot) == -1) || cur == 185 || cur == 141 ||
            cur == EQUIP_END || (def && (def->m_value & 0x30))) {
            unsigned int v = locked->GetValue();

            locked->SetValue(v + bit);
        }
        bit <<= 1;
    }
    set->SetValue(mask);
    return AnySlotLocked(slot, EQUIP_NONE);
}

}

/* "DoAllSelCharsHaveLockedEquip1" to "3" (SOCOM II's commands 0xC9-0xCB, the UI command table's
 * records 199-201 at 0x3DE140; the names ours, after the table's): equipment slots 0 to 2 */
bool UIDoAllSelCharsHaveLockedEquip3(C2DAnimCmdHdr *hdr, float *dT)
{
    return SelCharsHaveLockedEquip(2, hdr);
}

bool UIDoAllSelCharsHaveLockedEquip2(C2DAnimCmdHdr *hdr, float *dT)
{
    return SelCharsHaveLockedEquip(1, hdr);
}

bool UIDoAllSelCharsHaveLockedEquip1(C2DAnimCmdHdr *hdr, float *dT)
{
    return SelCharsHaveLockedEquip(0, hdr);
}

namespace {

/* whether a selected character may take an item with the weapons it has (SOCOM II's, 0x280E90 --
 * the name ours): the class-195 item only with a class-101 primary, item 186 only beside a 146 and
 * 146 or 145 only once; items 171/173/175 only on an M203 or item-142 primary, 176/178/179 only on
 * a 143, 181 to 183 only on a 63. The item's own class is looked up and dropped */
#pragma auto_inline off
bool CanTakeItem(EQUIP_ITEM item, const CSelectedCharsItr &it)
{
    CZWeapon::GetIDType(item);
    EQUIP_ITEM primary = (*(CCharacterWeapList *)it.m_weapons)[0].m_wepId;

    if (CZWeapon::GetIDType(primary) != 101 && item == 195)
        return false;
    if (item == 186 && FindItemSlot(it, (EQUIP_ITEM)146, -1) == -1)
        return false;
    if (item == 146 && FindItemSlot(it, (EQUIP_ITEM)146, -1) != -1)
        return false;
    if (item == 145 && FindItemSlot(it, (EQUIP_ITEM)145, -1) != -1)
        return false;
    if (!CZFTSWeapon::IsM203Weapon(primary) && primary != 142 && (item == 171 || item == 173 || item == 175))
        return false;
    if (primary != 143 && (item == 176 || item == 178 || item == 179))
        return false;
    if (primary != 63 && (item == 181 || item == 182 || item == 183))
        return false;
    return true;
}
#pragma auto_inline reset

}

/* "CanCharactersSelectUIWeapon" (SOCOM II's command 0xCF, the UI command table's record at 0x3DE1A0;
 * the name ours, after the table's): the item whose menu value UiWeapon holds -- the first of the
 * items below EQUIP_END to give it -- and whether every selected character may take it. The value
 * a local: compared in the call's expression, MW loads the valve's ahead of the call; the flag and
 * the counter first set in the for's head, the flag first (s0 to the counter's s1, the counter's
 * zero in the branch's delay slot) */
bool UICanCharactersSelectUIWeapon(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *v = CValve::GetByName("UiWeapon");

    if (!v)
        return false;
    int i;
    bool found;

    for (found = false, i = 0; i < 254 && !found; i++) {
        int value = CZWeapon::GetMenuValue((EQUIP_ITEM)i);

        if (value == v->m_value)
            found = true;
    }
    EQUIP_ITEM item = (EQUIP_ITEM)(i - 1);

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (!CanTakeItem(item, it))
            return false;
    return true;
}

namespace {

/* the item put on one selected character: none when the character has it already or carries fewer
 * than two weapons, else a primary or a secondary through its pick, anything else into the first free
 * equipment slot (-1); whether it took, the character's UI variables updated (SOCOM 1's, which set the
 * two weapons up itself and pushed the rest when four slots allowed). OPEN, 0x154 against 0x184:
 * the walk and the early return are the target's (its `i != e` narrowed and reused after the loop;
 * a1/a2 swapped), the three picks are not -- the target materialises each pick's result into the
 * bool (`move s0,zero; beqz v0; daddiu s0,zero,1; andi s0,s0,0xff` in every arm, the secondary's
 * inlined pick behind a `daddiu a1,zero,1; beqz a1` test of its own), where a bool assigned a bool
 * call is a move. `ok = false; if (f()) ok = true;` in the two calling arms gives their first three
 * instructions (0x16C); ternaries, int-returning picks (sltu), if-return wrappers, `&&` forms and a
 * ternary chain fold or lose (build/scratch/core/v_cefc.py). Round 9: each pick `!= false`, `?
 * true : false`, `!!`, `== true` or `!= 0`, and the three picks as an inline returning each (or
 * through a bool local) -- 0x154 to 0x16C. The target's secondary arm tests a constant 1 (`daddiu
 * a1,zero,1; beqz a1`) ahead of the inlined pick and narrows the pick's result (andi) at the join:
 * the pick sits inside an inline of its own whose bool MW does not fold */
bool ChooseEquipmentForCharacter(const CSelectedCharsItr &it, EQUIP_ITEM item)
{
    CCharacterWeapList *weapons = (CCharacterWeapList *)it.m_weapons;
    CCharacterWeapList::iterator i = weapons->begin(), e = weapons->end();

    for (; i != e; ++i)
        if ((*i).m_wepId == item)
            break;
    if (i != e || weapons->size() < 2)
        return false;

    bool ok;

    if (CZWeapon::IsPrimary(item))
        ok = ChoosePrimaryForCharacter(it, item);
    else if (CZWeapon::IsSecondary(item))
        ok = ChooseSecondaryForCharacter(it, item);
    else
        ok = ChooseSlotForCharacter(it, item, -1);
    UpdateCharWeaponUiVars(it.m_index);
    return ok;
}

/* the item the argument names put on every selected character: whether any took it (SOCOM 1's) */
bool ExecuteChooseEquipment(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool chosen = false;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (ChooseEquipmentForCharacter(it, item))
            chosen = true;
    return chosen;
}

}

namespace {

/* the default weapons of each selected character's type given to it, padded, or its weapons emptied
 * when the type has none (SOCOM 1's; SOCOM II pads the slots) -- the vector's assignment inlined:
 * the self test, then the range assign called */
bool ExecuteChooseDefaultEquipment(C2DAnimCmdHdr *hdr, float *dT)
{
    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance()) {
        CCharacterType *type = CCharacterType::GetByNameMod(GetDefCharTypeName(it.m_index));
        CCharacterWeapList *def = type ? (CCharacterWeapList *)type->m_weapons : 0;

        if (def) {
            *(CCharacterWeapList *)it.m_weapons = *def;
            ((CCharacterWeapList *)it.m_weapons)->PadWeaponSlots();
        } else
            ((CCharacterWeapList *)it.m_weapons)->clear();
        UpdateCharWeaponUiVars(it.m_index);
    }
    return true;
}

}

/* "ChooseDefaultEquipmentOnly" (SOCOM II's command 0x8F, the UI command table's record at 0x3DDDB0;
 * the name ours, after the table's): each selected character carrying two weapons or more given its
 * type's default weapons -- the vector's assignment inlined, as ExecuteChooseDefaultEquipment's --
 * its own primary and secondary set up again in the first two slots */
bool UIChooseDefaultEquipmentOnly(C2DAnimCmdHdr *hdr, float *dT)
{
    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance()) {
        if (((CCharacterWeapList *)it.m_weapons)->size() < 2)
            continue;
        EQUIP_ITEM primary = (*(CCharacterWeapList *)it.m_weapons)[0].m_wepId;
        EQUIP_ITEM secondary = (*(CCharacterWeapList *)it.m_weapons)[1].m_wepId;
        CCharacterType *type = CCharacterType::GetByNameMod(GetDefCharTypeName(it.m_index));
        CCharacterWeapList *def = type ? (CCharacterWeapList *)type->m_weapons : 0;

        if (def) {
            *(CCharacterWeapList *)it.m_weapons = *def;
            if (((CCharacterWeapList *)it.m_weapons)->size() >= 2) {
                (*(CCharacterWeapList *)it.m_weapons)[0].SetupCharacterWeapon(primary);
                (*(CCharacterWeapList *)it.m_weapons)[1].SetupCharacterWeapon(secondary);
                UpdateCharWeaponUiVars(it.m_index);
            }
        }
    }
    return true;
}

/* the primary the argument names given to every selected character: whether all took it (SOCOM II's;
 * the name ours by the command's) */
bool UIChoosePrimary(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool ok = true;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (!ChoosePrimaryForCharacter(it, item))
            ok = false;
    return ok;
}

/* the secondary the argument names given to every selected character: whether any took it (SOCOM
 * II's; the name ours by the command's) */
bool UIChooseSecondary(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool chosen = false;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (ChooseSecondaryForCharacter(it, item))
            chosen = true;
    return chosen;
}

/* the item the argument names put into an equipment slot of every selected character: whether any
 * took it (SOCOM II's three, slots 2 to 4; the names ours by the commands') */
bool UIChooseEquip1(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool chosen = false;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (ChooseSlotForCharacter(it, item, 2))
            chosen = true;
    return chosen;
}

bool UIChooseEquip2(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool chosen = false;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (ChooseSlotForCharacter(it, item, 3))
            chosen = true;
    return chosen;
}

bool UIChooseEquip3(C2DAnimCmdHdr *hdr, float *dT)
{
    EQUIP_ITEM item = CZWeapon::GetWeaponId(ArgName(hdr));
    bool chosen = false;

    for (CSelectedCharsItr it("UiCharSet"); it.Valid(); it.Advance())
        if (ChooseSlotForCharacter(it, item, 4))
            chosen = true;
    return chosen;
}

namespace {

/* the menu closed for the screen the argument names: the front end's state switched back to -- a
 * session's master instead moves the game on to the scores, unless it is over -- or, off the
 * network, switched back to with the loading image restored (SOCOM 1's; SOCOM II restores the
 * image) */
bool OnCloseAndSwitch(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_menuState.SetDefaultMenu(ArgText(ArgName(hdr)));
    if (theNetwork.m_bNetwork) {
        if (theNetwork.m_bSessionMaster) {
            CZNetGame *game = theMission.m_pNetGame;

            if (game->m_game_over) {
                theGame.Pop(0);
                theGame.Switch((CGameState *)&theGame.m_menuState, 0);
            } else
                game->MPMajorGameState((CZNetGame::MP_MAJOR_GAME_STATE)5);
            CZGameState::UpdateLocalGame(game);
        }
    } else {
        theGame.Pop(0);
        theGame.Switch((CGameState *)&theGame.m_menuState, 0);
        CVideo::RestoreImage("RUN\\RAW\\LOADING.RAW");
        func_003B0C00(1);
    }
    return true;
}

}

/* a UI variable's value given to another: the first named variable, when it exists, copied into
 * the second, made if need be (SOCOM II's; the name ours by the command's) */
bool UICopyUIVar(C2DAnimCmdHdr *hdr, float *dT)
{
    UICopyUIVarCmd *cmd = (UICopyUIVarCmd *)hdr;
    CZAnim *anim = ZAnim.m_CurAnim;
    const char *from = cmd->m_from >= 0 ? anim->GetName(cmd->m_from) : 0;
    CUIVariable *src = from ? theUIVarManager.Find(from) : 0;

    if (!src)
        return true;
    const char *to = cmd->m_to >= 0 ? anim->GetName(cmd->m_to) : 0;

    if (to)
        theUIVarManager.Add(to, 0, UIVAR_SESSION)->Set(src);
    return true;
}

namespace {

bool OnSetCounterSpeedFactor(C2DAnimCmdHdr *hdr, float *dT)
{
    UiSetCounterSpeedFactor(atof(ArgText(ArgName(hdr))));
    return true;
}

bool OnSetTtySpeedFactor(C2DAnimCmdHdr *hdr, float *dT)
{
    UiSetTtySpeedFactor(atof(ArgText(ArgName(hdr))));
    return true;
}

}

bool UIUpdateMpFinalStatsUiVars(C2DAnimCmdHdr *hdr, float *dT)
{
    SetFinalStatsUiVars();
    return true;
}

/* CopyUIVar's parser: the two variables' names (-1 for none). The command's type goes where the
 * other records keep the command's name (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICopyUIVarParse(const char *name, _zrdr *rdr)
{
    UICopyUIVarCmd *cmd = (UICopyUIVarCmd *)_calloc(1, sizeof(UICopyUIVarCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UICopyUIVarCmd);
    cmd->node_ref_index = 0;
    cmd->m_command_index = GetUICommandType(name);
    s = zrdr_findstring(rdr, "From");
    cmd->m_from = s ? ZAnim.m_CurAnim->AddName(s) : -1;
    s = zrdr_findstring(rdr, "To");
    cmd->m_to = s ? ZAnim.m_CurAnim->AddName(s) : -1;
    return (_zanim_cmd_hdr *)cmd;
}

/* SendChatText's and SendMediusChat's parser: the argument and the UI variable holding whom the
 * text is for (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIChatParse(const char *name, _zrdr *rdr)
{
    UIChatCmd *cmd = (UIChatCmd *)_calloc(1, sizeof(UIChatCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIChatCmd);
    cmd->node_ref_index = 0;
    cmd->m_command_index = GetUICommandType(name);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (s)
        cmd->SetArgument(s);
    s = zrdr_findstring(rdr, "RECIP_UIVAR");
    if (s && strlen(s) < 32)
        strcpy(cmd->m_recip_uivar, s);
    else
        cmd->m_recip_uivar[0] = 0;
    return (_zanim_cmd_hdr *)cmd;
}

namespace {

bool UISetControllerConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    CCtrlrConfigs::SelectConfig(ArgText(ArgName(hdr)));
    return true;
}

/* a two-digit BCD byte of the clock as a number (ours) */
#define FROM_BCD(x) ((((x) >> 4) & 0xf) * 10 + ((x) & 0xf))

/* a saved game written to the slot the argument numbers: its title the mission, the difficulty and
 * the local date and time; the saving valve cleared.
 * OPEN, 47 instructions, all in the six BCD conversions feeding sprintf (the frame, the calls, the
 * loads' order -- the arguments right to left -- and the rest match): the target's temporaries
 * take other registers and the products are added first. The conversion as a macro either way
 * round or as an inline taking an int or a byte measured the same */
bool _SaveSavedGame(C2DAnimCmdHdr *hdr, float *dT)
{
    sceCdCLOCK clock;
    char title[264];
    int slot = atoi(ArgText(ArgName(hdr)));

    theMemCard.DeleteScratchpad();
    sceCdReadClock(&clock);
    sceScfGetLocalTimefromRTC(&clock);
    int difficulty = CValve::Create("ai_difficulty", (VALVE_TYPE)1)->m_value;

    if (difficulty > 9)
        difficulty = 9;
    sprintf(title, "%-3s%d%d/%d/20%.2d %.2d:%.2d:%.2d", database, difficulty, FROM_BCD(clock.month),
            FROM_BCD(clock.day), FROM_BCD(clock.year), FROM_BCD(clock.hour), FROM_BCD(clock.minute),
            FROM_BCD(clock.second));
    sprintf(s_savename, "SaveGame%d", slot);
    theMemCard.SaveConfig(s_savename, title);
    theMemCard.CreateReserveFiles();
    CValve *valve = CValve::GetByName("SavingSavedGame");

    if (valve)
        SetValveValue(valve, 0);
    return true;
}

/* a saved game loaded from the slot the argument numbers, the loading valve cleared */
bool _LoadSavedGame(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgText(ArgName(hdr)));
    CValve *valve = CValve::GetByName("LoadingSavedGame");

    sprintf(s_savename, "SaveGame%d", slot);
    theMemCard.LoadConfig(s_savename);
    if (valve)
        SetValveValue(valve, 0);
    return true;
}

}

void SetDatabase(const char *name)
{
    strcpy(database, name);
}

char *GetDatabase()
{
    return database;
}

void RegisterMenuNode(zdb::CNode *node)
{
    g_menuRootNode = node;
}

void UnRegisterMenuNode()
{
    g_menuRootNode = 0;
}

zdb::CNode *GetMenuNode()
{
    return g_menuRootNode;
}


/* (pinned: __sinit_zanim_menu.cpp calls it, where MW would inline it into the initialiser) */
#pragma auto_inline off
CMenuStack::CMenuStack()
{
    m_top = 0;
    m_count = 0;
    m_popup = 0;
}
#pragma auto_inline reset

/* OPEN, 1 instruction (round 2: the size now right, written as FlushHistory(true) and CleanPopup()):
 * the history's member destruction reads m_free's data_ through the pointer its this-tests use,
 * `addiu v0,s2,3636 ... lw a0,8(v0)`, where MW folds it into lw a0,0xE3C(s2) -- m_used's, destroyed
 * after, is folded in the target too (lw a0,2096(s2)). Round 1: with FlushHistory written out the
 * same instruction was left; m_used as CMenuHistory's base gives the same fold. include/vector's
 * pointer back() and push_back a level lower would likely settle the first form. Round 3 (the core
 * lane): a user-declared ~CMenuHistory() {}, m_free or m_used as vector<void *>/<const char *>, a
 * CMenuHistory reference local, FlushHistory written out, the constructor defined after the
 * destructor or with mem-initializers -- the same instruction each time. (Our unit also emits an
 * out-of-line ~CMenuHistory and Flush the image does not hold; whether the linker strips them is
 * not known) */
CMenuStack::~CMenuStack()
{
    FlushHistory(true);
    CleanPopup();
}

/* the top entry's command done with: its argument's name reset, the entry popped as its command
 * says (SOCOM 1's, the same ten cases in the same order; the name unused in both) */
void CMenuStack::HandleCommand(const char *name)
{
    strcpy(m_entries[m_top].m_argname, DEFAULT_ARGNAME);
    CMenuEntry &entry = m_entries[m_top];

    switch (entry.m_id) {
    case -1:
        break;
    case 0:
        entry.m_id = -1;
        break;
    case 2:
        entry.m_id = -1;
        m_top--;
        break;
    case 1:
        m_top--;
        break;
    case 3:
    case 4:
        entry.m_id = -1;
        m_top--;
        break;
    case 5:
        entry.m_id = -1;
        m_top--;
        break;
    case 6:
    case 7:
    case 8:
        entry.m_id = -1;
        m_top--;
        break;
    }
    m_count--;
}

void CMenuStack::AddToHistory(const char *name)
{
    char *slot = m_history.m_free.back();

    m_history.m_free.pop_back();
    m_history.m_used.PtrVec::insert(m_history.m_used.PtrVec::end(), 1, (unsigned int &)slot);
    strcpy(slot, name);
}

/* the pop-up, made when there is none (SOCOM 1's, 0xBF0 bytes here: its constructor inlined, the
 * bitmap's and the text's constructors called) */
CPopUpDialog *CMenuStack::GetPopUp()
{
    if (!m_popup)
        m_popup = new CPopUpDialog;
    return m_popup;
}

/* the pop-up put on the stack with its argument (the text typed when the name says INPUT_STRING),
 * its node shown and the menu's sound played (SOCOM 1's, the sound SOCOM II's) */
void CMenuStack::PopUpDialog(const char *name)
{
    m_top++;
    std::string arg;

    if (!strcmp(name, "INPUT_STRING"))
        arg = ZAnim.m_CurAnim->m_arg_list->GetCharP(0);
    else
        arg = name;
    m_entries[m_top].m_id = 3;
    strcpy(m_entries[m_top].m_argname, arg.c_str());
    m_count++;
    CNode *node = GetMenuNode()->FindChild("POPUP", true);

    if (!node)
        return;
    node->SetActive(true);
    CSnd *sound = CSnd::GetSoundByName(".METAL");

    if (sound)
        sound->PlaySound(true, 0, 255);
}

/* the mission's reader archive let go of (SOCOM 1's builds the path and does not use it either) */
void Unload_MissionReader()
{
    const char *db = theMission.m_database;

    if (db) {
        char path[1024] = "run/";

        func_0039C580();
        strcat(path, db);
        CRdrArchive::RemoveArchive("readerm.zar", db);
    }
}

CRdrFile *FetchMissionFontsRdr()
{
    return theMission.m_fontsRdr;
}

void CMenuStack::CleanPopup()
{
    if (m_popup) {
        delete m_popup;
        m_popup = 0;
    }
}

/* (the base's push_back() and end() stand in for the vector's push_back() and back(): include/
 * vector's pointer back() is three high -- it calls the base's, which calls end() -- and its
 * push_back two. With them the name is fetched out of line (push_back's argument is checked a level
 * deeper), and FlushHistory, ~CMenuStack and FlushMenuHistory all need these pieces inlined whole
 * at their depths, which only the one-level forms give: SOCOM II's back() must be a level lower) */
void CMenuHistory::Flush()
{
    while (m_used.size()) {
        m_free.PtrVec::push_back(*(m_used.PtrVec::end() - 1));
        strcpy((char *)*(m_free.PtrVec::end() - 1), MENUHISTORY_STRPOOL);
        m_used.pop_back();
    }
}

void CMenuStack::FlushHistory(bool all)
{
    for (int i = 0; i < 40; i++)
        strcpy(m_entries[i].m_argname, DEFAULT_ARGNAME);
    m_history.Flush();
    if (m_popup && all) {
        delete m_popup;
        m_popup = 0;
    }
}

void CMenuStack::KillPopUp()
{
    if (m_popup) {
        CNode *node = GetMenuNode()->FindChild("POPUP", true);

        if (node)
            node->SetActive(false);
    }
}

/* (pinned: the stack's constructor calls it, where MW would inline it) */
#pragma auto_inline off
CMenuHistory::CMenuHistory()
{
    m_used.reserve(32);
    m_free.reserve(32);
    for (unsigned int i = 0; i < 32; i++) {
        strcpy(m_pool[i], MENUHISTORY_STRPOOL);
        m_free.push_back(m_pool[i]);
    }
}
#pragma auto_inline reset

/* the mission set by name (or by the UI variable of that name), the background film stopped
 * around the change and put back */
bool UISetMission(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));
    bool background = func_001F3BE0(&theGame.m_menuState);
    CUIVariable *var = theUIVarManager.Find(name);

    if (var && var->type == 3)
        name = var->String();
    strcpy(database, name);
    theMission.PreOpen(name);
    if (background)
        func_001F3BD0(&theGame.m_menuState, 0);
    return true;
}

bool UIPopUpDialog(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->PopUpDialog(ArgName(hdr));
    return true;
}

bool UIIsSessionMaster(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.m_bNetwork && theNetwork.m_bSessionMaster)
        return true;
    return false;
}

bool UIIsNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    return theNetwork.m_bNetwork;
}

/* the lobby's events, run as event anims by whichever of the two states with them is on top
 * (SOCOM 1's: then only the menu state's; InitNetwork hands it to the lobby) */
bool UILobbyEventHandler(const char *eventname, const char *argument)
{
    if (theGame.CurrentState() == (CGameState *)&theGame.m_menuState)
        return theGame.m_menuState.RunEventAnim(eventname, argument);
    if (theGame.CurrentState() == (CGameState *)theGame.m_coreState)
        return func_001EB070(theGame.m_coreState, eventname, argument);
    return false;
}

bool UIUnInitNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.m_event_handler = 0;
    theLobby.Close();
    func_0030D770(&theNetwork);
    func_0030E3A0(&theNetwork, 0);
    return true;
}

bool UIHostLANGame(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;

    theMission.CreateNetGame();
    if (data_0044FE10.IsActive()) {
        if (DMEConnectToGameServer()) {
            theLobby.ReleaseUIResources();
            ok = true;
        } else {
            theMission.DestroyNetGame();
        }
    }
    return ok;
}

bool UIConnectToGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok;

    theMission.CreateNetGame();
    if (func_002B85C0()) {
        if (data_0044FE10.IsActive())
            theLobby.ReleaseUIResources();
        ok = true;
    } else {
        theLobby.RejoinRoom();
        theMission.DestroyNetGame();
        ok = false;
    }
    return ok;
}

bool UIDisconnectFromGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    if (!data_0044FE10.IsActive() && theNetwork.m_bSessionMaster
        && theNetwork.zNetGetValidGameClientCount() == 1)
        theLobby.SendEndGameReport();
    ftsNetworkGameUninit();
    DMEDisconnectFromGameServer();
    theLobby.RejoinRoom();
    theMission.DestroyNetGame();
    if (data_0044FE10.IsActive())
        theLobby.CreateUIResources();
    return true;
}

bool UIJoinGameServer(C2DAnimCmdHdr *hdr, float *dT)
{
    if (!theMission.m_pNetGame->m_late_joiner) {
        if (!DMEJoinGameServer())
            return false;
        if (!theNetwork.m_bSessionMaster && !data_00441470)
            return false;
        ftsNetworkGameInit();
    }
    if (theNetwork.m_bSessionMaster)
        CZGameState::UpdateLocalGame(theMission.m_pNetGame);
    return true;
}

/* CheckJoinMP's last counts: function-local statics in the original (SOCOM 1's had one, its guard
 * beside it), held to their addresses by their placeholders; -1 in data */
extern "C" int data_003DD490;
extern "C" int data_003DD498;

bool UICheckJoinMP(C2DAnimCmdHdr *hdr, float *dT)
{
    int clients = theNetwork.zNetGetValidGameClientCount();

    if (clients != data_003DD490)
        data_003DD490 = clients;
    NetSessionInfo info;

    func_0030CDD0(&theNetwork, &info);
    if (info.m_count != data_003DD498)
        data_003DD498 = info.m_count;
    if (data_003DD490 == data_003DD498 && data_003DD490 == 1) {
        SetValveValue(theMission.m_pNetGame->m_pJoinGameValve, 10);
        theMission.m_pNetGame->m_joined = 1;
    }
    return true;
}

/* the voice chat's button: 1 on (unless already), 0 off -- not for a late joiner */
bool UIVCButton(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theMission.m_pNetGame && !theMission.m_pNetGame->m_late_joiner) {
        const char *text = ArgText(ArgName(hdr));

        if (*text == '1' && !data_0045B718->m_4a)
            func_0030E6E0(data_0045B718, 1);
        else if (*text == '0')
            func_0030E6E0(data_0045B718, 0);
    }
    return true;
}

bool UIIsMemCardInserted(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.IsInserted();
}

bool UIIsMemCardFormatted(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.IsFormatted();
}

bool UIIsMemcardSaveOK(C2DAnimCmdHdr *hdr, float *dT)
{
    int error = theMemCard.GetCardStatus();

    SetValveValue(CValve::Create("memcard_error_code", (VALVE_TYPE)2), error);
    return error == 0;
}

/* whether the slot the argument numbers holds a real saved game (not the dummies SaveConfig
 * writes) */
bool UIDoesSlotHaveGame(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgText(ArgName(hdr)));
    bool has = false;
    MemCardFileInfo info;

    sprintf(s_savename, "SaveGame%d", slot);
    if (theMemCard.Exists(s_savename)) {
        sprintf(s_savename, "SaveGame%d", slot);
        theMemCard.m_configSave->GetHeader(s_savename, 12, (CSaveManager::CSaveHeader *)&info);
        if (info.m_text[0] == 'm' && strcmp(info.m_text, "This game is a dummy game."))
            has = true;
    }
    return has;
}

/* the real saved games counted into the valve the argument names */
bool UIGetNumSavedGames(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);
    int count = 0;
    MemCardFileInfo info;

    for (int i = 0; i < 10; i++) {
        sprintf(s_savename, "SaveGame%d", i);
        if (theMemCard.Exists(s_savename)) {
            sprintf(s_savename, "SaveGame%d", i);
            theMemCard.m_configSave->GetHeader(s_savename, 12, (CSaveManager::CSaveHeader *)&info);
            if (info.m_text[0] == 'm' && strcmp(info.m_text, "This game is a dummy game."))
                count++;
        }
    }
    SetValveValue(valve, count);
    return true;
}

bool UIFormatMemCard(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.Format();
}

bool UILobbyRegister(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *name = theUIVarManager.Add("PLAYERNAMEVAR", 0, UIVAR_SESSION);
    CUIVariable *password = theUIVarManager.Add("PLAYERPASSWORD", 0, UIVAR_SESSION);
    const char *n = name->String();
    const char *p = password->String();

    if (n && *n && p && *p)
        return theLobby.Register(n, p);
    return false;
}

bool UILobbyLogin(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *name = theUIVarManager.Add("PLAYERNAMEVAR", 0, UIVAR_SESSION);
    CUIVariable *password = theUIVarManager.Add("PLAYERPASSWORD", 0, UIVAR_SESSION);
    const char *n = name->String();
    const char *p = password->String();

    if (n && *n && p && *p)
        return theLobby.Login(n, p);
    return false;
}

bool UILobbyLogout(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;

    if (theLobby.m_current_state == 0)
        done = true;
    else if (theLobby.m_current_state != 18)
        theLobby.Logout();
    return done;
}

bool UILobbyBailout(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.Bailout();
    return true;
}

bool UIGetAcctInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    return false;
}

bool UIRefreshGameList(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0044FE10.IsActive())
        return data_0044FE10.RefreshGameList(GetCommandC2D(hdr->node_ref_index));
    return theLobby.RefreshGameList(GetCommandC2D(hdr->node_ref_index));
}

bool UIRefreshRoomList(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.RefreshRoomList(GetCommandC2D(hdr->node_ref_index));
}

bool UIInitializeOnlineArmory(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0045B718) {
        func_0030E6E0(data_0045B718, 0);
        func_0030E490(data_0045B718);
    }
    return true;
}

bool UIRefreshRoomPlayerList(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text && !strcasecmp(text, "CLAN_TAGS"))
        return func_002F0EA0(&theLobby, GetCommandC2D(hdr->node_ref_index));
    return func_002F11F0(&theLobby, GetCommandC2D(hdr->node_ref_index));
}

bool UIRefreshGamePlayerList(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.RefreshGamePlayerList(GetCommandC2D(hdr->node_ref_index));
}

bool UICreateMPGame(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0044FE10.IsActive()) {
        data_0044FE10.CreateGame();
        return true;
    }
    return theLobby.CreateGame();
}

bool UIJoinMPGame(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);
    bool ok;

    name = !strcmp(name, "INPUT_STRING") ? ZAnim.m_CurAnim->m_arg_list->GetCharP(0) : name;
    if (data_0044FE10.IsActive())
        ok = data_0044FE10.JoinGame(name);
    else
        ok = theLobby.JoinGame(name);
    return ok;
}

/* a clan game's sides swapped: START (when the lobby's game is in state 4) locks the game and
 * reports the status, FINISH ends the swap */
bool UIClanGameSwapSides(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = true;
    const char *text = ArgText(ArgName(hdr));

    if (text) {
        if (!strcasecmp(text, "START")) {
            if (func_002E99E0() == 4) {
                SetValveValue(CValve::Create("mp_locked_game", (VALVE_TYPE)1), 1);
                theLobby.ForceStatusReport();
                done = true;
            } else {
                done = false;
            }
        } else if (!strcasecmp(text, "FINISH")) {
            func_002BADA0();
            done = true;
        }
    }
    return done;
}

bool UICanJoinRoom(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;
    const char *text = ArgText(ArgName(hdr));

    if (text)
        ok = theLobby.CanJoinRoom(text);
    return ok;
}

bool UIClanGameClosed(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8C60(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGameRejectsRank(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8BC0(&theLobby, ArgText(ArgName(hdr)));
}

bool UIGameHasPassword(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *game = ArgText(ArgName(hdr));

    if (data_0044FE10.IsActive())
        return data_0044FE10.GameHasPassword(game);
    return theLobby.GameIsPasswordProtected(game);
}

bool UIGameHasSpectatorPassword(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8D50(&theLobby, ArgText(ArgName(hdr)));
}

bool UIJoinMPRoom(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);
    const char *room;

    if (!strcmp(name, "INPUT_STRING"))
        room = ZAnim.m_CurAnim->m_arg_list->GetCharP(0);
    else if (!strcmp(name, "RANDOM"))
        room = 0;
    else
        room = name;
    return theLobby.JoinRoom(room);
}

/* SendMediusChat's text, sent to the lobby (SOCOM II's; the name ours) */
static void SendMediusChatCallback(const char *text, const char *to)
{
    theLobby.SendChat(text, to);
}

bool UISendMediusChat(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00281D40(hdr, SendMediusChatCallback, 1);
    return true;
}

bool UIGetMediusAnnouncements(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetAnnouncements();
    return true;
}

bool UIKillPopUp(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->KillPopUp();
    return true;
}

bool UIIsTerrorist(C2DAnimCmdHdr *hdr, float *dT)
{
    return !func_002C31A0();
}

unsigned int g_can_use_switch;

bool UISwitchSides(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.zNetGetLocalTime() > g_can_use_switch) {
        NetSendRequestTeamSwitch(theMission.m_pNetGame->m_persona_object_index);
        g_can_use_switch = theNetwork.zNetGetLocalTime() + 5000;
    }
    return true;
}

bool UIForceMPLaunch(C2DAnimCmdHdr *hdr, float *dT)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!game->m_ten_second_launch) {
        game->TenSecondLaunch(true);
        CZGameState::UpdateLocalGame(game);
    } else if (game->m_ten_second_launch) {
        unsigned int launch = game->m_launch_time;
        unsigned int now = theNetwork.zNetGetTime();

        if (now < launch - 5000) {
            game->TenSecondLaunch(false);
            CZGameState::UpdateLocalGame(game);
        }
    }
    return true;
}

/* SendChatText's text: to one player through the lobby, or to the game (SOCOM II's; the name ours) */
static void SendChatTextCallback(const char *text, const char *to)
{
    if (to)
        theLobby.SendChat(text, to);
    else
        SendChatMsg(text);
}

bool UISendChatText(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00281D40(hdr, SendChatTextCallback, 0);
    return true;
}

unsigned int g_can_use_ready;

bool UIReadyToLoad(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.zNetGetLocalTime() > g_can_use_ready) {
        if (theMission.m_pNetGame->m_late_joiner) {
            theMission.m_pNetGame->MyMajorGameState(CZNetGame::MP_MAJOR_LOADING);
            theMission.m_pNetGame->MyMinorGameState(CZNetGame::MP_MINOR_NOTREADY);
        } else {
            CZPersonaState::ToggleReady();
        }
        g_can_use_ready = theNetwork.zNetGetLocalTime() + 5000;
    }
    return true;
}

bool UIFlushMenuHistory(C2DAnimCmdHdr *hdr, float *dT)
{
    GetMenuStack()->FlushHistory(false);
    return true;
}

bool UIClearGameState(C2DAnimCmdHdr *hdr, float *dT)
{
    return true;
}

/* the game list's filter the argument names given its title (the filters' UI variables made first) */
bool UISetFilters(C2DAnimCmdHdr *hdr, float *dT)
{
    /* (the out-of-line call written out: through ArgName the inline's result would take its
     * saved register after the named locals) */
    const char *name = ZAnim.m_CurAnim->GetName(hdr->m_argument_index);
    CUIVariable *gametype = theUIVarManager.Add("FILTER_GAMETYPE", 0, UIVAR_SESSION);
    CUIVariable *ratecategory = theUIVarManager.Add("FILTER_RATECATEGORY", 0, UIVAR_SESSION);
    CUIVariable *map = theUIVarManager.Add("FILTER_MAP", 0, UIVAR_SESSION);
    CUIVariable *clanaffiliation = theUIVarManager.Add("FILTER_CLANAFFILIATION", 0, UIVAR_SESSION);
    CUIVariable *responsetime = theUIVarManager.Add("FILTER_RESPONSETIME", 0, UIVAR_SESSION);
    CUIVariable *buddies = theUIVarManager.Add("FILTER_BUDDIES", 0, UIVAR_SESSION);
    CUIVariable *clanonly = theUIVarManager.Add("FILTER_CLANONLY", 0, UIVAR_SESSION);
    CUIVariable *raterequirement = theUIVarManager.Add("FILTER_RATEREQUIREMENT", 0, UIVAR_SESSION);

    if (!strcasecmp(name, "GameType"))
        *gametype = "Game Type";
    else if (!strcasecmp(name, "RateCategory"))
        *ratecategory = "Rate Category";
    else if (!strcasecmp(name, "Map"))
        *map = "Map";
    else if (!strcasecmp(name, "ClanAffiliation"))
        *clanaffiliation = "Clan Affiliation";
    else if (!strcasecmp(name, "ResponseTime"))
        *responsetime = "Response Time";
    else if (!strcasecmp(name, "BuddiesOnly"))
        *buddies = "Buddies Only";
    else if (!strcasecmp(name, "ClanOnly"))
        *clanonly = "Clan Only";
    else if (!strcasecmp(name, "RateRequirement"))
        *raterequirement = "Rate Requirement";
    return true;
}

bool UIToggleAutoMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("AUTOMESSAGES", 0, UIVAR_SESSION);

    if (GetPlayAutoMessages()) {
        *var = "OFF";
        SetPlayAutoMessages(false);
    } else {
        *var = "ON";
        SetPlayAutoMessages(true);
    }
    return true;
}

bool UIActivateButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(6, button);
    return true;
}

bool UIEnableButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(7, button);
    return true;
}

bool UIDisableButton(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *button = ArgText(ArgName(hdr));

    GetMenuStack()->Push(8, button);
    return true;
}

bool UICheckVideoMode(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *mode = ArgText(ArgName(hdr));

    if (mode && *mode) {
        if (!strcasecmp(mode, "NTSC"))
            return zVid.vidMode == 0;
        if (!strcasecmp(mode, "DTV"))
            return zVid.vidMode == 2;
    }
    return true;
}

bool UISwitchProgressiveVideo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *mode = ArgText(ArgName(hdr));

    if (mode && *mode) {
        if (!strcasecmp(mode, "NTSC"))
            zVid.videoMode = 0;
        else if (!strcasecmp(mode, "DTV"))
            zVid.videoMode = 2;
    }
    return true;
}

bool UISetScreenOrigin(C2DAnimCmdHdr *hdr, float *dT)
{
    int y = CValve::Create("ScreenOriginY", (VALVE_TYPE)1)->m_value;
    int x = CValve::Create("ScreenOriginX", (VALVE_TYPE)1)->m_value;

    if (zVid.videoMode == 2)
        y += 18;
    zVid.pcrtcOffsetX = x - 128;
    zVid.pcrtcOffsetY = y - 128;
    return true;
}

/* the first pad's right stick given the preset the valve picks, its figures shown in the UI
 * variables (the sensitivity, the inertia as 1 less it, the dead zone less 0.02) */
bool UISetStickPreset(C2DAnimCmdHdr *hdr, float *dT)
{
    CPad *pad = CInput::m_pads[0];

    if (pad) {
        unsigned int preset = CValve::Create("stick_preset", (VALVE_TYPE)2)->m_value;

        if (preset < 3) {
            pad->SetPreset(1, preset);
            CUIVariable *sensitivity = theUIVarManager.Add("STICK_SENSITIVITY", 0, UIVAR_SESSION);
            CUIVariable *inertia = theUIVarManager.Add("STICK_INERTIA", 0, UIVAR_SESSION);
            CUIVariable *deadzone = theUIVarManager.Add("STICK_DEADZONE", 0, UIVAR_SESSION);

            sensitivity->SetReal(pad->GetAcceleration(1));
            inertia->SetReal(1.0f - pad->m_stickType[1].m_socom2_20);
            deadzone->SetReal(pad->GetDeadZone(1) - 0.02f);
        }
    }
    return true;
}

bool UIIsSpaceForSaveGameFile(C2DAnimCmdHdr *hdr, float *dT)
{
    int space = theMemCard.GetFreeSpace();

    if (space <= 3000)
        space += theMemCard.GetUsedSpace(0);
    return space >= 3000;
}

bool UIMC_Changed(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot;
    bool changed = func_003A1370(&theMemCard, 1, &slot);

    SetValveValue(CValve::Create("mc_changed_slot", (VALVE_TYPE)1), slot);
    return changed;
}

bool UIMC_ChDir(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.chdir(ArgName(hdr));
}

bool UIMC_MkDir(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.mkdir(ArgName(hdr));
}

bool UIMC_ChPort(C2DAnimCmdHdr *hdr, float *dT)
{
    int port = atoi(ArgName(hdr));

    if (port >= 0)
        theMemCard.m_selPort = port;
    theMemCard.SelectPortSlot();
    return true;
}

bool UIMC_ChSlot(C2DAnimCmdHdr *hdr, float *dT)
{
    int slot = atoi(ArgName(hdr));

    if (slot >= 0)
        theMemCard.m_selSlot = slot;
    theMemCard.SelectPortSlot();
    return true;
}

bool UIMC_Exists(C2DAnimCmdHdr *hdr, float *dT)
{
    return theMemCard.Exists(ArgName(hdr));
}

bool UIMC_CreateIconFiles(C2DAnimCmdHdr *hdr, float *dT)
{
    theMemCard.CreateIconFiles();
    return true;
}

bool UILoadConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    theMemCard.LoadGame(ArgName(hdr));
    CUIVariable *var = theUIVarManager.Add("AUTOMESSAGES", 0, UIVAR_SESSION);

    if (var->type == 3) {
        if (!strcasecmp(var->String(), "ON"))
            SetPlayAutoMessages(false);
        else
            SetPlayAutoMessages(true);
    }
    return true;
}

bool UISaveConfig(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);

    theMemCard.DeleteScratchpad();
    theMemCard.SaveGame(name, 0);
    for (int i = 0; i < 7; i++) {
        sprintf(s_savename, "SaveGame%d", i);
        if (!theMemCard.Exists(s_savename)) {
            sprintf(s_savename, "SaveGame%d", i);
            theMemCard.SaveConfig(s_savename, "This game is a dummy game.");
        }
    }
    theMemCard.CreateReserveFiles();
    return true;
}

bool UIMediusEnabled(C2DAnimCmdHdr *hdr, float *dT)
{
    return theNetwork.m_bMediusEnabled;
}

/* PlayMPEG's parser: the film by ARGUMENT or FILE (or its INDEX), whether it fades, plays in the
 * game and interlaced, the audio stream (a number, a valve or a UI variable), the rectangles and
 * the filtering (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIPlayMPEGParse(const char *name, _zrdr *rdr)
{
    char *var;
    char buf[800];
    UIPlayMPEGCmd *cmd = (UIPlayMPEGCmd *)_calloc(1, sizeof(UIPlayMPEGCmd), 0, 0);
    const char *s;
    _zrdr *stream;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPlayMPEGCmd);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (!s)
        s = zrdr_findstring(rdr, "FILE");
    cmd->m_by_index = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "INDEX");
        if (s)
            cmd->m_by_index = 1;
    }
    if (s && *s) {
        strcpy(buf, s);
        cmd->SetArgument(buf);
    }
    cmd->m_fade = true;
    zrdr_findbool(rdr, "FADE", &cmd->m_fade);
    cmd->m_ingame = true;
    zrdr_findbool(rdr, "INGAME", &cmd->m_ingame);
    cmd->m_interlace = true;
    zrdr_findbool(rdr, "INTERLACE", &cmd->m_interlace);
    cmd->m_stream_type = 0;
    cmd->m_stream = 0;
    stream = zrdr_findtag(rdr, "AUDIOSTREAM");
    if (stream) {
        if (stream->type == 1) {
            cmd->m_stream_type = 0;
            cmd->m_stream = stream->Int();
        } else {
            var = 0;
            if (zrdr_findSTRING(stream, "VALVE", (const char **)&var)) {
                cmd->m_stream_type = 1;
                cmd->m_stream = ZAnim.m_CurAnim->AddName(var);
            } else if (zrdr_findSTRING(stream, "UIVAR", (const char **)&var)) {
                cmd->m_stream_type = 2;
                cmd->m_stream = ZAnim.m_CurAnim->AddName(var);
            }
        }
    }
    cmd->m_has_dest = zrdr_findRECT(rdr, "DEST_RECT", (tag_RECT *)cmd->m_dest);
    zrdr_findRECT(rdr, "SOURCE_RECT", (tag_RECT *)cmd->m_source);
    cmd->m_bilinear = false;
    zrdr_findbool(rdr, "BILINEAR", &cmd->m_bilinear);
    return (_zanim_cmd_hdr *)cmd;
}

/* PlayMPEGNoFade's toggle: a function-local static in the original (SOCOM 1's UIPlayMPEG has one,
 * alt), held to its address by its placeholder -- scdiff leaves a local static's object unchecked */
extern "C" bool data_00414EA8;

bool UIPlayMPEGNoFade(C2DAnimCmdHdr *hdr, float *dT)
{
    CZSealBody *player = ftsGetPlayer();

    if (player && !player->m_e1_4)
        return true;
    bool was = data_00414EA8;

    data_00414EA8 = !was;
    if (!was) {
        ((CCinematicState *)theGame.m_cinematicSubState)->SetMovie(ArgName(hdr), 0, 0);
        theGame.Push((CGameState *)theGame.m_cinematicSubState, 0);
    }
    return was;
}

bool UIGetClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.GetClan();
}

bool UIClearCareerStats(C2DAnimCmdHdr *hdr, float *dT)
{
    ((CCareerStats *)data_00437E30)->Clear();
    return true;
}

bool UISetCareerStatsUIVars(C2DAnimCmdHdr *hdr, float *dT)
{
    ((CCareerStats *)data_00437E30)->SetUIVars();
    return true;
}

bool UICreateClan(C2DAnimCmdHdr *hdr, float *dT)
{
    CZAnim *anim = ZAnim.m_CurAnim;
    CUIVariable *var = theUIVarManager.Add("CLANNAMEVAR", 0, UIVAR_SESSION);

    if (var && var->type == 3) {
        theLobby.CreateClan(var->String());
        return true;
    }
    theLobby.CreateClan(anim->GetName(hdr->m_argument_index));
    return true;
}

bool UIDisbandClan(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.DisbandClan();
    return true;
}

bool UIIsMemberOfClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.IsMemberOfClan();
}

bool UIIsLeaderOfClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.IsLeaderOfClan(ArgText(ArgName(hdr)));
}

bool UIRemovePlayerFromClan(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.RemovePlayerFromClan(ZAnim.m_CurAnim->GetArgCharP(0));
    return true;
}

bool UICanInviteMorePlayersToClan(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002E5FE0(&theLobby);
}

bool UIInvitePlayerToClan(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("PLAYER_INVITE_NAME", 0, UIVAR_SESSION);

    if (var->type == 3) {
        CSaferStr name;

        if (var->type == 3)
            name = var->String();
        bool ok = theLobby.InvitePlayerToClan(name.c_str());

        return ok;
    }
    return true;
}

bool UIInvitePlayerToClanWithArg(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.InvitePlayerToClan(ArgText(ArgName(hdr)));
}

bool UIVagStreaming(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON")) {
        data_0048DDA0 = 1;
        CSnd::UIOpen();
    } else if (!strcmp(text, "OFF")) {
        CSnd::Close();
        data_0048DDA0 = 0;
    }
    return true;
}

/* the menu's background film: ON the default, OFF none, else the one named */
bool UIBackgroundMPEG(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON"))
        func_001F3BD0(&theGame.m_menuState, 0);
    else if (!strcmp(text, "OFF"))
        func_001F3BE0(&theGame.m_menuState);
    else
        func_001F3BD0(&theGame.m_menuState, text);
    return true;
}

bool UIAutoAim(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (!strcmp(text, "ON"))
        data_003DF1A0 = 1;
    else if (!strcmp(text, "OFF"))
        data_003DF1A0 = 0;
    return true;
}

bool UIGetInvitedPlayers(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetInvitedPlayers();
    return true;
}

bool UICheckClanInvites(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.CheckClanInvitations();
    return true;
}

bool UIGetClanRoster(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetClanRoster();
    return true;
}

bool UIRevokeInvite(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.RevokeInvite(ArgText(ArgName(hdr)));
}

bool UIGetFriendsList(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetFriendsList();
    return true;
}

bool UIGetFriendInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetFriendInvitations();
    return true;
}

bool UIGetIgnoreList(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetIgnoreList();
    return true;
}

bool UIGetClanRanking(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetClanRanking(0);
    return true;
}

bool UIDeleteClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.DeleteClanMessage(atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UIModifyClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    int index = atoi(ArgText(ArgName(hdr)));
    CUIVariable *var = theUIVarManager.Add("SEND_CLAN_MESSAGE", 0, UIVAR_SESSION);

    if (var && var->type == 3)
        theLobby.ModifyClanMessage(index, var->String());
    return true;
}

bool UIGetLeaderMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetLeaderMessages();
    return true;
}

bool UIAddFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("PLAYER_FRIEND_NAME", 0, UIVAR_SESSION);

    theLobby.AddFriend(var->String());
    return true;
}

bool UIAddFriendDirectly(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.AddFriendDirectly(atoi(text));
    return true;
}

bool UIAddToIgnore(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("PLAYER_IGNORE_NAME", 0, UIVAR_SESSION);

    theLobby.AddToIgnore(var->String());
    return true;
}

bool UIConfirmFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.ConfirmFriend(atoi(text));
    return true;
}

bool UIRejectFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.RejectFriend(atoi(text));
    return true;
}

bool UIGetGameInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *game = ArgText(ArgName(hdr));

    if (game) {
        if (!strcasecmp(game, "CurrentGame"))
            theLobby.GetCurrentGameInfo();
        else
            theLobby.GetGameInfo(game);
    }
    return true;
}

/* GetPlayerInfo's parser: the argument and what to ask for (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetPlayerInfoParse(const char *name, _zrdr *rdr)
{
    char buf[128];
    UIPlayerInfoCmd *cmd = (UIPlayerInfoCmd *)_calloc(1, sizeof(UIPlayerInfoCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPlayerInfoCmd);
    s = zrdr_findstring(rdr, "ARGUMENT");
    if (s && *s) {
        strcpy(buf, s);
        cmd->SetArgument(buf);
    }
    if (zrdr_findtag(rdr, "BRIEFING"))
        cmd->m_what = 0;
    else if (zrdr_findtag(rdr, "CLAN"))
        cmd->m_what = 1;
    else if (zrdr_findtag(rdr, "FRIEND"))
        cmd->m_what = 2;
    else if (zrdr_findtag(rdr, "LADDER"))
        cmd->m_what = 3;
    else
        cmd->m_what = 0;
    return (_zanim_cmd_hdr *)cmd;
}

bool UIGetPlayerInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));

    if (name)
        func_002EFE50(&theLobby, name, ((UIPlayerInfoCmd *)hdr)->m_what);
    return true;
}

bool UIGetFriendInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.GetFriendInfo(atoi(text));
    return true;
}

bool UIGetClanMemberInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        func_002E6000(&theLobby, text);
    return true;
}

bool UIRemoveFriend(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.RemoveFriend(atoi(text));
    return true;
}

bool UIRemoveFromIgnore(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *text = ArgText(ArgName(hdr));

    if (text)
        theLobby.RemoveFromIgnore(atoi(text));
    return true;
}

bool UISendClanMessage(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("SEND_CLAN_MESSAGE", 0, UIVAR_SESSION);

    if (var->type == 3) {
        CSaferStr text;

        if (var->type == 3)
            text = var->String();
        bool ok = theLobby.SendClanMessage(text.c_str());

        return ok;
    }
    return true;
}

bool UIGetUsagePolicy(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetUsagePolicy();
    return true;
}

/* the player's character type changed to the custom one the argument numbers (from 1), the armory
 * told its figures */
bool UIChangeCharacterType(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *type = GetCustomCharTypeName(atoi(ArgText(ArgName(hdr))) - 1);
    CZNetGame *game = theMission.m_pNetGame;

    strncpy(game->m_chartype, type, 31);
    game->m_chartype[30] = 0;
    CCharacterType *ct = CCharacterType::GetByNameMod(type);

    if (ct) {
        CharTypeFigures figures;
        void *stats = ct->m_stats;

        figures.m_0 = ((const CVoiceFont *)stats)->GetPitchOffset();
        figures.m_4 = ((const CVoiceFont *)stats)->GetSpectralFactor();
        figures.m_8 = ((const CVoiceFont *)stats)->GetSpectralLedge();
        figures.m_min = ((const CVoiceFont *)stats)->GetRoboOffset();
        figures.m_max = ((const CVoiceFont *)stats)->GetRoboSquash();
        figures.m_scale = ((const CVoiceFont *)stats)->GetWhisperFactor();
        if (figures.m_min < 0.0f || figures.m_max < 0.0f) {
            figures.m_min = 0.0f;
            figures.m_max = 0.0f;
            data_0045B718->m_50 = 1;
        }
        if (figures.m_scale < 0.0f) {
            figures.m_scale = 0.5f;
            data_0045B718->m_4f = 1;
        }
        func_0030E6A0(data_0045B718, &figures);
    }
    return true;
}

bool UIGetClanMessages(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.GetClanMessages();
    return true;
}

bool UIIsClanChallengeGame(C2DAnimCmdHdr *hdr, float *dT)
{
    return false;
}

/* the next mission of the rotation: the game's launch timed, and the session master picks the
 * mission (the lobby's choice, 254 the one after the current, 255 the first) and tells the game */
bool UIAdvanceMPMission(C2DAnimCmdHdr *hdr, float *dT)
{
    CZNetGame *game = theMission.m_pNetGame;

    if (!(game && theNetwork.m_bNetwork))
        return true;
    int wait = game->m_10->m_value ? 45000 : 30000;

    game->m_c8 = wait + theNetwork.zNetGetTime();
    if (theNetwork.m_bSessionMaster) {
        char name[16];
        char current[16];
        int index = theLobby.AdvanceMissionSequence();

        if (index == 254) {
            strncpy(current, database, 16);
            index = func_002C8DB0(&data_00441650, atoi(current + 2), 1)->m_index;
        }
        if (index == 255)
            index = static_cast<CMultiMissionList &>(data_00441650).GetRandom(true)->m_index;
        if (index > 0) {
            sprintf(name, "mp%d", index);
            strcpy(database, name);
        }
        CZGameState::UpdateLocalGame(game);
    }
    theMission.PreOpen(database);
    return true;
}

bool UISelectLocale(C2DAnimCmdHdr *hdr, float *dT)
{
    SetLanguage(LanguageFromName(ArgText(ArgName(hdr))));
    return true;
}

/* the UI variable the argument names, an array of missions, cut down to the suppression maps:
 * every entry kept but an array whose first value names a mission of another multiplayer kind
 * than 5 (SOCOM II's; the name ours) */
bool UIRemoveNonSuppressionMaps(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add(ArgText(ArgName(hdr)), 0, UIVAR_SESSION);

    if (var && var->type == 4) {
        CRdrEditor ed;

        ed.push(0);
        for (unsigned int i = 0; i < var->Count(); i++) {
            _zrdr *e = var->Item(i);

            if (e->type == 4) {
                if ((unsigned char)static_cast<CMultiMissionList &>(data_00441650).GetMultiValue(e->Item(0)->Int()) == 5)
                    ed.addrdrnode(e);
            } else {
                ed.addrdrnode(e);
            }
        }
        ed.pop();
        var->Set(&ed);
    }
    return true;
}

bool UIHaveBeenBannedFromGame(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002C3550();
}

bool UIGameStatePopCommand(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_menuState.SetDefaultMenu(ArgText(ArgName(hdr)));
    theGame.Pop(0);
    theGame.Switch((CGameState *)&theGame.m_menuState, 0);
    return true;
}

bool UISetDefaultMenuScreen(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_menuState.SetDefaultMenu(ArgText(ArgName(hdr)));
    return true;
}

bool UIBanSelfFromGame(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.BanSelfFromGame();
    return true;
}

/* the hard disk operation's last result (a byte; the name ours) */
extern "C" unsigned char data_00414EB0;

bool UIQuitClan(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.QuitClan();
    return true;
}

/* GetLadderInfo's parser: which list and which move, the clan ladder's numbered after the player
 * ladder's (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetLadderInfoParse(const char *name, _zrdr *rdr)
{
    UILadderCmd *cmd = (UILadderCmd *)_calloc(1, sizeof(UILadderCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UILadderCmd);
    if (zrdr_findtag(rdr, "CLAN")) {
        if (zrdr_findtag(rdr, "Top10"))
            cmd->m_op = 7;
        else if (zrdr_findtag(rdr, "LadderPressUp"))
            cmd->m_op = 8;
        else if (zrdr_findtag(rdr, "LadderPressDown"))
            cmd->m_op = 9;
        else if (zrdr_findtag(rdr, "LadderUp"))
            cmd->m_op = 10;
        else if (zrdr_findtag(rdr, "LadderDown"))
            cmd->m_op = 11;
        else
            cmd->m_op = 6;
    } else if (zrdr_findtag(rdr, "Top10")) {
        cmd->m_op = 1;
    } else if (zrdr_findtag(rdr, "LadderPressUp")) {
        cmd->m_op = 2;
    } else if (zrdr_findtag(rdr, "LadderPressDown")) {
        cmd->m_op = 3;
    } else if (zrdr_findtag(rdr, "LadderUp")) {
        cmd->m_op = 4;
    } else if (zrdr_findtag(rdr, "LadderDown")) {
        cmd->m_op = 5;
    } else {
        cmd->m_op = 0;
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* the hard disk operation's result, told to the menu through a valve and the lobby's event handler
 * (HardDriveOperation hands it on; SOCOM II's, the name ours) */
bool HardDriveOperationCallback(int result)
{
    int code;

    data_00414EB0 = result;
    switch (data_00414EB0) {
    case 1:
        code = 1;
        break;
    case 2:
        code = 2;
        break;
    case 3:
        code = 3;
        break;
    default:
        code = 5;
        break;
    }
    SetValveValue(CValve::Create("hard_disk_error_code", (VALVE_TYPE)1), code);
    if (theLobby.m_event_handler)
        theLobby.m_event_handler("HDD_ERROR", 0);
    return true;
}

/* the UI variable the argument names trimmed of its leading and trailing spaces */
bool UIPruneUIVarSpaces(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Find(ArgText(ArgName(hdr)));

    if (!var || var->type != 3)
        return true;
    {
        char text[800];

        sprintf(text, "%s", var->String());
        int len = strlen(text);

        if (len > 0) {
            char *end = &text[len - 1];

            while (*end == ' ')
                end--;
            end[1] = 0;
        }
        char *start = text;

        while (*start == ' ')
            start++;
        *var = start;
    }
    return true;
}

/* whether the UI variable the argument names holds something: a number not 0, a text not empty, an
 * array with items */
bool UITestUIVar(C2DAnimCmdHdr *hdr, float *dT)
{
    bool result = false;
    const char *name = ArgName(hdr);

    if (name) {
        CUIVariable *var = theUIVarManager.Find(name);

        if (var) {
            int type = var->type;

            if (type == 1) {
                result = var->Int() != 0;
            } else if (type == 2) {
                result = var->Real() != 0.0f;
            } else if (type == 3) {
                const char *text = var->String();

                result = text && *text;
            } else if (type == 4) {
                result = var->Count() != 0;
            }
        }
    }
    return result;
}

/* the UI variable the argument names emptied as its type has it: 0, 0.0, "" or an empty array. The
 * type read through _zrdr's own accessor (a `const _zrdr *` view): the inlined setters' reads of
 * the same bitfield through CUIVariable are then not merged with it, and re-read it as the target
 * does (lbu v0,0(s0)); read as `var->type`, MW reuses the dispatch's load in them (0x154 of 0x15C) */
bool UIClearUIVar(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgName(hdr);

    if (name) {
        CUIVariable *var = theUIVarManager.Find(name);

        if (var) {
            int type = var->Type();

            if (type == 1) {
                var->SetInt(0);
            } else if (type == 2) {
                var->SetReal(0.0f);
            } else if (type == 3) {
                *var = "";
            } else if (type == 4) {
                CRdrEditor rdr;

                rdr.push(0);
                rdr.pop();
                var->Set(&rdr);
            }
        }
    }
    return true;
}

bool UIAbortLoad(C2DAnimCmdHdr *hdr, float *dT)
{
    theGame.m_loadState.AbortLoad(ArgName(hdr));
    return false;
}

bool UINetCnfInit(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0030C6E0();
    return true;
}

bool UINetCnfUninit(C2DAnimCmdHdr *hdr, float *dT)
{
    func_0030C6A0();
    return true;
}

bool UINetCnfClose(C2DAnimCmdHdr *hdr, float *dT)
{
    CRdrEditor rdr;

    rdr.push(0);
    rdr.pop();
    theUIVarManager.Add("NETCNF_DB", &rdr, UIVAR_SESSION);
    strncpy(data_003DF360, "", 256);
    return true;
}

/* the valve the argument names set to the index of the configuration in use, among the valid ones.
 * The list walked through a pointer local defined after the default's lookup, its count after it
 * (SelectCombo's shape): valve s5, the default s4, the cursor s3, the count s2, the index s1 and
 * i s0, as the target numbers them (the count read before the lookup was 22 instructions off) */
bool UINetCnfCurrCombo(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgText(ArgName(hdr)), (VALVE_TYPE)2);
    NetCnfEntry *cur = ezNetCnfGetDefault(&data_00457C40);
    NetCnfList *list = &data_00457C40;
    int count = list->m_count;
    int index = 0;

    for (int i = 0; i < count; i++) {
        NetCnfEntry *e = list->Entry(i);

        if (e && e->m_type >= 0) {
            if (!strcmp(cur->m_name, e->m_name)) {
                SetValveValue(valve, index);
                break;
            }
            index++;
        }
    }
    return true;
}

/* the configuration the argument's valve indexes (among the valid ones) selected, when no memory
 * card has changed. The list is walked through a pointer local and its count read into a local
 * after it, both ahead of the index: the loop's strength-reduced cursor merges into the pointer
 * (numbered first), so the colouring runs entry a1, i a2, index a3, count t0, cursor t1 as the
 * target's; through the global itself the cursor and the count were the optimiser's temporaries,
 * numbered last (16 instructions off) */
bool UINetCnfSelectCombo(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = false;

    if (!func_003A1370(&theMemCard, 0, 0)) {
        CValve *valve = CValve::Create(ArgText(ArgName(hdr)), (VALVE_TYPE)2);

        if (valve) {
            NetCnfList *list = &data_00457C40;
            int count = list->m_count;
            int index = 0;

            for (int i = 0; i < count; i++) {
                NetCnfEntry *e = list->Entry(i);

                if (e && e->m_type >= 0 && index++ == valve->m_value) {
                    if (ezNetCnfGetNetcnfifData(data_003DF360, e->m_name, data_004567C0) >= 0)
                        ok = true;
                    break;
                }
            }
        }
    }
    return ok;
}

/* the network interfaces listed into the UI variable the argument names, "vendor/product" each.
 * The list is the buffer the inet library's interface calls fill (func_0030BD40 calls
 * sceInetGetInterfaceList and sceInetInterfaceControl, whose RPC buffers Sony's samples align to
 * 64): 64-byte aligned, it takes the frame's bottom (0x50) and pushes the reader to the top
 * (0xE80), the frame 0xED0 as the target's; the text, a block local of the loop, lands at 0xE00
 * past the alignment's hole (declared at the top it sits at 0xDD0, 2 instructions off; with no
 * alignment the reader sat lowest, 12 off) */
bool UINetGetInterfaceList(C2DAnimCmdHdr *hdr, float *dT)
{
    NetInterfaceInfo list[6] __attribute__((aligned(64)));
    int n = func_0030BD40(list, 6);
    CUIVariable *var = theUIVarManager.Add(ArgText(ArgName(hdr)), 0, UIVAR_SESSION);
    CRdrEditor rdr;

    rdr.push(0);
    for (int i = 0; i < n; i++) {
        char text[128];

        sprintf(text, "%s/%s", list[i].m_vendor, list[i].m_product);
        rdr.addstringnode(text);
    }
    rdr.pop();
    var->Set(&rdr);
    return true;
}

bool UIClanHasInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.ClanHasInvitations();
}

bool UIHasClanInvitations(C2DAnimCmdHdr *hdr, float *dT)
{
    return theLobby.HasClanInvitations();
}

bool UIStopAllSounds(C2DAnimCmdHdr *hdr, float *dT)
{
    func_00346C00();
    return true;
}

/* the IOP rebooted for what comes next (SOCOM 1's took the mode's name the same way) */
bool UIRebootIOP(C2DAnimCmdHdr *hdr, float *dT)
{
    CIOPManager::RebootMode mode = (CIOPManager::RebootMode)2;
    const char *text = ArgText(ArgName(hdr));

    if (!strcasecmp(text, "Singleplayer"))
        mode = (CIOPManager::RebootMode)2;
    else if (!strcasecmp(text, "Multiplayer"))
        mode = (CIOPManager::RebootMode)3;
    else if (!strcasecmp(text, "NetConfig"))
        mode = (CIOPManager::RebootMode)4;
    else if (!strcasecmp(text, "MainMenu"))
        mode = (CIOPManager::RebootMode)5;
    theIOPManager.RebootIOP(mode);
    return true;
}

bool UIFadeAllSounds(C2DAnimCmdHdr *hdr, float *dT)
{
    CSndInstance::FadeOutAllSounds();
    return true;
}

void AuxNetCensorText(const char *text, void *var)
{
    if (var)
        *((CUIVariable *)var) = text;
}

bool UINetCensorText(C2DAnimCmdHdr *hdr, float *dT)
{
    if (theNetwork.m_bNetwork && !data_0044FE10.IsActive()) {
        const char *name;
        CZAnim *anim = ZAnim.m_CurAnim;

        name = anim->GetName(hdr->m_argument_index);
        if (name && !strcasecmp("$1", name))
            name = anim->m_arg_list->GetCharP(0);
        CUIVariable *var = theUIVarManager.Find(name);

        if (var && var->type == 3)
            theLobby.Censor(var->String(), AuxNetCensorText, var);
    }
    return true;
}

bool UIForceMediusStatusReport(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.ForceStatusReport();
    return true;
}

bool UISetPlayerDescription(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *var = theUIVarManager.Add("PlayerDescription", 0, UIVAR_SESSION);

    if (var && var->type == 3)
        theLobby.SetPlayerDescription(var->String());
    return true;
}

bool UIUpdateClanStats(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.UpdateClanStats();
    return true;
}

bool UINotOnCorrectClanTeam(C2DAnimCmdHdr *hdr, float *dT)
{
    return !theLobby.OnCorrectClanTeam();
}

bool UIAcceptInvitation(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.AcceptClanInvitation(atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UIDeclineInvitation(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.DeclineClanInvitation(atoi(ArgText(ArgName(hdr))));
    return true;
}

bool UITransferLeadership(C2DAnimCmdHdr *hdr, float *dT)
{
    theLobby.TransferLeadership(ArgText(ArgName(hdr)));
    return true;
}

bool UIIsSwitchTeamsOK(C2DAnimCmdHdr *hdr, float *dT)
{
    return func_002F8CE0(&theLobby);
}

bool UIPasswordSecurity(C2DAnimCmdHdr *hdr, float *dT)
{
    CZOnlineLobby::PasswordSecurity();
    return true;
}

bool UISysPostInit(C2DAnimCmdHdr *hdr, float *dT)
{
    bool reopen;

    if (CFileCD::m_streamingSndActive) {
        reopen = true;
        CSnd::Close();
    } else {
        reopen = false;
    }
    zSysPostInit();
    if (reopen)
        CSnd::UIOpen();
    return true;
}

bool UISysLoadIRX(C2DAnimCmdHdr *hdr, float *dT)
{
    char name[256];

    strncpy(name, ArgText(ArgName(hdr)), 256);
    return true;
}

bool UICheckTeamStatus(C2DAnimCmdHdr *hdr, float *dT)
{
    bool ok = true;

    if (!theMission.m_pNetGame || !theNetwork.m_bSessionMaster && !data_00441470)
        ok = false;
    return ok;
}

bool UISuspendMenuInput(C2DAnimCmdHdr *hdr, float *dT)
{
    func_001F3BB0(&theGame.m_menuState, atof(ArgText(ArgName(hdr))));
    return true;
}

/* waiting for the network: on failure the lobby is reset and the menu told (an error text for
 * MP_MEDIUS_ERROR), the valve 0; once it is up, the valve 1 -- then done either way */
void CSMInitNetwork::WaitForNetwork(unsigned char event)
{
    char text[256];

    switch (event) {
    case 0:
        if (!theNetwork.zNetLoadIRX()) {
            theLobby.ChangeState(CONNECT_ERROR);
            theLobby.Tick();
            sprintf(text, "Network Initialization Error: %s (%d)", "Cannot Initialize Network", 0);
            theGame.m_menuState.RunEventAnim("MP_MEDIUS_ERROR", text);
            SetValveValue(m_valve, 0);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    case 1:
        if (func_0030BF20()) {
            SetValveValue(m_valve, 1);
            SetState(&CSimpleStateMachine::Finish);
        }
        break;
    case 2:
        break;
    }
}

/* the network's initialisation started, then waited for */
void CSMInitNetwork::Initialize(unsigned char event)
{
    switch (event) {
    case 0:
        func_0030E290(&theNetwork);
        SetState((SSMState)&CSMInitNetwork::WaitForNetwork);
        break;
    case 1:
        break;
    case 2:
        break;
    }
}

bool CSMInitNetwork::Start()
{
    CSimpleStateMachine::Start();
    SetState((SSMState)&CSMInitNetwork::Initialize);
    return true;
}

/* the machine InitNetwork runs (a function-local static in the original, likely: held to its
 * address by its placeholder) */
extern "C" CSMInitNetwork *data_00414EB8;

/* the personas on the lobby server listed into PLAYERPERSONALIST: each one's index and name, then
 * "<New Persona>" up to three (SOCOM II's; the name ours: UpdateAcctInfo, DeleteMUISPersona and
 * LobbyConnect call it) */
void UpdatePersonaList(const char *host, int port)
{
    CRdrEditor rdr;

    int n = data_0044FC60.Count(host, port);
    int i = 0;

    rdr.push(0);
    for (; i < n; i++) {
        const char *name = data_0044FC60.GetName(host, port, i);

        rdr.push(0);
        rdr.addintnode(i);
        rdr.addstringnode(name);
        rdr.pop();
    }
    for (; i < 3; i++) {
        rdr.push(0);
        rdr.addintnode(-1);
        rdr.addstringnode("<New Persona>");
        rdr.pop();
    }
    rdr.pop();
    (theUIVarManager.Add("PLAYERPERSONALIST", 0, UIVAR_SESSION))->Set(&rdr);
}

/* the network started: the lobby's events handed to the menu, the machine made and run until it
 * is done, the valve the argument names told how it went (SOCOM 1's built no machine).
 * OPEN, 0 instructions: every byte is right; the two calls name CSimpleStateMachine's constructor
 * and destructor, rows outside this lane (0x3A5DD0 __ct__19CSimpleStateMachineFv: the vtable
 * 0x408240 stored, the queue built, the idle state 0x3E07D8 copied in; 0x3A5CC0
 * __dt__19CSimpleStateMachineFv: the same vtable stored first, deleted when asked) -- matches once
 * the map names them (measured with both renamed) */
bool UIInitNetwork(C2DAnimCmdHdr *hdr, float *dT)
{
    bool done = false;

    if (!data_00414EB8) {
        CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

        theLobby.m_event_handler = UILobbyEventHandler;
        data_00414EB8 = new CSMInitNetwork(valve);
        data_00414EB8->Start();
    } else {
        data_00414EB8->Tick();
        if (data_00414EB8->IsDone()) {
            delete data_00414EB8;
            data_00414EB8 = 0;
            done = true;
        }
    }
    return done;
}

bool UIIsControllerConnected(C2DAnimCmdHdr *hdr, float *dT)
{
    bool connected = false;
    int port = atoi(ArgText(ArgName(hdr)));
    CPad *pad = (unsigned int)port < CInput::NUM_PADS ? CInput::m_pads[port] : 0;

    if (pad)
        connected = pad->m_dc == 1;
    return connected;
}

bool UIDMEInit(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

    theLobby.DMEInit();
    if (!theNetwork.zNetInitialize()) {
        func_0030E3A0(&theNetwork, 1);
        func_002B8660();
        SetValveValue(valve, 1);
    } else {
        SetValveValue(valve, 0);
    }
    return true;
}

bool UIMediusInit(C2DAnimCmdHdr *hdr, float *dT)
{
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);

    SetValveValue(valve, MediusInit());
    return true;
}

/* whether the persona the argument names exists on the lobby server (its host and port the UI
 * variables') */
bool UIMUISPersonaExists(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *name = ArgText(ArgName(hdr));
    const char *host = theUIVarManager.Add("LOBBY_HOST", 0, UIVAR_PERMANENT)->String();
    CUIVariable *port = theUIVarManager.Add("LOBBY_PORT", 0, UIVAR_PERMANENT);

    return data_0044FC60.Find(host, port->Int(), name);
}

bool UIVerifyPatchLevel(C2DAnimCmdHdr *hdr, float *dT)
{
    const char *level = ArgName(hdr);

    level = !strcmp(level, "INPUT_STRING") ? ZAnim.m_CurAnim->m_arg_list->GetCharP(0) : level;
    bool ok = data_0044FC60.VerifyPatchLevel(level);

    if (!ok) {
        CUIVariable *var = theUIVarManager.Add("PatchServerURL", 0, UIVAR_SESSION);

        *var = data_0044FC60.GetPatchURL(level);
    }
    return ok;
}

/* a mission's information put in the names its record carries: the mission the index valve picks
 * among the set's (single or multiplayer), its name to a UI variable and its number to a valve */
bool UIGetMissionInformation(C2DAnimCmdHdr *hdr, float *dT)
{
    UIMissionInfoCmd *cmd = (UIMissionInfoCmd *)hdr;
    const char *name = ZAnim.m_CurAnim->GetName(cmd->m_name);
    const char *number = ZAnim.m_CurAnim->GetName(cmd->m_number);
    const char *index = ZAnim.m_CurAnim->GetName(cmd->m_index);

    if (index && name && number) {
        int want = CValve::Create(index, (VALVE_TYPE)1)->m_value - 1;
        unsigned char multiplayer = cmd->m_multiplayer;
        int n = 0;

        for (std::vector<MPMissionInfo *>::iterator it = data_00441650.begin(); it != data_00441650.end(); ++it) {
            if (multiplayer == (*it)->m_multiplayer) {
                if (n == want) {
                    *(theUIVarManager.Add(name, 0, UIVAR_SESSION)) = (*it)->m_name.c_str();
                    SetValveValue(CValve::Create(number, (VALVE_TYPE)1), (*it)->m_number);
                    break;
                }
                n++;
            }
        }
    }
    return true;
}

/* GetMissionInformation's parser: the mission set and the names the mission's fields go to
 * (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UIGetMissionInformationParse(const char *name, _zrdr *rdr)
{
    UIMissionInfoCmd *cmd = (UIMissionInfoCmd *)_calloc(1, sizeof(UIMissionInfoCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIMissionInfoCmd);
    if (zrdr_findtag(rdr, "MULTIPLAYER"))
        cmd->m_multiplayer = 1;
    else
        cmd->m_multiplayer = 0;
    cmd->m_name = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "NAME"));
    cmd->m_number = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "NUMBER"));
    cmd->m_index = ZAnim.m_CurAnim->AddName(zrdr_findstring(rdr, "INDEX"));
    return (_zanim_cmd_hdr *)cmd;
}

/* CharacterProfile's parser: what to do with the profile (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICharacterProfileParse(const char *name, _zrdr *rdr)
{
    UIProfileCmd *cmd = (UIProfileCmd *)_calloc(1, sizeof(UIProfileCmd), 0, 0);

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIProfileCmd);
    if (zrdr_findtag(rdr, "EXISTS"))
        cmd->m_op = UIPROFILE_EXISTS;
    else if (zrdr_findtag(rdr, "LOAD"))
        cmd->m_op = UIPROFILE_LOAD;
    else if (zrdr_findtag(rdr, "SAVE"))
        cmd->m_op = UIPROFILE_SAVE;
    else if (zrdr_findtag(rdr, "SaveToServer"))
        cmd->m_op = UIPROFILE_SAVE_TO_SERVER;
    else if (zrdr_findtag(rdr, "WaitForServerSaveComplete"))
        cmd->m_op = UIPROFILE_WAIT_FOR_SERVER_SAVE;
    else if (zrdr_findtag(rdr, "LoadFromServer"))
        cmd->m_op = UIPROFILE_LOAD_FROM_SERVER;
    else if (zrdr_findtag(rdr, "WaitForLoadComplete"))
        cmd->m_op = UIPROFILE_WAIT_FOR_LOAD;
    else if (zrdr_findtag(rdr, "DELETE"))
        cmd->m_op = UIPROFILE_DELETE;
    else
        cmd->m_op = UIPROFILE_NONE;
    return (_zanim_cmd_hdr *)cmd;
}

/* the account's details sent to the lobby server (its host and port, the name, password, home town,
 * gender and whether the password is kept secure), then the personas listed again */
/* the gender a ternary of ACCT_GENDER's enumerators passed straight to SetAcctInfo's enum parameter
 * (the target's li/movz select: `x ? 1 : 0` to an int folds to sltu; the online lane's round 14
 * settled the parameter's type) */
bool UIUpdateAcctInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    CUIVariable *host = theUIVarManager.Add("LOBBY_HOST", 0, UIVAR_PERMANENT);
    CUIVariable *port = theUIVarManager.Add("LOBBY_PORT", 0, UIVAR_PERMANENT);
    CUIVariable *name = theUIVarManager.Add("PLAYERNAMEVAR", 0, UIVAR_SESSION);
    CUIVariable *password = theUIVarManager.Add("PLAYERPASSWORD", 0, UIVAR_SESSION);
    CUIVariable *hometown = theUIVarManager.Add("PLAYERHOMETOWN", 0, UIVAR_SESSION);
    CValve *gender = CValve::Create("PLAYERGENDER", (VALVE_TYPE)3);
    bool secure = CValve::Create("SecurePassword", (VALVE_TYPE)1)->m_value != 0;

    data_0044FC60.SetAcctInfo(host->String(), port->Int(), name->String(), password->String(), hometown->String(),
                              gender->m_value ? ACCT_FEMALE : ACCT_MALE, secure);
    UpdatePersonaList(host->String(), port->Int());
    return true;
}

/* GetMUISPersonaInfo's parser: the persona by INDEX or by NAME, the argument the one given (SOCOM
 * II's; the name ours) */
_zanim_cmd_hdr *UIPersonaParse(const char *name, _zrdr *rdr)
{
    UIPersonaCmd *cmd = (UIPersonaCmd *)_calloc(1, sizeof(UIPersonaCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UIPersonaCmd);
    s = zrdr_findstring(rdr, "INDEX");
    if (s) {
        cmd->m_by = 1;
    } else {
        s = zrdr_findstring(rdr, "NAME");
        cmd->m_by = 2;
    }
    cmd->m_argument_index = ZAnim.m_CurAnim->AddName(s);
    return (_zanim_cmd_hdr *)cmd;
}

/* CompareValues's parser: the two values, each a name or a UI variable's, and how to compare them
 * (SOCOM II's; the name ours) */
_zanim_cmd_hdr *UICompareValuesParse(const char *name, _zrdr *rdr)
{
    UICompareValuesCmd *cmd = (UICompareValuesCmd *)_calloc(1, sizeof(UICompareValuesCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UICompareValuesCmd);
    s = zrdr_findstring(rdr, "VALUE1");
    cmd->m_uivar1 = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "UIVAR1");
        cmd->m_uivar1 = 1;
    }
    if (s)
        cmd->m_value1 = ZAnim.m_CurAnim->AddName(s);
    s = zrdr_findstring(rdr, "VALUE2");
    cmd->m_uivar2 = 0;
    if (!s) {
        s = zrdr_findstring(rdr, "UIVAR2");
        cmd->m_uivar2 = 1;
    }
    if (s)
        cmd->m_value2 = ZAnim.m_CurAnim->AddName(s);
    if (zrdr_findtag(rdr, "CASE_INSENSITIVE"))
        cmd->m_case_insensitive = 1;
    else
        cmd->m_case_insensitive = 0;
    if (zrdr_findtag(rdr, "COMPARE_AFTER_FORMATTING"))
        cmd->m_after_formatting = 1;
    else
        cmd->m_after_formatting = 0;
    return (_zanim_cmd_hdr *)cmd;
}

/* CompareValues: the two values -- each a name, the typed text for INPUT_STRING, or the string of
 * the UI variable it names -- compared as strings, case-insensitively when asked and after each is
 * formatted when asked: whether they are the same (SOCOM II's; the name ours) */
bool UICompareValues(C2DAnimCmdHdr *hdr, float *dT)
{
    UICompareValuesCmd *cmd = (UICompareValuesCmd *)hdr;
    bool same = false;
    C2DString s1, s2;
    const char *a = CurAnim()->GetName(cmd->m_value1);
    const char *b = CurAnim()->GetName(cmd->m_value2);

    if (a && !strcmp(a, "INPUT_STRING"))
        a = CurAnim()->m_arg_list->GetCharP(0);
    if (b && !strcmp(b, "INPUT_STRING"))
        b = CurAnim()->m_arg_list->GetCharP(0);
    if (cmd->m_uivar1 == 1) {
        CUIVariable *v = theUIVarManager.Add(a, 0, UIVAR_SESSION);

        if (v->type == 3)
            a = v->String();
    }
    if (cmd->m_uivar2 == 1) {
        CUIVariable *v = theUIVarManager.Add(b, 0, UIVAR_SESSION);

        if (v->type == 3)
            b = v->String();
    }
    if (a && b && cmd->m_after_formatting) {
        s1.Format(a);
        a = s1.m_string + s1.m_firstCharPos;
        s2.Format(b);
        b = s2.m_string + s2.m_firstCharPos;
    }
    if (a && b) {
        if (cmd->m_case_insensitive)
            same = !strcasecmp(a, b);
        else
            same = !strcmp(a, b);
    }
    return same;
}

bool UILobbyDisconnect(C2DAnimCmdHdr *hdr, float *dT)
{
    if (data_0044FE10.IsActive())
        data_0044FE10.Disconnect();
    else
        theLobby.Close();
    return true;
}

/* the server's news put in SERVER_NEWS (its message, then the lines) and its name in SERVER_NAME,
 * for the server the argument's valve picks. The counter and the variable are defined first (dead
 * first definitions: MW numbers a variable where it is first defined), so the valve is numbered last
 * and takes s0, the two s1 -- declared at their first uses they took s0 and the valve s1 */
bool UIUpdateServerInfo(C2DAnimCmdHdr *hdr, float *dT)
{
    int i = 0;
    CUIVariable *name = 0;
    CValve *valve = CValve::Create(ArgName(hdr), (VALVE_TYPE)2);
    CRdrEditor rdr;

    rdr.push(0);
    rdr.addstringnode(data_0044FC60.GetUniverseDescription(valve->m_value));
    rdr.addstringnode("\n\n");
    for (i = 0; i < data_0044FC60.GetNewsCount(); i++)
        rdr.addstringnode(data_0044FC60.GetNewsLine(i));
    rdr.pop();
    theUIVarManager.Add("SERVER_NEWS", &rdr, UIVAR_SESSION);
    name = theUIVarManager.Add("SERVER_NAME", 0, UIVAR_SESSION);

    if (name)
        *name = data_0044FC60.GetUniverseName(valve->m_value);
    return true;
}

/* the menu's screen stack (SOCOM 1's g_MenuStack, 0x414ED0): the unit's one dynamically built
 * static, so its initialiser (__sinit_zanim_menu.cpp, 0x3FFCA0: CMenuStack's constructor, then
 * ~CMenuStack registered) sits first of the body code's in the table, ahead of bone_init.cpp's */
CMenuStack g_MenuStack;

CMenuStack *GetMenuStack()
{
    return &g_MenuStack;
}

/* the command's type by its name: 0 for none, the count of types for no name at all */
int GetUICommandType(const char *name)
{
    if (!name)
        return 232;
    if (!*name)
        return 0;
    /* (a do-while: the table is never empty -- SOCOM 1's has the same shape) */
    int i = 0;

    do {
        if (!strcasecmp(UICommandTable[i].name, name))
            return UICommandTable[i].id;
    } while (UICommandTable[++i].id);
    return 0;
}

/* a UI command's handler run: the table's own record when the type indexes it, else the record
 * that carries the type; true when there is none (ours) */
inline bool RunUICommand(C2DAnimCmdHdr *hdr, float *dT)
{
    int i = 0;
    int type;

    if (hdr->m_command_type)
        type = hdr->m_command_type;
    else {
        type = GetUICommandType(ZAnim.m_CurAnim->GetName(hdr->m_command_index));
        hdr->m_command_type = type;
    }
    if (UICommandTable[type].id == type) {
        if (UICommandTable[type].func)
            return UICommandTable[type].func(hdr, dT);
    } else {
        do {
            if (UICommandTable[i].id == type && UICommandTable[i].func)
                return UICommandTable[i].func(hdr, dT);
        } while (UICommandTable[++i].id);
    }
    return true;
}

/* UI_COMMAND's tick: the command goes to its node's 2D object when the node has one, else to the
 * handler its name registers (the names looked up and left unused are SOCOM 1's too: debug output
 * compiled away). The scan's index is declared first in RunUICommand (the target's s5, zeroed
 * before the type's lookup, the scan's preheader after the epilogue with a trampoline), the type an
 * if/else (the cached arm's `b` with the byte's `andi` in its slot, round 12: 0x2AC -> the size);
 * node_ref declared ahead of anim (17 -> 9).
 * OPEN, 9 instructions, registers only: node_ref and the C2D trade s3/s5 (the target's colouring
 * wants the C2D -- GetCommandC2D's own local, numbered after the caller's -- pushed ahead of anim and
 * node_ref in the first sweep: numbered below both). Tried in round 12: the declarations permuted
 * with a C2D local among them (tools/perm.py, 1544 steps, no gain), the C2D declared first, last or
 * as a dead first definition, the lookup written out in the tick (13), an early-return inline with
 * no local (0x2BC), the type through an inline of its own, the lookup in the caller, the index by
 * reference -- build/scratch/core/v_menu*.py */
bool zAnimUIMenuTick(_zanim_cmd_hdr *cmd, float *dT)
{
    C2DAnimCmdHdr *hdr = (C2DAnimCmdHdr *)cmd;
    bool handled = false;
    bool done = true;
    unsigned short node_ref;
    CZAnim *anim = ZAnim.m_CurAnim;

    anim->GetName(hdr->m_command_index);
    node_ref = hdr->node_ref_index;

    if (node_ref) {
        C2D *c2d = GetCommandC2D(node_ref);

        if (c2d) {
            handled = c2d->TickZAnimCmd(cmd, dT, &done);
        } else {
            ZAnim.m_CurAnim->GetNode(hdr->node_ref_index, false);
            anim->GetName();
        }
    }
    if (!handled) {
        anim->GetName(hdr->m_argument_index);
        done = RunUICommand(hdr, dT);
    }
    return done;
}

bool zAnimUIAppMenuTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
}

namespace {

/* on to the next selected character that has a type (and so its weapons). Called out of line by
 * every equipment handler in both builds, so pinned */
#pragma auto_inline off
void CSelectedCharsItr::Advance()
{
    for (m_index++; Valid(); m_index++) {
        if ((1 << m_index) & m_mask) {
            CCharacterType *type = CCharacterType::GetByNameMod(GetCustomCharTypeName(m_index));

            m_weapons = type ? type->m_weapons : 0;
            if (m_weapons)
                break;
        }
    }
}
#pragma auto_inline reset

}
