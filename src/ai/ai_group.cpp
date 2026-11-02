/* ai_group.cpp -- SOCOM II's scripted AI groups, CAiGroup, with its actions act_camera, act_zanim,
 * act_anim and act_sound (the class names their RTTIs'; no SOCOM 1 counterpart). OVL2 0x60C170 to
 * ~0x60EDFF, after SEAL_AI_PARAMS's constructor (seal_aiparams.cpp) and before the vehicle state
 * (seal_aivehicle.cpp); the unit's name is ours. Many of its small functions are byte-exact drafts
 * (the actions' destructors among them); so far the group list's statics here. Defined in reverse
 * of their image order (-inline deferred). */

#include <list>

/* a group (only what a written function touches; the names ours) */
class CAiGroup {
public:
    /* the group of an id, or 0 (OVL2 0x60E460: CTGroup::Tick calls it, 0x60E7A0 jumps to it) */
    static CAiGroup *Find(int id);

    unsigned char _0[4];
    int m_id;   // 0x4

    /* every group (OVL1 bss 0x44D668: a pointer list, its size word, the sentinel at 0x44D66C) */
    static std::list<CAiGroup *> m_list;
};

/* (OVL2 0x60E460).
 * OPEN, 12 instructions, registers only: the shape is the target's (an assigned iterator living in
 * the frame, its begin() copied bitwise; the found group kept in a result zeroed in the first
 * test's delay slot), but the target keeps the first node in a2, the sentinel in a3 and the group
 * in a1 and compares `bne v1,a0` (the field first); here they rotate (a3, a1, a2; `bne a0,v1`).
 * Tried: the result declared after the iterator, the group in a local of its own (11), the test
 * `id == ...`, the assignment inside the test (8 bytes short) */
CAiGroup *CAiGroup::Find(int id)
{
    CAiGroup *found = 0;
    std::list<CAiGroup *>::iterator i;

    for (i = m_list.begin(); i != m_list.end(); ++i)
        if ((*i)->m_id == id) {
            found = *i;
            break;
        }
    return found;
}
