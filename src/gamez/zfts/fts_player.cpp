/* fts_player.cpp -- the spawners, the mission's team valves and vehicles, and the player: its
 * ZAnim commands, its SEAL's creation and the copy controller (SOCOM 1's order: CSpawners' code
 * opens its fts_player.cpp, after fts_mission.cpp's template instances, and the CSpawners list's
 * instances close it, before fts_action.cpp's CNodeAction code). OVL1 0x2B2190-0x2B418F; its
 * static initialiser is 0x400330. Defined in reverse of their image order (-inline deferred). */

#include "../zentity/zentity.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"

/* a SEAL's controller (SOCOM 1's CSealCtrl, 592 bytes at SOCOM II's offsets: only what this unit
 * touches) and the three kinds ftsCreateSeal makes besides */
class CSealCtrl {
public:
    CSealCtrl();
    virtual ~CSealCtrl();

    unsigned char _4[0x171 - 0x4];
    unsigned char _171_0 : 3;
    unsigned char m_input_enabled : 1;   // 0x171, bit 3 (SOCOM 1's at 0x169)
    unsigned char _171_4 : 3;
    unsigned char m_171_7 : 1;   // 0x171, bit 7
    unsigned char _172[0x250 - 0x172];
};

class CSealCtrlAi : public CSealCtrl {
public:
    CSealCtrlAi();

    unsigned char _250[0x7c0 - 0x250];
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

/* a character type (only its temporary types' cleanup) */
class CCharacterType {
public:
    static void CleanupTemporary();
};

/* the SEAL (only what this unit touches) */

class CZSealBody : public CEntity {
public:
    static CZSealBody *Create(const CCharacterType *type, const char *name, CSealCtrl *ctrl);

    unsigned char _170[0x1060 - 0x170];
    unsigned char _1060_0 : 2;
    unsigned char m_inside : 1;   // 0x1060, bit 2 (SOCOM 1's at 0xf5c)

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

    CZSealBody *m_body;   // 0x0
};

extern CftsPlayer ftsPlayer;

/* the spawners: the vehicles waiting to come back, one every two seconds (SOCOM 1's CSpawners and
 * its global, 32 bytes at 0x440C10) */
class CVehicleRdrEntry;

class CSpawners {
public:
    struct spawn_map {
        unsigned int index;   // 0x0: the recycler's
        CVehicleRdrEntry *entry;   // 0x4
    };

    void Clear();
    void Tick(float dT);

    std::list<unsigned int> m_ids;   // 0x0
    std::list<spawn_map> m_spawns;   // 0xc
    int m_count;   // 0x18
    float m_timer;   // 0x1c
};

extern CSpawners spawners;

/* the vehicles' rdr (only its opening and closing) */
class CVehicleRdr {
public:
    void Open(const char *name);
    void Close();
};

/* the mission (only what this unit touches, at SOCOM II's offsets) */

class CMission {
public:
    void RecycleEntity(CEntity *e);
    CEntity *LoadVehicle(CVehicleRdrEntry *entry, bool b);
    void InitalizeTeamValves();
    void CloseVehicleRdr();
    void OpenVehicleRdr();

    unsigned char _0[0x1814];
    CVehicleRdr *m_vehicle_rdr;   // 0x1814 (SOCOM 1's at 0x590)
    unsigned char _1818[0x1868 - 0x1818];
    CValve *m_team_valves[32];   // 0x1868 (SOCOM 1's at 0x5d4)
    CValve *m_player_valve_alive;   // 0x18e8 (SOCOM 1's at 0x654)
};

/* unnamed yet (SOCOM 1's names in the comments) */
extern CMission theMission;

extern "C" {
extern bool data_003DF188;   /* (SOCOM II's: the spawners wait while it is clear) */
void func_005EC9F0(CZSealBody *seal, int arg);   /* CSealUnit::InitalizeSealUnit (SOCOM II's second
                                                    argument) */
void func_0026ADA0(CZAnimMain *zanim, zdb::CNode *node);

CValve *func_003520D0(const char *name, unsigned int value, VALVE_TYPE type);   /* CValve::Create,
                                     SOCOM 1's with a value: zanim.h's CValve lacks the overload */
}

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
    if (func_0032F0D0(rdr, "ON"))
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
    if (func_0032F0D0(rdr, "SEARCH_CHILDREN"))
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
        m_team_valves[i] = func_003520D0(name, 0, (VALVE_TYPE)2);
    }
    m_player_valve_alive = func_003520D0("player_valve_alive", 0, (VALVE_TYPE)2);
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
                    func_005EC9F0(e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0, 0);
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

/* every spawn forgotten (OVL1 0x2B2190) */
void CSpawners::Clear()
{
    m_spawns.clear();
}
