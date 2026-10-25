/* fts_mission.cpp -- the mission: CMission, the online game's state (CZNetGame), the ladders, the
 * HUD's ZAnim commands, the archive and the custom character types (SOCOM 1's fts_mission.cpp;
 * its static initialiser is SOCOM II's 0x400160). OVL1 0x2A3B90-0x2B218F, the template instances
 * closing it (the objective cameras' vector, the UI variables' vector) before fts_player.cpp's
 * CSpawners code. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include <vector>

#include "../zanim/zanim.h"
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
    unsigned char _dd[0xe4 - 0xdd];
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
    unsigned char m_1;   // 0x1: no fade to the mission's end while set (the name ours)
};

extern CZNetwork theNetwork;

/* a cell of an AI map (zentity.h's CAiMapLoc: only its word) */
class CAiMapLoc {
public:
    unsigned int m_word;
};

/* a compass anchor (zentity.h's CCompassAnchor: only what the nav points set) */
class CCompassAnchor {
public:
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
};

/* the AI maps' nav points: a cell and its name (SOCOM 1's, 24 bytes; the names ours) */
struct CAiNavPoint {
    CAiMapLoc m_loc;   // 0x0
    char m_name[20];   // 0x4
};

/* the AI's maps (OVL2's AiMaps: only the cell's centre and the nav points) */
class CAiMaps {
public:
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);

    unsigned char _0[0x48];
    std::vector<CAiNavPoint> m_navpoints;   // 0x48
};

extern CAiMaps AiMaps;

/* the mission (only what this unit touches, at SOCOM II's offsets) */
class CMission {
public:
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

    unsigned char _0[0xe0];
    int m_e0;   // 0xe0: cleared before the second unload (SOCOM II's; the name ours)
    unsigned char _e4[0xe8 - 0xe4];
    float m_e8;   // 0xe8: the mission's own clock, in seconds (the name ours)
    unsigned char _ec[0xfc - 0xec];
    unsigned char m_DelayEndMission : 1;   // 0xfc (SOCOM 1's bytes at 0x70, 0x71)
    unsigned char m_FadingToEndMission : 1;
    int m_fade_counter;   // 0x100 (SOCOM 1's at 0x74)
    float m_next_fade_time;   // 0x104 (SOCOM 1's at 0x78)
    unsigned char _108[0x118 - 0x108];
    std::vector<CCompassAnchor *> m_anchors;   // 0x118: the nav points' anchors (SOCOM 1's at 0x8c)
    unsigned char _124[0x1808 - 0x124];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x1860 - 0x180c];
    float m_music_hold;   // 0x1860: how long the music's raised intensity holds (SOCOM II's; the name ours)
};

extern CMission theMission;
extern "C" char *strncpy(char *dst, const char *src, unsigned int n);

/* the world, the entities, the AI events, the scheduler and the sound (their units'; called by their
 * map names) */
class CEntity {
public:
    static void RemoveAll();
};
class CAiEvent {
public:
    static void Close();
};
extern CSched_Manager zTaskScheduler;
extern "C" void ClearFreeList__14CSched_ManagerFb(CSched_Manager *sched, bool all);
extern "C" bool AddTask__14CSched_ManagerFPCcPFfPv_bfPv(CSched_Manager *sched, const char *name,
                                                         bool (*func)(float dT, void *data), float priority, void *data);

/* the mission completed (SOCOM 1's name, at its call; OVL1 0x2AA910, not written yet) */
void _OnMissionComplete();

