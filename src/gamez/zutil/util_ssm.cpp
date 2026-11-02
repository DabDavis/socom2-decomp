/* util_ssm.cpp -- CSimpleStateMachine (OVL1 0x3A59D0-0x3A719F; the class by its RTTI at 0x3EF738,
 * its vtable 0x408240): a state machine whose states are member functions, each told what happens
 * to it (0 entered, 1 ticked, 2 left), changed through a queue of the member pointers, run on a
 * thread of its own or ticked by its owner. The network's start-up (CSMInitNetwork), the DNAS
 * authentication, the patch download and the Medius server list derive from it. SOCOM 1 has
 * none of it: the file's name and every member's but the class's are ours. Defined in reverse of
 * their image order (-inline deferred); the queue's deque<SSMState> pieces close the unit.
 *
 * Start is the class's one non-inline virtual (so this unit emits the vtable); SetState is inline,
 * its out-of-line copy linked from an earlier unit (0x276F40). */

#include <queue>
#include "zutil.h"

extern "C" {
/* the EE kernel's (eekernel.h) */
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
int RotateThreadReadyQueue(int priority);

/* the linker's $gp */
extern char _gp[];
}

class CSimpleStateMachine;
typedef void (CSimpleStateMachine::*SSMState)(unsigned char event);

extern "C" void func_002852B0(void *queue, SSMState state);   /* the state queue's push (0x2852B0) */

/* the state queue (ours: a class of its own around std::queue -- its destructor is the fourth
 * this-tested level ~CSimpleStateMachine inlines before __deque_buf's) */
class CStateQueue : public std::queue<SSMState> {
};

class CSimpleStateMachine {
public:
    CSimpleStateMachine();
    ~CSimpleStateMachine();
    /* the machine put back in its idle state, not done */
    virtual bool Start();
    /* a state change queued (inline: every machine's vtable holds the copy linked first, 0x276F40) */
    virtual void SetState(const SSMState &state)
    {
        SSMState s = state;

        func_002852B0(&m_queue, s);
    }

    bool StartThread(int stackSize, int priority);
    static void ThreadEntry(void *arg);
    void Step();
    unsigned char IsDone();
    void Tick();
    void Idle(unsigned char event);
    void Finish(unsigned char event);

    int m_thread;   // 0x4
    SSMState m_state;   // 0x8
    CStateQueue m_queue;   // 0x14
    unsigned char m_done;   // 0x2c
    void *m_stack;   // 0x30
    int m_priority;   // 0x34
};   // 0x38 bytes

CSimpleStateMachine::CSimpleStateMachine()
{
    m_state = &CSimpleStateMachine::Idle;
    m_thread = 0;
    m_stack = 0;
}

/* a running thread told to finish (unless it is done) and waited for, its stack freed */
CSimpleStateMachine::~CSimpleStateMachine()
{
    if (m_thread) {
        if (!IsDone())
            SetState(&CSimpleStateMachine::Finish);
        while (m_thread)
            RotateThreadReadyQueue(m_priority);
    }
    if (m_stack)
        _free(m_stack, 0, 0);
    m_stack = 0;
}

/* the idle state: nothing */
void CSimpleStateMachine::Idle(unsigned char event)
{
}

/* the finishing state: done once entered or ticked */
void CSimpleStateMachine::Finish(unsigned char event)
{
    switch (event) {
    case 0:
        m_done = true;
        break;
    case 1:
        m_done = true;
        break;
    case 2:
        break;
    }
}

/* a tick from the owner: a threaded machine's thread given the processor, else a step */
void CSimpleStateMachine::Tick()
{
    if (m_thread) {
        RotateThreadReadyQueue(m_priority);
        return;
    }
    Step();
}

/* done: no thread and the flag */
unsigned char CSimpleStateMachine::IsDone()
{
    if (m_thread)
        return false;
    return m_done;
}

/* a step: a queued state is changed to (the old one left, the new one entered), else the state
 * ticked (called out of line by Tick and ThreadEntry: pinned) */
#pragma auto_inline off
void CSimpleStateMachine::Step()
{
    if (m_queue.size()) {
        (this->*m_state)(2);
        m_state = m_queue.front();
        m_queue.pop();
        (this->*m_state)(0);
    } else
        (this->*m_state)(1);
}

#pragma auto_inline reset

bool CSimpleStateMachine::Start()
{
    SetState(&CSimpleStateMachine::Idle);
    m_done = 0;
    return false;
}

/* the thread's body: started, then stepped until done, the processor given up between steps */
void CSimpleStateMachine::ThreadEntry(void *arg)
{
    CSimpleStateMachine *sm = (CSimpleStateMachine *)arg;

    sm->Start();
    while (!sm->m_done) {
        sm->Step();
        RotateThreadReadyQueue(sm->m_priority);
    }
    sm->m_thread = 0;
    ExitDeleteThread();
}

/* the machine run on a thread of its own (its stack allocated once) */
bool CSimpleStateMachine::StartThread(int stackSize, int priority)
{
    ThreadParam param;

    m_priority = priority;
    if (!m_stack)
        m_stack = _memalign(16, stackSize, 0, 0);
    param.entry = ThreadEntry;
    param.stack = m_stack;
    param.stackSize = stackSize;
    param.initPriority = m_priority;
    param.gpReg = _gp;
    param.option = 0;
    m_thread = CreateThread(&param);
    if (m_thread)
        ::StartThread(m_thread, this);
    return true;
}
