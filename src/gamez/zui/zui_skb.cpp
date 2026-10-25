/* zui_skb.cpp -- the soft keyboard: SOCOM 1's unit (its symbol table's @unnamed@zui_skb_cpp@ names
 * CSkbKey, CSkbKeyProps, CSkbGrid, CBmpWithNode and CSoftKeyboard), OVL1 0x38B180-0x39039F: the
 * keyboards' keys, their props map and the grid, then the template tail (the key map's tree, the
 * grid's vectors) closed by CBmpWithNode's this-adjusting destructor thunk at 0x390390, as SOCOM
 * 1's unit ends on @128@__dt__...CBmpWithNodeFv. SOCOM II moved the keyboard's rdr parsing, its
 * input and its drawing into the units after it (0x3903A0 on: the Korean input rules, the look's
 * parser 0x3963F0, CSkbKeyProps' destructor 0x394370). Its static initialiser is 0x403730 (the
 * keyboards' vector). Defined in reverse of their image order (-inline deferred).
 *
 * There is no DWARF for the unit: the layouts are read off SOCOM II's code -- CreateSoftKeyboard
 * (0x38E160: new of 0x710 bytes, the constructor inlined one level, its members' constructors
 * called) and the destructor (0x38CCA0) give the members. The statics are SOCOM 1's names (its
 * symbol table: g_iSkbGridPosX/Y, g_pActiveSkb, g_arrSkbs), at SOCOM II's addresses (data.txt). */

#include <map>
#include <algorithm>
#include "zui_2d.h"
#include "../zsys/zsys.h"

namespace zdb {
class CNode;
}

/* an rdr file (zrdr.h's: : _zrdr) and its reader */
class CRdrFile : public _zrdr {};
CRdrFile *zrdr_read(const char *name, const char *dir, int flags);
void zrdr_free(CRdrFile *file);

/* a file read for a block and freed with it (ours: ZuiInit keeps the file in a stack slot, read
 * back for the tag and the free, as only an object holding it explains) */
class CRdrFileHolder {
public:
    CRdrFileHolder(const char *name, const char *dir) : m_file(zrdr_read(name, dir, 0)) {}
    ~CRdrFileHolder() { zrdr_free(m_file); }
    operator CRdrFile *() const { return m_file; }

    CRdrFile *m_file;
};

/* a font (zui.cpp's C2DFont, 0x64 bytes in SOCOM II: its glyph map's constructor 0x1FEAC0, its
 * destructor 0x369EB0 -- a stand-in of what the keyboard's three need) */
class C2DFont {
public:
    unsigned char _0[0x64];
};

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

/* a member zeroed by its own constructor (SOCOM 1's CInit<T, v>: its constructor, copy, assignment
 * from T and conversion back are the demo's symbols) */
template <class T, int v>
class CInit {
public:
    CInit() : m_val(v) {}
    CInit &operator=(T x)
    {
        m_val = x;
        return *this;
    }
    operator T() const { return m_val; }

    T m_val;
};

/* a key: a row byte, then a 16-bit code. SOCOM II's tree compares the byte unsigned, then the
 * halfword at 2 (SOCOM 1's had two bytes), and copies the byte with lb: a signed member compared
 * through unsigned casts, the copy member-wise */
class CSkbKey {
public:
    CSkbKey() : m_row(0), m_code(0) {}
    CSkbKey(signed char row, unsigned short code) : m_row(row), m_code(code) {}
    CSkbKey(const CSkbKey &k) : m_row(k.m_row), m_code(k.m_code) {}
    bool operator<(const CSkbKey &k) const
    {
        if ((unsigned char)m_row != (unsigned char)k.m_row)
            return (unsigned char)m_row < (unsigned char)k.m_row;
        return m_code < k.m_code;
    }

    signed char m_row;   // 0x0
    unsigned short m_code;   // 0x2
};

class CSkbKeyProps;
class CSoftKeyboard;

typedef std::map<CSkbKey, CSkbKeyProps *> CSkbKeyMap;

/* the cursor's grid position (SOCOM 1's g_iSkbGridPosX/Y) and the keyboard showing (its
 * g_pActiveSkb) */
int g_iSkbGridPosX;
int g_iSkbGridPosY;
CSoftKeyboard *g_pActiveSkb;

/* the keys by grid position, a vector of rows (SOCOM 1's CSkbGrid: GetSkbKey at 0x3A3230 there,
 * inlined in SOCOM II). The column tests are an inline bool's tail: they branch the target's way
 * only so */
class CSkbGrid : public std::vector<std::vector<CSkbKey> > {
public:
    bool IsValidRow(int y) const { return y >= 0 && y < size(); }
    bool IsValidPos(int x, int y) const { return IsValidRow(y) && x >= 0 && x < (*this)[y].size(); }
    /* the key at a position, or a null key (one function-local static: SOCOM 1's GetSkbKey and
     * GetKeyAtGridPos share its guard) */
    const CSkbKey &GetSkbKey(int x, int y) const
    {
        static CSkbKey s_nullKey;

        if (IsValidPos(x, y))
            return (*this)[y][x];
        return s_nullKey;
    }
};

