/* ai_states.h -- the AI controller's states: CAiState, the stack of them, and the CAiS* family
 * (OVL2; SOCOM 1's units seal_aistate.cpp, seal_aifollow.cpp, seal_airesponse.cpp and their
 * neighbours -- the file names in SOCOM 1's static initialisers; this header's name is ours).
 * SOCOM 1's DWARF describes none of these classes but CAiStateStack, so every layout here is read
 * off SOCOM II's own code (CAiState's constructor at 0x5EA5D0 fixes its members) and only what a
 * written function touches is named.
 *
 * SOCOM II's CAiState vtable (0x669950) has 18 slots where SOCOM 1's had 10: OnSignal +0x8, a new
 * slot +0xC, Tick +0x10, a new +0x14, GetStatus +0x18, CanPushAiState +0x1C, CanSwitchAiState
 * +0x20, CanAttack +0x24, CanScan +0x28, CanPursue +0x2C, GetScriptResponse +0x30, six new slots
 * (+0x34-0x48; the base's answer 1, 1, 1, 0, 0, 0) and the destructor last, +0x4C. Every override
 * SOCOM 1 names for the fifteen classes the demo has lands on its slot. The new slots' names are
 * ours, their parameters only what the overrides read. */
#ifndef AI_STATES_H
#define AI_STATES_H

#include <list>

class CSealCtrlAi;
class CAiEvent;

/* the state kinds (SOCOM 1's AI_STATE, a byte: CAiState's constructor stores it with sb; the
 * values are not needed yet) */
enum AI_STATE { AI_STATE_UNKNOWN = 0, AI_STATE_LAST = 255 };

class CAiState;

/* what a state's Tick says (a byte: every Tick ends on a ternary between two enumerators, `li 1;
 * movz` narrowed; the stack's tick returns 2 once its state is done -- the names ours) */
enum AI_RESULT { AI_RUNNING = 0, AI_DONE = 1, AI_FINISHED = 2, AI_RESULT_LAST = 255 };

/* the states a controller (or a state) runs, the current one at the front (SOCOM 1's DWARF:
 * CAiStateStack : list, 0xc bytes -- CSealCtrlAi::m_stateStack there, followed by m_stateTimer
 * and m_stateFlags; SOCOM II also gives every CAiState one, at +8). SOCOM II's is 0x20 bytes: its
 * constructor (0x5F8520) sets three words after the list, and its tick (0x5F8030) runs the clock
 * at 0x18 that SOCOM 1 kept in the controller (CSealCtrlAi's 0x434: the stack sits at 0x41C there).
 * The stack owns its states. */
class CAiStateStack : public std::list<CAiState *> {
public:
    CAiStateStack() : m_c(-1), m_10(0), m_14(0) {}
    /* both out of line, seal_aistate.cpp's: every other unit calls the destructor (~CAiState's
     * copy in seal_unit.cpp's tail, 0x5EA450, calls it where an inline one would be inlined a
     * level) */
    ~CAiStateStack();
    /* every state deleted, the stack emptied (the name ours; 0x5F8390) */
    void Clear();
    /* the current state, the front, or 0 (seal_aistate.cpp's, out of line: the SEAL units call
     * them; CSealCtrlAi::GetAiState inlines one; the names ours -- 0x5F8330, 0x5F8360) */
    CAiState *GetState() const;
    CAiState *GetState();

    int m_c;   // 0xc (-1 when new)
    unsigned int m_10;   // 0x10
    unsigned int m_14;   // 0x14
    float m_timer;   // 0x18: the time in the current state (SOCOM 1's CSealCtrlAi::m_stateTimer)
    int m_result;   // 0x1c: the last state's result (the name ours)
};

class CAiState {
public:
    /* what a state is told (SOCOM 1's CAiState::SIGNAL, a byte; SOCOM 1's names unknown: the
     * states test 0-4, CAiSPursue's and CAiSAnimate's OnSignal reset on 4) */
    enum SIGNAL { SIGNAL_0 = 0, SIGNAL_1 = 1, SIGNAL_2 = 2, SIGNAL_3 = 3, SIGNAL_4 = 4, SIGNAL_LAST = 255 };

