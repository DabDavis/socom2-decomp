/* zui_skb.cpp -- the soft keyboard: SOCOM 1's unit (its symbol table's @unnamed@zui_skb_cpp@ names
 * CSkbKey, CSkbKeyProps, CSkbGrid, CBmpWithNode and CSoftKeyboard), OVL1 0x38B180-0x39039F: the
 * keyboards' keys, their props map and the grid, then the template tail (the key map's tree, the
 * grid's vectors) closed by CBmpWithNode's this-adjusting destructor thunk at 0x390390, as SOCOM
 * 1's unit ends on @128@__dt__...CBmpWithNodeFv. SOCOM II moved the keyboard's rdr parsing, its
 * input and its drawing into the units after it (0x3903A0 on: the Korean input rules, the look's
 * parser 0x3963F0, CSkbKeyProps' destructor 0x394370), and its classes into a header they share
 * (zui_skb.h, which says why). Its static initialiser is 0x403730 (the keyboards' vector). Defined
 * in reverse of their image order (-inline deferred).
 *
 * The statics are SOCOM 1's names (its symbol table: g_iSkbGridPosX/Y, g_pActiveSkb, g_arrSkbs),
 * at SOCOM II's addresses (data.txt), still in the unit's unnamed namespace: no other unit
 * addresses them. */

#include <map>
#include <algorithm>
#include "zui_2d.h"
#include "../zsys/zsys.h"
#include "../znode/znode.h"
#include "zui_skb.h"

/* (the units after this one, by address: an rdr array's count, 0 for anything else or none; the
 * keyboards' shared bitmaps prepared with a callback; three more of their invalidations) */
extern "C" {
int func_003910B0(_zrdr *rdr);
void func_00398FC0();
void func_003958D0(zdb::CNode *node, zdb::CTextureRelocMgr *mgr, C2DFont *font, void (*callback)());
void func_00396F30();
void func_003997F0();
void func_0039AD10();
}

namespace {

/* the cursor's grid position (SOCOM 1's g_iSkbGridPosX/Y) and the keyboard showing (its
 * g_pActiveSkb) */
int g_iSkbGridPosX;
int g_iSkbGridPosY;
CSoftKeyboard *g_pActiveSkb;
/* the showing keyboard's IgnoreUnchangedString, copied when it is shown (the name ours) */
bool g_bIgnoreUnchanged;

/* the keyboards, as CreateSoftKeyboard makes them (SOCOM 1's g_arrSkbs) */
std::vector<CSoftKeyboard *> g_arrSkbs;

CSkbKey StrToSkbKey(const char *s);
void CreateSoftKeyboard(_zrdr *rdr);

/* the key an rdr item names: a key name, or a code (ours) */
inline CSkbKey SkbItemKey(_zrdr *item)
{
    return item->Type() == 3 ? StrToSkbKey(item->String()) : CSkbKey(0, item->Integer());
}

/* the grid grown to hold (x, y) and the key set there (ours) */
inline void SkbSetGridKey(CSoftKeyboard *skb, int x, int y, const CSkbKey &key)
{
    if (skb->m_grid.size() <= y)
        skb->m_grid.resize(y + 1);
    if (skb->m_grid[y].size() <= x)
        skb->m_grid[y].resize(x + 1);
    skb->m_grid[y][x] = key;
}

/* a keyboard deleted (as a function object, std::for_each's) */
struct DeleteSkb {
    DeleteSkb() {}
    void operator()(CSoftKeyboard *skb) { delete skb; }
};

}

/* the key under the cursor (SOCOM 1's GetCurKey: the grid positions are loaded first, as an
 * inline's arguments) */
inline const CSkbKey &CSoftKeyboard::GetCurKey() const
{
    return m_grid.GetSkbKey(g_iSkbGridPosX, g_iSkbGridPosY);
}


/* every keyboard's display invalidated, none showing (SOCOM 1's; CGameMenu::Clear calls it) */
void InvalidateSkbsDisplay()
{
    g_pActiveSkb = 0;
    for (int i = 0; i < g_arrSkbs.size(); i++)
        g_arrSkbs[i]->InvalidateDisplay();
    func_00396F30();
    func_003997F0();
    func_0039AD10();
}

/* every keyboard prepared for the menu's display, none showing (SOCOM 1's; CGameMenu::LoadFromDesign
 * calls it) */
void PrepSkbsForDisplay(zdb::CNode *node, zdb::CTextureRelocMgr *mgr, C2DFont *font)
{
    g_pActiveSkb = 0;
    func_003958D0(node, mgr, font, func_00398FC0);
    for (int i = 0; i < g_arrSkbs.size(); i++)
        g_arrSkbs[i]->PrepForDisplay(mgr, font);
}

