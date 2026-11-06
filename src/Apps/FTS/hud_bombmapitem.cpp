/* hud_bombmapitem.cpp -- the multiplayer bomb on the HUD's map (OVL1 0x20CF40-0x20D38F): SOCOM 1's
 * unit (its DWARF names it, Z:\dev\Apps\FTS\hud_bombmapitem.cpp): the implicit destructor, Draw,
 * Tick and the constructor in SOCOM 1's order. Defined in reverse of their image order
 * (-inline deferred). */

#include "hud_bombmapitem.h"
#include "hud_newhudmap.h"

/* the network game as the bomb reads it (hud_posebitmap.cpp's CZNetwork; the mission's net game at
 * SOCOM II's 0x1808, zanim_menu.h; fts_net_seal.cpp's func_002A72A0: a game type 1 or 2) */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

class CZNetGame;

class CMission {
public:
    unsigned char _0[0x1808];
    CZNetGame *m_pNetGame;   // 0x1808 (SOCOM 1's at 0x584)
};

extern CMission theMission;

extern "C" bool func_002A72A0(CZNetGame *game);

/* the bomb's state (SOCOM 1's DWARF: TypeBombStatus, its two state bytes first) */
class TypeBombStatus {
public:
    unsigned char CurrentBombState;   // 0x0 (EnumBombState: 2 gone, 5 detonated, as the map reads it)
    unsigned char NewBombState;   // 0x1
};

class CZBombState {
public:
    static TypeBombStatus m_BombStatus;
};

/* the node's position, copied out of its matrix's translation row (zdb's CNode::GetPosition: the
 * matrix first) */
static inline CPnt3D NodePos(const zdb::CNode *node)
{
    return *(const CPnt3D *)((const CMatrix *)node)->m_matrix[3];
}

zdb::CTexture *CZMPBombMapItem::m_mpbombtex;
float CZMPBombMapItem::m_U0;
float CZMPBombMapItem::m_V0;
float CZMPBombMapItem::m_U1 = 1.0f;
float CZMPBombMapItem::m_V1 = 1.0f;
zdb::CTexture *CZMPBombMapItem::m_explosiontex;
float CZMPBombMapItem::m_explosionU0;
float CZMPBombMapItem::m_explosionV0;
float CZMPBombMapItem::m_explosionU1;
float CZMPBombMapItem::m_explosionV1;

/* the bomb's node kept, a red bitmap */
CZMPBombMapItem::CZMPBombMapItem(zdb::CNode *node)
{
    m_node = node;
    m_wasdetonated = false;
    m_size = 3.0f;
    m_graphic.SetColor(255.0f, 0.0f, 0.0f);
}

/* the bomb shown where its node is until the network game's bomb state says it went off (or is
 * gone: true, the map drops it), then a burst grown in place, true once past 154 (SOCOM 1's Tick;
 * the game-type test SOCOM II's call).
 * Byte-exact since round 65: the grown size read back through a reference local, as CZClaymoreMapItem::Tick */
bool CZMPBombMapItem::Tick(CZNewHudMap *map, float dT)
{
    if (!m_wasdetonated) {
        if (!m_graphic.IsOn())
            m_graphic.On();
        if (m_node && !m_wasdetonated && theNetwork.m_bNetwork && theMission.m_pNetGame
            && func_002A72A0(theMission.m_pNetGame)) {
            TypeBombStatus *pBombStatus = &CZBombState::m_BombStatus;

            if (pBombStatus->CurrentBombState == 5)
                m_wasdetonated = true;
            else if (pBombStatus->CurrentBombState == 2)
                return true;
        }
        CPnt3D pos = NodePos(m_node);

        map->m_matrix.Transform(&pos, 1);
        const float &size = m_size;

        m_graphic.Load(pos.x - m_size, map->CalcDrawPos(pos.y - size), pos.x - size,
                       map->CalcDrawPos(pos.y + size), pos.x + size, map->CalcDrawPos(pos.y + size),
                       pos.x + size, map->CalcDrawPos(pos.y - size), m_mpbombtex);
        m_graphic.SetUV(m_U0, m_V0, m_U1, m_V1);
        return false;
    }
    CPnt3D pos = NodePos(m_node);

    map->m_matrix.Transform(&pos, 1);
    m_size += 10.0f * dT;
    float size = m_size;

    m_graphic.Load(pos.x - m_size, map->CalcDrawPos(pos.y - size), pos.x - size,
                   map->CalcDrawPos(pos.y + size), pos.x + size, map->CalcDrawPos(pos.y + size),
                   pos.x + size, map->CalcDrawPos(pos.y - size), m_explosiontex);
    m_graphic.SetUV(m_explosionU0, m_explosionV0, m_explosionU1, m_explosionV1);
    return m_size > 154.0f;
}

void CZMPBombMapItem::Draw(zdb::CCamera *camera)
{
    m_graphic.Draw(camera);
}
