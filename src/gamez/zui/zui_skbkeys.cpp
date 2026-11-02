/* zui_skbkeys.cpp -- the soft keyboard's USB-keyboard keys (OVL1 ~0x39AD00-0x39ADFF so far: the unit's
 * extent is not settled -- it lies between the keyboard's display unit, initialiser 0x403800, and the
 * buffered file at 0x39BBC0; the CRenderableString globals of initialiser 0x404110 may be its): the
 * handlers zui_skb.cpp's USB loop (0x38C630) calls for a key by its HID usage -- the digit row, the
 * punctuation, the dash -- each typing its character, shifted or not, through 0x399040. SOCOM 1 has
 * none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * OPEN, not written yet: the rest of the unit (the letter key 0x39ADA0, through the Korean key map). */

/* a USB key as the loop hands it on: its HID usage and whether shift is held (the names ours) */
struct SKBUSBKEY {
    unsigned char m_usage;   // 0x0
    unsigned char _1[3];
    int m_shift;   // 0x4
};

/* a character typed into the keyboard's text (0x399040: the text's insertion 0x399EC0, then the
 * composition's 0x391350; the second word passed on) */
extern "C" void func_00399040(unsigned short c, int arg);

/* the digit row's characters, HID usages 30-39 (1 .. 0), and shifted */
static const int s_digitChars[12] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
static const int s_shiftDigitChars[12] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
/* the punctuation's, HID usages 49-56, and shifted */
static const int s_shiftPunctChars[12] = {'|', '}', ':', '"', '~', '<', '>', '?'};
static const int s_punctChars[12] = {'\\', ']', ';', '\'', '`', ',', '.', '/'};

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
