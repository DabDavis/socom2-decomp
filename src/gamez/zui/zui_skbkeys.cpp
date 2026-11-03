/* zui_skbkeys.cpp -- the soft keyboard's text and USB-keyboard keys (OVL1 ~0x398AE0-0x39BBBF, its
 * initialiser 0x404110 building the three CRenderableString statics that 0x398B50-0x39A790 use):
 * the text being typed (two edited strings and a third), its taking and testing for OnEnterKey,
 * the jamo typed, and the handlers zui_skb.cpp's USB loop (0x38C630) calls for a key by its HID
 * usage -- the letters, the digit row, the punctuation, the dash -- each typing its character,
 * shifted or not, through 0x399040. SOCOM 1 has none of it: the file's name and every name here
 * are ours. Defined in reverse of their image order (-inline deferred).
 *
 * The unit inlines none of its own small functions: 0x398C00 (two calls) is called out of line by
 * 0x398AE0, 0x3999D0 and 0x399C20, 0x399040 by the USB handlers -- a unit without auto-inlining, or
 * pins on each (con_ps2.cpp and ai_util.cpp are the same, README "The inlining mode, measured").
 *
 * OPEN, not written yet: the rest of the unit (0x39AE50-0x39BBB0 holds the string assign through
 * the template replace, 0x39B620, the MSL lane's request). */

#include "zui_skb.h"
#include "../znode/znode.h"

/* the text being typed: which of the two edited strings is current (0x4A1330: string 0 when set),
 * the node shown while it is (0x4A1358), and the three strings on screen (0x4A1370, 0x4A1450,
 * 0x4A1520: the initialiser 0x404110 builds them). The names ours */
static bool s_textSel;
static zdb::CNode *s_textNode;
static CRenderableString s_textString0;
static CRenderableString s_textString1;
static CRenderableString s_textString2;

/* the string being edited (ours: an inline, the address computed at each use) */
static inline C2DString *EditString()
{
    return s_textSel ? &s_textString0 : &s_textString1;
}

/* a string's text from its first shown character (ours) */
static inline const char *ShownText(const C2DString *s)
{
    return s->m_string + s->m_firstCharPos;
}

/* the text typed: string 1's, then string 2's (ours: built in the caller's temporary) */
static inline CSaferStr TypedText()
{
    CSaferStr text(ShownText(&s_textString1));

    text += ShownText(&s_textString2);
    return text;
}

/* the keyboard's text variables' names (zui_skbview.cpp's): the edited one's, or the opened one's */
const char *SkbGetText(bool edited);

/* a USB key as the loop hands it on: its HID usage and whether shift is held (the names ours) */
struct SKBUSBKEY {
    unsigned char m_usage;   // 0x0
    unsigned char _1[3];
    int m_shift;   // 0x4
    unsigned char _8[0x14 - 0x8];
    int m_capsLock;   // 0x14
};

/* a character typed into the keyboard's text (0x399040: the text's insertion 0x399EC0, then the
 * composition's 0x391350; the second word passed on) */
extern "C" void func_00399040(unsigned short c, int arg);
/* a letter cased (0x39A8D0, its third argument 0 here) and a jamo typed into the composition
 * (0x399200: the argument first) */
extern "C" unsigned short func_0039A8D0(int c, bool upper, int x);
extern "C" void func_00399200(int arg, signed char jamo);
/* the jamo a key types (zui_hangul.cpp's) */
signed char KeyToJamo(char c);
extern "C" int toupper(int c);

/* the digit row's characters, HID usages 30-39 (1 .. 0), and shifted */
static const int s_digitChars[12] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
static const int s_shiftDigitChars[12] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
/* the punctuation's, HID usages 49-56, and shifted */
static const int s_shiftPunctChars[12] = {'|', '}', ':', '"', '~', '<', '>', '?'};
static const int s_punctChars[12] = {'\\', ']', ';', '\'', '`', ',', '.', '/'};

/* a letter key, HID usages 4-29: a jamo in Korean input (the shifted letter's), else the letter
 * cased by shift against caps lock */
