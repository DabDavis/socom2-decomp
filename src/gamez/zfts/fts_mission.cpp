/* fts_mission.cpp -- the mission: CMission, the online game's state (CZNetGame), the ladders, the
 * HUD's ZAnim commands, the archive and the custom character types (SOCOM 1's fts_mission.cpp;
 * its static initialiser is SOCOM II's 0x400160). OVL1 0x2A3B90-0x2B218F, the template instances
 * closing it (the objective cameras' vector, the UI variables' vector) before fts_player.cpp's
 * CSpawners code. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include <iostream>
#include <vector>

/* the UI variables first: zui.h's _zrdr is the one the unit sees (zanim.h leaves its own out
 * after zui.h's), as the original's one zrdr.h served both (the states lane, round 22) */
#include "../zui/zui.h"
#include "../zanim/zanim.h"
#include "../zsav/zsav_lobby.h"
#include "../zsys/zsys.h"
#include "../../Apps/FTS/hud_letterbox.h"
#include "../../Apps/FTS/hud_missioncams.h"

/* the HUD: only what the commands touch (SOCOM 1's CHUD at SOCOM II's offsets; its On and Off
 * inline here, out of line in SOCOM 1) */
class CHUD {
public:
    void On() { m_ison = true; }
    void Off() { m_ison = false; }

    unsigned char _0[0x1a100];
    CLetterBox m_letterbox;   // 0x1a100 (SOCOM 1's at 0x14280)
    unsigned char _1a230[0x1a2d0 - 0x1a230];
    unsigned char m_flashes[0xc];   // 0x1a2d0: a CFlashFXVector
    CZHudMissionCams m_missioncams;   // 0x1a2dc (SOCOM 1's at 0x14590)
    unsigned char _1a698[0x1a6f5 - 0x1a698];
    unsigned char m_ison : 1;   // 0x1a6f5 (SOCOM 1's at 0x14955)
};

extern CHUD *theHUD;

/* CValve::SetValue, inlined in both builds (CValve, VALVE_STATE and MakeCallbacks are zanim.h's) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the online game (SOCOM 1's CZNetGame, at SOCOM II's offsets: only what this unit touches) */
class CZNetGame {
public:
    /* the states (SOCOM 1's DWARF) */
    enum MP_MINOR_GAME_STATE {
        MP_MINOR_UNKNOWN = 0,
        MP_MINOR_NOTREADY = 1,
        MP_MINOR_READY = 2
    };
    enum MP_MAJOR_GAME_STATE {
        MP_MAJOR_UNKNOWN = 0,
        MP_MAJOR_MENUS = 1,
        MP_MAJOR_LOADING = 2,
        MP_MAJOR_INPLAY = 3,
        MP_MAJOR_ROUNDOVER = 4,
        MP_MAJOR_NEWROUND = 5,
        MP_MAJOR_GAMEOVER = 6,
        MP_MAJOR_PLAY = 12,
        MP_MAJOR_GAMELOBBY = 13,
        MP_MAJOR_GHOST = 14
    };

    void Initialize();
    void UnInitialize();
    bool TenSecondLaunch(bool launch);
    unsigned int DecrementRoundClockSec(int sec);

    /* the round's clock in whole seconds (SOCOM 1's CNetClock accessors, zfts.h: `0.001f * ms`) */
    unsigned int GetMaxGameTimeSec() { return 0.001f * m_max_game_time; }
    void SetE3RoundCount(int count);
    MP_MAJOR_GAME_STATE MPMajorGameState(MP_MAJOR_GAME_STATE state);
    MP_MAJOR_GAME_STATE MyMajorGameState(MP_MAJOR_GAME_STATE state);
    MP_MINOR_GAME_STATE MyMinorGameState(MP_MINOR_GAME_STATE state);
    bool LateJoiner(bool late);
    void ResetScores();

    unsigned char _0[0xc];
    CValve *m_pE3RoundValve;   // 0xc
    int m_10;   // 0x10 (SOCOM 1's at 0xc)
    int m_14;   // 0x14 (SOCOM 1's at 0x10)
    int m_18;   // 0x18 (SOCOM 1's at 0x14)
    int m_1c;   // 0x1c (SOCOM 1's at 0x18)
    CValve *m_pMyMajorGameStateValve;   // 0x20 (SOCOM 1's at 0x1c)
    CValve *m_pMyMinorGameStateValve;   // 0x24 (SOCOM 1's at 0x20)
    int m_28;   // 0x28 (SOCOM 1's at 0x24)
    CValve *m_pLateJoinerValve;   // 0x2c (SOCOM 1's at 0x28)
    unsigned char _30[0x34 - 0x30];
    CValve *m_pLaunchSecondsValve;   // 0x34 (SOCOM 1's at 0x30)
    unsigned char _38[0x40 - 0x38];
    int m_40;   // 0x40 (SOCOM 1's at 0x3c)
    unsigned char _44[0x4c - 0x44];
    int m_4c;   // 0x4c (SOCOM 1's at 0x48)
    unsigned char _50[0x84 - 0x50];
    int m_84[5];   // 0x84 (SOCOM 1's at 0x50)
    unsigned char _98[0xbc - 0x98];
    int m_bc;   // 0xbc (SOCOM 1's at 0x80)
    int m_c0;   // 0xc0 (SOCOM 1's at 0x84)
    unsigned char _c4[0xc8 - 0xc4];
    int m_c8;   // 0xc8 (SOCOM 1's at 0x8c)
    bool m_late_joiner;   // 0xcc (SOCOM 1's at 0x94)
    unsigned char _cd;
    unsigned char m_ce;   // 0xce (SOCOM 1's at 0x96)
    unsigned char m_cf;   // 0xcf (SOCOM 1's at 0x97)
    unsigned char _d0[0xd4 - 0xd0];
    unsigned char m_d4;   // 0xd4 (SOCOM 1's at 0x9b)
    unsigned char m_d5;   // 0xd5 (SOCOM 1's at 0x9c)
    unsigned char m_d6;   // 0xd6 (SOCOM 1's at 0x9d)
    unsigned char _d7;
    bool m_ten_second_launch;   // 0xd8 (SOCOM 1's at 0x9f)
    unsigned char m_d9;   // 0xd9: SOCOM II's
    unsigned char _da[0xdc - 0xda];
    bool m_respawn;   // 0xdc: respawning on (with the valve "Respawn")
    unsigned char _dd[0xe0 - 0xdd];
    unsigned int m_max_game_time;   // 0xe0: the round's clock in milliseconds (SOCOM 1's m_clock.m_max_game_time, at 0xa0)
    int m_launch_time;   // 0xe4 (SOCOM 1's at 0xa8)
    unsigned char _e8[0xec - 0xe8];
    unsigned int m_round_clock;   // 0xec: the round's time left, in milliseconds (the name ours)
    unsigned char _f0[0xf4 - 0xf0];
    unsigned char m_launch_f4;   // 0xf4 (SOCOM 1's at 0xbc)
    unsigned char _f5[0xf8 - 0xf5];
    int m_f8;   // 0xf8 (SOCOM 1's at 0xc0)
    unsigned char _fc[0x100 - 0xfc];
    int m_100;   // 0x100 (SOCOM 1's at 0xc8)
    unsigned char _104[0x110 - 0x104];
    unsigned char m_110;   // 0x110 (SOCOM 1's at 0xd8)
    unsigned char m_game_type;   // 0x111 (SOCOM 1's m_game_type at 0xd9)
    unsigned char m_112;   // 0x112 (SOCOM 1's at 0xda)
    MP_MAJOR_GAME_STATE m_mp_major_game_state;   // 0x113 (SOCOM 1's at 0xdb)
    MP_MAJOR_GAME_STATE m_my_major_game_state;   // 0x114 (SOCOM 1's at 0xdc)
    MP_MINOR_GAME_STATE m_my_minor_game_state;   // 0x115 (SOCOM 1's at 0xdd)
    unsigned char _116[0x11c - 0x116];
    int m_e3_round_count;   // 0x11c
    int m_120;   // 0x120
    int m_124;   // 0x124
    int m_128;   // 0x128
    char m_12c[31];   // 0x12c: a character type's name, its last byte the terminator
};

/* the network (only its session-master flag and its clock; theNetwork at 0x45A0C0, fts_remote.cpp's
 * head says why) */
class CZNetwork {
public:
    int zNetGetTime();

    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1: a network game (SOCOM 1's at 0x9): no fade to the mission's end then
};

extern CZNetwork theNetwork;

/* a cell of an AI map (zentity.h's CAiMapLoc: only its word) */
class CAiMapLoc {
public:
    unsigned int m_word;
};

/* a compass anchor (zentity.h's CCompassAnchor: only what the nav points and Open set) */
class CCompassAnchor {
public:
    /* (zentity.h's: a lock holds it off) */
    void SetEnabled(bool on)
    {
        if (!m_locked)
            m_enabled = on;
    }

    unsigned int m_enabled : 1;   // 0x0
    unsigned int m_arrow_enabled : 1;
    unsigned int m_name_enabled : 1;
    unsigned int m_echo : 1;
    unsigned int m_type : 16;
    unsigned int m_locked : 1;
    unsigned int m_unk21 : 10;
    unsigned int m_unused : 1;
    CPnt3D m_posW;   // 0x4
    char *m_name;   // 0x10
    CAiMapLoc *m_aiMapLoc;   // 0x14
};

class CCompass {
public:
    static CCompassAnchor *CreateAnchor(const CPnt3D &pos, const char *name);
    void Open();   /* 0x2A2BF0 */

