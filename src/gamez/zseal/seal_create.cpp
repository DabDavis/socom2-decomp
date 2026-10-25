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