extern "C" {
void Flash__4CHUDFfRC6CPnt3Dfff(CHUD *hud, float time, const CPnt3D *colour, float in, float hold, float out);
void Reset__14CFlashFXVectorFv(void *flashes);
void ResetDeathCam__10CAppCameraFv(void *camera);
void Push__5CGameFP10CGameStateUi(void *game, void *state, unsigned int arg);
void func_003AFD50(int a0, int a1, int a2, int a3, int a4);   /* (the screen cleared: SOCOM 1's CVideo::Clear) */
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
        func_003AFD50(1, 0, 0, 0, 0);
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

/* the task's callback (SOCOM 1's fadeToEndMissionTick, the same jump; OVL1 0x2A98A0) */
bool CMission::fadeToEndMissionTick(float dT, void *data)
{
    return ((CMission *)data)->FadeToEndMissionTick(dT);
}

/* the fade to the mission's end begun, or brought nearer (SOCOM 1's, its two flags bits now;
 * OVL1 0x2A97C0) */
void CMission::FadeToEndMission(float time, int counter)
{
    if (theNetwork.m_1)
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

    m_e0 = 0;
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
    if (theNetwork.m_1 && m_pNetGame)
        return 0.001f * m_pNetGame->m_round_clock;
    return (int)m_e8;
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

/* a character type (zchar's CCharacterType: only what the custom types touch) */
class CCharacterType {
public:
    static CCharacterType *RdrFetch_CharacterType(const char *name);
    static const CCharacterType *GetByNameMod(const char *name);
    static CCharacterType *Create(const char *name, const CCharacterType *base, bool temporary);

    char *m_name;   // 0x0
    unsigned char _4[0x300 - 0x4];
    unsigned char m_custom;   // 0x300: a custom type (the name ours: stored with li, so no bool)
};

const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
/* a custom type's weapons' UI variables (SOCOM 1's; OVL1 0x2AF190, not written yet) */
void UpdateCharWeaponUiVars(int i);

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

extern "C" void func_0053B070(void *weapons);   /* (OVL2: a custom type's weapons, at 0x2E8) */

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
            func_0053B070((char *)custom + 0x2e8);
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

    int m_count;
    char m_banks[1][10];
};

extern "C" {
extern MissionBankList data_00440B80;
void *func_00361BC0();                                  /* (the sound system) */
void func_003A3900(void *snd, const char *bank, void *arg);   /* (a bank loaded) */
void *func_003A4460();
}

extern "C" char *strcpy(char *dst, const char *src);

/* the extra string tables the mission's rdr lists, by name (SOCOM II's; PreOpen calls it; OVL1
 * 0x2AF410).
 * OPEN, 0x10C against 0x140: the target keeps two empty if/else tests on the count -- `0 < count`
 * before it is cleared, `count + 1 < count` before each increment -- both arms empty, the else arm
 * a jump placed after the epilogue: a fixed array's resize(n) whose element loops vanish for char
 * names. MW folds every empty arm tried away: empty inline helpers (plain or template), empty for
 * loops. The rest (the list's Count/Item/String, the name's address recomputed each pass) is
 * written as the target reads */
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
    func_003A3900(func_00361BC0(), bank, func_003A4460());
    int n = data_00440B80.m_count;

    for (int i = 0; i < n; i++)
        func_003A3900(func_00361BC0(), data_00440B80.GetBank(i), func_003A4460());
}

/* the player's SEAL and its controller (only the controller's state byte) */
class CSealCtrl {
public:
    unsigned char _0[0x221];
    unsigned char m_221;   // 0x221
};

class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_control;   // 0xc0
};

CZSealBody *ftsGetPlayer();

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
    if (rdr->type == 4 && rdr->Count() && rdr->Item(0)->type == 3) {
        CZAnim *anim = ZAnim.m_CurAnim;

        cmd = (_trigger_mission_cam_cmd *)_calloc(1, sizeof(_trigger_mission_cam_cmd), 0, 0);
        cmd->hdr.data_size = sizeof(_trigger_mission_cam_cmd);
        cmd->hdr.timeless = 1;
        cmd->name = anim->AddName(rdr->Item(0)->String());
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

/* a bare timeless command (SOCOM II's, between the defuse timer's tick and parser; OVL1
 * 0x2AE510) */
extern "C" _zanim_cmd_hdr *func_002AE510(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}
