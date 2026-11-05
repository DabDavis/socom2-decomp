/* fts_mission.cpp -- the mission: CMission, the online game's state (CZNetGame), the ladders, the
 * HUD's ZAnim commands, the archive and the custom character types (SOCOM 1's fts_mission.cpp;
 * its static initialiser is SOCOM II's 0x400160). OVL1 0x2A3B90-0x2B218F, the template instances
 * closing it (the objective cameras' vector, the UI variables' vector) before fts_player.cpp's
 * CSpawners code. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include <iostream>
#include <new>
#include <vector>

/* the UI variables first: zui.h's _zrdr is the one the unit sees (zanim.h leaves its own out
 * after zui.h's), as the original's one zrdr.h served both (the states lane, round 22) */
#include "../zui/zui.h"
#include "../zanim/zanim.h"
#include "../zsav/zsav_lobby.h"
#include "../zsys/zsys.h"
#include "../../Apps/FTS/hud_letterbox.h"
#include "../../Apps/FTS/hud_missioncams.h"

/* the HUD's map (hud_newhudmap.h's CZNewHudMap, 0x830 bytes: only its objective list's selection,
 * at SOCOM 1's m_objectivelist.m_cur_selected_objective -- Close clears it with the objectives) */
class CMissionObjectiveCam;
class CZNewHudMap {
public:
    unsigned char _0[0x7fc];
    CMissionObjectiveCam *m_cur_selected_objective;   // 0x7fc
};

/* the HUD: only what the commands touch (SOCOM 1's CHUD at SOCOM II's offsets; its On and Off
 * inline here, out of line in SOCOM 1) */
class CHUD {
public:
    void On() { m_ison = true; }
    void Off() { m_ison = false; }
    void ReInit_Uninit();   /* 0x1FB790 */

    unsigned char _0[0x11c90];
    CZNewHudMap *m_newmap;   // 0x11c90 (SOCOM 1's map embedded, at 0xe370)
    unsigned char _11c94[0x15ef4 - 0x11c94];
    float m_nvg_noise;   // 0x15ef4: SOCOM II's night vision noise, 0 to 128 (NVG_NOISE sets it; the name ours)
    unsigned char _15ef8[0x1a100 - 0x15ef8];
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
    float UnInitialize();   /* (no value returned: see the definition) */
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
    static void DeleteAnchor(CCompassAnchor *anchor);   /* 0x2A27E0 */
    void Close();   /* 0x2A2AC0 */
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
    void Close();   /* OVL2 0x52E780 */
    void Reset();   /* OVL2 0x52E720 */
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

/* the mission's objective anchors (SOCOM 1's CObjAnchorVec: a class over the vector -- ~CMission
 * tests `this` once more above the vector's chain) */
class CObjAnchorVec : public std::vector<SObjectiveCam_Anchor> {
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

/* a mission camera (fts_objectivecam.cpp's CMissionCam, SOCOM 1's DWARF 0x1C bytes: out of line) and
 * the mission's lists of them (SOCOM 1's DWARF: each a list and nothing more) */
class CMissionCam {
public:
    CMissionCam();   /* 0x2C78E0 */
    ~CMissionCam();   /* 0x2C7810 */
    void Set(const char *identifier, const char *name, const char *desc0, const char *desc1,
             const char *desc2, const char *anim, const char *music);   /* 0x2C7640 */

    unsigned char _0[0x1c];
};

class CMissionCamList : public std::list<CMissionCam *> {
};

class CMissionObjectiveCamList : public std::list<CMissionObjectiveCam *> {
public:
    void Reset();   /* 0x2C6F40 */
    void ClearValves();   /* 0x2C7000 */
};

/* a point of a camera path (the names ours): where, which way, and its name -- 0x98 bytes, the
 * stride of the mission's path at 0x437DF0 (theMission's +0x1910: Close clears it, ~CMission tears
 * it down; its only other user, 0x295260, steps through it) */
struct SCamPathPoint {
    CPnt3D m_pos;   // 0x0
    CPnt3D m_dir;   // 0xc
    char m_name[0x80];   // 0x18
};

class CCamPath : public std::vector<SCamPathPoint> {
public:
    int GetPoint(int index, int step, CPnt3D *pos, CPnt3D *dir, const char **name);
};

/* ---- the player's message queue (theMission's at 0x11B0, fts_state.h's m_netQueue: the chat's and
 * the core state's lines, shown by the menu state and the HUD; SOCOM II's, the names ours) ---- */

/* a message's type (fts_state.h's: a byte enum; the chat's CHAT_COLOR is the same three values) */
enum MSGTYPE {
    MSGTYPE_PLAYER = 0,
    MSGTYPE_SYSTEM = 1,
    MSGTYPE_ALERT = 5
};

extern "C" {
unsigned int strlen(const char *s);
void *memset(void *p, int c, unsigned int n);
}

/* a queued message: its type, its text and its sender, 0xA1 bytes (st_menu.cpp's CQueuedMessage) */
class CQueuedMessage {
public:
    CQueuedMessage();
    /* the text kept to 127 characters, an empty one (a single character too) left as it was, none
     * given made "[empty message]" */
    void SetText(const char *text)
    {
        if (text && strlen(text) < 128)
            strcpy(m_text, text);
        else if (text && strlen(text) > 1) {
            strncpy(m_text, text, 126);
            m_text[127] = 0;
        } else if (!text)
            strcpy(m_text, "[empty message]");
    }
    MSGTYPE GetType() { return m_type; }
    void SetType(MSGTYPE type) { m_type = type; }
    /* the sender's name, or none */
    void SetName(const char *name)
    {
        if (name && *name)
            strncpy(m_name, name, 32);
        else
            memset(m_name, 0, 32);
    }

    MSGTYPE m_type;   // 0x0
    char m_text[128];   // 0x1
    char m_name[32];   // 0x81
};

/* ten messages in a ring: taken at the head, added at the tail, one slot always left free */
class CMessageQueue {
public:
    bool HasMessages();
    void Dequeue(CQueuedMessage *msg);
    void Enqueue(const char *text, MSGTYPE type, const char *name);

    CMessageQueue();
    ~CMessageQueue();

    int m_head;   // 0x0
    int m_tail;   // 0x4
    CQueuedMessage m_msgs[10];   // 0x8
};


/* emptied, every text made "NULL MSG QUEUE(dtor)" -- auto-inlined into ~CMission, its own copy
 * stripped: NOT `inline`, or its literal is pooled as the inline's and SetText's null tests on it
 * are kept (build/scratch/states/p15.cpp) */
CMessageQueue::~CMessageQueue()
{
    m_head = 0;
    m_tail = 0;
    for (unsigned int i = 0; i < 10; i++)
        m_msgs[i].SetText("NULL MSG QUEUE(dtor)");
}

/* built empty, every text "NULL MSG QUEUE(ctor)" after MW's own construction of the ten messages --
 * the destructor's twin, auto-inlined into CMission() and its own copy stripped as well */
CMessageQueue::CMessageQueue()
{
    m_head = 0;
    m_tail = 0;
    for (unsigned int i = 0; i < 10; i++)
        m_msgs[i].SetText("NULL MSG QUEUE(ctor)");
}

/* the HUD's message lists (fts_message.cpp's C2DMessage_Q, 0x4E0 bytes) and the chat line's editor
 * (fts_missionlist.cpp's CMsgEditor, 0x150 bytes): only their construction and teardown */
class C2DMessage_Q {
public:
    C2DMessage_Q();   /* 0x2B7610 */
    ~C2DMessage_Q();   /* 0x2B6EE0 */
    void Clear_Messages();   /* 0x2B6DE0 */
    void ResetScroll();

    unsigned char _0[0x438];
    /* the scroll: the top line, the move's time and the top it moves to (SOCOM 1's m_cur_top,
     * m_moveTime and m_target_top, 0x15C-0x164 there) */
    int m_cur_top;   // 0x438
    float m_moveTime;   // 0x43c
    float m_target_top;   // 0x440
    unsigned char _444[0x4e0 - 0x444];
};

class CMsgEditor {
public:
    CMsgEditor();   /* 0x2CA2D0 */
    ~CMsgEditor();   /* 0x2CA240 */

    unsigned char _0[0x150];
};

class CVehicleRdr;

/* the single-player career's stats (theMission's two, at 0x191C and 0x1950: CMission() clears both
 * through the constructor -- Clear's stores, m_kills before m_18, twice; SOCOM II's, the names ours) */
class CCareerStats {
public:
    CCareerStats() { Clear(); }
    void Clear();
    void SetUIVars();

    int m_0;   // 0x0
    int m_4;   // 0x4
    int m_8;   // 0x8
    int m_c;   // 0xc
    int m_10;   // 0x10
    int m_14;   // 0x14
    int m_18;   // 0x18
    int m_kills;   // 0x1c: the valve sp_career_kills mirrors it (the name ours)
    int m_20;   // 0x20
    int m_24;   // 0x24
    int m_28;   // 0x28
    int m_2c;   // 0x2c
    int m_30;   // 0x30
};

/* the mission (only what this unit touches, at SOCOM II's offsets) */
class CMission : public CSaveModule {
public:
    /* the mission's AI parameters (SOCOM 1's CMission::m_ai_params, an AI_PARAMS of 0x24 bytes there:
     * SOCOM II's has six more words, light_scale to nosnooze_limit, by Read's "ai_params" tags). Nested
     * here: the character types' AI_PARAMS is the top-level one, and this unit uses both. Its defaults
     * are an out-of-line member (0x2AB360) the constructor and Read share -- SOCOM 1's Read and
     * constructor expand the same seven stores in place; the reset's name ours */
    class AI_PARAMS {
    public:
        AI_PARAMS()
        {
            Reset();
            player_grid_spacing = 15.0f;
            player_grid_count = 0;
        }
        void Reset();

        float weather_factor;   // 0x0
        unsigned int fireteam_command;   // 0x4
        float recycle_range;   // 0x8
        float recycle_time;   // 0xc
        float respawn_time;   // 0x10
        float respawn_fade;   // 0x14
        float respawn_range;   // 0x18: squared as read
        float light_scale;   // 0x1c (SOCOM II's)
        float light_inside;   // 0x20
        float shadow_scale;   // 0x24
        float shadow_inside;   // 0x28
        float stealth_noise;   // 0x2c
        float ambient_noise;   // 0x30
        unsigned int active_limit;   // 0x34
        unsigned int nosnooze_limit;   // 0x38
        float player_grid_spacing;   // 0x3c (SOCOM 1's at 0x1c)
        unsigned int player_grid_count;   // 0x40
    };   // 0x44 bytes

