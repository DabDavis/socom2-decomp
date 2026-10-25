/* zveh.h -- the vehicles: CTurret (SOCOM 1's veh_turret.cpp, its definitions CTurretDef and
 * CTurretDefVector) and CVehicle (new in SOCOM II), both CEntity subclasses. SOCOM 1's DWARF has no
 * layout for either, so every offset here is SOCOM II's own, read off its code, and only what the
 * matched functions touch is placed. The header's name is ours.
 *
 * CTurret's overrides of CEntity's new virtuals show +0x60 and +0x68 fill a CPnt3D and return
 * true, +0x64 returns a pointer -- zentity.h declares them so. Its +0x0C is SOCOM 1's GetPosW (a
 * const CMatrix *), as zentity.h has it. */
#ifndef ZVEH_H
#define ZVEH_H

#include <stdlib.h>
#include <vector>

#include "../zentity/zentity.h"
#include "../zweapon/zweapon.h"
#include "../zseal/zseal.h"
#include "../zrdr/zrdr.h"
#include "../zsys/zsys.h"

/* a random float: a minimum and a range (SOCOM 1's DWARF, 8 bytes; zkit.h has its own view). Its
 * two constructors are out of line here (0x3AA900, 0x3AA920: CTurret's constructor calls them),
 * the value inline (SOCOM 1's operator float) */
