/* seal_create.cpp -- the SEAL made and unmade: respawns, the remote SEAL, Create and ReCreate, the
 * gear, the character's model (SOCOM 1's unit, __sinit_seal_create.cpp; SOCOM 1's order from
 * RemoteRespawn to GetSeal). OVL2 0x598B90 (RemoteRespawn, after seal_control.cpp's template
 * tail: DrawFunc<CDynGrenade> at 0x598860) to seal_dyn.cpp (0x59BA80, CharacterDynamics::Load).
 * Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include <stdio.h>

/* (CCharacterType::Acquire and CreateLODs -- SOCOM 1's Acquire__14CCharacterTypeCFv and
 * CreateLODs__14CCharacterTypeFPQ23zdb5CNode, called here where SOCOM 1's PreCreateSeal calls them;
 * OVL2 0x53CCA0, 0x53CB20 -- are zchar.h's, called by name) */

class CZSealState {
public:
    static void SealDeleteCleanup(int id);
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
};

extern CMission theMission;

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
