/* fts_player.cpp -- the spawners, the mission's team valves and vehicles, and the player: its
 * ZAnim commands, its SEAL's creation and the copy controller (SOCOM 1's order: CSpawners' code
 * opens its fts_player.cpp, after fts_mission.cpp's template instances, and the CSpawners list's
 * instances close it, before fts_action.cpp's CNodeAction code). OVL1 0x2B2190-0x2B418F; its
 * static initialiser is 0x400330. Defined in reverse of their image order (-inline deferred). */

#include "../zentity/zentity.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include "../../ai/ai_maps.h"

/* the body's states (zseal.h's SEAL_STATE: only its name and range, for OnStateChange's) */
enum SEAL_STATE {
    stateStand = 0, stateCrouch = 1, stateProne = 2, statePeek = 3, stateHang = 4,
    stateClimbLadder = 5, stateInAir = 6, stateIdle = 7, stateDead = 8, stateCarrying = 9,
    stateCarried = 10, stateDone = 11
};

class CVehicleRdrEntry;

/* how a character behaves (zchar.h's AI_PARAMS, 0x3C bytes, as the vehicle entry and the controller
 * hold it: the constructor user-declared, so the assignment copies member by member -- the floats
 * through the FPRs, the words with lw/sw, as LoadVehicle's inline copy does) */
class AI_PARAMS {
public:
    AI_PARAMS();

    unsigned int mask;   // 0x0
    float accuracy;   // 0x4
    float courage;   // 0x8
    float surrender;   // 0xc
    float stealth;   // 0x10
    float training;   // 0x14
    float visual_sens;   // 0x18
    float visual_range;   // 0x1c
    float visual_fov;   // 0x20
    float aware_decay;   // 0x24
    float thip_range;   // 0x28
    unsigned int respawn;   // 0x2c
    float target_delay;   // 0x30
    float target_discipline;   // 0x34
    CVehicleRdrEntry *m_veh;   // 0x38
};

/* a SEAL's controller (SOCOM 1's CSealCtrl, 592 bytes at SOCOM II's offsets: only what this unit
 * touches) and the three kinds ftsCreateSeal makes besides */
class CSealCtrl {
public:
    CSealCtrl();
    virtual ~CSealCtrl();
    /* zseal.h's inline (slot 0x64 of the real table): CSealCtrlCopy's table, emitted here with its
     * key function, the destructor, makes this unit emit OVL1's out-of-line copy, 0x2B3BE0 -- as
     * the original's did (OVL2's copy is 0x58A710) */
    virtual void OnStateChange(SEAL_STATE from, SEAL_STATE to) {}

    unsigned char _4[0xd0 - 0x4];
    AI_PARAMS m_aiParams;   // 0xd0 (zseal.h's)
    unsigned char _10c[0x114 - 0x10c];
    int m_copy_index;   // 0x114: the player's copies' number (LoadVehicles sets it; the name ours)
    unsigned char _118[0x130 - 0x118];
    float m_aimPitch;   // 0x130 (SOCOM 1's DWARF, at 0x130 there too)
    unsigned char _134[0x171 - 0x134];
    unsigned char _171_0 : 3;
    unsigned char m_input_enabled : 1;   // 0x171, bit 3 (SOCOM 1's at 0x169)
    unsigned char _171_4 : 3;
    unsigned char m_171_7 : 1;   // 0x171, bit 7
    unsigned char _172[0x250 - 0x172];
};

class CSealCtrlAi : public CSealCtrl {
public:
    CSealCtrlAi();
    /* the brain built from the parameters (OVL2 0x573130, SOCOM 1's) */
    void InitAiBrain();
    /* the debug bit (SOCOM 1's SetDebug, out of line there; inline here: LoadVehicle stores it
     * through a parameter, `andi a3,zero,1`) */
    void SetDebug(bool on) { m_debug = on; }

    unsigned char _250[0x264 - 0x250];
    /* 0x264: SOCOM 1's bits at 0x280 (its DWARF), zseal.h's SEAL_AI_PARAMS word */
    unsigned int m_debug : 1;
    unsigned int m_navyseal : 1;
    unsigned int m_noturn_travel : 1;
    unsigned int m_evasive_travel : 1;
    unsigned int m_creeping : 1;
    unsigned int m_nofade : 1;
    unsigned int m_nobindanimset : 1;
    unsigned int m_do_reaction : 1;
    unsigned int m_sleeping : 1;
    unsigned int m_snoozing : 1;
    unsigned int m_nosnooze : 1;
    unsigned int m_264_11 : 21;
    unsigned char _268[0x7c0 - 0x268];
};

class CSealCtrlSquirm : public CSealCtrl {
public:
    CSealCtrlSquirm();

    unsigned char _250[0x280 - 0x250];
};

/* a controller copying another's (SOCOM 1's CSealCtrlCopy: the player's controller under another
 * vtable; its constructor inline) */
class CSealCtrlCopy : public CSealCtrl {
public:
    virtual ~CSealCtrlCopy();
};

