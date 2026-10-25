/* seal_create.cpp -- the SEAL made and unmade: respawns, the remote SEAL, Create and ReCreate, the
 * gear, the character's model (SOCOM 1's unit, __sinit_seal_create.cpp; SOCOM 1's order from
 * RemoteRespawn to GetSeal). OVL2 0x598B90 (RemoteRespawn, after seal_control.cpp's template
 * tail: DrawFunc<CDynGrenade> at 0x598860) to seal_dyn.cpp (0x59BA80, CharacterDynamics::Load).
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include <stdio.h>

extern "C" {
/* CCharacterType::Acquire and CreateLODs (SOCOM 1's Acquire__14CCharacterTypeCFv and
 * CreateLODs__14CCharacterTypeFPQ23zdb5CNode, called here where SOCOM 1's PreCreateSeal calls
 * them; OVL2 0x53CCA0, 0x53CB20 -- the AI-core lane's range) */
const CCharacterType *func_0053CCA0(const CCharacterType *type);
void func_0053CB20(const CCharacterType *type, zdb::CNode *node);
}

class CZSealState {
public:
    static void SealDeleteCleanup(int id);
};

extern "C" {
/* CZWeapon's projectile list (OVL1 bss 0x4B5220, data.txt's m_pProjectilelist__8CZWeapon) and its
 * clear of an owner's projectiles (SOCOM 1's Clear__16CZProjectileListFi, a further int in SOCOM
 * II; OVL1 0x3CCEB0) */
extern CZProjectileList *data_004B5220;
void func_003CCEB0(CZProjectileList *list, int id, int a);
/* CMission::RefreshTeamValves (SOCOM 1's, called here; OVL1 0x2B23A0) on theMission (0x4364E0) */
void func_002B23A0(void *mission);
extern unsigned char data_004364E0[];
/* a squad's own clean-up of the SEAL (OVL2 0x5EDF90, the kit lane's range) */
void func_005EDF90(CZSealBody *seal);
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
    if (data_004B5220)
        func_003CCEB0(data_004B5220, id, 255);
    CZSealState::SealDeleteCleanup(id);
    seal->SetHealth(0.0f, DEATH_NONE);
    seal->m_time_of_death = 0.0f;
    seal->SetTeamMask(0);
    seal->m_id = CEntity::invalidId;
    seal->BodySetState(8);
    func_002B23A0(data_004364E0);
    CSealUnit *unit = ((CSealCtrl *)seal->m_control)->m_unit;

    if (unit) {
        if (theNetwork.m_8)
            unit->Notify(seal, UNIT_MESSAGE_DEATH, 0);
        func_005EDF90(seal);
    }
}

extern "C" {
/* CCharacterType::UnPack: the packed weapons read into a type (SOCOM 1's UnPack__14CCharacterType
 * FPC20CCharacterWeapPackedP14CCharacterType, called here where SOCOM 1's CreateRemoteSeal calls it;
 * OVL2 0x53EC60, the AI-core lane's range) */
void func_0053EC60(const unsigned char *packed, CCharacterType *type);
}

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
        func_0053EC60(packed, t);
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

/* a node's tree counted (OVL2 0x59A450: a walk of the child vector; the map's FindGear was SOCOM
 * 1's by position only) */
extern "C" int func_0059A450(zdb::CNode *node);

/* an iterator's `==` (the copy at 0x59A630 is a draft's) */
static inline bool IterEqual(void *const *a, void *const *b)
{
    return *a == *b;
}

/* an iterator's `!=` as `!(==)`, materialised (OVL2 0x59A680: one of the small iterator copies
 * after 0x59A450, which calls it twice -- a stand-in under the placeholder until that walk is
 * written) */
extern "C" bool func_0059A680(void *const *a, void *const *b)
{
    return !IterEqual(a, b);
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

/* the character type taken and its model instanced: dynamic, its LODs made, added to the world,
 * flagged, its textures added; none without a type or a model (OVL2 0x59A7F0; SOCOM 1's with a
 * second flag) */
#pragma auto_inline off
const CCharacterType *CZSealBody::PreCreateSeal(const CCharacterType *type, const char *name, zdb::CNode **node)
{
    const CCharacterType *t;

    if ((t = func_0053CCA0(type)) != 0) {
        *node = ((CCharacterType *)t)->CreateInstance(name);
        zdb::CNode *n = *node;

        if (n) {
            n->SetDynamic(true, true);
            func_0053CB20(t, n);
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
