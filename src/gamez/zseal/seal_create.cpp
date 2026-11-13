/* seal_create.cpp -- the SEAL made and unmade: respawns, the remote SEAL, Create and ReCreate, the
 * gear, the character's model (SOCOM 1's unit, __sinit_seal_create.cpp; SOCOM 1's order from
 * RemoteRespawn to GetSeal). OVL2 0x598B90 (RemoteRespawn, after seal_control.cpp's template
 * tail: DrawFunc<CDynGrenade> at 0x598860) to seal_dyn.cpp (0x59BA80, CharacterDynamics::Load).
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include <stdio.h>
#include <new>

/* (CCharacterType::Acquire and CreateLODs -- SOCOM 1's Acquire__14CCharacterTypeCFv and
 * CreateLODs__14CCharacterTypeFPQ23zdb5CNode, called here where SOCOM 1's PreCreateSeal calls them;
 * OVL2 0x53CCA0, 0x53CB20 -- are zchar.h's, called by name) */

class CZSealState {
public:
    static void SealDeleteCleanup(int id);
    /* (SOCOM 1's; OVL1 0x2BD3D0: the packed weapons a remote SEAL came in with) */
    static void *GetPackedWeaponData(CZSealBody *seal);
};

/* (CZWeapon's projectile list, m_pProjectilelist, and its clear of a shooter's projectiles,
 * CZProjectileList::Clear(int, EQUIP_ITEM) -- SOCOM 1's took the shooter only; OVL1 0x3CCEB0 --
 * are zweapon.h's; a squad's clean-up of the SEAL is CSealUnit::SealLeaveUnit, zseal.h's) */

/* the mission, only its team valves' refresh (SOCOM 1's CMission::RefreshTeamValves, OVL1
 * 0x2B23A0, on theMission, OVL1 bss 0x4364E0: fts_state.h's class, which this unit does not
 * include) */
class CMission {
public:
    void RefreshTeamValves();

    unsigned char _0[0x1814];
    /* the vehicle file's entries (seal_tasks.cpp's reading: OVL1 bss 0x437CF4) */
    CVehicleRdr *m_vehicles;   // 0x1814
};

extern CMission theMission;

extern "C" {
/* the top blend's representative flagged (seal_body.cpp's, OVL2 0x57B510), the body's top blend and a
 * halfword's bits (seal.cpp's tail copies, 0x555170 and 0x5551A0) */
bool func_0057B510(CZSealBody *seal);
void *func_00555170(void *stack);
bool func_005551A0(unsigned short flags, unsigned short bits);
/* AnimTypes statics past SOCOM 1's list (OVL1 data 0x3DEF08, 0x3DEF50) */
extern short data_003DEF08;
extern short data_003DEF50;
}

/* the root's height followed: under a flagged anim whose top blend lacks flag 0x20, or anim 46 (an
 * inline: the target builds the answer -- `daddiu 1` / `move zero` -- and then tests it) */
static inline bool FollowsRoot(CZSealBody *seal)
{
    if ((func_0057B510(seal) &&
         !func_005551A0(((CZBodyAnimBlend *)func_00555170(&seal->m_blends))->m_flags, 32)) ||
        seal->IsCurrentBlend((AnimType)46))
        return true;
    return false;
}

/* the SEAL's altitude moved with its root part against a point: under the 0x3DEF08 anim m_fac eased
 * down to 2.2 first (and there the SEAL lands); under the 0x3DEF50 anim the root held at 11.44; always
 * true (OVL2 0x59B870, new in SOCOM II: 0x57A330 calls it with a point of its own; after GetSeal, before
 * seal_dyn.cpp -- the names ours). The height is 0x41370A3E, one ulp above 11.44f (what `2.2f * 5.2`
 * folds to); the helper's answer an if/return (the `||` of a ternary or a plain return materialises
 * the second call's test, sltu/xori) */
extern "C" bool func_0059B870(CZSealBody *seal, const CPnt3D *p)
{
    CZBodyPart *root = seal->GetRootPart();

    if (seal->IsCurrentBlend((AnimType)data_003DEF08)) {
        if (seal->m_1360 > 0.0f)
            return true;
        seal->m_fac += -seal->m_133c * seal->m_2e0;
        if (seal->m_fac < 2.2f)
            seal->m_fac = 2.2f;
        if (seal->m_bit9)
            seal->m_alt += p->y - root->m_pos.y;
        else if (seal->m_fac <= 2.2f) {
            seal->SetAirborne(false);
            seal->m_133c = 0.0f;
        }
    } else if (seal->IsCurrentBlend((AnimType)data_003DEF50))
        root->m_pos.y = 11.440001f;
    else {
        if (seal->m_bit9)
            seal->m_alt += p->y - root->m_pos.y;
        if (FollowsRoot(seal))
            seal->m_alt += p->y - root->m_pos.y;
    }
    return true;
}

