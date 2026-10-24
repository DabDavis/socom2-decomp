/* st_core.cpp -- the core state: the mission's load, its tick, its teardown (SOCOM 1's
 * Apps/FTS/st_core.cpp, its DWARF's line order; OVL1 0x1EB070-0x1EDFFF). Defined in reverse of
 * their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_00356BE0(void *texbuffer, bool now);   /* zdb::CGSTexBuffer::Load */
void func_001F1820(float goal);                  /* CameraZoom::SetGoal */
void func_001F1870(void *seal);                  /* CameraZoom::AttachSeal */
void *func_002B3580();                           /* ftsGetPlayer */
void func_002A7D40(void *mission);               /* CMission::OnMissionStart */
void func_001FB420(CHUD *hud);                   /* CHUD::ReInit_Init */
void func_00356130(void *texmanager);            /* zdb::CTexManager::doSetupBuffers */
int func_001EA720();                             /* testLoad */
void func_00192BF8(int code);                    /* exit */
void func_00223290(void *state);                 /* CMPExitState::LoadLibs */
void func_002A6950(void *mission);
void func_003B17D0(int rate);                    /* CVideo::MaxFrameRate */
void func_003B1760(int a, int b, int c, float d);
void func_00351CB0();                            /* CValve::Reset */
void func_005B61A0();

extern unsigned char data_004A4511;
extern int data_0049E940;                        /* zdb::CTexManager::TexPhase's */
extern int data_0049E950;
extern unsigned char data_0045A0C1;              /* theNetwork's +9 (an online game) */
extern void *data_00408C10;                      /* the load thread's stack */
}

void zVid_Sync0();

/* zdb's task scheduler: a list of tasks (SOCOM 1's DWARF: 0x20 bytes) */
class CSched_Manager {
public:
    void ClearFreeList(bool all);

    unsigned char _0[0x20];
};

extern CSched_Manager zTaskScheduler;

/* the mission's texture buffers: a vector of them */
struct TexBufferVector {
    unsigned int capacity_;
    unsigned int size_;
    void **data_;
};

extern "C" TexBufferVector data_0049E880;          /* zdb::CTexManager::m_texmanager */

static void LoadThreadEntry(void *arg);

/* the film's name let go of */
CCinematicState::~CCinematicState()
{
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
}

/* a pad's actuators stopped (the first pad's) */
void CCoreState::OnPush(int userdata)
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    CPad *pad = CInput::m_pads[0];

    pad->m_ActData[1] = 0;
    pad->m_ActData[0] = 0;
}

/* back on top: the two texture phases' buffers loaded again, and from a sub-state (1) the mission
 * reopened and restarted, the camera's zoom reset onto the player, the HUD set up again */
void CCoreState::OnPop(int userdata)
{
    func_00356BE0(data_0049E880.data_[data_0049E940], true);
    func_00356BE0(data_0049E880.data_[data_0049E950], true);
    zVid_Sync0();
    theMission.m_108 = 10.0f;
    if (userdata == 1) {
        func_001F1820(1.0f);
        theMission.ReOpen_Load();
        func_002A7D40(&theMission);
        func_001F1870(func_002B3580());
        func_001FB420(m_hud);
    }
    zVid_Sync0();
    m_skip_swap = 2;
}

CCoreState::CCoreState() : CGameState("CoreState")
{
    m_loading = false;
    m_loadstarted = false;
    m_skip_swap = 0;
    m_c0 = 1.0f;
    m_c4 = 1.0f / m_c0;
    m_c8 = m_c0;
    m_LoadThreadId = -1;
}

/* the base's destructor called in the body, so it runs twice: SOCOM 1's and SOCOM II's both inline
 * CGameState's teardown (vtable, name freed) two times over */
CCoreState::~CCoreState()
{
    CGameState::~CGameState();
}

/* torn down: the GIF's path 3 masked again, the frame rate freed, the mission unloaded, the
 * valves reset and the scheduler's free list cleared */
void CCoreState::UnInit()
{
    *(volatile unsigned int *)0x10003010 &= ~4;
    asm volatile {
        sync
    }
    func_003B17D0(-1);
    func_003B1760(0, 0, 0, 1.0f);
    data_004A4511 = 1;
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
    func_00351CB0();
    zTaskScheduler.ClearFreeList(true);
    func_005B61A0();
}

/* the load thread: the sky's colour, the texture buffers, the load itself (exiting when it
 * fails), an online game's exit screens, then the mission's own */
static void LoadThreadEntry(void *arg)
{
    theGame.m_coreState.skyColor[0] = 190;
    theGame.m_coreState.skyColor[1] = 220;
    theGame.m_coreState.skyColor[2] = 250;
    func_00356130(&data_0049E880);
    if (!func_001EA720())
        func_00192BF8(0);
    if (data_0045A0C1)
        func_00223290(theGame.m_MPexitState);
    theGame.m_coreState.m_loading = false;
    func_002A6950(&theMission);
    ExitDeleteThread();
}

/* the load thread started on a stack of its own (allocated once, its ends marked). Written so
 * it is 15 statements and Load would inline it; the original's had more (Load calls it) */
#pragma auto_inline off
void CCoreState::StartLoadThread()
{
    ThreadParam param;

    if (!data_00408C10)
        data_00408C10 = _memalign(16, 0x11000, 0, 0);
    memset(data_00408C10, 250, 2048);
    memset((char *)data_00408C10 + 0x10800, 250, 2048);
    memset((char *)data_00408C10 + 2048, 234, 0x10000);
    param.entry = LoadThreadEntry;
    param.option = 0;
    param.stackSize = 0x10000;
    param.initPriority = 6;
    param.gpReg = data_001DD2F0;
    param.stack = (char *)data_00408C10 + 2048;
    m_LoadThreadId = CreateThread(&param);
    if (m_LoadThreadId)
        StartThread(m_LoadThreadId, this);
}

#pragma auto_inline reset

/* the load thread's stack let go of (the name ours) */
void CCoreState::FreeLoadThread()
{
    m_LoadThreadId = -1;
    if (data_00408C10)
        _free(data_00408C10, 0, 0);
    data_00408C10 = 0;
}

/* the mission loaded: on its own thread, or here and now */
void CCoreState::Load(bool async)
{
    m_loading = true;
    m_loadstarted = true;
    zsys_FullAllocAndFree();
    if (async) {
        StartLoadThread();
        return;
    }
    skyColor[0] = 190;
    skyColor[1] = 220;
    skyColor[2] = 250;
    func_00356130(&data_0049E880);
    if (!func_001EA720())
        func_00192BF8(0);
    if (data_0045A0C1)
        func_00223290(theGame.m_MPexitState);
    m_loading = false;
}

void CCoreState::Unload()
{
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
}
