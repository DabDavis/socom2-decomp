/* zsys_init.cpp -- the system's start and reset: the DMA channels and the timer, the IOP modules
 * (the memory card's and SOCOM II's hard disk's), sleeping, and the IOP manager (OVL1
 * 0x34F0E0-0x35094F). The file name is SOCOM 1's (its static initialiser, which builds the IOP
 * manager's vector, pairs with __sinit_zsys_init.cpp). Defined in reverse of their image order
 * (-inline deferred). */

#include <vector>

#include "zsys_public.h"
#include "../zvid/zvid.h"
/* CFileIO (zutil.h): its static PS2FileName (SOCOM 1's) builds the module paths */
#include "../zutil/zutil.h"

extern "C" {
int GetThreadId(void);
int SetAlarm(unsigned short time, void (*handler)(int, unsigned short, void *), void *arg);
int SleepThread(void);
void sceGsResetPath(void);
void sceDmaReset(int mode);
sceDmaChan *sceDmaGetChan(int id);
int sceSifLoadModule(const char *filename, int args, const char *argp);
int sceSifLoadStartModule(const char *filename, int args, const char *argp, int *result);
int sceSifStopModule(int modid, int args, const char *argp, int *result);
int sceSifUnloadModule(int modid);
int sceDevctl(const char *name, int cmd, void *arg, unsigned int arglen, void *buf,
              unsigned int buflen);
int sceUmount(const char *name);
void func_0034BCA0(void);                                  /* SOCOM II's, a sound flag cleared */
}

/* the IOP's manager (SOCOM 1's class; SOCOM II adds reboot modes 4-6, names ours, and the mode
 * last rebooted into at 0xC, which RebootIOP tests first) */
class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0, PostReboot = 1 };
    enum RebootMode {
        FirstBoot = 0,
        Singleplayer = 1,
        Multiplayer = 2,
        NetConfig = 3,
        RebootMode4 = 4,
        RebootMode5 = 5,
        Shutdown = 6
    };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);

    /* a registered callback (SOCOM 1's CRegistrant, its DWARF's members). Its user-declared
     * constructor builds nothing, but makes the implicit copy member by member: the reserve
     * 0x3506B0 copies the three words through v0 and the flag with lbu/sb, as SOCOM 1's does --
     * implicit (a trivial class), the copy is the FPU's four-word block */
    class CRegistrant {
    public:
        CRegistrant() {}

        RebootCallback m_pcb;           // 0x0
        void *m_userdata;               // 0x4
        int m_id;                       // 0x8
        bool m_flag_for_deletion;       // 0xc
    };   // 0x10 bytes

    /* room for sixteen callbacks, the first boot's mode (SOCOM 1's reserve; inline: the unit's
     * initialiser 0x403460 zeroes the vector itself, calls the reserve 0x3506B0, then clears the
     * mode) */
    CIOPManager()
    {
        m_reboot_callback_vec.reserve(16);
        m_mode = FirstBoot;
    }
    void RebootIOP(RebootMode mode);

    std::vector<CRegistrant> m_reboot_callback_vec;   // 0x0 (SOCOM 1's)
    RebootMode m_mode;                                // 0xc (SOCOM II's; the name ours)
};

/* SOCOM 1's (its __sinit_zsys_init.cpp builds it, as SOCOM II's 0x403460 does; ~CIOPManager
 * 0x3508C0 closes the unit's tail) */
CIOPManager theIOPManager;   /* 0x49E2A0 */

/* an IOP module and how to load it (SOCOM II's table rows; names ours) */
struct IRX_MODULE {
    bool start;         /* 0x00: loaded and started, or loaded only */
    int id;             /* 0x04: -1 while not loaded */
    char *name;         /* 0x08 */
    int argc;           /* 0x0C */
    char *argv;         /* 0x10 */
    int result;         /* 0x14: the module's start result */
};

/* the hard disk's arguments (SOCOM II's) */
static char s_hddArgs[] = "-o\0" "1\0" "-n\0" "3";             /* 0x3E0218 */
static char s_pfsArgs[] = "-m\0" "1\0" "-o\0" "1\0" "-n\0" "10";  /* 0x3E0230 */

