/* zchar.h -- the character data (OVL2): the vehicle file's entries (CVehicleRdr, the SEALs and
 * tangos a mission places) and the character types (CCharacterType, char_type.cpp in the press
 * beta's asserts). The layouts are SOCOM 1's DWARF moved where SOCOM II's code puts them. The
 * directory is ours: SOCOM 1's DWARF names neither class's file. */
#ifndef ZCHAR_H
#define ZCHAR_H

#include <list>
#include <string>
#include <vector>

#include "../zweapon/zweapon.h"

class CVehicleRdrEntry;
struct _zrdr;
class CRdrFile;

/* how a character behaves (SOCOM 1's DWARF, 0x3C bytes; SOCOM II's vehicle entry keeps it whole,
 * 0x120-0x15B) */
class AI_PARAMS {
public:
    /* the defaults (SOCOM 1's __ct__9AI_PARAMSFv, out of line there; SOCOM II's visual sensitivity
     * 1 where SOCOM 1's was 0.8 -- zveh.h's CTurretAiParams has the same): inline, CVehicleRdrEntry's
     * constructor stores them in place (0x5331D0) */
    AI_PARAMS()
    {
        mask = 0;
        accuracy = 0.8f;
        courage = 0.6f;
        surrender = 0.2f;
        stealth = 0.5f;
        training = 0.5f;
        visual_sens = 1.0f;
        visual_range = 400.0f;
        visual_fov = 60.0f;
        aware_decay = -0.075f;
        thip_range = 25.0f;
        respawn = 0;
        target_delay = 0.75f;
        target_discipline = 0.5f;
        m_veh = 0;
    }
    void Parse(_zrdr *rdr);
    /* each field the first set has (its mask bit), else the second's, else the first's; the masks
     * joined (SOCOM 1's Merge, 0x533DB0) */
    void Merge(const AI_PARAMS &a, const AI_PARAMS &b);
    /* the cosine of the aiming cone: 10 degrees wide at no discipline, 2 at full (new in SOCOM II,
     * the name ours: the turrets' AI asks it, OVL1 0x3A8030 -- 0x533CF0) */
    float GetAimConeCos() const;

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
    /* no direction, no name (CVehicleRdrEntry's constructor builds the four of each set in a loop:
     * the direction stored, then the name's first byte) */
    start() : m_dir(0) { m_name[0] = 0; }

    char m_name[16];   // 0x0
    int m_dir;   // 0x10
};

/* up to four of them (SOCOM 1's DWARF) */
class startvec {
public:
    /* none (the count zeroed, then each start built) */
    startvec() : m_count(0) {}

    unsigned int m_count;   // 0x0
    start m_start[4];   // 0x4

    /* the starts out of an rdr array: each item a name and a direction (new in SOCOM II, the name
     * ours: CVehicleRdrEntry::Load reads both sets through it, 0x533A60) */
    void Load(_zrdr *rdr);

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
    /* an empty entry (out of line, 0x5331D0: the resize's fill value, 0x534320) */
    CVehicleRdrEntry();
    const char *GetStartPos() const;
    int GetStartDir() const;
    /* the respawning set's (new in SOCOM II; the names are ours) */
    const char *GetRespawnPos() const;
    int GetRespawnDir() const;
    void SetCharacterName(const char *name);
    void Randomize(bool reset);
    /* the start's place as a matrix: the start picked (at random for -1), its location found and
     * its cell's centre taken, the rotation its direction gives (SOCOM 1's; 0x532D10) */
    bool GetStartMatrix(class CMatrix &m, unsigned int index);
    /* one entry out of the file's item (SOCOM 1's CVehicleRdr::Load did every entry; the name is
     * ours) */
    void Load(_zrdr *rdr);

