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

/* the states a controller (or a state) runs, the current one at the front (SOCOM 1's DWARF:
 * CAiStateStack : list, 0xc bytes -- CSealCtrlAi::m_stateStack there; SOCOM II also gives every
 * CAiState one, at +8). The stack owns its states. */
class CAiStateStack : public std::list<CAiState *> {
public:
    /* both out of line, seal_aistate.cpp's: every other unit calls the destructor (~CAiState's
     * copy in seal_unit.cpp's tail, 0x5EA450, calls it where an inline one would be inlined a
     * level) */
    ~CAiStateStack();
    /* every state deleted, the stack emptied (the name ours; 0x5F8390) */
    void Clear();
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
    virtual bool Tick(float dT, CSealCtrlAi *ai) { return true; }        /* +0x10 */
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

    unsigned int m_4;   // 0x4 (zeroed by the constructor)
    CAiStateStack m_stack;   // 0x8: the state's own sub-states (new in SOCOM II)
    unsigned char _14[0x2c - 0x14];
    unsigned int m_flags;   // 0x2c (zeroed by the constructor; the overrides test single bits)
    AI_STATE m_state;   // 0x30
    unsigned char _31[0x34 - 0x31];
    unsigned int m_34;   // 0x34 (zeroed by the constructor; CAiSAction's phase)
    unsigned char _38[0x58 - 0x38];
};

/* the states whose code is this lane's (each class only what a written function needs) */
class CAiSCQBAttack : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual bool CanAttack() const { return false; }
    virtual ~CAiSCQBAttack();

    unsigned int m_58;   // 0x58
    float m_5c;   // 0x5c: a delay (0.75, or none when the AI's bit at 0x264 is set)
};

class CAiSUnknown : public CAiState {
public:
    virtual void OnSignal(SIGNAL sig, CSealCtrlAi *ai);
    virtual ~CAiSUnknown();
};

class CAiSSuspended : public CAiState {
public:
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
    virtual bool CanAttack() const;
    virtual ~CAiSScript();
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

class CAiSUseVehicle : public CAiState {
public:
    virtual ~CAiSUseVehicle();
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