    /* the base's answers are inline: a unit that emits a state's table emits the copies its
     * slots need (seal_ai.cpp's are the ones linked, 0x555380-0x5553B0, 0x570D10, 0x575990,
     * 0x575AA0; seal_unit.cpp's OnSignal and Tick, 0x5F81D0) */
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai) {}               /* +0x08 */
    virtual void Virtual0C(int a, int b, int c);                          /* +0x0C (new) */
    virtual AI_RESULT Tick(float dT, CSealCtrlAi *ai) { return AI_DONE; }   /* +0x10 */
    virtual int Virtual14() { return 0; }                                 /* +0x14 (new) */
    virtual const char *GetStatus() const { return ""; }                  /* +0x18 */
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai) { return true; }   /* +0x1C */
    virtual bool CanSwitchAiState(AI_STATE state, CSealCtrlAi *ai) { return true; } /* +0x20 */
    virtual bool CanAttack() const { return true; }                       /* +0x24 */
    virtual bool CanScan() const { return true; }                         /* +0x28 */
    virtual bool CanPursue(CSealCtrlAi *ai) const { return true; }        /* +0x2C */
    virtual const char *GetScriptResponse() const { return 0; }           /* +0x30: a string, or 0 */
    virtual bool Virtual34() { return true; }                             /* +0x34 (new) */
    virtual bool Virtual38() { return true; }                             /* +0x38 (new) */
    virtual bool Virtual3C() { return true; }                             /* +0x3C (new) */
    virtual bool Virtual40() { return false; }                            /* +0x40 (new) */
    virtual bool Virtual44() { return false; }                            /* +0x44 (new) */
    virtual bool Virtual48() { return false; }                            /* +0x48 (new) */
    virtual ~CAiState() {}                                                /* +0x4C */

    /* the step a state's Tick ends on: a new phase restarts its clock (inlined in every Tick and
     * in the stack's delay; the name ours) */
    void SetPhase(unsigned int phase)
    {
        if (phase != m_phase) {
            m_phaseTime = 0.0f;
            m_phase = phase;
        }
    }

    /* what every Tick ends on: running while m_active holds, else done -- kept in m_done too (the
     * name ours) */
    AI_RESULT Result() { return (AI_RESULT)(m_done = m_active ? AI_RUNNING : AI_DONE); }

    /* the frame's time, as the stack's tick sets it for the states (OVL1 bss 0x44D620, read by
     * every Tick in place of its dT; the name ours) */
    static float m_dT;

    float m_time;   // 0x4: a time on the AI's state clock (SOCOM 1's at 0x8; zeroed by Reset)
    CAiStateStack m_stack;   // 0x8: the state's own sub-states (new in SOCOM II)
    int m_done;   // 0x28: what the last Tick returned (the name ours)
    unsigned int m_flags;   // 0x2c (zeroed by the constructor; the overrides test single bits)
    AI_STATE m_state;   // 0x30
    unsigned char _31[0x34 - 0x31];
    unsigned int m_phase;   // 0x34: the state's step (SOCOM 1's at 0x14; zeroed by Reset)
    float m_phaseTime;   // 0x38: the time in that step (the stack's tick adds dT to it)
    unsigned int m_nextPhase;   // 0x3c: the step a delay ends on (0x5F61E0)
    float m_delay;   // 0x40: that delay
    unsigned char _44[0x48 - 0x44];
    unsigned int m_48 : 16;   // 0x48 (never touched by the code read so far)
    unsigned int m_bit0 : 1;   // 0x4a bit 0 (set by Reset)
    unsigned int m_active : 1;   // bit 1: running (a state's Tick returns !m_active once it is cleared)
    unsigned int m_bit2 : 1;
    unsigned int m_bit3 : 1;
    unsigned int m_bit4 : 1;
    unsigned int m_nibble5 : 4;   // bits 5-8 (Reset: all set)
    unsigned int m_nibble9 : 4;   // bits 9-12 (Reset: 1)
    unsigned int m_bit13 : 1;
    unsigned int m_bit14 : 1;
    unsigned int m_bit15 : 1;
    float m_4c;   // 0x4c (Reset: 1.0)
    unsigned int m_loc;   // 0x50: a CAiMapLoc (Reset: the invalid cell; its constructor here is an
                          // empty one, 0x516120, where zentity.h's copies the invalid cell in)
    int m_54;   // 0x54 (Reset: -1)
};

/* the states whose code is this lane's (each class only what a written function needs) */
class CAiSCQBAttack : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual AI_RESULT Tick(float dT, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual ~CAiSCQBAttack();

    class CEntity *m_target;   // 0x58: the SEAL attacked (SOCOM 1's at 0x28)
    float m_5c;   // 0x5c: a delay (0.75, or none when the AI's bit at 0x264 is set)
};

class CAiSUnknown : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual AI_RESULT Tick(float dT, CSealCtrlAi *ai);
    virtual ~CAiSUnknown();
};

