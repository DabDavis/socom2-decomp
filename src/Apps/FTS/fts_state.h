/* The game's states (SOCOM 1's Apps/FTS/st_*.cpp) and what they reach for. CGameState is SOCOM 1's
 * gamez/zfts/fts_state.h (its DWARF puts CCoreState's inline PreInit there too); kept here, with
 * the engine pieces the states touch, until the engine's own headers carry them. Layouts are
 * SOCOM 1's DWARF moved to where SOCOM II's code reads them. */
#ifndef FTS_STATE_H
#define FTS_STATE_H

#include "../../gamez/zmath/zmath.h"

extern "C" {
void *memset(void *p, int c, unsigned int n);

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
int sceGsSyncPath(int mode, unsigned short timeout);

/* unnamed yet: the linker's _gp, 0x1DD2F0 */
extern char data_001DD2F0[];
}

void zSysFifoEnd();
void zSysFifoDisable();
void zVid_Sync0();

/* zVid as the states see it: SOCOM 1's fields at 0x30, 0x34 and 0x58 (the frame's clear and the
 * film's restore); SOCOM II's struct runs past SOCOM 1's 0x80 bytes to the flags at 0x90-0x92.
 * The field names are ours */
struct CZVidState {
    unsigned char _0[0x30];
    int m_30;   // 0x30
    int m_34;   // 0x34
    unsigned char _38[0x44 - 0x38];
    int m_44;   // 0x44
    int m_48;   // 0x48
    unsigned char _4c[0x75 - 0x4c];
    unsigned char m_75;   // 0x75
    unsigned char _76[0x7c - 0x76];
    unsigned char m_7c;   // 0x7c
    unsigned char m_7d;   // 0x7d
    unsigned char _7e[0x90 - 0x7e];
    unsigned char m_90;   // 0x90
    unsigned char m_91;   // 0x91: set while a state's screen is up
    unsigned char m_92;   // 0x92
};

extern CZVidState zVid;
void zsys_FullAllocAndFree();
void *_memalign(unsigned int align, unsigned int size, const char *file, int line);

/* a pad: only the actuator bytes the states clear (SOCOM 1's m_ActData at 0xC; SOCOM II's 0xB4) */
class CPad {
public:
    unsigned char _0[0xb4];
    unsigned char m_ActData[6];   // 0xb4
};

class CInput {
public:
    static CPad *m_pads[2];
};

class CHUD;
class CZAnimZAR;

/* the anims (gamez/zanim's; only the archive calls) */
class CZAnimMain {
public:
    int CloseZAR(CZAnimZAR *zar);
    CZAnimZAR *OpenZAR(const char *name);
};

extern CZAnimMain ZAnim;

/* a menu screen: SOCOM 1's CGameMenu is 0x890 bytes with its vtable pointer at 0x884, after
 * every member; SOCOM II's at 0x910 (CExitState's delete, CExitMenu's constructor) */
class CTextureRelocMgr;

#include <vector>

namespace zdb {
class CNode;
class CTexHandle;
class CAssetLib;
class CWorld;

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static void ReleaseLibRef(CAssetLib *lib);

    static CAssetList m_assets;
};

enum tag_ZCAM_TYPE {
    ZCAM_TYPE_0
};

class CCamera {
public:
    void Update(tag_ZCAM_TYPE type);
    float GetLandmarkFarPlane(CWorld *world);
};

/* the world: only what the states touch -- its nodes' vector at 0x68 (reserved ahead by the
 * core state), its camera at 0x128 (SOCOM 1's at 0xC0) */
class CWorld {
public:
    static CTexHandle *GetTexHandle(const char *name);

    unsigned char _0[0x68];
    std::vector<CNode *> m_nodes;   // 0x68
    unsigned char _74[0x128 - 0x74];
    CCamera *m_camera;   // 0x128

    static CWorld *m_world;
};
}

/* the mission's texture buffers: a vector of them (zdb::CTexManager::m_texmanager's) */
struct TexBufferVector {
    unsigned int capacity_;
    unsigned int size_;
    void **data_;
};

/* the render pipe (SOCOM 1's CPipe: the camera at 0x628, the UI node it draws at 0x630, a flag at
 * 0x63D -- SOCOM II's at 0x628, 0x630 and 0x711) */
class CPipe {
public:
    void RenderUiNode(zdb::CNode *node);

    unsigned char _0[0x628];
    zdb::CCamera *m_camera;   // 0x628
    unsigned char _62c[0x630 - 0x62c];
    zdb::CNode *m_uiNode;   // 0x630
    unsigned char _634[0x711 - 0x634];
    unsigned char m_renderingUi;   // 0x711
};

extern "C" {
extern TexBufferVector data_0049E880;            /* zdb::CTexManager::m_texmanager */
extern CPipe data_004887C0;                      /* thePipe */
}

class CGameMenu {
public:
    unsigned char _0[0xc4];
    zdb::CNode *m_pBackplaneNode;   // 0xc4 (SOCOM 1's DWARF)
    unsigned char _c8[0x910 - 0xc8];