/* 0x3E0250: the hard disk's modules (SOCOM II's) */
static IRX_MODULE s_hddModules[3] = {
    { true, -1, "RUN\\IRX\\HDD\\ATAD.IRX", 0, 0, 0 },
    { true, -1, "RUN\\IRX\\HDD\\HDD.IRX", 9, s_hddArgs, 0 },
    { true, -1, "RUN\\IRX\\HDD\\PFS.IRX", 16, s_pfsArgs, 0 },
};

/* 0x3E02A0: the memory card's modules (SOCOM II's) */
static IRX_MODULE s_mcModules[2] = {
    { false, -1, "RUN\\IRX\\MCMAN.IRX", 0, 0, 0 },
    { false, -1, "RUN\\IRX\\MCSERV.IRX", 0, 0, 0 },
};

int s_hddDisabled;              /* 0x3E01B4: SOCOM II's, 1 keeps the disk off (name ours) */
int s_hddStatus;                /* 0x49E2DC: the disk's devctl answer, -1 before (name ours) */
bool s_hddMounted;              /* 0x49E2E0 (name ours) */

#define VIF0_FBRST ((volatile unsigned int *)0x10003810)
#define VIF1_ERR ((volatile unsigned int *)0x10003C20)
#define T0_MODE ((volatile unsigned int *)0x10000010)
#define D_CTRL ((volatile unsigned int *)0x1000E000)

/* the bus's H-blank rate (SOCOM 1's _zvid_public::hblnkRate, 0x78 in SOCOM II's) */
#define HBLNK_RATE (*(float *)((char *)&zVid + 0x78))

/* SOCOM II's: the IOP rebooted for good, the sound told (name ours) */
void zSysShutdown()
{
    theIOPManager.RebootIOP(CIOPManager::Shutdown);
    func_0034BCA0();
}

/* SOCOM 1's, empty in SOCOM II */
void zSysPostInit()
{
}

void zSysInit()
{
    zsys_FullAllocAndFree();
    zSys.isCdBoot = 0;
    s_hddStatus = -1;
    s_hddMounted = false;
    theIOPManager.RebootIOP(CIOPManager::Singleplayer);
}

/* SOCOM 1's; inlined in SOCOM II, its own copy not linked */
inline void zSysUnloadIRX(int id)
{
    int result;

    sceSifStopModule(id, 0, 0, &result);
    sceSifUnloadModule(id);
}

int zSysLoadIRX(char *name, int argc, char *argv, bool retry)
{
    char path[256];
    int id;

    CFileIO::PS2FileName(name, path, false);
    do
        id = sceSifLoadModule(path, argc, argv);
    while (id < 0 && retry);
    return id;
}

/* SOCOM II's: a module loaded and started (name ours; inlined, not linked) */
inline int zSysLoadStartIRX(char *name, int argc, char *argv, int *result)
{
    char path[256];

    CFileIO::PS2FileName(name, path, false);
    return sceSifLoadStartModule(path, argc, argv, result);
}

/* SOCOM II's: the memory card's modules loaded (name ours) */
void zSysLoadMcIRX()
{
    IRX_MODULE *m = s_mcModules;

    for (int i = 0; i < 2; i++, m++) {
        if (!m->start)
            m->id = zSysLoadIRX(m->name, m->argc, m->argv, true);
        else
            m->id = zSysLoadStartIRX(m->name, m->argc, m->argv, &m->result);
    }
    zSys.mcIrxLoaded = 1;
}

/* SOCOM II's: and unloaded, last first (name ours). A named pointer started at the table and
 * stepped to its last module as a statement of its own: the +24 then stays a separate addiu after
 * the relocation, as the target's, and the pointer, a named local, takes s2 above the counter
 * (indexing, or &s_mcModules[1], folds the offset and swaps the two) */