    char m_name[32];   // 0x0
    char m_character[48];   // 0x20
    char m_display_name[32];   // 0x50
    char m_unk70;   // 0x70: a byte (Load stores it with sb, the assignment copies it lb/sb)
    startvec m_start;   // 0x74
    startvec m_respawn;   // 0xc8: new in SOCOM II ("respawn_setup")
    int m_teamMask;   // 0x11c
    AI_PARAMS m_aiParams;   // 0x120
    /* 0x15c: a word of flags in SOCOM II -- the constructor's mem-initializer list clears them with
     * word read-mask-writes, the getters read the index through lhu (MW narrows it) */
    unsigned int m_debug : 1;
    unsigned int m_disabled : 1;
    unsigned int m_nofade : 1;
    unsigned int m_noshoot : 1;
    unsigned int m_recycle : 1;
    unsigned int m_nosnooze : 1;
    unsigned int m_startIndex : 4;   // SOCOM 1's m_startIndex, a word at 0x108 there
    unsigned int m_respawnIndex : 4;   // new (the name is ours)
    unsigned int m_unused : 2;
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

/* a part's health and armour (SOCOM 1's DWARF, 0x48 bytes, the six parts' health and maximum
 * interleaved, then the armour; SOCOM II's are three arrays of six, by HEALTH_PARAMS::Parse
 * 0x53DDB0: the "health" block's head, body, larm, rarm, rleg and lleg at 0x0, 0xC, 0x8, 0x4, 0x10
 * and 0x14, the "armor" block's the same 0x18 on, and the health copied whole to 0x30 as the
 * maxima -- CCharacterType's constructor copies each array four and two) */
class HEALTH_PARAMS {
public:
    /* the parts' default health, no armour, the maxima the health (CCharacterType's constructor
     * builds it in place: the stores, then memset and memcpy) */
    HEALTH_PARAMS()
    {
        m_health[HEAD] = 40.0f;
        m_health[BODY] = 80.0f;
        m_health[LARM] = 50.0f;
        m_health[RARM] = 50.0f;
        m_health[LLEG] = 75.0f;
        m_health[RLEG] = 75.0f;
        memset(m_armor, 0, sizeof(m_armor));
        memcpy(m_healthMax, m_health, sizeof(m_health));
    }
    void Parse(_zrdr *rdr);

    /* the parts, by their slots (the names ours, after Parse's tags) */
    enum { HEAD = 0, RARM = 1, LARM = 2, BODY = 3, RLEG = 4, LLEG = 5 };

    float m_health[6];   // 0x0
    float m_armor[6];   // 0x18
    float m_healthMax[6];   // 0x30
};

class CSnd;
class CModel;

/* a weapon a type carries, with its ammunition (SOCOM 1's DWARF, 0xC bytes) */
class CCharacterWeap {
public:
    /* none, no ammunition (AddWeapon's temporary, 0x53B3A0: 255 in both bytes, the counts zeroed) */
    CCharacterWeap() : m_wepId(EQUIP_NONE), m_ammoId(AMMO_NONE), m_ammoCount(0), m_magCount(0) {}
    /* the item set, and its magazine size, magazines and default ammunition when the weapon
     * list knows it (SOCOM 1's; 0x53EEE0 -- the weapon selector calls it after AddWeapon) */
    void SetupCharacterWeapon(EQUIP_ITEM item);