void SkbUsbLetter(const SKBUSBKEY *key, int arg, bool korean)
{
    char c = key->m_usage + ('a' - 4);

    if (korean)
        func_00399200(arg, KeyToJamo(key->m_shift ? toupper(c) : c));
    else
        func_00399040(func_0039A8D0(c, (key->m_shift != 0) ^ (key->m_capsLock != 0), 0), arg);
}

/* the dash key: an underscore shifted */
void SkbUsbDash(const SKBUSBKEY *key, int arg)
{
    func_00399040(key->m_shift ? '_' : '-', arg);
}

/* a punctuation key */
void SkbUsbPunct(const SKBUSBKEY *key, int arg)
{
    func_00399040((key->m_shift ? s_shiftPunctChars : s_punctChars)[key->m_usage - 49], arg);
}

/* a digit key */
void SkbUsbDigit(const SKBUSBKEY *key, int arg)
{
    func_00399040((key->m_shift ? s_shiftDigitChars : s_digitChars)[key->m_usage - 30], arg);
}

/* (the keyboard units' functions not yet written, by address: the text set and laid out, the
 * cursor drawn, the composition flushed, the keyboard's state; the edit mode switched, the text's
 * fields placed; a jamo typed; whether shift composes) */
extern "C" {
void func_0039A5B0(const char *text, CSoftKeyboard *skb);
void func_0039A1D0(int a, CSoftKeyboard *skb, int b);
void func_00391350();
void func_00395DE0(int state);
void func_0039A400(CSoftKeyboard *skb);
bool func_00394840(CSoftKeyboard *skb);
void func_00394920(CSoftKeyboard *skb);
int func_00396E60(CSoftKeyboard *skb);
}

extern "C" {
int func_00396EF0();
/* CGameMenu::RunEventAnim (zui.cpp's 0x367810: the menu, an event, here the text) */
void func_00367810(CGameMenu *menu, int event, const char *text);
}

/* the text taken: the typed text into the variable SkbGetText(false) names, string 0's into the
 * other's, the menu's event 15 (or 14, by 0x396EF0) run with it (0x399D20, OnEnterKey's; the name
 * ours).
 * OPEN, 2 instructions long: the target keeps the string's address in s1 from before its zeroing
 * through the assign and the append (and SkbTextUnchanged builds the same text the same way in a
 * conditional temporary), the shape of an inlined function returning the string by value, its
 * hidden result pointer a variable; MW folds the local's address into each use here. TypedText()
 * as such an inline is never inlined (static or not; warn_notinlined gives no reason), and the
 * local written out loses the register */
void SkbTakeText(CGameMenu *menu)
{
    CSaferStr text(ShownText(&s_textString1));

    text += ShownText(&s_textString2);
    *theUIVarManager.Add(SkbGetText(false), 0, UIVL_TEMPORARY) = text.c_str();
    const char *s = ShownText(&s_textString0);

    *theUIVarManager.Add(SkbGetText(true), 0, UIVL_TEMPORARY) = s ? s : "";
    func_00367810(menu, func_00396EF0() ? 15 : 14, text.c_str());
}

extern "C" {
unsigned short func_003910E0();
void func_00398C00(unsigned short c, CSoftKeyboard *skb);
void func_00398C40(CSoftKeyboard *skb);
void func_003952F0();
}
int UTF8CharLen(const char *&s);

/* two words of the text's (0x4A1348, 0x4A1350: one says whether string 1 counts, the other is
 * added to its count; the names ours) */
static int s_textCount1;
static int s_textCount0;

/* a character's length (a copy of the pointer handed on: the target stores it for each call) */
static inline int CharLen(const char *s)
{
    return UTF8CharLen(s);
}

/* the string has a character at p (ours: materialised before the loop's branch) */
static inline bool HasChar(const char *p)
{
    return p && *p;
}

