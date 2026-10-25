/* ai_target.cpp -- the AI's target, CAiTarget, and SOCOM 1's ai::CheckLineOfFire: the stretch after
 * seal_airesponse.cpp, OVL2 0x609DF0-0x60BF3F (CAiTarget's constructor at 0x60A940; SEAL_AI_PARAMS's
 * at 0x60BF40 follows, seal_aiparams.cpp). SOCOM 1 had the same functions in the same place, right
 * after the response states' destructors; the unit's name is ours. So far one getter. Defined in
 * reverse of their image order (-inline deferred). */

/* the mission's clock, which the target's times are taken on (theMission's float at 0xE0, OVL1
 * 0x4365C0: SOCOM 1's at 0x60 of its theMission, as its GetTimeSinceLastSighting reads it) --
 * a view of the mission, fts_state.h's CMission not reaching it */
struct CMissionClock {
    unsigned char _0[0xe0];
    float m_time;   // 0xe0
};
extern CMissionClock theMission;

/* SOCOM 1's DWARF: CAiTarget, 0x64 bytes; SOCOM II's is 0x78 (its CTarget 8 bytes longer, two
 * counters added at 0x40) -- only what a written function touches */
class CAiTarget {
public:
    CAiTarget();
    float GetTimeSinceLastSighting() const;
    /* the time it was last seen (an inline: expanded ahead of the subtraction, its load first) */
    float GetTimeLostSight() const { return m_time_lost_sight; }

    unsigned char _0[0x54];
    float m_time_lost_sight;   // 0x54 (SOCOM 1's at 0x40)
    unsigned char _58[0x78 - 0x58];
};

/* the time since the target was last seen (OVL2 0x60A590; SOCOM 1's, the same 0x14 bytes with the
 * field 0x14 on).
 * OPEN, 3 instructions: the target loads the field into $f0 first and the clock into $f1 after
 * (`sub.s $f0,$f1,$f0`); every spelling tried loads the clock first -- the clock as a float
 * global, a volatile, theMission's member, through an inline accessor or a local; the field
 * through a local, an inline accessor; a negated reverse difference (0x18 bytes) */
float CAiTarget::GetTimeSinceLastSighting() const
{
    return theMission.m_time - GetTimeLostSight();
}
