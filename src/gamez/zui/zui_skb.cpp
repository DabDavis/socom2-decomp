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
#include <string.h>
#include <algorithm>
#include "zui_2d.h"
#include "../zsys/zsys.h"
#include "../znode/znode.h"
#include "zui_skb.h"

/* (the units after this one, by address: an rdr array's count, 0 for anything else or none; the
 * keyboards' shared bitmaps prepared with a callback; three more of their invalidations) */
unsigned int SkbRdrCount(_zrdr *rdr);
void SkbForgetDisplay();
extern "C" {
void func_00398FC0();
void func_003958D0(zdb::CNode *node, zdb::CTextureRelocMgr *mgr, C2DFont *font, void (*callback)());
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
    SkbForgetDisplay();
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

    for (int i = 0; i < (int)SkbRdrCount(skbs); i++)
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
 * CSkbLook() (0x38E790, byte-exact since round 13 made m_sources CSkbBgPieces, a class over
 * vector<CSkbBgPiece>: its destructor 0x38D130 tests `this` three times) and the key vectors'
 * instances: with the pieces' element real, the tail generates in the target's order, and
 * insert<CSkbKey> 0x38F1F0 and the grid's clear 0x38FEF0 match, with the grid's resize 0x38EDC0 and
 * erase 0x38F640.
 * OPEN, the grid's insert 0x38F7D0 (0x5C0 against 0x5D8): the old block's rows destroyed in the
 * target call the deleter's clear and then, for a row's block, two out-of-line accessors that
 * return `this` (0x38FDB0, 0x38FDC0: the capacity pair's first()/alloc(), as SOCOM 1's ~vector<T *>
 * copies call compressed_pair_imp's second() and first()) -- a destructor chain cut at its leaves,
 * one inline level deeper than these headers (README "Level 6, or 8?"); the in-place copies also
 * swap s0/s1 (the class insert's end register, include/vector's insert comment) */
void CreateSoftKeyboard(_zrdr *rdr)
{
    CSoftKeyboard *skb = new CSoftKeyboard;

    g_arrSkbs.push_back(skb);
    skb->ParseLook(rdr);
    skb->ParseKeyProps(rdr);
    _zrdr *grid = zrdr_findtag(rdr, "Grid");
    for (int y = 0; y < (int)SkbRdrCount(grid); y++) {
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

/* (the next units' and this one's not yet written, by address: a keyboard's elements shown or
 * hidden) */
extern "C" {
void func_00394420();
void func_00398B50();
void func_00399C10(CSoftKeyboard *skb);
}

/* the game menu (zui.cpp's: only its event runner) */
class CGameMenu {
public:
    bool RunEventAnim(ActionItem::EVENT event, const char *arg);
};
void SkbPlaySound(unsigned char which);

extern "C" {

int func_00395110();

void func_0038C000(CSoftKeyboard *skb, CGameMenu *menu);
}
bool SkbTextUnchanged();
int SkbTypedTextNotBlank();
int SkbEnterKana(CSoftKeyboard *skb);
void SkbTakeText(CGameMenu *menu);

/* (the next unit's node lists' layout from a grid size, 0x395140; this unit's elements shown or
 * hidden, 0x38B580) */
extern "C" {
void func_00395140(int w, int h, CSkbTexLists *lists, CSoftKeyboard *skb);
void func_0038B580(CSoftKeyboard *skb, int which);
}

/* (this unit's out-of-line copies of SOCOM 1's CIntPair members and the look's start position,
 * after their first caller 0x38B440 -- SOCOM 1's zui_skb.cpp called its CIntPair's members out of
 * line too, while zui_skbview.cpp inlines them: the start position's address (0x38B570), the pair's
 * copy (0x38B550) and its halves (0x38B540, 0x38B530); and the cursor put at a grid position,
 * 0x38B8A0) */
struct SKBINTPAIR {
    int m_1st;
    int m_2nd;
};
extern "C" {
SKBINTPAIR *func_0038B570(CSoftKeyboard *skb);
SKBINTPAIR *func_0038B550(SKBINTPAIR *dst, const SKBINTPAIR *src);
int func_0038B540(const SKBINTPAIR *p);
int func_0038B530(const SKBINTPAIR *p);
void func_0038B8A0(CSoftKeyboard *skb, int x, int y);
}

/* (the units after this one, by address: the keyboard's captions and text shown with its fonts and
 * the text (0x398EA0), its key caps (0x395540); zui.cpp's menu fade, 0x36A210) */
extern "C" {
void func_00398EA0(CSoftKeyboard *skb, CSkbFonts *fonts, const char *text);
void func_00395540(CSoftKeyboard *skb, C2DFont *font);
void func_0036A210(float t);
}

/* the keyboard shown with a text, unless one is showing: its cursor at its start position, its
 * node lists laid out, its text, captions and caps made, its node on, the active one, its first
 * element set shown, the menu faded to half, its IgnoreUnchangedString taken (SOCOM 1's ShowSkb,
 * SOCOM II's with the text; 0x38B440, the first caller of the CIntPair copies before it; SkbOpen
 * calls it out of line: pinned) */
#pragma auto_inline off
void CSoftKeyboard::ShowSkb(const char *text)
{
    if (!g_pActiveSkb) {
        SKBINTPAIR pos;

        func_0038B550(&pos, func_0038B570(this));
        int x = func_0038B540(&pos) - 1;
        func_0038B8A0(this, x, func_0038B530(&pos) - 1);
        func_00395140(m_17c, m_180, &m_texLists, this);
        func_00398EA0(this, &m_fonts, text);
        func_00395540(this, &m_fonts.m_fonts[1]);
        m_node->SetActive(true);
        g_pActiveSkb = this;
        func_0038B580(this, 0);
        func_0036A210(0.5f);
        g_bIgnoreUnchanged = m_ignoreUnchangedString;
    }
}
#pragma auto_inline reset

/* a keyboard by its name, or none (ours) */
static inline CSoftKeyboard *FindSkb(const char *name)
{
    for (int i = 0; i < g_arrSkbs.size(); i++)
        if (!strcasecmp(g_arrSkbs[i]->m_name.c_str(), name))
            return g_arrSkbs[i];
    return 0;
}

/* the keyboard named by the look's i'th string switched to, its cursor at its start position
 * (0x38BE80; the name ours: the key handler 0x38D200 calls it with a key's code). The null name's
 * arm first: the target lays `to = 0` ahead of the search */
void CSoftKeyboard::SwitchToNamed(int i)
{
    const char *name = i < m_strings.size() ? m_strings[i].c_str() : 0;
    CSoftKeyboard *to;

    if (!name)
        to = 0;
    else
        to = FindSkb(name);

    if (to) {
        SKBINTPAIR pos;

        SwitchTo(to);
        func_0038B550(&pos, func_0038B570(to));
        int x = func_0038B540(&pos) - 1;
        func_0038B8A0(to, x, func_0038B530(&pos) - 1);
    }
}

/* a valve (zanim.h's CValve; the stand-in util_memcard.cpp and util_loc.cpp carry: the unit
 * reaches only its value and callbacks) */
enum VALVE_TYPE {};
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};
class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* CValve::SetValue, inlined (util_memcard.cpp's SetValveValue, taking a bool here: the target
 * re-extends the value at each use) */
static inline void SetValveValue(CValve *valve, bool value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the display unit's: the name of the keyboard to open (0x397640) and its root node */
extern "C" const char *func_00397640();
extern zdb::CNode *g_skbDisplayNode;

/* the pads (zinput.h's CInput, which this unit cannot include beside zui_spec.h: its pads only) and
 * whether a USB keyboard is plugged in (the world lane's KeyboardAttached) */
#ifndef ZINPUT_H
class CInput {
public:
    static CPad *m_pads[2];
};
#endif
bool KeyboardAttached();

extern "C" {
/* the display unit's: the display ticked, its count of key events (0x3949C0); the next event's key
 * (0x3978C0); whether the display is busy (0x396EC0); the keyboard's scaling stepped, 1 grown, 2
 * shrunk away (0x394DA0); the scaled-in hook (0x398BB0). This unit's key handler (0x38DFE0) */
int func_003949C0(CSoftKeyboard *skb);
int func_003978C0();
int func_00396EC0();
unsigned char func_00394DA0(CSoftKeyboard *skb, float dT);
void func_00398BB0();

}

extern "C" {
/* the cursor moved in a direction (0x38DFC0: through 0x38CB80, SkbMoveCursor's jump); the cursor's
 * key pressed (0x38D200); the two edit keys (0x399530, 0x399320) */
void func_0038DFC0(unsigned char dir, CPad *pad);
void func_0038D200(const CSkbKey &key, CSoftKeyboard *skb, CPad *pad, CGameMenu *menu);
void func_00399320(CSoftKeyboard *skb);
}
void SkbCursorBack(CSoftKeyboard *skb);

/* a key event of the showing keyboard: the four cursor moves (events 1-4 move up, down, left and
 * right: 3, 1, 2, 4 -- ours), the cursor's key (5), the two edit keys (6, 7) (0x38DFE0; the name
 * ours) */
void SkbHandleKey(int key, CPad *pad, CGameMenu *menu)
{
    switch ((unsigned char)key) {
    case 1:
        func_0038DFC0(3, pad);
        break;
    case 2:
        func_0038DFC0(1, pad);
        break;
    case 3:
        func_0038DFC0(2, pad);
        break;
    case 4:
        func_0038DFC0(4, pad);
        break;
    case 5:
        func_0038D200(g_pActiveSkb->GetCurKey(), g_pActiveSkb, pad, menu);
        break;
    case 6:
        SkbCursorBack(g_pActiveSkb);
        break;
    case 7:
        func_00399320(g_pActiveSkb);
        break;
    }
}

/* the showing keyboard ticked: the display's key events handled (when the menu takes input), the
 * USB toggles' bitmaps shown while a keyboard is plugged in, its scaling stepped -- grown, its first
 * element set shown; shrunk away, its node and the display's hidden, none showing (0x38D8F0; the
 * name ours: CGameMenu's tick 0x3654C0 calls it) */
void SkbTick(CGameMenu *menu, bool input, float dT)
{
    if (g_pActiveSkb) {
        int n = func_003949C0(g_pActiveSkb);

        if (input) {
            for (int i = 0; i < n; i++)
                SkbHandleKey(func_003978C0(), CInput::m_pads[0], menu);
        }
        CSoftKeyboard *skb = g_pActiveSkb;

        if (!func_00396EC0() && skb->m_480.m_node) {
            bool on = KeyboardAttached();

            if (skb->m_480.m_node)
                skb->m_480.m_node->SetActive(on);
            if (skb->m_530.m_node)
                skb->m_530.m_node->SetActive(on);
        }
        /* an if chain: a switch tests 2 first */
        unsigned char scaled = func_00394DA0(skb, dT);

        if (scaled == 1) {
            func_0038B580(skb, 1);
            func_00398BB0();
        } else if (scaled == 2) {
            skb->m_node->SetActive(false);
            g_skbDisplayNode->SetActive(false);
            g_pActiveSkb = 0;
        }
    }
}

namespace {

/* the text the keyboard opens with (0x49EC70, the unit's first static: only SkbOpen reads it, an
 * empty string; the name ours) */
char s_openText[0x40];

}

/* the UI command opening the keyboard the display unit names (KEYBOARD's when it names none) with
 * the empty text, the display's root on, skb_done set to whether none was found, the sound played
 * (0x38D770; the name ours: the command table's handler 0x2808D0 jumps here). The valve's value
 * a ternary: the target computes it at each of the setter's uses, after the value's load (`!skb`
 * is computed once, ahead of it) */
bool SkbOpen()
{
    const char *p = func_00397640();
    const char *name;

    if (*p)
        name = p;
    else
        name = "KEYBOARD";
    CSoftKeyboard *skb;

    if (!name)
        skb = 0;
    else
        skb = FindSkb(name);
    if (skb) {
        skb->ShowSkb(s_openText);
        g_skbDisplayNode->SetActive(true);
    }
    SetValveValue(CValve::Create("skb_done", (VALVE_TYPE)1), skb ? false : true);
    SkbPlaySound(0);
    return true;
}

extern "C" {
/* the cursor's two steps (0x38C4B0, 0x38C4C0: SOCOM 1's Local Decr and Incr) */
int func_0038C4B0(int i);
int func_0038C4C0(int i);
}

/* the cursor's bitmap put on a key (the display unit's) */
void SkbPlaceCursor(CSkbKeyProps *props);

/* the cursor moved along an axis by a step until it lands on another key that is set: the old key
 * let go, the cursor put on the new one, the move's sound (0x38C130; the name ours: SOCOM 1's
 * SyncHighlightToGridPos and CSkbGrid::MoveToNextPos, the latter inlined here) */
void SkbMoveAlong(CSoftKeyboard *skb, SKBAXIS axis, int (*step)(int))
{
    void (C2DButton::*normal)() = &C2DButton::SetNormal;

    skb->SetCurKeyState(normal);
    const CSkbKey &start = skb->GetCurKey();
    const CSkbKey *key;

    do {
        skb->m_grid.MoveToNextPos(&g_iSkbGridPosX, &g_iSkbGridPosY, axis, step);
        key = &skb->GetCurKey();
    } while (!(*key != start) || !key->IsSet());
    SkbPlaceCursor(skb->GetCurKeyProps());
    SkbPlaySound(2);
}

/* the keyboard's display forgotten: no node and no grid size, its texture lists freed, its six
 * bitmaps' nodes cleared, its fonts unloaded and every key's display forgotten (SOCOM 1's,
 * rewritten for SOCOM II's members; the constructor calls it) */
void CSoftKeyboard::InvalidateDisplay()
{
    m_node = 0;
    m_17c = 0;
    m_180 = 0;
    m_texLists.Free();
    m_1c0.Slot10(0);
    m_270.Slot10(0);
    m_480.Slot10(0);
    m_530.Slot10(0);
    m_320.Slot10(0);
    m_3d0.Slot10(0);
    m_fonts.Unload();
    for (CSkbKeyMap::iterator it = m_keyProps.begin(); it != m_keyProps.end(); ++it)
        (*it).second->InvalidateDisplay();
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

    SkbMoveAlong(skb, axis, dir == 2 || dir == 4 ? func_0038C4C0 : func_0038C4B0);
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
    if (!SkbEnterKana(this)) {
        bool unchanged = false;

        if (g_bIgnoreUnchanged && SkbTextUnchanged())
            unchanged = true;
        bool empty = false;

        if (IsNone(func_00395110()) && IsNone(SkbTypedTextNotBlank()))
            empty = true;
        if (unchanged || empty) {
            func_0038C000(this, menu);
            return;
        }
        SkbTakeText(menu);
        ShutdownBegin(menu);
        SkbPlaySound(4);
    }
}

/* the keyboard closing: the cursor's key let go, its elements hidden, the shared pieces put away,
 * the menu's event 16 run (SOCOM 1's ShutdownBegin, SOCOM II's helpers out of line) */
void CSoftKeyboard::ShutdownBegin(CGameMenu *menu)
{
    void (C2DButton::*normal)() = &C2DButton::SetNormal;

    SetCurKeyState(normal);
    func_0038B580(this, 0);
    func_00394420();
    func_00398B50();
    func_00399C10(this);
    menu->RunEventAnim(ActionItem::ON_SKB_CLOSE, 0);
    SkbPlaySound(1);
}

/* the cursor's key let go, this keyboard's node hidden and the other's shown, the other made the
 * active one and its two element sets shown (SOCOM II's; the name ours) */
void CSoftKeyboard::SwitchTo(CSoftKeyboard *to)
{
    func_00395140(to->m_17c, to->m_180, &to->m_texLists, to);
    void (C2DButton::*normal)() = &C2DButton::SetNormal;

    SetCurKeyState(normal);
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
 * inlined; the name ours). Round 14: the row test is materialised (slt, xori, the size's sltu, andi)
 * as the target's because IsValidPos tests IsValidRow `!= false` (zui_skb.h), and FindKeyProps'
 * end() is taken into a local (as an argument of the iterator's `!=` it was a level too deep here,
 * calling the tree's tail() out of line) */
inline CSkbKeyProps *CSoftKeyboard::GetCurKeyProps()
{
    return FindKeyProps(GetCurKey());
}

inline void CSoftKeyboard::SetCurKeyState(void (C2DButton::*state)())
{
    if (GetCurKeyProps())
        GetCurKeyProps()->SetButtonState(state);
}

/* the keys named in a keyboard's rdr beside the letters (0x3FACC0, 0x3FACE0 and 0x3FAC60; the names
 * ours): the five characters' names and the characters, and the fourteen action keys */
static const char *s_charKeyNames[5] = {"Quote1", "Quote2", "ParenL", "ParenR", "Space"};
static const char *s_charKeyChars = "'\"() ";
static const char *s_actionKeyNames[14] = {"CapsLock", "Recip", "BackSpace", "Clear", "Enter", "Exit", "Apply",
                                           "Shift", "Legend", "Dakuten", "Handakuten", "WideNarrow",
                                           "MsgPlayer", "Action2"};

extern "C" int strncasecmp(const char *a, const char *b, size_t n);
/* zui_hangul.cpp's jamo by name (its JAMO: JAMO_COUNT for none) */
enum JAMO { JAMO_COUNT = 33 };
JAMO FindJamo(const char *name);


namespace {

/* the key a name in the rdr gives (0x38DA70, SOCOM 1's StrToSkbKey; ParseKeyProps and
 * CreateSoftKeyboard call it): none for no name, a single character as itself, "Switch<n>" the
 * switch key n (row 3, SOCOM II's), a named character, an action key (row 1, by its index) or a
 * jamo (row 2, SOCOM II's), else none. The switch key is built in a frame slot and copied out (a
 * local returned through a reference: returned by name, MW scalarises it), the jamo's in the
 * result itself (a constructor: a named local is scalarised and stored at the end) */
CSkbKey StrToSkbKey(const char *name)
{
    if (!name)
        return CSkbKey();
    if (strlen(name) == 1)
        return CSkbKey(0, (unsigned char)name[0]);
    if (!strncasecmp(name, "Switch", 6)) {
        CSkbKey k;
        CSkbKey &r = k;

        k.Set(3, atoi(name + 6));
        return r;
    }
    for (unsigned int i = 0; i < 5; i++) {
        if (!strcasecmp(name, s_charKeyNames[i]))
            return CSkbKey(0, (unsigned char)s_charKeyChars[i]);
    }
    for (unsigned int i = 0; i < 14; i++) {
        if (!strcasecmp(name, s_actionKeyNames[i]))
            return CSkbKey(1, (unsigned char)i);
    }
    return CSkbKey((unsigned char)FindJamo(name));
}

}

/* a key and its props paired for the map, the key taken by value (ours: as SOCOM 1's make_pair,
 * whose key parameter is a copy -- the target block-copies it, then copies it into the pair and
 * that into the map's value) */
static inline std::pair<CSkbKey, CSkbKeyProps *> SkbKeyPair(CSkbKey key, CSkbKeyProps *props)
{
    return std::pair<CSkbKey, CSkbKeyProps *>(key, props);
}

/* an int the rdr may give, else the default (ours: as the display unit's) */
static inline int SkbFindInt(_zrdr *rdr, const char *tag, int def)
{
    int i = def;

    zrdr_findint(rdr, tag, &i, 1);
    return i;
}

/* a rectangle from a place and a size (ours: built in place, as the display unit's out-of-line
 * 0x396C80 builds it) */
static inline CSkbRect SkbRect(const CIntPair &pos, const CIntPair &size)
{
    CSkbRect r;

    r.Set(pos, size);
    return r;
}

/* the keys' props a keyboard's KeyProps list gives: each item's Key (a name, or a code), its Pos and
 * Size and its ButtonSpec (0x38DCB0; CreateSoftKeyboard calls it; the name ours).
 * OPEN, 0x220 against 0x1F0 (written round 15, untuned): the calls and their order are the target's
 * (SkbRdrCount each pass, Size read before Pos, the ButtonSpec tag before the inlined FindKeyProps);
 * the frame differs -- the target keeps the code path's key at 212, StrToSkbKey's at 216, the key at
 * 220, the pairs at 184/192 and the rectangle at 160, with sp+214, 222, 196 and 164 hoisted into
 * s4/s6/s7/s8, `this` in s2 and this+360 in s3; here the rectangle's own addresses are hoisted and
 * `this` is s5 (SkbRect's named result, and the ternary key, are the first suspects) */
void CSoftKeyboard::ParseKeyProps(_zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "KeyProps");

    for (int i = 0; i < (int)SkbRdrCount(list); i++) {
        _zrdr *item = list->Item(i);
        const char *name = zrdr_findstring(item, "Key");
        CSkbKey key = name ? StrToSkbKey(name) : CSkbKey(0, SkbFindInt(item, "Key", 0));

        AddKeyProps(key, SkbRect(ReadIntPair(item, "Pos"), ReadIntPair(item, "Size")));
        FindKeyProps(key)->SetSpec(zrdr_findtag(item, "ButtonSpec"));
    }
}

/* a key's props found, or made and mapped -- a key of row 2 sets 0x174 -- then given the key's
 * place and size (0x38B770; ParseKeyProps calls it; the name ours). Written round 15 for the map's
 * insert_one 0x38EC10 and insert_node_at 0x38EF50, byte-exact through it.
 * OPEN, 0xFC against 0x124: the target keeps the insert's three temporaries in memory -- the key
 * block-copied (lh/sh, lh/sh) to 104, a pair<CSkbKey, CSkbKeyProps *> at 80 copy-constructed from
 * it (lb/lhu) with the props stored and reloaded, the map's value at 88 copied from that -- where
 * MW scalarises every form tried: this by-value helper (SOCOM 1's make_pair takes its key by value:
 * its pair constructor 0x3A8E40 is out of line there), include/utility's make_pair, a named pair,
 * value_type(pair), a local key copy. Fields kept in memory point at MSL's pair holding them in a
 * base class (the deque iterator's tell, round 14 of the MSL headers) -- tried in the MSL headers'
 * round 16 (include/utility's comment): the base folds as well once the pair inlines; what does give
 * the key copy and the value word for word (0x10C) is std::make_pair by value with CSkbKey's copy
 * left implicit (a by-value argument is then copied bitwise, as the target's lh/sh), its m_row/m_code
 * views first in their unions and its assignment written over m_urow/m_scode (this unit stays at 98)
 * -- the pair temporary between them is still folded.
 * Round 16 (build/scratch/ui/v_akp1.py, v_akp2.py, over that 0x10C form): MW keeps a class object in
 * memory only as a BLOCK COPY's source or destination -- a pair passed BY VALUE to an inline insert
 * helper is built member-wise at one slot, copied bitwise (ld/sd) to the parameter's and read
 * member-wise from there (0x124, the target's size, 27 off: one bitwise copy too many) -- while the
 * target's pair at 80 is written and read member-wise only. Folded as well: the pair returned through
 * a reference local in the helper, bound to a const reference, a named value_type, the key by value
 * into a helper building the pair, a pair or value_type by value with a user copy constructor
 * (elided, then scalarised: 0xFC) */
void CSoftKeyboard::AddKeyProps(const CSkbKey &key, const CSkbRect &r)
{
    CSkbKeyProps *props = FindKeyProps(key);

    if (!props) {
        props = new CSkbKeyProps;
        m_keyProps.insert(SkbKeyPair(key, props));
        if (key.m_urow == 2)
            m_174 = true;
    }
    props->m_x = r.m_pos[0];
    props->m_y = r.m_pos[1];
    props->m_w = r.m_size[0];
    props->m_h = r.m_size[1];
}

/* a key's properties, or none (SOCOM 1's): the found branch first -- `i == end() ? 0 : ...` and
 * its if/return lay out a bne and a b, 8 bytes longer */
CSkbKeyProps *CSoftKeyboard::FindKeyProps(const CSkbKey &key)
{
    CSkbKeyMap::iterator i = m_keyProps.find(key);
    /* the end taken into a local: tested in the `!=` call's argument list it is checked a level
     * deeper, and inlined here into GetCurKeyProps it calls the tree's tail() out of line */
    CSkbKeyMap::iterator e = m_keyProps.end();

    if (i != e)
        return (*i).second;
    return 0;
}
