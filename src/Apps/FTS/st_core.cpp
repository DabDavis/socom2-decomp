/* st_core.cpp -- the core state: the mission's load, its tick, its teardown (SOCOM 1's
 * Apps/FTS/st_core.cpp, its DWARF's line order; OVL1 0x1EB070-0x1EDFFF). Defined in reverse of
 * their image order (-inline deferred). */

#include "fts_state.h"

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_00356BE0(void *texbuffer, bool now);   /* zdb::CGSTexBuffer::Load */
void func_001F1820(float goal);                  /* CameraZoom::SetGoal */
void func_001F1870(void *seal);                  /* CameraZoom::AttachSeal */
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
void func_002B6530(void *queue, const char *text, int a, int b, int c, float time);   /* C2DMessage_Q::Add_Message */
void func_005B61A0();

}

void ftsNetworkGameUninit();
void DMEDisconnectFromGameServer();

class CZProjectileList {
public:
    void UnInit();
};

class CZMarkList {
public:
    void UnInit();
};

class CZWeapon {
public:
    static CZProjectileList *m_pProjectilelist;
    static CZMarkList *m_pMarklist;
};

class CEntity {
public:
    static bool PreTickAll(float dT, void *data);
};

const char *GetDatabase();

/* a seal's controller (CEntity's at 0xC0) */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    void *m_control;   // 0xc0
};

extern "C" {
void func_00309A10();
void func_003B7150(int arg);
void func_00356B30(void *texbuffer);
void func_001ED000(void *v);
void func_001ECF60(void *v);
int func_00288930();
void func_003B1990(int addr, int buffer, int size);   /* CVideo::ReadGsMemory */
void func_003B0B90(int a, int b, int w, int h);
void func_003B09E0(int a, int b, int w, int h);
void func_00592CA0(void *control);
void func_003466F0();
CZSealBody *func_002B3580();                     /* ftsGetPlayer */

extern char data_00408BD0[];
}

extern "C" {
CHUD *func_001FCD80(CHUD *hud);                  /* CHUD::CHUD */
void func_001FA360(CHUD *hud, zdb::CWorld *world);   /* CHUD::Init */
void func_0032C4F0();                            /* CRdrArchive::CloseAll */
void func_00356B90(void *texbuffer, int size);
void func_0031CED0(zdb::CWorld *world);
void func_0033B420(CPipe *pipe, zdb::CTexHandle *tex);

/* the tick tasks (SOCOM 1's in the comments, where it had them) */
bool func_005E7FD0(float dT, void *data);        /* CSealUnit::TickAll */
bool func_005CCCD0(float dT, void *data);        /* CSealCtrlAi::PreTick */
bool func_003D1F90(float dT, void *data);        /* CZWeapon::PreTick */
bool func_0050ECD0(float dT, void *data);        /* CAiEvent::Tick */
bool func_0029F430(float dT, void *data);        /* CEntity::TickAll */
bool func_0054F570(float dT, void *data);
bool func_0029F350(float dT, void *data);        /* CSealCtrlAi::PrePostTick's place */
bool func_0060BC40(float dT, void *data);
bool func_0029F1A0(float dT, void *data);        /* CEntity::PostTickAll */
bool func_003D1F50(float dT, void *data);        /* CZWeapon::PostTick */
bool func_00510570(float dT, void *data);        /* ai::TickPathPlanner */
bool func_0060B8F0(float dT, void *data);
bool func_005CCA30(float dT, void *data);        /* CSealCtrlAi::PostTick */
bool func_00341A60(float dT, void *data);        /* CSnd::PostTick */
bool func_00299C10(float dT, void *data);

}

extern CHUD *theHUD;

extern "C" {
int func_001EA6E0();                             /* testUnload */
void func_001F1840(CHUD *hud);                   /* CameraZoom::AttachHud */
void func_001FCB70(CHUD **hud);                  /* CHUD::ReleaseHUD */
void func_003D3930();                            /* CZWeapon::DestroyProjectileList */
void func_003D37E0();                            /* CZWeapon::DestroyMarkList */
void func_0032C2B0(const char *name, const char *path);   /* CRdrArchive::RemoveArchive */

}

void ftsSetPlayer(CZSealBody *seal);

