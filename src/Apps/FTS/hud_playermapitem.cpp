/* hud_playermapitem.cpp -- a seal on the HUD's map (OVL1 0x222190-0x22328F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_playermapitem.cpp), the same functions in the same order,
 * from the implicit destructor to InitStaticItems. Not written yet: SetViewCone.
 * Defined in reverse of their image order (-inline deferred). */

#include "hud_playermapitem.h"
#include "hud_newhudmap.h"

class CEntityCtrl;

/* the node an entity is drawn by: its matrix first (znode.h's) */
namespace zdb {
class CNode {
public:
    CMatrix m_matrix;   // 0x0
};
}

/* a cell of an AI map (zentity.h's CAiMapLoc, SOCOM 1's): the map's id in the low six bits */
class CAiMapLoc {
public:
    unsigned int m_mapid : 6;
    unsigned int x : 13;
    unsigned int y : 13;
};

/* an entity as the blip reads it (CEntity's fields, zentity.h): its type (2, a SEAL), its
 * position and node, its controller, its team bits, the count of those aware of it, whether it is
 * alive and the map cell it was last validly in */
class CEntity {
public:
    static bool EntityIsValid(CEntity *entity);
    /* the position (inline: TickSealCone's pos is a temporary numbered after the SEAL's ternary,
     * the target's saved registers; taken plainly the two swap) */
    const CPnt3D *GetPosW() const { return &m_posW; }

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10
    unsigned char _11[0x1c - 0x11];
    CPnt3D m_posW;   // 0x1c
    zdb::CNode *m_node;   // 0x28
    unsigned char _2c[0xc0 - 0x2c];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0xdc - 0xcc];
    int m_aware_counter;   // 0xdc
    unsigned int m_bits0 : 12;   // 0xe0
    unsigned int m_isAlive : 1;
    CAiMapLoc m_mapLoc;   // 0xe4
    CAiMapLoc m_mapLocValid;   // 0xe8
};

/* whether one map is seen from another (fts_mission.cpp's) */
bool ftsIsMapInVisibleList(unsigned int map, unsigned int list);

class CZSealBody;
class CSealCtrlAi;

/* a SEAL's controller: CEntityCtrl's seven slots and CSealCtrl's (zseal.h), to its AI's */
class CEntityCtrl {
public:
    virtual ~CEntityCtrl();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CEntityCtrl *GetRemoteControl();
    virtual void SetThrottle(unsigned int which, float value);
};

/* a decaying value (zseal.h's CAiDecay): its value first */
class CAiDecay {
public:
    float m_value;   // 0x0
    float m_goal;   // 0x4
    float m_8;   // 0x8
};

/* the SEAL's controller (zseal.h's; SOCOM 1's DWARF names the view cone's two members, at the same
 * offsets in both builds) */
class CSealCtrl : public CEntityCtrl {
public:
    virtual void PlayAiSound(int sound);
    virtual bool AllowCommand(CZSealBody *seal, int command, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsAi() const;   // +0x30
    virtual bool IsRemote() const;   // +0x34
    virtual CSealCtrlAi *GetSealCtrlAi();   // +0x38

    unsigned char _4[0x68 - 0x4];
    float m_fovtan;   // 0x68: the view cone's half-width per unit of length
    unsigned char _6c[0xa8 - 0x6c];
    CAiDecay m_visrange;   // 0xa8: the view's range, the cone's length
};

enum AI_AWARE { AIA_UNAWARE = 0, AIA_SUSPICIOUS = 1, AIA_GENERAL = 2, AIA_DIRECTION = 3, AIA_SPECIFIC = 4 };

/* the AI: its alertness, and SOCOM 1's m_sleeping (bit 8 of the word at 0x264, zseal.h) */
class CSealCtrlAi : public CSealCtrl {
public:
    AI_AWARE GetAiAware() const;
    /* (zseal.h's: the bit materialised, `? true : false`) */
    bool IsSleeping() const { return m_sleeping ? true : false; }

