/* zui_hangul.cpp -- the soft keyboard's Korean input (OVL1 0x3903A0-0x3941CF, its initialiser
 * 0x403770 building two vectors and a word at 0x49ED08-0x49ED40; its template tail ends on the two
 * vectors' destructors the initialiser registers, 0x3940F0 and 0x394170, so 0x3941D0-0x39441F --
 * CSkbKeyProps' members zui_skb.cpp calls, ~CSkbKeyProps 0x394370, SkbLoadUiParams -- open the
 * next unit, whichever file that is: SkbLoadUiParams stays here until its unit is written, as
 * nothing inlines it): the 33 jamo by name, the 2-set
 * keyboard's letters mapped to jamo (shifted and not), the syllable being composed, and the rdr
 * helpers the keyboard units share (an rdr array's count, an rdr array walked by index). SOCOM 1 has
 * none of it: the file's name and every name here are ours. Defined in reverse of their image order
 * (-inline deferred).
 *
 * The 1-byte pod vector<char> (include/vector, the MSL headers' round 9) gives the unit's seven
 * vector<char> instances byte-exact through the code below (insert 0x3923E0, resize 0x392600, erase
 * 0x392A30, the assignment 0x393D30, the copy 0x393D80, init 0x393F00, assign 0x393FB0), with the
 * sequences' resize and deleter, s_rules' and s_complete's destructors and the fixed array's members.
 *
 * THE DESTRUCTOR DEPTHS (not modelled, the MSL lane's note): the target's vector destructors here
 * test `this` once more than include/vector's (CSkbVec stands in for the level), and the piece's
 * destructor 0x390760 inlines its jamo string's whole chain -- six this-tests -- where here the chain
 * is a level too tall and is cut (~SKBPIECE calls ~CSkbVec<char>): PopPiece 0x390740, RemovePiece
 * 0x390730, the pieces' clear 0x3929C0 and deleter destructor 0x3928E0 keep their placeholders (their
 * drafts match), the pieces' and completions' inserts/erases/clears stay open with it; the
 * completions' element copies want the 4-byte pod's out-of-class operator= and assign (0x259AF0,
 * 0x259D20), which include/vector does not have.
 * OPEN, the initialiser 0x403770 (0x78 against 0x84): s_complete's type needs a level above CSkbVec
 * for its destructor 0x394170 (two this-tests, ~vector called) and none for its constructor (three
 * stores inline): CSkbCompleteVec gives the first and calls CSkbVec's constructor out of line; a
 * typedef gives the second and loses 0x394170 (a user-declared empty destructor: neither).
 * OPEN, not written yet: the recursive index stepper 0x3903A0, the composer 0x391670 and its rule
 * matcher 0x3919D0, 0x391E70 (a sum of an index vector's first n), 0x391F80 (the index walker's
 * constructor); everything they call out of line keeps its placeholder.
 * OPEN, written: SkbParseKoreanRules (0x4DC against 0x464), SkbParseComplete (0x318 against 0x2A4):
 * the rdr walks' loop counters (the target keeps the productions' in a stack slot) and the inlined
 * Item()s' shapes; SkbHangulType (0x180/0x174), SkbHangulRestart (0x1DC/0x1C4), SkbHangulBack
 * (0x18C/0x190), SkbHangulClear (0xF0/0xF8): the fixed array's Resize inlined one level in the
 * target (Count/Item/SetCount and the string's constructor and destructor called), here whole or
 * otherwise; Append (0x110/0x100), PrevCode (0x30/0x2C). */

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

/* SOCOM II's MSL vectors here test `this` once more in their destructors than include/vector's do
 * (the static rule list's 0x3940F0: four tests where a pointer vector's 0x38D5C0 has three; the
 * index vectors inside 0x390690's and the completions' elements: four) -- a stand-in level (ours) */
template <class T>
class CSkbVec : public std::vector<T> {
};

/* a syllable's jamo (ours: five tests in its destructor 0x3912A0, its constructor 0x391280) */
class CJamoStr : public CSkbVec<char> {
};

/* a composed piece: its jamo and its character code (16 bytes; ours: the destructor 0x390760, six
 * tests; the code's own constructor 0x392760) */
struct SKBPIECE {
    CJamoStr m_jamo;   // 0x0
    CInit<unsigned short, 0> m_code;   // 0xc
};

/* a sequence of a production: the jamo it reads and the two values it adds (KoreanInputRules'
 * Seq/Val/Val2; ours, 20 bytes) */
struct SKBSEQ {
    SKBSEQ() : m_val(0), m_val2(0) {}

    std::vector<char> m_jamo;   // 0x0
    int m_val;   // 0xc
    int m_val2;   // 0x10
};

/* a production: its name and its sequences (24 bytes, new'd by the rules' parser) */
struct SKBRULE {
    SKBSEQ *Seq(int i);

    CSaferStr m_name;   // 0x0
    std::vector<SKBSEQ> m_seqs;   // 0xc
};