    unsigned char _0[0xc];
};

extern CCompass theCompass;   /* 0x436390 */

/* the AI maps' nav points: a cell and its name (SOCOM 1's, 24 bytes; the names ours) */
struct CAiNavPoint {
    CAiMapLoc m_loc;   // 0x0
    char m_name[20];   // 0x4
};

/* the AI's maps (OVL2's AiMaps, ai_maps.h's CAiMaps : list<CAiMap *>: only the list, the map set's
 * read, the cell's centre and the nav points) */
class CAiMap;
class CAiMaps : public std::list<CAiMap *> {
public:
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
    void ReadMapSet(const char *file, const char *path);   /* OVL2 0x52B0D0 */

    unsigned char _c[0x48 - 0xc];
    std::vector<CAiNavPoint> m_navpoints;   // 0x48
};

extern CAiMaps AiMaps;

/* an objective camera's compass anchor (SOCOM 1's DWARF, 8 bytes) */
class CMissionObjectiveCam;
class SObjectiveCam_Anchor {
public:
    SObjectiveCam_Anchor() {}

    CMissionObjectiveCam *m_objective;   // 0x0
    CCompassAnchor *m_anchor;   // 0x4
};

/* a mission's state (SOCOM 1's DWARF: a byte) */
enum MISSION_STATE {
    MISSION_UNLOADED = 0,
    MISSION_RUNNING = 1,
    MISSION_SUCCESS = 2,
    MISSION_FAILURE = 3,
    MISSION_ABORTED = 4,
    MISSION_TIMEDOUT = 5
};

class CRdrFile;
namespace zdb {
class CNode;
}

/* the mission (only what this unit touches, at SOCOM II's offsets) */
class CMission {
public:
    CMission();    /* 0x2ADBC0 */
    ~CMission();   /* 0x2AD940 */
    bool Open(const char *name, int load_ai);
    static bool tick(float dT, void *data);   /* 0x2AAB00 */
    void ZAnimLoad();   /* 0x2ABAD0 */
    void InitalizeTeamValves();   /* 0x2B2840 */
    void RefreshTeamValves();   /* 0x2B23A0 */
    void CreateCompassNavPoints();
    CZNetGame *CreateNetGame();
    bool ReOpen_Unload2();
    void FadeToEndMission(float time, int counter);
    bool FadeToEndMissionTick(float dT);
    static bool fadeToEndMissionTick(float dT, void *data);
    static void RecoverFromNetError(int error);
    bool _RecoverFromNetError(int error);
    void SetDelayEndMission(bool b) { m_DelayEndMission = b; }
    void SetFadingToEndMission(bool b) { m_FadingToEndMission = b; }
    unsigned int GetTimeLeft();
    void EndMission();
    void CreateLadderTable();

    unsigned char _0[0x18];
    MISSION_STATE m_state;   // 0x18 (SOCOM 1's)
    char *m_database;   // 0x1c (SOCOM 1's)
    unsigned char _20[0x60 - 0x20];
    char m_db_string[0x80];   // 0x60 (SOCOM 1's at 0x20)
    int m_timer;   // 0xe0: cleared by Open and before the second unload (SOCOM 1's at 0x60)
    unsigned char _e4[0xe8 - 0xe4];
    float m_e8;   // 0xe8: the mission's own clock, in seconds (the name ours)
    unsigned char _ec[0xfc - 0xec];
    unsigned char m_DelayEndMission : 1;   // 0xfc (SOCOM 1's bytes at 0x70, 0x71)
    unsigned char m_FadingToEndMission : 1;
    int m_fade_counter;   // 0x100 (SOCOM 1's at 0x74)
    float m_next_fade_time;   // 0x104 (SOCOM 1's at 0x78)
    float m_wait_before_falling;   // 0x108 (SOCOM 1's at 0x7c)
    std::vector<CCompassAnchor *> m_anchor_extraction;   // 0x10c (SOCOM 1's at 0x80)
    std::vector<CCompassAnchor *> m_anchors;   // 0x118: the nav points' anchors (SOCOM 1's at 0x8c)
    CCompassAnchor *m_whoistalking;   // 0x124: SOCOM II's (Open makes it "WhoIsTalking"; the name ours)
    std::vector<SObjectiveCam_Anchor> m_objanchor_vec;   // 0x128 (SOCOM 1's CObjAnchorVec at 0x98)
    unsigned char _134[0x1a4 - 0x134];
    CValve *m_valve_complete;   // 0x1a4 (SOCOM 1's at 0xe0)
    CValve *m_valve_failure;   // 0x1a8
    CValve *m_valve_abort;   // 0x1ac
    CValve *m_valve_timeout;   // 0x1b0
    CValve *m_valve_nofade;   // 0x1b4
    unsigned char _1b8[0x1804 - 0x1b8];
    class CLadderList *m_ladders;   // 0x1804 (SOCOM 1's at 0x580)
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char m_chars_loaded;   // 0x180c (SOCOM 1's at 0x588)
    unsigned char _180d[0x181c - 0x180d];
    CRdrFile *m_materialsRdr;   // 0x181c (SOCOM 1's at 0x598)
    unsigned char _1820[0x1860 - 0x1820];
    float m_music_hold;   // 0x1860: how long the music's raised intensity holds (SOCOM II's; the name ours)
    unsigned char _1864[0x18ec - 0x1864];
    zdb::CNode *m_sealbombshack;   // 0x18ec (SOCOM 1's at 0x658)
    zdb::CNode *m_terroristbombshack;   // 0x18f0 (SOCOM 1's at 0x65c)
};

/* ---- the unit's statics with constructors, in the order its initialiser (0x400160) builds them,
 * after <iostream>'s two (SOCOM 1's __sinit_fts_mission.cpp builds the same, but for the lobby's
 * save manager, SOCOM II's, and the extra string tables below) ---- */
CMemCard theMemCard;                        // 0x436460
CLobbySaveManager theLobbySaveManager;      // 0x4364A0 (zsav_lobby.h's; the name ours)
CMission theMission;                        // 0x4364E0

/* the UI variables' manager, a module of the game's save: 256 dormant slots (SOCOM 1's inline, as
 * its initialiser shows) */
inline CUIVarManager::CUIVarManager() : CSaveModule("CUIVarManager", theMemCard.m_gameSave)
{
    m_currtab = 0;
    resize(256);
}

CUIVarManager theUIVarManager;              // 0x437E80

/* the persistent valves' part of the game's save (valve_main.cpp's class, SOCOM 1's: its slots
 * CSaveModule's, its implicit destructor's copy this unit's, 0x2B1910) */
class CValveSaveManager : public CSaveModule {
public:
    CValveSaveManager() : CSaveModule("CValveSaveManager", theMemCard.m_gameSave) {}
    void Serialize(int flags);
    void UnSerialize(const char *name, CRdrFile *file, int flags);
};

CValveSaveManager theValveSaveManager;      // 0x43FF00

/* the template tail these statics bring (resize<CUIVariable> and what it calls, emitted after the
 * objective cameras' vector):
 * OPEN, resize<CUIVariable> 0x2B1390 (0x240 against 0x19C): the target inlines CUIVariable() one
 * level -- _zrdr() 0x2B17E0, UIVarMixin() 0x2B17C0, vector<UIVarMixin *>() 0x2B17A0 and the watchers'
 * reserve 0x2B1600 (a reserve of its own whose body is the pod's, its string this unit's) called --
 * where MW inlines the whole constructor, four high here (the pointer vector's constructor three).
 * A SIZE cut: under `#pragma inline_max_size(180)` (its expansion here is between 180 and 200
 * units; the original's passed 256) the body is the target's but for that reserve's callee, the
 * pod's 0x1EEBA0; with a forwarder `void reserve(unsigned int n) { __base::reserve(n); }` added to
 * include/vector's pointer __vector_imp as well, resize is byte-exact calling
 * reserve__Q23std62__vector_imp<P10UIVarMixin,Q23std24allocator<P10UIVarMixin>,1>FUi at 0x2B1600 --
 * but that forwarder's own copy is then a `j` to the pod's, where 0x2B1600 holds the pod's body
 * inlined (probes: build/scratch/states/, the states lane's round 22; neither change adopted).
 * OPEN, insert<CUIVariable> 0x2B1BD0 (0x510, 46 instructions, registers only): the copy loops'
 * end pointer takes s6 where the target's takes s0 (and the cursors s1/s6 against s0/s1) -- the
 * class insert's end register, as the cue vector's 0x1EED40 (include/vector's, the MSL lane's) */

/* the lobby's save manager (SOCOM II's; CMenuState::UnInit closes it through this, CAcctDB registers
 * with it; OVL1 0x2AFBA0, the name ours) */
CLobbySaveManager *GetLobbySaveManager()
{
    return &theLobbySaveManager;
}

extern "C" char *strncpy(char *dst, const char *src, unsigned int n);

/* the world, the entities, the AI events, the scheduler and the sound (their units'; called by their
 * map names) */
class CEntity;
class CEntityList : public std::list<CEntity *> {
};
class CEntity {
public:
    static void RemoveAll();

    static const unsigned int invalidId;   /* 0x3EFFB0 */
    static CEntityList m_list;   /* 0x4362E0 */
};
class CAiEvent {
public:
    static void Open(unsigned int count);   /* OVL2 0x50F150 */
    static void Close();
};
extern CSched_Manager zTaskScheduler;
extern "C" void ClearFreeList__14CSched_ManagerFb(CSched_Manager *sched, bool all);
extern "C" bool AddTask__14CSched_ManagerFPCcPFfPv_bfPv(CSched_Manager *sched, const char *name,
                                                         bool (*func)(float dT, void *data), float priority, void *data);

/* the mission completed (SOCOM 1's name, at its call; OVL1 0x2AA910, not written yet) */
void _OnMissionComplete();

/* the video (zvid's CVideo: only the screen's clear) */
class CVideo {
public:
    static void Clear(bool on, unsigned char r, unsigned char g, unsigned char b, unsigned char a);   /* 0x3AFD50 */
};

extern "C" {
void Flash__4CHUDFfRC6CPnt3Dfff(CHUD *hud, float time, const CPnt3D *colour, float in, float hold, float out);
void Reset__14CFlashFXVectorFv(void *flashes);
void ResetDeathCam__10CAppCameraFv(void *camera);
void Push__5CGameFP10CGameStateUi(void *game, void *state, unsigned int arg);
extern void *appCamera;
extern unsigned char data_004086A0[];                    /* (a state in theGame, at 0x1E0) */
}

/* the game and the two states a network error reaches (SOCOM 1's CGame and COurGame, only what this
 * unit touches; Apps/FTS/fts_state.h has the whole of them): the stack's top, PopTo and Switch, the
 * front end's menu state at 0x78 and the load state at 0xEC */
class CGameState {
public:
    virtual ~CGameState();

