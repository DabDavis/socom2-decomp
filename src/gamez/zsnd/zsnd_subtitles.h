/* zsnd_subtitles.h -- the subtitle table (zsnd_subtitles.cpp; SOCOM II's, the file, class and member
 * names ours): two vectors of 12-byte records looked up by a string's hash. zsnd_csnd.cpp defines
 * the global (0x48DC30) its initialiser builds, and its tail holds the table's implicit destructor
 * (0x346160) with the records' deleter pieces. */
#ifndef ZSND_SUBTITLES_H
#define ZSND_SUBTITLES_H

#include <vector>

/* a record (ours): two words given back and the hash it is found by */
struct CSubtitleEntry {
    CSubtitleEntry() : m_hash(0) {}
    CSubtitleEntry(const CSubtitleEntry &e) : m_0(e.m_0), m_4(e.m_4), m_hash(e.m_hash) {}

    int m_0;   // 0x0
    int m_4;   // 0x4
    unsigned int m_hash;   // 0x8
};   // 0xc bytes

/* the records, searched by hash: a class over std::vector -- the table's implicit destructor tests
 * each member three times before the deleter's destructor, a level more than a plain vector, and
 * the table's lookup (0x34D480) calls Find on each member (this + 0xC, then this) */
namespace zar {
class CZAR;
}

class CSubtitleList : public std::vector<CSubtitleEntry> {
public:
    bool Find(const char *name, int *a, int *b);
    void Dump();
    /* the open key's files walked (directories descended), each one's offset, size and name's
     * hash pushed (0x34D5D0, not written: it walks the key ring's vector iterator through zar's
     * out-of-line pieces 0x257B90-0x257CB0, unnamed yet, and the recursive next-file walk 0x34D740) */
    bool Load(zar::CZAR *zar);
};

/* the game's languages (util_loc.cpp's names; a byte under -enum min) */
enum LANGUAGE {
    LANG_STATES,
    LANG_UK,
    LANG_FRANCE,
    LANG_GERMANY,
    LANG_SPAIN,
    LANG_ITALY,
    LANG_JAPAN,
    LANG_KOREA,
    LANG_TEST,
    LANG_COUNT,
    LANG_UNSET = 99
};

class CSubtitleTable {
public:
    CSubtitleTable();
    /* the language's VAG index and VAGSTORE's read off the disc (0x34D190) */
    bool Load(LANGUAGE lang);
    /* the first load, English (0x34D180, a tail call: CSnd::Init's; the name ours) */
    void Init();

    CSubtitleList m_entries;   // 0x0: the second lookup's records (10 reserved)
    CSubtitleList m_lines;   // 0xc: the first lookup's, 11500 reserved
    int m_18;   // 0x18
    int m_1c;   // 0x1c
    unsigned char m_20;   // 0x20
};

#endif
