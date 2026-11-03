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

/* the text's three strings on screen (0x4A1370, 0x4A1450, 0x4A1520: the initialiser 0x404110 builds
 * them; the names ours) */
static CRenderableString s_textString0;
static CRenderableString s_textString1;
static CRenderableString s_textString2;

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