    char *m_name;   // 0x4
};

class CMenuState : public CGameState {
public:
    void SetDefaultMenu(const char *name) { strncpy(m_DefaultMenu, name, 64); }

    unsigned char _8[0x30 - 0x8];
    char m_DefaultMenu[64];   // 0x30
    int m_iopregh;   // 0x70
};

class CLoadState : public CGameState {
public:
    void AbortLoad(const char *dlgname);

    unsigned char _8[0x24 - 0x8];
};

class CGame {
public:
    enum { MAX_STATES = 16 };

    virtual ~CGame();

    CGameState *PopTo(CGameState *state, unsigned int arg);
    CGameState *Switch(CGameState *state, unsigned int arg);
    CGameState *GetCurrentState() { return m_CurrentState >= MAX_STATES ? 0 : m_Stack[m_CurrentState]; }

    unsigned char m_Active;   // 0x4
    CGameState *m_InitialState;   // 0x8
    unsigned int m_CurrentState;   // 0xc
    CGameState *m_TestVariable;   // 0x10
    CGameState *m_Stack[MAX_STATES];   // 0x14
    int m_MinIdlePeriod;   // 0x54
    unsigned char m_StateChangeQueue[0x18];   // 0x58
    unsigned int m_pad;   // 0x70
    float m_maxtick;   // 0x74
};

class COurGame : public CGame {
public:
    CMenuState m_menuState;   // 0x78
    CLoadState m_loadState;   // 0xec
};

extern COurGame theGame;

/* the network game's end (fts_remote.cpp's units: SOCOM 1's names) and the net-error handler
 * st_load.cpp installs */
void ftsNetworkGameUninit();
void DMEDisconnectFromGameServer();
extern void (*data_0044F3CC)(int);

/* the mission ended, unless held: completed, the screen and the flashes cleared, the camera's death
 * view reset and the exit state pushed (SOCOM II's, the name ours: SOCOM 1's fade tick did this in
 * place; CCoreState::Tick calls it, the fade tick below inlines it; OVL1 0x2A9A70) */
void CMission::EndMission()
{
    if (!m_DelayEndMission) {
        _OnMissionComplete();
        CVideo::Clear(true, 0, 0, 0, 0);
        Reset__14CFlashFXVectorFv(theHUD->m_flashes);
        theMission.SetDelayEndMission(false);
        SetFadingToEndMission(false);
        ResetDeathCam__10CAppCameraFv(appCamera);
        Push__5CGameFP10CGameStateUi(&theGame, data_004086A0, 0);
    }
}

/* the fade's steps: two flashes, then the mission completed and the exit state pushed (SOCOM 1's,
 * the same steps; OVL1 0x2A98B0) */
bool CMission::FadeToEndMissionTick(float dT)
{
    if (!(m_next_fade_time <= 0.0f)) {
        m_next_fade_time -= dT;
        if (m_next_fade_time <= 0.0f) {
            if (m_fade_counter > 0) {
                CPnt3D black(0.0f, 0.0f, 0.0f);

                Flash__4CHUDFfRC6CPnt3Dfff(theHUD, 0.0f, &black, 0.5f, 0.5f, 0.5f);
                m_next_fade_time = 5.0f;
            } else if (m_fade_counter == 0) {
                CPnt3D black(0.0f, 0.0f, 0.0f);

                Flash__4CHUDFfRC6CPnt3Dfff(theHUD, 0.0f, &black, 0.8f, 5.0f, 0.8f);
                m_next_fade_time = 1.1f;
            } else {
                SetDelayEndMission(false);
                EndMission();
                return true;
            }
            m_fade_counter--;
        }
    }
    return false;
}

class CZSealBody;
CZSealBody *ftsGetPlayer();

extern "C" {
float func_002A4A10(CZSealBody *body, int a, float range);   /* (the danger around the player) */
extern float data_003DE618;   /* (the raised intensity's hold time) */
}

/* the music's intensity kept up with the danger around the player: above 1 it is raised and held
 * for a while, renewed when the hold runs out; once the danger passes, the hold runs down and the
 * intensity drops to nothing (SOCOM II's; OVL1 0x2A4920) */
extern "C" void func_002A4920(CMission *mission, float dT)
{
    float intensity = 0.0f;
    CZSealBody *player = ftsGetPlayer();

    if (player)
        intensity = func_002A4A10(player, 2, 350.0f);
    if (intensity > 1.0f) {
        if (mission->m_music_hold <= 0.0f) {
            CSndJukebox::SetIntensity(intensity);
            mission->m_music_hold = data_003DE618;
        }
        mission->m_music_hold -= dT;
    } else if (mission->m_music_hold != 0.0f) {
        mission->m_music_hold -= dT;
        if (mission->m_music_hold <= 0.0f) {
            mission->m_music_hold = 0.0f;
            CSndJukebox::SetIntensity(0.0f);
        }
    }
}

/* a dialog shown for the online game, by its state: in play, the front end opened on it; at a
 * round's or the game's end, the stack popped first; in the menus or loading, the load aborted to
 * it (or the same pop) -- _RecoverFromNetError's chain without its teardown (SOCOM II's; a client
 * leaving a game under way calls it with "dlgNetAbandoned.rdr"; OVL1 0x2A47B0) */
extern "C" bool func_002A47B0(CMission *mission, const char *dialog)
{
    bool handled = false;

    if (mission->m_pNetGame) {
        switch (mission->m_pNetGame->m_my_major_game_state) {
        case CZNetGame::MP_MAJOR_PLAY:
        case CZNetGame::MP_MAJOR_UNKNOWN:
            break;
        case CZNetGame::MP_MAJOR_MENUS:
        case CZNetGame::MP_MAJOR_LOADING:
            if (theGame.GetCurrentState() == &theGame.m_loadState) {
                theGame.m_loadState.AbortLoad(dialog);
                handled = true;
                break;
            }
            theGame.PopTo(0, 0);
            theGame.m_menuState.SetDefaultMenu(dialog);
            theGame.Switch(&theGame.m_menuState, 0);
            handled = true;
            break;
        case CZNetGame::MP_MAJOR_ROUNDOVER:
        case CZNetGame::MP_MAJOR_GAMEOVER:
        case CZNetGame::MP_MAJOR_NEWROUND:
            theGame.PopTo(0, 0);
        case CZNetGame::MP_MAJOR_INPLAY:
            theGame.m_menuState.SetDefaultMenu(dialog);
            theGame.Switch(&theGame.m_menuState, 0);
            handled = true;
            break;
        }
    }
    return handled;
}

/* a network error handled by the online game's state: in play, the front end's error dialog; at a
 * round's or the game's end, the network game torn down and the stack popped first; loading, the
 * load aborted to the dialog (SOCOM II's test) or the same teardown (OVL1 0x2A4600; SOCOM 1's
 * chain, the cases in reverse of its tests) */
bool CMission::_RecoverFromNetError(int error)
{
    bool handled = false;

    if (error && m_pNetGame) {
        switch (m_pNetGame->m_my_major_game_state) {
        case CZNetGame::MP_MAJOR_PLAY:
        case CZNetGame::MP_MAJOR_UNKNOWN:
        case CZNetGame::MP_MAJOR_MENUS:
            break;
        case CZNetGame::MP_MAJOR_LOADING:
            if (theGame.GetCurrentState() == &theGame.m_loadState) {
                theGame.m_loadState.AbortLoad("dlgNetError.rdr");
                data_0044F3CC = 0;
                handled = true;
                break;
            }
            ftsNetworkGameUninit();
            DMEDisconnectFromGameServer();
            theGame.PopTo(0, 0);
            data_0044F3CC = 0;
            theGame.m_menuState.SetDefaultMenu("dlgNetError.rdr");
            theGame.Switch(&theGame.m_menuState, 0);
            handled = true;
            break;
        case CZNetGame::MP_MAJOR_ROUNDOVER:
        case CZNetGame::MP_MAJOR_GAMEOVER:
        case CZNetGame::MP_MAJOR_NEWROUND:
            ftsNetworkGameUninit();
            DMEDisconnectFromGameServer();
            theGame.PopTo(0, 0);
        case CZNetGame::MP_MAJOR_INPLAY:
            data_0044F3CC = 0;
            theGame.m_menuState.SetDefaultMenu("dlgNetError.rdr");
            theGame.Switch(&theGame.m_menuState, 0);
            handled = true;
            break;
        }
    }
    return handled;
}

/* the net-error callback: theMission's own handler, a tail call (SOCOM 1's static
 * RecoverFromNetError; OVL1 0x2A45F0) */
void CMission::RecoverFromNetError(int error)
{
    theMission._RecoverFromNetError(error);
}

/* the external whispers' option: its global (0x3DE620, the HUD reads it) and EnableExternalWhispers
 * (SOCOM II's; the options' store and the HUD call it; OVL1 0x2A41F0) */
extern "C" bool data_003DE620;

extern "C" void func_002A41F0(bool on)
{
    /* the valve set written out: `on` converted for the comparison and again for the store */
    CValve *valve = CValve::Create("EnableExternalWhispers", (VALVE_TYPE)1);
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != on)
        state = on == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = on;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
    data_003DE620 = on;
}

extern "C" int func_0019AFB0(int c);   /* (MAIN: tolower) */