/* a remote SEAL unmade: its node's name dropped, its projectiles and its state cleared, dead, no
 * team, its id invalid, state 8, the team valves refreshed, its squad told (OVL2 0x5998B0; SOCOM
 * 1's with the name and the unit's clean-up) */
void CZSealBody::Destroy(CZSealBody *seal)
{
    if (!seal)
        return;
    unsigned int id = seal->m_id;
    bool remote = seal->m_control ? ((CSealCtrl *)seal->m_control)->IsRemote() : false;

    if (!remote)
        return;
    seal->m_node->SetName("");
    if (id == CEntity::invalidId)
        return;
    if (CZWeapon::m_pProjectilelist)
        CZWeapon::m_pProjectilelist->Clear(id, (EQUIP_ITEM)255);
    CZSealState::SealDeleteCleanup(id);
    seal->SetHealth(0.0f, DEATH_NONE);
    seal->m_time_of_death = 0.0f;
    seal->SetTeamMask(0);
    seal->m_id = CEntity::invalidId;
    seal->BodySetState(8);
    theMission.RefreshTeamValves();
    CSealUnit *unit = ((CSealCtrl *)seal->m_control)->m_unit;

    if (unit) {
        if (theNetwork.m_8)
            unit->Notify(seal, UNIT_MESSAGE_DEATH, 0);
        CSealUnit::SealLeaveUnit(seal);
    }
}

/* (CCharacterType::UnPack, the packed weapons read into a type -- SOCOM 1's UnPack__14CCharacterType
 * FPC20CCharacterWeapPackedP14CCharacterType, called here where SOCOM 1's CreateRemoteSeal calls it;
 * OVL2 0x53EC60 -- is zchar.h's, called by name) */

/* another console's SEAL made: a network controller newed, the character type taken -- for the two
 * kinds the flags name the type itself, else a temporary copy named for the client with the packed
 * weapons read into it -- the SEAL created, then its client index, id, vehicle and name set
 * (OVL2 0x599690; SOCOM 1's with Create no longer expanded in it, and a bit test where SOCOM 1's
 * compared the type's number with 17 and 28) */
CZSealBody *CZSealBody::CreateRemoteSeal(int index, int id, const char *name, int vehicle, char *charname,
                                         unsigned int flags, unsigned char *packed)
{
    CSealCtrl *ctrl = new CRemoteCtrl;
    CCharacterType *type = CCharacterType::GetByNameMod(charname);
    CZSealBody *seal;

    if (flags & ((1 << 17) | (1 << 28)))
        seal = Create(type, name, ctrl);
    else {
        char buf[32];

        sprintf(buf, "netchar%d", index);
        CCharacterType *t = CCharacterType::Create(buf, type, true);

        t->m_istemporary = 1;
        CCharacterType::UnPack((const CCharacterWeapPacked *)packed, t);
        seal = Create(t, name, ctrl);
    }
    if (seal) {
        seal->m_id = id;
        seal->m_ClientIndex = index;
        seal->SetJoinedUnit(false);
        seal->m_vehicle_index = vehicle;
        seal->SetDisplayName(name);
        seal->m_movespeedmod = 1.0f;
        seal->m_full_update_time = 0;
    }
    return seal;
}

/* a SEAL's stats as RemoteRespawn keeps them: thirty counters, the score, three more and the hit percent
 * member by member (CSealStats' layout; MW scalarises the assigned local where CSealStats' array is copied as
 * a block) */
struct CSealStatsCopy {
    unsigned short m_0, m_2, m_4, m_6, m_8, m_a, m_c, m_e, m_10, m_12, m_14, m_16, m_18, m_1a, m_1c, m_1e, m_20,
        m_22, m_24, m_26, m_28, m_2a, m_2c, m_2e, m_30, m_32, m_34, m_36, m_38, m_3a;
    short m_score;
    unsigned short m_3e, m_40, m_42;
    float m_hit_percent;
};

extern "C" {
extern unsigned char data_0045A0C1;
/* the place a respawning SEAL takes (OVL1 0x2B8100: SOCOM 1's findAlign__FP10CZSealBodyP6CPnt3DP7CMatrix, a
 * kind more), which side it is on (0x2C30B0), and its state told the network (0x2BD640: SOCOM 1's
 * CZSealState::UpdateLocalSeal) */
void func_002B8100(CZSealBody *seal, CPnt3D *pos, CMatrix *m, int kind);
bool func_002C30B0(const CZSealBody *seal);
void func_002BD640(CEntity *seal, int x);
bool func_002C2FD0(CZSealBody *seal);
}

/* a vehicle entry by index, or none (an inline, the name ours) */
static inline CVehicleRdrEntry *VehicleEntry(CVehicleRdr *rdr, unsigned int i)
{
    return i < rdr->size() ? &(*rdr)[i] : 0;
}