    CMission();    /* 0x2ADBC0 */
    ~CMission();   /* 0x2AD940: its vtable's slot 2 (0x406290) */
    void Serialize(int flags);   /* 0x2A6340: slot 3 */
    void UnSerialize(const char *name, CRdrFile *file, int flags);   /* 0x2A54E0: slot 4 */
    bool Open(const char *name);
    static bool tick(float dT, void *data);   /* 0x2AAB00 */
    bool ZAnimLoad();
    bool LoadMissionCams(CRdrFile *rdr);
    bool LoadObjectiveCams(CRdrFile *rdr);   /* 0x2A4D30 */
    void Sample_ExtractionLoc();   /* 0x2A42B0 */
    void Close();   /* 0x2AB3F0 */
    void InitalizeTeamValves();   /* 0x2B2840 */
    void CloseVehicleRdr();   /* 0x2B3430 */
    void RefreshTeamValves();   /* 0x2B23A0 */
    bool LoadVehicles();   /* 0x2B2E50 (fts_player.cpp's) */
    void CreateCompassNavPoints();
    CZNetGame *CreateNetGame();
    bool ReOpen_Unload2();
    bool ReOpen_Load();
    void FadeToEndMission(float time, int counter);
    bool FadeToEndMissionTick(float dT);
    static bool fadeToEndMissionTick(float dT, void *data);
    static void RecoverFromNetError(int error);
    bool _RecoverFromNetError(int error);
    void SetDelayEndMission(bool b) { m_DelayEndMission = b; }
    void SetFadingToEndMission(bool b) { m_FadingToEndMission = b; }
    void SetFc2(bool b) { m_fc_2 = b; }
    void SetFc3(unsigned int v) { m_fc_3 = v; }
    bool ReOpen_Unload1();
    unsigned int GetTimeLeft();
    void EndMission();
    void CreateLadderTable();

    MISSION_STATE m_state;   // 0x18 (SOCOM 1's; the save module's base ahead)
    char *m_database;   // 0x1c (SOCOM 1's)
    unsigned char m_20;   // 0x20: SOCOM II's, Close clears it last (the name ours)
    unsigned char _21[0x60 - 0x21];
    char m_db_string[0x80];   // 0x60 (SOCOM 1's at 0x20)
    float m_timer;   // 0xe0: the mission's time in seconds, cleared by Open and before the second unload (SOCOM 1's float at 0x60)
    unsigned char _e4[0xe8 - 0xe4];
    float m_e8;   // 0xe8: the mission's own clock, in seconds (SOCOM 1's m_winlosstimer at 0x68; the name ours)
    float m_winlossinc;   // 0xec (SOCOM 1's at 0x6c: the constructor sets both -1)
    unsigned char _f0[0xf4 - 0xf0];
    float m_elevation_max;   // 0xf4: Read's "elevation" max (127 by default; SOCOM II's, the names ours)
    float m_elevation_min;   // 0xf8: its min (-127)
    unsigned char m_DelayEndMission : 1;   // 0xfc (SOCOM 1's bytes at 0x70, 0x71)
    unsigned char m_FadingToEndMission : 1;
    unsigned char m_fc_2 : 1;   // SOCOM II's (a reload's unload clears it through a bool setter; the name ours)
    unsigned int m_fc_3 : 29;   // SOCOM II's: the word's rest, the constructor zeroes it through a setter (word-wide: lw/sw)
    int m_fade_counter;   // 0x100 (SOCOM 1's at 0x74)
    float m_next_fade_time;   // 0x104 (SOCOM 1's at 0x78)
    float m_wait_before_falling;   // 0x108 (SOCOM 1's at 0x7c)
    std::vector<CCompassAnchor *> m_anchor_extraction;   // 0x10c (SOCOM 1's at 0x80)
    std::vector<CCompassAnchor *> m_anchors;   // 0x118: the nav points' anchors (SOCOM 1's at 0x8c)
    CCompassAnchor *m_whoistalking;   // 0x124: SOCOM II's (Open makes it "WhoIsTalking"; the name ours)
    CObjAnchorVec m_objanchor_vec;   // 0x128 (SOCOM 1's at 0x98)
    CMissionObjectiveCam *m_prev_selected_objcam;   // 0x134 (SOCOM 1's at 0xa4)
    float m_objectives_timer;   // 0x138 (SOCOM 1's at 0xa8)
    unsigned char m_ignore_objectives;   // 0x13c (SOCOM 1's at 0xac)
    unsigned int m_load_ai;   // 0x140 (SOCOM 1's at 0xb0)
    AI_PARAMS m_ai_params;   // 0x144 (SOCOM 1's at 0xb4)
    float m_init_aim_pitch;   // 0x188: Read's "init_aim_pitch" (SOCOM II's)
    float m_seefarther_binocs;   // 0x18c (SOCOM 1's at 0xd8)
    float m_seefarther_weapzoom;   // 0x190 (SOCOM 1's at 0xdc)
    unsigned char _194[0x1a4 - 0x194];
    CValve *m_valve_complete;   // 0x1a4 (SOCOM 1's at 0xe0)
    CValve *m_valve_failure;   // 0x1a8
    CValve *m_valve_abort;   // 0x1ac
    CValve *m_valve_timeout;   // 0x1b0
    CValve *m_valve_nofade;   // 0x1b4
    unsigned char _1b8[0x1c0 - 0x1b8];
    C2DMessage_Q m_messages;   // 0x1c0 (SOCOM 1's three at 0xf0, ~CMission tears them down)
    C2DMessage_Q m_messages2;   // 0x6a0
    C2DMessage_Q m_messages3;   // 0xb80
    CMsgEditor m_messages4;   // 0x1060: the chat line (hud_main.cpp's)
    CMessageQueue m_netQueue;   // 0x11b0: the player's message ring
    class CLadderList *m_ladders;   // 0x1804 (SOCOM 1's at 0x580)
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char m_chars_loaded;   // 0x180c (SOCOM 1's at 0x588)
    unsigned char _180d[0x1814 - 0x180d];
    CVehicleRdr *m_vehicle_rdr;   // 0x1814 (SOCOM 1's at 0x590; fts_player.cpp's name)
    CRdrFile *m_ordersRdr;   // 0x1818 (SOCOM 1's at 0x594)
    CRdrFile *m_materialsRdr;   // 0x181c (SOCOM 1's at 0x598)
    CRdrFile *m_decalsRdr;   // 0x1820 (SOCOM 1's at 0x59c)
    CRdrFile *m_fontsRdr;   // 0x1824 (SOCOM 1's at 0x5a0)
    _zrdr *m_subtitles;   // 0x1828: the films' subtitles (fts_state.h's; the constructor clears it last)
    CRdrFile *m_messagesRdr;   // 0x182c (SOCOM 1's at 0x5a4)
    CRdrFile *m_subtitlesRdr;   // 0x1830 (SOCOM 1's at 0x5a8)
    CRdrFile *m_smallmessagesRdr;   // 0x1834 (SOCOM 1's at 0x5ac)
    CRdrFile *m_1838;   // 0x1838: SOCOM II's
    CSaferStr m_183c;   // 0x183c: SOCOM 1's m_strLoadingScrLib (at 0x5b0): "ui/assetlib/junk_name" by default
    CMissionCamList m_missioncams;   // 0x1848 (SOCOM 1's at 0x5b4)
    CMissionObjectiveCamList m_objectivecams;   // 0x1854 (SOCOM 1's at 0x5c0)
    float m_music_hold;   // 0x1860: how long the music's raised intensity holds (SOCOM 1's m_fighttimer at 0x5cc; the name ours)
    float m_satcheltimer;   // 0x1864 (SOCOM 1's at 0x5d0: 10 by default, Read's "SATCHEL_TIMER")
    CValve *m_team_valves[32];   // 0x1868 (SOCOM 1's m_teamValve at 0x5d4; fts_player.cpp's name)
    CValve *m_player_valve_alive;   // 0x18e8 (SOCOM 1's m_player_valve at 0x654; fts_player.cpp's name)
    zdb::CNode *m_sealbombshack;   // 0x18ec (SOCOM 1's at 0x658)
    zdb::CNode *m_terroristbombshack;   // 0x18f0 (SOCOM 1's at 0x65c)
    unsigned char m_e3_team;   // 0x18f4 (SOCOM 1's E3_TEAMS_ENUM at 0x660, a byte)
    unsigned char _18f5[0x1910 - 0x18f5];
    CCamPath m_campath;   // 0x1910: SOCOM II's camera path (the name ours)
    CCareerStats m_careerstats0;   // 0x191c (SOCOM II's; the names ours)
    CCareerStats m_careerstats;   // 0x1950: the UI's CLEAR_CAREER_STATS and SET_CAREER_STATS_UI_VARS ones