class Rfloat {
public:
    Rfloat(float f = 0.0f)
    {
        m_min = f;
        m_range = 0.0f;
    }
    Rfloat(float lo, float hi)
    {
        m_min = lo;
        m_range = hi - lo;
    }
    /* a copy, field by field: user-declared, it makes CAiTrigger's implicit assignment a function
     * of its own, which the definitions' copy, fill and copy_backward call out of line (0x3ABD50);
     * left implicit, MW block-copies the trigger inside them */
    Rfloat &operator=(const Rfloat &r)
    {
        m_min = r.m_min;
        m_range = r.m_range;
        return *this;
    }
    /* a fixed value (SOCOM 1's __as__6RfloatFf) */
    Rfloat &operator=(float f)
    {
        m_min = f;
        m_range = 0.0f;
        return *this;
    }
    /* the random factor through a local of its own, as zkit.h's (CAiTrigger::Set's second draw
     * wants its registers and label pads) */
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else {
            float r = 4.656613e-10f * rand();
            f = m_min + m_range * r;
        }
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* a turret's firing cycle: a random time up and a random time down, the cycle's length and the up
 * time drawn (SOCOM 1's DWARF: CAiTrigger, 0x18 bytes; CTurretDef and CTurret hold one) */
class CAiTrigger {
public:
    /* (inline: too big to inline whole into CTurret's and CTurretDef's constructors, which call the
     * Rfloats' constructors and SetTimes out of line -- 0x3AA920, 0x3AA810: down 0.5-1.2, up
     * 0.1-0.5) */
    CAiTrigger() { SetTimes(Rfloat(0.5f, 1.2f), Rfloat(0.1f, 0.5f)); }
    /* the two times set and a cycle drawn: the up time, then the cycle up plus down (the name the
     * kit lane's, OVL2's copy 0x5FF200; the down time is the first stored; CTurretDef's constructor
     * and Parse inline it) */
    void SetTimes(const Rfloat &dn, const Rfloat &up)
    {
        m_dn_time = dn;
        m_up_time = up;
        m_marker = m_up_time;
        m_timer = m_marker + m_dn_time;
    }

    Rfloat m_up_time;   // 0x0
    Rfloat m_dn_time;   // 0x8
    float m_timer;   // 0x10
    float m_marker;   // 0x14
};   // 0x18 bytes

/* how a turret's AI behaves: SOCOM 1's AI_PARAMS (zchar.h's, 0x3C bytes) with its default
 * constructor -- SOCOM 1's __ct__9AI_PARAMSFv, SOCOM II's visual sensitivity 1 where SOCOM 1's was
 * 0.8 -- which zchar.h's does not declare. Layout-compatible: the definition's Parse hands it to
 * AI_PARAMS::Parse (OVL2 0x534040). Its constructor being user-declared makes the copy constructor
 * member-wise (0x3AC6D0) while the assignment stays a block copy (the 15-word loop in 0x3ABAE0) */
class CTurretAiParams {
public:
    CTurretAiParams()
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
    AI_PARAMS *Params() { return (AI_PARAMS *)this; }

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
};   // 0x3c bytes

/* a range of angles (radians), and the sines and cosines of its ends (the names ours) */
struct TURRET_LIMIT {
    float m_min;   // 0x0
    float m_max;   // 0x4
};
struct TURRET_LIMIT_TRIG {
    float m_sinmin;   // 0x0
    float m_cosmin;   // 0x4
    float m_sinmax;   // 0x8
    float m_cosmax;   // 0xc
};

/* the definition's flag word (a class of its own: the copy constructor copies it as a word, lw/sw,
 * the assignment as a 4-byte block, lwc1/swc1 -- the AI_PARAMS pattern, a user-declared
 * constructor over plain members; the names ours) */
class CTurretDefFlags {
public:
    CTurretDefFlags(unsigned int f) { m_bits = f; }

    unsigned int m_bits;   // 0x0
};

/* a turret's definition, one per "TURRET" entry of the turrets' rdr (SOCOM 1's: 400 bytes, its
 * constructor, copy constructor and assignment named; SOCOM II's is 412 -- the vector steps 0x19C).
 * The layout is read off SOCOM II's copy constructor (0x3AC6D0) and Parse (0x3AAD20), the member
 * names ours after Parse's rdr fields */
class CTurretDef {
public:
    /* (out of line, veh_turret.cpp's: right after Parse, as SOCOM 1's) */
    CTurretDef();
    /* empty and inline (SOCOM 1's was out of line, 0x44 bytes): a destructor makes the resize build
     * its temporary first, then read the size and data again (0x3AB8C0 calls 0x3AB5F0 ahead of
     * them; without one MW expands end() and size() ahead of the out-of-line constructor) */
    ~CTurretDef() {}
    void Parse(const char *name, _zrdr *rdr);
    bool GetFlag(unsigned int mask) const { return (m_flags.m_bits & mask) != 0; }
    void SetFlag(unsigned int mask, bool on)
    {
        m_flags.m_bits &= ~(on ? 0 : mask);
        m_flags.m_bits |= on ? mask : 0;
    }

    char m_name[0x20];   // 0x0: the node-name prefix it hooks up to (PrefixMatch's strncmp)
    char m_20[0x20];   // 0x20 (not parsed)
    char m_parts[3][0x20];   // 0x40: "PARTS", up to three node names
    char m_destroyAnim[0x20];   // 0xa0: "DESTROY_ANIM"
    char m_effect[0x20];   // 0xc0: "EFFECT"'s name
    EQUIP_ITEM m_weapon;   // 0xe0: "WEAPON"'s NAME (byte enums: copied with lb)
    EQUIP_AMMO m_ammo;   // 0xe1: its AMMO
    bool m_thermal;   // 0xe2: "THERMAL"
    bool m_compass;   // 0xe3: "COMPASS"
    TURRET_LIMIT m_yaw;   // 0xe4: "LIMITS" YAW (degrees in, radians kept)
    TURRET_LIMIT m_pitch;   // 0xec: PITCH
    TURRET_LIMIT_TRIG m_yawTrig;   // 0xf4
    TURRET_LIMIT_TRIG m_pitchTrig;   // 0x104
    float m_yawRate;   // 0x114: "RATES" YAW (Tick scales the throttle's third axis by it)
    float m_pitchRate;   // 0x118: PITCH (the fourth's)
    float m_maxAngle;   // 0x11c: the largest limit, held in 0.01..pi/2
    float m_tanHalfAngle;   // 0x120: its half's tangent (PreTick)
    float m_health;   // 0x124: "HEALTH"
    float m_effectValue;   // 0x128: "EFFECT"'s number
    int m_team;   // 0x12c: "TEAM"
    CTurretDefFlags m_flags;   // 0x130: 2 cleared by "MANUAL", 0x10 set by "AUTOSCAN"
    float m_mountRange;   // 0x134: "MOUNTABLE_RANGE" squared (22 by default)
    float m_mountAngle;   // 0x138: "MOUNTABLE_ANGLE"'s cosine (45 degrees by default)
    CAiTrigger m_trigger;   // 0x13c: "TRIGGER" up_time, dn_time
    CTurretAiParams m_aiParams;   // 0x154: "ai_params"
    CPnt3D m_lookDir;   // 0x190: "AI_LOOKDIR"
};   // 0x19c bytes

/* the definitions (SOCOM 1's DWARF: CTurretDefVector : vector<CTurretDef>) */
class CTurretDefVector : public std::vector<CTurretDef> {
public:
    /* the definitions read from a file: as many as its "TURRET" list holds pairs (new in SOCOM II,
     * SOCOM 1's CTurret::Init held it; the name ours) */
    int Init(const char *name, const char *path);
    const CTurretDef *PrefixMatch(const char *name);
};

/* the turrets made, a pointer list (new in SOCOM II: each turret adds itself in its constructor
 * and takes itself out in its destructor) */
class CTurretList : public std::vector<class CTurret *> {
};

class CTurret : public CEntity {
public:
    CTurret(zdb::CNode *node, const CTurretDef &def);
    virtual ~CTurret();
    virtual void UpdatePosW();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PrePostTick(float dT);
    virtual void ComputeNextPosition(float dT);
    virtual void UpdateAiMapLoc();
    virtual void OnMissionStart();
    virtual void UpdateCompassAnchor();
    /* the three new slots CTurret answers (+0x60, +0x64, +0x68; see above): two points and the
     * node at 0x288 */
    virtual bool Virtual60(CPnt3D *out)
    {
        *out = m_380;
        return true;
    }
    virtual zdb::CNode *Virtual64() { return m_288; }
    virtual bool Virtual68(CPnt3D *out)
    {
        *out = m_38c;
        return true;
    }

    void SampleDynamics(float dT);
    /* SOCOM 1's statics: the definitions read, the world's landmarks scanned for nodes they name */
    static int Init(const char *name, const char *path);
    static void LoadTurrets();
    static int ScanForTurretDefinitionHookups(zdb::CNode *node, bool recurse);
    /* a turret for the node universe's index, if a definition names its node (new in SOCOM II; the
     * name ours) */
    static CTurret *Create(int index);
    /* every turret's ammunition refilled (new in SOCOM II, the mission's exit state calls it; the
     * name ours) */
    static void ResetAmmo();
    /* the flag word's bits (the names ours) */
    bool GetFlag(unsigned int mask) const { return (m_flags & mask) != 0; }
    void SetFlag(unsigned int mask, bool on)
    {
        m_flags &= ~(on ? 0 : mask);
        m_flags |= on ? mask : 0;
    }
    /* a controller drives it (the name ours) */
    bool IsControlled() const { return m_control != 0; }
    /* alive and switched on (the name ours) */
    bool IsOperational() const { return m_isAlive && GetFlag(0x20); }
    /* switched on or off, as alive (new in SOCOM II; the name ours) */
    void SetActive(bool on);
    /* the scope's two looks shown, one or the other (new in SOCOM II; the name ours) */
    void SetThermal(zdb::CNode *node, bool thermal);

    CAiTimer m_fireTimer;   // 0x170
    unsigned char _17c[0x220 - 0x17c];
    float m_220;   // 0x220
    unsigned char _224[0x240 - 0x224];
    int m_240;   // 0x240: cleared at the mission's start
    unsigned char _244[0x254 - 0x244];
    float m_254;   // 0x254: PreTick's copy of m_220, its product with the definition's m_120,
    float m_258;   // 0x258: 25, and its inverse
    float m_25c;   // 0x25c
    float m_260;   // 0x260
    CPnt3D m_264;   // 0x264: the position, copied each PreTick
    CPnt3D m_270;   // 0x270: m_38c, copied each PreTick
    const CTurretDef *m_def;   // 0x27c
    zdb::CNode *m_yawNode;   // 0x280
    zdb::CNode *m_pitchNode;   // 0x284
    zdb::CNode *m_288;   // 0x288
    zdb::CNode *m_root;   // 0x28c: the node's ancestor under the world
    zdb::CNode *m_scope;   // 0x290: "scope"
    zdb::CNode *m_thermalScope;   // 0x294: "thermal_scope"
    bool m_thermal;   // 0x298
    unsigned char _299[0x2a8 - 0x299];
    int m_ammo;   // 0x2a8
    CZWeapon *m_weapon;   // 0x2ac
    unsigned char _2b0[0x2c0 - 0x2b0];
    CMatrix m_2c0;   // 0x2c0
    CMatrix m_300;   // 0x300
    CMatrix m_340;   // 0x340
    CPnt3D m_380;   // 0x380
    CPnt3D m_38c;   // 0x38c
    CPnt3D m_398;   // 0x398
    unsigned int m_flags;   // 0x3a4
    float m_3a8;   // 0x3a8: the third throttle, scaled (Tick)
    float m_3ac;   // 0x3ac: the fourth
    float m_3b0;   // 0x3b0
    float m_3b4;   // 0x3b4
    float m_3b8;   // 0x3b8
    float m_3bc;   // 0x3bc
    float m_3c0;   // 0x3c0
    unsigned char _3c4[0x3d0 - 0x3c4];

    /* the definitions (SOCOM 1's static, 0x4A4038 here) and the turrets (new, 0x4A4058) */
    static CTurretDefVector m_turret_def;
    static CTurretList m_turrets;
};   // 0x3d0 bytes (Create news 976)

/* a vehicle's definition, one per "VEHICLE" entry of the vehicles' rdr (new in SOCOM II, the
 * turret's twin: 136 bytes, the vector steps 0x88; the names ours) */
class CVehicleDef {
public:
    /* the strings emptied and the word at 0x84 cleared (inline: the resize 0x3AE750 builds its
     * temporary in place -- four byte stores and a word) */
    CVehicleDef()
    {
        m_name[0] = 0;
        m_20[0] = 0;
        m_40[0] = 0;
        m_60[0] = 0;
        m_84 = 0;
    }
    void Parse(const char *name, _zrdr *rdr);

    /* four strings, a byte and a word: the erase's inlined assignment (0x3AE800) copies the strings
     * a byte at a time (lb), the byte with lbu and the word with lw */
    char m_name[0x20];   // 0x0: the node-name prefix it hooks up to
    char m_20[0x20];   // 0x20
    char m_40[0x20];   // 0x40
    char m_60[0x20];   // 0x60
    unsigned char m_80;   // 0x80
    int m_84;   // 0x84
};   // 0x88 bytes

class CVehicleDefVector : public std::vector<CVehicleDef> {
public:
    int Init(const char *name, const char *path);
    const CVehicleDef *PrefixMatch(const char *name);
    /* the definition of that name, case-blind (inline: the spawn's; the name ours) */
    const CVehicleDef *Find(const char *name)
    {
        if (name) {
            for (iterator it = begin(); it != end(); ++it)
                if (!strcasecmp(name, (*it).m_name))
                    return &*it;
        }
        return 0;
    }
};

/* an AI map's region: ai.h's CAiMap::region (0x1C bytes, out of sight here), its constructor's
 * mem-initializer bitfields word by word (OVL2's walker 0x536230 takes one) */
class CVehicleAiRegion {
public:
    CVehicleAiRegion() : m_dx(0), m_dy(0), m_type(0), m_nodisplay(0), m_flags(0), m_unused(0) { m_name[0] = 0; }

    CAiMapLoc m_loc;   // 0x0
    unsigned int m_dx : 16;   // 0x4
    unsigned int m_dy : 16;
    unsigned int m_type : 5;   // 0x8
    unsigned int m_nodisplay : 1;
    unsigned int m_flags : 16;
    unsigned int m_unused : 10;
    char m_name[16];   // 0xc
};

/* the AI-map region a vehicle marks (0x28 bytes): a region and the cells it covers (the names ours).
 * CVehicle's constructor inlines it whole (five high: the region's bitfields, then the vector's three
 * words) -- with the cells a vector subclass, as CAiMapLocVec, it would be six and inlined one level;
 * ~CVehicle wants that subclass level (see there) */
class CVehicleRegion : public CVehicleAiRegion {
public:
    std::vector<CAiMapLoc> m_cells;   // 0x1c
};

/* a vehicle (new in SOCOM II, CEntity's type 4; CTurret's unit's twin -- the names ours). Its
 * table overrides UpdatePosW, PreTick, Tick, PrePostTick, ComputeNextPosition, GetNeckPosW,
 * UpdateAiMapLoc and OnMissionStart */
class CVehicle : public CEntity {
public:
    CVehicle(zdb::CNode *node, const CVehicleDef &def);
    virtual ~CVehicle();
    virtual void UpdatePosW();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PrePostTick(float dT);
    virtual void ComputeNextPosition(float dT);
    virtual CPnt3D *GetNeckPosW(CPnt3D *out);
    virtual void UpdateAiMapLoc();
    virtual void OnMissionStart();

    static int Init(const char *name, const char *path);
    static void LoadVehicles();
    static int ScanForVehicleDefinitionHookups(zdb::CNode *node, bool recurse);
    /* a mission entry's vehicles: its character names the definition, its name the nodes
     * (CMission's spawner calls it; nothing is returned -- the name ours) */
    static CEntity *Spawn(CVehicleRdrEntry *entry);
    /* the same scan for one definition, by the node's whole name (the name ours) */
    static int ScanForVehicleByName(const char *name, const CVehicleDef *def, zdb::CNode *node,
                                    bool recurse);
    bool GetFlag(unsigned int mask) const { return (m_flags & mask) != 0; }
    void SetFlag(unsigned int mask, bool on)
    {
        m_flags &= ~(on ? 0 : mask);
        m_flags |= on ? mask : 0;
    }

    /* an AI-map region it marks (OVL2's 0x515880 fills it from the corners) */
    CVehicleRegion m_region;   // 0x170
    CPnt3D m_corners[4];   // 0x198: m_bounds grown by 5 in x and z, in the world
    const CVehicleDef *m_def;   // 0x1c8
    int m_1cc;   // 0x1cc
    zdb::CNode *m_1d0;   // 0x1d0: the node it was made for
    zdb::CNode *m_1d4;   // 0x1d4: deactivates the vehicle once switched off
    zdb::CNode *m_root;   // 0x1d8: the node's ancestor under the world
    unsigned char _1dc[0x1e0 - 0x1dc];
    CMatrix m_1e0;   // 0x1e0
    CMatrix m_220;   // 0x220
    unsigned char _260[0x268 - 0x260];
    CPnt3D m_bounds[4];   // 0x268
    unsigned char _298[0x2ac - 0x298];
    unsigned int m_flags;   // 0x2ac

    /* the definitions (0x4A4138; the name ours, after the turret's) */
    static CVehicleDefVector m_vehicle_def;
};   // 0x2b0 bytes (the scan news 688)

#endif