/* another console's SEAL brought back (OVL2 0x598B90, this unit's first function; SOCOM 1's RemoteRespawn):
 * online only; its packed weapons read into its type unless a hostage or escort or a player's; made over with
 * its two stats and its AI state kept -- the player's at the place of its side and the respawn's kind (the point
 * put in the matrix's last row), the AI's at its vehicle entry's start, the remote's the same way or from its
 * controller's kept place -- its alive valve opened, its fade reset. The two stats assigned from a member by
 * member view: MW scalarises the locals (34 shorts and a float, each its own register or 16-byte slot) where
 * CSealStats' array would be copied as a block */
void CZSealBody::RemoteRespawn(bool first)
{
    if (!data_0045A0C1)
        return;
    unsigned int team = m_team;

    if (!(team & 0x10000000) && !(team & 0x20000)) {
        if (!(m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false)) {
            const CCharacterWeapPacked *packed = (const CCharacterWeapPacked *)CZSealState::GetPackedWeaponData(this);

            if (packed) {
                CCharacterType *t = m_charType;

                CCharacterType::UnPack(packed, t);
                t->m_istemporary = 1;
            }
        }
    }
    CPnt3D pos(m_posW);
    CMatrix mat(m_node->m_matrix);
    CSealStatsCopy stats;
    CSealStatsCopy stats2;

    stats = *(CSealStatsCopy *)&m_stats;
    stats2 = *(CSealStatsCopy *)&m_stats2;
    unsigned int net = ((CSealCtrl *)m_control)->GetAiNetState();

    ReCreate(this, 0);
    *(CSealStatsCopy *)&m_stats = stats;
    *(CSealStatsCopy *)&m_stats2 = stats2;
    int kind;

    if (first)
        kind = func_002C30B0(this) ? 3 : 1;
    else
        kind = func_002C30B0(this) ? 2 : 0;
    if (m_control ? ((CSealCtrl *)m_control)->IsPlayer() : false) {
        func_002B8100(this, &pos, &mat, kind);
        mat.m_matrix[3][0] = pos.x;
        mat.m_matrix[3][1] = pos.y;
        mat.m_matrix[3][2] = pos.z;
        TeleportTo(&mat);
        ((CSealCtrl *)m_control)->OnMissionStart();
        func_002BD640(this, 0);
    } else if (m_control ? ((CSealCtrl *)m_control)->IsAi() : false) {
        CVehicleRdr *rdr = theMission.m_vehicles;
        CVehicleRdrEntry *entry = rdr ? VehicleEntry(rdr, m_vehicle_index) : 0;

        if (entry)
            entry->GetStartMatrix(mat, -1);
        TeleportTo(&mat);
        ((CSealCtrl *)m_control)->GetSealCtrlAi()->InitAiBrain();
        func_002BD640(this, 0);
    } else if (m_control ? ((CSealCtrl *)m_control)->IsRemote() : false) {
        CSealCtrl *remote = (CSealCtrl *)((CSealCtrl *)m_control)->GetRemoteControl();

        if (func_002C2FD0(this))
            SetHealth(0.0f, (SEAL_DEATH_TYPE)6);
        if (!first) {
            CVehicleRdr *rdr = theMission.m_vehicles;

            if (m_vehicle_index) {
                if (rdr) {
                    CVehicleRdrEntry *entry = VehicleEntry(rdr, m_vehicle_index);

                    if (entry)
                        entry->GetStartMatrix(mat, -1);
                    TeleportTo(&mat);
                    CPnt3D *const k = (CPnt3D *)mat.m_matrix[3];

                    pos = *k;
                }
            } else {
                func_002B8100(this, &pos, &mat, kind);
                mat.m_matrix[3][0] = pos.x;
        mat.m_matrix[3][1] = pos.y;
        mat.m_matrix[3][2] = pos.z;
                TeleportTo(&mat);
            }
            *(CPnt3D *)((char *)remote + 0x1fc) = pos;
        } else
            TeleportTo((CPnt3D *)((char *)remote + 0x1fc), (CQuat *)((char *)remote + 0x210));
        remote->SetAiNetState(net);
    }
    CSealUnit *unit = ((CSealCtrl *)m_control)->m_unit;

    if (unit)
        unit->Revive(this);
    char name[100];

    sprintf(name, "%s_valve_alive", m_node->m_name);
    CValve *v = CValve::GetByName(name);

    if (v)
        v->SetValue(1);
    m_full_update_time = 0;
    m_movespeedmod = 1.0f;
    theMission.RefreshTeamValves();
    m_needs_respawn = 0;
}

