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
#include "zui_hangul.h"
#include "../znode/znode.h"

/* the SDK libc's character table (newlib's _ctype_, MAIN 0x1D08D0): bit 8 is white space */
extern "C" const char _ctype_[];
#define rdr_isspace(c) ((_ctype_ + 1)[c] & 8)

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

/* the text typed: string 1's, then string 2's, built in place by a constructor (the class ours, the
 * UI lane's round 17): its `this` is a variable of the caller's, so the object's address sits in a
 * saved register from before the words' zeroing through the assign and the append, as the target's
 * SkbTakeText 0x399D20 and 0x3996B0 keep it -- an inline returning the string by value, which MW
 * never inlines (warn_notinlined gives no reason), and a local built and appended to in the caller
 * (the address folded into each use) both miss it */
struct CTypedText : public CSaferStr {
    CTypedText() : CSaferStr(ShownText(&s_textString1)) { *this += ShownText(&s_textString2); }
};

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
/* a letter cased (0x39A8D0, its third argument 0 here) */
extern "C" unsigned short func_0039A8D0(int c, bool upper, int x);
/* the keyboard's text given the composed pieces (0x398AE0: the argument first; whether it took
 * them) */
extern "C" int func_00398AE0(int arg, CSkbPieces *pieces, bool fewer);
void SkbHangulTypeJamo(int arg, signed char jamo);
/* the jamo a key types, its last jamo taken back (zui_hangul.cpp's) */
signed char KeyToJamo(char c);
unsigned short SkbHangulBack();
extern "C" int toupper(int c);

/* the digit row's characters, HID usages 30-39 (1 .. 0), and shifted */
static const int s_digitChars[12] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};
static const int s_shiftDigitChars[12] = {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')'};
/* the punctuation's, HID usages 49-56, and shifted */
static const int s_shiftPunctChars[12] = {'|', '}', ':', '"', '~', '<', '>', '?'};
static const int s_punctChars[12] = {'\\', ']', ';', '\'', '`', ',', '.', '/'};

/* a piece of the text the wrap lays out (ours): a word, a run of spaces, a line break */
enum SKBTOKENTYPE {
    SKBTOKEN_WORD,
    SKBTOKEN_SPACE,
    SKBTOKEN_NEWLINE
};

/* where the cut is (ours): before any piece or after a break, in a word, in spaces */
enum SKBWRAPSTATE {
    SKBWRAP_NONE,
    SKBWRAP_WORD,
    SKBWRAP_SPACE
};

/* a piece and its extent (ours: 12 bytes, the type a byte -- the insert 0x39B670 copies it lb/sb) */
struct SKBTOKEN {
    SKBTOKEN(SKBTOKENTYPE type, const char *start) : m_type(type), m_start(start), m_end(0) {}

    SKBTOKENTYPE m_type;   // 0x0
    const char *m_start;   // 0x4
    const char *m_end;   // 0x8
};

/* a string's measured width (ours: the extent UpdatePos leaves at 0x3C, read through its address) */
static inline const float &StringWidth(const C2DString &s)
{
    return s.m_length;
}

/* the pieces laid out in lines no wider than width at the scale: each line as many pieces as fit,
 * measured as a C2DString in the font, a line break ending one; false when a single word is too
 * wide (0x39B350; the name ours). The word's string is a std::string built by the (s, n) constructor
 * (0x39B5B0, destroyed with -1, as the C2DString), the line assigned through assign(s, n) (0x39B620).
 * OPEN, 5 instructions (same size): the outer loop's first block schedules the first piece's read
 * otherwise -- the target loads the data pointer first, then i * 12, `first = i` between; here the
 * index first. Tried: the three declarations in every order, `tokens[first]`, `begin() + i`, a
 * reference to the piece, a for loop, an int index; round 19: `tokens.data()[i]`, `&tokens[i]` into a
 * pointer, `(&*begin())[i]`, a reference to the vector, the const operator[] -- all 5 off (the
 * pre-colouring code has the index's sll/addu ahead of the data load in every one; the inner loop's
 * `SKBTOKEN &t = tokens[i]` loads the data first in both builds) */