    static char m_weaponmodelpostfix[30];   // 0x436430 (SOCOM 1's static, 0x4BC590 there)
};

/* ---- the unit's statics with constructors, in the order its initialiser (0x400160) builds them,
 * after <iostream>'s two (SOCOM 1's __sinit_fts_mission.cpp builds the same, but for the lobby's
 * save manager, SOCOM II's, and the extra string tables below) ---- */
char CMission::m_weaponmodelpostfix[30];    // 0x436430 (SOCOM 1's, before theMemCard there too)
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
 * objective cameras' vector): resize<CUIVariable> 0x2B1390 inlines CUIVariable() one level --
 * _zrdr() 0x2B17E0, UIVarMixin() 0x2B17C0, vector<UIVarMixin *>() 0x2B17A0 and the watchers' reserve
 * 0x2B1600 called, the last the pointer vector's reserve forwarder with the pod's reserve inlined --
 * byte-exact under the unit's closing `#pragma inline_max_auto_size(17)` with Open's reserve of a
 * pointer vector generating the pod's first (the MSL headers round 16, the states round 24).
 * reserve<SObjectiveCam_Anchor> 0x2B00E0 (Open's m_objanchor_vec.reserve(16) brings it) inlines its
 * temporary's teardown (`sw zero`, the free) only with the deleter's clear and destructor generated
 * first: CMission::Close 0x2AB3F0 does that (its FreeVector(m_objanchor_vec) copies 0x2ABA00-0x2ABAB0,
 * m_ladders' clear 0x2B1290 called from its own code), and with Close written both deleter
 * destructors (0x2B1220, 0x2B12A0) inline their clears as the target's (the states round 25).
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
class CQuat;
class CEntity {
public:
    /* zentity.h's virtual functions up to OnMissionEnd (+0x50), for the mission's end */
    virtual ~CEntity();                                         /* +0x08 */
    virtual const CMatrix *GetPosW() const;
    virtual void UpdatePosW();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PostPreTick(float dT);                         /* +0x1C */
    virtual void PrePostTick(float dT);
    virtual void PostTick(float dT);
    virtual void ComputeNextPosition(float dT);
    virtual void UpdateNextPosition();                          /* +0x2C */
    virtual CPnt3D *GetTargetPosW(CPnt3D *out);
    virtual CPnt3D *GetHeadPosW(CPnt3D *out);
    virtual CPnt3D *GetNeckPosW(CPnt3D *out);
    virtual void TeleportTo(const CPnt3D *pos, const CQuat *quat);   /* +0x3C */
    virtual void TeleportTo(const CMatrix *m);
    virtual void ResetToInitialState();
    virtual void UpdateAiMapLoc();                              /* +0x48 */
    virtual void OnMissionStart();
    virtual void OnMissionEnd();                                /* +0x50 */

    static void RemoveAll();
    /* the entities, const (zentity.h's) */
    static const CEntityList &GetEntityList() { return m_list; }

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

/* the mission completed (SOCOM 1's name, at its call; OVL1 0x2AA910, below) */
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
extern class CAppCamera *appCamera;
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

/* the player's SEAL and its controller (only the controller's state byte and its AI -- zseal.h's
 * CSealCtrl::GetSealCtrlAi, the vtable's +0x38 --, the network player's creation and the target
 * list the danger walks) */
class CSealCtrlAi;

class CSealCtrl {
public:
    void StopRumbler();   /* OVL2 0x592AC0 */
    virtual ~CSealCtrl();   /* +0x08 (zentity.h's CEntityCtrl's, then zseal.h's CSealCtrl's) */
    virtual void _slot0c();
    virtual void _slot10();
    virtual void _slot14();
    virtual void _slot18();
    virtual void _slot1c();
    virtual void _slot20();
    virtual void _slot24();
    virtual void _slot28();
    virtual void _slot2c();
    virtual void _slot30();
    virtual void _slot34();
    virtual CSealCtrlAi *GetSealCtrlAi();   /* +0x38 */

    unsigned char _4[0x221 - 0x4];
    unsigned char m_221;   // 0x221
};

/* what an entity knows of another (zentity.h's CTarget: only the entity and its range squared) */
class CZSealBody;

struct CTarget {
    CZSealBody *m_ent;   // 0x0 (an entity: zentity.h's CEntity)
    unsigned char _4[0x10 - 0x4];
    float m_distSq;   // 0x10
    unsigned char _14[0x30 - 0x14];
};

class CTargetList : public std::vector<CTarget> {
};

class CZSealBody {
public:
    static void NetworkPlayerCreate();   /* OVL2 0x5442C0 */
    static void UninitDatabaseHandles();   /* OVL2 0x5A3BB0 */
    static void InitDatabaseHandles();   /* OVL2 0x5A4CE0 */
    static void ReInitDatabaseHandles();   /* OVL2 0x5A49F0 */
    static void InitDecalHandles();   /* OVL2 0x5A4CF0 */
    static void Uninit_ReInitDatabaseHandles();   /* OVL2 0x5A3B50 (SOCOM 1's) */

    /* whether two entities share a team (ours: zentity.h's IsEnemy turned round) */
    bool IsFriend(const CZSealBody *e) const { return (e->m_team & m_team) != 0; }
    /* the AI driving it, if any (zseal.h's) */
    CSealCtrlAi *GetSealCtrlAi() { return m_control->GetSealCtrlAi(); }

    unsigned char _0[0x4];
    unsigned int m_id;   // 0x4 (zentity.h's CEntity)
    unsigned char _8[0x10 - 0x8];
    unsigned char m_type;   // 0x10 (zentity.h's: 2 a SEAL)
    unsigned char _11[0x1c - 0x11];
    CPnt3D m_posW;   // 0x1c
    unsigned char _28[0xc0 - 0x28];
    CSealCtrl *m_control;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0xd0 - 0xcc];
    CTargetList m_targets;   // 0xd0
    unsigned char _dc[0xe1 - 0xdc];
    unsigned char m_e1_0 : 4;   // 0xe1
    unsigned char m_e1_4 : 1;   // 0xe1, bit 4: zentity.h's m_isAlive (TRIGGER_POPUP's test too)
};

CZSealBody *ftsGetPlayer();

/* the AI's target (ai_target.cpp's CAiTarget: only its entity) */
struct CAiTarget {
    CZSealBody *m_entity;   // 0x0
};

/* the SEALs' AI controller (zseal.h's CSealCtrlAi: its database-handle statics, SOCOM 1's, and what
 * the danger reads: the alertness, the asleep and snoozing bits, the alert floor, the target) */
class CSealCtrlAi {
public:
    static void ReleaseDatabaseHandles();   /* OVL2 0x574BD0 */
    static void AcquireDatabaseHandles();   /* OVL2 0x574C30 */

    unsigned char _0[0xb4];
    float m_alert;   // 0xb4
    unsigned char _b8[0x264 - 0xb8];
    unsigned int m_aibits0 : 8;   // 0x264
    unsigned int m_aibit8 : 1;   // bits 8 and 9: SOCOM 1's m_sleeping and m_snoozing
    unsigned int m_aibit9 : 1;
    unsigned int m_bits10 : 22;
    unsigned char _268[0x5d0 - 0x268];
    unsigned char m_alert_floor;   // 0x5d0: the alert band it never drops under (the name ours)
    unsigned char _5d1[0x67c - 0x5d1];
    CAiTarget m_target;   // 0x67c
    unsigned char _680[0x6a8 - 0x680];
    unsigned int m_6a8;   // 0x6a8: the target's flags (bit 9: held)
};

namespace ai {
template <class T>
const T &_max(const T &a, const T &b)
{
    return a >= b ? a : b;
}
}

extern "C" {
bool func_0060A850(CAiTarget *target);   /* (OVL2: the target valid) */
unsigned char func_00560AC0(float alert);   /* (OVL2: an alertness's band) */
extern float data_003DE608;   /* (the danger of a SEAL hunting the player) */
extern float data_003DE610;   /* (the danger of a SEAL near) */
}

/* the entity as a SEAL, if it is one; an AI awake; its target held (seal.cpp's) */
inline CZSealBody *AsSeal(CZSealBody *e)
{
    return e && e->m_type == 2 ? e : 0;
}

inline bool IsAwake(CSealCtrlAi *ai)
{
    return !(ai->m_aibit9 || ai->m_aibit8);
}

inline bool TargetHeld(CSealCtrlAi *ai)
{
    return func_0060A850(&ai->m_target) && (ai->m_6a8 & 0x200);
}

/* the danger around a SEAL (SOCOM II's; OVL1 0x2A4A10, the music's intensity -- 0x2A4920 asks it of
 * the player with 2 and 350): every live enemy SEAL it knows of whose AI is awake -- one hunting it
 * counts 1.5, one holding another target 0.75 when it or that target is in range, one holding none
 * 0.75 in range and alert to the band asked for or more. The target's shape: the range squared in
 * place (the parameter moved to $f21 first), the list walked through a reference (its address kept
 * for the end's re-reads), the team test a positive inline used as a plain truth test with the SEAL
 * as `this` (`!seal->IsEnemy(self)` keeps the bool's sltu/xori; `self->...` loses the hoisted
 * address of self's team), the AI through CZSealBody's inline (its result the inline's temporary,
 * numbered after the band's: ai s2, the band s3), the band an int (an unsigned char re-narrows it
 * with andi before the compare); `_max<Uc>` (0x2AFC40) called out of line, its instance in the tail */
extern "C" float func_002A4A10(CZSealBody *self, unsigned char level, float range)
{
    CTargetList &targets = self->m_targets;
    float danger = 0.0f;

    range *= range;
    for (CTargetList::iterator t = targets.begin(); t != targets.end(); ++t) {
        CZSealBody *seal = AsSeal(t->m_ent);

        if (!seal || !seal->m_e1_4 || seal->IsFriend(self))
            continue;
        CSealCtrlAi *ai = seal->GetSealCtrlAi();

        if (!ai || !IsAwake(ai))
            continue;
        float distSq = t->m_distSq;
        unsigned char band = func_00560AC0(ai->m_alert);
        int alert = ai::_max(band, ai->m_alert_floor);

        if (TargetHeld(ai)) {
            CZSealBody *target = ai->m_target.m_entity;

            if (target == self)
                danger += data_003DE608;
            else if (distSq < range)
                danger += data_003DE610;
            else {
                CPnt3D d;

                target->m_posW.Sub(&self->m_posW, &d);
                if (d.x * d.x + d.y * d.y + d.z * d.z < range)
                    danger += data_003DE610;
            }
        } else if (distSq < range && alert >= level)
            danger += data_003DE610;
    }
    return danger;
}

extern "C" {
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

/* ---- the single-player career's stats (theMission's at 0x1950, the UI's CLEAR_CAREER_STATS and
 * SET_CAREER_STATS_UI_VARS commands; SOCOM II's, the names ours) ---- */

/* the rdr editor (zrdr.h's CRdrEditor : CRdrFile : _zrdr, 0x4C bytes: an rdr built in memory -- only
 * what the stats call) */
class CRdrEditor : public _zrdr {
public:
    CRdrEditor();   /* 0x333AE0 */
    ~CRdrEditor();   /* 0x333A20 */
    bool push(const char *tag);   /* 0x3339B0 */
    bool pop();   /* 0x333770 */
    bool addstringnode(const char *s);   /* 0x333610 */

    unsigned char _8[0x4c - 0x8];
};

extern "C" int sprintf(char *buf, const char *fmt, ...);

/* the stats made the UI variable SPCareerStats, a list of their thirteen numbers as strings, and
 * the kills the valve sp_career_kills (OVL1 0x2A3BC0) */
void CCareerStats::SetUIVars()
{
    CRdrEditor rdr;
    char s0[16], s1[16], s2[16], s3[16], s4[16], s5[16], s6[16], s7[16], s8[16], s9[16], s10[16], s11[16], s12[16];

    rdr.push(0);
    sprintf(s0, "%d", m_0);
    rdr.addstringnode(s0);
    sprintf(s1, "%d", m_4);
    rdr.addstringnode(s1);
    sprintf(s2, "%d", m_8);
    rdr.addstringnode(s2);
    sprintf(s3, "%d", m_c);
    rdr.addstringnode(s3);
    sprintf(s4, "%d", m_10);
    rdr.addstringnode(s4);
    sprintf(s5, "%d", m_14);
    rdr.addstringnode(s5);
    sprintf(s6, "%d", m_kills);
    rdr.addstringnode(s6);
    sprintf(s7, "%d", m_18);
    rdr.addstringnode(s7);
    sprintf(s8, "%d", m_2c);
    rdr.addstringnode(s8);
    sprintf(s9, "%d", m_30);
    rdr.addstringnode(s9);
    sprintf(s10, "%d", m_24);
    rdr.addstringnode(s10);
    sprintf(s11, "%d", m_28);
    rdr.addstringnode(s11);
    sprintf(s12, "%d", m_20);
    rdr.addstringnode(s12);
    rdr.pop();
    theUIVarManager.Add("SPCareerStats", 0, UIVL_TEMPORARY)->Set(&rdr);
    CValve *valve = CValve::GetByName("sp_career_kills");

    if (valve)
        SetValveValue(valve, m_kills);
}

/* every stat back to 0 (OVL1 0x2A3E20) */
void CCareerStats::Clear()
{
    m_0 = 0;
    m_4 = 0;
    m_8 = 0;
    m_c = 0;
    m_10 = 0;
    m_14 = 0;
    m_kills = 0;
    m_18 = 0;
    m_20 = 0;
    m_24 = 0;
    m_28 = 0;
    m_2c = 0;
    m_30 = 0;
}

/* an empty message, untyped (OVL1 0x2A4290: the menu state's and the HUD's locals) */
CQueuedMessage::CQueuedMessage()
{
    m_type = MSGTYPE_PLAYER;
}

/* whether a message waits (OVL1 0x2A4270) */
bool CMessageQueue::HasMessages()
{
    return m_head != m_tail;
}

/* a message added at the tail unless the ring is full; a text shorter than two characters is not
 * queued (OVL1 0x2A4050: the chat's lines and the core state's alerts) */
void CMessageQueue::Enqueue(const char *text, MSGTYPE type, const char *name)
{
    if (text && strlen(text) > 1) {
        int last = m_head == 0 ? 9 : m_head - 1;

        if (last != m_tail) {
            int next = m_tail == 9 ? 0 : m_tail + 1;

            m_msgs[m_tail].SetText(text);
            m_msgs[m_tail].SetName(name);
            m_msgs[m_tail].SetType(type);
            m_tail = next;
        }
    }
}

/* the message at the head copied out and its slot cleared (OVL1 0x2A3E60: the menu state's and the
 * HUD's ticks) */
void CMessageQueue::Dequeue(CQueuedMessage *msg)
{
    if (m_head != m_tail) {
        msg->SetText(m_msgs[m_head].m_text);
        msg->SetName(m_msgs[m_head].m_name);
        msg->SetType(m_msgs[m_head].GetType());
        m_msgs[m_head].SetText("QHEAD RETRIEVED (strdup)");
        m_msgs[m_head].SetType(MSGTYPE_PLAYER);
        m_head = m_head == 9 ? 0 : m_head + 1;
    }
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
 * and grid_main.cpp's gridAddNodeToGrids call, after the corners (0x2A96B0, below BoxCorners: a
 * wrapper inlining them one level, their CPnt3D(fff) and assignments called) and their transform
 * (0x2A9680) */
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

/* a node's box carried through a matrix: its corners (0x2A96B0, BoxCorners' wrapper below, the
 * corners' CPnt3D(fff) and assignments called), transformed (0x2A9680) and boxed again (0x2A9550)
 * -- one inline level here, as the world's ladder walk and gridAddNodeToGrids call all four (ours).
 * Measured (the states' round 30, build/scratch/states/p_corners.cpp): the eight corners as an inline
 * `CPnt3D *f(CPnt3D *p, const CBBox *b)` (eight `p[i] = CPnt3D(...)` / `= b->m_min` stores) expand past
 * inline_max_size, so a caller inlines it one level with the sixteen leaves called -- 0x2A8C90's
 * shape, no pragma needed; an out-of-line copy of f inlines the leaves (0xC8) -- 0x2A96B0 is a
 * plain function CALLING f (round 39), so f is cut one level inside it as well */
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

namespace ai {
template <class T>
const T &_min(const T &a, const T &b)
{
    return a <= b ? a : b;
}
}

/* the eight corners of a box, round the bottom from the minimum's x and y at the far z, then the
 * top (ours; round 30's probe): too big for inline_max_size, inlined one level where it is used,
 * the corners' CPnt3D(fff) and assignments called (0x2A8F30, and an earlier unit's 0x20F1F0). By
 * reference, as its out-of-line copy 0x2A96B0 reads the box (CreateLadderTable passes GetBBox's
 * result straight in) */
inline CPnt3D *BoxCorners(CPnt3D *p, const CBBox &b)
{
    p[0] = CPnt3D(b.m_min.x, b.m_min.y, b.m_max.z);
    p[1] = b.m_min;
    p[2] = CPnt3D(b.m_max.x, b.m_min.y, b.m_min.z);
    p[3] = CPnt3D(b.m_max.x, b.m_min.y, b.m_max.z);
    p[4] = CPnt3D(b.m_min.x, b.m_max.y, b.m_max.z);
    p[5] = CPnt3D(b.m_min.x, b.m_max.y, b.m_min.z);
    p[6] = CPnt3D(b.m_max.x, b.m_max.y, b.m_min.z);
    p[7] = b.m_max;
    return p;
}

/* a box's eight corners, out of line (OVL1 0x2A96B0: the world's ladder walk 0x2A9260 and
 * grid_main.cpp's gridAddNodeToGrids call it; the name ours). A function of its own, not
 * BoxCorners' copy: the corners are too big to inline whole, so here, as in 0x2A8C90, they are
 * inlined one level -- their sixteen leaves called, the target's shape -- and a function holding a
 * call that cannot inline whole is never auto-inlined, so its callers call it (the states' round
 * 39; round 30's out-of-line copy of the corners inlined the leaves, 0xC8) */
extern "C" CPnt3D *func_002A96B0(CPnt3D *pts, const CBBox *box)
{
    return BoxCorners(pts, *box);
}

/* a matrix's translation set from a point, read through the pointer (ours: the three stores
 * written on the point itself keep its y's address in a register, an instruction longer) */
inline void SetTranslation(CMatrix *m, const CPnt3D *p)
{
    m->m_matrix[3][0] = p->x;
    m->m_matrix[3][1] = p->y;
    m->m_matrix[3][2] = p->z;
}

/* the ground under a point (fts_main.cpp's, SOCOM II's three-argument form: the hit, its point
 * first) */
float *ftsGetAltitudeApprox(const CPnt3D *pos, float range, bool flag);

/* a node set down on the ground: the point dropped to the floor found under it, the node's box
 * turned by the rotation and its lowest corner on the axis asked for put on the floor, an offset
 * added, the node lit (SOCOM II's; OVL1 0x2A8C90, ftsFixupAiMap calls it twice -- the names ours).
 * The box goes to the corners as an explicit TEMPORARY copy, `BoxCorners(pts, CBBox(*GetBBox()))`
 * (the states' round 33): a temporary or a local copy-constructed from a CBBox copies min and max as
 * two 12-byte blocks ($f2-$f0 each, the target's), where a by-value parameter's copy is one 24-byte
 * word copy ($f3-$f0, 104 instructions off); and GetBBox, the copy constructor's argument inside the
 * inline call's, sits two levels down and stays a call (a local `CBBox b(*node->GetBBox())`, an
 * assignment, or a copy helper inlines it a level: 0x2B0). The corners helper is cut one level by
 * its size, its sixteen leaves called and their copies emitted (CPnt3D(fff) 0x2A8F30 and _min<f>'s
 * OVL1 copy 0x2AFC60 byte-exact from here); the translation is stored through a pointer
 * (SetTranslation above: written on the point in place, y's address is kept in a register). */
extern "C" void func_002A8C90(zdb::CNode *node, const CPnt3D *at, int axis, const CPnt3D *offset,
                              const CPnt3D *rot)
{
    CPnt3D pos = *at;
    float *ground = ftsGetAltitudeApprox(&pos, 10.0f, false);

    if (ground)
        pos.y = ground[1];
    CMatrix m;

    m.SetIdentity();
    if (rot)
        m.SetRotate(rot);
    CPnt3D pts[8];

    BoxCorners(pts, CBBox(*node->GetBBox()));
    m.Transform(pts, 8);
    float lowest = (&pts[0].x)[axis];

    for (int i = 1; i < 8; i++)
        lowest = ai::_min((&pts[i].x)[axis], lowest);
    pos.y -= lowest;
    if (offset)
        pos.Add(offset, &pos);
    SetTranslation(&m, &pos);
    node->SetMatrix(&m);
    node->SetDynamicLight(true, true);
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
    static void RemoveArchive(const char *name, const char *path);   /* 0x32C2B0 */
    static void CloseAll();   /* 0x32C4F0 */
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
    void Unload(const char *name);   /* 0x3A3740 */
    void Load(const char *name, LANGUAGE language);   /* 0x3A3900 */
};
CStringTables *GetStringTables();   /* 0x361BC0 */

/* the particles (zparticle.h's namespace: only its open) */
namespace Particle {
void Close();   /* 0x324820 */
void Open();   /* 0x324B20 */
void FlushDynTexList();   /* 0x3246F0 */
void FreeNodePool();   /* 0x3248E0 */
void ReserveNodePool();   /* 0x3249D0 (SOCOM 1's, at ReOpen_Load's call) */
void FreeInstances();   /* 0x326FF0 */
}

/* the materials (int_material.cpp's CMaterialExArray: only the default's setter) */
class CMaterialExArray {
public:
    static void SetDefaultMaterial(int index);   /* 0x2DDC30 */
    static bool ReadMaterials(CRdrFile *file);   /* 0x2DDE40 */
    static bool UnInit();   /* 0x2DE750 */
    static int ResolveMaterialIndex(const char *name);   /* 0x2DE9E0 */
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

class CZWeaponList;
class CZProjectileList;
class CZWeapon {
public:
    static void Close();   /* 0x3D0350 */
    static void OpenDecals();   /* 0x3D03C0 */
    static void Open();   /* 0x3D03D0 */
    static void ClearTimer(float *pCurval);

    static CZWeaponList m_listWeapons;   /* 0x4B5210 */
    static CZProjectileList *m_pProjectilelist;   /* 0x4B5220 */
    static CZMarkList *m_pMarklist;   /* 0x4B5248 */
};

/* an item a SEAL carries (zweapon.h's EQUIP_ITEM: a byte enum, unsigned; only the bounds) */
enum EQUIP_ITEM {
    EQUIP_PISTOL = 0,
    EQUIP_NONE = 255
};

/* the weapons by item (zweapon.h's CZWeaponList: only the lookup) */
class CZWeaponList {
public:
    CZWeapon *GetWeapon(EQUIP_ITEM item);   /* 0x3C4B40 */
};

/* a weapon a type carries (zchar.h's CCharacterWeap, 0xC bytes: only the item) */
class CCharacterWeap {
public:
    EQUIP_ITEM m_wepId;   // 0x0
    unsigned char _1[0xc - 0x1];
};

/* a character type's weapons (zchar's CCharacterWeapList, a vector: and the slots' padding, OVL2's) */
class CCharacterWeapList : public std::vector<CCharacterWeap> {
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
    static void ClearMultiplayerMessageSounds();   /* OVL2 0x53BB00 */
    static void CleanupTemporary();   /* OVL2 0x53DF50 */
    static void ResetGears();   /* OVL2 0x53F0B0 */

    char *m_name;   // 0x0
    unsigned char _4[0x2e8 - 0x4];
    CCharacterWeapList m_weapons;   // 0x2e8
    unsigned char _2f4[0x300 - 0x2f4];
    unsigned char m_custom;   // 0x300: a custom type (the name ours: stored with li, so no bool)
};


/* the SEALs' units (zkit's CSealUnit: only its statics) */
class CSealUnit {
public:
    static void UnInit();   /* OVL2 0x5ECEF0 */
    static void CreateSinglePlayerUnits();   /* OVL2 0x5EC6A0 */
    static void CreateMultiPlayerUnits();   /* OVL2 0x5ECCD0 */
};

/* the map actions (fts_action.cpp's CNodeAction: only the static open) and the AI maps' fix-up
 * (SOCOM 1's names, at Open's calls) */
class CNodeAction {
public:
    static void Open(zdb::CWorld *world, const char *name);   /* 0x2B4F40 */
    static void Close();   /* 0x2B55E0 */
    static void Reset();   /* 0x2B5510 */
};

void ftsFixupAiMap(CAiMap *map);   /* 0x2A8140 */

/* the pickups (OVL2's CPickup: only its open) */
class CPickup {
public:
    static void Close(bool all);   /* OVL2 0x542E50 */
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
extern const char *data_0064F950;   /* (OVL2: "aimaps.mps") */
void func_00295660();
}

/* the mission built (OVL1 0x2ADBC0; __sinit_fts_mission.cpp's theMission): the save module "CMission"
 * of the config save, the members in order -- the AI parameters' defaults, the three message queues,
 * the chat editor and the message ring, the lists, the camera path, the career stats -- then SOCOM
 * 1's body in its order, SOCOM II's members among it (the talker's anchor, the bit setters, the
 * elevation), the camera path reserved last. The default text goes through CSaferStr's
 * operator=(const char *): its null test folds, and the erase arm it drops leaves the 16 frame bytes
 * the target has and nothing touches (assign() called straight is the same code in an 80-byte frame;
 * dead locals, an allocator temporary or a volatile pointer add nothing) */
CMission::CMission()
    : CSaveModule("CMission", theMemCard.m_configSave), m_state(MISSION_UNLOADED), m_database(0),
      m_timer(0.0f), m_ladders(0), m_pNetGame(0), m_music_hold(0.0f), m_satcheltimer(10.0f)
{
    strncpy(m_weaponmodelpostfix, "", 30);
    m_chars_loaded = 0;
    m_ignore_objectives = 0;
    m_winlossinc = -1.0f;
    m_e8 = -1.0f;
    m_e3_team = 0;
    m_20 = 0;
    m_vehicle_rdr = 0;
    m_183c = "ui/assetlib/junk_name";
    memset(m_team_valves, 0, sizeof(m_team_valves));
    m_player_valve_alive = 0;
    m_whoistalking = 0;
    m_objectives_timer = 0.0f;
    m_prev_selected_objcam = 0;
    strcpy(m_db_string, "UNKNOWN");
    m_ordersRdr = 0;
    m_materialsRdr = 0;
    m_decalsRdr = 0;
    m_fontsRdr = 0;
    m_messagesRdr = 0;
    m_subtitlesRdr = 0;
    m_smallmessagesRdr = 0;
    m_1838 = 0;
    m_subtitles = 0;
    m_seefarther_binocs = 200.0f;
    m_seefarther_weapzoom = 300.0f;
    SetDelayEndMission(false);
    SetFadingToEndMission(false);
    SetFc2(false);
    SetFc3(0);
    m_elevation_max = 127.0f;
    m_elevation_min = -127.0f;
    m_campath.reserve(16);
}

/* the mission torn down: its members in reverse, then the save module (OVL1 0x2AD940, SOCOM 1's
 * implicit chain with SOCOM II's camera path, chat editor and message ring added) */
CMission::~CMission()
{
}

/* the mission opened: the database's archive and string tables, the vectors reserved, the compass,
 * the scheduler's three tasks, the sounds and the jukebox, the materials, the mission's valves, the
 * AI maps and the nav points, the talker's anchor (SOCOM II's), the particles, the anims, the
 * objective and mission cameras, the actions, the ladders, the decals and marks, the SEALs, the
 * vehicles, the weapons, the AI events, the pickups, the units, the AI maps fixed up, the world's
 * descendants, the bomb shacks of a network game, the extraction point (SOCOM 1's Open, its sounds
 * SOCOM II's own loaders; OVL1 0x2AC410) */
bool CMission::Open(const char *name)
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
    m_timer = 0.0f;
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
    CMaterialExArray::ReadMaterials(m_materialsRdr);
    CMaterialExArray::SetDefaultMaterial(world->m_default_soiltype =
                                             CMaterialExArray::ResolveMaterialIndex(world->m_default_soiltype_name));
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
        LoadObjectiveCams(rdr);
        zrdr_free(rdr);
    }
    rdr = zrdr_read("missioncams.rdr", 0, 0);
    LoadMissionCams(rdr);
    zrdr_free(rdr);
    CNodeAction::Open(world, "actions.rdr");
    CreateLadderTable();
    CCharacterModelCache::m_ModelCache.clear();
    CZWeapon::OpenDecals();
    if (CZWeapon::m_pMarklist)
        CZWeapon::m_pMarklist->Init();
    if (theNetwork.m_bNetwork)
        CZSealBody::NetworkPlayerCreate();
    CZSealBody::InitDecalHandles();
    CSealCtrlAi::AcquireDatabaseHandles();
    LoadVehicles();
    CCharacterType::AcquireSoundHandles();
    CZWeapon::Open();
    world->AddCustomCharacters();
    CZSealBody::InitDatabaseHandles();
    func_00295660();
    unsigned int count = CEntity::m_list.size();

    CAiEvent::Open(count);
    if (!theNetwork.m_bNetwork)
        CPickup::Open(count);
    if (!theNetwork.m_bNetwork)
        CSealUnit::CreateSinglePlayerUnits();
    else
        CSealUnit::CreateMultiPlayerUnits();
    RefreshTeamValves();
    CAiMaps::iterator i, e = AiMaps.end();

    for (i = AiMaps.begin(); i != e; ++i)
        ftsFixupAiMap(*i);
    world->InitDescendantsDI();
    world->InitDescendantsVisuals();
    if (theNetwork.m_bNetwork) {
        m_sealbombshack = world->FindChild("seal_tent", true);
        m_terroristbombshack = world->FindChild("terrorist_tent", true);
    }
    Sample_ExtractionLoc();
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

/* the application's camera (camera.h's CAppCamera: its two virtuals, declared only, the zoom and the
 * controlled view) */
class CAppCamera {
public:
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);   /* its table's third word */
    void SetZoom(float zoom);   /* 0x29B2F0 */

    unsigned char _4[0xe4 - 0x4];
    unsigned char m_ctrl_view;   // 0xe4 (camera.h's CAMVIEW: cam_view_third 0)
};

/* the second half of a reload's unload: the entities and AI events gone, the world's lists and
 * garbage cleared (SOCOM 1's), then -- SOCOM II's -- the scheduler's free list, the headset and
 * the sounds (OVL1 0x2AC060) */
bool CMission::ReOpen_Unload2()
{
    zdb::CWorld *world = zdb::CWorld::m_world;

    m_timer = 0.0f;
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

extern "C" {
void func_002B6430(C2DMessage_Q *q, float dT);   /* (fts_message.cpp's: SOCOM 1's ScrollUp here) */
}

/* a queue's lines gone and its scroll put back (SOCOM 1's ScrollUp and the two members, SOCOM II's
 * clear first; the name ours) */
inline void C2DMessage_Q::ResetScroll()
{
    Clear_Messages();
    func_002B6430(this, 0.0f);
    m_moveTime = 2.0f;
    m_cur_top = (int)m_target_top;
}

/* the mission reloaded over its world: the reader's UI archive dropped, the valves reset and the
 * mission's made again, the AI maps reset, the extraction point, the weapons, the vehicles, the
 * characters' sounds and the SEALs' handles, the AI events, the pickups and the units, the ladders
 * freed, the AI maps fixed up, the camera on the player, the queues cleared, the clock off unless a
 * network game (SOCOM 1's ReOpen_Load with SOCOM II's additions; OVL1 0x2ABD40) */
bool CMission::ReOpen_Load()
{
    m_wait_before_falling = 10.0f;
    CRdrArchive::CloseAll();
    CRdrArchive::RemoveArchive("readerc.zar", "run/ui");
    zdb::CWorld *world = zdb::CWorld::m_world;

    CValve::Reset();
    m_objectivecams.Reset();
    m_valve_complete = CValve::Create("mission_complete", 0, VALVE_PERM);
    m_valve_failure = CValve::Create("mission_failure", 0, VALVE_PERM);
    m_valve_abort = CValve::Create("mission_abort", 0, VALVE_PERM);
    m_valve_timeout = CValve::Create("mission_timeout", 0, VALVE_PERM);
    CValve::Create("mission_replay", 1, VALVE_PERM);
    m_valve_nofade = CValve::Create("nofade", 0, VALVE_PERM);
    InitalizeTeamValves();
    AiMaps.Reset();
    Sample_ExtractionLoc();
    CZWeapon::Open();
    CSealCtrlAi::AcquireDatabaseHandles();
    LoadVehicles();
    world->AddCustomCharacters();
    m_chars_loaded = 1;
    CCharacterType::AcquireSoundHandles();
    CZSealBody::ReInitDatabaseHandles();
    func_00295660();
    unsigned int count = CEntity::m_list.size();

    CAiEvent::Open(count);
    CPickup::Open(count);
    CSealUnit::CreateSinglePlayerUnits();
    RefreshTeamValves();
    CLadderList *ladders = m_ladders;

    if (ladders) {
        for (CLadderList::iterator l = ladders->begin(); l != ladders->end(); ++l) {
            l->m_users = 0;
            l->m_owner = CEntity::invalidId;
        }
    }
    CAiMaps::iterator i, e = AiMaps.end();

    for (i = AiMaps.begin(); i != e; ++i)
        ftsFixupAiMap(*i);
    Particle::ReserveNodePool();
    appCamera->AttachTo(ftsGetPlayer());
    CRdrArchive::CloseAll();
    /* each queue through a pointer of its own, declared where it is used: MW keeps each in a saved
     * register, s0, s2, s0 as the target (the members named straight fold the address into every
     * access; one pointer reassigned numbers them s2, s0, s2) */
    C2DMessage_Q *q = &m_messages;

    q->ResetScroll();
    C2DMessage_Q *r = &m_messages3;

    r->ResetScroll();
    C2DMessage_Q *t = &m_messages2;

    t->ResetScroll();
    if (!theNetwork.m_bNetwork)
        m_e8 = -1.0f;
    return true;
}

const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);

/* the mission cameras: the old ones deleted and the list emptied, then each of the rdr's MISSIONCAMS
 * made from its seven strings, a missing NAME a blank (SOCOM 1's; OVL1 0x2A5290) */
bool CMission::LoadMissionCams(CRdrFile *rdr)
{
    for (CMissionCamList::iterator i = m_missioncams.begin(), e = m_missioncams.end(); i != e; ++i)
        delete *i;
    m_missioncams.erase(m_missioncams.begin(), m_missioncams.end());
    if (!rdr)
        return false;
    _zrdr *cams = zrdr_findtag((const _zrdr *)rdr, "MISSIONCAMS");

    if (!cams)
        return false;
    for (int i = 0; i < cams->Count(); i++) {
        _zrdr *cam = cams->Item(i);
        const char *identifier = zrdr_findstring(cam, "IDENTIFIER");
        const char *name = zrdr_findstring(cam, "NAME");
        const char *desc0 = zrdr_findstring(cam, "DESCRIPTION");
        const char *desc1 = zrdr_findstring(cam, "DESCRIPTION2");
        const char *desc2 = zrdr_findstring(cam, "DESCRIPTION3");
        const char *anim = zrdr_findstring(cam, "ON_CALL_ANIMATION");
        const char *music = zrdr_findstring(cam, "ON_CALL_MUSIC");

        if (!name)
            name = " ";
        CMissionCam *mc = new CMissionCam;

        mc->Set(identifier, name, desc0, desc1, desc2, anim, music);
        m_missioncams.push_back(mc);
    }
    return true;
}


/* the mission's anims: from the data directories' rdrs (the paths added first), or from the
 * mission's mzanim.zar -- with the common czanim.zar when that loads, else its zanim.zar (SOCOM 1's,
 * the archives' choice SOCOM II's; OVL1 0x2ABAD0) */
bool CMission::ZAnimLoad()
{
    char path[1024];
    char zar[1024];
    char rdr[1024];

    if (!ZAnim.m_LoadFromZAR) {
        sprintf(path, "data/");
        ZAnim.AddPath(path);
        sprintf(path, "data/common/");
        ZAnim.AddPath(path);
        sprintf(path, "data/%s/", m_database);
        ZAnim.AddPath(path);
    }
    if (ZAnim.m_LoadFromZAR) {
        if (!strncmp(m_database, "mp", 2))
            sprintf(zar, "/run/mp/%s/mzanim.zar", m_database);
        else if (!strncmp(m_database, "m", 1))
            sprintf(zar, "/run/sp/%s/mzanim.zar", m_database);
        else
            sprintf(zar, "/run/%s/mzanim.zar", m_database);
        if (ZAnim.LoadZAR(zar, 0)) {
            sprintf(zar, "run/czanim.zar");
            ZAnim.LoadZAR(zar, 0);
        } else {
            if (!strncmp(m_database, "mp", 2))
                sprintf(zar, "/run/mp/%s/zanim.zar", m_database);
            else if (!strncmp(m_database, "m", 1))
                sprintf(zar, "/run/sp/%s/zanim.zar", m_database);
            else
                sprintf(zar, "/run/%s/zanim.zar", m_database);
            ZAnim.LoadZAR(zar, 0);
        }
    }
    if (!ZAnim.m_LoadFromZAR) {
        ZAnim.Load("data/common/zrdr/czanim.rdr", "common");
        sprintf(rdr, "data/%s/zrdr/zanim.rdr", m_database);
        ZAnim.Load(rdr, "mission");
    }
    return true;
}

/* the mission's extra string tables: their count, then their names, ten characters each (the
 * names ours) */
struct MissionBank {
    char m_name[10];
};

struct MissionBankList {
    /* the i'th table's name (the index unsigned: the target recomputes the address every pass) */
    const char *GetBank(unsigned int i) const { return m_banks[i].m_name; }
    char *GetBank(unsigned int i) { return m_banks[i].m_name; }

    /* the i'th slot (the names ours) */
    MissionBank *At(int i) { return &m_banks[i]; }

    /* the count changed: the slots past the new count destroyed, or the new ones built in place --
     * a fixed array's resize. Both loops vanish for a name, but reached through At() (an inline
     * returning a member's address) MW leaves their arms' shells: the test kept, the build loop's
     * arm a lone jump back placed after the function's body, as the target's (func_002AF410,
     * Close). Without At() (round 23's loops over the array itself) MW folds both arms away */
    void Resize(int n)
    {
        if (n < m_count) {
            for (int i = n; i < m_count; i++)
                At(i)->~MissionBank();
        } else {
            for (int i = m_count; i < n; i++)
                new (At(i)) MissionBank;
        }
        m_count = n;
    }

    /* (none to begin with: the unit's initialiser clears the count last) */
    MissionBankList() : m_count(0) {}

    int m_count;
    MissionBank m_banks[7];   /* (the most 0x440B84-0x440BCF holds) */
};

/* (defined below, with the unit's static initialiser's statics) */
extern "C" MissionBankList data_00440B80;

namespace {
void UpdateMissionArchive(const char *olddb, const char *newdb);
}

/* the zoom (thezoom.cpp's CameraZoom: only its lens effects' reset), the bone callbacks, the
 * world's ripples and texture effects (vis_ripple.h's and vis_texfx.h's managers: only their
 * clears), the decal set, the dodge, the SEAL units, the spawners and the projectiles (only what
 * Close calls) */
class CameraZoom {
public:
    static void ClearLensFx();   /* 0x1EFF90 */
};

void zBoneResetCallbacks();   /* 0x2875D0 */

namespace zdb {
class CWaterRippleManager {
public:
    void Clear();   /* 0x3C2C80 */
};

class CTextureFXManager {
public:
    void Clear(bool all);   /* 0x3C0660 */
};

extern CWaterRippleManager theWaterRippleManager;   /* 0x4B5170 */
extern CTextureFXManager theTextureFXManager;   /* 0x4B50D0 */
}

class CZDecalSet {
public:
    static void DestroyDecalSet();   /* 0x3D09D0 */
};

class CAiDodge {
public:
    static void Destroy();   /* OVL2 0x5FAD50 */
};

class CSpawners {
public:
    void Clear();   /* 0x2B2190 */
};

extern CSpawners spawners;   /* 0x440C10 */

/* the projectiles in flight (zweapon.h's CZProjectileList: only the three the mission calls) */
class CZProjectileList {
public:
    void Dump();   /* 0x3CBB30 */
    void Clear(int owner, EQUIP_ITEM item);   /* 0x3CCEB0 */
    void SetAllFlag18();   /* 0x3CCC90 */
};

/* an objective (fts_objectivecam.cpp's: only its destructor, deleted here) */
class CMissionObjectiveCam : public CMissionCam {
public:
    ~CMissionObjectiveCam();   /* 0x2C7490 */
};

extern "C" {
CZAnimSet *GetAnimSet__10CZAnimMainFPCc(CZAnimMain *main, const char *name);
void func_00295610();
void func_005B61A0();   /* (OVL2) */
}

/* the AI's planner queue flushed (OVL2's ai::FlushPlannerQueue), the pipe (zrndr's CPipe: only its
 * flush) and the texture manager's packet buffers */
namespace ai {
void FlushPlannerQueue();   /* OVL2 0x510D10 */
}

class CPipe {
public:
    void Flush();   /* 0x33B720 */
};
extern CPipe thePipe;   /* 0x4887C0 */

namespace zdb {
class CTexManager {
public:
    static void ClearPktBufs(CTexManager &manager, bool all);   /* 0x355F20 */

    static CTexManager m_texmanager;   /* 0x49E880 */
};
}

/* a queue's lines gone, through its pointer (SOCOM 1's ReOpen_Unload1 tests each queue's address
 * too; the name ours) */
inline void ClearQueue(C2DMessage_Q *q)
{
    if (q)
        q->Clear_Messages();
}

/* the first half of a reload's unload: the extraction and objective anchors deleted, the HUD map's
 * selection, the headset, the world's decals and characters, the fade's task, the camera let go,
 * the planner queue, the clutter's effectors and animations, the SEALs' and the weapons' handles,
 * the pickups, the common and mission anims stopped and collected and reset, the ripples, dodges,
 * spawners, bone callbacks and units, the actions, the objectives' valves, the valves, the HUD, the
 * stream, the queues, the particles, the pipe and the packet buffers (SOCOM 1's ReOpen_Unload1 with
 * SOCOM II's additions; OVL1 0x2AC0E0) */
bool CMission::ReOpen_Unload1()
{
    while (m_anchor_extraction.size()) {
        CCompass::DeleteAnchor(m_anchor_extraction.back());
        m_anchor_extraction.pop_back();
    }
    while (m_objanchor_vec.size()) {
        CCompass::DeleteAnchor(m_objanchor_vec.back().m_anchor);
        m_objanchor_vec.pop_back();
    }
    if (theHUD && !theNetwork.m_bNetwork)
        theHUD->m_newmap->m_cur_selected_objective = 0;
    Headset::Reset();
    zdb::CWorld *world = zdb::CWorld::m_world;

    world->m_diIntDelayCnt = 0;
    world->RemoveDecals();
    world->WipeoutTextureAssetCharacters();
    zTaskScheduler.RemoveTask("MissionFadeToEnd", true);
    ClearFreeList__14CSched_ManagerFb(&zTaskScheduler, true);
    appCamera->AttachTo(0);
    m_chars_loaded = 0;
    ai::FlushPlannerQueue();
    world->m_grid.m_ClutterAnimManager->ClearEffectors();
    world->m_grid.m_ClutterAnimManager->ClearAnimations(true);
    CZSealBody::Uninit_ReInitDatabaseHandles();
    func_00295610();
    CZWeapon::Close();
    CPickup::Close(true);
    CZAnimSet *set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "common");

    if (set) {
        set->Stop();
        set->GarbageCollect();
    }
    set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "mission");
    if (set) {
        set->Stop();
        set->GarbageCollect();
    }
    set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "common");
    if (set)
        set->Reset();
    set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "mission");
    if (set)
        set->Reset();
    zdb::theWaterRippleManager.Clear();
    CAiDodge::Destroy();
    CSealCtrlAi::ReleaseDatabaseHandles();
    spawners.Clear();
    zBoneResetCallbacks();
    CSealUnit::UnInit();
    func_005B61A0();
    CNodeAction::Reset();
    m_objectivecams.ClearValves();
    CValve::Close();
    theHUD->ReInit_Uninit();
    Headset::ResetStream();
    SetFc2(false);
    ClearQueue(&m_messages);
    ClearQueue(&m_messages2);
    ClearQueue(&m_messages3);
    Particle::FreeInstances();
    Particle::FlushDynTexList();
    Particle::FreeNodePool();
    thePipe.Flush();
    zdb::CTexManager::ClearPktBufs(zdb::CTexManager::m_texmanager, true);
    return true;
}