/* a nav point's anchor type from its name's first letter, 'c' onwards (SOCOM 1's
 * FilterNavPointString, SOCOM II's types 14 on where SOCOM 1's started at 9; inlined). The
 * target's layout -- the zero arm first, `slti 24` then `bnez` to the sum -- is this if/else into
 * a result with `i > 23`: `>= 24`, the `&&` forms, returns and ternaries all lay it out otherwise */
inline int FilterNavPointString(const char *s)
{
    int i = (char)func_0019AFB0(*s) - 'c';
    int r;

    if (i < 0 || i > 23)
        r = 0;
    else
        r = i + 14;
    return r;
}

/* a compass anchor for each of the AI maps' nav points whose cell has a centre, typed by its name
 * (SOCOM 1's, the centre now tested; OVL1 0x2ACAD0) */
void CMission::CreateCompassNavPoints()
{
    std::vector<CAiNavPoint> &navs = AiMaps.m_navpoints;
    unsigned int n = navs.size();

    if (n) {
        for (unsigned int i = 0; i < n; i++) {
            /* the element through the data pointer itself: through the inline [] the reference
             * takes its saved register after the name's (s1 and s2 swapped) */
            CAiNavPoint &nav = navs.data_[i];
            const char *name = nav.m_name;
            CPnt3D pos;

            if (AiMaps.GetCellCenter(nav.m_loc, &pos)) {
                CCompassAnchor *anchor = CCompass::CreateAnchor(pos, name);

                anchor->m_type = FilterNavPointString(name);
                anchor->m_aiMapLoc = &nav.m_loc;
                m_anchors.push_back(anchor);
            }
        }
    }
}

/* a box: its two corners (the names ours) */
struct CornerBox {
    float min[3];   // 0x0
    float max[3];   // 0xc
};

/* a box's corners put in order, axis by axis (SOCOM II's; the rdr save code at 0x338B30 and
 * 0x339058 calls it; OVL1 0x2A94E0) */
extern "C" void func_002A94E0(CornerBox *box)
{
    if (box->min[0] > box->max[0]) {
        float t = box->min[0];

        box->min[0] = box->max[0];
        box->max[0] = t;
    }
    if (box->min[1] > box->max[1]) {
        float t = box->min[1];

        box->min[1] = box->max[1];
        box->max[1] = t;
    }
    if (box->min[2] > box->max[2]) {
        float t = box->min[2];

        box->min[2] = box->max[2];
        box->max[2] = t;
    }
}

/* the box around eight points -- a box's corners carried through a matrix -- each bound a ternary
 * keeping the point when it is beyond (`p <= min ? p : min`, `p >= max ? p : max`: MIN/MAX's operand
 * order compares the other way round). OVL1 0x2A9550: the copy the world's ladder walk (0x2A9260)
 * and grid_main.cpp's gridAddNodeToGrids call, after the corners (0x2A96B0, open: its CPnt3D(fff)
 * and assignments are calls in the target's copy, inlined in every shape tried -- a class, a member,
 * a class template, a template member out of class) and their transform (0x2A9680) */
extern "C" void func_002A9550(CBBox *out, const CPnt3D *p)
{
    out->m_min = p[0];
    out->m_max = p[0];
    for (int i = 1; i < 8; i++) {
        out->m_min.x = p[i].x <= out->m_min.x ? p[i].x : out->m_min.x;
        out->m_min.y = p[i].y <= out->m_min.y ? p[i].y : out->m_min.y;
        out->m_min.z = p[i].z <= out->m_min.z ? p[i].z : out->m_min.z;
        out->m_max.x = p[i].x >= out->m_max.x ? p[i].x : out->m_max.x;
        out->m_max.y = p[i].y >= out->m_max.y ? p[i].y : out->m_max.y;
        out->m_max.z = p[i].z >= out->m_max.z ? p[i].z : out->m_max.z;
    }
}

/* a ladder (SOCOM 1's CLadderData, 0x28 bytes, SOCOM II's laid out again: its box first, then its
 * node, the SEALs on it and the one holding it -- seal_env.cpp's -- and its place in the list; the
 * last three names ours) */
class CLadderData {
public:
    CLadderData() {}
    /* a new ladder: its box, its node, no SEALs on it and none holding it, its place (ours: inline,
     * its arguments read before the stores, the fields' addresses kept across the walk) */
    CLadderData(const CBBox &box, zdb::CNode *node, int index)
    {
        m_bbox = box;
        m_node = node;
        m_users = 0;
        m_owner = CEntity::invalidId;
        m_index = index;
    }

    CBBox m_bbox;   // 0x0 (SOCOM 1's at 0xc, after its normal)
    zdb::CNode *m_node;   // 0x18
    int m_users;   // 0x1c
    unsigned int m_owner;   // 0x20
    int m_index;   // 0x24
};

/* the bit a ladder's index carries out of GetClosestLadder (SOCOM 1's global, OVL1 0x3DE5F8) */
extern int LADDER_MASK;

/* the mission's ladders (SOCOM 1's CLadderList : vector<CLadderData>) */
class CLadderList : public std::vector<CLadderData> {
public:
    int GetClosestLadder(const CPnt3D *pos, float range);
    CLadderData *GetLadderByNode(zdb::CNode *node, int depth);
    CLadderData *GetLadderByIndex(int index);
};

/* a ladder entered once: the one for a node, else a new one for it, found again (ours: inlined in
 * CreateLadderTable -- the node tested again, the find's result unused there) */
inline CLadderData *AddLadder(CLadderList *ladders, zdb::CNode *node, const CBBox &box)
{
    CLadderData *ladder = ladders->GetLadderByNode(node, 0);

    if (!ladder && node) {
        CLadderData d(box, node, ladders->size());

        ladders->push_back(d);
        ladder = ladders->GetLadderByNode(node, 0);
    }
    return ladder;
}

/* a node's box carried through a matrix: its corners (0x2A96B0, open: the copy calls CPnt3D(fff) and
 * the assignments out of line), transformed (0x2A9680) and boxed again (0x2A9550) -- one inline
 * level here, as the world's ladder walk and gridAddNodeToGrids call all four (ours) */
extern "C" CPnt3D *func_002A96B0(CPnt3D *pts, const CBBox *box);
extern "C" CPnt3D *func_002A9680(const CMatrix *m, CPnt3D *pts);

inline void GetWorldBBox(zdb::CNode *node, const CMatrix &m, CBBox *out)
{
    PNT3D pts[8];

    func_002A96B0((CPnt3D *)pts, node->GetBBox());
    func_002A9680(&m, (CPnt3D *)pts);
    func_002A9550(out, (CPnt3D *)pts);
}

/* a DI's app flags (tag_DI_HEAD's m_appflags, the flags word's bits 20-22): 2 a ladder polygon
 * (the names ours) */
enum DI_APPFLAGS {
    DI_APP_0,
    DI_APP_1,
    DI_APP_LADDER
};

/* the ladders from the world: every node with a ladder polygon, its box carried through its matrix
 * (SOCOM 1's CreateLadderTable; OVL1 0x2A9260, Open calls it). The flags are read as a byte and
 * masked again -- `(unsigned char)flags & 7`, the target's `andi 0xff; andi 7` after the extract
 * (SOCOM 1's has the `andi 7` alone; no cast, enum bitfield or accessor gives both); the box helper
 * takes the corners themselves, not the transform's return; the ladder's index is read ahead of its
 * stores, as a constructor's argument */
void CMission::CreateLadderTable()
{
    if (!zdb::NodeUniverse)
        return;
    if (!m_ladders) {
        m_ladders = new CLadderList;
        if (!m_ladders)
            return;
    }
    for (zdb::CNodeVector::iterator i = zdb::NodeUniverse->begin(); i != zdb::NodeUniverse->end(); ++i) {
        zdb::CNode *node = *i;

        if (!node)
            continue;
        for (zdb::CDIVector::iterator d = node->m_di.begin(); d != node->m_di.end(); ++d) {
            if (((unsigned char)(*d)->m_flags.m_appflags & 7) == DI_APP_LADDER) {
                CBBox world;
                CMatrix m;

                m.SetIdentity();
                if (node->BuildMTW(m)) {
                    GetWorldBBox(node, m, &world);
                    CLadderList *ladders = m_ladders;

                    AddLadder(ladders, node, world);
                }
                break;
            }
        }
    }
}

/* a point less another by value, through Sub (the copy out of the temporary: the target's) */
static inline CPnt3D Minus(const CPnt3D *a, const CPnt3D *b)
{
    CPnt3D r;

    a->Sub(b, &r);
    return r;
}

/* the ground-plane length, the x and z made a unit direction (ours) */
static inline float NormalizeXZ(CPnt3D *v)
{
    float len = sqrtf(v->x * v->x + v->z * v->z);

    if (len != 0.0f) {
        float inv = 1.0f / len;

        v->x *= inv;
        v->z *= inv;
    }
    return len;
}

/* the ladder at an index, the mask's bit taken off it, or none (SOCOM 1's; OVL1 0x2A9210) */
CLadderData *CLadderList::GetLadderByIndex(int index)
{
    index &= ~LADDER_MASK;
    if (index < 0 || (unsigned int)index >= size())
        return 0;
    return &(*this)[index];
}

/* the ladder a node is, else one among its children to some depth (SOCOM II's; the world's ladder
 * walk 0x2A9260 and the SEALs call it; OVL1 0x2A90F0) */
CLadderData *CLadderList::GetLadderByNode(zdb::CNode *node, int depth)
{
    CLadderData *found = 0;

    if (node) {
        for (iterator i = begin(); i != end(); ++i) {
            if (i->m_node == node) {
                found = &*i;
                break;
            }
        }
    }
    if (!found && depth > 0) {
        zdb::CNodeVector &children = node->GetChildren();

        for (zdb::CNodeVector::iterator c = children.begin(); c != children.end(); ++c) {
            found = GetLadderByNode(*c, depth - 1);
            if (found)
                break;
        }
    }
    return found;
}