/* a complete character's rule: the productions it is made of, by index, and whether it takes a
 * second value (CompleteCharacter's Seq and Val2; ours, 16 bytes) */
struct SKBCOMPLETE {
    SKBCOMPLETE() : m_val2(false) {}

    CSkbVec<unsigned int> m_rules;   // 0x0
    bool m_val2;   // 0xc
};

/* the complete characters (ours: its destructor 0x394170 calls ~vector<SKBCOMPLETE> 0x394090 out of
 * line after two this-tests, a level above CSkbVec's) */
class CSkbCompleteVec : public CSkbVec<SKBCOMPLETE> {
};

/* the pieces composed so far (a vector of SKBPIECE; the members ours) */
class CSkbPieces : public std::vector<SKBPIECE> {
public:
    void AddPiece();
    void RemovePiece();
    void PopPiece();
    unsigned short PrevCode();
    unsigned short LastCode();
    char LastJamo(int i);
    void Append(SKBSEQ &seq, bool second);
};

/* a fixed array built and destroyed in place (ours: the composition's single jamo string at
 * 0x49ED40 -- the count zeroed by the initialiser, nothing registered; its members out of line,
 * 0x391270-0x391340, called from the resizes inlined one level) */
template <class T, int N>
class CSkbFixedVec {
public:
    CSkbFixedVec() : m_count(0) {}
    int Count() const;
    T *Item(int i);
    void SetCount(int n);
    inline void Resize(int n);
    T &operator[](int i) { return ((T *)m_items)[i]; }

    int m_count;   // 0x0
    char m_items[N * sizeof(T)];   // 0x4
};

template <class T, int N>
int CSkbFixedVec<T, N>::Count() const
{
    return m_count;
}

template <class T, int N>
T *CSkbFixedVec<T, N>::Item(int i)
{
    return (T *)(m_items + i * sizeof(T));
}

template <class T, int N>
void CSkbFixedVec<T, N>::SetCount(int n)
{
    m_count = n;
}

template <class T, int N>
inline void CSkbFixedVec<T, N>::Resize(int n)
{
    if (Count() > n) {
        for (int i = n; i < Count(); i++)
            Item(i)->~T();
    } else {
        for (int i = Count(); i < n; i++)
            new (Item(i)) T();
    }
    SetCount(n);
}

/* the rules (KoreanInputRules' Productions), the complete characters, the jamo being composed */
static CSkbCompleteVec s_complete;   // 0x49ED08
static CSkbVec<SKBRULE *> s_rules;   // 0x49ED28
static CSkbFixedVec<CJamoStr, 1> s_compose;   // 0x49ED40

unsigned int SkbRdrCount(_zrdr *rdr);
JAMO FindJamo(const char *name);
_zrdr *zrdr_flat_findtag_startidx(const _zrdr *rdr, const char *tag, int start);
char *zrdr_flat_findstring(const _zrdr *rdr, const char *tag, int start);

/* the jamo composed into pieces by the rules (0x391670, not written yet: OPEN; the name ours) */
bool SkbHangulCompose(CSkbPieces *pieces, std::vector<char> *jamo);

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

/* the complete characters (CompleteCharacter: each a Seq of production names, by their index in the
 * rules, and a Val2 flag), the vector grown to each index read (0x392130; the name ours) */
void SkbParseComplete(CSkbCompleteVec *complete, _zrdr *rdr, const char *tag)
{
    _zrdr *list = zrdr_flat_findtag_startidx(rdr, tag, 0);

    for (int i = 0; i < (int)SkbRdrCount(list); i++) {
        _zrdr *seq = zrdr_flat_findtag_startidx(list->Item(i), "Seq", 0);

        for (int j = 0; j < (int)SkbRdrCount(seq); j++) {
            for (unsigned int r = 0; r < s_rules.size(); r++) {
                if (!SafeStrcasecmp(seq->Item(j)->String(), s_rules[r]->m_name.c_str())) {
                    unsigned int index = r;

                    if (i >= complete->size())
                        complete->resize(i + 1);
                    (*complete)[i].m_rules.push_back(index);
                    break;
                }
            }
        }
        if (zrdr_flat_findtag_startidx(list->Item(i), "Val2", 0)) {
            if (i >= complete->size())
                complete->resize(i + 1);
            (*complete)[i].m_val2 = true;
        }
    }
}

/* the jamo being composed set again from the last piece's (0x3914A0, after a composition; the name
 * ours) */
void SkbHangulRestart(CSkbPieces *pieces)
{
    s_compose.Resize(1);
    s_compose[0].resize(0, 0);
    for (int i = 0; i < (int)SkbPrev(pieces->end())->m_jamo.size(); i++)
        s_compose[0].push_back(pieces->LastJamo(i));
}

/* the jamo a key types: a lowercase letter's, or a shifted one's */
signed char KeyToJamo(char c)
{
    if (c >= 'a' && c <= 'z')
        return s_keyJamo[c - 'a'];
    return s_shiftKeyJamo[c - 'A'];
}