/* zdb's task scheduler: a list of tasks (SOCOM 1's DWARF: 0x20 bytes) */
class CSched_Manager {
public:
    bool AddTask(const char *name, bool (*task)(float, void *), float priority, void *data);
    bool RemoveTask(const char *name, bool all);
    void ClearFreeList(bool all);

    unsigned char _0[0x20];
};

extern CSched_Manager zTaskScheduler;

extern "C" void func_00356080(TexBufferVector *texmanager, bool all);   /* zdb::CTexManager::DestroyBuffers */

/* what the core state's tick reaches for */
namespace zdb {
/* the render phases (SOCOM 1's _RenderPhase; only the HUD's three, by number) */
enum _RenderPhase {
    RENDER_PHASE_12 = 12,
    RENDER_PHASE_13 = 13,
    RENDER_PHASE_14 = 14
};
}

class MapCompass {
public:
    void Draw(zdb::CCamera *cam);
};

class CFlashFXVector {
public:
    void Tick(float dT);
    void Draw(zdb::CCamera *cam);
};

/* a 2D item: its vtable pointer after a list (C2D's), Draw the second virtual */
class CTickItem {
public:
    unsigned char _0[0xc];
    virtual ~CTickItem();
    virtual void Draw(zdb::CCamera *cam);
};

/* the HUD as the core state ticks it (SOCOM 1's CHUD: its compass, its items, its flashes) */
class CHUD {
public:
    void PreDITick(float dT);
    void PostDITick(float dT);
    void Tick(float dT);
    void DrawPhase(zdb::_RenderPhase phase, zdb::CCamera *cam);

    unsigned char _0[0x4fe0];
    MapCompass m_compass;   // 0x4fe0
    unsigned char _4fe1[0x18900 - 0x4fe1];
    CTickItem m_18900;   // 0x18900
    unsigned char _18910[0x1a2d0 - 0x18910];
    CFlashFXVector m_flashfx;   // 0x1a2d0
};

/* the entity's node as the tick hides it: its active bit, SetActive the 13th virtual (CNode's,
 * its vtable pointer at 0x60) */
class CTickNode {
public:
    unsigned char _0[0x5c];
    unsigned int m_active : 1;   // 0x5c
    unsigned int _5c : 31;
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual bool SetActive(bool on);
};

/* SOCOM II's voice recognition (unnamed: 0x414680; main sets it up, the tick ticks it, and bit 1
 * at 0x38 is SOCOM 1's recoIsRecognizing) */
struct CVoice {
    unsigned char _0[0x38];
    unsigned char _b0 : 1;
    unsigned char m_recognizing : 1;   // 0x38
    unsigned char _b2 : 6;
};

class CameraZoom {
public:
    static bool Tick(float dT);
};

extern unsigned char OurGameName;

extern "C" {
extern CVoice data_00414680;                      /* the voice (0x414680) */
extern int data_00408C08;                        /* SOCOM 1's oddeven, a static of Tick's */
extern char data_00488F10[];
extern char data_004B5170[];
void func_0023AD90(CVoice *voice, float dT);
bool func_001FA300(CHUD *hud);                   /* CHUD::PauseGame */
void func_0034E070(CSched_Manager *sched, float dT);   /* CSched_Manager::Tick */
void func_00336230(void *arg);
void func_003C3390(void *arg, float dT);
void func_003B1160(int arg);
CTickNode *func_0029ED20(CZSealBody *entity);    /* CEntity::GetNode */
int func_00339DE0(CPipe *pipe, zdb::CWorld *world);   /* CPipe::RenderWorld */
void func_0033CD50(void *arg);
void func_0033B370(CPipe *pipe);
void func_0033B5B0(CPipe *pipe);
void func_0033B570(CPipe *pipe);
void func_0033B500(CPipe *pipe);
void func_0033BF30(CPipe *pipe, void *texbuffer);   /* CPipe::FIFO_Signal (SOCOM 1's second argument gone) */
void func_0033BE70(CPipe *pipe, void *texbuffer);   /* CPipe::FIFO_Signal */
CTickItem *func_0022D590();
void func_003AFD50(int r, int g, int b, int a, int z);   /* CVideo::Clear */
void func_002D9E40(float dT);                    /* CInput::Tick */
void func_003AFF30(bool wait, bool swap);        /* zVid_Swap */
}