void zSysUnloadMcIRX()
{
    IRX_MODULE *m = s_mcModules;

    m++;
    for (int i = 1; i >= 0; i--, m--) {
        if (m->id != -1) {
            zSysUnloadIRX(m->id);
            m->id = -1;
        }
    }
    zSys.mcIrxLoaded = 0;
}

void zSysReset()
{
    sceGsResetPath();
    sceDmaReset(1);
    *VIF0_FBRST = 1;
    *VIF1_ERR = 6;
    zSysTimerInit();
    *T0_MODE = 0x83;
    zSys.DmaToVif0 = sceDmaGetChan(0);
    zSys.DmaToVif1 = sceDmaGetChan(1);
    zSys.DmaToGif = sceDmaGetChan(2);
    zSys.DmaFromSPR = sceDmaGetChan(8);
    zSys.DmaToVif0->chcr.DIR = 1;
    zSys.DmaToVif0->chcr.MOD = 1;
    zSys.DmaToVif0->chcr.ASP = 0;
    zSys.DmaToVif0->chcr.TTE = 1;
    zSys.DmaToVif0->chcr.TIE = 0;
    zSys.DmaToVif1->chcr.DIR = 1;
    zSys.DmaToVif1->chcr.MOD = 1;
    zSys.DmaToVif1->chcr.ASP = 0;
    zSys.DmaToVif1->chcr.TTE = 1;
    zSys.DmaToVif1->chcr.TIE = 0;
    zSys.DmaToGif->chcr.DIR = 1;
    zSys.DmaToGif->chcr.MOD = 1;
    zSys.DmaToGif->chcr.ASP = 0;
    zSys.DmaToGif->chcr.TTE = 1;
    zSys.DmaToGif->chcr.TIE = 0;
    zSys.DmaFromSPR->chcr.DIR = 0;
    zSys.DmaFromSPR->chcr.MOD = 1;
    zSys.DmaFromSPR->chcr.ASP = 0;
    zSys.DmaFromSPR->chcr.TTE = 1;
    zSys.DmaFromSPR->chcr.TIE = 0;
    *D_CTRL = 1;
    zSys.sprPacketEnd[0] = (u_long128 *)0x70001F20;
    zSys.sprPacketEnd[1] = (u_long128 *)0x70003E50;
    zSys.sprPx = 0;
    asm volatile { sync }
    zSysFifoInit();
}

/* SOCOM II's: the hard disk's modules loaded once and the disk asked for (name ours) */
bool zSysHddInit()
{
    if (s_hddDisabled != 1 && s_hddStatus == -1) {
        bool ok = true;

        if (!zSys.hddIrxLoaded) {
            unsigned int i;
            IRX_MODULE *m = s_hddModules;

            for (i = 0; i < 3 && ok; i++, m++) {
                if (!m->start)
                    m->id = zSysLoadIRX(m->name, m->argc, m->argv, true);
                else
                    m->id = zSysLoadStartIRX(m->name, m->argc, m->argv, &m->result);
                if (m->result == 1)
                    ok = false;
            }
            zSys.hddIrxLoaded = 1;
        }
        if (ok)
            s_hddStatus = sceDevctl("hdd0:", 0x4807, 0, 0, 0, 0);
    }
    return s_hddStatus == 0;
}

/* SOCOM II's: the disk's partition unmounted (name ours) */
bool zSysHddUnmount(const char *name)
{
    int r = 0;

    if (s_hddMounted) {
        r = sceUmount(name);
        s_hddMounted = false;
        s_hddStatus = -1;
    }
    return r != 0;
}

/* SOCOM II's: the timer's rate and its seconds per tick, from the H-blank rate (name ours) */
void zSysTimerInit()
{
    zSys.timerTicksPerSecond = HBLNK_RATE;
    zSys.timerScale = 1.0f / zSys.timerTicksPerSecond;
}

static void SleepAlarmCB(int id, unsigned short time, void *thread)
{
    iWakeupThread((int)thread);
}

void zSysSleep(int ticks)
{
    SetAlarm(ticks, SleepAlarmCB, (void *)GetThreadId());
    SleepThread();
}