/* the mission closed: its sounds, decals and archive, the extra string tables, the objective and
 * mission cameras, the camera path, the pickups, the zoom's lens effects, the anim sets, the bone
 * callbacks, the ripples, the HUD map's selection, the models, materials, decals, particles, dodges,
 * spawners and entities, the jukebox and the sounds, the ladders, the gears, the vehicles, the AI
 * events, the weapons, the texture effects, the SEAL units, the actions, the AI maps, the valves,
 * the world's lists, the scheduler's three tasks, the online game's character type, the headset, the
 * bomb shacks, every compass anchor and the anchor vectors freed, the temporary types, the
 * projectiles and the UI variables (SOCOM 1's Close with SOCOM II's additions; OVL1 0x2AB3F0) */
void CMission::Close()
{
    m_state = MISSION_UNLOADED;
    if (theNetwork.m_bNetwork)
        data_0044F3CC = 0;
    CSndInstance::StopAllSounds();
    zdb::CWorld *world = zdb::CWorld::m_world;

    if (world) {
        world->m_diIntDelayCnt = 0;
        world->RemoveDecals();
        world->WipeoutTextureAssetCharacters();
    }
    m_chars_loaded = 0;
    UpdateMissionArchive(m_database, 0);
    if (m_database)
        GetStringTables()->Unload(m_database);
    int n = data_00440B80.m_count;

    for (int i = 0; i < n; i++)
        GetStringTables()->Unload(data_00440B80.GetBank(i));
    data_00440B80.Resize(0);
    m_database = 0;
    for (CMissionObjectiveCamList::iterator e = m_objectivecams.end(), i = m_objectivecams.begin(); i != e; ++i)
        delete *i;
    m_objectivecams.erase(m_objectivecams.begin(), m_objectivecams.end());
    for (CMissionCamList::iterator e = m_missioncams.end(), i = m_missioncams.begin(); i != e; ++i)
        delete *i;
    m_missioncams.erase(m_missioncams.begin(), m_missioncams.end());
    m_campath.clear();
    CPickup::Close(true);
    CameraZoom::ClearLensFx();
    CZAnimSet *set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "common");

    if (set) {
        set->Stop();
        set->Unload();
    }
    set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "mission");
    if (set) {
        set->Stop();
        set->Unload();
    }
    ZAnim.FreeNetworkPacketBuffer();
    zBoneResetCallbacks();
    zdb::theWaterRippleManager.Clear();
    if (theHUD && !theNetwork.m_bNetwork)
        theHUD->m_newmap->m_cur_selected_objective = 0;
    CCharacterModelCache::m_ModelCache.clear();
    CCharacterType::ClearMultiplayerMessageSounds();
    CMaterialExArray::UnInit();
    CZSealBody::UninitDatabaseHandles();
    CSealCtrlAi::ReleaseDatabaseHandles();
    CZDecalSet::DestroyDecalSet();
    Particle::Close();
    CAiDodge::Destroy();
    spawners.Clear();
    CEntity::RemoveAll();
    CSndJukebox::Stop();
    CSnd::UnloadAllSounds();
    CSndJukebox::Cleanup();
    CSndInstance::DebugSoundList();
    CSnd::Close();
    if (m_ladders) {
        m_ladders->clear();
        delete m_ladders;
        m_ladders = 0;
    }
    CCharacterType::ResetGears();
    CloseVehicleRdr();
    CAiEvent::Close();
    CZWeapon::Close();
    zdb::theTextureFXManager.Clear(true);
    CSealUnit::UnInit();
    CNodeAction::Close();
    AiMaps.Close();
    CValve::Close();
    world = zdb::CWorld::m_world;
    if (world) {
        world->ClearLightMapList();
        world->ClearShadowList();
    }
    zTaskScheduler.RemoveTask("Mission", false);
    zTaskScheduler.RemoveTask("Update visual effects", false);
    zTaskScheduler.RemoveTask("diTick", false);
    ClearFreeList__14CSched_ManagerFb(&zTaskScheduler, true);
    if (theNetwork.m_bNetwork) {
        CZNetGame *game = m_pNetGame;

        strncpy(game->m_12c, "0", 31);
        game->m_12c[30] = 0;
    }
    Headset::Reset();
    m_sealbombshack = 0;
    m_terroristbombshack = 0;
    while (m_anchor_extraction.size()) {
        CCompass::DeleteAnchor(m_anchor_extraction.back());
        m_anchor_extraction.pop_back();
    }
    while (m_anchors.size()) {
        CCompass::DeleteAnchor(m_anchors.back());
        m_anchors.pop_back();
    }
    while (m_objanchor_vec.size()) {
        CCompass::DeleteAnchor(m_objanchor_vec.back().m_anchor);
        m_objanchor_vec.pop_back();
    }
    if (m_whoistalking) {
        CCompass::DeleteAnchor(m_whoistalking);
        m_whoistalking = 0;
    }
    theCompass.Close();
    FreeVector(m_anchor_extraction);
    FreeVector(m_anchors);
    FreeVector((std::vector<SObjectiveCam_Anchor> &)m_objanchor_vec);
    CCharacterType::CleanupTemporary();
    if (CZWeapon::m_pProjectilelist)
        CZWeapon::m_pProjectilelist->Dump();
    theUIVarManager.CollectGarbage();
    m_20 = 0;
}