/* a character type (only its temporary types' cleanup and the lookup by name) */
class CCharacterType {
public:
    static void CleanupTemporary();
    static CCharacterType *GetByNameMod(const char *name);   /* OVL2 0x53E170 */
};

/* the SEAL (only what this unit touches) */

class CZSealBody : public CEntity {
public:
    static CZSealBody *Create(const CCharacterType *type, const char *name, CSealCtrl *ctrl);

    unsigned char _170[0x105c - 0x170];
    /* 0x105c: SOCOM 1's bits at 0xf58 (zseal.h's names) */
    unsigned int m_HudUpdateViewCone : 1;
    unsigned int m_hudwashit : 1;
    unsigned int m_aim_point_valid : 1;
    unsigned int m_invincible : 1;
    unsigned int m_invisible : 1;
    unsigned int m_infiniteammo : 1;
    unsigned int m_105c_6 : 26;
    unsigned char _1060_0 : 2;
    unsigned char m_inside : 1;   // 0x1060, bit 2 (SOCOM 1's at 0xf5c)

};

/* the SEALs' units (zseal's CSealUnit: only a SEAL's unit set up, OVL2 0x5EC9F0 -- SOCOM II's
 * second argument the unit to join, null for its own) */
class CSealUnit {
public:
    static void InitalizeSealUnit(CZSealBody *seal, CSealUnit *unit);
};

enum SEAL_CONTROL_TYPE {
    SEAL_CONTROL_PLAYER,
    SEAL_CONTROL_AI,
    SEAL_CONTROL_NETWORK,
    SEAL_CONTROL_SQUIRM,
    SEAL_CONTROL_COPY
};