/* a SEAL brought back: an AI one at a random start of its vehicle entry's respawning set -- the
 * location's cell centre, turned the start's way -- made over, put there, its controller marked
 * respawned, the entry's AI parameters taken but the respawn count, the entry kept and the brain
 * restarted; with no start to be had the count cleared; any other made over where it stands; the
 * respawn asked no more (OVL2 0x5994A0; SOCOM 1's DoRespawn, the start the entry's now, not one out
 * of the player's sight) */
void CZSealBody::DoRespawn()
{
    /* (the controller's test written out as a ternary: IsAi()'s if-assign threads the zero past the
     * andi; the entry and the flag declared before the controller, the flag set after it -- the
     * target's s2/s1/s0) */
    if (m_control ? ((CSealCtrl *)m_control)->IsAi() : false) {
        CVehicleRdrEntry *entry;
        bool failed;
        CSealCtrlAi *ai = ((CSealCtrl *)m_control)->GetSealCtrlAi();

        entry = ai->m_aiParams.m_veh;
        failed = true;

        entry->Randomize(false);
        if (entry->m_respawn.m_count) {
            const char *name = entry->GetRespawnPos();
            int dir = entry->GetRespawnDir();
            CAiMapLoc loc;
            CPnt3D pos;
            CQuat q;

            if (AiMaps.GetLocation(name, loc) && AiMaps.GetCellCenter(loc, &pos)) {
                CAiMap::GetDirQuat((CAiMap::DIR)dir, q, true);
                ReCreate(this, 0);
                TeleportTo(&pos, &q);
                ai->m_has_respawned = true;
                unsigned int respawn = ai->m_aiParams.respawn;

                ai->m_aiParams = entry->m_aiParams;
                ai->m_aiParams.m_veh = entry;
                ai->m_aiParams.respawn = respawn;
                ai->InitAiBrain();
                failed = false;
            }
        }
        if (failed)
            ai->m_aiParams.respawn = 0;
    } else
        ReCreate(this, 0);
    m_needs_respawn = 0;
}

/* a node's tree counted, the node and its children's trees (OVL2 0x59A450: the map's FindGear was
 * SOCOM 1's by position only; Create prints the count). MW inlines the recursion into itself twice:
 * the outer copy walks the child vector inline, the middle calls begin/end/== and the postfix ++'s
 * pieces, the inner every piece -- OVL2's copies of CNodeVector's iterator pieces, 0x59A630-0x59A720,
 * after it */
extern "C" int func_0059A450(zdb::CNode *node)
{
    int n = 1;
    zdb::CNodeVector &children = node->GetChildren();
    zdb::CNodeVector::iterator i;

    for (i = children.begin(); i != children.end(); i++)
        n += func_0059A450(*i);
    return n;
}

/* a gear by name, or none: the made gears walked (OVL2 0x59A730; SOCOM 1's FindGear walked a list,
 * SOCOM II's walks a vector -- zchar.h's CCharacterType::m_gear_list) */
CCharacterGear *CZSealBody::FindGear(const char *name)
{
    std::vector<CCharacterGear *> &gears = CCharacterType::m_gear_list;

    for (std::vector<CCharacterGear *>::iterator i = gears.begin(); i != gears.end(); ++i) {
        const char *n = (*i)->m_name;

        if (name && !strcmp(name, n))
            return *i;
    }
    return 0;
}

/* a node's two visual flags set together (an inline: one `(b & 1)` serves both stores) */
static inline void SetHasVisuals(zdb::CNode *node, bool on)
{
    node->m_hasDI = on;
    node->m_hasVisuals = on;
}

/* a SEAL made: its type taken and its model made, the body built and entered as an entity, its
 * controller fitted; a player's gets a camera-effects node; the node count printed for the log
 * (OVL2 0x5999E0; SOCOM 1's with the camera node) */
CZSealBody *CZSealBody::Create(const CCharacterType *type, const char *name, CSealCtrl *ctrl)
{
    if (!type)
        return 0;
    zdb::CNode *node = 0;
    const CCharacterType *t = PreCreateSeal(type, name, &node);

    if (!t)
        return 0;
    CZSealBody *seal = new CZSealBody(node, (CCharacterType *)t);

    CEntity::CreateEntity(seal, true);
    PostCreateSeal(seal, ctrl);
    if (seal->IsPlayer()) {
        zdb::CNode *fx = new zdb::CNode;

        if (fx) {
            fx->SetName("camera_effects");
            SetHasVisuals(fx, false);
            if (fx)
                seal->GetNode()->AddChild(fx);
        }
    }
    char log[64];

    sprintf(log, "CZSealBody::Create(%s) %d nodes END", seal->m_node->m_name, func_0059A450(seal->GetNode()));
    return seal;
}

extern "C" {
/* the SEALs made so far (OVL2 bss 0x66B420), shadows' switch (OVL1 data 0x3DF138, a byte) and the
 * kit's step after the items (OVL2 0x5C7830) */
extern int data_0066B420;
extern unsigned char data_003DF138;
void func_005C7830(CZKit *kit);
}

