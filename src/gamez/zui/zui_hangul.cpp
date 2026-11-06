/* zui_hangul.cpp -- the soft keyboard's Korean input (OVL1 0x3903A0-0x3941CF, its initialiser
 * 0x403770 building two vectors and a word at 0x49ED08-0x49ED40; its template tail ends on the two
 * vectors' destructors the initialiser registers, 0x3940F0 and 0x394170, so 0x3941D0-0x39441F --
 * CSkbKeyProps' members zui_skb.cpp calls, ~CSkbKeyProps 0x394370, SkbLoadUiParams 0x3943E0 -- open
 * the next unit, zui_skbview.cpp (the members since the UI lane's round 20, SkbLoadUiParams since
 * round 21: its two strings sit in that unit's data, 0x3FB398/0x3FB3A8 between s_padKeys 0x3FB388
 * and 0x394DA0's valve name 0x3FB3B8)): the 33 jamo by name, the 2-set
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
 * typedef gives the second and loses 0x394170 (a user-declared empty destructor: neither). A LEVEL
 * CUT (round 17, measured): at -inline level=6 the unit as it stands gives the initialiser and
 * resize<SKBCOMPLETE> 0x392770 byte-exact and loses CInit<Us,0>() 0x392760 (inlined everywhere, no
 * copy left) -- 44 against 43 of the unit's rows; CSkbCompleteVec() is six high here (itself,
 * CSkbVec's, vector's, __vector_imp's, the deleter's and the capacity's constructors) and the target
 * inlines it whole, while its destructor keeps the two this-tests and ~vector's call.
 * OPEN, not written yet: the composer 0x391670 and its rule matcher 0x3919D0; everything they call
 * out of line keeps its placeholder or its draft. The recursive index stepper 0x3903A0 is written
 * since round 25 (CSkbSplit::Next, OPEN on a level: its note). The
 * split's sum 0x391E70 is written since round 24 (OPEN, 12: CSkbSplit::Sum's note). Round 23 wrote the split (CSkbSplit:
 * its constructor 0x391F80, Part 0x391E50) and a sequence's jamo read 0x391E20, byte-exact; the
 * composer and matcher call the parts' operator[] 0x391D80, end() 0x391D40 (the scalar vector's
 * class end(), out of line: only the matcher's call emits it), the reverse iterator's operator*
 * 0x391D30 and the vector's constructor 0x391DA0 out of line, and keep their counters in stack
 * slots past nine saved registers.
 * THE TWO RDR PARSERS, byte-exact since round 38 (SkbParseKoreanRules under `opt_loop_invariants off`,
 * SkbParseComplete with strength reduction off as well: their notes -- the pragma scan named the pass;
 * the unit's template tail wants the loop optimiser on, measured at the unit's end: 3 lost, 0 gained).
 * Written: SkbHangulBack (0x19C/0x190 -- byte-exact since the GCC front's round-43a pragma
 * scan under `#pragma opt_common_subs off`: with CSE on, the local pieces' size_ and data_ addresses,
 * stored by the constructor and read by end(), were kept in s0/s1 across the composer's call where
 * the target addresses the stack slots directly); Append (1 instruction, below), PrevCode (0x30/0x2C).
 * Round 13: the fixed array's Resize tests `n < Count()` (slt/beqz; `Count() > n` branches on blez
 * for a constant n) -- SkbHangulClear and SkbHangulRestart byte-exact, the latter with LastJamo
 * pinned (its one caller calls it); SkbHangulType copies each jamo through `(char)` (a push_back
 * argument the target stores to a stack byte) off a reference to the string. */

#include <string.h>
#include "zui.h"
#include "zui_skb.h"
#include "zui_hangul.h"