/* the ladder nearest a point in the ground plane, of those whose foot or top is level with it
 * (within 15), closer than a range: its index with the mask's bit, else -1 (SOCOM 1's
 * GetClosestLadder: SOCOM II's tolerance a constant, the mask set at the find; OVL1 0x2A8F50).
 * The iterator is taken before the unused matrix is built, as both builds load begin() ahead of
 * sceVu0UnitMatrix; the normalise's z store is dead (the target keeps only x's) */
int CLadderList::GetClosestLadder(const CPnt3D *pos, float range)
{
    int index = 0;
    iterator i = begin();
    CMatrix unused;
    int closest = -1;

    for (; i != end(); ++i, ++index) {
        if (_close_enough(pos->y, i->m_bbox.m_min.y, 15.0f) || _close_enough(pos->y, i->m_bbox.m_max.y, 15.0f)) {
            CPnt3D mid;

            i->m_bbox.GetCenter(&mid);
            CPnt3D d = Minus(pos, &mid);
            float dist = NormalizeXZ(&d);

            if (dist < range) {
                range = dist;
                closest = index | LADDER_MASK;
            }
        }
    }
    return closest;
}

/* a range kept at 0xF4 (the top) and 0xF8 (the bottom) of the object 0x341A60 asks (SOCOM II's; the
 * names ours) */
struct FadeRange {
    unsigned char _0[0xf4];
    float top;   // 0xf4
    float bottom;   // 0xf8
};

/* where a value lies in the range, 0 to 1 (a half when the range is empty), the ends put in order
 * first (OVL1 0x2ACA30) */
extern "C" float func_002ACA30(FadeRange *r, float x)
{
    float f = 0.5f;

    if (r->top < r->bottom) {
        float t = r->top;

        r->top = r->bottom;
        r->bottom = t;
    }
    if (!(x <= r->top))
        f = 1.0f;
    else if (x < r->bottom)
        f = 0.0f;
    else {
        float d = r->top - r->bottom;

        if (d != 0.0f)
            f = (x - r->bottom) / d;
    }
    return f;
}

/* ---- what CMission::Open calls (their units'; the placeholders by their map names) ---- */

/* the archives (zrdr.h's CRdrArchive: only the two statics Open calls) */
class CRdrArchive {
public:
    static CRdrArchive *AddArchive(const char *name, const char *path);   /* 0x32C1E0 */
    static void OpenAll();   /* 0x32C570 */
};

const char *GetDatabase();   /* 0x27F800 */
CRdrFile *zrdr_read(const char *name, const char *dir, int flags);   /* 0x32F630 */
void zrdr_free(CRdrFile *file);   /* 0x32D6E0 */

/* the localised string tables (util_loc.cpp's; only the load), the language a word as st_mpexit.cpp
 * has it */
enum LANGUAGE {
    LANGUAGE_ALIGNMENT_HACK = 0x7FFFFFFF
};
LANGUAGE GetLanguage();   /* 0x3A4460 */

class CStringTables {
public:
    void Load(const char *name, LANGUAGE language);   /* 0x3A3900 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

/* the particles (zparticle.h's namespace: only its open) */
namespace Particle {
void Open();   /* 0x324B20 */
}

/* the materials (int_material.cpp's CMaterialExArray: only the default's setter) */
class CMaterialExArray {
public:
    static void SetDefaultMaterial(int index);   /* 0x2DDC30 */
};

/* the character models' cache (zchar's CCharacterModelCache: only its vector) */
class CCharacterModelCache {
public:
    static zdb::CModelVector m_ModelCache;   /* 0x44CDA8 */
};

/* the bullet marks (zweapon's CZMarkList: only its init) and the weapons' statics */
class CZMarkList {
public:
    void Init();   /* 0x3D16B0 */
};

class CZWeapon {
public:
    static void Open();   /* 0x3D03D0 */
    static void ClearTimer(float *pCurval);

    static CZMarkList *m_pMarklist;   /* 0x4B5248 */
};

/* a character type's weapons (zchar's CCharacterWeapList: only the slots' padding, OVL2's) */
class CCharacterWeapList {
public:
    void PadWeaponSlots();   /* OVL2 0x53B070 */
};

/* a character type (zchar's CCharacterType: only what the custom types and Open touch) */
class CCharacterType {
public:
    static CCharacterType *RdrFetch_CharacterType(const char *name);
    static const CCharacterType *GetByNameMod(const char *name);
    static CCharacterType *Create(const char *name, const CCharacterType *base, bool temporary);
    static void AcquireSoundHandles();   /* OVL2 0x53C200 */

    char *m_name;   // 0x0
    unsigned char _4[0x2e8 - 0x4];
    CCharacterWeapList m_weapons;   // 0x2e8
    unsigned char _2e9[0x300 - 0x2e9];
    unsigned char m_custom;   // 0x300: a custom type (the name ours: stored with li, so no bool)
};

/* the player's SEAL and its controller (only the controller's state byte, and the network player's
 * creation) */
class CSealCtrl {
public:
    unsigned char _0[0x221];
    unsigned char m_221;   // 0x221
};

class CZSealBody {
public:
    static void NetworkPlayerCreate();   /* OVL2 0x5442C0 */

