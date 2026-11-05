/* pickup_main.cpp -- the pickups, CPickup: the weapons, ammunition, grenades and the MP bomb lying
 * in the world, and their list (SOCOM 1's unit, __sinit_pickup_main.cpp; SOCOM 1's order, Close
 * and Open last). OVL2 0x541650-0x542FD7: SetVisibleByUnit first, Open last, seal.cpp right after
 * (0x542E50 is Close, not the heartbeat). The list and the statics are OVL1 data, so this unit
 * defines none of them. Defined in reverse of their image order (-inline deferred). */

#include "zseal.h"
#include "../zweapon/zweapon.h"
#include <algorithm>
#include <vector>

/* an ammunition pickup's contents: the weapons' stash (zwep_weapon.cpp's CZItemStash, three parallel
 * lists -- a weapon, an ammunition and a count per entry, OVL1's: its rows Compact 0x3CFB20,
 * Count 0x3CFCF0, Get 0x3CFD70, GetAnim 0x3CFB00 -- only what the pickups call; seal_env.cpp's
 * stand-in too) and the pickup's view of it (zseal.h's CPickupAmmo). Its members typed as the
 * weapons' round 23 read them: Destroy's delete runs the implicit destructor, which calls the three
 * members' destructors out of line, last member first -- this unit is the first in OVL2 to ask for
 * them, so their copies follow Destroy (0x5427E0 the weapons', 0x542850 the ammunitions', 0x5428C0
 * the counts') */
class CZItemStash {
public:
    void Compact();
    int Count();
    bool Get(int i, int *a, int *b, int *c);
    CZAnim *GetAnim();

    std::vector<CZWeapon *> m_weapons;   // 0x0
    std::vector<CZAmmo *> m_ammo;   // 0xc
    std::vector<int> m_rounds;   // 0x18
    CZAnim *m_anim;   // 0x24
};

class CPickupAmmo : public CZItemStash {
};

extern "C" {
/* a made pickup told to the other consoles: a weapon by its item, its id, place and turn;
 * ammunition by its kinds' packed ids and rounds (OVL1, unowned) */
void func_002BA660(int item, unsigned int netid, const CPnt3D *pos, CQuat *quat);
void func_002BA3F0(int count, unsigned short *packed, unsigned short *rounds, unsigned int netid,
                   const CPnt3D *pos);
/* (this unit's, below) */
void func_00541700(CPickup *p);
void func_005418B0(CPickup *p);
/* an animation stopped (OVL1, zanim_set.cpp's callers pass false too) */
void func_002707C0(CZAnim *anim, bool now);
}

/* the pickup lists' push_back as SOCOM II's MSL has it: through the base's push_back, which is
 * insert(end(), x) -- SOCOM 1's chain, one level taller than include/list's pointer-list push_back
 * (ai_events.cpp's CAiEvent list, "variant vc"): Destroy's push_back temporaries then take the
 * frame's lowest slots, below the remove's equal_to, as the target's */
template <>
inline void std::list<CPickup *, std::allocator<CPickup *> >::push_back(CPickup *const &x)
{
    std::__list_deleter<const void *, std::allocator<const void *> >::push_back((const void *const &)x);
}

/* what a pickup of kind 10 holds (only its delete is seen: a destructor of its own, empty -- the
 * delete's null test stays behind the case's own) */
struct CPickupData10 {
    ~CPickupData10() {}

    int m_0;
};

/* the distance (squared) past which a pickup whose time ran out goes, and the time it is given
 * again (OVL2 data, 8 bytes apart: MW 3.0's layout of initialised statics) */
static float s_far2 = 90000.0f;
static float s_life = 60.0f;

/* (OVL2 0x542FC0; SOCOM 1's) */
void CPickup::Open(unsigned int bits)
{
    bit_count = bits;
    m_isOpen = true;
}

/* a pool pickup deleted, its slot left null (std::transform's operation) */
struct DeletePickup {
    DeletePickup() {}
    CPickup *operator()(CPickup *p) const
    {
        delete p;
        return 0;
    }
};

/* every pickup destroyed, the MP bomb's node deleted (SOCOM 1's destroyed the bomb's pickup first)
 * and, when told to, the pool's pickups deleted and the pool emptied (OVL2 0x542E50; SOCOM 1's) */
