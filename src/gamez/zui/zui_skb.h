/* zui_skb.h -- the soft keyboard's classes, shared by its four units (the name ours). SOCOM 1 kept
 * them in zui_skb.cpp's unnamed namespace (its symbols' @unnamed@zui_skb_cpp@, its RTTI string
 * "@unnamed@zui_skb_cpp@::CBmpWithNode"); SOCOM II split the keyboard over four units that share
 * them, so they came from a header:
 *   - CBmpWithNode's RTTI string is plain "CBmpWithNode" (0x3FAEA8, the RTTI 0x3FAF00);
 *   - the display unit (zui_skbview.cpp) builds its ten-bitmap rows with zui_skb.cpp's copies of
 *     CBmpWithNode's constructor and destructor (0x38E620 and 0x38E490, taken by 0x398AA0 and
 *     0x398A40 for __construct_array, and by its initialiser 0x403800), the linker having kept the
 *     first unit's copies;
 *   - the Korean input unit (zui_hangul.cpp) defines ~CSkbKeyProps (0x394370), which zui_skb.cpp's
 *     ~CSoftKeyboard calls;
 *   - zui_skb.cpp calls the display unit's ReadIntPair (0x395300) and the Korean unit's
 *     SkbRdrCount (0x3910B0) out of line.
 * MW names an unnamed namespace after the COMPILED file, so a header's unnamed namespace would have
 * given each unit classes of its own. What only zui_skb.cpp uses stays in its unnamed namespace
 * there (the cursor's statics, the keyboards' vector, CreateSoftKeyboard).
 *
 * There is no DWARF for the keyboard: the layouts are read off SOCOM II's code -- CreateSoftKeyboard
 * (0x38E160: new of 0x710 bytes, the constructor inlined one level, its members' constructors
 * called) and the destructor (0x38CCA0) give the members. */
#ifndef ZUI_SKB_H
#define ZUI_SKB_H

#include <map>
#include <vector>
#include "zui_2d.h"
/* the fonts (the HUD lane's c2dfont.h: SOCOM II's 0x64-byte C2DFont), after zui_spec.h's CSaferStr */
#include "../ztwod/c2dfont.h"

namespace zdb {
class CNode;
class CTextureRelocMgr;
}
class CGameMenu;

/* the pad (zinput.h's CPad, whose enums zui_spec.h repeats, so the two cannot meet in one unit): the
 * keyboard units read only its button states, a KEY_STATE byte each from 0x1 (0x38D200's pad: L1
 * at 0xD, triangle 0x7, circle 0x8, R1 0xB) */
#ifndef ZINPUT_H
class CPad {
public:
    unsigned char m_0;   // 0x0
    unsigned char m_state[PAD_MAX_BUTTONS];   // 0x1
};
#endif

/* the iterator before i, i taken BY VALUE (ours): the keyboard units' last-element reads build the
 * end iterator, copy it into a slot of its own and fold the step into the access (0x390820's
 * `lhu -4`, SkbWrapText's and SkbWrapTokens' last pieces) -- the shape of a by-value iterator
 * parameter; include/vector's class back() (`*(end().m_p - 1)`) computes the address first */
template <class It>
inline It SkbPrev(It i)
{
    return i - 1;
}

/* an rdr file (zrdr.h's: : _zrdr) and its reader */
class CRdrFile : public _zrdr {};
CRdrFile *zrdr_read(const char *name, const char *dir, int flags);
void zrdr_free(CRdrFile *file);

/* a file read for a block and freed with it (ours: ZuiInit and SkbLoadUiParams keep the file in a
 * stack slot, read back for the tag and the free, as only an object holding it explains) */
class CRdrFileHolder {
public:
    CRdrFileHolder(const char *name, const char *dir) : m_file(zrdr_read(name, dir, 0)) {}
    ~CRdrFileHolder() { zrdr_free(m_file); }
    operator CRdrFile *() const { return m_file; }

