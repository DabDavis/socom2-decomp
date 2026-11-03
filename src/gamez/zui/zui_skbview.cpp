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
#include "../znode/znode.h"

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
/* the caption's three texts: plain, and the two modes' (0x4A08F0, 0x4A0930, 0x4A09B0) */
static char s_captionText0[0x40];
static char s_captionText1[0x80];
static char s_captionText2[0x80];
/* the name of the UI variable the typed text goes to (0x4A0A30) */
static char s_editText[0x30];
/* the caption's mode: 0 plain, 1 or 2 the modes 0x394840 and 0x394920 toggle (0x4A0A60) */
enum CAPTIONMODE { CAPTION_PLAIN, CAPTION_1, CAPTION_2 };
static CAPTIONMODE s_captionMode;
/* the caption's node (0x4A1118) */
static zdb::CNode *s_captionNode;
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

/* whether a bitmap's node is shown (ours) */
static inline bool SkbShown(const CBmpWithNode &b)
{
    return b.m_node && b.m_node->m_active;
}

/* L1 held or just pressed (ours) */
static inline bool SkbPadShift(const CPad *pad)
{
    return pad->m_state[PAD_TRIGGER_L1] == KEY_FALLING || pad->m_state[PAD_TRIGGER_L1] == KEY_DOWN;
}

/* whether the keys type shifted: L1 against the shift lock (bitmap 5); 0x399070 asks (0x396E60;
 * the names ours) */
bool SkbShifted(CPad *pad)
{
    bool shift = SkbPadShift(pad);

    return shift != SkbShown(s_bmp5);
}

/* the same, against the second lock (bitmap 0) as well (0x38D200 asks; 0x396CB0, ours) */
bool SkbShifted2(CPad *pad)
{
    return SkbShifted(pad) != SkbShown(s_bmp0);
}

/* a look rectangle set by index (the look parser's five, out of line; 0x396BD0, ours) */
void CSkbLook::SetRect(unsigned char i, const CSkbRect &r)
{
    m_rects[i] = r;
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

/* the caption's node placed at the keyboard's second cap's position, raised by dy (ours: the
 * 0.0f a caller passes is still added) */
static inline void PlaceCaption(CSoftKeyboard *skb, float dy)
{
    s_captionNode->SetPosition((float)skb->m_caps[1].m_0, dy + (float)skb->m_caps[1].m_4, 0.0f);
}

/* the caption's text set (a virtual call on the static: MW does not devirtualise it) */
static inline void SetCaption(const char *text)
{
    s_caption.SetString(text, true);
}

/* the caption's mode 2 toggled: its text, then its place (0x394920; the name ours) */
void SkbToggleCaption2(CSoftKeyboard *skb)
{
    bool on = s_captionMode != CAPTION_2;

    s_captionMode = on ? CAPTION_2 : CAPTION_PLAIN;
    SetCaption(on ? s_captionText2 : s_captionText0);
    PlaceCaption(skb, 0.0f);
}

/* the second lock (bitmap 0) toggled (0x3948F0; the name ours) */
void SkbToggleLock2()
{
    s_bmp0.m_node->SetActive(!s_bmp0.m_node->m_active);
}

/* how far mode 1 raises the caption: the look's float at 0x64, or nothing (ours) */
static inline float CaptionRaise(CSoftKeyboard *skb, bool on)
{
    return on ? *(float *)&skb->m_64 : 0.0f;
}

/* the caption's mode 1 toggled: its place (raised by the look's float at 0x64 when on), then its
 * text; whether it is on now (0x394840; the name ours) */
bool SkbToggleCaption1(CSoftKeyboard *skb)
{
    bool on = s_captionMode != CAPTION_1;

    s_captionMode = on ? CAPTION_1 : CAPTION_PLAIN;
    int x = skb->m_caps[1].m_0, y = skb->m_caps[1].m_4;
    float dy = CaptionRaise(skb, on);

    s_captionNode->SetPosition((float)x, dy + (float)y, 0.0f);
    SetCaption(on ? s_captionText1 : s_captionText0);
    return on;
}

/* the shift lock (bitmap 5) toggled, or hidden (0x3947D0; the name ours) */
void SkbToggleShiftLock(bool toggle)
{
    if (toggle)
        s_bmp5.m_node->SetActive(!s_bmp5.m_node->m_active);
    else if (s_bmp5.m_node)
        s_bmp5.m_node->SetActive(false);
}
