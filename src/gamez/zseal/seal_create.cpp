/* seal_create.cpp -- the SEAL made and unmade: respawns, the remote SEAL, Create and ReCreate, the
 * gear, the character's model (SOCOM 1's unit, __sinit_seal_create.cpp; SOCOM 1's order from
 * RemoteRespawn to GetSeal). OVL2 0x598B90 (RemoteRespawn, after seal_control.cpp's template
 * tail: DrawFunc<CDynGrenade> at 0x598860) to seal_dyn.cpp (0x59BA80, CharacterDynamics::Load).
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"

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
/* CSealUnit::Notify (the kit lane's 0x5ED7B0) and a unit's own clean-up of the SEAL (0x5EDF90) */
void func_005ED7B0(CSealUnit *unit, CZSealBody *seal, int message, void *data);
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
            func_005ED7B0(unit, seal, 1, 0);
        func_005EDF90(seal);
    }
}

/* the character type taken and its model instanced: dynamic, its LODs made, added to the world,
 * flagged, its textures added; none without a type or a model (OVL2 0x59A7F0; SOCOM 1's with a
 * second flag) */
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