bool SkbWrapTokens(std::vector<CSaferStr> &lines, std::vector<SKBTOKEN> &tokens, C2DFont *font, float scale,
                   float width)
{
    unsigned int i = 0;

    while (i < tokens.size()) {
        unsigned int first = i;
        bool fits = true;
        const char *start = tokens[i].m_start;
        const char *end = start;

        while (fits && i < tokens.size()) {
            SKBTOKEN &t = tokens[i];

            switch (t.m_type) {
            case SKBTOKEN_WORD: {
                std::string s(start, t.m_end - start);
                C2DString str;

                str.Load(scale, s.c_str(), font, 0.0f, 0.0f, false);
                str.UpdatePos();
                fits = StringWidth(str) <= width;
                if (fits) {
                    end = t.m_end;
                    i++;
                } else if (first == i) {
                    return false;
                }
                break;
            }
            case SKBTOKEN_SPACE:
                i++;
                break;
            case SKBTOKEN_NEWLINE:
                fits = false;
                i++;
                break;
            }
        }
        lines.resize(lines.size() + 1);
        SkbPrev(lines.end())->assign(start, end - start);
    }
    return true;
}

int UTF8CharLen(const char *&s);

/* the width a string's first n characters take as shown (0x39B1D0; the name ours -- 0x399320,
 * 0x399530, 0x39A1D0 and 0x39A400 call it with one of the three text strings): its text from the
 * first shown character copied, the first n UTF-8 characters' bytes counted, the string set to that
 * substring (include/string's (str, pos, n) constructor, 0x332320) and measured, then set back */
float SkbTextWidth(C2DString *s, int n)
{
    CSaferStr text(ShownText(s));
    const char *p = text.c_str();
    int len = 0;

    for (int i = 0; i < n; i++) {
        const char *q = p + len;

        len += UTF8CharLen(q);
    }
    std::string sub(text, 0, len);

    s->SetString(sub.c_str(), true);
    s->UpdatePos();
    float w = StringWidth(*s);

    s->SetString(text.c_str(), true);
    s->UpdatePos();
    return w;
}

/* a flag's negation (ours: materialised, sltu and xori, before each test branches on it) */
static inline bool SkbNot(int x)
{
    return !(bool)x;
}

/* the text cut into pieces and wrapped into lines (SkbWrapTokens): split after each run of white
 * space when byWord, else into words, spaces and line breaks; true for no text (0x39AE50; the name
 * ours: the keyboard's text layout 0x39A5B0 and 0x22B4A0 call it). The pieces are built in place by
 * their constructor (the push_back's temporary, its fields' addresses hoisted), the last one's end
 * set through SkbPrev; the state is a byte enum (held unnarrowed, `movz` into a1), the flags' tests
 * materialised through SkbNot.
 * OPEN, 3 instructions (same size): `state == SKBWRAP_WORD` takes its xori temporary in v1 in the
 * target (then v1 zeroed for wordChar), here in a0, inWord's own register. Tried: the flags in
 * every declaration order, `!(state != ...)`, an int inWord, `(int)state == 1`. Round 19, read in
 * the igdump record: MW's pre-colouring schedule puts wordChar's zero (`dmove r43 r0`) between the
 * xori and its sltiu whatever its declaration's place (after `space`, assigned after the notEnd
 * test, a second assignment), so the xori temporary interferes with wordChar and cannot share v1;
 * inWord through an inline (`SkbIsWord`, SkbNot of the inequality) is 8-24 bytes longer */