    EQUIP_ITEM m_wepId;   // 0x0
    EQUIP_AMMO m_ammoId;   // 0x1
    int m_ammoCount;   // 0x4
    int m_magCount;   // 0x8
};

/* a weapon as the network carries it (SOCOM 1's CCharacterWeapPacked, 6 bytes: the item and
 * ammunition packed into one halfword, then the counts) */
class CCharacterWeapPacked {
public:
    unsigned short m_packed;   // 0x0
    unsigned short m_ammoCount;   // 0x2
    unsigned short m_magCount;   // 0x4
};

/* a type's weapons (SOCOM 1's DWARF: a vector and nothing more) */
class CCharacterWeapList : public std::vector<CCharacterWeap> {
public:
    /* room for eight (CCharacterType's constructor: the class reserve 0x540270 called right after
     * the vector's words are zeroed, ahead of the members that follow) */
    CCharacterWeapList() { reserve(8); }
    /* an empty weapon pushed (SOCOM 1's AddWeapon; the item unread -- the callers set it up after,
     * through SetupCharacterWeapon 0x53EEE0) */
    void AddWeapon(EQUIP_ITEM item);
    /* every weapon of the item dropped, true if there was one (SOCOM 1's RemoveWeapon, char_type.cpp's
     * last function in both builds: 0x53B300) */
    bool RemoveWeapon(EQUIP_ITEM item);
    /* a set-up weapon of the item put at an index, the ones from there on moved up one, the last
     * pushed (new in SOCOM II, the name ours: 0x53B160) */
    void InsertWeapon(int index, EQUIP_ITEM item);
    /* after every weapon taking several slots, an EQUIP_END for each slot past the first (new in
     * SOCOM II, the name ours: 0x53B070 -- CreateCustomCharTypes calls it on a custom type's
     * weapons) */
    void PadWeaponSlots();
};

/* a multiplayer message a type can say (SOCOM 1's MultiplayerMsg, DWARF, 0x14 bytes) */
class MultiplayerMsg {
public:
    /* all cleared (InitMultiplayerMessages' `new`, both builds: the index, then the three strings
     * right to left -- one chained assignment -- then the sound) */
    MultiplayerMsg()
    {
        idx = 0;
        message = sound = tcmtag = 0;
        sound_pointer = 0;
    }

    unsigned int idx;   // 0x0
    char *message;   // 0x4
    char *sound;   // 0x8
    char *tcmtag;   // 0xc
    CSnd *sound_pointer;   // 0x10
};

/* a type's multiplayer messages (SOCOM 1's CMultiplayerMsgVec: a vector of pointers) */
class CMultiplayerMsgVec : public std::vector<MultiplayerMsg *> {
};

/* the type's animation sets' names (SOCOM 1's `vector m_anim_sets`; a pointer vector in a class of
 * its own here -- ~CCharacterType tests it four times, as a derived pointer vector; the name ours) */
class CAnimSetVec : public std::vector<char *> {
};

/* the gears (SOCOM 1's CCharacterGearList; SOCOM II's a vector of pointers -- CCharacterType::
 * m_gear_list stays declared as the plain vector, which its users bind; zseal.h has CCharacterGear) */
class CCharacterGearList : public std::vector<class CCharacterGear *> {
public:
    /* the gear of that name, none for no name (SOCOM 1's FindGear, const there; SOCOM II's is not -- the
     * walk reads the size and data off `this` each pass: 0x53EFF0, called on m_gear_list) */
    CCharacterGear *FindGear(const char *name);
};

/* a kind of character: model, sounds, gear, weapons, AI and health (SOCOM 1's DWARF, 0x2FC bytes;
 * SOCOM II's is 0x3AC -- 85 sounds where SOCOM 1 had 66, each name a plain pointer where SOCOM 1
 * kept a 4-byte CSaferStr, so everything from the LODs on sits 0x98 further, the temporary flag at
 * 0x300; 0x18 bytes new at the end). The vectors and the gear list are holes of their size until
 * code reads them */
namespace zdb { class CNode; }

/* a character's voice mask: six figures CCharacterType::Parse reads off the type's "VoiceFont"
 * block, each tag named after its setter ("SetPitchOffset" ... "SetWhisperFactor") -- new in
 * SOCOM II, the names after those tags. The armory's character change (UIChangeCharacterType)
 * reads them through the getters, out of line (0x53B010-0x53B06F, defined last in char_type.cpp) */
class CVoiceFont {
public:
    /* no shift, the robot and whisper figures -1 (CCharacterType's constructor) */
    CVoiceFont()
        : m_pitchOffset(0.0f), m_spectralFactor(0.0f), m_spectralLedge(0.0f), m_roboOffset(-1.0f),
          m_roboSquash(-1.0f), m_whisperFactor(-1.0f)
    {
    }
    float GetPitchOffset() const;
    float GetSpectralFactor() const;
    float GetSpectralLedge() const;
    float GetRoboOffset() const;
    float GetRoboSquash() const;
    float GetWhisperFactor() const;