/* the 2-set keyboard: the jamo each letter key types, unshifted (a-z) and shifted (A-Z) */
static const JAMO s_keyJamo[26] = {
    JAMO_MIEUM, JAMO_YU, JAMO_CHIEUCH, JAMO_IEUNG, JAMO_TIKEUT, JAMO_RIEUL, JAMO_HIEUH, JAMO_O,
    JAMO_YA, JAMO_EO, JAMO_A, JAMO_I, JAMO_EU, JAMO_U, JAMO_AE, JAMO_E,
    JAMO_PIEUP, JAMO_KIYEOK, JAMO_NIEUN, JAMO_SIOS, JAMO_YEO, JAMO_PHIEUPH, JAMO_CIEUC, JAMO_THIEUTH,
    JAMO_YO, JAMO_KHIEUKH
};
static const JAMO s_shiftKeyJamo[26] = {
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

/* a production: its name and its sequences (24 bytes, new'd by the rules' parser) */
struct SKBRULE {
    SKBSEQ *Seq(int i);
    /* a sequence added and returned (an inline: the address-taken `rule` it is called on is not
     * substituted for `this`, so the parser keeps the rule in a saved register here, s0) */
    SKBSEQ &AddSeq()
    {
        m_seqs.resize(m_seqs.size() + 1);
        return m_seqs.back();
    }

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
    /* none built (ours: SkbHangulBack materialises the test, sltu/xori, as only an inline bool
     * returning the negation gives) */
    bool Empty() const { return !(bool)m_count; }

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
    if (n < Count()) {
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

/* the complete characters (CompleteCharacter: each a Seq of production names, by their index in the
 * rules, and a Val2 flag), the vector grown to each index read (0x392130; the name ours).
 * BYTE-EXACT WITH THE LOOP OPTIMISER OFF (round 38; the GCC front's round-43a pragma scan named the
 * pass): with both opt_loop_invariants and opt_strength_reduction off IRO_FindLoops never runs
 * (IrOptimizer.c), so the rules' s_rules[r] is computed afresh each pass and no count hoisted, as the
 * target's; the five locals declared bare at the top give its numbering (r s4, seq s3, list s2, i s1,
 * j s0 -- 40 words of pure register permutation otherwise) */
#pragma opt_loop_invariants off
#pragma opt_strength_reduction off
void SkbParseComplete(CSkbCompleteVec *complete, _zrdr *rdr, const char *tag)
{
    unsigned int r;
    _zrdr *seq;
    _zrdr *list;
    int i;
    int j;

    list = zrdr_flat_findtag_startidx(rdr, tag, 0);

    for (i = 0; i < (int)SkbRdrCount(list); i++) {
        seq = zrdr_flat_findtag_startidx(list->Item(i), "Seq", 0);

        for (j = 0; j < (int)SkbRdrCount(seq); j++) {
            for (r = 0; r < s_rules.size(); r++) {
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
#pragma opt_loop_invariants reset
#pragma opt_strength_reduction reset

/* a split of n jamo into k consecutive parts, the composer's walk over every split (ours, 16
 * bytes: the total, then the parts). The stepper Next 0x3903A0 (recursive) is written at the unit's
 * end (OPEN, the level); not written: the destructor 0x390690 (its draft matches) and the
 * out-of-line copy of Left 0x391DF0 (only the composer calls it); the sum of the first n parts
 * 0x391E70 is written below (OPEN). */
class CSkbSplit {
public:
    CSkbSplit() : m_n(0) {}
    CSkbSplit(int n, int k);
    unsigned int Sum(int n);
    unsigned int Part(int i);
    void Next();
    /* the parts through a reference: the walks' vector address hoisted (ours) */
    std::vector<unsigned int> &Parts() { return m_parts; }
    /* a split is left (0x391DF0, the composer's out-of-line copy; the stepper inlines it) */
    bool Left() { return m_parts.size() != 0; }

    int m_n;   // 0x0
    std::vector<unsigned int> m_parts;   // 0x4
};

/* the first split of n into k parts: k - 1 of one and the rest last, none when k is out of 1..n
 * (0x391F80; ours). Byte-exact (the UI's round 23): the test `k > n || k <= 0` (`n < k` puts the
 * compare in v0), the last part stored through the parts' reverse iterator (include/vector's
 * rbegin() over std::reverse_iterator since the MSL headers' round 23, retiring the unit's stand-in:
 * two stack temporaries and an FPR copy, as the scalar vector's class end() and __iter give; the
 * matcher 0x3919D0 calls that end() 0x391D40 and the reverse * 0x391D30 out of line) */
CSkbSplit::CSkbSplit(int n, int k) : m_n(n)
{
    if (k > n || k <= 0)
        return;
    Parts().resize(k, 0);
    for (int i = 0; i < k - 1; i++)
        Parts()[i] = 1;
    *m_parts.rbegin() = n - k + 1;
}

/* the sum of the split's first n parts (0x391E70; the composer calls it out of line; the name
 * ours). MW unrolls the loop by 8.
 * OPEN, 12 instructions (the target's 0xD0): a t3/t4/t5 rotation of three loop temporaries. The
 * size needs the parts read through a reference-returning accessor in the body (`Parts()[i]`: the
 * vector's address hoisted into the test block laid after the remainder loop, the UI's round-22
 * preheader rule; `m_parts[i]` or a reference local is 0xB0/0xB8). MW's colouring (igdump, round
 * 24) numbers the unroller's bound n - 8 first, the hoisted data pointer next and the element
 * address last (t5, t4, t3); the target's registers (t4, t3, t5) want the element address numbered
 * FIRST -- the operator[]'s reference temporary, which here is created by the inliner (just after
 * the Parts() result) and dies folded, would be. Tried (round 23): the counter's declaration, the
 * sum's form, a while loop, CSkbVec or unsigned parts; (round 24) the element through a reference
 * or a value local, a pointer local, `(unsigned int)`, `s = s + x` and `s = x + s`, an int sum, an
 * accessor returning the part by value, by reference or by const reference through Parts(), a
 * named reference or a pointer inside it, a vector reference local inside the loop, the pointer
 * form `(*PParts())[i]`: all 12; an unsigned counter 27, `*(begin() + i)` 53. Round 35: igreplay
 * --movers puts the element address below the unroller's bound (36.5-37.5) and the hoisted data
 * pointer after every other temporary (50.5 on) -- the optimiser's own temporaries; `#pragma
 * opt_propagation off` in the body (the kit's round 56) 13, opt_common_subs off 0x108,
 * opt_loop_invariants off 0xB0, opt_strength_reduction / opt_lifetimes off 12 (build/scratch/ui/
 * v_sum.py). */
unsigned int CSkbSplit::Sum(int n)
{
    unsigned int s = 0;

    for (int i = 0; i < n; i++)
        s += Parts()[i];
    return s;
}

/* a split's part i (0x391E50; the composer calls it out of line; ours) */
unsigned int CSkbSplit::Part(int i)
{
    return m_parts[i];
}

/* a sequence's jamo i, sign-extended (0x391E20; the composer calls it out of line; ours) */
char SkbSeqJamo(SKBSEQ *seq, int i)
{
    return seq->m_jamo[i];
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
JAMO KeyToJamo(char c)
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
 * composes to (0x3910E0; the name ours: SkbTypeOrBack's). Byte-exact under `#pragma opt_common_subs
 * off` in the body (the GCC front's round-43a pragma scan, applied by the integrator): with CSE on MW
 * keeps the local pieces' size_ and data_ addresses in s0/s1 across the composer's call, where the
 * target addresses the stack slots directly (28 off) */
unsigned short SkbHangulBack()
{
#pragma opt_common_subs off
    if (s_compose.Empty())
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
#pragma opt_common_subs reset

/* an rdr array's count, 0 for anything else or none (zui_skb.cpp's ZuiInit and CreateSoftKeyboard
 * call it) */
unsigned int SkbRdrCount(_zrdr *rdr)
{
    return rdr ? rdr->Count() : 0;
}

/* a sequence's jamo list held in a one-member object (ours, A MODELLING SHAPE: MW's class scalariser
 * makes the member a temporary numbered after the inlined FindJamo's counter, as the target has it --
 * names s1 under that counter's s2; a plain pointer local, however declared or defined, is numbered
 * ahead of every inline's variable) */
struct CSkbRdrHolder {
    CSkbRdrHolder(_zrdr *rdr) : m_rdr(rdr) {}

    _zrdr *m_rdr;
};

/* the Korean input rules (KoreanInputRules in the keyboards' rdr: each production new'd and listed
 * with its name and its sequences -- each sequence's jamo by name, its Val and Val2), then the
 * complete characters (0x390C40; the name ours: the display unit's 0x397070 calls it).
 * BYTE-EXACT UNDER `opt_loop_invariants off` (round 38; open at 205 off since round 10, the pragma scan
 * named the pass -- the rdr walks' Item() tests and the counts stay in the loops, the target's stack
 * slot for i the spill that follows): rule->AddSeq() is an inline over the address-taken `rule`, which
 * keeps its value in s0; the Val and Val2 records are read into named locals first, so each `= 0` store
 * lands in zrdr_findint's delay slot and the frame keeps val, val2 above the name's iterator tag; name
 * and k declared before the jamo list */
#pragma opt_loop_invariants off
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
            SKBSEQ &seq = rule->AddSeq();
            const char *name;
            int k;
            CSkbRdrHolder names(zrdr_flat_findtag_startidx(seqs->Item(j), "Seq", 0));

            for (k = 0; k < (int)SkbRdrCount(names.m_rdr); k++) {
                name = names.m_rdr->Item(k)->String();
                seq.m_jamo.push_back(FindJamo(name));
            }
            _zrdr *item = seqs->Item(j);
            int val = 0;

            zrdr_findint(item, "Val", &val, 1);
            seq.m_val = val;
            _zrdr *item2 = seqs->Item(j);
            int val2 = 0;

            zrdr_findint(item2, "Val2", &val2, 1);
            seq.m_val2 = val2;
        }
    }
    SkbParseComplete(&s_complete, rules, "CompleteCharacter");
}
#pragma opt_loop_invariants reset

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
std::pair<bool, bool> SkbHangulType(CSkbPieces *pieces, JAMO c)
{
    std::vector<char> jamo;

    if (s_compose.m_count > 0) {
        CJamoStr &s = s_compose[0];

        for (int i = 0; i < (int)s.size(); i++)
            jamo.push_back((char)s[i]);
    }
    jamo.push_back((char &)c);   /* (the parameter's own frame byte, `sb a2,127(sp)`: no temporary) */
    if (!SkbHangulCompose(pieces, &jamo))
        return std::pair<bool, bool>(false, false);
    bool more = s_compose.m_count < (int)pieces->size();

    SkbHangulRestart(pieces);
    return std::pair<bool, bool>(true, more);
}

/* (0x3909F0; the look parser calls it out of line) */
CSkbRdrIter::CSkbRdrIter(_zrdr *rdr, const char *tag) : m_rdr(zrdr_flat_findtag_startidx(rdr, tag, 0)), m_index(0)
{
}

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
 * (0x3908A0). Each jamo pushed as a copy (`(char)`: the target stores it to a stack byte of its own
 * after the piece's address is made, through a reference local), the code updated through one
 * reference to the last piece, its value narrowed (lhu). The code is added to through CInit's
 * `operator+=(const T &)`: the value converted and bound first, then `this`, then `m_val + x` --
 * the target's `addu v1,v1,a1`, the code first with the value numbered ahead of it (round 19; a
 * sum through the conversion operator T() makes the code a bare temporary, so the value came
 * first in the add whichever side it was written on, and a by-value `operator+=(T)` numbers the
 * value after the code) */
void CSkbPieces::Append(SKBSEQ &seq, bool second)
{
    for (int i = 0; i < (int)seq.m_jamo.size(); i++) {
        SKBPIECE &p = *SkbPrev(end());

        p.m_jamo.push_back((char)seq.m_jamo[i]);
    }
    const int &val = second ? seq.m_val2 : seq.m_val;
    SKBPIECE &last = *SkbPrev(end());

    last.m_code += val;
}

/* the last piece's jamo i (0x390860; SkbHangulRestart, its one caller, calls it out of line:
 * pinned) */
#pragma auto_inline off
char CSkbPieces::LastJamo(int i)
{
    return SkbPrev(end())->m_jamo[i];
}
#pragma auto_inline reset

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

/* the next split of the same total into as many parts, none (the parts cleared) after the last
 * (0x3903A0; the composer's walk; the name ours): the split of the rest after the first part stepped
 * by itself (recursively) and copied back, or -- that one done -- the first part grown by one with
 * the others reset to one each and the rest last, while the first part leaves room.
 * OPEN (UI round 25, written), 2 instructions short and one register pair off -- every block, call,
 * loop and saved register the target's (s0 the local's parts, s1 k, s2 this, s3 &m_parts.data_):
 *   - A LEVEL: the target's inlined destructor of the local split tests the parts' address FOUR
 *     times (as the out-of-line ~CSkbSplit 0x390690 does), the unit's CSkbVec level; with
 *     `CSkbVec<unsigned int> m_parts` the chain is six high (~CSkbSplit, ~CSkbVec, ~vector,
 *     ~__vector_imp, ~__vector_pod, deallocate) and at -inline level=5 ~CSkbVec is called out of
 *     line (0x2BC), so std::vector stays here (three tests, the rest of the unit alike either way);
 *   - the last part's rbegin() reads the data pointer through the parts' data() reference, which
 *     MW shares with the m_parts[0] reads (s3) -- the target reads it straight (`lw a2,12(s2)`) and
 *     loads it again through s3 after the iterator's FPR copy: the original scalar end() reads data_
 *     itself (include/vector's goes through the pod's end and data(), which the pointer vectors'
 *     hoisted ends need -- the MSL's to weigh).
 * The local's parts through a reference local (their address kept in s0 for the field reads, as a
 * reference to a stack object does; through sub.Parts() MW hoists sp-relative field addresses and
 * strength-reduces the copies), int counters in the copies (unsigned ones are strength-reduced), and
 * the room test `(int)m_parts[0] <= m_n - k` (branching on `slt at`; `m_n - k >= ...` puts the compare
 * in a register) are the target's */
void CSkbSplit::Next()
{
    int k = m_parts.size();

    if (k < 2) {
        m_parts.clear();
        return;
    }
    CSkbSplit sub;
    std::vector<unsigned int> &parts = sub.m_parts;

    sub.m_n = m_n - m_parts[0];
    parts.resize(k - 1, 0);
    for (int i = 0; i < parts.size(); i++)
        parts[i] = Parts()[i + 1];
    sub.Next();
    if (sub.Left()) {
        for (int i = 0; i < parts.size(); i++)
            Parts()[i + 1] = parts[i];
    } else if ((int)m_parts[0] <= m_n - k) {
        m_parts[0]++;
        for (int i = 1; i < k - 1; i++)
            Parts()[i] = 1;
        *m_parts.rbegin() = m_n - m_parts[0] - (k - 2);
    } else {
        m_parts.clear();
    }
}
