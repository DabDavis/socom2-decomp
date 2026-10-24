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
class CWorld;
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
class CGameMenu {
public:
    unsigned char _0[0x910];

    CGameMenu();
    virtual ~CGameMenu();
    void Draw2dOnly();

    unsigned char _914[0x920 - 0x914];
};

class CExitMenu : public CGameMenu {
public:
    CExitMenu() {}
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
    virtual void Slot2C() {}

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
    CWorld *m_world;   // 0xa0
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

/* the game: its state stack, then its states (SOCOM 1's CGame is 0x7C bytes; SOCOM II's states
 * start 0x10 further on) */
class COurGame {
public:
    unsigned char _0[0x110];
    CCoreState m_coreState;   // 0x110
};

/* theGame is at 0x4084C0 (what 43 references take as the object; data.txt's row says 0x4084CC) */
extern "C" COurGame data_004084C0;
#define theGame data_004084C0

#endif
