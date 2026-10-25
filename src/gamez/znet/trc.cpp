/* trc.cpp -- the console's power-off, as Sony's checklist (the TRC) requires: a thread waiting on a
 * semaphore the CD power-off callback signals, which closes the hard disk or the network adaptor
 * and powers the console off; the thread removed before an IOP reboot (SOCOM 1's TRC_* functions
 * and their globals, which follow zNetCnf.cpp's tail in the demo; OVL1 0x30C760-0x30CA4F, before
 * zNetDme.cpp; CIOPManager::RebootIOP calls TRC_PreparePowerOff). The unit's name is ours, after
 * the functions' prefix. Defined in reverse of their image order (-inline deferred). */

extern "C" {
/* the EE kernel's thread and semaphore parameters (Sony's) */
struct SemaParam {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    unsigned int attr;
    unsigned int option;
};
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
};
int CreateSema(SemaParam *sema);
int DeleteSema(int sema);
int WaitSema(int sema);
int iSignalSema(int sema);
int CreateThread(ThreadParam *thread);
int StartThread(int thread, void *arg);
int TerminateThread(int thread);
int DeleteThread(int thread);
int sceCdPOffCallback(void (*func)(void *), void *addr);
int sceCdPowerOff(int *stat);
int sceSifSearchModuleByName(const char *name);
int sceDevctl(const char *name, int cmd, void *arg, unsigned int arglen, void *buf,
              unsigned int buflen);
void func_0034BCA0(void);   /* (zsnd: a sound flag cleared, as zSysReset does) */
extern char _gp[];
}

class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0, PostReboot = 1 };
    enum RebootMode { FirstBoot = 0 };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);
    int RegisterRebootCallback(RebootCallback cb, void *data);
};
extern CIOPManager theIOPManager;

/* the power-off thread's state (SOCOM 1's globals) */
unsigned char m_hddStatus;   /* 0x4580A0: TRC_PreparePowerOff's argument */
int tid;   /* 0x4580A8: the thread, 0 when there is none */
unsigned char g_stack[0x2000];   /* 0x4580B0 */

bool _UnPreparePowerOff(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data);
void TRC_PowerOffThread(void *sema);
void TRC_PowerOffHandler(void *sema);

/* the CD's power-off callback: the thread woken (SOCOM 1's; OVL1 0x30CA40) */
void TRC_PowerOffHandler(void *sema)
{
    iSignalSema((int)sema);
}

/* the hard disk closed and spun down, or the network adaptor off, then the console off (SOCOM 1's;
 * OVL1 0x30C910) */
void TRC_PowerOffThread_Aux()
{
    int stat = 0;

    if (sceSifSearchModuleByName("hdd") >= 0) {
        switch (sceDevctl("hdd:", 0x4807, 0, 0, 0, 0)) {
        case 3:
            while (sceDevctl("dev9x:", 0x4402, 0, 0, 0, 0) < 0)
                ;
            break;
        default:
            sceDevctl("pfs:", 0x5003, 0, 0, 0, 0);
            while (sceDevctl("hdd:", 0x4806, 0, 0, 0, 0) < 0)
                ;
            break;
        }
    } else
        while (sceDevctl("dev9x:", 0x4402, 0, 0, 0, 0) < 0)
            ;
    func_0034BCA0();
    while (!sceCdPowerOff(&stat) || stat)
        ;
}

/* the thread: each signal powers the console off (SOCOM 1's; OVL1 0x30C8E0) */
void TRC_PowerOffThread(void *sema)
{
    for (;;) {
        WaitSema((int)sema);
        TRC_PowerOffThread_Aux();
    }
}

/* before a reboot, the thread and its semaphore removed (SOCOM 1's; OVL1 0x30C850) */
bool _UnPreparePowerOff(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    bool ok = true;
    int sema = (int)data;

    if (phase == CIOPManager::PreReboot) {
        if (tid && TerminateThread(tid) >= 0 && DeleteThread(tid) >= 0) {
            tid = 0;
            DeleteSema(sema);
        }
    } else
        ok = false;
    return ok;
}

/* the power-off thread started on the CD's power-off callback, and its removal registered for the
 * next IOP reboot; the semaphore returned, 0 if the thread was there already (SOCOM 1's; OVL1
 * 0x30C760) */
extern "C" int TRC_PreparePowerOff(unsigned char status)
{
    ThreadParam thread;
    SemaParam semaParam;
    int sema;

    if (tid)
        return 0;
    semaParam.maxCount = 1;
    semaParam.initCount = 0;
    semaParam.option = 0;
    sema = CreateSema(&semaParam);
    thread.stackSize = sizeof(g_stack);
    thread.gpReg = _gp;
    thread.entry = TRC_PowerOffThread;
    thread.stack = g_stack;
    thread.initPriority = 3;
    tid = CreateThread(&thread);
    StartThread(tid, (void *)sema);
    sceCdPOffCallback(TRC_PowerOffHandler, (void *)sema);
    m_hddStatus = status;
    theIOPManager.RegisterRebootCallback(_UnPreparePowerOff, (void *)sema);
    return sema;
}