static void LoadThreadEntry(void *arg);

/* MSL 3.0's basic_string members that close the unit in its template tail (OVL1 0x1EEB40,
 * 0x1EEB70, 0x1EF390, 0x1EF400; the template replace taking two iterators and their category,
 * 0x1EE780, between them is still a placeholder): out-of-class template members in the original,
 * written here as stand-ins under their placeholders over a copy of include/string's
 * representation -- fts_state.h keeps its own std::basic_string, so <string> cannot come in.
 * Defined first, so emitted last; pinned, or include/string's inline assign/append would take
 * them in (the same-unit stand-in rule) */
namespace {
/* include/string's twelve bytes: the long form's capacity | 1, size and text, the short form's
 * size in the first byte's upper seven bits */
struct CTailString {
    bool is_long() const { return w_[0] & 1; }
    unsigned int size() const { return is_long() ? w_[1] : (unsigned char)s_.size_; }

    struct short_rep {
        unsigned char is_long_ : 1;
        unsigned char size_ : 7;
        char data_[11];
    };
    union {
        unsigned int w_[3];
        short_rep s_;
    };
};
}

extern "C" void *func_001EF390(void *str, unsigned int pos, unsigned int n1, const char *s);

#pragma auto_inline off
extern "C" {
/* assign(first, last): the whole text replaced by the range (0x1EF400) */
void *func_001EF400(void *str, const char *first, const char *last)
{
    std::forward_iterator_tag category;

    return std::func_001EE780(str, 0, ((CTailString *)str)->size(), first, last, category);
}

/* replace(pos, n1, const char *): the range s..s+strlen(s) handed to the template replace
 * (0x1EF390) */
void *func_001EF390(void *str, unsigned int pos, unsigned int n1, const char *s)
{
    std::forward_iterator_tag category;

    return std::func_001EE780(str, pos, n1, s, s + strlen(s), category);
}

/* append(const char *): replace(size(), 0, s), a tail call (0x1EEB70) */
void *func_001EEB70(void *str, const char *s)
{
    return func_001EF390(str, ((CTailString *)str)->size(), 0, s);
}

/* assign(const char *): replace(0, size(), s), a tail call (0x1EEB40) */
void *func_001EEB40(void *str, const char *s)
{
    return func_001EF390(str, 0, ((CTailString *)str)->size(), s);
}
}
#pragma auto_inline reset

/* the load thread's stack (SOCOM 1's static array of it; SOCOM II's allocated once) */
static void *load_thread_stack;

/* the film's name let go of */
CCinematicState::~CCinematicState()
{
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
}

/* the film let go of: the film player, its items, its texture buffer and list, and -- if it was
 * up -- the GS memory it borrowed restored, the frame rate, the screen's mode or clear, path 3;
 * the film's name, the fade, the player's controller told */
void CCinematicState::UnInit()
{
    func_00309A10();
    func_003B7150(0);
    for (int i = 0; i < 3; i++) {
        if (m_items[i]) {
            delete m_items[i];
            m_items[i] = 0;
        }
    }
    func_00356B30(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[54]]);
    func_001ED000(data_00408BD0);
    func_001ECF60(data_00408BD0);
    if (m_2a) {
        zSysFifoEnd();
        zSysFifoDisable();
        func_003B1990(0x2400, func_00288930(), 0x1c0000);
        func_003B17D0(m_savedFrameRate);
        if (m_29) {
            if (zVid.m_75 == 2) {
                func_003B0B90(0, 0, zVid.m_44, zVid.m_48);
                func_003B09E0(0, 0, zVid.m_44, zVid.m_48);
            } else {
                if (m_1c != zVid.m_48) {
                    zVid.m_30 = 0;
                    zVid.m_34 = m_videoSave[0] > m_videoSave[1] ? m_videoSave[0] : m_videoSave[1];
                    zVid.m_48 = m_1c;
                }
                zVid.m_90 = 1;
            }
        } else {
            zVid.m_92 = m_20 != 0;
        }
        zVid.m_7c = 0;
        *(volatile unsigned int *)0x10003010 |= 4;
        asm volatile {
            sync
        }
    }
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
    func_003B1760(0, 0, 0, 1.0f);
    CZSealBody *player = func_002B3580();
    void *control = player ? player->m_control : 0;

    if (control)
        func_00592CA0(control);
    func_003466F0();
}

