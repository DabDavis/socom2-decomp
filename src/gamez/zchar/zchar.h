/* zchar.h -- the character data (OVL2): the vehicle file's entries (CVehicleRdr, the SEALs and
 * tangos a mission places) and the character types (CCharacterType, char_type.cpp in the press
 * beta's asserts). The layouts are SOCOM 1's DWARF moved where SOCOM II's code puts them. The
 * directory is ours: SOCOM 1's DWARF names neither class's file. */
#ifndef ZCHAR_H
#define ZCHAR_H

#include <vector>

#include "../zweapon/zweapon.h"

class CVehicleRdrEntry;
struct _zrdr;
class CRdrFile;

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
    void Randomize(bool reset);
    /* one entry out of the file's item (SOCOM 1's CVehicleRdr::Load did every entry; the name is
     * ours) */
    void Load(_zrdr *rdr);

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
    CVehicleRdrEntry *GetEntryByName(const char *name) const;
    bool Open(const char *file);
    void Close();
};

/* a character's sounds (SOCOM 1's CHARACTER_SOUND, a signed byte: CHRSND_NONE -1; SOCOM II has 85
 * where SOCOM 1 had 66). The enumerators are GetCharacterSoundTypeString's names, each at the value
 * its jump table gives it. zseal.h's stand-in (CHARACTER_SOUND_NONE/LAST) predates this */
enum CHARACTER_SOUND {
    CHRSND_NONE = -1,
    CHRSND_ABORT_PLAYER_TOO_FAR = 0,
    CHRSND_WAITING_FOR_PLAYER = 1,
    CHRSND_PLAYER_WARN_TOO_FAR = 2,
    CHRSND_AREA_CLEARED = 3,
    CHRSND_BREACH_COMPLETE = 4,
    CHRSND_INTIMIDATE = 5,
    CHRSND_CLEARING_BABBLE = 6,
    CHRSND_GOTO_COMPLETE = 7,
    CHRSND_LASE_STARTED = 8,
    CHRSND_LASE_COMPLETE = 9,
    CHRSND_BRAVO_IS_DISTANT = 10,
    CHRSND_DEFUSE_BOMB_COMPLETE = 11,
    CHRSND_MOVING_OUT = 12,
    CHRSND_UNDER_FIRE = 13,
    CHRSND_TANGO_DOWN = 14,
    CHRSND_TANGO_ONE_OCLOCK = 15,
    CHRSND_TANGO_TWO_OCLOCK = 16,
    CHRSND_TANGO_THREE_OCLOCK = 17,
    CHRSND_TANGO_FOUR_OCLOCK = 18,
    CHRSND_TANGO_FIVE_OCLOCK = 19,
    CHRSND_TANGO_SIX_OCLOCK = 20,
    CHRSND_TANGO_SEVEN_OCLOCK = 21,
    CHRSND_TANGO_EIGHT_OCLOCK = 22,
    CHRSND_TANGO_NINE_OCLOCK = 23,
    CHRSND_TANGO_TEN_OCLOCK = 24,
    CHRSND_TANGO_ELEVEN_OCLOCK = 25,
    CHRSND_TANGO_TWELVE_OCLOCK = 26,
    CHRSND_TANGO_ONE_OCLOCK_HIGH = 27,
    CHRSND_TANGO_TWO_OCLOCK_HIGH = 28,
    CHRSND_TANGO_THREE_OCLOCK_HIGH = 29,
    CHRSND_TANGO_FOUR_OCLOCK_HIGH = 30,
    CHRSND_TANGO_FIVE_OCLOCK_HIGH = 31,
    CHRSND_TANGO_SIX_OCLOCK_HIGH = 32,
    CHRSND_TANGO_SEVEN_OCLOCK_HIGH = 33,
    CHRSND_TANGO_EIGHT_OCLOCK_HIGH = 34,
    CHRSND_TANGO_NINE_OCLOCK_HIGH = 35,
    CHRSND_TANGO_TEN_OCLOCK_HIGH = 36,
    CHRSND_TANGO_ELEVEN_OCLOCK_HIGH = 37,
    CHRSND_TANGO_TWELVE_OCLOCK_HIGH = 38,
    CHRSND_TANGO_MOVING = 39,
    CHRSND_DEPLOY_GENERIC_COMPLETE = 40,
    CHRSND_DEPLOY_BLUE_CHEM_COMPLETE = 41,
    CHRSND_DEPLOY_RED_SMOKE_COMPLETE = 42,
    CHRSND_DEPLOY_C4 = 43,
    CHRSND_SATCHEL_PLACED = 44,
    CHRSND_DEPLOY_FRAG_COMPLETE = 45,
    CHRSND_DEPLOY_SMOKE_COMPLETE = 46,
    CHRSND_DEPLOY_FLASHBANG = 47,
    CHRSND_DEPLOY_TOOFAR = 48,
    CHRSND_WAIT_UP = 49,
    CHRSND_DEPLOY_TOO_FAR = 50,
    CHRSND_GRENADE_ALERT = 51,
    CHRSND_GRENADE_I_GOT_IT = 52,
    CHRSND_GRENADE_OH_HECK = 53,
    CHRSND_COVER_CANT_SEE = 54,
    CHRSND_YES = 55,
    CHRSND_NO = 56,
    CHRSND_TEAMMATE_DOWN = 57,
    CHRSND_PLAYER_DOWN = 58,
    CHRSND_DAMAGE = 59,
    CHRSND_DEATH = 60,
    CHRSND_DEATH_QUIET = 61,
    CHRSND_ENEMY_SIGHTED = 62,
    CHRSND_ENEMY_AWARE = 63,
    CHRSND_ENEMY_HOSTILE = 64,
    CHRSND_ENEMY_CLOSE = 65,
    CHRSND_ENGAGING = 66,
    CHRSND_NOAMMO = 67,
    CHRSND_RELOAD = 68,
    CHRSND_SUSPICIOUS = 69,
    CHRSND_SURPRISED = 70,
    CHRSND_BATTLE_CRY = 71,
    CHRSND_PANIC_CRY = 72,
    CHRSND_HELPME = 73,
    CHRSND_SURRENDER = 74,
    CHRSND_RESTRAINED = 75,
    CHRSND_THANKYOU = 76,
    CHRSND_GREETING = 77,
    CHRSND_STATUS_OK = 78,
    CHRSND_STATUS_HOT = 79,
    CHRSND_PLAYER_IDLE = 80,
    CHRSND_FRIENDLYFIRE = 81,
    CHRSND_TURNCOAT = 82,
    CHRSND_INPOSITION = 83,
    CHRSND_NOTSURE = 84,
    CHRSND_MAX = 85
};

