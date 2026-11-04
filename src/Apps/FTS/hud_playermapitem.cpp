/* hud_playermapitem.cpp -- a seal on the HUD's map (OVL1 0x222190-0x22328F): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_playermapitem.cpp), the same functions in the same order,
 * from the implicit destructor to InitStaticItems. Not written yet: SetViewCone and TickSealCone.
 * Defined in reverse of their image order (-inline deferred). */

#include "hud_playermapitem.h"

class CEntityCtrl;

/* an entity as the blip reads it (CEntity's fields, zentity.h): its type (2, a SEAL), its
 * controller, its team bits, the count of those aware of it and whether it is alive */
class CEntity {
public:
    static bool EntityIsValid(CEntity *entity);

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10
    unsigned char _11[0xc0 - 0x11];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0xdc - 0xcc];
    int m_aware_counter;   // 0xdc
    unsigned int m_bits0 : 12;   // 0xe0
    unsigned int m_isAlive : 1;
};

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

class CSealCtrl : public CEntityCtrl {
public:
    virtual void PlayAiSound(int sound);
    virtual bool AllowCommand(CZSealBody *seal, int command, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsAi() const;   // +0x30
    virtual bool IsRemote() const;   // +0x34
    virtual CSealCtrlAi *GetSealCtrlAi();   // +0x38
};

enum AI_AWARE { AIA_UNAWARE = 0, AIA_SUSPICIOUS = 1, AIA_GENERAL = 2, AIA_DIRECTION = 3, AIA_SPECIFIC = 4 };

/* the AI: its alertness, and SOCOM 1's m_sleeping (bit 8 of the word at 0x264, zseal.h) */
class CSealCtrlAi {
public:
    AI_AWARE GetAiAware() const;
    /* (zseal.h's: the bit materialised, `? true : false`) */
    bool IsSleeping() const { return m_sleeping ? true : false; }

    unsigned char _0[0x264];
    unsigned int m_bits0 : 8;   // 0x264
    unsigned int m_sleeping : 1;
};

class CZSealBody : public CEntity {
public:
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

void CZPlayerMapItem::MissionEndCleanup()
{
    m_ablestring.ClearAndWipe();
    m_bravostring.ClearAndWipe();
}
