/* zgame.h -- the game: its states' stack and the queue of changes to it (SOCOM 1's
 * Z:\dev\gamez\zgame\zgame.h, the DWARF's file for CGame's inlines). Layouts are SOCOM 1's DWARF
 * moved to where SOCOM II's code reads them. Apps/FTS/fts_state.h keeps its own stand-in CGame for
 * the states until the states include this. */
#ifndef ZGAME_H
#define ZGAME_H

#include <deque>
#include <vector>

/* a state of the game's stack, as the game calls it (Apps/FTS/fts_state.h has the whole class;
 * the slots here are its first eight) */
class CGameState {
public:
    virtual ~CGameState();
    virtual void PreInit();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void PreUnInit();
    virtual void UnInit();
    virtual void OnPush(int userdata);
    virtual void OnPop(int userdata);
};

/* a change of state waiting in the game's queue: what to do, to which state, with what -- kept
 * in a pool (m_pool) and newed only when the pool runs dry */
class CGameStateChangeCmd {
public:
    enum { POP, PUSH, SWITCH };

    /* the bools stored through parameters: MW computes `b & 1` for each bit even when the
     * argument is a constant (Release, the constructor, resize's default element) */
    CGameStateChangeCmd(unsigned int cmd = 0, CGameState *state = 0, unsigned int arg = 0,
                        bool pooled = false, bool inuse = false)
    {
        m_pooled = pooled;
        m_inuse = inuse;
        m_cmd = cmd;
        m_state = state;
        m_arg = arg;
    }
    /* user-declared (SOCOM 1 links an out-of-line copy): Release's delete keeps its null test,
     * which a trivial destructor lets MW fold into the test before it */
    ~CGameStateChangeCmd() {}
    void Set(unsigned int cmd, CGameState *state, unsigned int arg, bool inuse)
    {
        m_cmd = cmd;
        m_state = state;
        m_arg = arg;
        m_inuse = inuse;
    }

    static void DestroyPool();
    static void CreatePool(unsigned int n);
    static void Release(CGameStateChangeCmd *cmd);
    static CGameStateChangeCmd *Acquire(unsigned int cmd, CGameState *state, unsigned int arg);

    CGameState *m_state;   // 0x0
    unsigned int m_arg;   // 0x4
    unsigned char m_cmd;   // 0x8
    bool m_pooled : 1;   // 0x9, bit 0: one of the pool's
    bool m_inuse : 1;   // 0x9, bit 1: handed out
};   // 0xc bytes

typedef std::vector<CGameStateChangeCmd> CGameStateChangeCmdVector;

/* the queue of changes (SOCOM 1's CStateCmdQueue : deque<CGameStateChangeCmd *>) */
class CStateCmdQueue : public std::deque<CGameStateChangeCmd *> {
};

class CGame;
/* CGame::Pop: written as its placeholder while 0x2EAB80 (the online lane's lobby method) still
 * carries the name Pop__5CGameFUi */
extern "C" CGameState *func_002CF2F0(CGame *game, unsigned int arg);

/* the game (SOCOM 1's DWARF; SOCOM II's 0x78 bytes: the queue 0x18, m_pad and m_maxtick after it) */
class CGame {
public:
    enum { MAX_STATES = 16 };

    CGame();
    virtual ~CGame();
    virtual void StartPlay();
    virtual void StopPlay();
    virtual bool StartEngine();
    virtual void StopEngine();
    virtual int Tick(float dT);

    CGameState *Pop(unsigned int arg) { return func_002CF2F0(this, arg); }
    CGameState *Push(CGameState *state, unsigned int arg);
    CGameState *Switch(CGameState *state, unsigned int arg);
    CGameState *PopTo(CGameState *state, unsigned int arg);

    /* the state on top, none past the stack's end */
    CGameState *GetCurrentState() { return m_CurrentState >= MAX_STATES ? 0 : m_Stack[m_CurrentState]; }

    unsigned char m_Active;   // 0x4
    CGameState *m_InitialState;   // 0x8
    unsigned int m_CurrentState;   // 0xc
    CGameState *m_TestVariable;   // 0x10
    CGameState *m_Stack[MAX_STATES];   // 0x14
    int m_MinIdlePeriod;   // 0x54
    CStateCmdQueue m_StateChangeQueue;   // 0x58
    unsigned int m_pad;   // 0x70
    float m_maxtick;   // 0x74
};   // 0x78 bytes

#endif
