/* zui_hangul.cpp -- the soft keyboard's Korean input (OVL1 0x3903A0-~0x39441F, its initialiser
 * 0x403770 building two vectors and a word at 0x49ED08-0x49ED40): the 33 jamo by name, the 2-set
 * keyboard's letters mapped to jamo (shifted and not), the syllable being composed, and the rdr
 * helpers the keyboard units share (an rdr array's count, an rdr array walked by index). SOCOM 1 has
 * none of it: the file's name and every name here are ours. Defined in reverse of their image order
 * (-inline deferred).
 *
 * OPEN, not written yet: everything but the functions below -- the composition (0x3903A0,
 * 0x3908A0, 0x390A30, 0x390C40, 0x3910E0-0x3919D0), the vector<char> instances (insert 0x3923E0,
 * erase 0x392A30, resize 0x392600, the assignment 0x393D30, the copy 0x393D80, init 0x393F00,
 * assign 0x393FB0: no 1-byte pod vector in include/vector) and the rest of the unit. */

#include <string.h>
#include "zui.h"
#include "zui_skb.h"

/* the keyboard display unit's reader of UiParams.rdr (0x397070) */
extern "C" void func_00397070(CRdrFile *file);

/* the Korean letters (jamo), by name in alphabetical order, as the table of their names lists them
 * (the names ours: the table's own, e.g. Kiyeok, SsangKiyeok, A_Hang) */
enum JAMO {
    JAMO_A,
    JAMO_AE,
    JAMO_CHIEUCH,
    JAMO_CIEUC,
    JAMO_E,
    JAMO_EO,
    JAMO_EU,
    JAMO_HIEUH,
    JAMO_I,
    JAMO_IEUNG,
    JAMO_KHIEUKH,
    JAMO_KIYEOK,
    JAMO_MIEUM,
    JAMO_NIEUN,
    JAMO_O,
    JAMO_PHIEUPH,
    JAMO_PIEUP,
    JAMO_RIEUL,
    JAMO_SIOS,
    JAMO_SSANGCIEUC,
    JAMO_SSANGKIYEOK,
    JAMO_SSANGPIEUP,
    JAMO_SSANGSIOS,
    JAMO_SSANGTIKEUT,
    JAMO_THIEUTH,
    JAMO_TIKEUT,
    JAMO_U,
    JAMO_YA,
    JAMO_YAE,
    JAMO_YE,
    JAMO_YEO,
    JAMO_YO,
    JAMO_YU,
    JAMO_COUNT
};

/* the 2-set keyboard: the jamo each letter key types, unshifted (a-z) and shifted (A-Z) */
static const signed char s_keyJamo[26] = {
    JAMO_MIEUM, JAMO_YU, JAMO_CHIEUCH, JAMO_IEUNG, JAMO_TIKEUT, JAMO_RIEUL, JAMO_HIEUH, JAMO_O,
    JAMO_YA, JAMO_EO, JAMO_A, JAMO_I, JAMO_EU, JAMO_U, JAMO_AE, JAMO_E,
    JAMO_PIEUP, JAMO_KIYEOK, JAMO_NIEUN, JAMO_SIOS, JAMO_YEO, JAMO_PHIEUPH, JAMO_CIEUC, JAMO_THIEUTH,
    JAMO_YO, JAMO_KHIEUKH
};
static const signed char s_shiftKeyJamo[26] = {
    JAMO_MIEUM, JAMO_YU, JAMO_CHIEUCH, JAMO_IEUNG, JAMO_SSANGTIKEUT, JAMO_RIEUL, JAMO_HIEUH, JAMO_O,
    JAMO_YA, JAMO_EO, JAMO_A, JAMO_I, JAMO_EU, JAMO_U, JAMO_YAE, JAMO_YE,
    JAMO_SSANGPIEUP, JAMO_SSANGKIYEOK, JAMO_NIEUN, JAMO_SSANGSIOS, JAMO_YEO, JAMO_PHIEUPH,
    JAMO_SSANGCIEUC, JAMO_THIEUTH, JAMO_YO, JAMO_KHIEUKH
};

/* the jamo's names (the rdr's spelling) */
static const char *s_jamoNames[JAMO_COUNT] = {
    "A_Hang", "Ae", "Chieuch", "Cieuc", "E_Hang", "Eo", "Eu", "Hieuh", "I_Hang", "Ieung", "Khieukh",
    "Kiyeok", "Mieum", "Nieun", "O_Hang", "Phieuph", "Pieup", "Rieul", "Sios", "SsangCieuc",
    "SsangKiyeok", "SsangPieup", "SsangSios", "SsangTikeut", "Thieuth", "Tikeut", "U_Hang", "Ya", "Yae",
    "Ye", "Yeo", "Yo", "Yu"
};

/* a comparison that takes nulls: a null first sorts before anything, a null second after */
static inline int SafeStrcasecmp(const char *a, const char *b)
{
    return a ? (b ? strcasecmp(a, b) : 1) : -1;
}

/* the keyboards' display read again from UiParams.rdr (CMission::Init calls it; the image puts it
 * just before the display unit's first function, so it may be that unit's) */
void SkbLoadUiParams()
{
    CRdrFileHolder file("UiParams.rdr", "data/common");

    func_00397070(file);
}

/* the jamo a key types: a lowercase letter's, or a shifted one's */
signed char KeyToJamo(char c)
{
    if (c >= 'a' && c <= 'z')
        return s_keyJamo[c - 'a'];
    return s_shiftKeyJamo[c - 'A'];
}

/* an rdr array's count, 0 for anything else or none (zui_skb.cpp's ZuiInit and CreateSoftKeyboard
 * call it) */
unsigned int SkbRdrCount(_zrdr *rdr)
{
    return rdr ? rdr->Count() : 0;
}

/* a jamo by its name, JAMO_COUNT for none (zui_skb.cpp's key parser 0x38DA70 calls it) */
JAMO FindJamo(const char *name)
{
    for (unsigned int i = 0; i < JAMO_COUNT; i++) {
        if (!SafeStrcasecmp(name, s_jamoNames[i]))
            return (JAMO)i;
    }
    return JAMO_COUNT;
}

/* an rdr array walked by index (the look parser's) */
struct CSkbRdrIter {
    /* the index is still inside the array */
    bool More() const;

    _zrdr *m_rdr;   // 0x0
    int m_index;   // 0x4
};

bool CSkbRdrIter::More() const
{
    return m_index < (int)SkbRdrCount(m_rdr);
}
