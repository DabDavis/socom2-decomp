/* zsnd_positions.cpp -- SOCOM II's record of the 3D sounds played at fixed positions: every record
 * kept, the ones in hearing range, and those dropped by the two-second update (OVL1
 * 0x34BD40-0x34C55F; its static initialiser, 0x403370, builds the three vectors and registers
 * their destructor 0x34C4F0). Not in SOCOM 1; every name here is ours. Defined in reverse of their
 * image order (-inline deferred). The update (0x34BEC0) written in round 26 (OPEN). Round 18: at the
 * flags' -inline level=5, round 24's `#pragma inline_depth(6)` stand-in dropped (the add's walk
 * ends at Last(): the object byte-identical). */

#include "zsnd.h"

/* a sound played at a position: the position, the sound and the voice playing it (0x34C2F0 makes
 * them, 0x14 bytes) */
struct CSndPosition {
    PNT3D m_pos;                /* 0x00 */
    CSnd *m_sound;              /* 0x0C */
    CSndInstance *m_instance;   /* 0x10 */
};

/* the unit's statics, in its initialiser's order (0x403370: each vector's three words zeroed, then
 * its destructor 0x34C4F0 registered) */
std::vector<CSndPosition *> data_0049E1F0;  /* the records in hearing range */
std::vector<CSndPosition *> data_0049E210;  /* those the update drops */
std::vector<CSndPosition *> data_0049E230;  /* every record */
extern float data_0049E240;                         /* the update's countdown */

extern "C" void func_0034BEC0(float dT);
extern "C" void func_0034BD40();

/* a pointer vector's end built straight from its storage (ours): a walk testing `i != end()`
 * inside an inline is six deep where its caller expands it (include/vector's end() four high since
 * the MSL headers round 24), so at -inline level=5 MW keeps end()'s body and calls the raw end and
 * __iter; through this end the walk inlines whole at 5, as the target (the online lobby's round 20
 * Last(), the same refit). The raw end held in a const local, as end() holds it: written straight
 * into the iterator (`iterator(data_ + size_)`, the lobby's spelling) the walk's cursor and end
 * swap a1/a0 (7 off) -- the const local numbers the end after the cursor */
static inline std::vector<CSndPosition *>::iterator Last(std::vector<CSndPosition *> &v)
{
    unsigned int *const p = v.data_ + v.size_;

    return std::vector<CSndPosition *>::iterator(p);
}

/* whether a record holds this sound at this position (ours: the add inlines it) */
static inline bool PositionKnown(CSnd *sound, const CPnt3D *pos)
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != Last(data_0049E230); ++i) {
        CSndPosition *p = *i;

        if (p->m_sound == sound && *pos == *(const CPnt3D *)&p->m_pos)
            return true;
    }
    return false;
}

/* a voice's sound and position recorded, once, and the update run at once (CSnd::PlaySound3D
 * calls it) */
extern "C" void func_0034C2F0(CSndInstance *inst)
{
    CSnd *sound = inst->m_parent;
    const CPnt3D *pos = &inst->m_pos;

    if (!PositionKnown(sound, pos)) {
        CSndPosition *p;

        (p = new CSndPosition)->m_instance = inst;
        p->m_pos = *(const PNT3D *)&inst->m_pos;
        p->m_sound = sound;
        data_0049E230.push_back(p);
        CSndPosition *q = p;

        data_0049E1F0.push_back(q);
        func_0034BEC0(2.0f);
    }
}

/* whether a record is among those in hearing range (ours: the update inlines it) */
static inline bool InRange(CSndPosition *p)
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E1F0.begin(); i != Last(data_0049E1F0); ++i)
        if (*i == p)
            return true;
    return false;
}