void CPickup::Close(bool all)
{
    while (m_list.size())
        Destroy(m_list.front());
    DeleteMPBombNode();
    if (all) {
        std::transform(m_pool.begin(), m_pool.end(), m_pool.begin(), DeletePickup());
        m_pool.clear();
    }
    m_isOpen = false;
}

/* a pickup made and listed: from the pool when it holds one, cleared, given its kind, node,
 * contents, owner and id, no unit seeing it, unlocked and not secondary, its time never running;
 * a weapon's model shows its scope and hides its thermal one */
CPickup *CPickup::Create(unsigned int owner, PU_TYPE type, zdb::CNode *node, void *data)
{
    if (!m_isOpen)
        return 0;
    CPickup *p = 0;

    if (m_pool.size() == 0)
        p = new CPickup;
    else {
        p = m_pool.front();
        m_pool.pop_front();
    }
    p->m_visible_by_units = 0;
    p->Set(type, node, data, owner, nextPickupId++);
    p->SetLocked(false);
    p->SetSecondary(false);
    p->m_timer = NO_TIMEOUT;
    m_list.push_back(p);
    if (type == PU_WEAPON && node) {
        zdb::CNode *scope = node->FindChild("scope", true);
        zdb::CNode *thermal = node->FindChild("thermal_scope", true);

        if (scope)
            scope->SetActive(true);
        if (thermal)
            thermal->SetActive(false);
    }
    func_00541700(p);
    return p;
}

CPickup *CPickup::NetworkCreate(unsigned int netid, PU_TYPE type, zdb::CNode *node, void *data)
{
    if (!m_isOpen)
        return 0;
    CPickup *p = 0;

    if (m_pool.size() == 0)
        p = new CPickup;
    else {
        p = m_pool.front();
        m_pool.pop_front();
    }
    p->m_visible_by_units = 0;
    p->Set(type, node, data, GetEntityIDFromNetID(netid), netid);
    p->SetLocked(false);
    p->SetSecondary(false);
    p->m_timer = NO_TIMEOUT;
    m_list.push_back(p);
    if (type == PU_WEAPON && node) {
        zdb::CNode *scope = node->FindChild("scope", true);
        zdb::CNode *thermal = node->FindChild("thermal_scope", true);

        if (scope)
            scope->SetActive(true);
        if (thermal)
            thermal->SetActive(false);
    }
    func_005418B0(p);
    return p;
}

/* the list walked, its printing gone (OVL2 0x542DF0; SOCOM 1's, as empty) */
void CPickup::DebugPickups()
{
    iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e; ++i)
        ;
}

/* a listed pickup taken off the list, what it holds let go by its kind -- kind 10's data deleted,
 * a weapon's node taken out of the world, ammunition's stash deleted (its drop anim stopped) and
 * its node too, the MP bomb's node -- the pickup cleared and put back in the pool; one not listed
 * leaves the list walked (OVL2 0x542470; SOCOM 1's, the bomb's node now DeleteMPBombNode's) */
void CPickup::Destroy(CPickup *pickup)
{
    iterator it = std::find(m_list.begin(), m_list.end(), pickup);

    if (it == m_list.end()) {
        DebugPickups();
        return;
    }
    m_list.remove(pickup);
    /* (the kind taken into an enum local: the target loads it into a0 and keeps the pickup in v1) */
    PU_TYPE type = (PU_TYPE)pickup->m_type;

    switch (type) {
    case 10:
        if (pickup->m_data)
            delete (CPickupData10 *)pickup->m_data;
        pickup->m_data = 0;
        break;
    case PU_WEAPON: {
        zdb::CNode *node = pickup->m_node;

        if (node) {
            if (node->m_parent)
                node->m_parent->DeleteChild(node);
            zdb::CWorld::m_world->DeleteTextureAssetCharacter(zdb::CHARSETCOMMON, node);
            zdb::CNode::Delete(node);
        }
        break;
    }
    case PU_AMMO: {
        CZItemStash *stash = (CZItemStash *)pickup->m_data;
        CZAnim *anim = 0;

        if (stash) {
            anim = stash->GetAnim();
            delete stash;
        }
        pickup->m_data = 0;
        zdb::CNode *node = pickup->m_node;

        if (node) {
            if (node->m_parent)
                node->m_parent->DeleteChild(node);
            if (anim)
                func_002707C0(anim, false);
            zdb::CWorld::m_world->DeleteTextureAssetCharacter(zdb::CHARSETCOMMON, node);
            zdb::CNode::Delete(node);
        }
        break;
    }
    case PU_MPBOMB:
        DeleteMPBombNode();
        break;
    }
    pickup->m_data = 0;
    pickup->m_node = 0;
    pickup->m_type = PU_NONE;
    pickup->SetLocked(false);
    pickup->SetSecondary(false);
    pickup->m_owner = 0;
    pickup->m_id = 0;
    pickup->m_bits.ClearAll();
    m_pool.push_back(pickup);
}