/* the player (SOCOM 1's CftsPlayer: its SEAL at 0, the commands its statics) */
class CftsPlayer {
public:
    bool IsInside();
    static void RegisterAnimCommands();
    static bool CmdTickIsPlayerNode(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseIsPlayerNode(_zrdr *rdr);
    static bool CmdTickControls(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseControls(_zrdr *rdr);
    static bool CmdTickIndoorsTest(_zanim_cmd_hdr *hdr, float *dT);
    static _zanim_cmd_hdr *CmdParseIndoorsTest(_zrdr *rdr);

    /* no SEAL yet, the word after it set (the unit's initialiser 0x400330) */
    CftsPlayer() : m_body(0), m_4(1) {}

    CZSealBody *m_body;   // 0x0
    int m_4;   // 0x4
};


/* the spawners: the vehicles waiting to come back, one every two seconds (SOCOM 1's CSpawners and
 * its global, 32 bytes at 0x440C10) */
/* a vehicle's definition (zchar's CVehicleRdrEntry: only what the mission's filter reads -- its name
 * at 0, the teams it serves at 0x11C, a flag in bit 1 of the byte at 0x15C; the names ours) */
class CVehicleRdrEntry {
public:
    bool IsSingleOnly() { return m_15c_1 ? true : false; }
    bool IsRecycled() { return m_recycle ? true : false; }
    /* the start's place and direction (zchar.h's: OVL2 0x533000, 0x532FC0) and a start picked at
     * random (0x533040) */
    const char *GetStartPos() const;
    int GetStartDir() const;
    void Randomize(bool reset);

    char m_name[32];   // 0x0 (zchar.h's)
    char m_character[48];   // 0x20
    char m_display_name[32];   // 0x50
    unsigned char m_kind;   // 0x70: 1 a mission vehicle, 2 a spawned one (zchar.h's m_unk70)
    unsigned char _71[0x11c - 0x71];
    unsigned int m_teams;   // 0x11c
    AI_PARAMS m_aiParams;   // 0x120
    unsigned char m_debug : 1;   // 0x15c (zchar.h's bits)
    unsigned char m_15c_1 : 1;
    unsigned char m_nofade : 1;
    unsigned char m_noshoot : 1;
    unsigned char m_recycle : 1;   // bit 4 (zchar.h's m_recycle)
    unsigned char m_nosnooze : 1;
};

class CSpawners {
public:
    /* a vehicle waiting: its recycler and its entry. Its constructor is user-declared: the list's
     * insert (0x2B3DD0, LoadVehicles' push_back) copies the two words with lw/sw through a0, as an
     * element with a constructor is copied (without one, MW moves them through the FPRs). Its empty
     * destructor is the level the target's destructor chain shows below the range erase: with it the
     * deleter's destructor is five high in tail code, so ~list's copy (0x2B3F20) inlines ~__list_imp
     * one level and calls ~__list_deleter (0x2B3F80), which inlines the erase whole -- without it
     * ~list's copy inlines all three levels (a STAND-IN for the original MSL's deeper node allocator
     * maybe: one more level there gives the same chain, build/scratch/states/spawn1.cpp) */
    struct spawn_map {
        spawn_map(unsigned int i, CVehicleRdrEntry *e) : index(i), entry(e) {}
        ~spawn_map() {}

        unsigned int index;   // 0x0: the recycler's
        CVehicleRdrEntry *entry;   // 0x4
    };
    /* the waiting vehicles: a class over the list (the names ours). ~CSpawners' member destructor at
     * +0xC tests the list and calls ~list<spawn_map> (0x2B3F20) with 0, as a base -- and 0x2B3F20 is
     * itself two this-tests and a call to ~__list_deleter, a level inlined between: so the member is
     * one class above std::list (SOCOM 1's ~CSpawners agrees: three this-tests above its call to
     * ~__list_deleter, where its id list, a plain std::list, shows three above the deleter's body) */
    class spawn_list : public std::list<spawn_map> {
    };

    /* the lists built out of line, no vehicle counted (the unit's initialiser 0x400330); the
     * destructor inline, its copy in the template tail (0x2B4030) where the initialiser registers it:
     * Clear called (pinned below), the spawn list's chain through 0x2B3F20, the id list's three
     * levels inlined and its range erase called (0x2B40D0) */
    CSpawners() : m_count(0) {}
    ~CSpawners() { Clear(); }

    void Clear();
    void Tick(float dT);

    std::list<unsigned int> m_ids;   // 0x0
    spawn_list m_spawns;   // 0xc
    int m_count;   // 0x18
    float m_timer;   // 0x1c
};

CSpawners spawners;
CftsPlayer ftsPlayer;

/* the vehicles' rdr (zchar.h's: a vector of entries, only its opening and closing) */
class CVehicleRdr : public std::vector<CVehicleRdrEntry> {
public:
    void Open(const char *name);
    void Close();
};

/* the mission (only what this unit touches, at SOCOM II's offsets) */

/* the network game (fts_mission.cpp's CZNetGame: only the byte LoadVehicles sets) */
class CZNetGame {
public:
    unsigned char _0[0xdd];
    unsigned char m_dd;   // 0xdd
};

class CMission {
public:
    void RecycleEntity(CEntity *e);
    CEntity *LoadVehicle(CVehicleRdrEntry *entry, bool b);
    void RefreshTeamValves();
    /* a team's valve (ours: see RefreshTeamValves) */
    CValve *TeamValve(unsigned int team) { return m_team_valves[team]; }
    bool LoadVehicles();
    void InitalizeTeamValves();
    void CloseVehicleRdr();
    void OpenVehicleRdr();

    unsigned char _0[0x140];
    unsigned int m_vehicleFilter;   // 0x140: SOCOM II's (the name ours): which vehicles a mission takes
    unsigned char _144[0x180 - 0x144];
    float m_player_grid_spacing;   // 0x180 (fts_mission.cpp's m_ai_params.player_grid_spacing)
    unsigned int m_player_grid_count;   // 0x184 (its player_grid_count)
    float m_init_aim_pitch;   // 0x188 (fts_mission.cpp's: Read's "init_aim_pitch", in degrees)
    unsigned char _18c[0x1808 - 0x18c];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
    unsigned char _180c[0x1814 - 0x180c];
    CVehicleRdr *m_vehicle_rdr;   // 0x1814 (SOCOM 1's at 0x590)
    unsigned char _1818[0x1868 - 0x1818];
    CValve *m_team_valves[32];   // 0x1868 (SOCOM 1's at 0x5d4)
    CValve *m_player_valve_alive;   // 0x18e8 (SOCOM 1's at 0x654)
};

/* unnamed yet (SOCOM 1's names in the comments) */
extern CMission theMission;

extern "C" {
extern bool data_003DF188;   /* (SOCOM II's: the spawners wait while it is clear) */
void func_0026ADA0(CZAnimMain *zanim, zdb::CNode *node);
}

/* an rdr array's tag looked up (zrdr.h's) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);   /* 0x32F0D0 */

/* the squirm switch (SOCOM 1's file static) */
static bool doSquirm;

int sprintf(char *buf, const char *fmt, ...);
void *operator new(unsigned int size);

/* a controller's flag at 0x171, bit 7, set (SOCOM II's; OVL1 0x2B3C70, after the copy
 * controller's destructor) */
extern "C" void func_002B3C70(CSealCtrl *ctrl, bool on)
{
    ctrl->m_171_7 = on;
}

/* (OVL1 0x2B3B70: the copy controller's, linked after the unit's functions as SOCOM 1's is) */
CSealCtrlCopy::~CSealCtrlCopy()
{
}

/* the squirm flag (OVL1 0x2B3B60) */
bool ftsGetSquirm()
{
    return doSquirm;
}

/* a SEAL made with the controller its type asks for; the player's kept (OVL1 0x2B3A00). Its four
 * callees are named after it (SOCOM 1's ftsCreateSeal calls them from the same instructions):
 * spelled out through placeholders, a new-expression's null test no longer jumps to its own join
 * block. */
CZSealBody *ftsCreateSeal(const CCharacterType *type, const char *name, SEAL_CONTROL_TYPE control)
{
    CSealCtrl *ctrl;
    CZSealBody *body;

    if (!type)
        return 0;
    ctrl = 0;
    switch (control) {
    case SEAL_CONTROL_PLAYER:
        ctrl = new CSealCtrl;
        break;
    case SEAL_CONTROL_AI:
        ctrl = new CSealCtrlAi;
        break;
    case SEAL_CONTROL_NETWORK:
        break;
    case SEAL_CONTROL_SQUIRM:
        ctrl = new CSealCtrlSquirm;
        break;
    case SEAL_CONTROL_COPY:
        ctrl = new CSealCtrlCopy;
        break;
    }
    body = 0;
    if (ctrl) {
        body = CZSealBody::Create(type, name, ctrl);
        if (body && control == SEAL_CONTROL_PLAYER)
            ftsPlayer.m_body = body;
    }
    return body;
}

/* the player's SEAL indoors (OVL1 0x2B39D0) */
bool CftsPlayer::IsInside()
{
    if (m_body)
        return m_body->m_inside ? true : false;
    return false;
}

/* the player's ZAnim commands */
void CftsPlayer::RegisterAnimCommands()
{
    ZAnim.AddCmd("PLAYER_INDOORS", CmdParseIndoorsTest, 0, CmdTickIndoorsTest, 0);
    ZAnim.AddCmd("PLAYER_CONTROLS", CmdParseControls, 0, CmdTickControls, 0);
    ZAnim.AddCmd("IS_PLAYER_NODE", CmdParseIsPlayerNode, 0, CmdTickIsPlayerNode, 0);
}

/* PLAYER_INDOORS: a bare timeless command */
_zanim_cmd_hdr *CftsPlayer::CmdParseIndoorsTest(_zrdr *rdr)
{
    _zanim_cmd_hdr *cmd;

    if (!rdr)
        return 0;
    cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);
    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* PLAYER_INDOORS: whether the player's SEAL is indoors */
bool CftsPlayer::CmdTickIndoorsTest(_zanim_cmd_hdr *hdr, float *dT)
{
    if (ftsPlayer.m_body)
        return ftsPlayer.m_body->m_inside ? true : false;
    return false;
}

/* PLAYER_CONTROLS: ON gives the player the controls back, anything else takes them */
struct _player_controls_cmd {
    _zanim_cmd_hdr hdr;
    signed char off : 1;   // 0x4
    signed char on : 1;
};

_zanim_cmd_hdr *CftsPlayer::CmdParseControls(_zrdr *rdr)
{
    _player_controls_cmd *cmd;

    if (!rdr)
        return 0;
    cmd = (_player_controls_cmd *)_calloc(1, sizeof(_player_controls_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_player_controls_cmd);
    cmd->hdr.timeless = 1;
    if (zrdr_findtag(rdr, "ON"))
        cmd->on = 1;
    else
        cmd->off = 1;
    return (_zanim_cmd_hdr *)cmd;
}

bool CftsPlayer::CmdTickControls(_zanim_cmd_hdr *hdr, float *dT)
{
    if (ftsPlayer.m_body && ftsPlayer.m_body->m_control) {
        CSealCtrl *ctrl = (CSealCtrl *)ftsPlayer.m_body->m_control;

        if (((_player_controls_cmd *)hdr)->off)
            ctrl->m_input_enabled = false;
        else
            ctrl->m_input_enabled = true;
    }
    return true;
}

/* IS_PLAYER_NODE: a node of the anim's, and SEARCH_CHILDREN to take the player's node's
 * descendants too */
struct _is_player_node_cmd {
    _zanim_cmd_hdr hdr;
    unsigned char node;   // 0x4
    unsigned char search_children : 1;   // 0x5
};

_zanim_cmd_hdr *CftsPlayer::CmdParseIsPlayerNode(_zrdr *rdr)
{
    _is_player_node_cmd *cmd;
    CZAnim *anim = ZAnim.m_CurAnim;

    if (!rdr)
        return 0;
    cmd = (_is_player_node_cmd *)_calloc(1, sizeof(_is_player_node_cmd), 0, 0);
    cmd->hdr.data_size = sizeof(_is_player_node_cmd);
    cmd->hdr.timeless = 1;
    cmd->node = anim->LoadNodeNames(rdr, 0, 0, anim->node_search_scope);
    if (zrdr_findtag(rdr, "SEARCH_CHILDREN"))
        cmd->search_children = 1;
    return (_zanim_cmd_hdr *)cmd;
}

/* IS_PLAYER_NODE: whether the node is the player's (or, searching children, below it) */
bool CftsPlayer::CmdTickIsPlayerNode(_zanim_cmd_hdr *hdr, float *dT)
{
    _is_player_node_cmd *cmd = (_is_player_node_cmd *)hdr;
    zdb::CNode *node = ZAnim.m_CurAnim->GetNode(cmd->node, false);

    if (node) {
        zdb::CNode *player = ftsPlayer.m_body->GetNode();

        if (player == node)
            return true;
        if (cmd->search_children && player->FindChild(node, true))
            return true;
    }
    return false;
}

/* the player's SEAL (OVL1 0x2B3590 and 0x2B3580) */
void ftsSetPlayer(CZSealBody *body)
{
    ftsPlayer.m_body = body;
}

CZSealBody *ftsGetPlayer()
{
    return ftsPlayer.m_body;
}

/* a team block: its numbers as a mask, or the first number out of range (OVL1 0x2B34A0; `team >
 * 31`, not `>= 32`: only the first branches on slti into $at) */
unsigned int ParseTeamBlock(_zrdr *rdr)
{
    unsigned int mask = 0;

    if (!rdr || rdr->type != 4)
        return 0;
    for (unsigned int i = 0; i < rdr->Count(); i++) {
        _zrdr *item = rdr->Item(i);
        int team;

        if (item->type == 1)
            team = item->integer;
        else if (item->type == 2)
            team = (int)item->real;
        else
            team = 0;
        if (team < 0 || team > 31) {
            mask = team;
            break;
        }
        mask |= 1 << team;
    }
    return mask;
}

/* the vehicles' rdr (OVL1 0x2B3470 and 0x2B3430) */
void CMission::OpenVehicleRdr()
{
    if (m_vehicle_rdr)
        m_vehicle_rdr->Open("vehicles.rdr");
}

void CMission::CloseVehicleRdr()
{
    CCharacterType::CleanupTemporary();
    if (m_vehicle_rdr)
        m_vehicle_rdr->Close();
}

/* the network (znet.h's CZNetwork: only its two flags) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the vehicles' and turrets' loaders (zveh's, SOCOM 1's) */
class CTurret {
public:
    static void LoadTurrets();   /* 0x3AA930 */
};

class CVehicle {
public:
    static void LoadVehicles();   /* 0x3AE1A0 */
    static CEntity *Spawn(CVehicleRdrEntry *entry);   /* 0x3AE030 */
};

/* the gears' model cache (char_type.cpp's CCharacterModelCache: only the vector) */
class CCharacterModelCache {
public:
    static zdb::CModelVector m_ModelCache;
};

void zBoneResetCallbacks();
extern "C" float floorf(float);

/* the controller's player test (CSealCtrl's slot 0x30, fts_remote.cpp's IsPlayer) */
class CSealCtrlQuery {
public:
    virtual void _slot0();
    virtual void _slot1();
    virtual void _slot2();
    virtual void _slot3();
    virtual void _slot4();
    virtual void _slot5();
    virtual void _slot6();
    virtual void _slot7();
    virtual void _slot8();
    virtual bool Slot2C();   // 0x2c (unnamed: RefreshTeamValves asks it)
    virtual bool IsPlayer();   // 0x30
    virtual bool IsRemote();   // 0x34 (zseal.h's)
    virtual CSealCtrlAi *GetSealCtrlAi();   // 0x38 (zseal.h's: null but for an AI's)
};

/* the entity as a SEAL or a turret, if it is one (fts_remote.cpp's) */
inline CZSealBody *AsSeal(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
}

inline CEntity *AsTurret(CEntity *e)
{
    return e && e->m_type == CEntity::ENT_TYPE_TURRET ? e : 0;
}

/* whether a SEAL is a player's (its controller asked; fts_remote.cpp's) */
inline bool IsPlayerSeal(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrlQuery *)seal->m_control)->IsPlayer() : false;
}

extern "C" bool func_002B2D20(CMission *mission, CVehicleRdrEntry *entry, bool first);
extern "C" char data_0066B010[];

/* whether a vehicle is the debug filter's (ours) */
inline bool IsFilterVehicle(CVehicleRdrEntry *entry)
{
    return strcasecmp(data_0066B010, entry->m_name) == 0;
}

/* a recycled vehicle put on the spawners' list (ours): two inline levels with the insert written
 * out -- the spawn made a level down, after the grid's points, the insert's two iterator temporaries
 * at that level too, above the entity walk's end copy (as the target's frame has them: one level
 * gives the spawn above the points, push_back the iterators below the end copy) */
inline void AddSpawn(int index, CVehicleRdrEntry *entry)
{
    spawners.m_spawns.insert(spawners.m_spawns.end(), CSpawners::spawn_map(index, entry));
}

inline void AddSpawn(CVehicleRdrEntry *entry)
{
    AddSpawn(CEntity::CreateRecycler()->m_index, entry);
}

/* the first vehicle's copies placed on the mission's player grid, a copy loaded at every cell of the
 * smallest grid of `count` cells at least as wide as it is deep, the vehicle's matrix carrying the
 * cell (ours): an inline, so that its variables are numbered after the caller's -- and after the
 * entry iterator's scalarised temporary, which the target's colouring needs (cols, then rows,
 * declared in that order) */
inline void PlaceCopies(CMission *m, CVehicleRdrEntry *entry, CEntity *v)
{
    int count = m->m_player_grid_count;

    if (count) {
        int cols, rows;

        for (int d = (int)floorf(sqrtf((float)count)); d <= count; d++) {
            if (count % d == 0) {
                cols = d;
                rows = count / d;
                break;
            }
        }
        CPnt3D base;

        base.z = -m->m_player_grid_spacing;
        base.x = 0.5f * ((cols - 1) * m->m_player_grid_spacing);
        base.y = 0.0f;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                CPnt3D p = base;

                p.x += c * m->m_player_grid_spacing;
                p.z -= r * m->m_player_grid_spacing;
                v->m_node->m_matrix.Transform(&p, 1);
                CEntity *copy = m->LoadVehicle(entry, true);

                copy->TeleportTo(&p, &copy->m_quat);
            }
        }
    }
}

