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
#include "../zsnd/zsnd.h"

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
/* the display's root node (0x49ED50: zui_skb.cpp's 0x38D770 and 0x38D8F0 read it; the name ours) */
zdb::CNode *g_skbDisplayNode;
/* the display's six sounds, by SkbPlaySound's case (0x4A1110, 0x4A1108, 0x4A10F8, 0x4A10F0,
 * 0x4A10E8, 0x4A1100; the names ours) */
static CSnd *s_sound0;
static CSnd *s_sound1;
static CSnd *s_sound2;
static CSnd *s_sound3;
static CSnd *s_sound4;
static CSnd *s_sound5;
/* the look's limits on strings 0 and 1: bytes and characters (0x4A10C8-0x4A10E0; ours) */
static int s_maxBytes0;
static int s_maxChars0;
static int s_maxBytes1;
static int s_maxChars1;
/* the font a cap naming none is drawn in (0x4A1120: 0x3958D0 sets it; the name ours) */
static C2DFont *s_defaultFont;
/* bitmap 4's timer (0x3E04C8: -1 until SkbToggleBmp4 clears it; the name ours) */
static float s_bmp4Timer = -1.0f;

/* a 2D object given a node of its own under a parent (zui.cpp's, 0x36AB20) */
extern "C" zdb::CNode *func_0036AB20(C2D *obj, zdb::CTextureRelocMgr *mgr, zdb::CNode *parent);
int UTF8CharLen(const char *&s);

/* the caption's text set (a virtual call on the static: MW does not devirtualise it) */
static inline void SetCaption(const char *text)
{
    s_caption.SetString(text, true);
}

/* the display's sounds found by the names the look gave (0x397BC0, from 0x3958D0; the name ours) */
void SkbFindSounds()
{
    /* three names taken first: the target holds them in saved registers across the first call */
    const char *n3 = s_str3.c_str(), *n4 = s_str4.c_str(), *n5 = s_str5.c_str();

    s_sound3 = CSnd::GetSoundByName(s_str2.c_str());
    s_sound2 = CSnd::GetSoundByName(n3);
    s_sound5 = CSnd::GetSoundByName(n4);
    s_sound1 = CSnd::GetSoundByName(n5);
    s_sound0 = CSnd::GetSoundByName(s_str6.c_str());
    s_sound4 = CSnd::GetSoundByName(s_str1.c_str());
}

/* the text: the edited buffer, or the one the keyboard opened with (0x3998A0 and 0x399D20 ask) */
const char *SkbGetText(bool edited)
{
    return edited ? s_editText : s_openText.c_str();
}

/* the key's button spec replaced (0x394210; ours) */
void CSkbKeyProps::SetSpec(_zrdr *rdr)
{
    delete m_spec;
    m_spec = rdr ? new CButtonSpec(rdr) : 0;
}

/* the cursor's bitmap (2) sized to a key and put on it, the key's button made active (0x397590;
 * the name ours) */
void SkbPlaceCursor(CSkbKeyProps *props)
{
    if (props) {
        s_bmp2.Load(0.0f, 0.0f, (float)props->m_w, (float)props->m_h, s_bmp2.m_pTexHandle);
        s_bmp2.m_node->SetPosition((float)props->m_x, (float)props->m_y, 0.0f);
        void (C2DButton::*active)() = &C2DButton::SetActive;

        if (props->m_button)
            (props->m_button->*active)();
    }
}

/* every node the display holds forgotten: the bitmaps', the key rows' (through the bitmaps'
 * SetNode), the sounds, the display's root node, the caption's node and text (0x396F30, from
 * InvalidateSkbsDisplay; the name ours).
 * OPEN, 6 instructions short (0x11C of 0x134): the target enters the inner loop at its bottom test
 * through two jumps -- the counter zeroed in the outer body's first block, the bitmap pointer set in
 * a block MW put after the return path -- with two delay slots left empty, where every form here
 * rotates it into a do-while (the trip count known). Tried: a reference local for the row, a while
 * loop, the counter declared outside, `&m_bmps[0] + j`, the row's loop as an inline member, an
 * inline taking the count as a parameter: all 0x11C */
void SkbForgetDisplay()
{
    s_bmp1.m_node = 0;
    s_bmp2.m_node = 0;
    s_bmp3.m_node = 0;
    s_bmp6.m_node = 0;
    s_bmp7.m_node = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++)
            s_keyRows.m_rows[i].m_bmps[j].Slot10(0);
    s_bmp0.m_node = 0;
    s_bmp5.m_node = 0;
    s_bmp4.m_node = 0;
    s_sound4 = 0;
    s_sound3 = 0;
    s_sound2 = 0;
    s_sound5 = 0;
    s_sound1 = 0;
    s_sound0 = 0;
    g_skbDisplayNode = 0;
    s_captionNode = 0;
    SetCaption(0);
}

/* the string has a character at p (zui_skbkeys.cpp's, ours) */
static inline bool HasChar(const char *p)
{
    return p && *p;
}

/* a character's length (a copy of the pointer handed on, zui_skbkeys.cpp's) */
static inline int CharLen(const char *s)
{
    return UTF8CharLen(s);
}

