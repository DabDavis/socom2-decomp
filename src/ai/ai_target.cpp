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

/* the target's entity as the validity test reads it (a view: zentity.h's CEntity and zseal.h's
 * controller classes are not included here) -- its type (2 a SEAL), its controller, the alive bit
 * (0xE1 bit 4) -- and the controller's two virtuals it asks, IsAi (+0x30) and GetSealCtrlAi (+0x38),
 * and the AI word's bits 8 and 9 (0x264, read a byte at a time: the byte at 0x265) */
struct CTargetAi {
    unsigned char _0[0x265];
    unsigned char m_aibit8 : 1;   // 0x265 bit 0 (the word at 0x264's bit 8)
    unsigned char m_aibit9 : 1;   // bit 1
};

struct CTargetCtrl {
    virtual void Virtual08();
    virtual void Virtual0C();
    virtual void Virtual10();
    virtual void Virtual14();
    virtual void Virtual18();
    virtual void Virtual1C();
    virtual void Virtual20();
    virtual void Virtual24();
    virtual void Virtual28();
    virtual void Virtual2C();
    virtual bool IsAi() const;   // +0x30
    virtual void Virtual34();
    virtual CTargetAi *GetSealCtrlAi();   // +0x38
};

struct CTargetEntity {
    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10 (2 a SEAL)
    unsigned char _11[0xc0 - 0x11];
    CTargetCtrl *m_control;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1 : 4;
    unsigned char m_alive : 1;   // 0xe1 bit 4

    /* an AI drives it (as zseal.h's CZSealBody::IsPlayer: the controller tested, then asked) */
    bool IsAi() { return m_control ? m_control->IsAi() : false; }
};

/* the entity as a SEAL, or 0 (an inline: the entity tested again) */
static inline CTargetEntity *SealOf(CTargetEntity *e)
{
    return e && e->m_type == 2 ? e : 0;
}

/* SOCOM 1's DWARF: CAiTarget, 0x64 bytes; SOCOM II's is 0x78 (its CTarget 8 bytes longer, two
 * counters added at 0x40) -- only what a written function touches */
class CAiTarget {
public:
    CAiTarget();
    float GetTimeSinceLastSighting() const;
    /* the time it was last seen (an inline: expanded ahead of the subtraction, its load first) */
    float GetTimeLostSight() const { return m_time_lost_sight; }

    CTargetEntity *m_ent;   // 0x0: the CTarget's entity
    unsigned char _4[0x54 - 0x4];
    float m_time_lost_sight;   // 0x54 (SOCOM 1's at 0x40)
    unsigned char _58[0x78 - 0x58];
};

/* whether the target still counts: an entity, alive, and -- when it is a SEAL an AI drives -- that AI
 * with neither bit 8 nor bit 9 of its word at 0x264 set (OVL2 0x60A850, new in SOCOM II: some fifty
 * callers pass it the CAiTarget at controller+0x67C; where SOCOM 1's CAiSPursue::Tick tested the
 * target's entity and its alive bit inline, SOCOM II calls this. The integrator's suggested name,
 * CAiTarget::IsValid, is unproven -- SOCOM 1 has no such out-of-line member -- so the row keeps
 * its placeholder) */
extern "C" bool func_0060A850(const CAiTarget *t)
{
    bool r = false;
    CTargetEntity *e = t->m_ent;

    if (e && e->m_alive) {
        r = true;
        CTargetEntity *seal = SealOf(e);

        if (seal && seal->IsAi()) {
            CTargetAi *ai = seal->m_control->GetSealCtrlAi();

            if (ai)
                r = !(ai->m_aibit9 || ai->m_aibit8);
        }
    }
    return r;
}

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