    unsigned char _0[0xc0];
    CSealCtrl *m_control;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char m_e1_0 : 4;   // 0xe1
    unsigned char m_e1_4 : 1;   // 0xe1, bit 4: TRIGGER_POPUP's test (the name ours)
};

CZSealBody *ftsGetPlayer();

/* the pickups (OVL2's CPickup: only its open) */
class CPickup {
public:
    static void Open(unsigned int count);   /* OVL2 0x542FC0 */
};

extern "C" {
int strncmp(const char *a, const char *b, unsigned int n);
char *strcat(char *dst, const char *src);
char *strcpy(char *dst, const char *src);
int sprintf(char *buf, const char *fmt, ...);
void func_00344450(const char *db);   /* (the mission's sounds loaded) */
void func_00344910(const char *db);
void func_003448D0(const char *name);   /* (the jukebox's lists) */
void func_002DDE40(CRdrFile *rdr);   /* (SOCOM 1's CMaterialExArray::ReadMaterials) */
int func_002DE9E0(const char *name);   /* (SOCOM 1's CMaterialExArray::ResolveMaterialIndex) */
extern const char *data_0064F950;   /* (OVL2: "aimaps.mps") */
void func_002A4D30(CMission *mission, CRdrFile *rdr);   /* (SOCOM 1's CMission::LoadObjectiveCams) */
void func_002A5290(CMission *mission, CRdrFile *rdr);   /* (SOCOM 1's CMission::LoadMissionCams) */
void func_002B4F40(zdb::CWorld *world, const char *name);   /* (SOCOM 1's CNodeAction::Open) */
void func_003D03C0();   /* (SOCOM 1's CZWeapon::OpenDecals) */
void func_005A4CF0();   /* (OVL2: SOCOM 1's CZSealBody::InitDecalHandles) */
void func_00574C30();   /* (OVL2) */
void func_002B2E50(CMission *mission);   /* (SOCOM 1's CMission::LoadVehicles) */
void func_005A4CE0();   /* (OVL2: SOCOM 1's CZSealBody::InitDatabaseHandles) */
void func_00295660();
void func_005EC6A0();   /* (OVL2: SOCOM 1's CSealUnit::CreateSinglePlayerUnits) */
void func_005ECCD0();   /* (OVL2: SOCOM 1's CSealUnit::CreateMultiPlayerUnits) */
void func_002A8140(CAiMap *map);   /* (SOCOM 1's ftsFixupAiMap) */
void func_002A42B0(CMission *mission);   /* (SOCOM 1's CMission::Sample_ExtractionLoc) */
}

/* the mission opened: the database's archive and string tables, the vectors reserved, the compass,
 * the scheduler's three tasks, the sounds and the jukebox, the materials, the mission's valves, the
 * AI maps and the nav points, the talker's anchor (SOCOM II's), the particles, the anims, the
 * objective and mission cameras, the actions, the ladders, the decals and marks, the SEALs, the
 * vehicles, the weapons, the AI events, the pickups, the units, the AI maps fixed up, the world's
 * descendants, the bomb shacks of a network game, the extraction point (SOCOM 1's Open, its sounds
 * SOCOM II's own loaders; OVL1 0x2AC410) */
bool CMission::Open(const char *name, int load_ai)
{
    m_wait_before_falling = 10.0f;
    zdb::CWorld *world = zdb::CWorld::m_world;
    char path[512] = "run/";
    const char *db = GetDatabase();

    if (db) {
        if (!strncmp(db, "mp", 2))
            strcat(path, "mp/");
        else if (!strncmp(db, "m", 1))
            strcat(path, "sp/");
        strcat(path, db);
        CRdrArchive::AddArchive("readerm.zar", path);
        CStringTables *tables = GetStringTables();

        tables->Load(name, GetLanguage());
        CRdrArchive::OpenAll();
    }
    if (!name)
        return false;
    m_anchor_extraction.reserve(8);
    m_objanchor_vec.reserve(16);
    theCompass.Open();
    if (name)
        strcpy(m_database = m_db_string, name);
    else
        m_database = 0;
    m_timer = 0;
    m_chars_loaded = 0;
    AddTask__14CSched_ManagerFPCcPFfPv_bfPv(&zTaskScheduler, "diTick",
                                            (bool (*)(float, void *))zdb::CWorld::diTick, 5.0f, 0);
    AddTask__14CSched_ManagerFPCcPFfPv_bfPv(&zTaskScheduler, "Mission", tick, 0.98f, this);
    AddTask__14CSched_ManagerFPCcPFfPv_bfPv(&zTaskScheduler, "Update visual effects",
                                            (bool (*)(float, void *))zdb::CNode::VisualEffectsTick, 0.5f, 0);
    CSnd::Close();
    CSnd::UnloadAllSounds();
    func_00344450(m_database);
    func_00344910(m_database);
    func_003448D0("sndjukeboxm.rdr");
    CSndJukebox::Start();
    func_002DDE40(m_materialsRdr);
    CMaterialExArray::SetDefaultMaterial(world->m_default_soiltype =
                                             func_002DE9E0(world->m_default_soiltype_name));
    m_valve_complete = CValve::Create("mission_complete", VALVE_PERM);
    m_valve_failure = CValve::Create("mission_failure", VALVE_PERM);
    m_valve_abort = CValve::Create("mission_abort", VALVE_PERM);
    m_valve_timeout = CValve::Create("mission_timeout", VALVE_PERM);
    CValve::Create("mission_replay", 0, VALVE_PERM);
    m_valve_nofade = CValve::Create("nofade", VALVE_PERM);
    InitalizeTeamValves();

    char maps[1024];

    if (!strncmp(m_database, "mp", 2))
        sprintf(maps, "/run/mp/%s/", m_database);
    else if (!strncmp(m_database, "m", 1))
        sprintf(maps, "/run/sp/%s/", m_database);
    else
        sprintf(maps, "/run/%s/", m_database);
    AiMaps.ReadMapSet(data_0064F950, maps);
    CreateCompassNavPoints();
    m_whoistalking = CCompass::CreateAnchor(CPnt3D(0.0f, 0.0f, 0.0f), "WhoIsTalking");
    m_whoistalking->m_type = 9;
    m_whoistalking->SetEnabled(false);
    Particle::Open();
    ZAnimLoad();
    CRdrFile *rdr = zrdr_read("hudobjectives.rdr", 0, 0);

    if (rdr) {
        func_002A4D30(this, rdr);
        zrdr_free(rdr);
    }
    rdr = zrdr_read("missioncams.rdr", 0, 0);
    func_002A5290(this, rdr);
    zrdr_free(rdr);
    func_002B4F40(world, "actions.rdr");
    CreateLadderTable();
    CCharacterModelCache::m_ModelCache.clear();
    func_003D03C0();
    if (CZWeapon::m_pMarklist)
        CZWeapon::m_pMarklist->Init();
    if (theNetwork.m_bNetwork)
        CZSealBody::NetworkPlayerCreate();
    func_005A4CF0();
    func_00574C30();
    func_002B2E50(this);
    CCharacterType::AcquireSoundHandles();
    CZWeapon::Open();
    world->AddCustomCharacters();
    func_005A4CE0();
    func_00295660();
    unsigned int count = CEntity::m_list.size();

    CAiEvent::Open(count);
    if (!theNetwork.m_bNetwork)
        CPickup::Open(count);
    if (!theNetwork.m_bNetwork)
        func_005EC6A0();
    else
        func_005ECCD0();
    RefreshTeamValves();
    CAiMaps::iterator i, e = AiMaps.end();

    for (i = AiMaps.begin(); i != e; ++i)
        func_002A8140(*i);
    world->InitDescendantsDI();
    world->InitDescendantsVisuals();
    if (theNetwork.m_bNetwork) {
        m_sealbombshack = world->FindChild("seal_tent", true);
        m_terroristbombshack = world->FindChild("terrorist_tent", true);
    }
    func_002A42B0(this);
    m_state = MISSION_RUNNING;
    return true;
}

/* the task's callback (SOCOM 1's fadeToEndMissionTick, the same jump; OVL1 0x2A98A0) */
bool CMission::fadeToEndMissionTick(float dT, void *data)
{
    return ((CMission *)data)->FadeToEndMissionTick(dT);
}

/* the fade to the mission's end begun, or brought nearer (SOCOM 1's, its two flags bits now;
 * OVL1 0x2A97C0) */
void CMission::FadeToEndMission(float time, int counter)
{
    if (theNetwork.m_bNetwork)
        return;
    if (!m_FadingToEndMission) {
        m_FadingToEndMission = 1;
        theMission.m_DelayEndMission = 1;
        m_fade_counter = counter;
        m_next_fade_time = time;
        AddTask__14CSched_ManagerFPCcPFfPv_bfPv(&zTaskScheduler, "MissionFadeToEnd", fadeToEndMissionTick, 0.98f, this);
    } else {
        if (time < m_next_fade_time)
            m_next_fade_time = time;
        if (counter < m_fade_counter)
            m_fade_counter = counter;
    }
}

/* the second half of a reload's unload: the entities and AI events gone, the world's lists and
 * garbage cleared (SOCOM 1's), then -- SOCOM II's -- the scheduler's free list, the headset and
 * the sounds (OVL1 0x2AC060) */
bool CMission::ReOpen_Unload2()
{
    zdb::CWorld *world = zdb::CWorld::m_world;

    m_timer = 0;
    CEntity::RemoveAll();
    CAiEvent::Close();
    world->ClearLightMapList();
    world->ClearShadowList();
    world->GarbageCollect();
    ClearFreeList__14CSched_ManagerFb(&zTaskScheduler, true);
    Headset::Reset();
    CSndInstance::PostTick(0.0f, 0);
    return true;
}

/* a point of a camera path (the names ours): where, which way, and its name -- 0x98 bytes, the
 * stride of the list at 0x437DF0 (whose only user, 0x295260, steps through it) */
struct SCamPathPoint {
    CPnt3D m_pos;   // 0x0
    CPnt3D m_dir;   // 0xc
    char m_name[0x80];   // 0x18
};

class CCamPath : public std::vector<SCamPathPoint> {
public:
    int GetPoint(int index, int step, CPnt3D *pos, CPnt3D *dir, const char **name);
};

/* the point `step` on from `index`, wrapping both ways, read out through at(); the index it lands
 * on (OVL1 0x2AB260; the names ours) */
int CCamPath::GetPoint(int index, int step, CPnt3D *pos, CPnt3D *dir, const char **name)
{
    unsigned int n = size();

    if (n) {
        if (step > 0)
            index = (index + step) % n;
        else {
            if (index < 0)
                index = 0;
            index += step;
            if (index < 0)
                index += n;
        }
        *pos = at(index).m_pos;
        *dir = at(index).m_dir;
        *name = at(index).m_name;
    }
    return index;
}

/* the online game started (OVL1 0x2A9F00) */
CZNetGame *CMission::CreateNetGame()
{
    m_pNetGame->Initialize();
    return m_pNetGame;
}

/* respawning on in a game of type 5 only, and its valve (SOCOM II's; OVL1 0x2A7560) */
extern "C" bool func_002A7560(CZNetGame *game, bool on)
{
    CValve *valve = CValve::GetByName("Respawn");

    game->m_respawn = on && game->m_game_type == 5;
    SetValveValue(valve, game->m_respawn);
    return game->m_respawn;
}

/* the round's time left in whole seconds: the online game's clock in a network game, else the
 * mission's own (SOCOM II's, the name ours: the HUD's timer text, the bomb and CanPlaceBomb read it;
 * OVL1 0x2A6AD0) */
unsigned int CMission::GetTimeLeft()
{
    if (theNetwork.m_bNetwork && m_pNetGame)
        return 0.001f * m_pNetGame->m_round_clock;
    return (int)m_e8;
}

/* the round's clock cut by some seconds -- to one second when no more are left -- and its seconds
 * returned (SOCOM 1's DecrementRoundClockSec, the saved copy gone; CZGameState::UpdateLocalGame calls
 * it with 60; OVL1 0x2A6960) */
unsigned int CZNetGame::DecrementRoundClockSec(int sec)
{
    if (theNetwork.m_bSessionMaster) {
        int left = GetMaxGameTimeSec();

        if (sec < left)
            m_max_game_time = (left - sec) * 1000;
        else if (left)
            m_max_game_time = 1000;
    }
    return GetMaxGameTimeSec();
}

/* the online game let go of: its state and clocks cleared, the rounds back to none (OVL1 0x2A75F0,
 * SOCOM 1's UnInitialize ahead of Initialize, the fields moved; DestroyNetGame ends calling it).
 * OPEN, 6 instructions, registers only: the round valve's value, its state test and the constant 1
 * for 0x112 take v0 in the target, v1 here. Tried: the valve set through zanim.h's SetValue and
 * through the helper with the count stored apart */
void CZNetGame::UnInitialize()
{
    m_f8 = 0;
    m_100 = 0;
    m_mp_major_game_state = MP_MAJOR_UNKNOWN;
    m_my_minor_game_state = MP_MINOR_UNKNOWN;
    m_my_major_game_state = MP_MAJOR_UNKNOWN;
    SetE3RoundCount(0);
    m_bc = 0;
    m_c8 = 0;
    m_c0 = 0;
    m_112 = 1;
    m_d5 = 0;
    m_cf = 0;
    m_d6 = 0;
    m_late_joiner = false;
    m_ce = 0;
    m_ten_second_launch = false;
    m_110 = 0;
    m_18 = 0;
    m_10 = 0;
    m_1c = 0;
    m_d9 = 0;
    m_84[0] = 0;
    m_84[1] = 0;
    m_84[2] = 0;
    m_84[3] = 0;
    m_84[4] = 0;
    m_pMyMajorGameStateValve = 0;
    m_pMyMinorGameStateValve = 0;
    m_28 = 0;
    m_14 = 0;
    m_40 = 0;
    m_4c = 0;
}

/* the rounds played so far, and their valve (OVL1 0x2A7510) */
void CZNetGame::SetE3RoundCount(int count)
{
    m_e3_round_count = count;
    SetValveValue(m_pE3RoundValve, count);
}

/* the online game's character type: the custom one for the SEALs' team, else the first (SOCOM
 * II's; OVL1 0x2A74A0) */
const char *GetCustomCharTypeName(int i);
extern "C" bool func_002C3300(unsigned int team);   /* (fts_net_persona.cpp's: the SEALs' team) */

extern "C" void func_002A74A0(unsigned int team)
{
    int type = 1;

    if (func_002C3300(team))
        type = 5;
    int i = type - 1;   /* (an index of its own: passed as type - 1, MW folds the -1 into the table) */
    const char *name = GetCustomCharTypeName(i);
    CZNetGame *game = theMission.m_pNetGame;

    strncpy(game->m_12c, name, 31);
    game->m_12c[30] = 0;
}

/* the game's own state (OVL1 0x2A7490) */
CZNetGame::MP_MAJOR_GAME_STATE CZNetGame::MPMajorGameState(MP_MAJOR_GAME_STATE state)
{
    m_mp_major_game_state = state;
    return state;
}

/* this client's state, and its valve (OVL1 0x2A7420 and 0x2A73B0) */
CZNetGame::MP_MAJOR_GAME_STATE CZNetGame::MyMajorGameState(MP_MAJOR_GAME_STATE state)
{
    m_my_major_game_state = state;
    SetValveValue(m_pMyMajorGameStateValve, state);
    return state;
}

CZNetGame::MP_MINOR_GAME_STATE CZNetGame::MyMinorGameState(MP_MINOR_GAME_STATE state)
{
    m_my_minor_game_state = state;
    SetValveValue(m_pMyMinorGameStateValve, state);
    return state;
}

/* whether this client joined late, and its valve (OVL1 0x2A7340) */
bool CZNetGame::LateJoiner(bool late)
{
    m_late_joiner = late;
    SetValveValue(m_pLateJoinerValve, late ? 1 : 0);
    return late;
}

/* the scores back to the start (OVL1 0x2A72D0) */
void CZNetGame::ResetScores()
{
    SetE3RoundCount(0);
    m_120 = 0;
    m_124 = 0;
    m_128 = 64;
    m_d4 = 0;
}

/* the master's countdown to the launch, its seconds the valve's, started or called off
 * (OVL1 0x2A6B90; SOCOM II reads the valve ahead of the clock, and the clock into a local: in one
 * expression MW adds the product first) */
bool CZNetGame::TenSecondLaunch(bool launch)
{
    if (launch && !m_ten_second_launch && theNetwork.m_bSessionMaster) {
        int seconds = m_pLaunchSecondsValve->m_value;
        int now = theNetwork.zNetGetTime();

        m_launch_time = now + seconds * 1000;
        m_launch_f4 = 0;
    }
    if (!launch && m_ten_second_launch && theNetwork.m_bSessionMaster) {
        m_launch_time = 0;
        m_launch_f4 = 0;
    }
    m_ten_second_launch = launch;
    return launch;
}

/* the file's tables (SOCOM 1's names; SOCOM II has 32 visibility lists of 31 maps where SOCOM 1
 * had 10 of 9) */
namespace {
char g_arrDefCharTypeNames[32][64];
unsigned char g_arrMapVis[32][31];
int g_arrMapVisCounts[32];
int g_iNumUiVehicles;
}

extern "C" const char *data_003F02B0[32];   /* GetCustomCharTypeName's c_arrCustomCharTypeNames */
const char *GetCustomCharTypeName(int i);

const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
/* a custom type's weapons' UI variables (SOCOM 1's; OVL1 0x2AF190, not written yet) */
void UpdateCharWeaponUiVars(int i);

/* an array's count and its items through a non-const value (zanim.h's members, non-const and the
 * index an int: the HUD's parsers reuse the type they tested, where zui.h's const accessors -- the
 * _zrdr this unit now sees -- reload it) */
inline unsigned int ZCount(_zrdr *z) { return z->type == 4 ? z->length : 0; }
inline _zrdr *ZItem(_zrdr *z, int i) { return i < ZCount(z) ? &z->array[i] : 0; }

/* an array's element by an unsigned index (zrdr.h's Item: zanim.h's takes an int, which lets MW
 * walk the element address with the int counter where the target recomputes it each pass) */
inline _zrdr *ListItem(_zrdr *rdr, unsigned int i)
{
    return i < rdr->Count() ? &rdr->array[i] : 0;
}

/* a vehicle definition's entry (zchar's CVehicleRdrEntry: only its character's name) and the
 * definitions */
class CVehicleRdrEntry {
public:
    void SetCharacterName(const char *name);