/* whether a text keeps within the limits the look sets for string 0 or 1: its bytes, then its
 * characters (0x396D40, 0x399EC0's test; the name ours).
 * OPEN, 13 instructions (same blocks, n's ++ in UTF8CharLen's delay slot as the target's): the
 * saved registers are numbered otherwise -- the target keeps p in s0, the limit in s1 and the count
 * in s2 (here the limit s0, the count s1, p s2) -- and its `return true` is a bool assigned
 * (daddiu), as an inlined function's is. Tried (build/scratch/ui/tf.py): a bool result variable,
 * the count declared first, the count before p, the count as an inline returning it, the whole
 * character test as an inline bool -- 8 to 31 off */
bool SkbTextFits(const std::string &s, bool second)
{
    int maxBytes = second ? s_maxBytes1 : s_maxBytes0;

    if (maxBytes > 0 && maxBytes < s.size())
        return false;
    int maxChars = second ? s_maxChars1 : s_maxChars0;

    if (maxChars > 0) {
        const char *p = s.c_str();
        int n = 0;

        while (HasChar(p)) {
            n++;
            p += CharLen(p);
        }
        return n <= maxChars;
    }
    return true;
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

/* a string given cap i's look -- the default font when the cap names none, its colour and scale --
 * and its node put at the cap's place raised by dy, hidden (0x395440: 0x398EA0's three; ours) */
void SkbApplyCapLook(C2DString *str, zdb::CNode *node, CSoftKeyboard *skb, unsigned char i, C2DFont *font,
                     float dy)
{
    /* the array into a local first: the index is added before the keyboard (`&skb->m_caps[i]`
     * adds the keyboard first) */
    CSkbCapLook *caps = skb->m_caps;
    CSkbCapLook *cap = &caps[i];
    /* the name through a reference: its address kept apart for the size test */
    const CSaferStr &name = cap->m_texture;

    if (!name.size())
        font = s_defaultFont;
    str->SetFont(font);
    str->SetColor((float)cap->m_r, (float)cap->m_g, (float)cap->m_b);
    str->m_scale = cap->m_scale;
    node->SetPosition((float)cap->m_0, dy + (float)cap->m_4, 0.0f);
    node->SetActive(false);
}

/* a rectangle read from an rdr block's Pos and Size pairs (the look parser 0x3963F0 calls it) */
void ReadSkbRect(CSkbRect *r, _zrdr *rdr)
{
    r->Set(ReadIntPair(rdr, "Pos"), ReadIntPair(rdr, "Size"));
}

/* a look rectangle set by index (the look parser's five, out of line; 0x396BD0, ours) */
void CSkbLook::SetRect(unsigned char i, const CSkbRect &r)
{
    m_rects[i] = r;
}

/* a sound played once, if it was found (ours) */
static inline void PlayOnce(CSnd *snd)
{
    if (snd)
        snd->PlaySound(true, 0, 255);
}

/* one of the display's six sounds (0x395DE0; the name ours) */
void SkbPlaySound(unsigned char which)
{
    switch (which) {
    case 0:
        PlayOnce(s_sound0);
        break;
    case 1:
        PlayOnce(s_sound1);
        break;
    case 2:
        PlayOnce(s_sound2);
        break;
    case 3:
        PlayOnce(s_sound3);
        break;
    case 4:
        PlayOnce(s_sound4);
        break;
    case 5:
        PlayOnce(s_sound5);
        break;
    }
}

/* bitmap 4's node toggled, bitmap 3's shown when it comes on, its timer cleared (0x396300; the name
 * ours) */
void SkbToggleBmp4()
{
    bool on = !SkbShown(s_bmp4);

    if (s_bmp4.m_node)
        s_bmp4.m_node->SetActive(on);
    if (SkbShown(s_bmp4) && s_bmp3.m_node)
        s_bmp3.m_node->SetActive(true);
    s_bmp4Timer = 0.0f;
    SkbPlaySound(3);
}

/* a bitmap's node toggled (ours) */
static inline void SkbToggle(CBmpWithNode *b)
{
    b->m_node->SetActive(!b->m_node->m_active);
}

/* whether a bitmap has its node (ours) */
static inline bool SkbHasNode(const CBmpWithNode *b)
{
    return b->m_node != 0;
}

/* two bitmaps' nodes toggled together, the callback told whether the second is on now (0x396220:
 * 0x38D200 and 0x38C630 pass the keyboard's 0x1C0 and 0x270; the name ours) */
void SkbTogglePair(CBmpWithNode *a, CBmpWithNode *b, void (*onToggle)(bool))
{
    /* through a bool inline: tested in a temporary, the node is read again for the toggle (a plain
     * `a->m_node` test keeps it in a0) */
    if (!SkbHasNode(a) || !SkbHasNode(b))
        return;
    SkbToggle(a);
    SkbToggle(b);
    onToggle(SkbShown(*b));
    SkbPlaySound(3);
}

/* a bitmap loaded with a texture by name at a size, and hung under a node (0x395D50: PrepForDisplay
 * and 0x38B8F0 call it; the name ours) */
void SkbLoadBitmap(CBmpWithNode *bmp, zdb::CNode *parent, int w, int h, const char *tex,
                   zdb::CTextureRelocMgr *mgr)
{
    bmp->Load(0.0f, 0.0f, (float)w, (float)h, PrepTexture(tex, mgr));
    func_0036AB20(bmp, mgr, parent);
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