/* the task's select (seal_tasks.cpp's CTSelect::Create, OVL2 0x5D29F0) */
struct CTSelect {
    static void *Create(CZSealBody *seal);
};

/* what PostCreateSeal reads of the world: a shadow's owner and the clutter's animations (zdb::CWorld at 0x56C
 * and 0x6AC; a view) */
struct CWorldViews {
    unsigned char _0[0x56c];
    void *m_shadow_node;   // 0x56c
    unsigned char _570[0x6ac - 0x570];
    zdb::CClutterAnimManager *m_clutter_anims;   // 0x6ac
};

/* a box made of two corners (a stand-in: zmath.h's CBBox has no constructor; the first corner's copy through
 * an ld, the second's a member's) */
struct CBoxInit {
    CPnt3D m_min;
    CPnt3D m_max;

    CBoxInit(const CPnt3D &a, const CPnt3D &b) : m_min(a), m_max(b) {}
};

/* the SEAL finished as made: the bounds and the controller set, the event and the task's select made, the
 * gear added, the kit given each weapon's model and ammunition -- empty slots skipped --, the aim reset, the
 * controller started; a clutter effector as wide as the body and a shadow for a player when the world has
 * none yet (OVL2 0x599F00: SOCOM 1's PostCreateSeal, the effector's width and the shadow new).
 * The round counts are read twice and held across GetAmmo by IRO_CommonSubs, which numbers them above the
 * loop's scalarised iterator (s2): named locals would number below it. `fabsf(size.z > r)` is the disc's own
 * oddity: the comparison's int, unsigned to float, its absolute value tested against no constant */
void CZSealBody::PostCreateSeal(CZSealBody *seal, CSealCtrl *ctrl)
{
    seal->m_HudUpdateViewCone = 1;

    zdb::CNode *node = seal->GetNode();

    node->SetNodeEx((zdb::CNodeEx *)seal->m_ex, true);
    {
        CBoxInit t(CPnt3D(-12.0f, 0.0f, -14.0f), CPnt3D(12.0f, 20.0f, 14.0f));

        node->m_bbox = (const CBBox &)t;
    }
    seal->SetController(ctrl);
    seal->CreateAiEvent();
    seal->m_select = CTSelect::Create(seal);

    CCharacterType *type = seal->m_charType;

    data_0066B420++;

    CCharacterGearList &gear = type->m_gear;

    for (CCharacterGearList::iterator g = gear.begin(); g != gear.end(); ++g)
        seal->AddItem(*g);
    seal->EnableMPBombGear(false);
    seal->SetupEyes();
    seal->SetupGear();

    CZKit *kit = &seal->m_kit;

    if (kit) {
        kit->InitItemModel("null", "null");

        CCharacterWeapList &weapons = seal->m_charType->m_weapons;
        int slot = 0;

        for (CCharacterWeapList::iterator w = weapons.begin(); w != weapons.end(); ++w) {
            bool ok = kit->SetItem(slot, w->m_wepId);

            if (ok) {
                CZWeapon *item = (CZWeapon *)kit->GetItemByIndex(slot);

                if (item) {
                    char model[1024];

                    strcpy(model, item->GetModelName());
                    strlen(model);

                    const char *name = "grenade";

                    switch (CZKit::GetItemFireClass(item->m_ID)) {
                    case 1:
                        name = "rifle";
                        break;
                    case 2:
                        name = "pistol";
                        break;
                    case 4:
                        name = "launcher";
                        break;
                    }
                    kit->InitItemModel(model, name);
                }

                if (w->m_ammoCount * w->m_magCount)
                    kit->SetItemAmmo(slot, CZAmmo::GetAmmo(w->m_ammoId), w->m_ammoCount, w->m_magCount);
                else
                    kit->SetItemAmmo(slot, CZAmmo::GetAmmo(w->m_ammoId));
                slot++;
            }
        }
        kit->SetCurReticule(false);
        func_005C7830(kit);
    }
    seal->m_220 = 0;
    seal->m_224 = 0;
    seal->m_228 = 0;
    seal->SetAimDirR(&CPnt3D::unit_nz, false);
    seal->m_fd1 = 0;
    ctrl->Initalize();
    if (((CWorldViews *)zdb::CWorld::m_world)->m_clutter_anims) {
        const CBBox *box = node->GetBBox();
        CPnt3D size;

        box->GetSize(&size);

        float r = fabsf(size.x);

        if (fabsf(size.z > r))
            r = size.z;
        r *= 0.5f;
        ((CWorldViews *)zdb::CWorld::m_world)->m_clutter_anims->RegisterEffector(seal,
            (const CPnt3D *)node->m_matrix.m_matrix[3], r, &seal->m_velW);
    }
    if (data_003DF138 && seal->IsPlayer() && !((CWorldViews *)zdb::CWorld::m_world)->m_shadow_node) {
        node->m_socom2_a1_bits.bit5 = 1;
        zdb::CWorld::m_world->AddShadow(node);
    }
    seal->m_useCachedReticuleSeal = false;
    seal->m_cachedReticuleSeal = 0;
}

