/* game_engine.cpp -- the game's engine and play started and stopped: CGame's StopEngine,
 * StartEngine, StopPlay and StartPlay (SOCOM 1's game_engine.cpp, OVL1 0x2D0330-0x2D04EF, after
 * game_state.cpp's tail and before graph_main.cpp). Its initialiser __sinit_game_engine.cpp
 * 0x401C10 is <iostream>'s pair alone, between game_state.cpp's and graph_tree.cpp's -- SOCOM 1's
 * built the header CSTable SOCOM II dropped as well. SOCOM II's engine no longer starts or stops
 * the video (zVid_Init, zSysReset and zVid_Open are gone from the start) and keeps a second set
 * of SEAL anims' set-up in OVL2 (unnamed). Defined in reverse of their image order (-inline
 * deferred). */

#include "zgame.h"

#include <iostream>

/* the systems the engine starts and stops (their units'; only what is called here) */
namespace zdb {
class CWorld {
public:
    static void Init();   /* 0x320770 */
    static void Uninit();   /* 0x320760 */
};

class CTexture {
public:
    static void Init();   /* 0x354F40 */
};

class CRipple {
public:
    static void Init();   /* 0x3221F0 */
    static void Uninit();   /* 0x3221C0 */
};

/* the texture effects' manager (vis_texfx.h's: only its set-up and teardown) */
class CTextureFXManager {
public:
    void Init(int n);   /* 0x3C0740 */
    void Uninit();   /* 0x3C0620 */
};

extern CTextureFXManager theTextureFXManager;
}

class CZAnimMain {
public:
    void Close();   /* 0x26B120 */
};

extern CZAnimMain ZAnim;

/* the motion script's scheduler (zanim.h's CSched_Manager: only its clear) */
class CSched_Manager {
public:
    void Clear();   /* 0x34E480 */
};

extern CSched_Manager zTaskScheduler;

class AnimSet {
public:
    static void CloseAll();   /* OVL2 0x5E5070 */
};

class CValve {
public:
    static void Init();   /* 0x351EE0 */
    static void Uninit();   /* 0x351E60 */
};

class CSnd {
public:
    static void Uninit();   /* 0x345200 */
};

class CSndInstance {
public:
    static void InitInstancePool(int n);   /* 0x346380 */
};

class Headset {
public:
    static void Reset();   /* 0x34B8C0 (SOCOM 1's ftsDeleteHeadset in its place) */
};

class CRdrArchive {
public:
    static void CloseAll();   /* 0x32C4F0 */
};

class CInput {
public:
    static void Init();   /* 0x2D9F20 */
    static void Uninit();   /* 0x2D9EA0 */
    static void CreatePad(unsigned int port);   /* 0x2D9D00 */
    static void DeletePad(unsigned int port);   /* 0x2D9CC0 */
};

class C2D {
public:
    static void Init();   /* 0x364F30 */
    static void Open();   /* 0x364EE0 */
};

class CPipe {
public:
    static void Init();   /* 0x33BCD0 */
    static void Uninit();   /* 0x33BCA0 */
};

class CAiStar {
public:
    static void Init();   /* OVL2 0x5128F0 */
};

void UninitSystemTuners();   /* 0x38A210 */
void zUISchedulerClear();   /* 0x36ABE0 */
void zRndrInit();   /* 0x335FD0 */
void zMath_Init();   /* 0x306670 */
void uninit_trig_table();   /* 0x309400 */
void zUtilUninit();   /* 0x39C6F0 */

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void func_00576120();   /* (OVL2: SOCOM 1 deleted its CSealAnim here) */
void func_005761B0();   /* (OVL2: and newed it here) */
void func_003B11F0();   /* zVid_Uninit */
}

/* play started: the game active, the first state switched to (OVL1 0x2D04D0) */
void CGame::StartPlay()
{
    m_Active = 1;
    Switch(m_InitialState, 0);
}

/* play stopped (OVL1 0x2D04C0) */
void CGame::StopPlay()
{
    m_Active = 0;
}

/* the engine's systems started, in SOCOM 1's order with the video's set-up gone; always true
 * (OVL1 0x2D0410) */
bool CGame::StartEngine()
{
    zTaskScheduler.Clear();
    zdb::CRipple::Init();
    zdb::theTextureFXManager.Init(20);
    CValve::Init();
    zdb::CTexture::Init();
    C2D::Init();
    zdb::CWorld::Init();
    C2D::Open();
    CInput::Init();
    zRndrInit();
    CSndInstance::InitInstancePool(48);
    CInput::CreatePad(0);
    zMath_Init();
    CPipe::Init();
    func_005761B0();
    CGameStateChangeCmd::CreatePool(16);
    CAiStar::Init();
    return true;
}

/* the engine's systems stopped, the anims closed twice (before and after the pipe), as SOCOM 1's
 * (OVL1 0x2D0330) */
void CGame::StopEngine()
{
    zdb::CWorld::Uninit();
    ZAnim.Close();
    UninitSystemTuners();
    zdb::theTextureFXManager.Uninit();
    zdb::CRipple::Uninit();
    CPipe::Uninit();
    ZAnim.Close();
    AnimSet::CloseAll();
    func_00576120();
    CValve::Uninit();
    CSnd::Uninit();
    Headset::Reset();
    zTaskScheduler.Clear();
    zUISchedulerClear();
    CRdrArchive::CloseAll();
    CInput::Uninit();
    CInput::DeletePad(0);
    CInput::DeletePad(1);
    func_003B11F0();
    zUtilUninit();
    CGameStateChangeCmd::DestroyPool();
    uninit_trig_table();
}