    unsigned char _b4[0x264 - 0xb4];
    unsigned int m_bits0 : 8;   // 0x264
    unsigned int m_sleeping : 1;
};

class CZSealBody : public CEntity {
public:
    /* the way the SEAL looks (OVL2 0x5AF500, zseal's) */
    void GetLookDirW(CPnt3D *dir) const;
    bool IsAi() { return m_control ? ((CSealCtrl *)m_control)->IsAi() : false; }
    bool IsRemote() { return m_control ? ((CSealCtrl *)m_control)->IsRemote() : false; }
    CSealCtrlAi *GetSealCtrlAi() { return ((CSealCtrl *)m_control)->GetSealCtrlAi(); }
};

zdb::CTexture *CZPlayerMapItem::m_conetex;
zdb::CTexture *CZPlayerMapItem::m_alivebliptex;
zdb::CTexture *CZPlayerMapItem::m_deadbliptex;
C2DString CZPlayerMapItem::m_ablestring;
C2DString CZPlayerMapItem::m_bravostring;

/* the squad letters' strings: green, off */
void CZPlayerMapItem::InitStaticItems()
{
    m_ablestring.m_color[0] = 20.0f;
    m_ablestring.m_color[1] = 80.0f;
    m_ablestring.m_color[2] = 20.0f;
    m_ablestring.Off();
    m_bravostring.m_color[0] = 20.0f;
    m_bravostring.m_color[1] = 80.0f;
    m_bravostring.m_color[2] = 20.0f;
    m_bravostring.Off();
}

void CZPlayerMapItem::UnInitStaticItems()
{
    m_bravostring.SetString(0, true);
    m_ablestring.SetString(0, true);
}

/* a seal's map item: its blip and cone bitmaps, their clipped fans textured with the static
 * textures, alive, fading in, its name a dark blue */
CZPlayerMapItem::CZPlayerMapItem(CEntity *entity)
{
    m_wasalive = true;
    m_entity = entity;
    m_mybliptex = 0;
    m_myconetex = 0;
    m_myconetex = m_conetex;
    m_mybliptex = m_alivebliptex;
    m_clippedcone[0].m_texture = m_myconetex;
    m_clippedcone[1].m_texture = m_myconetex;
    m_clippedblip[0].m_texture = m_mybliptex;
    m_clippedblip[1].m_texture = m_mybliptex;
    m_oldpos = CPnt3D::zero;
    m_fadeval = 0.0f;
    m_fadein = true;
    m_namestring.m_color[0] = 10.0f;
    m_namestring.m_color[1] = 10.0f;
    m_namestring.m_color[2] = 40.0f;
}

/* the item faded in or out (about half a second either way) and its cone ticked; true once its
 * seal is gone (the map then drops it) */
bool CZPlayerMapItem::Tick(CZNewHudMap *map, float dT)
{
    if (!CEntity::EntityIsValid(m_entity))
        return true;
    if (m_fadein) {
        m_fadeval += 0.51f * dT;
        if (m_fadeval > 1.0f)
            m_fadeval = 1.0f;
    } else {
        m_fadeval -= 0.51f * dT;
        if (m_fadeval < 0.0f)
            m_fadeval = 0.0f;
    }
    TickSealCone(map);
    return false;
}

void CZPlayerMapItem::Clip(CZNewHudMap *map)
{
}

void CZPlayerMapItem::Draw(zdb::CCamera *camera)
{
    m_blip.Draw(camera);
    m_cone.Draw(camera);
    m_clippedcone[0].Draw(camera);
    m_clippedcone[1].Draw(camera);
    m_clippedblip[0].Draw(camera);
    m_clippedblip[1].Draw(camera);
}

void CZPlayerMapItem::PostDraw(zdb::CCamera *camera)
{
    m_namestring.Off();
}

/* the squad letters drawn once a frame, then turned off again */
void CZPlayerMapItem::DrawAlphaBravo(zdb::CCamera *camera)
{
    m_ablestring.Draw(camera);
    m_bravostring.Draw(camera);
    m_ablestring.Off();
    m_bravostring.Off();
}

/* the blip a viewer sees for this item's SEAL: 8 none (gone or dead); an AI's 9 asleep, 6, 0 or
 * 7 by its team bits, 0 a friend's, its alertness plus one when it is known to be seen (or the
 * enemies are shown), else 9; a remote player's 0 a friend's, 5 when seen (and all are shown),
 * else 9; anyone else's 0 a friend's, else 5 (SOCOM 1's; its two local statics need no guard
 * now) */
unsigned int CZPlayerMapItem::GetBlipID(CZSealBody *viewer)
{
    CEntity *entity = m_entity;

    if (!entity)
        return 8;
    CZSealBody *seal = entity && entity->m_type == 2 ? (CZSealBody *)entity : 0;

    if (!seal || !seal->m_isAlive)
        return 8;
    if (seal->IsAi()) {
        CSealCtrlAi *ai = seal->GetSealCtrlAi();

        if (ai->IsSleeping())
            return 9;
        AI_AWARE aw = ai->GetAiAware();

        if (seal->m_team & 8)
            return 6;
        if (seal->m_team & 4)
            return 0;
        if (seal->m_team & 0x20000)
            return 7;
        if (seal->m_team & viewer->m_team)
            return 0;
        static bool show_enemies = false;

        if (show_enemies || seal->m_aware_counter > 0)
            return aw + 1;
        return 9;
    } else if (seal->IsRemote()) {
        static bool show_all = true;

        if (seal->m_team & viewer->m_team)
            return 0;
        if (show_all && seal->m_aware_counter > 0)
            return 5;
        return 9;
    }
    /* (a byte: the select is zero-extended for the return, `andi 0xff`) */
    unsigned char result = (seal->m_team & viewer->m_team) ? 0 : 5;

    return result;
}

/* whether a map is seen from another: the same, or in its visible list (the name ours). The viewer's
 * map first: an inline's arguments evaluate right to left, so the entity's map is numbered first
 * and the two loads take the target's v1 / v0 -- with the compare written out (or the parameters
 * the other way round) they swap, with the compare's operands swapped the branch's do */
static inline bool MapSeen(unsigned int from, unsigned int map)
{
    return map == from ? true : ftsIsMapInVisibleList(map, from);
}

/* the item's six pictures off (TickSealCone's early outs; the name ours) */
static inline void AllOff(CZPlayerMapItem *item)
{
    item->m_blip.Off();
    item->m_cone.Off();
    item->m_clippedblip[0].Off();
    item->m_clippedblip[1].Off();
    item->m_clippedcone[0].Off();
    item->m_clippedcone[1].Off();
}

/* the seal's view cone on the map for the map's viewer: every picture off when the entity is gone
 * or not a SEAL, or the viewer shares no team with it or is on a map it cannot see; the dead blip
 * once, its cone dropped, when the SEAL has died; else the cone from the SEAL's look along its
 * controller's view range and cone width -- an AI's or a remote player's, the last cone kept when
 * the blip is 9 (unseen) -- or 200 units along its node's facing, 0.5 wide (SOCOM 1's
 * TickSealCone, 0x3DBE70 there: its DWARF's locals blipId, dir and pos; SOCOM II's position is
 * the entity's own, its map test by value -- round 58). The team test a ternary with its false arm
 * first, `== 0 ? false : ...` (the target lays the false arm out first; `x ? ... : false` puts the
 * true arm first, `!x ? false :` materialises the not with sltu / xori) */
void CZPlayerMapItem::TickSealCone(CZNewHudMap *map)
{
    if (!m_entity) {
        AllOff(this);
        return;
    }
    CZSealBody *seal = m_entity && m_entity->m_type == 2 ? (CZSealBody *)m_entity : 0;

    if (!seal) {
        AllOff(this);
        return;
    }
    CZSealBody *viewer = map->m_viewer;

    if (!viewer)
        return;
    if (!((viewer->m_team & m_entity->m_team) == 0
              ? false
              : MapSeen(viewer->m_mapLocValid.m_mapid, m_entity->m_mapLocValid.m_mapid))) {
        AllOff(this);
        return;
    }
    if (m_wasalive && !seal->m_isAlive) {
        m_myconetex = 0;
        m_mybliptex = m_deadbliptex;
        m_clippedcone[0].m_texture = m_myconetex;
        m_clippedcone[1].m_texture = m_myconetex;
        m_clippedblip[0].m_texture = m_mybliptex;
        m_clippedblip[1].m_texture = m_mybliptex;
        m_wasalive = false;
        m_blip.Off();
        m_cone.Off();
        return;
    }

    unsigned int blipId;
    CPnt3D dir;
    const CPnt3D *pos = seal->GetPosW();
    CSealCtrlAi *ai = seal ? seal->GetSealCtrlAi() : 0;
    CSealCtrl *remote = seal ? (CSealCtrl *)seal->m_control : 0;

    blipId = GetBlipID(viewer);
    if (ai) {
        seal->GetLookDirW(&dir);
        if (blipId == 9) {
            m_fadein = false;
            SetViewCone(map, &m_oldrot, &m_oldpos, blipId, ai->m_visrange.m_value, ai->m_fovtan);
        } else {
            m_fadein = true;
            SetViewCone(map, &dir, pos, blipId, ai->m_visrange.m_value, ai->m_fovtan);
            m_oldrot = dir;
            m_oldpos = *pos;
        }
    } else if (seal->IsRemote() && remote) {
        seal->GetLookDirW(&dir);
        if (blipId == 9) {
            m_fadein = false;
            SetViewCone(map, &m_oldrot, &m_oldpos, blipId, remote->m_visrange.m_value, remote->m_fovtan);
        } else {
            m_fadein = true;
            SetViewCone(map, &dir, pos, blipId, remote->m_visrange.m_value, remote->m_fovtan);
            m_oldrot = dir;
            m_oldpos = *pos;
        }
    } else {
        zdb::CNode *node = seal->m_node;

        dir.x = -node->m_matrix(2, 0);
        dir.y = -node->m_matrix(2, 1);
        dir.z = -node->m_matrix(2, 2);
        SetViewCone(map, &dir, pos, blipId, 200.0f, 0.5f);
    }
}


void CZPlayerMapItem::MissionEndCleanup()
{
    m_ablestring.ClearAndWipe();
    m_bravostring.ClearAndWipe();
}