    float m_pitchOffset;   // 0x0
    float m_spectralFactor;   // 0x4
    float m_spectralLedge;   // 0x8
    float m_roboOffset;   // 0xc
    float m_roboSquash;   // 0x10
    float m_whisperFactor;   // 0x14
};

class CCharacterType {
public:
    /* a level of detail: a model file and name and the distance it starts at (SOCOM 1's global
     * LOD, nested here: zanim.h's enum LOD shares the name and units see both; SOCOM II's strings
     * are 12 bytes, so 0x1C -- the vector's teardown 0x540A50 frees both) */
    class LOD {
    public:
        /* empty, at no distance (Parse's temporary: all seven words zeroed) */
        LOD() : m_dist(0.0f) {}

        std::string m_file;   // 0x0: the model's name, the file's without its extension
        std::string m_name;   // 0xc: the file's name (SOCOM 1's names, both builds' use)
        float m_dist;   // 0x18
    };
    /* the type's levels of detail (SOCOM 1's DWARF: a vector and nothing more) */
    class LODVec : public std::vector<LOD> {
    };

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
    /* the multiplayer messages read off the sound list (SOCOM 1's; OVL2 0x53BBD0) */
    void InitMultiplayerMessages(_zrdr *rdr);
    /* the sound list's kind/file pairs into the sound names, then its multiplayer messages (new in
     * SOCOM II as a function, the name ours: SOCOM 1's Parse did it in place; OVL2 0x53BEF0) */
    void ParseSounds(_zrdr *list);
    /* the type's record of chartype.rdr: model, texture, controller, anim sets, gear, weapons,
     * strength, health, voice font, levels of detail, AI parameters and sounds (SOCOM 1's; OVL2
     * 0x53CE00, Open's) */
    bool Parse(_zrdr *rdr);
    /* chartype.rdr read: the body items' and gear nodes' names registered, the angle units, the AI
     * accuracy, every gear, then each character -- a name, an optional ": base", a record -- made,
     * parsed and listed (SOCOM 1's; OVL2 0x53E220, CMission::Init's) */
    static bool Open(const char *file);
    static CCharacterType *RdrFetch_CharacterType(const char *name);
    /* a type's weapons replaced by the eight a packed loadout holds, up to its first empty one
     * (SOCOM 1's UnPack, 0x53EC60: CZSealBody's remote respawn and creation call it) */
    static void UnPack(const CCharacterWeapPacked *packed, CCharacterType *type);
    /* a type's weapons packed, up to eight (new in SOCOM II, the name ours: SOCOM 1's
     * KorusGetPackedWeapons did it in place; 0x53ED50) */
    static void PackWeapons(CCharacterWeapPacked *packed, CCharacterType *type);
    /* the weapons of the type named at +300 of OVL1's 0x437CE8 packed (SOCOM 1's took a fixed name;
     * 0x53EE10) */
    static void KorusGetPackedWeapons(CCharacterWeapPacked *packed);
    /* every gear let go (new in SOCOM II, the name ours: 0x53F0B0) */
    static void ResetGears();
    static const char *match_name_to_chartype(CRdrFile *rdr, const char *group, const char *name);
    EQUIP_ITEM GetWeaponByIndex(unsigned int i) const;
    /* the first holder loads the model and each gear's (from the model cache, else the world, then
     * cached); the type counted and handed back, none without a model (SOCOM 1's; OVL2 0x53CCA0) */
    CCharacterType *Acquire() const;
    char *GetBaseName();
    /* the character's model instanced under a name (SOCOM 1's; OVL2 0x53C9B0) -- the SEAL lane */
    zdb::CNode *CreateInstance(const char *name);
    /* the count dropped; at none every gear let go and the model and sounds forgotten, the count
     * returned (SOCOM 1's; OVL2 0x53CA40) */
    int Release();
    /* each level of detail's model (cached, else the world's, then cached) added to the node's
     * mesh as a level, when both hold one mesh (SOCOM 1's; OVL2 0x53CB20) */
    void CreateLODs(zdb::CNode *node);
    /* a sound by kind, none out of range (new in SOCOM II, the name ours: CSealCtrlAi::OnStunDamage
     * calls it) */
    CSnd *GetSound(CHARACTER_SOUND s) const;
    /* a sound played for the entity of that id, whether it went to the positional player (SOCOM
     * 1's, void there; OVL2 0x53B710 -- CSealCtrl's sound calls it) */
    bool PlayAt(CHARACTER_SOUND s, unsigned int id, const CPnt3D *pos);
    /* the voice font set from a copy (the name ours: the constructor copies its base's so -- the
     * parameter built on the stack four floats and two, then assigned member by member; a plain
     * assignment or a returned copy is scalarised) */
    void SetVoiceFont(CVoiceFont vf) { m_voiceFont = vf; }