/* a message for the player: the sender's name and the text built into a string (and not used),
 * the text queued on the mission's first message queue with the message's type (OVL1 0x1ECC40,
 * slot 0x2C) */
void CCoreState::DisplayMessage(const char *text, const char *who, int a, int b, int c, unsigned char d,
                                unsigned char e, char type)
{
    CSaferStr s;

    s = who;
    s += ": ";
    s += text;
    func_002B6530(&theMission.m_messages, text, type, 0, 0, 0.0f);
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
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[36]], true);
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[40]], true);
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

/* the asset manager's handle for a texture by name (out of line: every caller calls it) */
#pragma auto_inline off
zdb::CTexHandle *zdb::CWorld::GetTexHandle(const char *name)
{
    return CAssetMgr::m_assets.GetTexHandle(name);
}

#pragma auto_inline reset

/* the mission set up: the world and its camera, the HUD, the player under the camera, the
 * reader archives, the tick tasks, the camera's far plane, room for 300 more nodes, the texture
 * buffers, the mission started, the pipe's noise texture, the GIF's path 3 opened, the frame rate
 * back to the default */
bool CCoreState::Init()
{
    m_world = zdb::CWorld::m_world;
    zdb::CVisual::m_camera = thePipe.m_camera = m_world->m_camera;
    CHUD *hud = (CHUD *)operator new(0x1acd0);

    if (hud)
        hud = func_001FCD80(hud);
    theHUD = m_hud = hud;
    if (gopt[0]) {
        CHUD &h = *m_hud;

        func_001FA360(&h, m_world);
        func_001F1840(&h);
    }
    func_001F1870(func_002B3580());
    func_0032C4F0();
    func_0032C2B0("readerm.zar", GetDatabase());
    zTaskScheduler.AddTask("UnitTick", func_005E7FD0, 0.8f, 0);
    zTaskScheduler.AddTask("ai_pre_tick", func_005CCCD0, 0.9f, 0);
    zTaskScheduler.AddTask("entity_pre_tick", CEntity::PreTickAll, 1.0f, 0);
    zTaskScheduler.AddTask("weapon_pre_tick", func_003D1F90, 3.0f, 0);
    zTaskScheduler.AddTask("CAiEventTick", func_0050ECD0, 4.5f, 0);
    zTaskScheduler.AddTask("entity_tick", func_0029F430, 4.6f, 0);
    zTaskScheduler.AddTask("body_collide_tick", func_0054F570, 4.65f, 0);
    zTaskScheduler.AddTask("entity_preposttick", func_0029F350, 4.7f, 0);
    zTaskScheduler.AddTask("target_pretick", func_0060BC40, 4.8f, 0);
    zTaskScheduler.AddTask("entity_post_tick", func_0029F1A0, 6.0f, 0);
    zTaskScheduler.AddTask("weapon_post_tick", func_003D1F50, 8.0f, 0);
    zTaskScheduler.AddTask("ai_route_planner", func_00510570, 10.5f, 0);
    zTaskScheduler.AddTask("target_post_tick", func_0060B8F0, 10.6f, 0);
    zTaskScheduler.AddTask("ai_posttick", func_005CCA30, 10.7f, 0);
    zTaskScheduler.AddTask("sound_post_tick", func_00341A60, 11.0f, 0);
    zTaskScheduler.AddTask("appCamera_tick", func_00299C10, 12.0f, 0);
    appCamera->m_camera->GetLandmarkFarPlane(m_world);
    std::vector<zdb::CNode *> &nodes = m_world->m_nodes;
    unsigned int n = nodes.size() + 300;

    if (nodes.capacity() < n)
        nodes.reserve(n);
    func_00356B90(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[33]], 2048);
    func_00356B90(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[34]], 2048);
    func_0031CED0(m_world);
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[36]], true);
    func_00356BE0(zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[40]], true);
    func_002A7D40(&theMission);
    theMission.m_e8 = -1.0f;
    func_0033B420(&thePipe, zdb::CWorld::GetTexHandle("noise50.tif"));
    *(volatile unsigned int *)0x10003010 |= 4;
    zSysFifoEnd();
    zVid.m_91 = 0;
    func_003B17D0(CVideo::m_defaultFrameRate);
    zVid_Sync0();
    return true;
}

