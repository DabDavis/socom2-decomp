/* zui_skbview.cpp -- the soft keyboard's shared display and text (OVL1 ~0x394420-~0x3990xx, its
 * initialiser 0x403800: four rows of ten CBmpWithNode at 0x49ED70 (0x6E0 bytes each, their
 * constructor/destructor 0x398AA0/0x398A40) and eight CBmpWithNode globals from 0x4A0A80): the text
 * being edited, the keyboards' look parser (0x3963F0) and their key props (0x38DA70's callees). SOCOM
 * 1 has none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * The unit calls zui_skb.cpp's CBmpWithNode constructor and destructor (0x38E620, 0x38E490): SOCOM
 * II's CBmpWithNode is a shared class, not SOCOM 1's file-local one (the linker kept zui_skb.cpp's
 * copies of its inline members for both units) -- the keyboard's classes are zui_skb.h's.
 *
 * OPEN, not written yet: everything but the text getter, ReadIntPair and the rectangle reader below. */

#include <string>
#include "zui.h"
#include "zui_skb.h"

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);

/* an rdr array's item as an int, 0 for none (SOCOM 1's GetRdrArrayInt, zui_skb.cpp's there; inline
 * here) */
static inline int GetRdrArrayInt(_zrdr *rdr, int i)
{
    _zrdr *item = rdr ? rdr->Item(i) : 0;

    return item ? item->Integer() : 0;
}

/* ten bitmaps with their nodes: a row of the keyboards' shared keys (the name ours: its implicit
 * constructor and destructor, 0x398AA0 and 0x398A40, build and destroy the ten through zui_skb.cpp's
 * CBmpWithNode copies) */
struct CSkbBmpRow {
    CBmpWithNode m_bmps[10];
};   // 0x6e0 bytes

/* the four rows (the name ours: the initialiser inlines the implicit constructor, __construct_array
 * over the rows, and registers the implicit destructor 0x3989E0) */
struct CSkbBmpRows {
    CSkbBmpRow m_rows[4];
};   // 0x1b80 bytes

/* the keyboards' shared display (0x49ED70 on; the names ours) */
static CSkbBmpRows s_keyRows;
/* the text typed so far (0x4A0A30; a mode byte follows at 0x4A0A60) */
static char s_editText[0x30];
static CBmpWithNode s_bmp0;
static CBmpWithNode s_bmp1;
static CBmpWithNode s_bmp2;
static CBmpWithNode s_bmp3;
static CBmpWithNode s_bmp4;
static CBmpWithNode s_bmp5;
static CBmpWithNode s_bmp6;
static CBmpWithNode s_bmp7;
static CSkbRect s_rect;
static CRenderableString s_caption;
static CSaferStr s_str0;
static CSaferStr s_str1;
static CSaferStr s_str2;
static CSaferStr s_str3;
static CSaferStr s_str4;
static CSaferStr s_str5;
static CSaferStr s_str6;
/* the text the keyboard was opened with (0x4A1300) */
static CSaferStr s_openText;
static CSaferStr s_str8;

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
