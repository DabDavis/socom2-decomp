/* seal_aistate.cpp -- the AI states' core: CQB attack, unknown, suspended, breach, script, goto,
 * pickup (SOCOM 1's unit, __sinit_seal_aistate.cpp, with its static CAiSUnknown; SOCOM 1 filed it
 * beside the other seal_*.cpp under gamez/zseal, this tree keeps the AI states under src/ai).
 * OVL2 0x5F1960-0x5F8BFF. So far only the unit's tail: the states' destructors and the inline
 * virtuals their tables pull in, and CAiStateStack's two out-of-line members. Defined in reverse
 * of their image order (-inline deferred). */

#include "../gamez/zseal/zseal.h"

extern "C" {
/* a CSealCtrlAi function (OVL2 0x56D090, the SEAL lane's range), called twice on leaving the
 * unknown state */
void func_0056D090(CSealCtrlAi *ai);
}

/* the AI controller's bit the CQB attack's delay reads (CSealCtrlAi at 0x264, bit 1; zseal.h does
 * not name it -- the names ours) */
struct CAiCtrlBits {
    unsigned char _0[0x264];
    unsigned char m_bit0 : 1;
    unsigned char m_nodelay : 1;
};

/* the "action" state's answers (OVL2 0x5F07E0 back to 0x5F07A0): no attacking, scanning or
 * pushing a state of kind 3 while its object is held; kind 25 never */
bool CAiSAction::CanPushAiState(AI_STATE state, CSealCtrlAi *ai)
{
    bool ok = true;

    switch (state) {
    case 3:
        ok = m_84 == 0;
        break;
    case 25:
        ok = false;
        break;
    }
    return ok;
}

bool CAiSAction::CanScan() const
{
    return m_84 == 0;
}

bool CAiSAction::CanPursue(CSealCtrlAi *ai) const
{
    return false;
}

bool CAiSAction::CanAttack() const
{
    return m_84 == 0;
}

/* +0x38: the state's phase word below 6 (the name ours) */
bool CAiSAction::Virtual38()
{
    return m_34 < 6;
}

/* (OVL2 0x5F4080) */
bool CAiSScript::CanAttack() const
{
    return true;
}

/* the stack's destructor: its states deleted, then the list's own (OVL2 0x5F8490) */
CAiStateStack::~CAiStateStack()
{
    Clear();
}

/* every state deleted, the stack emptied (OVL2 0x5F8390; ~CSealCtrlAi calls it too).
 * OPEN, 13 instructions, stack slots only: the target walks two const_iterators assigned from
 * the const begin()/end() (the four copies each -- store, bitwise, word, bitwise -- and the walk
 * re-read from memory at every step, as here), but keeps the walking pair BELOW the copies'
 * temporaries (68/64 against 72/76), where every named pair lands above them; MSL's for_each over
 * the const iterators gives the target's order of operations but its parameters' slots too high
 * (14), over the plain iterators a register walk (0xA0). The pair is likely an inlined function's
 * parameters, as for_each's are, with a functor shape not found yet. */
void CAiStateStack::Clear()
{
    const CAiStateStack &s = *this;
    const_iterator i, e;

    e = s.end();
    for (i = s.begin(); i != e; ++i)
        delete *i;
    erase(begin(), end());
}

/* a pickup's reset (OVL2 0x5F5590; SOCOM 1's) */
void CAiSPickup::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        m_5c = 0;
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

/* the unknown state: the AI's mode 8 on entry and exit, 0x56D090 twice otherwise (OVL2 0x5F1BF0;
 * SOCOM 1's was empty) */
void CAiSUnknown::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        break;
    case SIGNAL_0:
    case SIGNAL_3:
        ai->SetAiMode((AI_MODE)8);
        break;
    case SIGNAL_1:
    case SIGNAL_2:
        func_0056D090(ai);
        func_0056D090(ai);
        break;
    }
}

/* a CQB attack's reset, and its delay on entry (OVL2 0x5F1AD0; SOCOM 1's) */
void CAiSCQBAttack::OnSignal(SIGNAL sig, CSealCtrlAi *ai)
{
    switch (sig) {
    case SIGNAL_4:
        m_58 = 0;
        m_5c = 0.0f;
        break;
    case SIGNAL_0:
        m_5c = ((CAiCtrlBits *)ai)->m_nodelay ? 0.0f : 0.75f;
        break;
    case SIGNAL_1:
        break;
    case SIGNAL_2:
        break;
    case SIGNAL_3:
        break;
    }
}

/* the destructors (OVL2 0x5F8A70, 0x5F89C0, 0x5F8940, 0x5F8880, 0x5F8800, 0x5F8770): each its
 * own table stored, then CAiState's, its sub-state stack destroyed out of line */
CAiSPickup::~CAiSPickup()
{
}

CAiSGoto::~CAiSGoto()
{
}

CAiSScript::~CAiSScript()
{
}

CAiSSuspended::~CAiSSuspended()
{
}

CAiSUnknown::~CAiSUnknown()
{
}

CAiSCQBAttack::~CAiSCQBAttack()
{
}

/* (OVL2 0x5EF9D0, the unit's first function) */
CAiSAction::~CAiSAction()
{
}