/* the online tick: no pickup's time runs out, only the MP bomb's node goes when it is flagged
 * (OVL2 0x5423E0; new in SOCOM II, DeleteMPBombNode inlined) */
void CPickup::NetTick(const CPnt3D *pos, float dT)
{
    if (m_deletempbomb) {
        DeleteMPBombNode();
        m_deletempbomb = false;
    }
}

/* every pickup the owner dropped destroyed (OVL2 0x5421F0; SOCOM 1's compared a byte of the id,
 * SOCOM II's the owner word). The iterator is assigned, so it stays in memory; the end taken
 * once. */
void CPickup::DestroyNetPickups(unsigned int owner)
{
    iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e;) {
        CPickup *p = *i;

        ++i;
        if (owner == p->m_owner)
            Destroy(p);
    }
}

/* (OVL2 0x542180) */
void CPickup::DeleteMPBombNode()
{
    zdb::CNode *node = m_mpbombnode;

    if (node) {
        if (node->m_parent)
            node->m_parent->DeleteChild(node);
        zdb::CWorld::m_world->DeleteTextureAssetCharacter(zdb::CHARSETCOMMON, node);
        zdb::CNode::Delete(node);
        m_mpbombnode = 0;
    }
}

/* the pickups aged: one whose time runs out is destroyed once the point is far from it, or given
 * its time again; a time of -1 never runs (OVL2 0x542290; new in SOCOM II, the name ours) */
void CPickup::TickTimers(const CPnt3D *pos, float dT)
{
    iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e;) {
        CPickup *p = *i;

        ++i;
        if (pos && p->m_timer != NO_TIMEOUT) {
            p->m_timer -= dT;
            if (p->m_timer <= 0.0f) {
                CPnt3D d;

                pos->Sub(p->GetPosW(), &d);
                if (!(d.x * d.x + d.y * d.y + d.z * d.z <= s_far2))
                    Destroy(p);
                else
                    p->m_timer = s_life;
            }
        }
    }
}

/* whether a weapon of the kind lies within the range of a point (OVL2 0x5420B0; new in SOCOM II,
 * the name ours) */
bool CPickup::IsWeaponNear(unsigned char item, const CPnt3D *pos, float range)
{
    iterator it = GetFirst();

    for (CPickup *p = GetNext(it, pos, range, PU_WEAPON); p; p = GetNext(it, pos, range, PU_WEAPON)) {
        CZWeapon *w = p->GetWeapon();

        if (w && item == w->m_ID)
            return true;
    }
    return false;
}

/* the walk's start (OVL2 0x542170; SOCOM 1's) */
CPickup::iterator CPickup::GetFirst()
{
    return m_list.begin();
}

/* the nearest pickup of a kind (any, for none) within the range of a point; a secondary weapon
 * goes to its own slot when asked for, and the special weapons (class 145) are passed over when
 * told to (OVL2 0x541D40; SOCOM 1's with the secondary slot and the flag new) */
CPickup *CPickup::GetClosest(const CPnt3D *pos, float range, PU_TYPE type, CPickup **secondary,
                             bool no_special)
{
    CPickup *closest = 0;

    if (m_list.size()) {
        iterator i = m_list.begin(), e = m_list.end();
        float best = range * range + 1.0f;
        float best2 = best;

        for (; i != e; ++i) {
            CPickup *p = *i;
            PU_TYPE t = (PU_TYPE)p->m_type;

            if (type == PU_NONE || t == type) {
                CPnt3D d;
                float dist;

                p->GetPosW()->Sub(pos, &d);
                dist = d.x * d.x + d.y * d.y + d.z * d.z;
                if (secondary && t == PU_WEAPON && p->m_secondary) {
                    if (dist < best2) {
                        *secondary = p;
                        best2 = dist;
                    }
                } else if (dist < best) {
                    if (!no_special) {
                        best = dist;
                        closest = p;
                    } else {
                        CZWeapon *w = p->GetWeapon();

                        if (!w || w->GetIDType() != 145) {
                            closest = p;
                            best = dist;
                        }
                    }
                }
            }
        }
    }
    return closest;
}