bool SkbWrapText(std::vector<CSaferStr> &lines, const char *text, C2DFont *font, bool byWord, float scale,
                 float width)
{
    if (!text || !*text)
        return true;

    std::vector<SKBTOKEN> tokens;

    if (byWord) {
        tokens.push_back(SKBTOKEN(SKBTOKEN_WORD, text));
        for (;;) {
            char c = *++text;

            if (!c)
                break;
            if (rdr_isspace(text[-1]) && !rdr_isspace(c)) {
                SkbPrev(tokens.end())->m_end = text;
                tokens.push_back(SKBTOKEN(SKBTOKEN_WORD, text));
            }
        }
        SkbPrev(tokens.end())->m_end = text;
    } else {
        SKBWRAPSTATE state = SKBWRAP_NONE;

        for (;;) {
            char c = *text;
            bool notEnd = false;
            bool inWord = state == SKBWRAP_WORD;
            bool wordChar = false;
            int newline = c == '\n';
            int space = c == ' ';

            if (!SkbNot(c) && SkbNot(newline))
                notEnd = true;
            if (notEnd && SkbNot(space))
                wordChar = true;
            if (inWord && !wordChar)
                SkbPrev(tokens.end())->m_end = text;
            bool newSpace = false;

            if (state != SKBWRAP_SPACE && space)
                newSpace = true;
            if (newline || newSpace || (!inWord && wordChar)) {
                tokens.push_back(SKBTOKEN(newline ? SKBTOKEN_NEWLINE : (space ? SKBTOKEN_SPACE : SKBTOKEN_WORD), text));
                state = newline ? SKBWRAP_NONE : (space ? SKBWRAP_SPACE : SKBWRAP_WORD);
            }
            if (!*text)
                break;
            text++;
        }
    }
    bool ok = SkbWrapTokens(lines, tokens, font, scale, width);

    return ok;
}

/* a letter key, HID usages 4-29: a jamo in Korean input (the shifted letter's), else the letter
 * cased by shift against caps lock */
