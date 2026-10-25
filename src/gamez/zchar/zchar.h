/* zchar.h -- the character data (OVL2): the vehicle file's entries (CVehicleRdr, the SEALs and
 * tangos a mission places) and the character types (CCharacterType, char_type.cpp in the press
 * beta's asserts). The layouts are SOCOM 1's DWARF moved where SOCOM II's code puts them. The
 * directory is ours: SOCOM 1's DWARF names neither class's file. */
#ifndef ZCHAR_H
#define ZCHAR_H

#include <vector>

class CVehicleRdrEntry;
struct _zrdr;

/* how a character behaves (SOCOM 1's DWARF, 0x3C bytes; SOCOM II's vehicle entry keeps it whole,
 * 0x120-0x15B) */
class AI_PARAMS {
public:
    void Parse(_zrdr *rdr);

    unsigned int mask;   // 0x0
    float accuracy;   // 0x4
    float courage;   // 0x8
    float surrender;   // 0xc
    float stealth;   // 0x10
    float training;   // 0x14
    float visual_sens;   // 0x18
    float visual_range;   // 0x1c
    float visual_fov;   // 0x20
    float aware_decay;   // 0x24
    float thip_range;   // 0x28
    unsigned int respawn;   // 0x2c
    float target_delay;   // 0x30
    float target_discipline;   // 0x34
    CVehicleRdrEntry *m_veh;   // 0x38
};

/* a starting point: a map location's name and a direction (SOCOM 1's DWARF) */
class start {
public:
    char m_name[16];   // 0x0
    int m_dir;   // 0x10
};

/* up to four of them (SOCOM 1's DWARF) */
class startvec {
public:
    unsigned int m_count;   // 0x0
    start m_start[4];   // 0x4

    /* a start's name and direction by index, or none (the names are ours: the four getters
     * inline them; the result through a local, or the index and count take other registers) */
    const char *GetName(unsigned int i) const
    {
        const char *name = 0;

        if (i < m_count)
            name = m_start[i].m_name;
        return name;
    }
    int GetDir(unsigned int i) const
    {
        int dir = 0;

        if (i < m_count)
            dir = m_start[i].m_dir;
        return dir;
    }
};

/* one character a mission places (SOCOM 1's DWARF, 0x10C bytes; SOCOM II's is 0x160: a word at
 * 0x70, a second set of starts for respawning, the start index packed into the flags word) */
class CVehicleRdrEntry {
public:
    const char *GetStartPos() const;
    int GetStartDir() const;
    /* the respawning set's (new in SOCOM II; the names are ours) */
    const char *GetRespawnPos() const;
    int GetRespawnDir() const;
    void SetCharacterName(const char *name);

    char m_name[32];   // 0x0
    char m_character[48];   // 0x20
    char m_display_name[32];   // 0x50
    unsigned int m_unk70;   // 0x70
    startvec m_start;   // 0x74
    startvec m_respawn;   // 0xc8: new in SOCOM II ("respawn_setup")
    int m_teamMask;   // 0x11c
    AI_PARAMS m_aiParams;   // 0x120
    unsigned short m_debug : 1;   // 0x15c: a halfword of flags in SOCOM II (lhu, no masking)
    unsigned short m_disabled : 1;
    unsigned short m_nofade : 1;
    unsigned short m_noshoot : 1;
    unsigned short m_recycle : 1;
    unsigned short m_nosnooze : 1;
    unsigned short m_startIndex : 4;   // SOCOM 1's m_startIndex, a word at 0x108 there
    unsigned short m_respawnIndex : 4;   // new (the name is ours)
    unsigned short m_unused : 2;
};

/* the vehicle file: every entry (SOCOM 1's DWARF: a vector and nothing more) */
class CVehicleRdr : public std::vector<CVehicleRdrEntry> {
public:
    void Load(_zrdr *rdr);
    CVehicleRdrEntry *GetEntryByName(const char *name) const;
    void Open(const char *file);
    void Close();
};

#endif