/* the next pickup of a kind (any, for none) within the range of a point, the iterator left past
 * it (OVL2 0x541F10; SOCOM 1's). Out of line: IsWeaponNear calls it, where MW would inline it */
#pragma auto_inline off
CPickup *CPickup::GetNext(iterator &it, const CPnt3D *pos, float range, PU_TYPE type)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        range *= range;
        for (; it != e; ++it) {
            CPickup *p = *it;

            if (type == PU_NONE || p->m_type == type) {
                CPnt3D d;

                p->GetPosW()->Sub(pos, &d);
                if (d.x * d.x + d.y * d.y + d.z * d.z < range) {
                    ++it;
                    return p;
                }
            }
        }
    }
    return 0;
}

#pragma auto_inline reset

/* the next pickup of a kind (any, for none), the iterator left past it (OVL2 0x542030; SOCOM 1's) */
CPickup *CPickup::GetNext(iterator &it, PU_TYPE type)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (; it != e; ++it) {
            CPickup *p = *it;

            if (type == PU_NONE || p->m_type == type) {
                ++it;
                return p;
            }
        }
    }
    return 0;
}

/* a pickup by its network id (OVL2 0x541CD0; SOCOM 1's compared m_id alone) */
CPickup *CPickup::FindPickup(unsigned int id)
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (iterator i = m_list.begin(); i != e; ++i)
            if (id == (*i)->GetPickupNetEntityID())
                return *i;
    }
    return 0;
}

/* (OVL2 0x541CC0) */
unsigned short CPickup::GetEntityIDFromNetID(unsigned int netid)
{
    return netid >> 16;
}

/* the MP bomb's pickup, if one lies about (OVL2 0x541C60; SOCOM 1's) */
CPickup *CPickup::GetMPBomb()
{
    if (m_list.size()) {
        iterator e = m_list.end();

        for (iterator i = m_list.begin(); i != e; ++i)
            if ((*i)->m_type == PU_MPBOMB)
                return *i;
    }
    return 0;
}

/* a pickup whose contents are gone destroyed: ammunition once its last round is taken, a weapon
 * once it has none (OVL2 0x541BB0; SOCOM 1's, the ammunition now a class of OVL1's) */
void CPickup::CleanupAmmoPickup(CPickup *pickup)
{
    switch (pickup->m_type) {
    case CPickup::PU_AMMO: {
        CPickupAmmo *ammo = pickup->GetAmmo();

        ammo->Compact();
        if (!ammo->Count())
            Destroy(pickup);
        break;
    }
    case CPickup::PU_WEAPON:
        if (!pickup->GetWeapon())
            Destroy(pickup);
        break;
    }
}

/* an ammunition pickup's kinds counted and packed: each one's weapon item and ammunition id, and
 * its rounds (OVL2 0x541AD0; new in SOCOM II, its name unknown) */
#pragma auto_inline off
extern "C" void func_00541AD0(CPickupAmmo *ammo, int *count, unsigned short *packed,
                              unsigned short *rounds)
{
    *count = ammo->Count();
    for (int i = 0; i < *count; i++) {
        CZWeapon *weapon = 0;
        CZAmmo *cartridge = 0;
        int n = 0;

        ammo->Get(i, (int *)&weapon, (int *)&cartridge, &n);
        *rounds = n;
        EQUIP_ITEM item = weapon ? weapon->m_ID : EQUIP_NONE;
        EQUIP_AMMO id = cartridge ? cartridge->m_ID : AMMO_NONE;

        *packed = CZWeapon::PackItemAndAmmoEnums(item, id);
        rounds++;
        packed++;
    }
}
#pragma auto_inline reset

/* the contents by kind, or 0 (OVL2 0x541AB0, 0x541A90, 0x541A20, 0x5419C0; new in SOCOM II, the
 * names ours) */
CPickupAmmo *CPickup::GetAmmo()
{
    return m_type == PU_AMMO ? (CPickupAmmo *)m_data : 0;
}

