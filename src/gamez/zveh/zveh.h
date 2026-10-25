/* zveh.h -- the vehicles: CTurret (SOCOM 1's veh_turret.cpp, its definitions CTurretDef and
 * CTurretDefVector) and CVehicle (new in SOCOM II), both CEntity subclasses. SOCOM 1's DWARF has no
 * layout for either, so every offset here is SOCOM II's own, read off its code, and only what the
 * matched functions touch is placed. The header's name is ours.
 *
 * zentity.h's CEntity declares its new virtuals +0x5C..+0x68 without parameters; CTurret's
 * overrides show +0x60 and +0x68 fill a CPnt3D and return true, +0x64 returns a pointer -- so the
 * two are declared here as CTurret's own (an override needs zentity.h's signatures corrected), the
 * third returns its node as an int. Its
 * +0x0C, SOCOM 1's GetPosW (a const CMatrix *), is zentity.h's GetPosNode. */
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
    operator float() const
    {
        float f;

        if (m_range == 0.0f)
            f = m_min;
        else
            f = m_min + m_range * (4.656613e-10f * rand());
        return f;
    }

    float m_min;   // 0x0
    float m_range;   // 0x4
};

/* a turret's definition, one per "TURRET" entry of the turrets' rdr (SOCOM 1's DWARF: 400 bytes;
 * SOCOM II's is 412 -- the vector steps 0x19C). Only what the matched code reads is named, the
 * names ours */
class CTurretDef {
public:
    void Parse(const char *name, _zrdr *rdr);

    char m_name[0x114];   // 0x0: the node-name prefix it hooks up to (PrefixMatch's strncmp)
    float m_114;   // 0x114: the throttle's third axis scaled by it (Tick)
    float m_118;   // 0x118: the fourth's
    unsigned char _11c[0x120 - 0x11c];
    float m_120;   // 0x120
    unsigned char _124[0x19c - 0x124];
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
    virtual int Virtual64() { return (int)m_288; }
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
    void Parse(const char *name, _zrdr *rdr);

    char m_name[0x88];   // 0x0: the node-name prefix it hooks up to
};   // 0x88 bytes

class CVehicleDefVector : public std::vector<CVehicleDef> {
public:
    int Init(const char *name, const char *path);
    const CVehicleDef *PrefixMatch(const char *name);
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
    bool GetFlag(unsigned int mask) const { return (m_flags & mask) != 0; }
    void SetFlag(unsigned int mask, bool on)
    {
        m_flags &= ~(on ? 0 : mask);
        m_flags |= on ? mask : 0;
    }

    /* an AI-map region it marks (0x28 bytes: bitfields, a byte, a vector at 0x1C -- OVL2's
     * 0x515880 fills it from the corners) */
    unsigned char m_region[0x28];   // 0x170
    CPnt3D m_corners[4];   // 0x198: m_bounds grown by 5 in x and z, in the world
    const CVehicleDef *m_def;   // 0x1c8
    unsigned char _1cc[0x1d4 - 0x1cc];
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
