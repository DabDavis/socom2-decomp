/* hud_playermapitem.cpp -- a seal on the HUD's map (OVL1 0x222190-0x22328F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_playermapitem.cpp), the same functions in the same order,
 * from the implicit destructor to InitStaticItems. Not written yet: SetViewCone, TickSealCone and
 * GetBlipID. Defined in reverse of their image order (-inline deferred). */

#include "hud_playermapitem.h"

class CEntity {
public:
    static bool EntityIsValid(CEntity *entity);
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

void CZPlayerMapItem::MissionEndCleanup()
{
    m_ablestring.ClearAndWipe();
    m_bravostring.ClearAndWipe();
}