/* the mission's vehicles (SOCOM 1's CMission::LoadVehicles, its argument gone; OVL1 0x2B2E50): the
 * model cache emptied; online, a client loads nothing -- the session master marks the game and
 * looks for a player's SEAL and a turret already made; the turrets loaded if there is none, nothing
 * more with a player; offline, the vehicles; then every entry the mission takes: a recycled one
 * put on the spawners' list, the others loaded or spawned by their kind and numbered, the first
 * one copied for every player after the first and placed again on the mission's player grid.
 * The shape is the target's: the entity walk's iterator initialised (one begin() copy, the iterator
 * in s0 -- func_002B7A90 assigns its own, two copies and the iterator in memory), the counts tested
 * `> 1` (slti at), the filter's bool compared `== false` (materialised, then bnez), the grid's base
 * stored z, x, y field by field (SOCOM 1's order: the three-float constructor evaluates x first and
 * is 8 bytes longer), the spawn put through an inline helper two levels deep (AddSpawn above), the
 * grid placed through another (PlaceCopies above). The colouring (the states' rounds 32-33, MW's
 * replayed with build/scratch/states/ig/): the target pops c, r, rows, cols and then the entry
 * iterator (c s0, r s1, rows s2, cols s3, e s4) -- reached only with the iterator's scalarised temp
 * numbered before cols and rows, which no order of the function's own named locals gives (round 32:
 * best of 9000, 4 off) and the grid's variables as an inline's do (an inline's variables are numbered
 * after the caller's named locals and its temporaries). */