    unsigned char _0[0x20];
    char m_charname[1];   // 0x20: the character type its crew is
};

class CVehicleRdr {
public:
    CVehicleRdrEntry *GetEntryByName(const char *name) const;
};


namespace {
/* the custom character types the "UiVehicles" list names, each a copy of the character type of
 * the vehicle entry its "Name" names -- the entry then crewed by the copy (SOCOM 1's; OVL1
 * 0x2AFA10).
 * OPEN, 10 instructions, registers only: the name and the entry's character name trade saved
 * registers (the target's name s3, the character name s2, which the copy then reuses; here s2 and
 * s3). Tried: the entry and the character name declared ahead of the name in four orders, the
 * character name at loop scope, written out twice (one register fewer, 0xC shorter), not const,
 * and the name read from the table directly (as a ternary, as an if-assign) */
void CreateCustomCharTypes(CVehicleRdr *vehicles, _zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "UiVehicles");

    g_iNumUiVehicles = list ? list->Count() : 0;
    for (int i = 0; i < g_iNumUiVehicles; i++) {
        const char *name = GetCustomCharTypeName(i);

        if (!name)
            break;
        CVehicleRdrEntry *entry =
            vehicles->GetEntryByName(zrdr_findstring(ListItem(list, i), "Name"));

        if (entry) {
            const char *base = entry->m_charname;

            strcpy(g_arrDefCharTypeNames[i], base);
            CCharacterType *custom =
                CCharacterType::Create(name, CCharacterType::GetByNameMod(base), false);

            custom->m_custom = 1;
            entry->SetCharacterName(name);
            custom->m_weapons.PadWeaponSlots();
            UpdateCharWeaponUiVars(i);
        }
    }
}

/* the custom character types the "UiVehicles" list names, up to the 32 names there are: each a copy
 * of the type its entry's "Name" fetches, that type's name kept as the default (SOCOM 1's, the
 * list's count SOCOM II's length; OVL1 0x2AF8C0) */
void CreateCustomCharTypes(_zrdr *rdr)
{
    _zrdr *list = zrdr_findtag(rdr, "UiVehicles");

    g_iNumUiVehicles = list ? list->Count() : 0;
    for (int i = 0; i < g_iNumUiVehicles; i++) {
        const char *name = GetCustomCharTypeName(i);

        if (!name)
            break;
        CCharacterType *type =
            CCharacterType::RdrFetch_CharacterType(zrdr_findstring(ListItem(list, i), "Name"));
        const char *base = type->m_name;

        strcpy(g_arrDefCharTypeNames[i], base);
        CCharacterType *custom = CCharacterType::Create(name, CCharacterType::GetByNameMod(base), false);

        custom->m_custom = 1;
        UpdateCharWeaponUiVars(i);
    }
}
}

/* the number of maps a list shows (OVL1 0x2AF890) */
int ftsGetNumVisibleMaps(unsigned int list)
{
    if (list < 32)
        return g_arrMapVisCounts[list];
    return 0;
}

/* a list's map (OVL1 0x2AF840) */
unsigned char ftsGetVisibleMap(unsigned int list, int i)
{
    return g_arrMapVis[list][i];
}

/* whether a list shows a map (OVL1 0x2AF7B0) */
bool ftsIsMapInVisibleList(unsigned int map, unsigned int list)
{
    if (list >= 32)
        return false;
    for (int i = 0; i < g_arrMapVisCounts[list]; i++)
        if (map == g_arrMapVis[list][i])
            return true;
    return false;
}

/* a custom character type's name (OVL1 0x2AF780) */
const char *GetCustomCharTypeName(int i)
{
    if (i < 32)
        return data_003F02B0[i];
    return 0;
}

/* a default character type's name (OVL1 0x2AF760) */
const char *GetDefCharTypeName(int i)
{
    return g_arrDefCharTypeNames[i];
}

/* the mission's extra string tables: their count, then their names, ten characters each (the
 * names ours) */
struct MissionBankList {
    /* the i'th table's name (the index unsigned: the target recomputes the address every pass) */
    const char *GetBank(unsigned int i) const { return m_banks[i]; }
    char *GetBank(unsigned int i) { return m_banks[i]; }

    /* the count changed: a fixed array's resize, its element loops empty for names -- the target
     * keeps both empty arms of the test (the names ours) */
    static void Destroy(int first, int last) {}
    static void Construct(int first, int last) {}
    void Resize(int n)
    {
        if (n < m_count)
            Destroy(n, m_count);
        else
            Construct(m_count, n);
        m_count = n;
    }

    /* (none to begin with: the unit's initialiser clears the count last) */
    MissionBankList() : m_count(0) {}

    int m_count;
    char m_banks[1][10];
};

extern "C" {
MissionBankList data_00440B80;
}

/* the extra string tables the mission's rdr lists, by name (SOCOM II's; PreOpen calls it; OVL1
 * 0x2AF410).
 * OPEN, 0x100 against 0x140: the target keeps two empty if/else tests on the count -- `0 < count`
 * before it is cleared, `count + 1 < count` before each increment -- both arms empty, the else arm
 * a jump placed after the epilogue: a fixed array's resize(n) whose element loops vanish for char
 * names. MW folds every empty arm tried away: empty inline helpers (plain or template), empty for
 * loops; round 23 added the resize as a ternary over the two helpers, int loops over a 10-byte
 * struct with a pseudo-destructor and a placement new (all folded), and loops MW cannot delete
 * (pointer loops with a user-declared empty destructor 0x280, a dead `while (k-- > 0)` 0x1C0). The
 * rest (the list's Count/Item/String, the name's address recomputed each pass) is written as the
 * target reads; the count is an `unsigned short` here where the target's lhu has no andi (an int
 * there, once the arms are found) */
