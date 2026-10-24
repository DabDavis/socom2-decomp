/* hud_mapcompass.cpp -- the HUD's compass (OVL1 0x211510-...): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_mapcompass.cpp), its small functions in SOCOM 1's order after Tick. SOCOM
 * II's Draw is empty, SetPos scales the rectangle by ztwod's screen scale, and On shows the
 * compass only for the local seal while it lives. Not written yet: Tick, the destructor (it frees
 * a static vector of nav packages), the constructor and InitHUD_Compass. Defined in reverse of
 * their image order (-inline deferred). */

#include "hud_mapcompass.h"
#include "../../gamez/znode/znode.h"

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
};

/* only what On reads (names ours) */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4: tested before the compass shows
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x200 - 0xe2];
    signed char m_socom2_200;   // 0x200: under 2 while the seal lives
    bool IsDead() { return m_socom2_200 >= 2; }
};

/* ztwod's screen scale, x then y (no name yet) */
extern float data_0049E9F0[2];

/* the seal the camera follows (no name yet) */
extern "C" CZSealBody *func_002B3580();

void MapCompass::SetScale(float scale)
{
    m_scale = scale;
}

/* shown with no owner, or for the local seal while it lives and is marked; hidden otherwise (both
 * tests as bools first: the target materialises each before it branches) */
void MapCompass::On()
{
    CZSealBody *owner = CHUD::GetHUDOwner();

    if (owner) {
        bool local = owner == func_002B3580();
        bool dead = owner->IsDead();

        if (!owner->m_socom2_e1_4 || dead || !local) {
            m_poly.Off();
            m_anchor_head.Off();
            return;
        }
    }
    m_poly.On();
    m_anchor_head.On();
}

/* the strip and the anchors (the list head's children) hidden */
void MapCompass::Off()
{
    m_poly.Off();
    m_anchor_head.Off();
}

/* the strip placed (scaled to the screen), its middle kept, and the bitmap loaded around the
 * middle at the compass's scale */
void MapCompass::SetPos(float topx, float topy, float botx, float boty)
{
    m_compasstopx = topx * data_0049E9F0[0];
    m_compasstopy = topy * data_0049E9F0[1];
    m_compassbotx = botx * data_0049E9F0[0];
    m_compassboty = boty * data_0049E9F0[1];
    m_compasscenterx = (m_compasstopx + m_compassbotx) / 2.0f;
    m_compasscentery = (m_compasstopy + m_compassboty) / 2.0f;
    float top = m_compasscentery + m_scale * (m_compasstopy - m_compasscentery);
    float bot = m_compasscentery + m_scale * (m_compassboty - m_compasscentery);

    m_poly.Load(m_compasstopx, top, m_compasstopx, bot, m_compassbotx, bot, m_compassbotx, top,
                m_newcompasstex);
}

void MapCompass::SetTexture(zdb::CTexture *tex)
{
    m_newcompasstex = tex;
    m_tex_width = tex ? (int)tex->m_width : 0.0f;   /* through int: MW converts unsigned the long way */
    m_tex_height = tex ? (int)tex->m_height : 0.0f;
}

void MapCompass::SetTrans(float trans)
{
    m_poly.SetTrans(trans);
}

void MapCompass::Draw(zdb::CCamera *camera)
{
}