/* the camera, only what ReCreate asks of it (camera.h's class, which this unit does not include) */
class CAppCamera {
public:
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    void ResetDeathCam();
};

extern CAppCamera *appCamera;

/* the player (fts_player.cpp's CftsPlayer, OVL1 bss 0x440C38: its SEAL first) */
class CftsPlayer {
public:
    CZSealBody *m_body;   // 0x0
    int m_4;   // 0x4
};

extern CftsPlayer ftsPlayer;

extern "C" {
/* a SEAL a spectator may not watch online (OVL1 0x2C2FD0, unowned: hud_tcmcmds.cpp's reading), what a
 * player online begins as when one is, the node's fade set (OVL2 0x543D50, 0x552780: the priority a byte), the
 * kit's hand mode and the byte after it read, the byte set (OVL2 0x5C4960, 0x5C4890, 0x5C48A0) */
bool func_002C2FD0(CZSealBody *seal);
void func_00543D50(CZSealBody *seal);
void func_00552780(CZSealBody *seal, unsigned char priority, float target, float rate);
int func_005C4890(CZKit *kit);
void func_005C48A0(CZKit *kit, int value);
}

/* an entity's team and vehicle index read (inlines: the values take their temporaries, numbered above every
 * named local and spilled to 16-byte slots: ReCreate) */
static inline unsigned int GetTeamOf(const CEntity *e) { return e->m_team; }
static inline unsigned int GetVehicleOf(const CEntity *e) { return e->m_vehicle_index; }

/* a SEAL made again in place: its camera node taken off and kept, its id, team, vehicle, kit modes, matrix,
 * name and display name carried across the destructor and the constructor, the player's camera put back on
 * it (OVL2 0x599B60; SOCOM 1's ReCreate with the camera node, the kit's modes and the online player's fade) */
bool CZSealBody::ReCreate(CZSealBody *seal, const CCharacterType *type)
{
    char *name;
    zdb::CNode *fx = seal->GetNode()->FindChild("camera_effects", true);

    if (fx) {
        zdb::CNode *parent = fx->m_parent;

        if (parent)
            parent->DeleteChild(fx);
    }

    CSealCtrl *ctrl = (CSealCtrl *)seal->m_control;

    seal->m_respawning = 1;
    ctrl->m_HudUpdateWeapon = 1;

    CZKit *kit = &seal->m_kit;

    if (kit)
        kit->SetWeaponInfoChanged();

    unsigned int id = seal->m_id;
    unsigned int team = GetTeamOf(seal);
    unsigned int vehicle = GetVehicleOf(seal);
    int mode = seal->m_kit.GetHandFireMode();
    int mode2 = func_005C4890(&seal->m_kit);
    CMatrix m(seal->m_node->m_matrix);

    if (!type)
        type = seal->m_charType;

    CCharacterType *acquired = type->Acquire();
    name = _strdup(seal->m_node->m_name, 0, 0);
    char *display = seal->m_display_name ? _strdup(seal->m_display_name, 0, 0) : 0;

    seal->~CZSealBody();

    zdb::CNode *node = 0;
    const CCharacterType *t = PreCreateSeal(type, name, &node);

    new (seal) CZSealBody(node, (CCharacterType *)t);
    seal->SetTeamMask(team);
    seal->m_id = id;
    seal->m_bit8 = 1;
    seal->SetIncludeInTargetList(true);
    seal->m_vehicle_index = vehicle;
    seal->SetDisplayName(display);
    seal->m_fd1 = 0;
    seal->m_fd3 = 1;
    seal->m_desiredState = (SEAL_STATE)1;
    PostCreateSeal(seal, ctrl);
    seal->m_kit.PostSetupItems();
    seal->m_kit.SetHandFireMode(mode);
    func_005C48A0(&seal->m_kit, mode2);
    theMission.RefreshTeamValves();
    seal->TeleportTo(&m);
    if (seal->IsPlayer()) {
        ftsPlayer.m_body = seal;
        if (theNetwork.m_9) {
            zdb::CNode *n = seal->GetNode();

            n->m_Opacity = 0.0f;
            if (func_002C2FD0(seal))
                func_00543D50(seal);
            else {
                appCamera->ResetDeathCam();
                func_00552780(seal, 2, 1.0f, 4.0f);
            }
        } else
            appCamera->ResetDeathCam();
        appCamera->AttachTo(seal);
        seal->m_camera = appCamera;
    }
    if (fx) {
        if (fx)
            seal->GetNode()->AddChild(fx);
    }
    acquired->Release();
    _free(name, 0, 0);
    _free(display, 0, 0);
    return true;
}