class CAiSSuspended : public CAiState {
public:
    virtual AI_RESULT Tick(float dT, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    virtual ~CAiSSuspended();
};

class CAiSBreach : public CAiState {
public:
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    /* +0x3C (the name ours, by slot) */
    virtual bool Virtual3C() { return false; }
    virtual ~CAiSBreach();
};

class CAiSScript : public CAiState {
public:
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual bool CanAttack() const;
    virtual bool CanPursue(CSealCtrlAi *ai) const;
    virtual ~CAiSScript();
    /* not held, and free to be pursued from (inlined in CanPushAiState; the names ours) */
    bool NotHeld() const { return !m_hold; }
    bool Pursuable(CSealCtrlAi *ai)
    {
        if (NotHeld() && CanPursue(ai))
            return true;
        return false;
    }

    unsigned int m_58;   // 0x58
    unsigned char m_hold : 1;   // 0x5c bit 0: no pursuit (the name ours)
    unsigned char m_5c_1 : 7;
    unsigned char m_mode;   // 0x5d: the AI's mode the script runs in (the name ours)
    unsigned char _5e[0x60 - 0x5e];
    float m_60;   // 0x60
};

class CAiSGoto : public CAiState {
public:
    /* +0x3C and +0x40 (the names ours, by slot) */
    virtual bool Virtual3C() { return !(m_flags & 4); }
    virtual bool Virtual40() { return true; }
    virtual ~CAiSGoto();
};

class CAiSPickup : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual ~CAiSPickup();

    unsigned int m_58;   // 0x58
    unsigned int m_5c;   // 0x5c
};

class CAiSPlan : public CAiState {
public:
    virtual ~CAiSPlan();
};

/* the "action" state (new in SOCOM II, the name its RTTI's): an object at +0x84 it waits on */
class CAiSAction : public CAiState {
public:
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual bool CanAttack() const;
    virtual bool CanScan() const;
    virtual bool CanPursue(CSealCtrlAi *ai) const;
    virtual bool Virtual38();
    virtual ~CAiSAction();

    unsigned char _58[0x84 - 0x58];
    void *m_84;   // 0x84
};

class CAiSFollow : public CAiState {
public:
    /* +0x3C (the name ours, by slot): close behind -- its distance under the follow range, and
     * its flag bit 3 set */
    virtual bool Virtual3C();
    virtual ~CAiSFollow();

    unsigned char _58[0x6c - 0x58];
    unsigned int m_6c;   // 0x6c: flags
    unsigned char _70[0xf4 - 0x70];
    float m_f4;   // 0xf4
};

/* the vehicle state (new in SOCOM II; seal_aivehicle.cpp) */
class CAiSUseVehicle : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual ~CAiSUseVehicle();
    void SetTimeout(float t);

    unsigned int m_58;   // 0x58
    unsigned int m_5c;   // 0x5c: an entity's id (CEntity::invalidId on a reset)
    float m_60;   // 0x60: the timeout
    unsigned int m_64;   // 0x64: its clock
};

/* seal_airesponse.cpp's states */
class CAiSHostage : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual bool CanScan() const { return true; }
    virtual ~CAiSHostage();

    int m_58;   // 0x58 (-1 on a reset)
    unsigned int m_5c;   // 0x5c: an entity's id (CEntity::invalidId on a reset)
};

class CAiSAvoid : public CAiState {
public:
    virtual ~CAiSAvoid();
};

class CAiSRush : public CAiState {
public:
    virtual ~CAiSRush();
};

class CAiSFlee : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return true; }
    virtual bool CanScan() const { return false; }
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    virtual ~CAiSFlee();

    bool m_58;   // 0x58 (set on a reset)
    unsigned char _59[0x60 - 0x59];
    unsigned int m_60;   // 0x60
};

class CAiSTarget : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual ~CAiSTarget();
};

class CAiSEvent : public CAiState {
public:
    virtual bool CanAttack() const { return !(m_flags & 0xc); }
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    virtual ~CAiSEvent();
};

class CAiSStunResponse : public CAiState {
public:
    virtual bool CanAttack() const { return false; }
    virtual ~CAiSStunResponse();
};

class CAiSHold : public CAiState {
public:
    /* +0x40: a hold the flag's bit 8 has not ended (the name ours) */
    virtual bool Virtual40() { return !(m_flags & 0x100); }
    virtual ~CAiSHold();
};

class CAiSSurrender : public CAiState {
public:
    virtual bool CanPushAiState(AI_STATE state, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual const char *GetScriptResponse() const { return "on_surrender"; }
    virtual ~CAiSSurrender();
};

class CAiSNoAmmo : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual ~CAiSNoAmmo();

    unsigned int m_58;   // 0x58
};

class CAiSReload : public CAiState {
public:
    virtual bool CanAttack() const { return false; }
    virtual ~CAiSReload();
};

class CAiSShotAt : public CAiState {
public:
    virtual bool CanPursue(CSealCtrlAi *ai) const { return false; }
    virtual ~CAiSShotAt();
};

#endif
