/* seal_airesponse.cpp -- the states an AI SEAL falls into in response: hostage, avoid, rush, path
 * recovery, flee, target, event, stun response, hold, surrender, no ammo, reload, shot at (SOCOM
 * 1's unit, __sinit_seal_airesponse.cpp -- its CAiSRestrained and CAiSCorpse are gone, CAiSAvoid
 * moved in from seal_aistate.cpp, CAiSHostage is here). OVL2 0x601880-0x609DEF. So far only the
 * states' destructors and the inline virtuals their tables pull in. Defined in reverse of their
 * image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"

/* the destructors (OVL2 0x609D60 back to 0x609710, and CAiSHostage's at 0x601880): each its own
 * table stored, then CAiState's, its sub-state stack destroyed out of line */
CAiSShotAt::~CAiSShotAt()
{
}

CAiSReload::~CAiSReload()
{
}

CAiSNoAmmo::~CAiSNoAmmo()
{
}

CAiSSurrender::~CAiSSurrender()
{
}

CAiSHold::~CAiSHold()
{
}

CAiSStunResponse::~CAiSStunResponse()
{
}

CAiSEvent::~CAiSEvent()
{
}

CAiSTarget::~CAiSTarget()
{
}

CAiSFlee::~CAiSFlee()
{
}

CAiSRush::~CAiSRush()
{
}

CAiSAvoid::~CAiSAvoid()
{
}

/* a no-ammunition state's reset (OVL2 0x608290; SOCOM 1's) */
void CAiSNoAmmo::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a surrender takes another state only once it is past its fifth phase: any, for an entity of the
 * 0x20000 team bit, else kinds 23 and 28 (OVL2 0x607D10) */
bool CAiSSurrender::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = false;

    if ((int)m_34 >= 5) {
        if (ai->m_entity->m_team & 0x20000)
            ok = true;
        else
            ok = state == 23 || state == 28;
    }
    return ok;
}

/* a target state: the AI's mode 4 on entry, its goal cleared on the way out (OVL2 0x604C60;
 * SOCOM 1's) */
void CAiSTarget::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
        ai->SetAiMode((AI_MODE)4);
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        ai->ClearGoal(GOAL_NONE);
        break;
    }
}

/* a flight's reset (OVL2 0x604830; SOCOM 1's) */
void CAiSFlee::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_60 = 0;
        m_58 = true;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a hostage's reset: no captor (OVL2 0x601F20; SOCOM 1's cleared one word) */
void CAiSHostage::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_5c = CEntity::invalidId;
        m_58 = -1;
        break;
    case SIGNAL_0:
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* a hostage takes any state but kind 16, and kind 19 only in its phases 5 and 6 (OVL2
 * 0x601EC0) */
bool CAiSHostage::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;
    unsigned char phase = m_34;

    switch (state) {
    case 19:
        ok = phase == 5 || phase == 6;
        break;
    case 16:
        ok = false;
        break;
    }
    return ok;
}

CAiSHostage::~CAiSHostage()
{
}