CZWeapon *CPickup::GetWeapon()
{
    return m_type == PU_WEAPON ? (CZWeapon *)m_data : 0;
}

void *CPickup::GetData4567()
{
    return IsType4567() ? m_data : 0;
}

void *CPickup::GetData123()
{
    return IsType123() ? m_data : 0;
}

/* an ammunition pickup's kinds walked, their printing gone (OVL2 0x541910; new in SOCOM II, its
 * name unknown: seal_env.cpp calls it eight times) */
#pragma auto_inline off
extern "C" void func_00541910(CPickup *p, const char *label)
{
    switch (p->m_type) {
    case CPickup::PU_AMMO: {
        CPickupAmmo *ammo = p->GetAmmo();

        if (ammo) {
            int n = ammo->Count();
            CZWeapon *weapon = 0;
            CZAmmo *cartridge = 0;
            int rounds;

            for (int i = 0; i < n; i++)
                ammo->Get(i, (int *)&weapon, (int *)&cartridge, &rounds);
        }
        break;
    }
    case CPickup::PU_WEAPON:
        break;
    }
}

/* the list walked once a pickup is made from the network, its checks gone (OVL2 0x5418B0; new in
 * SOCOM II, its name unknown: DebugPickups' walk, inlined) */
extern "C" void func_005418B0(CPickup *p)
{
    CPickup::DebugPickups();
}

/* a made pickup told to the other consoles in an online game, or given its time (OVL2 0x541700;
 * new in SOCOM II, its name unknown: Create calls it last), then the list walked (round 36: the switch
 * in an inline taking the kind as a PU_TYPE -- an `unsigned char` or `int` parameter, the kind read in
 * place or a local of it, a reference to the pickup, all keep MW's order) */
/* the made pickup's network side, by its kind (the name ours): an inline taking the kind as its enum --
 * the argument evaluated ahead of the body numbers the kind's load first, so MW colours it last (a3),
 * then the online byte (a2) and the id's halves (a1, a0), as the target does; read in the body, the
 * kind is forwarded to the switch and loaded last (MW's colouring read off igdump, round 36) */
static inline void NetCreate(CPickup *p, CPickup::PU_TYPE kind)
{
    unsigned char online = theNetwork.m_9;
    unsigned int netid = p->GetPickupNetEntityID();

    switch (kind) {
    case CPickup::PU_AMMO: {
        CPickupAmmo *ammo = (CPickupAmmo *)p->m_data;

        if (ammo) {
            if (online) {
                int count = 0;
                unsigned short packed[8];
                unsigned short rounds[8];

                func_00541AD0(ammo, &count, packed, rounds);
                func_002BA3F0(count, packed, rounds, netid, p->GetPosW());
            } else
                p->m_timer = s_life;
        }
        break;
    }
    case CPickup::PU_WEAPON: {
        CZWeapon *weapon = (CZWeapon *)p->m_data;

        if (weapon) {
            if (online) {
                CQuat q = CQuat::identity;
                EQUIP_ITEM item = weapon->m_ID;

                if (p->m_node)
                    p->m_node->m_matrix.ToQuat(&q);
                func_002BA660(item, netid, p->GetPosW(), &q);
            } else
                p->m_timer = s_life;
        }
        break;
    }
    }
}

extern "C" void func_00541700(CPickup *p)
{
    NetCreate(p, (CPickup::PU_TYPE)p->m_type);
    CPickup::DebugPickups();
}
#pragma auto_inline reset

/* (OVL2 0x5416E0; SOCOM 1's) */
const CPnt3D *CPickup::GetPosW() const
{
    if (m_node)
        return (const CPnt3D *)m_node->m_matrix.m_matrix[3];
    return 0;
}

/* the owner's entity id over the pickup's own (OVL2 0x5416C0) */
unsigned int CPickup::GetPickupNetEntityID()
{
    return m_owner << 16 | m_id;
}

/* (OVL2 0x5416B0, 0x541650; SOCOM 1's) */
bool CPickup::GetVisibleByUnit(unsigned int unit)
{
    return (m_visible_by_units & unit) != 0;
}

void CPickup::SetVisibleByUnit(unsigned int unit)
{
    if (unit == 2 || unit == 4 || unit == 8) {
        m_visible_by_units |= 1;
        m_visible_by_units |= 2;
        m_visible_by_units |= 3;
    } else
        m_visible_by_units |= unit;
}
