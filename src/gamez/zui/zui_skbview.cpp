/* zui_skbview.cpp -- the soft keyboard's shared display and text (OVL1 ~0x394420-~0x3990xx, its
 * initialiser 0x403800: four rows of ten CBmpWithNode at 0x49ED70 (0x6E0 bytes each, their
 * constructor/destructor 0x398AA0/0x398A40) and eight CBmpWithNode globals from 0x4A0A80): the text
 * being edited, the keyboards' look parser (0x3963F0) and their key props (0x38DA70's callees). SOCOM
 * 1 has none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * The unit calls zui_skb.cpp's CBmpWithNode constructor and destructor (0x38E620, 0x38E490): SOCOM
 * II's CBmpWithNode is a shared class, not SOCOM 1's file-local one (the linker kept zui_skb.cpp's
 * copies of its inline members for both units) -- writing this unit's row constructor needs the
 * keyboard's classes moved out of zui_skb.cpp's unnamed namespace into a header and their rows
 * renamed.
 *
 * OPEN, not written yet: everything but the text getter, ReadIntPair and the rectangle reader below. */

#include <string>
#include "zui.h"

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);

/* two ints read from an rdr pair (SOCOM 1's CIntPair: zui_skb.cpp's there, its Get1st and Get2nd) */
struct CIntPair {
    CIntPair(int a, int b) : m_1st(a), m_2nd(b) {}
    int Get1st() const { return m_1st; }
    int Get2nd() const { return m_2nd; }

    int m_1st;   // 0x0
    int m_2nd;   // 0x4
};

CIntPair ReadIntPair(_zrdr *rdr, const char *tag);

/* four words: a place and a size (zui_skb.cpp's CSkbRect, the look's rectangles) */
struct CSkbRect {
    /* (ours) */
    void Set(const CIntPair &pos, const CIntPair &size)
    {
        m_x = pos.Get1st();
        m_y = pos.Get2nd();
        m_w = size.Get1st();
        m_h = size.Get2nd();
    }

    int m_x, m_y, m_w, m_h;
};   // 0x10 bytes

/* an rdr array's item as an int, 0 for none (SOCOM 1's GetRdrArrayInt, zui_skb.cpp's there; inline
 * here) */
static inline int GetRdrArrayInt(_zrdr *rdr, int i)
{
    _zrdr *item = rdr ? rdr->Item(i) : 0;

    return item ? item->Integer() : 0;
}

/* the text typed so far (0x4A0A30; a mode byte follows at 0x4A0A60) and the text the keyboard was
 * opened with (0x4A1300) */
static char s_editText[0x30];
static std::string s_openText;

/* the text: the edited buffer, or the one the keyboard opened with (0x3998A0 and 0x399D20 ask) */
const char *SkbGetText(bool edited)
{
    return edited ? s_editText : s_openText.c_str();
}

/* a rectangle read from an rdr block's Pos and Size pairs (the look parser 0x3963F0 calls it) */
void ReadSkbRect(CSkbRect *r, _zrdr *rdr)
{
    r->Set(ReadIntPair(rdr, "Pos"), ReadIntPair(rdr, "Size"));
}

/* the pair of ints a tag names (SOCOM 1's ReadIntPair; the look parser and zui_skb.cpp's 0x38DCB0
 * call it out of line: pinned) */
#pragma auto_inline off
CIntPair ReadIntPair(_zrdr *rdr, const char *tag)
{
    _zrdr *t = zrdr_findtag(rdr, tag);
    int a = GetRdrArrayInt(t, 0);
    int b = GetRdrArrayInt(t, 1);

    return CIntPair(a, b);
}
#pragma auto_inline reset
