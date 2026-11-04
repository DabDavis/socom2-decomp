/* seal_sensors.cpp -- what an AI SEAL senses: its view cone, the visibility tests, the targets it
 * sees fall (SOCOM 1's unit, __sinit_seal_sensors.cpp: UpdateViewCone, SetViewFOV, ComputeVisibility,
 * RegisterCanSee ... NotifyTargetDown). OVL2 0x5C9400-0x5CDBCF, after seal_kit.cpp's CAiWepHint and
 * before seal_tasks.cpp. So far NotifyTargetDown. Defined in reverse of their image order (-inline
 * deferred). */

#include "zseal.h"
#include <list>

/* SOCOM 1's CDiPool (_diPool, 0x28 bytes there): the visibility tests' DiIntersects and the requests
 * that use them, a used list and a free list of SOCOM 1's CDiPool::dir records. SOCOM II's pool is
 * 0x3C bytes at 0x44D5E0 (OVL1's __sinit_fts_ovl2statics.cpp builds it: six DiIntersects of 76 bytes
 * at +0, the count 6 at +4, 2 and 1 at +8/+0xC, the lists at +0x10 and +0x1C, a counter at +0x38) --
 * only what a written function touches */
class CDiPool {
public:
    /* a request (SOCOM 1's CDiPool::dir; its constructor zeroes 0x0, 0x20, 0x28, 0x2C and 0x30) */
    struct dir {
        unsigned char m_state;   // 0x0 (0 free, 1 dropped, 2 pending, 4 registered)
        unsigned char _1[0x1c - 0x1];
        int m_index;   // 0x1C: its DiIntersect's, -1 for none
        unsigned char _20[0x28 - 0x20];
        CEntity *m_ent;   // 0x28: whose test it is
        int m_2c;   // 0x2C

        /* dropped: marked for the pool's next pass, its DiIntersect let go (inline; the name ours) */
        void Drop()
        {
            m_state = 1;
            m_index = -1;
        }
    };
    /* the free list (a class over the pointer list: SOCOM 1's ~CDiPool tests the member before the
     * deletion walk its destructor inlines; the names ours) */
    class dirlist : public std::list<dir *> {
    public:
        /* a request reset and put back (OVL2 0x5CC800: CloseDiPool and ai_target.cpp's 0x60B8F0 call
         * it -- SOCOM 1's CloseDiPool did it inline, the record re-read from the frame for each store,
         * its address handed to the insert) */
        void Free(dir *d);
        /* every request deleted, the list emptied (OVL2 0x5CC6C0: CloseDiPool calls it on the free
         * list -- SOCOM 1's did it inline, as its ~CDiPool does) */
        void DeleteAll();
    };

    /* every used request put back on the free list, then all of them deleted (inline: CloseDiPool's) */
    void Close()
    {
        dirlist::iterator i;

        for (i = m_used.begin(); i != m_used.end(); i++)
            m_free.Free(*i);
        m_used.clear();
        m_free.DeleteAll();
    }
    /* an entity's pending requests dropped (inline: CleanupDiPool's) */
    void Release(CEntity *e)
    {
        dirlist::iterator i;

        for (i = m_used.begin(); i != m_used.end(); i++) {
            dir *d = *i;

            if (d->m_ent == e && d->m_state) {
                d->m_state = 1;
                d->m_index = -1;
            }
        }
    }

    void *m_pool;   // 0x0: the DiIntersects
    int m_count;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    dirlist m_used;   // 0x10
    dirlist m_free;   // 0x1c
    unsigned char _28[0x3c - 0x28];
};

extern CDiPool _diPool;

/* what an entity knows of another, or 0 (inline: the ternary inside it keeps the call's result in
 * both arms, `b; move v1,v0` / `move v1,zero`, and pads no label -- written in the caller it pads
 * both, 0x198; the name ours) */
static inline CTarget *TargetOf(CEntity *e, CEntity *of)
{
    return e ? e->m_targets.FindTarget(of) : 0;
}

/* a dying SEAL's requests dropped (CDiPool::Release inlined one level: the list's pieces called out
 * of line, as CloseDiPool's), then each of its targets' entry for it unhooked from its request
 * (SOCOM 1's; OVL2 0x5CC8A0). CTarget's word at 0x28 is the request (zentity.h's m_unk28). The
 * targets through a reference local keep the vector's address in s1; the request's value declared
 * before its address gives the target's a2/a1 (a reference to the field, or the address first, swaps
 * them) */
