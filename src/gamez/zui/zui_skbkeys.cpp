/* zui_skbkeys.cpp -- the soft keyboard's USB-keyboard keys (OVL1 ~0x39AD00-0x39ADFF so far: the unit's
 * extent is not settled -- it lies between the keyboard's display unit, initialiser 0x403800, and the
 * buffered file at 0x39BBC0; the CRenderableString globals of initialiser 0x404110 may be its): the
 * handlers zui_skb.cpp's USB loop (0x38C630) calls for a key by its HID usage -- the letters, the digit row, the
 * punctuation, the dash -- each typing its character, shifted or not, through 0x399040. SOCOM 1 has
 * none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * OPEN, not written yet: the rest of the unit. */

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