    CGameMenu();
    virtual ~CGameMenu();
    virtual void MyTick(float dT);
    virtual const char *GetReaderFileName();
    virtual int Slot14();   /* SOCOM II's, before PrepControls (the name ours) */
    virtual void PrepControls(CTextureRelocMgr *reloc);
    void Draw2dOnly();
    void DrawForeplane();

    unsigned char _914[0x920 - 0x914];
};

class CExitMenu : public CGameMenu {
public:
    CExitMenu() {}
    virtual const char *GetReaderFileName();
};

/* a state of the game's stack: named, and told when it is pushed, popped, set up and torn down.
 * SOCOM II added two virtuals after OnPop (the second takes a dialog's arguments) */
class CGameState {
public:
    CGameState(const char *name)
    {
        m_name = 0;
        SetName(name);
    }
    virtual ~CGameState() { SetName(0); }
    virtual void PreInit() {}
    virtual bool Init() { return true; }
    virtual bool Tick(float dT) { return false; }
    virtual void PreUnInit() {}
    virtual void UnInit() {}
    virtual void OnPush(int userdata) {}
    virtual void OnPop(int userdata) {}
    virtual void SetSkipSwap(int swap) {}
    /* SOCOM II's: a message for the player (the menu state's tick passes the network's queued
     * ones: the text, no sender, the flag 1, the message's type last). The name and the middle
     * parameters' meaning are ours */
    virtual void DisplayMessage(const char *text, const char *who, int a, int b, int c,
                        unsigned char d, unsigned char e, char type) {}

    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = name ? _strdup(name, 0, 0) : 0;
    }

    char *m_name;   // 0x4
};

/* the mission itself: the world, the HUD, the load thread */
class CCoreState : public CGameState {
public:
    CCoreState();
    virtual ~CCoreState();
    virtual void PreInit() {}
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void PreUnInit() {}
    virtual void UnInit();
    virtual void OnPush(int userdata);
    virtual void OnPop(int userdata);
    virtual void SetSkipSwap(int swap) { m_skip_swap = swap; }

    /* whether the mission is done loading (the load state's tests materialise the bool: the
     * name ours) */
    bool IsLoaded() { return !m_loading; }
    void Load(bool async);
    void Unload();
    void auxUnload();
    void StartLoadThread();
    void FreeLoadThread();

    float m_agl;   // 0x8
    float m_posScale;   // 0xc
    unsigned char m_showingTB;   // 0x10
    CMatrix objMtx1;   // 0x20
    CMatrix objMtx2;   // 0x60
    zdb::CWorld *m_world;   // 0xa0
    unsigned int skyColor[3];   // 0xa4
    int m_LoadThreadId;   // 0xb0
    unsigned char m_loading;   // 0xb4
    unsigned char m_loadstarted;   // 0xb5
    CHUD *m_hud;   // 0xb8
    int m_skip_swap;   // 0xbc
    float m_c0;   // 0xc0 (SOCOM II's three: one, its reciprocal, it again)
    float m_c4;   // 0xc4
    float m_c8;   // 0xc8
};

class CLoadMenu : public CGameMenu {
public:
    CLoadMenu() {}
    virtual const char *GetReaderFileName();
};

/* the mission's load: its screen while the core state loads (SOCOM 1's DWARF, without the E3
 * build's config; SOCOM II's is 0x24 bytes, its abort flag at 0x1A) */
class CLoadState : public CGameState {
public:
    CLoadState();
    virtual ~CLoadState();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void UnInit();

    void AbortLoad(const char *dlgname);

    CLoadMenu *m_menu;   // 0x8
    unsigned char m_fadeup;   // 0xc
    unsigned char m_fadedown;   // 0xd
    float m_time_elapsed;   // 0x10
    int m_14;   // 0x14
    unsigned char m_18;   // 0x18
    unsigned char m_19;   // 0x19
    unsigned char m_abort;   // 0x1a
    int m_1c;   // 0x1c
    int m_20;   // 0x20
};

/* MSL's basic_string as the states hold it: three words, all zero when made (SOCOM II's library
 * makes it inline). A stand-in until a string model exists: the calls on it are the map's
 * (assign is func_001EEB40) */
struct CStateString {
    CStateString()
    {
        w[0] = 0;
        w[1] = 0;
        w[2] = 0;
    }
    /* the short form: its text in place after the first byte (bit 0 of the first word clear) */
    bool IsShort() const { return !(w[0] & 1); }
    const char *c_str() const { return IsShort() ? (const char *)this + 1 : (const char *)w[2]; }

    unsigned int w[3];
};

/* the end of a mission: its stats shown on the splash screen's menu. SOCOM II's strings are
 * 0xC bytes (two at 8 and 0x14), so the menu is at 0x24 */
class CExitState : public CGameState {
public:
    CExitState();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void UnInit();