    CRdrFile *m_file;
};

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
 * through unsigned casts, the copy member-wise. Its ASSIGNMENT reads the other way, the byte
 * with lbu and the code with lh (the key vector's erase 0x38F090 and insert 0x38F1F0, the grid's
 * store in CreateSoftKeyboard: every copy construction lb/lhu, every assignment lbu/lh), which no
 * plain members give -- MW's implicit operator= is member-wise in the members' own types, and a
 * cast of a byte or half that is only stored is folded away. Each member a union does it: the
 * implicit assignment copies a union as its FIRST member (the weapons lane's round 14), the copy
 * constructor reads the named views. A written operator= through casts gives the same loads, but
 * reads the source through its own reference (the grid store's addresses differ: the UI lane,
 * round 12; ours: the views' names) */
class CSkbKey {
public:
    CSkbKey() : m_row(0), m_code(0) {}
    CSkbKey(signed char row, unsigned short code) : m_row(row), m_code(code) {}
    CSkbKey(const CSkbKey &k) : m_row(k.m_row), m_code(k.m_code) {}
    /* a jamo's key (row 2), the null key for none -- jamo 33 (ours: StrToSkbKey's last case, built
     * in its result: the zeroing stores, then the test) */
    explicit CSkbKey(unsigned char jamo) : m_row(0), m_code(0)
    {
        if (jamo != 33) {
            m_row = 2;
            m_code = jamo;
        }
    }
    bool operator<(const CSkbKey &k) const
    {
        if ((unsigned char)m_row != (unsigned char)k.m_row)
            return (unsigned char)m_row < (unsigned char)k.m_row;
        return m_code < k.m_code;
    }
    /* (ours: the cursor mover 0x38C130 materialises the two `<` as one value, then branches) */
    bool operator!=(const CSkbKey &k) const { return *this < k || k < *this; }
    /* a key there, not the null key (ours: the bare conversions -- `m_row != 0` re-extends the row byte) */
    bool IsSet() const { return m_row || m_code; }
    /* (ours: StrToSkbKey sets its keys so, the arguments evaluated before either store) */
    void Set(signed char row, unsigned short code)
    {
        m_row = row;
        m_code = code;
    }

    union {
        unsigned char m_urow;
        signed char m_row;
    };   // 0x0
    union {
        short m_scode;
        unsigned short m_code;
    };   // 0x2
};

class CSoftKeyboard;

/* a key's props: its button among them (SOCOM 1's CSkbKeyProps; only what is read here). Its
 * destructor is the Korean input unit's (0x394370) */
class CSkbKeyProps {
public:
    /* (ours: AddKeyProps 0x38B770 news one, every word zeroed) */
    CSkbKeyProps() : m_x(0), m_y(0), m_w(0), m_h(0), m_spec(0), m_button(0), m_node(0) {}
    ~CSkbKeyProps();
    void SetButtonState(void (C2DButton::*state)());
    /* its button's spec replaced by one read from an rdr block, or none (0x394210; ours) */
    void SetSpec(_zrdr *rdr);
    /* its button made by the spec and put on a node of its own under the keyboard's, at the key's
     * place (0x394290; ours) */
    void PrepForDisplay(zdb::CTextureRelocMgr *mgr, C2DFont *font, zdb::CNode *parent);
    /* its node forgotten and its button deleted (0x394320; ours) */
    void InvalidateDisplay();

    int m_x;   // 0x0: the key's place and size (SkbPlaceCursor's; the names ours)
    int m_y;   // 0x4
    int m_w;   // 0x8
    int m_h;   // 0xc
    CButtonSpec *m_spec;   // 0x10
    C2DButton *m_button;   // 0x14
    zdb::CNode *m_node;   // 0x18
};

typedef std::map<CSkbKey, CSkbKeyProps *> CSkbKeyMap;

/* the keys by grid position, a vector of rows (SOCOM 1's CSkbGrid: GetSkbKey at 0x3A3230 there,
 * inlined in SOCOM II). The column tests are an inline bool's tail: they branch the target's way
 * only so */
/* the cursor's axis (ours: a byte enum, passed as `li 1; movz`; SOCOM 1's TwoDAxis) */
enum SKBAXIS { SKB_AXIS_ROW, SKB_AXIS_COLUMN };

class CSkbGrid : public std::vector<std::vector<CSkbKey> > {
public:
    /* the position stepped along an axis (x along a row, y down the rows), wrapping at either end;
     * nothing past the last row (SOCOM 1's, out of line there; inlined in SOCOM II's cursor mover) */
    void MoveToNextPos(int *x, int *y, SKBAXIS axis, int (*step)(int)) const
    {
        int *p = axis == SKB_AXIS_ROW ? x : y;

        *p = step(*p);
        int count;

        if (axis == SKB_AXIS_ROW) {
            if (*y >= size())
                return;
            count = (*this)[*y].size();
        } else
            count = size();
        if (*p < 0)
            *p = count - 1;
        else if (*p >= count)
            *p = 0;
    }
    bool IsValidRow(int y) const { return y >= 0 && y < size(); }
    /* the row test `!= false`: its && is then materialised (slt, xori, sltu, andi 0xff) before the
     * branch, as in every inlined copy in the target (GetCurKeyProps, SkbHandleKey) */
    bool IsValidPos(int x, int y) const { return (IsValidRow(y) != false) && x >= 0 && x < (*this)[y].size(); }
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
 * CPlainBmp's with the node set through C2D's slot 0x30, 0x38BAB0, a store to 0xA0 -- zui_2d.h's C2D
 * declares that slot with no parameter, so it is not overridden here). The implicit destructor's
 * copy (0x38E490) inlines CPlainBmp's (its tables, ~UIVarMixin's detach, ~C2DBitmap called): zui_2d.h
 * declares ~CPlainBmp inline, its out-of-line copy an earlier unit's (0x1FDBE0) */
class CBmpWithNode : public CPlainBmp {
public:
    CBmpWithNode() : m_node(0) {}

    zdb::CNode *m_node;   // 0xa0
} __attribute__((aligned(16)));   // 0xb0 bytes

/* the keyboard's three fonts: the implicit constructor's out-of-line copy is 0x38E550,
 * `__construct_array(this, C2DFont(), ~C2DFont(), 0x64, 3)` -- C2DFont's constructor zui_skb.cpp's
 * copy (0x38E590), its destructor zui.cpp's (0x369EB0, the linker keeping the first unit's) */
struct CSkbFonts {
    /* the three unloaded (ours: InvalidateDisplay's loop walks the fonts themselves) */
    void Unload()
    {
        for (int i = 0; i < 3; i++)
            m_fonts[i].Unload();
    }

    C2DFont m_fonts[3];
};

/* four texture-handle lists (the constructor's out-of-line copy is 0x38E6F0: __construct_array over
 * vector<zdb::CTexHandle *>'s constructor and destructor copies 0x1FB3D0/0x1FB300, so the elements
 * are texture handles, not nodes; the name ours) */
struct CSkbTexLists {
    /* the four freed (ours: InvalidateDisplay's loop walks the lists themselves, so a member of
     * theirs; FreeVector written out -- called, it is a level too deep and stays a call) */
    void Free()
    {
        for (int i = 0; i < 4; i++) {
            std::vector<zdb::CTexHandle *> tmp;

            m_lists[i].clear();
            m_lists[i].swap(tmp);
        }
    }

    std::vector<zdb::CTexHandle *> m_lists[4];
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

/* two ints read from an rdr pair (SOCOM 1's CIntPair: zui_skb.cpp's there, its Get1st and Get2nd) */
struct CIntPair {
    CIntPair(int a, int b) : m_1st(a), m_2nd(b) {}
    int Get1st() const { return m_1st; }
    int Get2nd() const { return m_2nd; }

    int m_1st;   // 0x0
    int m_2nd;   // 0x4
};

/* an rdr array walked by index: the array a tag names, and the index (the look parser's; the Korean
 * input unit's constructor and More, 0x3909F0 and 0x3909C0, the look parser calls out of line; ours) */
struct CSkbRdrIter {
    CSkbRdrIter(_zrdr *rdr, const char *tag);
    /* the index is still inside the array */
    bool More() const;
    /* the item at the index, or none (ours) */
    _zrdr *Item() const { return m_rdr->Item(m_index); }
    void Next() { m_index++; }

    _zrdr *m_rdr;   // 0x0
    int m_index;   // 0x4
};

/* the pair of ints a tag names (SOCOM 1's ReadIntPair; the display unit's, 0x395300) */
CIntPair ReadIntPair(_zrdr *rdr, const char *tag);

/* the display unit's locks and shift (zui_skbview.cpp; the names ours): whether the keys type
 * shifted (L1 against the shift lock, 0x396E60; and against the second lock, 0x396CB0), the
 * second lock toggled (0x3948F0), the shift lock toggled or hidden (0x3947D0) */
bool SkbShifted(CPad *pad);
bool SkbShifted2(CPad *pad);
void SkbToggleLock2();
void SkbToggleShiftLock(bool toggle);

/* four words: a place and a size (a rectangle, the names ours; the look's rectangles) */
struct CSkbRect {
    CSkbRect()
    {
        m_pos[0] = 0;
        m_pos[1] = 0;
        m_size[0] = 0;
        m_size[1] = 0;
    }
    /* a place and a size (the display unit's out-of-line copy, 0x396C80: the look parser's
     * background pieces; ours) */
    CSkbRect(const CIntPair &pos, const CIntPair &size);
    /* a copy, word by word: CSkbBgPiece's implicit copy (0x398910) moves the rectangles with lw/sw,
     * where the implicit assignment (SetRect, the look parser's pieces) block-copies each int pair
     * through the FPRs -- a user-declared copy constructor beside the implicit operator= */
    CSkbRect(const CSkbRect &r)
    {
        m_pos[0] = r.m_pos[0];
        m_pos[1] = r.m_pos[1];
        m_size[0] = r.m_size[0];
        m_size[1] = r.m_size[1];
    }
    /* (ours) */
    void Set(const CIntPair &pos, const CIntPair &size)
    {
        m_pos[0] = pos.Get1st();
        m_pos[1] = pos.Get2nd();
        m_size[0] = size.Get1st();
        m_size[1] = size.Get2nd();
    }

    /* two int pairs: SetRect's block copy moves each through the FPRs as a unit of its own (two
     * loads, two stores, twice), which named members (lw/sw) or one int[4] (four loads first) do
     * not give */
    int m_pos[2];   // 0x0
    int m_size[2];   // 0x8
};   // 0x10 bytes

/* a piece of a keyboard's background: a texture, the part of it taken and where it is drawn (the
 * look parser's BackgroundPieces, ShiftBgPieces, TwoEditsBgPieces and TwoEditsShiftBgPieces, each
 * piece a Source {Texture, Pos, Size} and a Display {Pos, Size}; the names ours). Its vector's
 * destructor chain is zui_skb.cpp's (CSkbLook's __construct_array, 0x38D130, 0x38EE70, 0x38FDD0),
 * its resize, erase, insert and copy the display unit's (0x397FE0-0x3989DF) */
struct CSkbBgPiece {
    CSaferStr m_texture;   // 0x0
    CSkbRect m_source;   // 0xc
    CSkbRect m_display;   // 0x1c
};   // 0x2c bytes

/* a list of them (ours): a class of its own over the vector -- its implicit destructor's copy
 * (0x38D130, CSkbLook's __construct_array takes it) tests `this` three times before it calls the
 * deleter's destructor, one level more than a plain vector's */
struct CSkbBgPieces : public std::vector<CSkbBgPiece> {};

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
    CSkbBgPieces m_sources[4];   // 0x24: the four background piece lists
    int m_54, m_58, m_5c, m_60, m_64, m_68, m_6c;   // 0x54
    CSkbRect m_rects[5];   // 0x70
    int m_c0;   // 0xc0
    float m_c4;   // 0xc4: UserStringLineHeight (a float member: SkbCursorBack 0x399530 reads it plainly)
    int m_c8, m_cc, m_d0, m_d4, m_d8, m_dc;   // 0xc8
    CSkbCapLook m_caps[3];   // 0xe0
    std::vector<CSaferStr> m_strings;   // 0x14c

    /* a rectangle set by index (the look parser's five, out of line: zui_skbview.cpp's 0x396BD0) */
    void SetRect(unsigned char i, const CSkbRect &r);
};   // 0x158 bytes

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
     * inline's arguments; defined in zui_skb.cpp, which holds them) */
    inline const CSkbKey &GetCurKey() const;
    /* the cursor's key's props: inline and too tall for its callers (they inline its body and call
     * GetCurKey and FindKeyProps out of line, 0x38C130), its own copy inlining both (0x38B2A0) */
    inline CSkbKeyProps *GetCurKeyProps();
    /* the cursor's key's button put into a state (ours: inlined one level, so its callers call
     * GetCurKeyProps out of line -- ShutdownBegin, SwitchTo, 0x38C130) */
    inline void SetCurKeyState(void (C2DButton::*state)());
    void ParseLook(_zrdr *rdr);
    void ParseKeyProps(_zrdr *rdr);
    /* a key's props found, or made and mapped, then placed (0x38B770; ParseKeyProps', the name
     * ours) */
    void AddKeyProps(const CSkbKey &key, const CSkbRect &r);
    void SwitchTo(CSoftKeyboard *to);
    void SwitchToNamed(int i);
    void ShowSkb(const char *text);
    void ShutdownBegin(CGameMenu *menu);
    void OnEnterKey(CGameMenu *menu);

    CSkbGrid m_grid;   // 0x158
    CSkbKeyMap m_keyProps;   // 0x164
    CInit<bool, 0> m_174;   // 0x174
    CInit<bool, 0> m_ignoreUnchangedString;   // 0x175: IgnoreUnchangedString
    zdb::CNode *m_node;   // 0x178
    int m_17c;   // 0x17c
    int m_180;   // 0x180
    CSkbTexLists m_texLists;   // 0x184
    CBmpWithNode m_1c0;   // 0x1c0
    CBmpWithNode m_270;   // 0x270
    CBmpWithNode m_320;   // 0x320
    CBmpWithNode m_3d0;   // 0x3d0
    CBmpWithNode m_480;   // 0x480
    CBmpWithNode m_530;   // 0x530
    CSkbFonts m_fonts;   // 0x5e0
};   // 0x710 bytes

#endif