const char *GetCharacterSoundTypeString(CHARACTER_SOUND s);

class CCharTypeVec;

/* a part's health and armour (SOCOM 1's DWARF, 0x48 bytes) */
class HEALTH_PARAMS {
public:
    void Parse(_zrdr *rdr);

    float m_head_health;   // 0x0
    float m_head_healthMax;   // 0x4
    float m_body_health;   // 0x8
    float m_body_healthMax;   // 0xc
    float m_larm_health;   // 0x10
    float m_larm_healthMax;   // 0x14
    float m_rarm_health;   // 0x18
    float m_rarm_healthMax;   // 0x1c
    float m_lleg_health;   // 0x20
    float m_lleg_healthMax;   // 0x24
    float m_rleg_health;   // 0x28
    float m_rleg_healthMax;   // 0x2c
    float m_armor_head;   // 0x30
    float m_armor_body;   // 0x34
    float m_armor_rarm;   // 0x38
    float m_armor_larm;   // 0x3c
    float m_armor_rleg;   // 0x40
    float m_armor_lleg;   // 0x44
};

class CSnd;
class CModel;

/* a weapon a type carries, with its ammunition (SOCOM 1's DWARF, 0xC bytes) */
class CCharacterWeap {
public:
    EQUIP_ITEM m_wepId;   // 0x0
    EQUIP_AMMO m_ammoId;   // 0x1
    int m_ammoCount;   // 0x4
    int m_magCount;   // 0x8
};

/* a type's weapons (SOCOM 1's DWARF: a vector and nothing more) */
class CCharacterWeapList : public std::vector<CCharacterWeap> {
};

/* a multiplayer message a type can say (SOCOM 1's MultiplayerMsg; only the sound laid out) */
class MultiplayerMsg {
public:
    unsigned char m_unk0[0x10];   // 0x0
    CSnd *m_sound;   // 0x10
};