    CStateString m_libname;   // 0x8
    CStateString m_texname;   // 0x14
    void *m_plib;   // 0x20
    CExitMenu *m_menu;   // 0x24
};

/* the online stats screen's menu: whether the game is over (SOCOM 1's m_gameover at 0x890, after
 * CGameMenu; SOCOM II's at 0x920) */
class CMPExitMenu : public CGameMenu {
public:
    CMPExitMenu() {}
    virtual const char *GetReaderFileName();

    unsigned char m_gameover;   // 0x920
};

/* the end of an online game: its stats screen (SOCOM 1's DWARF; SOCOM II's strings are 0xC bytes,
 * the menu at 0x24, a word more at 0x28) */
class CMPExitState : public CGameState {
public:
    CMPExitState();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void UnInit();

    CStateString m_libname;   // 0x8
    CStateString m_texname;   // 0x14
    void *m_plib;   // 0x20
    CMPExitMenu *m_menu;   // 0x24
    int m_28;   // 0x28
};

class CMenuStateMenu : public CGameMenu {
public:
    CMenuStateMenu() {}
    virtual const char *GetReaderFileName();
};

/* the front end's menus (SOCOM 1's DWARF; SOCOM II's two sound names are 0xC-byte strings, so the
 * selection sound is at 0x2C, the default menu at 0x30, the reboot handle at 0x70) */
class CMenuState : public CGameState {
public:
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void UnInit();
    virtual void OnPush(int userdata);
    virtual void OnPop(int userdata);

    bool RunEventAnim(const char *eventname, const char *argument);

    CMenuStateMenu *m_menu;   // 0x8
    int m_skip;   // 0xc
    int m_skip_swap;   // 0x10
    CStateString TtySoundString;   // 0x14
    CStateString BackSoundString;   // 0x20
    void *SelectionSound;   // 0x2c
    char m_DefaultMenu[64];   // 0x30
    int m_iopregh;   // 0x70
};

/* a film between missions (SOCOM 1's DWARF: the movie's name at 0xC) */
/* something the film screen draws (its vtable pointer at 0xC: a 2D element) */
class CCinematicItem {
public:
    unsigned char _0[0xc];
    virtual ~CCinematicItem();
};

class CCinematicState : public CGameState {
public:
    virtual ~CCinematicState();
    virtual bool Init();
    virtual bool Tick(float dT);
    virtual void UnInit();

    unsigned char m_movierunning;   // 0x8
    char *m_moviename;   // 0xc
    int m_savedFrameRate;   // 0x10
    int m_videoSave[2];   // 0x14 (SOCOM 1's)
    int m_1c;   // 0x1c
    int m_20;   // 0x20
    unsigned char _24[0x29 - 0x24];
    unsigned char m_29;   // 0x29
    unsigned char m_2a;   // 0x2a: the film is up
    unsigned char _2b[0x30 - 0x2b];
    CCinematicItem *m_items[3];   // 0x30
};

/* the game: its state stack, then its states -- SOCOM 1's at 0x7C, 0xE0, 0x100, 0x1C0, 0x1D8,
 * 0x1F0; SOCOM II's menu state at 0x78 (its m_skip at 0x84), load state at 0xEC, core state at
 * 0x110, exit state at 0x1E0, multiplayer exit state at 0x208 */
class COurGame {
public:
    unsigned char _0[0x78];
    CMenuState m_menuState;   // 0x78
    CLoadState m_loadState;   // 0xec
    CCoreState m_coreState;   // 0x110
    CExitState m_exitState;   // 0x1e0
    CMPExitState m_MPexitState;   // 0x208
};

/* the game's state stack (SOCOM 1's CGame; unnamed yet) */
extern "C" {
void func_002CF2F0(COurGame *game, unsigned int arg);                      /* CGame::Pop */
void func_002CF410(COurGame *game, void *state, unsigned int arg);         /* CGame::Switch */
}

/* the mission: only what the states touch */
class CMissionInfo {
public:
    unsigned int _0;
    unsigned short m_4;   // 0x4: 2 when the mission ends the game
};

class CMission {
public:
    static void _RecoverFromNetError(int error);
    void ReOpen_Load();
    void ReOpen_Unload1();
    void ReOpen_Unload2();

    unsigned char _0[0x18];
    unsigned char m_18;   // 0x18: 4 in an online game (SOCOM 1's too)
    unsigned char _19[0x1c - 0x19];
    const char *m_1c;   // 0x1c: the mission's database name
    unsigned char _20[0xe8 - 0x20];
    float m_e8;   // 0xe8 (-1 when the mission starts; SOCOM 1's at 0x68)
    unsigned char _ec[0x108 - 0xec];
    float m_108;   // 0x108
    unsigned char _10c[0x1ac - 0x10c];
    CMissionInfo *m_1ac;   // 0x1ac
    unsigned char _1b0[0x1854 - 0x1b0];
    unsigned char m_objectives[0xc];   // 0x1854: a list of the mission's objectives
};

extern CMission theMission;
extern COurGame theGame;

#endif