/* a frame of the mission. While the mission is not yet running (its m_18 at 1, its flag at 0xFC,
 * or online) the fade-in waits; otherwise it counts down and, done, starts the mission. Input;
 * while running the voice, the HUD's lens flare, the tasks (unless paused) and -- for SOCOM 2 --
 * the zoom; the frame swapped (or not, while swaps are skipped); the camera updated; the world
 * drawn with the camera's own entity hidden in first person or while its controller holds the
 * zoom; the HUD drawn between texture loads (with the option), its compass, flashes and items.
 * SOCOM 1's Tick without its benchmark timers (line order kept) */
bool CCoreState::Tick(float dT)
{
    bool running;
    bool hide;
    CTickNode *pEntNode;
    bool camera_entity_active;
    zdb::CCamera *cam;

    cam = thePipe.m_camera;
    running = true;
    if (theMission.m_18 != 1 && !(theMission.m_fc & 1) && !theNetwork.m_9) {
        m_c8 -= dT;
        float fade = m_c8 * m_c4;

        if (fade < 0.0f) {
            fade = 1.0f;
            theMission.EndMission();
        }
        func_003B1760(0, 0, 0, fade);
        running = false;
    } else {
        m_c8 = m_c0;
    }
    func_002D9E40(dT);
    if (running) {
        func_0023AD90(&data_00414680, dT);
        m_hud->PreDITick(dT);
        if (!data_00414680.m_recognizing && !func_001FA300(m_hud))
            func_0034E070(&zTaskScheduler, dT);
        if (OurGameName == 1)
            CameraZoom::Tick(dT);
    }
    zSysFifoEnd();
    if (m_skip_swap > 0) {
        m_skip_swap--;
        func_003AFF30(false, true);
        zVid.m_90 = 0;
    } else {
        func_003AFF30(true, true);
    }
    func_00336230(data_00488F10);
    cam->Update(zdb::ZCAM_TYPE_0);
    m_hud->PostDITick(dT);
    if (running && !func_001FA300(m_hud))
        func_003C3390(data_004B5170, dT);
    func_003B1160(1);
    hide = appCamera->m_camera_mode == 3;
    pEntNode = func_0029ED20(appCamera->m_bc);
    CZSealBody *player = func_002B3580();
    void *control = player ? player->m_control : 0;

    if (!hide && control && ((unsigned char *)control)[0x221] == 5)
        hide = true;
    if (hide) {
        camera_entity_active = pEntNode->m_active;
        pEntNode->SetActive(false);
    }
    data_00408C08 = func_00339DE0(&thePipe, m_world);
    if (hide)
        pEntNode->SetActive(camera_entity_active);
    if (running && !func_001FA300(m_hud)) {
        func_0033CD50(thePipe.m_688);
        func_0033B370(&thePipe);
    }
    func_0033B5B0(&thePipe);
    func_0033B570(&thePipe);
    func_0033B500(&thePipe);
    if (gopt[0]) {
        if (running)
            m_hud->Tick(dT);
        func_003B7150(1);
        thePipe.m_assetHUD2 =
            zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[39 + data_00408C08]];
        func_0033BF30(&thePipe, thePipe.m_assetHUD2);
        func_0033BE70(&thePipe, thePipe.m_assetHUD1);
        m_hud->DrawPhase(zdb::RENDER_PHASE_12, cam);
        thePipe.m_assetHUDX = zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[42]];
        func_0033BF30(&thePipe, thePipe.m_assetHUDX);
        func_0033BE70(&thePipe, thePipe.m_assetHUD2);
        m_hud->DrawPhase(zdb::RENDER_PHASE_13, cam);
        func_0033BF30(&thePipe, zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[0]]);
        func_0033BE70(&thePipe, thePipe.m_assetHUDX);
        m_hud->DrawPhase(zdb::RENDER_PHASE_14, cam);
        m_hud->m_compass.Draw(cam);
        if (!data_00414680.m_recognizing && !func_001FA300(m_hud))
            m_hud->m_flashfx.Tick(dT);
        m_hud->m_flashfx.Draw(cam);
        m_hud->m_18900.Draw(cam);
        func_0022D590()->Draw(cam);
        func_003B7150(0);
    } else {
        func_0033BF30(&thePipe, zdb::CTexManager::m_texmanager.data_[zdb::CTexManager::TexPhase[0]]);
    }
    if (!theNetwork.m_9 && theGame.m_6c)
        func_003AFD50(1, 0, 0, 0, 0);
    return false;
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
    zVid.m_91 = 1;
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
    func_00356130(&zdb::CTexManager::m_texmanager);
    if (!func_001EA720())
        func_00192BF8(0);
    if (theNetwork.m_9)
        func_00223290(&theGame.m_MPexitState);
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

    if (!load_thread_stack)
        load_thread_stack = _memalign(16, 0x11000, 0, 0);
    memset(load_thread_stack, 250, 2048);
    memset((char *)load_thread_stack + 0x10800, 250, 2048);
    memset((char *)load_thread_stack + 2048, 234, 0x10000);
    param.entry = LoadThreadEntry;
    param.option = 0;
    param.stackSize = 0x10000;
    param.initPriority = 6;
    param.gpReg = _gp;
    param.stack = (char *)load_thread_stack + 2048;
    m_LoadThreadId = CreateThread(&param);
    if (m_LoadThreadId)
        StartThread(m_LoadThreadId, this);
}

