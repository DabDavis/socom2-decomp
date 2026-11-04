/* zsnd_subtitles.h -- the subtitle table (zsnd_subtitles.cpp; SOCOM II's, the file, class and member
 * names ours): two vectors of 12-byte records looked up by a string's hash. zsnd_csnd.cpp defines
 * the global (0x48DC30) its initialiser builds, and its tail holds the table's implicit destructor
 * (0x346160) with the records' deleter pieces. */
#ifndef ZSND_SUBTITLES_H
#define ZSND_SUBTITLES_H

#include <vector>

/* a record (ours): two words given back and the hash it is found by */
struct CSubtitleEntry {
    int m_0;   // 0x0
    int m_4;   // 0x4
    unsigned int m_hash;   // 0x8
};   // 0xc bytes

/* a vector of records (ours: a class over std::vector -- the table's implicit destructor tests
 * each member three times before the deleter's destructor, a level more than a plain vector) */
class CSubtitleEntryVec : public std::vector<CSubtitleEntry> {
};

class CSubtitleTable {
public:
    CSubtitleTable();
    void Dump();
    bool Find(const char *name, int *a, int *b);

    CSubtitleEntryVec m_entries;   // 0x0: the lookup's records (10 reserved)
    CSubtitleEntryVec m_lines;   // 0xc: 11500 reserved
    int m_18;   // 0x18
    int m_1c;   // 0x1c
    unsigned char m_20;   // 0x20
};

#endif
