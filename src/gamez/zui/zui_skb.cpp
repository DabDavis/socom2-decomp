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
 * OPEN, 0x10 bytes long: the target calls the key map's default constructor out of line with no
 * arguments (0x38E740, which builds the tree from two unconstructed stack slots) where include/map
 * has only map(const Compare & = Compare(), const Allocator & = Allocator()), so the call site
 * builds the allocator (its copy called) and passes both; and it calls the fonts' constructor
 * 0x38E550 (C2DFont's, 0x38E590: a glyph map through 0x1FEAC0, a pointer vector, a string), which
 * the stand-in C2DFont has not got. The grid loop's registers follow once both are in (the target
 * keeps the grid in s8). It stays for the member constructors it emits (0x38E620, 0x38E730,
 * 0x38E770, 0x38E890, 0x38E8D0 byte-exact) and the grid's vector instances */
void CreateSoftKeyboard(_zrdr *rdr)
{
    CSoftKeyboard *skb = new CSoftKeyboard;

    g_arrSkbs.push_back(skb);
    skb->ParseLook(rdr);
    skb->ParseKeyProps(rdr);
    _zrdr *grid = zrdr_findtag(rdr, "Grid");
    for (int y = 0; y < func_003910B0(grid); y++) {
        _zrdr *row = grid->Item(y);
        for (unsigned int x = 0; x < row->Count(); x++) {
            _zrdr *item = row->Item(x);
            CSkbKey key = item->Type() == 3 ? StrToSkbKey(item->String()) : CSkbKey(0, item->Integer());

            if (skb->m_grid.size() <= y)
                skb->m_grid.resize(y + 1);
            if (skb->m_grid[y].size() <= x)
                skb->m_grid[y].resize(x + 1);
            skb->m_grid[y][x] = key;
        }
    }
    skb->m_ignoreUnchangedString = zrdr_findtag(rdr, "IgnoreUnchangedString") != 0;
}

}

/* (the next units' and this one's not yet written, by address: the node lists' layout from a grid
 * size, a keyboard's elements shown or hidden) */
extern "C" {
void func_00395140(int w, int h, CSkbNodeLists *lists, CSoftKeyboard *skb);
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
    func_00395140(to->m_17c, to->m_180, &to->m_nodeLists, to);
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