void CleanupDiPool(CZSealBody *seal)
{
    _diPool.Release(seal);
    CTargetList &targets = seal->m_targets;

    for (CTargetList::iterator t = targets.begin(); t != targets.end(); t++) {
        CTarget *found = TargetOf(t->m_ent, seal);

        if (found) {
            CDiPool::dir *d = (CDiPool::dir *)found->m_unk28;
            CDiPool::dir **p = (CDiPool::dir **)&found->m_unk28;

            if (d)
                d->Drop();
            *p = 0;
        }
    }
}

/* (SOCOM 1's, empty in both; OVL2 0x5CC890) */
void CSealCtrlAi::OpenDiPool()
{
}

void CDiPool::dirlist::Free(dir *d)
{
    d->m_state = 0;
    d->m_2c = 0;
    push_back(d);
}

/* The walking iterator the target keeps in the frame (copied in from begin()'s temporary through $f0,
 * re-read and re-stored at each step) is one ASSIGNED from begin() -- the wrapper iterator's implicit
 * assignment is a bitwise copy, which keeps both ends of it in memory -- with the end taken into a
 * local first (held in s1). Not for_each: its by-value `first` is built by the iterator's user-declared
 * copy constructor and scalarised (0x90); a plain walk `i != end()` is 0xA0 */
void CDiPool::dirlist::DeleteAll()
{
    iterator i;
    iterator e = end();

    for (i = begin(); i != e; ++i)
        delete *i;
    clear();
}

/* the pool emptied at the mission's end (SOCOM 1's; OVL2 0x5CC5F0) */
void CSealCtrlAi::CloseDiPool()
{
    _diPool.Close();
}

/* a weapon's item (zweapon.h's CZWeapon::m_ID at 0x7C, a byte enum: copied with lb, compared with
 * lbu -- zseal.h declares CZWeapon only by name) */
struct CSensorWeaponView {
    unsigned char _0[0x7c];
    EQUIP_ITEM m_ID;   // 0x7c
};

/* a bit of a CBits set (inlined; zmath.h's CBits has Test and Clear only -- the name ours) */
inline void SetSensorBit(CBits *bits, unsigned int i)
{
    bits->m_vec[i >> 5] |= 1 << (i & 31);
}

/* and tested, the mask first (the target builds it before the word's address, as the set does --
 * CBits::Test indexes first; the name ours) */
inline bool TestSensorBit(const CBits *bits, unsigned int i)
{
    return (1 << (i & 31)) & bits->m_vec[i >> 5];
}

/* a weapon pickup seen: unless the AI already goes for a pickup or this SEAL has marked it, a Navy
 * SEAL goes for item 152 when its kit has none of it or none with ammunition (the pickup state
 * pushed and handed it, flag 1); any other item is marked seen by this SEAL. Whether it went
 * (OVL2 0x5CB450: 0x5CB870 asks it; new in SOCOM II -- the name ours). `go` declared first and
 * assigned after the early return: the target numbers it before the SEAL (s2 to its s1).
 * OPEN, 8 instructions, the seen-bit test's registers only: the target masks the index first
 * (`andi a1,v1,0x1f` before `srl v1,v1,5`, the 1 in a0, the words in v0 -- the set below, which
 * matches, has the same allocation); every spelling here shifts first (srl a1, andi in place, the
 * 1 in v0): CBits::Test, the mask first or last, either in a local, the word's address in a local,
 * `(w >> s) & 1` (10) */
extern "C" bool func_005CB450(CSealCtrlAi *ai, CPickup *p, CZWeapon *w)
{
    bool go;
    CZSealBody *seal = (CZSealBody *)ai->m_entity;

    if (ai->AiStateIs(AI_STATE_PICKUP) || TestSensorBit((CBits *)p->m_bits, seal->m_index))
        return false;
    go = false;
    CZKit *kit = &seal->m_kit;

    if (ai->IsNavySeal() && kit) {
        EQUIP_ITEM item = ((CSensorWeaponView *)w)->m_ID;
        int i = kit->GetItemIndex(item);

        switch (item) {
        case 152:
            if (i == -1 || !kit->ItemHasAmmo(i))
                go = true;
            break;
        default:
            SetSensorBit((CBits *)p->m_bits, seal->m_index);
            break;
        }
    }
    if (go && ai->PushAiState(AI_STATE_PICKUP, 0)) {
        CAiState *s = ai->GetAiState();
        CAiSPickup *pick = s && s->m_state == AI_STATE_PICKUP ? (CAiSPickup *)s : 0;

        pick->SetPickup(p, 1);
    }
    return go;
}

/* a target seen to fall: the AI calls it, "tango down", at full chance (OVL2 0x5C9B10; SOCOM 1's
 * played its sound 18 the same way, through the same slot +0x24) */
void CSealCtrlAi::NotifyTargetDown(CZSealBody *seal)
{
    PlayAiSound(CHRSND_TANGO_DOWN, 1.0f, -1);
}