/* the keyboards deleted and their vector freed (SOCOM II's: CMenuState::UnInit calls it; the name
 * ours, after ZuiInit) */
void ZuiUnInit()
{
    std::for_each(g_arrSkbs.begin(), g_arrSkbs.end(), DeleteSkb());
    FreeVector(g_arrSkbs);
}

/* the keyboards made from UiParams.rdr's SoftKeyboards block (SOCOM 1's; CMenuState::Init calls it) */
void ZuiInit()
{
    CRdrFileHolder file("UiParams.rdr", "data/common");
    _zrdr *skbs = zrdr_findtag(file, "SoftKeyboards");

    for (int i = 0; i < func_003910B0(skbs); i++)
        CreateSoftKeyboard(skbs->Item(i));
}

namespace {

/* a keyboard made from its rdr block and added to the keyboards: its look, its key props, its grid
 * of keys (rows of names or codes) and whether an unchanged string counts (SOCOM 1's
 * CreateSoftKeyboard, SOCOM II's split three ways).
 * OPEN, the target's size, ~100 instructions shifted: one register short. Round 12 put in C2DFont
 * (the fonts' constructor 0x38E550 now called as the target does), an int column counter (Item's
 * unsigned index then recomputes x*8 each pass, as the target, which an unsigned counter
 * strength-reduces), the grid store through an inline taking the keyboard (the target reloads skb
 * once a pass into s0), the key bound to a const reference (the ternary's copy then lives at
 * 184(sp), the target's slot) and a const item (its type byte loaded once for Type(), String() and
 * Integer(), as the target). What is left: the grid store reads the key's byte off a REMATERIALISED
 * &key (`addiu v0,sp,184` each pass) and its code through the copy's hoisted &key.m_code (s6),
 * where this build hoists &key itself into a saved register for both reads and so spills the grid
 * (s8 in the target) to the stack. Tried: the key by value, through an inline returning it, passed
 * straight to the store's inline (by reference and by value), a written operator= (casts or the
 * union views, either order) -- none moves the reads off one base. It stays for the member
 * constructors it emits (0x38E550, 0x38E620, 0x38E730, 0x38E770, 0x38E890, 0x38E8D0 byte-exact),
 * CSkbLook() (0x38E790, code-exact: its sources' destructor 0x38D130 is ~vector of a 0x2C-byte
 * record, not of CSkbKey -- zui_skb.h's m_sources) and the key vectors' instances */
void CreateSoftKeyboard(_zrdr *rdr)
{
    CSoftKeyboard *skb = new CSoftKeyboard;

    g_arrSkbs.push_back(skb);
    skb->ParseLook(rdr);
    skb->ParseKeyProps(rdr);
    _zrdr *grid = zrdr_findtag(rdr, "Grid");
    for (int y = 0; y < func_003910B0(grid); y++) {
        _zrdr *row = grid->Item(y);
        for (int x = 0; x < row->Count(); x++) {
            const _zrdr *item = row->Item(x);
            const CSkbKey &key = item->Type() == 3 ? StrToSkbKey(item->String()) : CSkbKey(0, item->Integer());

            SkbSetGridKey(skb, x, y, key);
        }
    }
    skb->m_ignoreUnchangedString = zrdr_findtag(rdr, "IgnoreUnchangedString") != 0;
}

}

/* (the next units' and this one's not yet written, by address: the node lists' layout from a grid
 * size, a keyboard's elements shown or hidden) */
extern "C" {
void func_00395140(int w, int h, CSkbTexLists *lists, CSoftKeyboard *skb);
void func_0038B580(CSoftKeyboard *skb, int which);
void func_00394420();
void func_00398B50();
void func_00399C10(CSoftKeyboard *skb);
void func_00395DE0(int on);
/* CGameMenu::RunEventAnim (zui.cpp's 0x367810: the menu, an event, a flag) */
void func_00367810(CGameMenu *menu, int event, int flag);
}

extern "C" {
int func_003999D0();
int func_003998A0();
int func_00395110();
int func_003996B0();
void func_00399D20(CGameMenu *menu);
void func_0038C000(CSoftKeyboard *skb, CGameMenu *menu);
}

/* the cursor's axis (ours: a byte enum, passed as `li 1; movz`) */
enum SKBAXIS { SKB_AXIS_ROW, SKB_AXIS_COLUMN };