    char *m_name;   // 0x0
    mutable int m_ref_count;   // 0x4: mutable -- Acquire, const in both builds, counts
    char *m_model_file;   // 0x8
    char *m_model_name;   // 0xc
    unsigned int m_parentID;   // 0x10
    char *m_arrSoundNames[CHRSND_MAX];   // 0x14
    CSnd *m_sounds[CHRSND_MAX];   // 0x168
    LODVec m_LODs;   // 0x2bc
    CAnimSetVec m_anim_sets;   // 0x2c8
    char *m_texture_asset;   // 0x2d4
    mutable CModel *m_model;   // 0x2d8: mutable -- Acquire loads it
    CCharacterGearList m_gear;   // 0x2dc
    CCharacterWeapList m_weapons;   // 0x2e8
    int m_strength;   // 0x2f4
    int m_recovery_factor;   // 0x2f8
    int m_controller_id;   // 0x2fc
    unsigned char m_istemporary;   // 0x300
    AI_PARAMS m_aiParams;   // 0x304
    HEALTH_PARAMS m_healthParams;   // 0x340
    CVoiceFont m_voiceFont;   // 0x388
    /* the multiplayer messages (SOCOM II's, 0x3A0: ClearMultiplayerMessageSounds 0x53BB00 walks
     * them and zeroes each one's sound) */
    CMultiplayerMsgVec m_mpMessages;   // 0x3a0

    /* every type made, by name (SOCOM 1's; OVL1 bss in SOCOM II, 0x44CD48) */
    static CCharTypeVec m_char_list;
    /* every gear made (SOCOM 1's m_gear_list, a list 0x20 after m_char_list; SOCOM II's is a
     * vector of pointers, OVL1 bss 0x44CD68, the same 0x20 on: CZSealBody::FindGear walks its data
     * and size) -- added by the SEAL lane */
    static std::vector<class CCharacterGear *> m_gear_list;
    /* the anim sets' names, each kept once (SOCOM 1's m_anim_set_names, a pointer list 0x40 after
     * m_char_list; SOCOM II's is OVL1 bss at 0x44CD88, the same 0x40 on) */
    static std::list<char *> m_anim_set_names;
    /* the list's copy of an anim set's name, a copy added the first time (new in SOCOM II, the name
     * ours: SOCOM 1's Parse did it in place; OVL2 0x53C110) */
    static char *AddAnimSetName(const char *name);
};

/* the types (SOCOM 1's DWARF: a vector of pointers and nothing more) */
class CCharTypeVec : public std::vector<CCharacterType *> {
};

bool DeleteTemporaryCharType(CCharacterType *t);

#endif
