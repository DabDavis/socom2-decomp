/* zui_hangul.h -- the Korean input's composition types the keyboard units share (ours: the Korean
 * input unit, zui_hangul.cpp, defines their members; the keys unit, zui_skbkeys.cpp, types a jamo
 * into a CSkbPieces of its own, 0x399200). Moved out of zui_hangul.cpp in the UI lane's round 12. */
#ifndef ZUI_HANGUL_H
#define ZUI_HANGUL_H

#include <vector>
#include "zui_skb.h"

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

/* SOCOM II's MSL vectors here test `this` once more in their destructors than include/vector's do
 * (the static rule list's 0x3940F0: four tests where a pointer vector's 0x38D5C0 has three; the
 * index vectors inside 0x390690's and the completions' elements: four) -- a stand-in level (ours) */
template <class T>
class CSkbVec : public std::vector<T> {
};

/* (the split's parts: its resize(n) 0x391D90, a forwarder called out of line by the matcher's
 * one-level cut of CSkbSplit(n, k)) */
template <>
class CSkbVec<unsigned int> : public std::vector<unsigned int> {
public:
    using std::vector<unsigned int>::resize;
    void resize(unsigned int n) { resize(n, 0); }
    std::vector<unsigned int> &Vec() { return *this; }
    unsigned int &operator[](unsigned int n) { return Vec()[n]; }
};

/* a syllable's jamo (ours: five tests in its destructor 0x3912A0, its constructor 0x391280) */
class CJamoStr : public CSkbVec<char> {
};

/* a composed piece: its jamo and its character code (16 bytes; ours: the destructor 0x390760, six
 * tests; the code's own constructor 0x392760) */
struct SKBPIECE {
    ~SKBPIECE();

    CJamoStr m_jamo;   // 0x0
    CInit<unsigned short, 0> m_code;   // 0xc
};

/* a sequence of a production: the jamo it reads and the two values it adds (KoreanInputRules'
 * Seq/Val/Val2; ours, 20 bytes). The values are CInit<int, 0>s, as SKBPIECE's code is (the UI
 * lane's round 20): the vector's insert 0x393790 copy-constructs them word by word with lw/sw and
 * assigns them through an FPR each (lwc1/swc1), which plain ints (lw/sw both ways) and an int[2]
 * (block copies at both, a user copy constructor still pairing the loads) do not give */
struct SKBSEQ {
    SKBSEQ() {}
    /* (0x391E40: the composer's out-of-line copy) */
    int Size() { return m_jamo.size(); }

    std::vector<char> m_jamo;   // 0x0
    CInit<int, 0> m_val;   // 0xc
    CInit<int, 0> m_val2;   // 0x10
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

/* a jamo typed: composed with the jamo so far into pieces; whether it composed, and whether the
 * composition holds fewer jamo strings than pieces (zui_hangul.cpp's 0x390A30) */
std::pair<bool, bool> SkbHangulType(CSkbPieces *pieces, JAMO c);
/* the composition cleared (zui_hangul.cpp's 0x391350) */
void SkbHangulClear();

#endif