/* the character type taken and its model instanced: dynamic, its LODs made, added to the world,
 * flagged, its textures added; none without a type or a model (OVL2 0x59A7F0; SOCOM 1's with a
 * second flag) */
#pragma auto_inline off
const CCharacterType *CZSealBody::PreCreateSeal(const CCharacterType *type, const char *name, zdb::CNode **node)
{
    const CCharacterType *t;

    if ((t = type->Acquire()) != 0) {
        *node = ((CCharacterType *)t)->CreateInstance(name);
        zdb::CNode *n = *node;

        if (n) {
            n->SetDynamic(true, true);
            ((CCharacterType *)t)->CreateLODs(n);
            zdb::CWorld::m_world->AddChild(n);
            n->m_socom2_a1_bits.bit2 = 1;
            n->m_bumpmap = 1;
            zdb::CWorld::m_world->AddTextureAssetCharacter("clib", n);
        } else
            return 0;
    }
    return t;
}
#pragma auto_inline reset

/* the SEAL a node belongs to: its extension's, when that is a SEAL's (SOCOM 1's, the unit's last;
 * OVL2 0x59ACD0 -- the body-part commands' ticks below inline it) */
CZSealBody *CZSealBody::GetSeal(zdb::CNode *node)
{
    if (node) {
        CSealEx *ex = dynamic_cast<CSealEx *>(node->m_nodeEx);

        if (ex)
            return ex->m_seal;
    }
    return 0;
}

extern "C" {
/* the height above the ground a SEAL counts as in the air from (OVL1 bss 0x44C264), the probe for
 * ground under a falling SEAL and the landing's hit taken (seal.cpp's OVL2 0x54A8B0 and 0x5B0C10) */
extern float data_0044C264;
bool func_0054A8B0(CZSealBody *seal);
void func_005B0C10(CZSealBody *seal, zdb::IntersectStruct *hit);
}

/* whether the SEAL is in the air over a point: in the 0x3DEF08 anim, or with no air bit yet, at the
 * in-air height above the point it is airborne (the landed bit 8 cleared, and under the anim bit 9 set and 10
 * cleared); else, out of a flagged anim and played here, the ground probed when it is 5 above (3
 * prone); a SEAL with bit 9 and not 10 that sinks under the point while m_1360 has run out is set on
 * it -- bits 10 and 8 set, 9 cleared, the landing taken (OVL2 0x59AD30, new in SOCOM II: 0x5B0420
 * calls it; after GetSeal -- the name unknown) */
extern "C" bool func_0059AD30(CZSealBody *seal, const CPnt3D *pos)
{
    float h = data_0044C264;

    if (seal->IsCurrentBlend((AnimType)data_003DEF08) && !(seal->m_alt < pos->y + h)) {
        seal->SetAirborne(true);
        seal->SetLanded(false);
        seal->SetBit9(true);
        seal->SetBit10(false);
    }
    if (!seal->m_airborne) {
        if (!(seal->m_alt < pos->y + h)) {
            seal->SetAirborne(true);
            seal->SetLanded(false);
            if (!func_0057B510(seal) && !seal->IsRemote())
                func_0054A8B0(seal);
        } else if (!func_0057B510(seal) && !seal->IsRemote()) {
            float d = 5.0f;

            if (seal->m_state == stateProne)
                d = 3.0f;
            if (!(seal->m_alt <= pos->y + d))
                func_0054A8B0(seal);
        }
    }
    if (seal->m_bit9 && !seal->m_bit10) {
        float y = pos->y;

        if (seal->m_alt < y && seal->m_1360 <= 0.0f) {
            seal->m_alt = y;
            seal->SetBit9(false);
            seal->SetBit10(true);
            seal->SetLanded(true);
            func_005B0C10(seal, 0);
        }
    }
    return seal->m_airborne;
}

/* the body-part commands (new in SOCOM II, ZAnim commands this unit registers at 0x59AC60:
 * RESET_BODY_PARTS, parsed at 0x59ABC0 and ticked at 0x59AB30, and BODY_FALL_ON_MATERIAL_SOUND,
 * parsed at 0x59AA60 and ticked at 0x59A8E0; the functions' names unknown) -- a timeless command
 * holding a node reference */
struct _body_node_cmd {
    _zanim_cmd_hdr hdr;   // 0x0
    unsigned char node;   // 0x4: the node's reference in the current anim
};

extern "C" {
_zanim_cmd_hdr *func_0059ABC0(_zrdr *rdr);
bool func_0059AB30(_zanim_cmd_hdr *hdr, float *dT);
_zanim_cmd_hdr *func_0059AA60(_zrdr *rdr);
bool func_0059A8E0(_zanim_cmd_hdr *hdr, float *dT);
}