/* the composition's character typed, if there is one; else 0x398C40 when the text has characters
 * (string 1's counted when s_textCount1 says so, plus s_textCount0), 0x3952F0 when not; then
 * state 3 (0x399C20; the name ours).
 * OPEN, 13 instructions (same size, same blocks): the saved registers are numbered otherwise --
 * the target keeps the count in s0, skb in s1 and the pointer in s2 (here p s0, the count s1, skb
 * s2) -- and does the count's ++ in UTF8CharLen's delay slot, after the pointer's store. Tried: the
 * count as an inline CountChars(p) with its own locals (13 again), p and the count declared ahead
 * (0x10 short) */
void SkbTypeOrBack(CSoftKeyboard *skb)
{
    unsigned short c = func_003910E0();

    if (c) {
        func_00398C00(c, skb);
    } else {
        int n;

        if (s_textCount1) {
            n = 0;
            for (const char *p = ShownText(&s_textString1); HasChar(p); n++)
                p += CharLen(p);
        } else {
            n = 0;
        }
        if (s_textCount0 + n > 0)
            func_00398C40(skb);
        else
            func_003952F0();
    }
    func_00395DE0(3);
}

/* the text cleared: whether there was any, the composition flushed and the keyboard put in state 3
 * (0x399B30; the name ours) */
bool SkbClearText(CSoftKeyboard *skb)
{
    bool had = false;

    if (ShownText(EditString()) && *ShownText(EditString()))
        had = true;
    func_0039A5B0("", skb);
    func_0039A1D0(0, skb, 0);
    func_00391350();
    func_00395DE0(3);
    return had;
}

/* whether the typed text is the string variable SkbGetText(false) names already holds (0x3998A0,
 * OnEnterKey's IgnoreUnchangedString test; the name ours).
 * OPEN, 0x60 bytes short: the target builds the typed text in a temporary only when the variable
 * is a string (a flag in s1 guards its destructor), its address held in s1 through the assign and
 * the append and c_str() read off the temporary, not append's result -- TypedText() inlined, which
 * MW refuses here (see SkbTakeText) */
bool SkbTextUnchanged()
{
    CUIVariable *v = theUIVarManager.Find(SkbGetText(false));
    const char *val = v ? v->String() : 0;

    return val && !strcmp(val, TypedText().c_str());
}

/* string 0's edit closed: its node hidden, its text emptied, the text laid out again if it was the
 * current one (0x399280; the name ours) */
void SkbCloseText0(CSoftKeyboard *skb)
{
    bool was = s_textSel;

    s_textSel = false;
    s_textNode->SetActive(false);
    C2DString *s = &s_textString0;
    s->SetString(0, true);
    s_textString0.UpdatePos();
    if (was)
        func_0039A400(skb);
    func_00394920(skb);
}

/* the edit mode switched (0x394840 says whether string 0 is current now): its node shown or hidden,
 * the text laid out again when either is string 0, string 0 emptied (0x399150; the name ours) */
void SkbSwitchText(CSoftKeyboard *skb)
{
    bool was = s_textSel;

    s_textSel = func_00394840(skb);
    s_textNode->SetActive(s_textSel);
    if (s_textSel || was)
        func_0039A400(skb);
    C2DString *s = &s_textString0;
    s->SetString(0, true);
    s_textString0.UpdatePos();
}

/* the compound jamo a shifted key composes (0x3FB840: a jamo and its doubled form, seven pairs) */
struct SKBJAMOPAIR {
    unsigned char m_from;
    signed char m_to;
};
static const SKBJAMOPAIR s_shiftJamo[7] = {
    {0x19, 0x17}, {0x01, 0x1C}, {0x04, 0x1D}, {0x10, 0x15}, {0x0B, 0x14}, {0x12, 0x16}, {0x03, 0x13}
};

/* a jamo typed: composed with shift when 0x396E60 says so (0x399070; the name ours) */
void SkbTypeJamo(int arg, signed char jamo, CSoftKeyboard *view)
{
    if (func_00396E60(view)) {
        for (unsigned int i = 0; i < 7; i++) {
            if ((unsigned char)jamo == s_shiftJamo[i].m_from) {
                jamo = s_shiftJamo[i].m_to;
                break;
            }
        }
    }
    func_00399200(arg, jamo);
}