/* a type's multiplayer messages (SOCOM 1's CMultiplayerMsgVec: a vector of pointers) */
class CMultiplayerMsgVec : public std::vector<MultiplayerMsg *> {
};

/* a kind of character: model, sounds, gear, weapons, AI and health (SOCOM 1's DWARF, 0x2FC bytes;
 * SOCOM II's is 0x3AC -- 85 sounds where SOCOM 1 had 66, each name a plain pointer where SOCOM 1
 * kept a 4-byte CSaferStr, so everything from the LODs on sits 0x98 further, the temporary flag at
 * 0x300; 0x18 bytes new at the end). The vectors and the gear list are holes of their size until
 * code reads them */
namespace zdb { class CNode; }

class CCharacterType {
public:
    CCharacterType(const char *name, const CCharacterType *base);
    ~CCharacterType();

    static CCharacterType *GetByNameMod(const char *name);
    static CCharacterType *Create(const char *name, const CCharacterType *base, bool reinit);
    static void CleanupTemporary();
    static void AcquireSoundHandles();
    /* every type's multiplayer messages' sounds let go (SOCOM 1's; 0x53BB00) */
    static void ClearMultiplayerMessageSounds();
    /* the type's multiplayer messages deleted and their storage given up (new in SOCOM II, the
     * name ours: 0x53B930) */
    void DeleteMultiplayerMessages();
    static CCharacterType *RdrFetch_CharacterType(const char *name);
    static const char *match_name_to_chartype(CRdrFile *rdr, const char *group, const char *name);
    EQUIP_ITEM GetWeaponByIndex(unsigned int i) const;
    char *GetBaseName();
    /* the character's model instanced under a name (SOCOM 1's; OVL2 0x53C9B0) -- the SEAL lane */
    zdb::CNode *CreateInstance(const char *name);
    /* a sound by kind, none out of range (new in SOCOM II, the name ours: CSealCtrlAi::OnStunDamage
     * calls it) */
    CSnd *GetSound(CHARACTER_SOUND s) const;

    char *m_name;   // 0x0
    int m_ref_count;   // 0x4
    char *m_model_file;   // 0x8
    char *m_model_name;   // 0xc
    unsigned int m_parentID;   // 0x10
    char *m_arrSoundNames[CHRSND_MAX];   // 0x14
    CSnd *m_sounds[CHRSND_MAX];   // 0x168
    unsigned char m_LODs[0xc];   // 0x2bc: LODVec
    unsigned char m_anim_sets[0xc];   // 0x2c8: a vector
    char *m_texture_asset;   // 0x2d4
    CModel *m_model;   // 0x2d8
    unsigned char m_gear[0xc];   // 0x2dc: CCharacterGearList
    CCharacterWeapList m_weapons;   // 0x2e8
    int m_strength;   // 0x2f4
    int m_recovery_factor;   // 0x2f8
    int m_controller_id;   // 0x2fc
    unsigned char m_istemporary;   // 0x300
    AI_PARAMS m_aiParams;   // 0x304
    HEALTH_PARAMS m_healthParams;   // 0x340
    unsigned char MultiplayerMessages[0xc];   // 0x388: CMultiplayerMsgVec
    unsigned char m_unk394[0xc];   // 0x394: new in SOCOM II, not laid out yet
    /* the multiplayer messages (SOCOM II's, 0x3A0: ClearMultiplayerMessageSounds 0x53BB00 walks
     * them and zeroes each one's sound) */
    CMultiplayerMsgVec m_mpMessages;   // 0x3a0

    /* every type made, by name (SOCOM 1's; OVL1 bss in SOCOM II, 0x44CD48) */
    static CCharTypeVec m_char_list;
    /* every gear made (SOCOM 1's m_gear_list, a list 0x20 after m_char_list; SOCOM II's is a
     * vector of pointers, OVL1 bss 0x44CD68, the same 0x20 on: CZSealBody::FindGear walks its data
     * and size) -- added by the SEAL lane */
    static std::vector<class CCharacterGear *> m_gear_list;
};

/* the types (SOCOM 1's DWARF: a vector of pointers and nothing more) */
class CCharTypeVec : public std::vector<CCharacterType *> {
};

bool DeleteTemporaryCharType(CCharacterType *t);

#endif