/* the composition dropped (0x391350; the name ours: zui_skbkeys.cpp's text functions call it) */
void SkbHangulClear()
{
    s_compose.Resize(0);
}

/* the last jamo typed taken back: none left ends the composition (0), else the character the rest
 * composes to (0x3910E0; the name ours: SkbTypeOrBack's) */
unsigned short SkbHangulBack()
{
    if (!(bool)s_compose.m_count)
        return 0;
    CJamoStr &jamo = s_compose[0];

    jamo.pop_back();
    if (jamo.size() == 0) {
        s_compose.Resize(0);
        return 0;
    }
    CSkbPieces pieces;

    SkbHangulCompose(&pieces, &jamo);
    unsigned short code = SkbPrev(pieces.end())->m_code;

    return code;
}

/* an rdr array's count, 0 for anything else or none (zui_skb.cpp's ZuiInit and CreateSoftKeyboard
 * call it) */
unsigned int SkbRdrCount(_zrdr *rdr)
{
    return rdr ? rdr->Count() : 0;
}

/* the Korean input rules (KoreanInputRules in the keyboards' rdr: each production new'd and listed
 * with its name and its sequences -- each sequence's jamo by name, its Val and Val2), then the
 * complete characters (0x390C40; the name ours: the display unit's 0x397070 calls it) */
void SkbParseKoreanRules(_zrdr *rdr)
{
    _zrdr *rules = zrdr_flat_findtag_startidx(rdr->Item(0), "KoreanInputRules", 0);
    _zrdr *prods = zrdr_flat_findtag_startidx(rules, "Productions", 0);

    for (int i = 0; i < (int)SkbRdrCount(prods); i++) {
        SKBRULE *rule = new SKBRULE;

        s_rules.push_back(rule);
        rule->m_name = zrdr_flat_findstring(prods->Item(i), "Name", 1);
        _zrdr *seqs = zrdr_flat_findtag_startidx(prods->Item(i), "Sequences", 0);

        for (int j = 0; j < (int)SkbRdrCount(seqs); j++) {
            rule->m_seqs.resize(rule->m_seqs.size() + 1);
            SKBSEQ &seq = rule->m_seqs.back();
            _zrdr *names = zrdr_flat_findtag_startidx(seqs->Item(j), "Seq", 0);

            for (int k = 0; k < (int)SkbRdrCount(names); k++)
                seq.m_jamo.push_back(FindJamo(names->Item(k)->String()));
            int val = 0;

            zrdr_findint(seqs->Item(j), "Val", &val, 1);
            seq.m_val = val;
            int val2 = 0;

            zrdr_findint(seqs->Item(j), "Val2", &val2, 1);
            seq.m_val2 = val2;
        }
    }
    SkbParseComplete(&s_complete, rules, "CompleteCharacter");
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

/* a jamo typed: composed with the jamo so far into pieces; whether it composed, and whether the
 * composition holds fewer jamo strings than pieces (0x390A30; the name ours: 0x399200 calls it) */
std::pair<bool, bool> SkbHangulType(CSkbPieces *pieces, char c)
{
    std::vector<char> jamo;

    if (s_compose.m_count > 0) {
        for (int i = 0; i < (int)s_compose[0].size(); i++)
            jamo.push_back(s_compose[0][i]);
    }
    jamo.push_back(c);
    if (!SkbHangulCompose(pieces, &jamo))
        return std::pair<bool, bool>(false, false);
    bool more = s_compose.m_count < (int)pieces->size();

    SkbHangulRestart(pieces);
    return std::pair<bool, bool>(true, more);
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

/* a production's sequence (0x3909A0) */
SKBSEQ *SKBRULE::Seq(int i)
{
    return &m_seqs[i];
}

/* a sequence's jamo appended to the last piece, its value (or its second) added to the code
 * (0x3908A0) */
void CSkbPieces::Append(SKBSEQ &seq, bool second)
{
    for (int i = 0; i < (int)seq.m_jamo.size(); i++)
        SkbPrev(end())->m_jamo.push_back(seq.m_jamo[i]);
    const int &val = second ? seq.m_val2 : seq.m_val;

    SkbPrev(end())->m_code = SkbPrev(end())->m_code + val;
}

/* the last piece's jamo i (0x390860) */
char CSkbPieces::LastJamo(int i)
{
    return SkbPrev(end())->m_jamo[i];
}

/* the last piece's code (0x390820) */
unsigned short CSkbPieces::LastCode()
{
    return SkbPrev(end())->m_code;
}

/* the code of the piece before the last, 0 for none (0x3907F0) */
unsigned short CSkbPieces::PrevCode()
{
    return (int)size() >= 2 ? (unsigned short)(*this)[size() - 2].m_code : 0;
}

/* the last piece destroyed (0x390740) */
void CSkbPieces::PopPiece()
{
    (data() + --size_)->~SKBPIECE();
}

/* (0x390730) */
void CSkbPieces::RemovePiece()
{
    PopPiece();
}

/* an empty piece added (0x390720) */
void CSkbPieces::AddPiece()
{
    resize(size() + 1);
}