extern "C" {
/* the cursor moved along an axis by a step until it lands on another key (0x38C130), and its two
 * steps (0x38C4B0, 0x38C4C0) */
void func_0038C130(CSoftKeyboard *skb, SKBAXIS axis, int (*step)(int));
int func_0038C4B0(int i);
int func_0038C4C0(int i);
}

/* up or down (directions 1 and 2) move within the column (ours) */
static inline bool SkbIsVertical(unsigned char dir)
{
    return dir == 1 || dir == 2;
}

/* the cursor moved one key in a direction: 1 up, 2 down, 3 left, 4 right (0x38C450; 0x38CB80 jumps
 * here; the name ours) */
void SkbMoveCursor(CSoftKeyboard *skb, unsigned char dir)
{
    /* the axis into a local first: in the call's list it would be chosen after the step */
    SKBAXIS axis = SkbIsVertical(dir) ? SKB_AXIS_ROW : SKB_AXIS_COLUMN;

    func_0038C130(skb, axis, dir == 2 || dir == 4 ? func_0038C4C0 : func_0038C4B0);
}

/* a count of none (ours: an inline bool returning the negation, materialised before each branch) */
static inline bool IsNone(int n)
{
    return !n;
}

/* the enter key: nothing while 0x3999D0 says so; an unchanged string (when that counts) or an
 * empty one goes to 0x38C000, anything else is taken and the keyboard closes (SOCOM II's; the
 * name ours) */
void CSoftKeyboard::OnEnterKey(CGameMenu *menu)
{
    if (!func_003999D0()) {
        bool unchanged = false;

        if (g_bIgnoreUnchanged && func_003998A0())
            unchanged = true;
        bool empty = false;

        if (IsNone(func_00395110()) && IsNone(func_003996B0()))
            empty = true;
        if (unchanged || empty) {
            func_0038C000(this, menu);
            return;
        }
        func_00399D20(menu);
        ShutdownBegin(menu);
        func_00395DE0(4);
    }
}

/* the keyboard closing: the cursor's key let go, its elements hidden, the shared pieces put away,
 * the menu's event 16 run (SOCOM 1's ShutdownBegin, SOCOM II's helpers out of line) */
void CSoftKeyboard::ShutdownBegin(CGameMenu *menu)
{
    void (C2DButton::*normal)() = &C2DButton::SetNormal;

    if (GetCurKeyProps())
        GetCurKeyProps()->SetButtonState(normal);
    func_0038B580(this, 0);
    func_00394420();
    func_00398B50();
    func_00399C10(this);
    func_00367810(menu, 16, 0);
    func_00395DE0(1);
}

/* the cursor's key let go, this keyboard's node hidden and the other's shown, the other made the
 * active one and its two element sets shown (SOCOM II's; the name ours) */
void CSoftKeyboard::SwitchTo(CSoftKeyboard *to)
{
    func_00395140(to->m_17c, to->m_180, &to->m_texLists, to);
    void (C2DButton::*normal)() = &C2DButton::SetNormal;

    if (GetCurKeyProps())
        GetCurKeyProps()->SetButtonState(normal);
    m_node->SetActive(false);
    to->m_node->SetActive(true);
    g_pActiveSkb = to;
    func_0038B580(to, 0);
    func_0038B580(to, 1);
}

/* the key's button put into a state, if it has one (SOCOM 1's, instruction for instruction; called
 * out of line everywhere: pinned) */
#pragma auto_inline off
void CSkbKeyProps::SetButtonState(void (C2DButton::*state)())
{
    if (m_button)
        (m_button->*state)();
}
#pragma auto_inline reset

/* the props of the key under the cursor, or none (SOCOM II's: FindKeyProps of GetCurKey, both
 * inlined; the name ours).
 * OPEN, 2 instructions short: the target materialises the row test (slt, xori, the size's sltu,
 * then andi 0xff) before branching on it, as SOCOM 1's GetSkbKey does; here it branches straight.
 * No change from a bool local, `!= false`, `== true`, a ternary, an int, signed or unsigned char
 * return, an early return, or splitting the column tests into an inline of their own (which
 * makes the tree a level too tall: end() then calls out of line) */
CSkbKeyProps *CSoftKeyboard::GetCurKeyProps()
{
    return FindKeyProps(GetCurKey());
}

/* a key's properties, or none (SOCOM 1's): the found branch first -- `i == end() ? 0 : ...` and
 * its if/return lay out a bne and a b, 8 bytes longer */
CSkbKeyProps *CSoftKeyboard::FindKeyProps(const CSkbKey &key)
{
    CSkbKeyMap::iterator i = m_keyProps.find(key);

    if (i != m_keyProps.end())
        return (*i).second;
    return 0;
}