bool CMission::LoadVehicles()
{
    CCharacterModelCache::m_ModelCache.clear();
    if (theNetwork.m_bNetwork && !theNetwork.m_bSessionMaster)
        return true;
    zBoneResetCallbacks();
    bool player = false;
    bool turret = false;

    if (theNetwork.m_bNetwork) {
        m_pNetGame->m_dd = 1;
        for (CEntityList::const_iterator i = CEntity::GetEntityList().begin(); i != CEntity::GetEntityList().end();
             ++i) {
            CZSealBody *seal = AsSeal(*i);

            if (seal && IsPlayerSeal(seal))
                player = true;
            if (AsTurret(*i))
                turret = true;
        }
    }
    if (!turret)
        CTurret::LoadTurrets();
    if (player)
        return true;
    if (!theNetwork.m_bNetwork)
        CVehicle::LoadVehicles();
    if (ftsPlayer.m_4 > 1)
        m_vehicleFilter |= 8;
    CVehicleRdr *rdr = m_vehicle_rdr;
    int index = 0;

    for (CVehicleRdr::iterator e = rdr->begin(); e != rdr->end(); ++e, ++index) {
        bool first = index == 0;

        if (e->IsRecycled() && func_002B2D20(this, &*e, first) && IsFilterVehicle(&*e) == false)
            AddSpawn(&*e);
        else {
            CEntity *v = 0;

            switch (e->m_kind) {
            case 1:
                v = LoadVehicle(&*e, first);
                break;
            case 2:
                v = CVehicle::Spawn(&*e);
                break;
            }
            if (v)
                v->m_vehicle_index = index;
            if (first && v) {
                int players = ftsPlayer.m_4;

                if (players > 1) {
                    for (int k = 1; k < players; k++) {
                        v = LoadVehicle(&*e, first);
                        ((CSealCtrl *)AsSeal(v)->m_control)->m_copy_index = k;
                    }
                }
                PlaceCopies(this, &*e, v);
            }
        }
    }
    CEntity::UpdateTargetList(true);
    CCharacterModelCache::m_ModelCache.clear();
    return true;
}

