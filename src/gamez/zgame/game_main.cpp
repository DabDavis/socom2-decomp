/* game_main.cpp -- the game's core: its tick, which carries out the queued changes of state, its
 * constructor and destructor, and the network's tick thread (SOCOM 1's game_main.cpp, whose static
 * initialiser only sets OurGameName, as SOCOM II's 0x401BC0 does). OVL1 0x2CE800-0x2CF1C7: the
 * thread's start and its stack, NetTickThreadEntry, CGame::Tick, ~CGame and CGame, then the template
 * tail -- the queue's deque<const void *> pieces (include/deque), which every later unit with a deque
 * of pointers links to. The state stack's commands (Pop, Push, Switch, PopTo) are game_state.cpp's:
 * the deque's push_back they call is emitted in that unit's tail, among the pool's vector pieces.
 * Defined in reverse of their image order (-inline deferred).
 *
 * The tail: byte-exact but two. OPEN __deque_buf's destructor (0x2CEFB0): the walk over the blocks
 * is right, but the target then inlines five this-tested levels down to the buffer's storage, where
 * every shape measured stops at four under an out-of-class destructor (build/scratch/states/av3.py)
 * -- the target's buffer destructor (0x2CEE90, three levels) was generated before it in the tail,
 * which may let MW count it as one. OPEN clear (0x2CF120): right but for cdeque::pop_front, which
 * the target inlines (generated before clear: 0x2CF0F0) and this unit's tail generates after it.
 * Both are tail order: MW also emits here, unlike the target, out-of-line copies of the queue's
 * implicit destructors (~CStateCmdQueue down to ~__deque_imp, referenced by CGame's constructor),
 * and those copies generate __deque_buf's destructor ahead of the buffer's. */

#include "zgame.h"

extern "C" {
void *memset(void *p, int c, unsigned int n);

struct ThreadParam {
    int status;
    void (*entry)(void *);
    void *stack;
    int stackSize;
    void *gpReg;
    int initPriority;
    int currentPriority;
    unsigned int attr;
    unsigned int option;
    int waitType;
    int waitId;
    int wakeupCount;
};
int CreateThread(ThreadParam *param);
int StartThread(int id, void *arg);
void ExitDeleteThread(void);

/* the linker's $gp */
extern char _gp[];
}

void zSysSleep(int ms);
void *_memalign(unsigned int align, unsigned int size, const char *file, int line);

/* the network (SOCOM 1's CZNetwork: zNetUpdate and zNetTick, the session flags at 0 and 1) */
class CZNetwork {
public:
    unsigned char m_8;   // 0x0
    unsigned char m_9;   // 0x1: an online game
};
extern CZNetwork theNetwork;
extern "C" void func_0030DA00(CZNetwork *net);   /* CZNetwork::zNetUpdate (SOCOM 1's call here) */
extern "C" int func_0030D9B0(CZNetwork *net);    /* CZNetwork::zNetTick */

class CConsole {
public:
    void Tick();
};
extern CConsole data_00416240;   /* SOCOM 1's theConsole, ticked here */

/* the voice chat's pre-tick in SOCOM 1's place (SOCOM II's, unnamed) */
extern "C" void func_003B05E0(void);

/* the network tick thread's state: running, its period, its stack (SOCOM 1's gp-relative statics;
 * the names ours) */
extern volatile bool data_003DF1F0;   /* the thread runs while set; it sets it again on leaving */
extern unsigned int data_0049E2D4;     /* the tick period in ms */
extern unsigned int data_0044D698;     /* the thread's sleep, a twentieth of it to start */
extern char *data_0044D690;            /* the thread's stack */
extern char *data_0044D6A0;            /* the block it is carved from */

void NetTickThreadEntry(void *arg);

/* the network's tick thread started: its stack painted (250 at both ends, 234 between) and the
 * thread created at priority 4 (OVL1 0x2CE800; SOCOM 1's Tick did this in place -- SOCOM II's Tick
 * calls it, so it is pinned) */