extern "C" void func_002AF410(const _zrdr *rdr)
{
    data_00440B80.Resize(0);
    _zrdr *list = zrdr_findtag(rdr, "ExtraStringTables");
    unsigned short n = 0;

    if (list)
        n = list->Count();
    for (int i = 0; i < n; i++) {
        data_00440B80.Resize(data_00440B80.m_count + 1);
        strcpy(data_00440B80.GetBank(i), list->Item(i)->String());
    }
}

/* a string table loaded, then every extra one the mission lists (SOCOM II's; CLoadState::Init calls
 * it with the load screen's; OVL1 0x2AF360) */
extern "C" void func_002AF360(const char *bank)
{
    CStringTables *tables = GetStringTables();

    tables->Load(bank, GetLanguage());
    int n = data_00440B80.m_count;

    for (int i = 0; i < n; i++) {
        CStringTables *t = GetStringTables();

        t->Load(data_00440B80.GetBank(i), GetLanguage());
    }
}

/* whether the player's controller is in none of states 2 to 5 (SOCOM II's; OVL1 0x2AA890). One
 * negated && chain of `!=` tests: the null test branches to the shared return-0 block, the four
 * compares stay a chain (`==` joined by || folds three of them into a range test) and the two nops
 * of the vanished blocks come back; an early `if (!ctrl) return false;` has a block of its own. */
extern "C" bool func_002AA890()
{
    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player ? player->m_control : 0;

    if (!(ctrl && ctrl->m_221 != 2 && ctrl->m_221 != 3 && ctrl->m_221 != 4 && ctrl->m_221 != 5))
        return false;
    return true;
}

/* a switch command: ON or OFF (the letterbox's, the HUD's) */
struct _hud_switch_cmd {
    _zanim_cmd_hdr hdr;
    signed char on : 1;   // 0x4
    signed char off : 1;
};

/* STOP_IMAGE_RECORDER: a bare timeless command (OVL1 0x2AECC0) */
_zanim_cmd_hdr *StopImageRecorderParse(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* HUD ON (OVL1 0x2AEC20) */
_zanim_cmd_hdr *HUDCmdParseOn(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->on = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* HUD OFF (OVL1 0x2AEBA0) */
_zanim_cmd_hdr *HUDCmdParseOff(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->off = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* HUD ON / OFF: the HUD shown or hidden (OVL1 0x2AEB00) */
bool HUDCmdTickControlSwitch(_zanim_cmd_hdr *hdr, float *dT)
{
    _hud_switch_cmd *cmd = (_hud_switch_cmd *)hdr;

    if (!theHUD)
        return true;
    if (cmd->on)
        theHUD->On();
    else if (cmd->off)
        theHUD->Off();
    return true;
}

/* LETTERBOX ON (OVL1 0x2AEA70) */
_zanim_cmd_hdr *HUDLetterBoxCmdParseOn(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->on = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* LETTERBOX OFF (OVL1 0x2AE9F0) */
_zanim_cmd_hdr *HUDLetterBoxCmdParseOff(_zrdr *rdr)
{
    _hud_switch_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_hud_switch_cmd *)_calloc(1, sizeof(_hud_switch_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_hud_switch_cmd);
    cmd->hdr.timeless = 1;
    cmd->off = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* LETTERBOX ON / OFF: the bars brought in or taken away (OVL1 0x2AE970) */
bool HUDLetterBoxCmdTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _hud_switch_cmd *cmd = (_hud_switch_cmd *)hdr;

    if (!theHUD)
        return true;
    if (cmd->on)
        theHUD->m_letterbox.AddLetterbox();
    else if (cmd->off)
        theHUD->m_letterbox.RemoveLetterbox();
    return true;
}

/* TRIGGER_MISSION_CAM: the mission camera the command names played (OVL1 0x2AE7D0) */
struct _trigger_mission_cam_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char name;   // 0x4
};

/* TRIGGER_MISSION_CAM: the camera's name, the command's one string, kept among the current
 * anim's names (SOCOM 1's; OVL1 0x2AE820) */
_zanim_cmd_hdr *HUDTriggerMissionCamParse(_zrdr *rdr)
{
    _trigger_mission_cam_cmd *cmd = 0;

    if (!rdr)
        return 0;
    if (rdr->type == 4 && ZCount(rdr) && ZItem(rdr, 0)->type == 3) {
        CZAnim *anim = ZAnim.m_CurAnim;

        cmd = (_trigger_mission_cam_cmd *)_calloc(1, sizeof(_trigger_mission_cam_cmd), 0, 0);
        cmd->hdr.data_size = sizeof(_trigger_mission_cam_cmd);
        cmd->hdr.timeless = 1;
        cmd->name = anim->AddName(ZItem(rdr, 0)->String());
    }
    return (_zanim_cmd_hdr *)cmd;
}

void HUDTriggerMissionCamBegin(_zanim_cmd_hdr *hdr)
{
    CHUD *hud = theHUD;

    if (hud)
        hud->m_missioncams.PlayCamera(ZAnim.m_CurAnim->GetName(((_trigger_mission_cam_cmd *)hdr)->name));
}

/* TRIGGER_MISSION_CAM: done when no mission camera plays (OVL1 0x2AE760) */
bool HUDTriggerMissionCamTick(_zanim_cmd_hdr *hdr, float *dT)
{
    if (!theHUD)
        return true;
    CZHudMissionCams *cams = &theHUD->m_missioncams;

    if (cams && cams->IsPlaying())
        return false;
    return true;
}

/* a bare timeless command, the weapons timer clear (SOCOM II's, before the defuse timer's pair; OVL1
 * 0x2AE510) */
_zanim_cmd_hdr *HUDClearTimerParse(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* TRIGGER_POPUP (SOCOM II's, before the defuse timer's tick; the names ours): the popup the command
 * names, its one string kept among the current anim's names (OVL1 0x2AE3A0 the parser, the same code
 * as TRIGGER_MISSION_CAM's; 0x2AE340 the tick, a popup shown for the player when its flag at 0xE1 is
 * set) */
struct _trigger_popup_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char name;   // 0x4
};

void hudTriggerPopup(const char *name);

extern "C" _zanim_cmd_hdr *func_002AE3A0(_zrdr *rdr)
{
    _trigger_popup_cmd *cmd = 0;

    if (!rdr)
        return 0;
    if (rdr->type == 4 && ZCount(rdr) && ZItem(rdr, 0)->type == 3) {
        CZAnim *anim = ZAnim.m_CurAnim;

        cmd = (_trigger_popup_cmd *)_calloc(1, sizeof(_trigger_popup_cmd), 0, 0);
        cmd->hdr.data_size = sizeof(_trigger_popup_cmd);
        cmd->hdr.timeless = 1;
        cmd->name = anim->AddName(ZItem(rdr, 0)->String());
    }
    return (_zanim_cmd_hdr *)cmd;
}

extern "C" bool func_002AE340(_zanim_cmd_hdr *hdr, float *dT)
{
    CZSealBody *player = ftsGetPlayer();

    if (player && player->m_e1_4)
        hudTriggerPopup(ZAnim.m_CurAnim->GetName(((_trigger_popup_cmd *)hdr)->name));
    return true;
}

/* START_DEFUSE_TIMER (SOCOM 1's, SOCOM II's with a COUNT_UP flag; the tick 0x2AE580 hands both to
 * hudStartDefuseTimer(bool, float), a draft's): the time, a number (a thousandth by default), and the
 * flag (OVL1 0x2AE5B0; SOCOM 1's parser refused a bare command, SOCOM II's builds one on its
 * defaults) */
struct _start_defuse_timer_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char countup : 1;   // 0x4
    float time;   // 0x8
};

/* a number, an integer or a real (ours) */
inline bool IsNumber(_zrdr *z)
{
    return z->type == 1 || z->type == 2;
}

_zanim_cmd_hdr *HUDStartDefuseTimerParse(_zrdr *rdr)
{
    _start_defuse_timer_cmd *cmd;
    float time;
    bool countup;

    if (!rdr)
        return 0;
    time = 0.001f;
    countup = false;
    if (rdr->type == 4 && ZCount(rdr)) {
        if (IsNumber(ZItem(rdr, 0)))
            time = ZItem(rdr, 0)->Real();
        else
            return 0;
        if (zrdr_findtag(rdr, "COUNT_UP"))
            countup = true;
    }
    cmd = (_start_defuse_timer_cmd *)_calloc(1, sizeof(_start_defuse_timer_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_start_defuse_timer_cmd);
    cmd->hdr.timeless = 1;
    cmd->time = time;
    cmd->countup = countup;
    return (_zanim_cmd_hdr *)cmd;
}

/* START_DEFUSE_TIMER's tick: the HUD's defuse timer started on the command's flag and time (SOCOM 1's,
 * SOCOM II's with the flag; OVL1 0x2AE580) */
void hudStartDefuseTimer(bool countup, float time);

bool HUDStartDefuseTimerTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _start_defuse_timer_cmd *cmd = (_start_defuse_timer_cmd *)hdr;

    hudStartDefuseTimer(cmd->countup, cmd->time);
    return true;
}

/* the bare command's tick: the weapons' timer cleared (SOCOM II's, the name ours; OVL1 0x2AE4F0) */
bool HUDClearTimerTick(_zanim_cmd_hdr *hdr, float *dT)
{
    CZWeapon::ClearTimer(0);
    return true;
}

/* the auto-inline limit raised by two statements: MW inlines an out-of-class template member into an
 * inline function's out-of-line copy only under it, and the pod's reserve needs 17 -- so the pointer
 * vector's reserve 0x2B1600 that resize<CUIVariable> calls is the pod reserve inlined, as Open's own
 * reserve generated it first (the MSL headers round 16, include/vector's pointer __vector_imp) */
#pragma inline_max_auto_size(17)