/* whether the mission takes a vehicle (SOCOM II's; LoadVehicle and LoadVehicles ask it, OVL1 0x2B2D20):
 * one flagged single-player-only never online nor after the first; the first's teams kept, the
 * others' tested against them by the mission's filter bits; then a vehicle named in the debug
 * filter (OVL2 bss 0x66B010) only. The names ours */
extern "C" {
extern unsigned int data_00440C40;   /* the first vehicle's teams */
extern char data_0066B010[];   /* (a vehicle's name: the only one taken, when set) */
extern unsigned char data_0045A0C1;   /* theNetwork's online byte */
}

extern "C" bool func_002B2D20(CMission *mission, CVehicleRdrEntry *entry, bool first)
{
    bool ok;

    if (entry->IsSingleOnly() && (!first || data_0045A0C1))
        return false;
    unsigned int teams = entry->m_teams;

    ok = false;
    if (first) {
        data_00440C40 = teams;
        ok = true;
    } else {
        unsigned int filter = mission->m_vehicleFilter;

        if (!(filter & 1)) {
            ok = true;
            if (filter & 2)
                ok = (teams & data_00440C40) == 0;
            if (ok && (filter & 4))
                ok = (teams & data_00440C40) != 0;
            if (ok && (filter & 0x28))
                ok = (teams & 8) == 0;
            if (ok && (filter & 0x18))
                ok = (teams & 4) == 0;
        }
    }
    const char *only = data_0066B010;

    if (!first && *only)
        ok = strcasecmp(only, entry->m_name) == 0;
    return ok;
}