#pragma auto_inline off
extern "C" void func_002CE800(void)
{
    ThreadParam param;

    if (!data_0044D690)
        data_0044D690 = data_0044D6A0;
    memset(data_0044D690, 250, 2048);
    memset(data_0044D690 + 0x10800, 250, 2048);
    memset(data_0044D690 + 2048, 234, 0x10000);
    param.entry = NetTickThreadEntry;
    param.option = 0;
    param.stackSize = 0x10000;
    param.initPriority = 4;
    param.gpReg = _gp;
    param.stack = data_0044D690 + 2048;
    int id = CreateThread(&param);

    if (id)
        StartThread(id, 0);
}
#pragma auto_inline reset

/* the thread's stack allocated once (OVL1 0x2CE8E0; CMission::Init calls it) */
extern "C" void func_002CE8E0(void)
{
    if (!data_0044D6A0)
        data_0044D6A0 = (char *)_memalign(16, 0x11000, 0, 0);
}

/* the network ticked every period until told to stop (OVL1 0x2CE930, SOCOM 1's) */
void NetTickThreadEntry(void *arg)
{
    data_0044D698 = data_0049E2D4 / 20;
    data_003DF1F0 = true;
    while (data_003DF1F0) {
        zSysSleep(data_0044D698);
        if (func_0030D9B0(&theNetwork))
            data_0044D698 = data_0049E2D4;
    }
    data_003DF1F0 = true;
    ExitDeleteThread();
}

/* the network and the console ticked, then the first queued change carried out -- the new state
 * told and set up, the old one torn down -- or the current state ticked (OVL1 0x2CE9E0).
 * OPEN, 0x324 against 0x32C: registers and the queue's inlined front() are right (cmd declared
 * ahead of online gives the target's s2/s1); the target leaves a nop after each clamp's
 * `m_CurrentState = 15` store (PUSH and SWITCH) and schedules SWITCH's `cur->Init()` call
 * differently -- the clamp's shape, not its value (SOCOM 1's debug build shows no nop) */
int CGame::Tick(float dT)
{
    CGameStateChangeCmd *cmd;
    unsigned char online = theNetwork.m_9;

    func_0030DA00(&theNetwork);
    data_00416240.Tick();
    CGameState *cur = GetCurrentState();

    if (m_Active) {
        if (m_StateChangeQueue.size()) {
            if (online)
                func_002CE800();
            cmd = m_StateChangeQueue.front();

            switch (cmd->m_cmd) {
            case CGameStateChangeCmd::PUSH:
                if (cmd->m_state) {
                    func_003B05E0();
                    m_Stack[m_CurrentState]->OnPush(cmd->m_arg);
                    if (cmd->m_state->Init()) {
                        m_CurrentState++;
                        if (m_CurrentState >= MAX_STATES)
                            m_CurrentState = MAX_STATES - 1;
                        m_Stack[m_CurrentState] = cmd->m_state;
                    }
                }
                break;
            case CGameStateChangeCmd::POP:
                if (cur)
                    cur->UnInit();
                m_Stack[m_CurrentState] = 0;
                m_CurrentState--;
                m_Stack[m_CurrentState]->OnPop(cmd->m_arg);
                break;
            case CGameStateChangeCmd::SWITCH:
                if (cmd->m_state) {
                    if (cur)
                        cur->UnInit();
                    func_003B05E0();
                    if (m_CurrentState >= MAX_STATES)
                        m_CurrentState = MAX_STATES - 1;
                    if (cmd->m_state->Init())
                        m_Stack[m_CurrentState] = cmd->m_state;
                    else if (cur)
                        cur->Init();
                }
                break;
            }
            m_StateChangeQueue.pop_front();
            CGameStateChangeCmd::Release(cmd);
            if (online) {
                if (data_003DF1F0) {
                    data_003DF1F0 = false;
                    while (data_003DF1F0 == false)
                        ;
                    data_003DF1F0 = false;
                }
                data_0044D690 = 0;
            }
        } else if (cur)
            cur->Tick(dT);
    }
    return 0;
}

/* (OVL1 0x2CED10: the queue's destructor inlined four levels, its blocks' owner called) */
CGame::~CGame()
{
}

/* (OVL1 0x2CED90: the members before the queue initialised ahead of it, SOCOM 1's order) */
CGame::CGame()
    : m_Active(0), m_InitialState(0), m_CurrentState(0), m_MinIdlePeriod(100)
{
    m_TestVariable = 0;
    memset(m_Stack, 0, sizeof m_Stack);
    m_pad = 0;
    m_maxtick = 0;
}