#pragma auto_inline reset

/* the load thread's stack let go of (the name ours) */
void CCoreState::FreeLoadThread()
{
    m_LoadThreadId = -1;
    if (load_thread_stack)
        _free(load_thread_stack, 0, 0);
    load_thread_stack = 0;
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
    func_00356130(&zdb::CTexManager::m_texmanager);
    if (!func_001EA720())
        func_00192BF8(0);
    if (theNetwork.m_9)
        func_00223290(&theGame.m_MPexitState);
    m_loading = false;
}

void CCoreState::Unload()
{
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
}

/* the mission let go of: an online game's network, the tick tasks, the projectiles and the marks,
 * the loaded data (exiting if that fails), the world's texture buffers, the HUD, the player, the
 * camera's seal, the weapons' lists, the mission's reader archive, the scheduler's free list */
void CCoreState::auxUnload()
{
    if (theNetwork.m_9 && theMission.m_18 == 4) {
        ftsNetworkGameUninit();
        DMEDisconnectFromGameServer();
    }
    zTaskScheduler.RemoveTask("CAiEventTick", false);
    zTaskScheduler.RemoveTask("UnitTick", false);
    zTaskScheduler.RemoveTask("ai_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_posttick", false);
    zTaskScheduler.RemoveTask("entity_pre_tick", false);
    zTaskScheduler.RemoveTask("target_pretick", false);
    zTaskScheduler.RemoveTask("target_post_tick", false);
    zTaskScheduler.RemoveTask("weapon_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_post_tick", false);
    zTaskScheduler.RemoveTask("ai_direg_tick", false);
    zTaskScheduler.RemoveTask("entity_post_tick", false);
    zTaskScheduler.RemoveTask("entity_tick", false);
    zTaskScheduler.RemoveTask("body_collide_tick", false);
    zTaskScheduler.RemoveTask("entity_preposttick", false);
    zTaskScheduler.RemoveTask("weapon_post_tick", false);
    zTaskScheduler.RemoveTask("ai_route_planner", false);
    zTaskScheduler.RemoveTask("sound_post_tick", false);
    zTaskScheduler.RemoveTask("light_tick", false);
    zTaskScheduler.RemoveTask("appCamera_tick", false);
    CZProjectileList *proj = CZWeapon::m_pProjectilelist;
    CZMarkList *marks = CZWeapon::m_pMarklist;

    if (proj)
        proj->UnInit();
    if (!func_001EA6E0())
        func_00192BF8(0);
    if (marks)
        marks->UnInit();
    m_world = 0;
    func_00356080(&zdb::CTexManager::m_texmanager, false);
    func_001F1840(0);
    func_001FCB70(&m_hud);
    ftsSetPlayer(0);
    func_001F1870(0);
    func_003D3930();
    func_003D37E0();
    func_0032C2B0("readerm.zar", 0);
    zTaskScheduler.ClearFreeList(true);
}