/* a bitmap with the node it draws on (SOCOM 1's CBmpWithNode: 0xB0 bytes, its vtable after
 * CPlainBmp's with the node set through C2D's slot 0x30) */
class CBmpWithNode : public CPlainBmp {
public:
    CBmpWithNode() : m_node(0) {}

    zdb::CNode *m_node;   // 0xa0
} __attribute__((aligned(16)));   // 0xb0 bytes

/* the keyboard's three fonts (the constructor's out-of-line copy is 0x38E550) */
struct CSkbFonts {
    C2DFont m_fonts[3];
};

/* four node lists (the constructor's out-of-line copy is 0x38E6F0) */
struct CSkbNodeLists {
    std::vector<zdb::CNode *> m_lists[4];
};

/* a key cap's look: a texture name, a scale and a colour (the names ours) */
struct CSkbCapLook {
    CSkbCapLook() : m_0(0), m_4(0), m_scale(1.0f), m_r(128), m_g(128), m_b(128) {}

    int m_0;   // 0x0
    int m_4;   // 0x4
    CSaferStr m_texture;   // 0x8
    float m_scale;   // 0x14
    int m_r;   // 0x18
    int m_g;   // 0x1c
    int m_b;   // 0x20
};   // 0x24 bytes

/* four words zeroed (a rectangle, the name ours) */
struct CSkbRect {
    CSkbRect() : m_x(0), m_y(0), m_w(0), m_h(0) {}

    int m_x, m_y, m_w, m_h;
};   // 0x10 bytes

/* what the rdr says about a keyboard's look (the look parser 0x3963F0's: Name, Background,
 * ShiftBackground, the sources ... TwoEditsGrowUp, StartGridPos; the names ours) */
struct CSkbLook {
    CSkbLook()
        : m_54(0), m_58(0), m_5c(0), m_60(0), m_64(0), m_68(0), m_6c(0), m_c0(0), m_c4(0), m_c8(0), m_cc(0),
          m_d0(0), m_d4(0), m_d8(0), m_dc(0)
    {
    }

    CSaferStr m_name;   // 0x0
    CSaferStr m_background;   // 0xc
    CSaferStr m_shiftBackground;   // 0x18
    std::vector<CSkbKey> m_sources[4];   // 0x24
    int m_54, m_58, m_5c, m_60, m_64, m_68, m_6c;   // 0x54
    CSkbRect m_rects[5];   // 0x70
    int m_c0, m_c4, m_c8, m_cc, m_d0, m_d4, m_d8, m_dc;   // 0xc0
    CSkbCapLook m_caps[3];   // 0xe0
    std::vector<CSaferStr> m_strings;   // 0x14c
};   // 0x158 bytes

/* the keyboards, as CreateSoftKeyboard makes them (SOCOM 1's g_arrSkbs) */
std::vector<CSoftKeyboard *> g_arrSkbs;

/* the keyboard (SOCOM II's 0x710 bytes: the key map at 0x164 where SOCOM 1's is at 0x2C, the grid
 * just before it) */
class CSoftKeyboard : public CSkbLook {
public:
    CSoftKeyboard() { InvalidateDisplay(); }
    ~CSoftKeyboard();
    void InvalidateDisplay();
    void PrepForDisplay(zdb::CTextureRelocMgr *mgr, C2DFont *font);
    CSkbKeyProps *FindKeyProps(const CSkbKey &key);
    /* the key under the cursor (SOCOM 1's GetCurKey: the grid positions are loaded first, as an
     * inline's arguments) */
    const CSkbKey &GetCurKey() const { return m_grid.GetSkbKey(g_iSkbGridPosX, g_iSkbGridPosY); }
    CSkbKeyProps *GetCurKeyProps();
    void ParseLook(_zrdr *rdr);
    void ParseKeyProps(_zrdr *rdr);

    CSkbGrid m_grid;   // 0x158
    CSkbKeyMap m_keyProps;   // 0x164
    CInit<bool, 0> m_174;   // 0x174
    CInit<bool, 0> m_ignoreUnchangedString;   // 0x175: IgnoreUnchangedString
    zdb::CNode *m_node;   // 0x178
    int m_17c;   // 0x17c
    int m_180;   // 0x180
    CSkbNodeLists m_nodeLists;   // 0x184
    CBmpWithNode m_1c0;   // 0x1c0
    CBmpWithNode m_270;   // 0x270
    CBmpWithNode m_320;   // 0x320
    CBmpWithNode m_3d0;   // 0x3d0
    CBmpWithNode m_480;   // 0x480
    CBmpWithNode m_530;   // 0x530
    CSkbFonts m_fonts;   // 0x5e0
};   // 0x710 bytes

CSkbKey StrToSkbKey(const char *s);
void CreateSoftKeyboard(_zrdr *rdr);

/* a keyboard deleted (as a function object, std::for_each's) */
struct DeleteSkb {
    DeleteSkb() {}
    void operator()(CSoftKeyboard *skb) { delete skb; }
};

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
 * CreateSoftKeyboard, SOCOM II's split three ways) */
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

}