extern "C" unsigned char data_004412D8;   /* the sides swapped (fts_net_persona.cpp's) */

/* a vehicle's SEAL made (SOCOM 1's; OVL1 0x2B28D0): the entry's start drawn and the mission asked
 * whether it takes the vehicle (SOCOM 1's filter, moved out to 0x2B2D20); the character type's SEAL
 * made at the start's cell, turned its way -- the player's (squirm or copy as the switches say) or an
 * AI; online an AI's sides swapped when the game swaps them (bits 30 and 31), offline the player put
 * on team 26 and given the mission's initial aim pitch; the team mask and the no-shoot bit set; an
 * AI given the entry's bits and parameters (the entry kept) and its brain, the player the mission's
 * three cheats (bits 12-14 of its flags: invincible, invisible, infinite ammo) */
CEntity *CMission::LoadVehicle(CVehicleRdrEntry *entry, bool player)
{
    if (!entry)
        return 0;
    entry->Randomize(false);
    if (func_002B2D20(this, entry, player) == false)
        return 0;
    CCharacterType *type = CCharacterType::GetByNameMod(entry->m_character);

    if (!type)
        return 0;
    CPnt3D pos = CPnt3D::zero;
    CQuat quat = CQuat::identity;
    CAiMapLoc loc;

    if (AiMaps.GetLocation(entry->GetStartPos(), loc)) {
        AiMaps.GetCellCenter(loc, &pos);
        CAiMap::GetDirQuat((CAiMap::DIR)entry->GetStartDir(), quat, true);
    }
    SEAL_CONTROL_TYPE control;

    if (player) {
        if (doSquirm)
            control = SEAL_CONTROL_SQUIRM;
        else if (ftsPlayer.m_body)
            control = SEAL_CONTROL_COPY;
        else
            control = SEAL_CONTROL_PLAYER;
    } else
        control = SEAL_CONTROL_AI;
    CZSealBody *seal = ftsCreateSeal(type, entry->m_name, control);

    if (!seal)
        return 0;
    unsigned int teams = entry->m_teams;

    seal->m_node->SetName(entry->m_name);
    if (entry->m_display_name[0])
        seal->SetDisplayName(entry->m_display_name);
    seal->TeleportTo(&pos, &quat);
    if (theNetwork.m_bNetwork) {
        if (!player && data_004412D8) {
            unsigned int t = teams & 0x3fffffff;

            if (teams & 0x40000000)
                t |= 0x80000000;
            if (teams & 0x80000000)
                t |= 0x40000000;
            teams = t;
        }
    } else if (player) {
        teams |= 0x4000000;
        if (m_init_aim_pitch != 0.0f)
            ((CSealCtrl *)seal->m_control)->m_aimPitch = 0.017453292f * m_init_aim_pitch;
    }
    seal->SetTeamMask(teams);
    seal->m_noshoot = entry->m_noshoot != 0;
    CSealCtrlAi *ai = ((CSealCtrlQuery *)seal->m_control)->GetSealCtrlAi();

    if (ai) {
        ai->SetDebug(false);
        ai->m_nofade = entry->m_nofade != 0;
        ai->m_nosnooze = entry->m_nosnooze != 0;
        ai->m_aiParams = entry->m_aiParams;
        ai->m_aiParams.m_veh = entry;
        ai->InitAiBrain();
    } else if (player) {
        if (m_vehicleFilter & 0x1000)
            seal->m_invincible = true;
        if (m_vehicleFilter & 0x2000)
            seal->m_invisible = true;
        if (m_vehicleFilter & 0x4000)
            seal->m_infiniteammo = true;
    }
    return seal;
}

/* an entity given back: a SEAL's anims stopped, or, with vehicles waiting, the spawners' timer
 * restarted and one more owed (OVL1 0x2B2310; SOCOM 1's had only the second). The count is read
 * into a local before the timer's store: only so does the constant take v0 and the store go ahead
 * of the add (lui v0; lw v1; sw v0; addiu v0,v1,1), as in both builds; `m_count++` after the
 * store coalesces the sum with the load and stores both after the add. */
void CMission::RecycleEntity(CEntity *e)
{
    bool b = e->m_bit13;

    if (b || spawners.m_spawns.size()) {
        if (!b) {
            int n = spawners.m_count;

            spawners.m_timer = 2.0f;
            spawners.m_count = n + 1;
        } else
            func_0026ADA0(&ZAnim, e->GetNode());
        CEntity::RecycleEntity(e);
    }
}

/* a valve per AI team and the player's alive valve (OVL1 0x2B2840) */
void CMission::InitalizeTeamValves()
{
    char name[32];

    for (int i = 0; i < 32; i++) {
        sprintf(name, "aiteam_%02d", i);
        m_team_valves[i] = CValve::Create(name, 0, (VALVE_TYPE)2);
    }
    m_player_valve_alive = CValve::Create("player_valve_alive", 0, (VALVE_TYPE)2);
}