void SkbUsbLetter(const SKBUSBKEY *key, int arg, bool korean)
{
    char c = key->m_usage + ('a' - 4);

    if (korean)
        SkbHangulTypeJamo(arg, KeyToJamo(key->m_shift ? toupper(c) : c));
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

/* (the keyboard units' functions not yet written, by address: the cursor drawn, the composition
 * flushed, the keyboard's state; the edit mode switched, the text's fields placed; a jamo typed;
 * whether shift composes) */
extern "C" {
void func_0039A1D0(int a, CSoftKeyboard *skb, int b);
}
/* the text set and laid out (0x39A5B0), the cursor put after it (0x39A400) */
bool SkbSetText(const char *text, CSoftKeyboard *skb);
void SkbCursorToEnd(CSoftKeyboard *skb);
void SkbPlaySound(unsigned char which);
bool SkbToggleCaption1(CSoftKeyboard *skb);
void SkbToggleCaption2(CSoftKeyboard *skb);

extern "C" {
int func_00396EF0();
}

/* the game menu (zui.cpp's: only its event runner) */
class CGameMenu {
public:
    bool RunEventAnim(ActionItem::EVENT event, const char *arg);
};

/* the text taken: the typed text into the variable SkbGetText(false) names, string 0's into the
 * other's, the menu's ON_SKB_APPLY2 (or ON_SKB_APPLY, by 0x396EF0) run with it (0x399D20, OnEnterKey's; the name
 * ours). The typed text built in place (CTypedText); the second name fetched before string 0's text
 * is read, the empty string an if-assign (the ternary lays its arms out the other way), and the event
 * chosen into a local ahead of the run (in the argument list c_str() is computed before the call) */
void SkbTakeText(CGameMenu *menu)
{
    CTypedText text;

    *theUIVarManager.Add(SkbGetText(false), 0, UIVL_TEMPORARY) = text.c_str();
    const char *name = SkbGetText(true);
    const char *s = ShownText(&s_textString0);

    if (!s)
        s = "";
    *theUIVarManager.Add(name, 0, UIVL_TEMPORARY) = s;
    ActionItem::EVENT e = func_00396EF0() ? ActionItem::ON_SKB_APPLY2 : ActionItem::ON_SKB_APPLY;

    menu->RunEventAnim(e, text.c_str());
}

extern "C" {
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

/* the string has a character at p, into the caller's flag (ours: the counting walks' test). The flag is
 * the caller's own local, declared ahead of the cursor, cleared and then set -- so it stays a variable
 * of its own (an inline's bool return is folded into a temporary) and MW numbers it before the
 * cursor: a low-degree neighbour pushed in the first simplify sweep, which lets the cursor (25
 * neighbours otherwise, one over K) be pushed there too, so the count and skb take s0/s1 and the
 * cursor s2, as the target's. The code is HasChar's */
static inline bool TestChar(bool &more, const char *p)
{
    more = false;
    if ((bool)p && *p)
        more = true;
    return more;
}

/* the composition's character typed, if there is one; else 0x398C40 when the text has characters
 * (string 1's counted when s_textCount1 says so, plus s_textCount0), 0x3952F0 when not; then
 * state 3 (0x399C20; the name ours). The count's walk tests through TestChar's flag (its numbering:
 * open from round 9 to 17 with the walk's test an inline bool, HasChar) and counts ahead of the step
 * (the ++ in UTF8CharLen's delay slot) */
void SkbTypeOrBack(CSoftKeyboard *skb)
{
    unsigned short c = SkbHangulBack();

    if (c) {
        func_00398C00(c, skb);
    } else {
        int n;

        if (s_textCount1) {
            bool more;

            n = 0;
            const char *p = ShownText(&s_textString1);
            while (TestChar(more, p)) {
                n++;
                p += CharLen(p);
            }
        } else {
            n = 0;
        }
        if (s_textCount0 + n > 0)
            func_00398C40(skb);
        else
            func_003952F0();
    }
    SkbPlaySound(3);
}

/* the text cleared: whether there was any, the composition flushed and the keyboard put in state 3
 * (0x399B30; the name ours) */
bool SkbClearText(CSoftKeyboard *skb)
{
    bool had = false;

    if (ShownText(EditString()) && *ShownText(EditString()))
        had = true;
    SkbSetText("", skb);
    func_0039A1D0(0, skb, 0);
    SkbHangulClear();
    SkbPlaySound(3);
    return had;
}

/* whether the typed text is the string variable SkbGetText(false) names already holds (0x3998A0,
 * OnEnterKey's IgnoreUnchangedString test; the name ours): the typed text a conditional temporary,
 * built only when the variable is a string (a flag in s1 guards its destructor), CTypedText's
 * constructor keeping its address in s1 through the assign and the append */
bool SkbTextUnchanged()
{
    CUIVariable *v = theUIVarManager.Find(SkbGetText(false));
    const char *val = v ? v->String() : 0;

    return val && !strcmp(val, CTypedText().c_str());
}

unsigned short UTF8GetChar(const char *&s);

/* two more words of the text's (0x4A1338: the kana entry enabled while positive, never written in
 * OVL1; 0x4A1340: the entry's mode, 1 kana, 0x399C00 clears it; the names ours) */
static int s_kanaEnabled;
static int s_kanaMode;

/* the string the cursor is in (ours): string 0 while it is edited, else string 2 when string 1
 * counts, else string 1 */
static inline C2DString *CursorString()
{
    return s_textSel ? &s_textString0 : (s_textCount1 ? &s_textString2 : &s_textString1);
}

/* a text's n-th UTF-8 character, or none for a negative n (ours) */
static inline const char *NthChar(const char *text, int n)
{
    if (n >= 0) {
        int len = 0;

        for (int i = 0; i < n; i++)
            len += CharLen(text + len);
        return text + len;
    }
    return 0;
}

/* the character at p, 0 for none (ours: read off a copy of the cursor declared in the arm -- the
 * store stays out of the test's delay slot, both labels padded, as the target's) */
static inline unsigned short CharCode(const char *p)
{
    if (p) {
        const char *q = p;

        return UTF8GetChar(q);
    }
    return 0;
}

/* the enter key's kana step (0x3999D0, OnEnterKey's first test; the name ours): nothing while the
 * kana entry is off; in kana mode an `n` before the cursor typed as the syllabic n (U+3093) first;
 * then the composition cleared */
int SkbEnterKana(CSoftKeyboard *skb)
{
    if (!(s_kanaEnabled > 0))
        return 0;
    if (s_kanaMode == 1) {
        int n = s_textCount0 - 1;
        const char *p = NthChar(ShownText(CursorString()), n);

        if (CharCode(p) == 'n')
            func_00398C00(0x3093, skb);
    }
    SkbHangulClear();
    return 1;
}

/* the byte length of the character at a byte offset of the text being edited -- string 0's while it
 * is edited, else string 1's with string 2's after it when string 2 has any (0x39A790; the name
 * ours: 0x398C40 calls it) -- the string default-built, then assigned (the target zeroes its words
 * before it reads the text), the test a ternary into a bool local (materialised, andi at the join) */
int SkbEditCharLen(int pos)
{
    CSaferStr text;

    text = ShownText(EditString());
    bool more = s_textSel ? false : HasChar(ShownText(&s_textString2));

    if (more)
        text += ShownText(&s_textString2);
    return CharLen(text.c_str() + pos);
}

/* the text cursor put at a place (0x395120, the display unit's: its node's position) */
extern "C" void func_00395120(float x, float y);

/* which string props a cursor follows (ours: the look's three, User 0, Purpose 1, MsgPlayer 2) */
enum SKBPROPS { SKB_PROPS_USER, SKB_PROPS_PURPOSE, SKB_PROPS_MSGPLAYER };

/* the cursor's y (ours: CursorY2nd while string 0 is edited, else CursorY -- by reference, so the
 * address is selected and then read, as the target's) */
static inline const int &CursorY(const CSoftKeyboard *skb)
{
    return s_textSel ? skb->m_cc : skb->m_c8;
}

/* the line offset of the counted strings (ours): a line height per counted string. The height a
 * named local of the inline -- numbered before the temporaries, so the count's are coloured first
 * and take $f0, the height $f1 -- and the whole an inline expanded ahead of the y's sum (a bare
 * temporary there, so the y is the sum's first operand), as the target's */
static inline float LineOffset(const CSoftKeyboard *skb)
{
    float h = skb->m_c4;

    return (float)s_textCount1 * h;
}

/* the text set into the strings (0x39A5B0; the name ours: SkbClearText sets ""): wrapped to the
 * look's width at the edited string's scale and its font's, refused when it takes more lines than
 * there are -- one while string 0 is edited, else two when the look has a line height --, the first
 * line the edited string's and the second string 2's */
bool SkbSetText(const char *text, CSoftKeyboard *skb)
{
    float width = skb->m_c0;
    C2DString *s = EditString();
    C2DFont *font = s->m_font;
    float scale = s->m_scale * (font ? font->m_scale_factor : 1.0f);
    std::vector<CSaferStr> lines;

    if (!SkbWrapText(lines, text, font, true, scale, width))
        return false;
    int n = lines.size();
    int max = s_textSel ? 1 : (!skb->m_c4 ? 1 : 2);

    if (max < n)
        return false;
    s->SetString(n ? lines[0].c_str() : "", true);
    s->UpdatePos();
    if (!s_textSel) {
        s_textString2.SetString(n >= 2 ? lines[1].c_str() : "", true);
        s_textString2.UpdatePos();
    }
    return true;
}

/* the cursor put after the text (0x39A400; the name ours: SkbCloseText0 and SkbSwitchText call it,
 * out of line, so pinned): whether string 2 holds characters made the count's flag (SkbEditCharLen's
 * ternary), the cursor's string's characters counted -- SkbCursorBack's walk -- and the cursor
 * placed after them as SkbCursorBack places it */
#pragma auto_inline off
void SkbCursorToEnd(CSoftKeyboard *skb)
{
    s_textCount1 = s_textSel ? false : HasChar(ShownText(&s_textString2));
    bool more;
    const char *p = ShownText(CursorString());
    int n = 0;

    while (TestChar(more, p)) {
        n++;
        p += CharLen(p);
    }
    s_textCount0 = n;
    float w = SkbTextWidth(CursorString(), n);
    SKBPROPS props = s_textSel ? SKB_PROPS_MSGPLAYER : SKB_PROPS_USER;
    float x = w + (float)skb->m_caps[props].m_0;
    float y = (float)CursorY(skb) + LineOffset(skb);

    func_00395120(x, y);
}
#pragma auto_inline reset

/* the cursor stepped back a character (0x399530; the name ours: 0x38C630's left key and
 * SkbHandleKey call it with the showing keyboard): string 1's count down by one, or at its start,
 * while string 1 counts, string 1 given up and its characters counted into the count; then the
 * cursor placed after that many characters of the cursor's string -- the props' x plus their width,
 * the cursor's y plus a line height per counted string -- and the composition cleared. The walk is
 * SkbTypeOrBack's (TestChar's flag declared ahead of the cursor); the count read into a local before
 * the width call gives the target's argument order (the count loaded ahead of the string's ternary) */
void SkbCursorBack(CSoftKeyboard *skb)
{
    if (s_textCount0 > 0) {
        s_textCount0--;
    } else if (s_textCount1) {
        bool more;
        int n = 0;

        s_textCount1 = 0;
        const char *p = ShownText(&s_textString1);
        while (TestChar(more, p)) {
            n++;
            p += CharLen(p);
        }
        s_textCount0 = n;
    }
    int count = s_textCount0;
    float w = SkbTextWidth(CursorString(), count);
    SKBPROPS props = s_textSel ? SKB_PROPS_MSGPLAYER : SKB_PROPS_USER;
    float x = w + (float)skb->m_caps[props].m_0;
    float y = (float)CursorY(skb) + LineOffset(skb);

    func_00395120(x, y);
    SkbHangulClear();
}

/* the typed text holds a character other than white space (0x3996B0; the name ours: OnEnterKey takes
 * the text only when this or 0x395110 finds one) -- the typed text built in place, each UTF-8
 * character read off a copy of the cursor and tested against the SDK's table, the cursor stepped by
 * the character's length; an int (OnEnterKey's test of it needs one) */
int SkbTypedTextNotBlank()
{
    CTypedText text;

    for (const char *p = text.c_str(); HasChar(p); p += CharLen(p)) {
        const char *q = p;

        if (!rdr_isspace(UTF8GetChar(q)))
            return true;
    }
    return false;
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
        SkbCursorToEnd(skb);
    SkbToggleCaption2(skb);
}

/* a jamo typed into the composition: the pieces composed with it handed to the keyboard's text
 * (0x398AE0), the composition dropped when the text does not take them (0x399200; the name ours:
 * SkbTypeJamo and SkbUsbLetter call it out of line, so pinned) */
#pragma auto_inline off
void SkbHangulTypeJamo(int arg, signed char jamo)
{
    CSkbPieces pieces;
    std::pair<bool, bool> r = SkbHangulType(&pieces, jamo);

    if (r.first && !func_00398AE0(arg, &pieces, r.second))
        SkbHangulClear();
}
#pragma auto_inline reset

/* the edit mode switched (0x394840 says whether string 0 is current now): its node shown or hidden,
 * the text laid out again when either is string 0, string 0 emptied (0x399150; the name ours) */
void SkbSwitchText(CSoftKeyboard *skb)
{
    bool was = s_textSel;

    s_textSel = SkbToggleCaption1(skb);
    s_textNode->SetActive(s_textSel);
    if (s_textSel || was)
        SkbCursorToEnd(skb);
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

/* a jamo typed: composed with shift when SkbShifted says so (0x399070; the name ours) */
void SkbTypeJamo(int arg, signed char jamo, CPad *pad)
{
    if (SkbShifted(pad)) {
        for (unsigned int i = 0; i < 7; i++) {
            if ((unsigned char)jamo == s_shiftJamo[i].m_from) {
                jamo = s_shiftJamo[i].m_to;
                break;
            }
        }
    }
    SkbHangulTypeJamo(arg, jamo);
}