/* the two commands registered, neither with a begin or an end (OVL2 0x59AC60: CMission::Init calls
 * it) */
extern "C" bool func_0059AC60()
{
    ZAnim.AddCmd("RESET_BODY_PARTS", func_0059ABC0, 0, func_0059AB30, 0);
    ZAnim.AddCmd("BODY_FALL_ON_MATERIAL_SOUND", func_0059AA60, 0, func_0059A8E0, 0);
    return true;
}

/* (OVL1 0x26E2D0, zanim_anim.cpp's: a ZRDR path's node reference, searched as the current anim
 * searches) */
extern "C" int func_0026E2D0(CZAnim *anim, _zrdr *rdr);

/* RESET_BODY_PARTS: the node is the command's own argument (OVL2 0x59ABC0) */
extern "C" _zanim_cmd_hdr *func_0059ABC0(_zrdr *rdr)
{
    _body_node_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_body_node_cmd *)_calloc(1, sizeof(_body_node_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_body_node_cmd);
    cmd->hdr.timeless = 1;
    cmd->node = func_0026E2D0(ZAnim.m_CurAnim, rdr);
    return (_zanim_cmd_hdr *)cmd;
}

/* RESET_BODY_PARTS run: the auxiliary parts of the node's SEAL oriented, if the node is a SEAL's
 * (OVL2 0x59AB30) */
extern "C" bool func_0059AB30(_zanim_cmd_hdr *hdr, float *dT)
{
    zdb::CNode *node = ZAnim.m_CurAnim->GetNode(((_body_node_cmd *)hdr)->node, false);

    if (node) {
        CZSealBody *seal = CZSealBody::GetSeal(node);

        if (seal)
            seal->OrientAuxParts();
    }
    return true;
}

/* BODY_FALL_ON_MATERIAL_SOUND: the node under the BODY_NODE tag, the command dropped without one
 * (OVL2 0x59AA60) */
/* (zrdr.h's: a block's tag) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);

extern "C" _zanim_cmd_hdr *func_0059AA60(_zrdr *rdr)
{
    _body_node_cmd *cmd;
    _zrdr *tag;

    if (!rdr)
        return 0;
    cmd = (_body_node_cmd *)_calloc(1, sizeof(_body_node_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_body_node_cmd);
    cmd->hdr.timeless = 1;
    if ((tag = zrdr_findtag(rdr, "BODY_NODE")) != 0)
        cmd->node = func_0026E2D0(ZAnim.m_CurAnim, tag);
    else {
        _free(cmd, 0, 0);
        cmd = 0;
    }
    return (_zanim_cmd_hdr *)cmd;
}

/* a material's extension (int_material.h's CMaterialEx, which zweapon.h's stand-in of the table
 * leaves incomplete -- seal_env.cpp's view, with the fall sound): only the sounds */
struct CMaterialEx {
    char *m_name;   // 0x0
    unsigned char _4[0x18 - 0x4];
    CSnd *m_fallsound;   // 0x18: FALLSOUND
};

/* a material's extension, the first one for an index past the table (seal_env.cpp's) */
static inline CMaterialEx *MaterialEx(unsigned int i)
{
    if (i >= CMaterialExArray::m_materialex_list.size())
        i = 0;
    return (CMaterialEx *)CMaterialExArray::m_materialex_list[i];
}

/* BODY_FALL_ON_MATERIAL_SOUND run: the fall sound of the material under the node's SEAL, or
 * ".FALL_GROUND", heard flat by the player looking through his own eyes, else at the SEAL (OVL2
 * 0x59A8E0: HandleFootfallSound's shape, 0x5A39B0) */
extern "C" bool func_0059A8E0(_zanim_cmd_hdr *hdr, float *dT)
{
    zdb::CNode *node = ZAnim.m_CurAnim->GetNode(((_body_node_cmd *)hdr)->node, false);

    if (node) {
        CZSealBody *seal = CZSealBody::GetSeal(node);

        if (seal) {
            int mat = seal->m_onMaterial;

            if (mat < (int)CMaterialExArray::m_materialex_list.size()) {
                CMaterialEx *ex = MaterialEx(mat);

                if (ex) {
                    CSnd *snd = ex->m_fallsound;

                    if (!snd)
                        snd = CSnd::GetSoundByName(".FALL_GROUND");
                    if (snd) {
                        if (seal->IsPlayer() && seal->m_zoomstate > ZOOMSTATE_3RDPERSON)
                            snd->PlaySound((CSndInstance *)0, 255);
                        else
                            snd->PlaySound3D(&seal->m_posW, 1.0f, 0, 0, 255);
                    }
                }
            }
        }
    }
    return true;
}