/* a SEAL's team in a network game: the three tests RefreshTeamValves asks in turn (SOCOM II's,
 * OVL1 0x2C31D0 / 0x2C30B0 / 0x2C2FD0; st_exit.cpp and hud_mpweaponsel.cpp call two of them) */
extern "C" {
bool func_002C31D0(CZSealBody *seal);
bool func_002C30B0(CZSealBody *seal);
bool func_002C2FD0(CZSealBody *seal);
}

/* whether the controller's slot 0x2C says yes (the slot unnamed: SOCOM 1's RefreshTeamValves asks the
 * same slot before setting the player's alive valve) */
inline bool CtrlSlot2C(CZSealBody *seal)
{
    return seal->m_control ? ((CSealCtrlQuery *)seal->m_control)->Slot2C() : false;
}

/* each AI team's valve recounted: none while a network client; all cleared (nothing at all when the
 * first was never made), the player's alive valve set from a player's SEAL, then each live entity
 * counted on its teams -- in a network game by the three team tests (SOCOM II's: teams 0 and 30, 8
 * and 31, or 16) --, and each vehicle waiting on its entry's teams (SOCOM 1's; OVL1 0x2B23A0). Both
 * builds index the valves afresh on every pass, which an accessor taking the team unsigned gives
 * (indexed straight, MW strength-reduces the int counter's walk into a pointer, as
 * InitalizeTeamValves' own loop is); one counter serves all three loops (each loop's own: the seal
 * and the counters trade s1/s2/s3 -- the states' round 42, read off MW's colouring) */
void CMission::RefreshTeamValves()
{
    if (theNetwork.m_bNetwork && !theNetwork.m_bSessionMaster)
        return;
    int i;

    for (i = 0; i < 32; i++) {
        if (!TeamValve(i) && i == 0)
            return;
        TeamValve(i)->SetValue(0);
    }
    CEntityList::const_iterator it;

    for (it = CEntity::GetEntityList().begin(); it != CEntity::GetEntityList().end(); ++it) {
        CEntity *e = *it;
        CZSealBody *seal = AsSeal(e);

        if (seal && CtrlSlot2C(seal) && m_player_valve_alive)
            m_player_valve_alive->SetValue(e->m_isAlive ? true : false);
        if (e->m_isAlive) {
            if (theNetwork.m_bNetwork && func_002C31D0(seal)) {
                TeamValve(0)->SetValue(TeamValve(0)->m_value + 1);
                TeamValve(30)->SetValue(TeamValve(30)->m_value + 1);
            } else if (theNetwork.m_bNetwork && func_002C30B0(seal)) {
                TeamValve(8)->SetValue(TeamValve(8)->m_value + 1);
                TeamValve(31)->SetValue(TeamValve(31)->m_value + 1);
            } else if (theNetwork.m_bNetwork && func_002C2FD0(seal))
                TeamValve(16)->SetValue(TeamValve(16)->m_value + 1);
            else {
                for (i = 0; i < 32; i++) {
                    if ((1 << i) & e->m_team)
                        TeamValve(i)->SetValue(TeamValve(i)->m_value + 1);
                }
            }
        }
    }
    for (CSpawners::spawn_list::iterator s = spawners.m_spawns.begin(); s != spawners.m_spawns.end(); ++s) {
        CVehicleRdrEntry *entry = (*s).entry;

        for (i = 0; i < 32; i++) {
            if ((1 << i) & entry->m_teams)
                TeamValve(i)->SetValue(TeamValve(i)->m_value + 1);
        }
    }
}

/* the next vehicle waiting brought back when the timer runs out, its SEAL given a unit
 * (OVL1 0x2B21D0) */
void CSpawners::Tick(float dT)
{
    CEntity *e;
    spawn_map *spawn;

    if (m_count > 0 && data_003DF188) {
        m_timer -= dT;
        if (m_timer <= 0.0f) {
            if (m_spawns.size()) {
                std::list<spawn_map>::node_base *const &first = m_spawns.end_node().next_;
                spawn = &((std::list<spawn_map>::node *)first)->data_;

                CEntity::OpenRecycler(spawn->index);
                e = theMission.LoadVehicle(spawn->entry, false);

                if (e) {
                    CEntity::RecycleEntity(spawn->index, e, true);
                    CSealUnit::InitalizeSealUnit(
                        e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0, 0);
                    e->OnMissionStart();
                }
                m_spawns.erase(std::list<spawn_map>::iterator(first));
                m_timer = 2.0f;
                CEntity::CloseRecycler();
            }
            m_count--;
        }
    }
}

/* every spawn forgotten (OVL1 0x2B2190). Pinned: ~CSpawners' copy calls it (SOCOM 1's inlines it),
 * where these headers' shallower chain would let it inline */
#pragma auto_inline off
void CSpawners::Clear()
{
    m_spawns.clear();
}
#pragma auto_inline reset