/* the shortest respawn time Read lets "ai_params" set, and the default (10 in data: a variable, it
 * is loaded with lwc1; SOCOM 1's default was the constant 5) */
extern float data_003DE5F0;

/* the mission's AI parameters' defaults (OVL1 0x2AB360): SOCOM 1's seven, in its order (the recycle
 * time ahead of the range), SOCOM II's six after them -- sixteen statements, so never auto-inlined:
 * CMission()'s AI_PARAMS() and Read call it */
void CMission::AI_PARAMS::Reset()
{
    weather_factor = 0.0f;
    fireteam_command = 4;
    recycle_time = 60.0f;
    recycle_range = 10000.0f;
    respawn_time = data_003DE5F0;
    respawn_fade = 0.75f;
    respawn_range = 2500.0f;
    light_scale = 0.85f;
    light_inside = 0.65f;
    shadow_scale = 1.0f;
    shadow_inside = 0.5f;
    ambient_noise = 1.0f;
    stealth_noise = 1.0f;
    active_limit = 9;
    nosnooze_limit = 5;
}

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
 * Declared float and returning nothing: MW 3.0's return of a void (or int) function keeps v0 live
 * from the entry along any path without a call, so the valve's value, its state test and the
 * constant 1 for 0x112 took v1 -- the target gives them v0, which only a return in $f0 leaves free
 * (build/scratch/states/p_ret.cpp: void, int with and without a return, static void and float
 * probed; SOCOM 1's void copy, an older build, used v0 too). The return type is the reproduction's,
 * not evidence of the original's */
float CZNetGame::UnInitialize()
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
float g_fTacMapZoomInit;   /* the "TacMapZoom" list's three: the zoom the full-screen map opens */
float g_fTacMapZoomNear;   /* at (1), the nearest (2.2) and the farthest (0.55) it zooms to, */
float g_fTacMapZoomFar;    /* each over 2200 a map width (PreOpen 0x2ACC10 reads them) */
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

/* the tactical map's zoom getters, SOCOM 1's three by order (0x1FA850 Near, 0x1FA860 Far, 0x1FA870
 * the opening zoom) and by their statics' order (Init, Near, Far: SOCOM 1's gp-29328/-29324/-29320,
 * here 0x440BB8/0x440BC0/0x440BC8, which PreOpen 0x2ACC10 fills from "TacMapZoom"'s items 0, 1
 * and 2 as SOCOM 1's PreOpen does) -- and by their callers' roles: SwitchMapToFullScreen 0x20A780
 * opens at 2200 / Zoom as SOCOM 1's does (0x3C4F5C), the zoom step 0x20A330 bounds the width by
 * 2200 / Far above and 2200 / Near below, and FrameObjective 0x20B800 by 2200 / Far, as SOCOM 1's
 * TickFullScreenMap does at 0x3C47A0/0x3C47DC and 0x3C46D0 (OVL1 0x2AF880) */
float ftsGetTacMapZoom()
{
    return g_fTacMapZoomInit;
}

/* (OVL1 0x2AF870) */
float ftsGetTacMapZoomFar()
{
    return g_fTacMapZoomFar;
}

/* (OVL1 0x2AF860) */
float ftsGetTacMapZoomNear()
{
    return g_fTacMapZoomNear;
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

namespace {
/* the AI map visibility lists the mission's rdr names: every list emptied, then from
 * "ai_map_visibility" up to 32 lists of up to 31 map numbers each (SOCOM 1's, its 10 lists of 9
 * SOCOM II's 32 of 31; OVL1 0x2AF550). The bounds are the arrays' own sizes, unsigned */
void LoadMissionMapVisibilities(CRdrFile *rdr)
{
    for (int i = 0; i < sizeof(g_arrMapVisCounts) / sizeof(g_arrMapVisCounts[0]); i++)
        g_arrMapVisCounts[i] = 0;
    _zrdr *vis = zrdr_findtag((const _zrdr *)rdr, "ai_map_visibility");
    int n = vis ? vis->Count() : 0;

    if (n > 32)
        n = 32;
    for (int i = 0; i < n; i++) {
        if (i >= sizeof(g_arrMapVisCounts) / sizeof(g_arrMapVisCounts[0]))
            break;
        _zrdr *list = vis->Item(i);
        int count = list ? list->Count() : 0;

        g_arrMapVisCounts[i] = count < 32 ? count : 31;
        for (int j = 0; j < g_arrMapVisCounts[i]; j++)
            g_arrMapVis[i][j] = list->Item(j)->Integer();
    }
}
}

extern "C" {
MissionBankList data_00440B80;
}

/* the extra string tables the mission's rdr lists, by name (SOCOM II's; PreOpen calls it; OVL1
 * 0x2AF410).
 * The two empty if/else tests on the count (`0 < count` before it is cleared, `count + 1 < count`
 * before each increment, the else arms jumps placed after the body) are MissionBankList::Resize's
 * loops reached through At(); the count a ternary, the name's address taken ahead of the copy */
extern "C" void func_002AF410(const _zrdr *rdr)
{
    data_00440B80.Resize(0);
    _zrdr *list = zrdr_findtag(rdr, "ExtraStringTables");
    int n = list ? list->Count() : 0;

    for (int i = 0; i < n; i++) {
        data_00440B80.Resize(data_00440B80.m_count + 1);
        char *bank = data_00440B80.GetBank(i);

        strcpy(bank, list->Item(i)->String());
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

extern "C" const char *func_003D2820(CZWeapon *weapon);   /* (a weapon's description, its word at 0xC) */

/* a custom type's weapons' UI variables: each of its first six weapons' description under the type's
 * slot names, "<NONE>" in the slots it leaves (SOCOM 1's; OVL1 0x2AF190) */
void UpdateCharWeaponUiVars(int i)
{
    const char *names[8][6] = {
        {"Char1PrimaryWeaponDescr", "Char1SecondaryWeaponDescr", "Char1Equip1Descr", "Char1Equip2Descr", "Char1Equip3Descr", "Char1Equip4Descr"},
        {"Char2PrimaryWeaponDescr", "Char2SecondaryWeaponDescr", "Char2Equip1Descr", "Char2Equip2Descr", "Char2Equip3Descr", "Char2Equip4Descr"},
        {"Char3PrimaryWeaponDescr", "Char3SecondaryWeaponDescr", "Char3Equip1Descr", "Char3Equip2Descr", "Char3Equip3Descr", "Char3Equip4Descr"},
        {"Char4PrimaryWeaponDescr", "Char4SecondaryWeaponDescr", "Char4Equip1Descr", "Char4Equip2Descr", "Char4Equip3Descr", "Char4Equip4Descr"},
        {"Char5PrimaryWeaponDescr", "Char5SecondaryWeaponDescr", "Char5Equip1Descr", "Char5Equip2Descr", "Char5Equip3Descr", "Char5Equip4Descr"},
        {"Char6PrimaryWeaponDescr", "Char6SecondaryWeaponDescr", "Char6Equip1Descr", "Char6Equip2Descr", "Char6Equip3Descr", "Char6Equip4Descr"},
        {"Char7PrimaryWeaponDescr", "Char7SecondaryWeaponDescr", "Char7Equip1Descr", "Char7Equip2Descr", "Char7Equip3Descr", "Char7Equip4Descr"},
        {"Char8PrimaryWeaponDescr", "Char8SecondaryWeaponDescr", "Char8Equip1Descr", "Char8Equip2Descr", "Char8Equip3Descr", "Char8Equip4Descr"}
    };

    if (i >= 8)
        return;
    const CCharacterType *type = CCharacterType::GetByNameMod(GetCustomCharTypeName(i));

    if (!type)
        return;
    CZWeaponList *list = &CZWeapon::m_listWeapons;
    int n = 0;

    for (CCharacterWeapList::const_iterator w = type->m_weapons.begin(), e = type->m_weapons.end();
         w != e && n < 6; ++w, ++n) {
        CZWeapon *weapon = list->GetWeapon((*w).m_wepId);
        const char *name = names[i][n];
        const char *descr = weapon ? func_003D2820(weapon) : "(unknown)";

        *theUIVarManager.Add(name, 0, UIVL_TEMPORARY) = descr;
    }
    for (; n < 6; n++)
        *theUIVarManager.Add(names[i][n], 0, UIVL_TEMPORARY) = "<NONE>";
}

/* the mission's package (util_main.cpp's CPackage: only its open and close) and the package itself
 * (util_main.cpp's thePackage, 0x4A15F8) */
class CPackage {
public:
    bool Close();   /* 0x39C3F0 */
    bool Open(const char *name);   /* 0x39C440 */
};

extern CPackage thePackage;

extern "C" {
unsigned int strlen(const char *s);
}

namespace {
/* the old mission's archive taken off the reader and its package closed, then the new mission's
 * package and archive opened -- each archive under run/, run/mp/ or run/sp/ by the database's
 * prefix (SOCOM 1's, the packages SOCOM II's; OVL1 0x2AEF90) */
void UpdateMissionArchive(const char *olddb, const char *newdb)
{
    if (olddb) {
        CSaferStr path;

        path = "run/";
        if (!strncmp(olddb, "mp", 2))
            path += "mp/";
        else if (!strncmp(olddb, "m", 1))
            path += "sp/";
        path += olddb;
        CRdrArchive::RemoveArchive("readerm.zar", path.c_str());
        thePackage.Close();
    }
    if (newdb && strlen(newdb)) {
        char name[32] = "run/";

        strcat(name, newdb);
        strcat(name, ".zdb");
        thePackage.Open(name);
        CSaferStr path;

        path = "run/";
        if (!strncmp(newdb, "mp", 2))
            path += "mp/";
        else if (!strncmp(newdb, "m", 1))
            path += "sp/";
        path += newdb;
        CRdrArchive::AddArchive("readerm.zar", path.c_str());
        CRdrArchive::OpenAll();
    }
}
}

/* the input (zinput's CInput: only the flush) */
class CInput {
public:
    static void Flush();   /* 0x2D9E00 */
};

extern "C" {
CZAnimSet *GetAnimSet__10CZAnimMainFPCc(CZAnimMain *main, const char *name);
void func_00326FB0();
void func_005781C0(CZSealBody *player);   /* (OVL2) */
extern unsigned char data_0048DB50;
extern int data_00488EA0;
}

/* the mission over: the common and mission anims stopped, the sounds and the music, the player's
 * rumble and the input; in a network game the player's projectiles, the pickups and the rest of
 * the projectiles let go; every entity told, the network packets freed, the camera's zoom and view
 * put back (SOCOM 1's, SOCOM II's anim sets first and the network part its own; OVL1 0x2AA910) */
void _OnMissionComplete()
{
    CZAnimSet *set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "common");

    if (set)
        set->Stop();
    set = GetAnimSet__10CZAnimMainFPCc(&ZAnim, "mission");
    if (set)
        set->Stop();
    func_00326FB0();
    CSndInstance::StopAllSounds();
    CSnd::Close();
    CSndJukebox::Stop();
    data_0048DB50 = 0;
    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player ? player->m_control : 0;

    if (ctrl)
        ctrl->StopRumbler();
    CInput::Flush();
    if (theNetwork.m_bNetwork) {
        CZSealBody *seal;
        CZProjectileList *list = CZWeapon::m_pProjectilelist;

        seal = ftsGetPlayer();

        if (seal) {
            func_005781C0(seal);
            list->Clear(seal->m_id, EQUIP_NONE);
        }
        CPickup::Close(false);
        list->SetAllFlag18();
        data_00488EA0 = 0;
    }
    CEntityList::const_iterator i;

    for (i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end(); ++i)
        (*i)->OnMissionEnd();
    ZAnim.FreeNetworkPacketBuffer();
    appCamera->SetZoom(1.0f);
    appCamera->m_ctrl_view = 0;
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

/* START_IMAGE_RECORDER: the recorder's file name, its lines and rate (SOCOM 1's command; SOCOM II's
 * antialias flag and intensity new) */
struct _start_image_recorder_cmd {
    _zanim_cmd_hdr hdr;
    unsigned int name : 8;   // 0x4: the file name's index among the current anim's names
    unsigned int top_line : 10;
    unsigned int bottom_line : 10;
    unsigned int antialias : 1;
    unsigned int unused : 3;
    float rate;   // 0x8
    float intensity;   // 0xc
};

extern "C" int data_004A44C8;   /* (the screen's lines: the bottom line's default is the last) */
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
bool zrdr_findbool(const _zrdr *rdr, const char *tag, bool *out);

/* START_IMAGE_RECORDER's parser: the FILENAME kept among the current anim's names, the whole screen
 * at 29.97 frames by default, then TOP_LINE, BOTTOM_LINE, RATE, ANTIALIAS and INTENSITY where given
 * (SOCOM 1's; OVL1 0x2AED40) */
_zanim_cmd_hdr *StartImageRecorderParse(_zrdr *rdr)
{
    _start_image_recorder_cmd *cmd;
    int i;
    float f;
    bool b;

    if (!rdr)
        return 0;
    cmd = (_start_image_recorder_cmd *)_calloc(1, sizeof(_start_image_recorder_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_start_image_recorder_cmd);
    cmd->hdr.timeless = 1;
    CZAnim *anim = ZAnim.m_CurAnim;
    const char *name = zrdr_findstring(rdr, "FILENAME");

    if (name)
        cmd->name = anim->AddName(name);
    cmd->top_line = 0;
    cmd->bottom_line = data_004A44C8 - 1;
    cmd->rate = 29.97f;
    cmd->antialias = false;
    cmd->intensity = 1.0f;
    if (zrdr_findint(rdr, "TOP_LINE", &i, 1))
        cmd->top_line = i;
    if (zrdr_findint(rdr, "BOTTOM_LINE", &i, 1))
        cmd->bottom_line = i;
    if (zrdr_findreal(rdr, "RATE", &f, 1))
        cmd->rate = f;
    if (zrdr_findbool(rdr, "ANTIALIAS", &b))
        cmd->antialias = b;
    if (zrdr_findreal(rdr, "INTENSITY", &f, 1))
        cmd->intensity = f;
    return &cmd->hdr;
}

/* START_IMAGE_RECORDER's tick: done at once (SOCOM 1's; OVL1 0x2AED30) */
bool StartImageRecorderTick(_zanim_cmd_hdr *hdr, float *dT)
{
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

/* STOP_IMAGE_RECORDER's tick: done at once (SOCOM 1's; OVL1 0x2AECB0) */
bool StopImageRecorderTick(_zanim_cmd_hdr *hdr, float *dT)
{
    return true;
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

/* NVG_NOISE: the night vision's noise, a WEIGHT, or a WEIGHT_SEQUENCE of {time, weight} keys appended
 * to the command, timed by its last key (SOCOM II's; the names ours) */
struct _nvg_noise_cmd {
    _zanim_cmd_hdr hdr;
    unsigned short keys;   // 0x4: the keys' offset from the command, 0 without
    unsigned short count;   // 0x6
    float weight;   // 0x8
    float time;   // 0xc
};

struct nvg_noise_key {
    float t;
    float v;
};

void _free(void *p, const char *file, int line);
bool zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);

/* NVG_NOISE's parser: each key's weight held to 1 at most, the command timeless without a key past
 * 0 (OVL1 0x2AE050).
 * OPEN, 6 instructions, registers only: the loop's two induction temporaries trade t0 and t1 -- the
 * target's keys cursor is t1 and the items' offset t0, here the other way round (the keys walked by
 * a named pointer, a reference to keys[i] first in the body, an int counter, the body under an if
 * instead of the continue, keys declared first: no better). The assignment in the sequence's test
 * gives the target's `beqz v0` and Type() its reuse of the type byte */
_zanim_cmd_hdr *NVGNoiseParse(_zrdr *rdr)
{
    _nvg_noise_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_nvg_noise_cmd *)_calloc(1, sizeof(_nvg_noise_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_nvg_noise_cmd);
    cmd->hdr.timeless = 1;
    cmd->time = 0.0f;
    _zrdr *seq;

    if ((seq = zrdr_findtag(rdr, "WEIGHT_SEQUENCE")) != 0) {
        if (seq->Type() == 4) {
            nvg_noise_key *keys = (nvg_noise_key *)_calloc(sizeof(nvg_noise_key), seq->Count(), 0, 0);

            for (unsigned int i = 0; i < seq->Count(); i++) {
                _zrdr *item = seq->Item(i);

                if (item->Count() < 2)
                    continue;
                float t = item->Item(0)->Real();

                keys[i].t = t;
                if (!(t <= cmd->time))
                    cmd->time = t;
                float w = item->Item(1)->Real();

                keys[i].v = w;
                if (!(w <= 1.0f))
                    keys[i].v = 1.0f;
            }
            cmd->count = seq->Count();
            cmd->keys = cmd->hdr.data_size;
            cmd = (_nvg_noise_cmd *)zAnimCmdAddData(&cmd->hdr, keys, cmd->count * sizeof(nvg_noise_key));
            _free(keys, 0, 0);
        }
    } else
        zrdr_findreal(rdr, "WEIGHT", &cmd->weight, 1);
    if (!(cmd->time <= 0.0f))
        cmd->hdr.timeless = false;
    return &cmd->hdr;
}

/* NVG_NOISE's tick: the weight, or the keys' weight at the command's time, set as the HUD's noise
 * (times 128); done once its time has passed (OVL1 0x2ADF10) */
bool NVGNoiseTick(_zanim_cmd_hdr *hdr, float *dT)
{
    _nvg_noise_cmd *cmd = (_nvg_noise_cmd *)hdr;
    bool done = true;
    float v = cmd->weight;

    if (cmd->keys > 0) {
        nvg_noise_key *key = (nvg_noise_key *)((char *)cmd + cmd->keys);
        float t = *dT + ZAnim.m_CurSeq->cmd_timer;

        if (!(t <= key[0].t)) {
            bool found;
            int i;

            found = false;
            for (i = 0; i < cmd->count - 1; i++) {
                bool in = t <= key[i + 1].t;

                if (in) {
                    found = true;
                    v = key[i].v + (t - key[i].t) / (key[i + 1].t - key[i].t) * (key[i + 1].v - key[i].v);
                    break;
                }
            }
            if (!found)
                v = key[cmd->count - 1].v;
        }
    }
    theHUD->m_nvg_noise = 128.0f * v;
    float time = cmd->time;

    if (!(time <= 0.0f)) {
        float t = ZAnim.m_CurSeq->cmd_timer;
        float step = *dT;

        if (!(step + t <= time)) {
            done = true;
            *dT -= time - t;
        } else {
            done = false;
            *dT = 0.0f;
        }
    }
    return done;
}

/* the auto-inline limit raised by two statements: MW inlines an out-of-class template member into an
 * inline function's out-of-line copy only under it, and the pod's reserve needs 17 -- so the pointer
 * vector's reserve 0x2B1600 that resize<CUIVariable> calls is the pod reserve inlined, as Open's own
 * reserve generated it first (the MSL headers round 16, include/vector's pointer __vector_imp) */
#pragma inline_max_auto_size(17)