/* every two seconds of the countdown: the records in range whose voice is now out of range --
 * 1.15 times the sound's far range from the listener -- stopped and released, and taken off the
 * in-range list; every record not in range and now within it played again in 3D and listed (the
 * add runs it at once with 2 seconds, CSnd::PostTick with the frame time). Written in the world
 * lane's round 26 (build/scratch/wld/v_pos_*.py): the listener's matrix through a pointer local
 * taken ahead of the dump's call (s0 across all three walks), the far range converted signed (an
 * int cast: the unsigned short alone takes the unsigned conversion), the dropped walk's record read
 * once ahead of its search, the last walk's push through a copy of the record (the add's `q`: the
 * two pushes' and the erase's temporaries then take the target's slots).
 * OPEN, 15 instructions at the target's size: (1) the first walk's range block -- its reload of
 * the record (v77) is numbered ahead of the 1.15 constant's lui/ori (v81/v82), so it takes a0 and is
 * scheduled first, where the target's takes v1 after the constant (`igreplay --want 77=v1 --movers
 * 77`: any place after 82.5); not moved by the range as a named product, a named factor, `*=`, an
 * int or float local for the range, the product's operands swapped, the distance first or as a
 * local, inline helpers for the range, the far range or the product (15 each); the world lane's
 * round 31: the SEALs' round-55 lever, a no-op (float) cast on the constant, the converted range,
 * the whole product, the squared sums or the squared range (build/scratch/wld/v_pos1.py), 15 each --
 * the reload is a GPR node, which the float operand weight does not move; (2) the last walk's
 * far-range lhu is scheduled ahead of the y product (2) */
extern "C" void func_0034BEC0(float dT)
{
    data_0049E240 -= dT;
    if (data_0049E240 <= 0.0f) {
        CMatrix *m = &CSnd::m_listenermatrix;

        data_0049E240 = 2.0f;
        func_0034BD40();
        data_0049E210.clear();
        for (std::vector<CSndPosition *>::iterator i = data_0049E1F0.begin(); i != data_0049E1F0.end(); ++i) {
            CSndPosition *p = *i;
            CSndInstance *inst = p->m_instance;
            CPnt3D d;

            m->Transform((const CPnt3D *)&p->m_pos, &d, 1);
            if (inst) {
                float range = 1.15f * (int)p->m_sound->m_range_max;

                if (!(d.x * d.x + d.y * d.y + d.z * d.z < range * range)) {
                    inst->Stop();
                    p->m_instance = 0;
                    CSndInstance::ReleaseSoundInstance(inst);
                    data_0049E210.push_back(p);
                }
            }
        }
        for (std::vector<CSndPosition *>::iterator j = data_0049E210.begin(); j != data_0049E210.end(); ++j) {
            CSndPosition *p = *j;
            std::vector<CSndPosition *>::iterator k;

            for (k = data_0049E1F0.begin(); k != data_0049E1F0.end(); ++k)
                if (*k == p) {
                    data_0049E1F0.erase(k);
                    break;
                }
        }
        for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); ++i) {
            CSndPosition *p = *i;
            CPnt3D d;

            m->Transform((const CPnt3D *)&p->m_pos, &d, 1);
            if (!InRange(p)) {
                float range = 1.15f * (int)p->m_sound->m_range_max;

                if (d.x * d.x + d.y * d.y + d.z * d.z < range * range && p->m_sound) {
                    p->m_instance = p->m_sound->PlaySound3D((const CPnt3D *)&p->m_pos, 1.0f, 0, 0, 255);
                    CSndPosition *q = p;

                    data_0049E1F0.push_back(q);
                }
            }
        }
    }
}

/* the vectors' room made and the countdown zeroed (CSnd::Init calls it) */
extern "C" void func_0034C4A0()
{
    data_0049E1F0.reserve(20);
    data_0049E210.reserve(20);
    data_0049E230.reserve(60);
    data_0049E240 = 0;
}

/* every record deleted, the three vectors emptied (CSnd::Close and UnloadAllSounds call it) */
extern "C" void func_0034BE10()
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); ++i)
        if (*i)
            delete *i;
    data_0049E1F0.clear();
    data_0049E210.clear();
    data_0049E230.clear();
}

/* two empty walks, the records in range and every record (a debug print compiled out; the update
 * calls it) */
extern "C" void func_0034BD40()
{
    for (std::vector<CSndPosition *>::iterator i = data_0049E1F0.begin(); i != data_0049E1F0.end(); i++)
        ;
    for (std::vector<CSndPosition *>::iterator i = data_0049E230.begin(); i != data_0049E230.end(); i++)
        ;
}
