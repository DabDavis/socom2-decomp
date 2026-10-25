/* hud_weaponsel.cpp -- the single-player weapon selector (CWeaponSel): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_weaponsel.cpp), its functions in SOCOM 1's order (OVL1
 * 0x219330-0x21DE4F: PlaceWeaponText ... OnClose, the state handlers, OnOpen, UnInit, Init, the
 * constructor). Written: AfterClose (0x21AAA0), AfterCloseFirst (0x21B0A0), OnClose (0x21B3E0)
 * and UnInit (0x21CBA0), which inlines OnClose as SOCOM 1's called it. The layout is hud_weaponsel.h's.
 * Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"
#include "hud_weaponsel.h"

/* the seal's kit: the reticule switch (SOCOM 1's SetCurReticule, a flag added), its items and
 * their icons */
class CZKit {
public:
    void SetCurReticule(bool flag);
    int GetNumItems() const;
    zdb::CTexHandle *GetIcon(int index);
    int GetNumMags(int index);
    const char *GetItemName(int index);
    class CZFTSWeapon *GetItemByIndex(int index) const;
};

/* a kit item as the selector reads it: its type byte (11: no ammunition texts) */
class CZFTSWeapon {
public:
    unsigned char _0[0x7c];
    unsigned char m_type;   // 0x7c
};

/* SOCOM 1's CZKit::GetAmmoCount and GetMaxAmmoCount (OVL2 0x5C38A0, 0x5C3CF0: the rows unnamed,
 * called as C) */
extern "C" int func_005C38A0(CZKit *kit, int index);
extern "C" int func_005C3CF0(CZKit *kit, int index);

/* a string-table string formatted with a number (hud_main.cpp's tail, 0x1FE120; no row named) */
extern "C" void func_001FE120(char *buf, int size, int id, int n);

/* SOCOM 1's CZKit::isSelectableItem (OVL2 0x5BDC30, the row unnamed: called as C) */
extern "C" int func_005BDC30(CZKit *kit, int index);

/* the seal's controller as the selector reads it: the pad it reads (SOCOM 1's at 0xE8) */
class CSealCtrl {
public:
    unsigned char _0[0x114];
    unsigned int m_padindex;   // 0x114
    unsigned char _118[0x224 - 0x118];
    float m_hotswap1;   // 0x224: the item the first hot-swap button selects (a float, read as an int)
    float m_hotswap2;   // 0x228: the second's
};

/* a localised string by its id (the string table's; 0x361B40, no row named yet) */
extern "C" const char *func_00361B40(int id);

/* SOCOM 1's CSealCtrl::SetMenuState with a flag added (OVL2; the row unnamed: called as C) */
extern "C" void func_00597740(CSealCtrl *ctrl, int state, int flag);

/* the seal as the selector reads it: its controller and kit */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4: the selector opens only with it (hud_main.cpp's)
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x5e0 - 0xe2];
    CZKit m_kit;   // 0x5e0

    CZKit *GetKit() { return &m_kit; }
};

/* the selector's sounds: the second slot plays one (zsnd.h's PlaySound(inst, flags)) */
class CSnd {
public:
    virtual ~CSnd();
    virtual void *PlaySound(void *inst, unsigned int flags);

    static CSnd *GetSoundByName(const char *name);
};

/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* the HUD as the selector reads it: the reticule flag, the owner */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    static void HudText_to_upper(char *text);
    static PNT4D m_hud_textcolor;

    unsigned char _0[0xb1];
    bool m_usehudret;   // 0xb1
};

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

/* a texture's size, first (c2dbitmap.cpp's) */
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

/* a texture's handle: its name, and the texture at 0xC */
class CTexHandle {
public:
    const char *m_name;   // 0x0
    unsigned char _4[0xc - 0x4];
    CTexture *m_texture;   // 0xc
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* a texture of the world's by name (hud_main.cpp's) */
zdb::CTexture *GetTextureByName(zdb::CWorld *world, const char *name);


/* the corner boxes' and the off arrow's textures (hud_main.cpp's data; SOCOM 1's
 * c_szCornerBoxesTx) */
extern const char *data_003E3048;
extern const char *data_003E3068;

/* the selector's width, and its rows' tops (the unit's statics: its initialiser sets them from
 * the HUD's layout) */
extern int data_0040C108;
extern float data_003E4A90;
extern float data_003E4A98;
extern float data_003E4AA0;

/* the HUD's backdrop alpha (hud_main.cpp's layout float, 76.8) */
extern float data_003E3000;

/* the backdrops' fade-in clock: -1 idle, counting up to half a second (SOCOM 1's
 * g_fFadeInTime, the unit's anonymous namespace: its symbol table names it at gp-30408) */
namespace {
float g_fFadeInTime = -1.0f;
}

/* SOCOM 1's CCtrlrConfigs::GetButtonState(PadBtnFunc, const CPad &) (0x2C64E0, no row named) and
 * CZSealBody::IsChangingWeapons with a flag added (OVL2 0x5A7980): called as C */
extern "C" unsigned char func_002C64E0(int func, CPad *pad);
extern "C" bool func_005A7980(CZSealBody *seal, int flag);

/* SOCOM II's two per-tick updates after the state's (no rows named yet) */
extern "C" void func_0021A1F0(CWeaponSel *sel);

/* the textures fetched, the backdrops and backings loaded, the texts coloured and loaded off, the
 * pieces listed (SOCOM 1's, the backings three members now) */
void CWeaponSel::Init(zdb::CWorld *world, C2DFont *font)
{
    data_0040C108 = 160;
    m_openstate = 0;
    m_weapontex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weapontex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("reticuletex.tif");
    m_weaponselbkrndtex = zdb::CAssetMgr::m_assets.GetTexHandle(data_003E3048);
    m_arrowtex = GetTextureByName(0, data_003E3068);
    m_myfont = font;
    m_selectionback2.SetHasTexture(false);
    m_selectionback2.Load(0.0f, data_003E4A98, data_0040C108, 2.0f + (64.0f + data_003E4A98), 0);
    m_selectionback2.SetColor(80.0f, 80.0f, 80.0f);
    m_selectionback2.SetIsTrans(true);
    m_selectionback2.SetTrans(0.0f);
    m_selectionback2.m_fade_dx = 2.0f;
    m_selectionback2.Off();
    push_back(&m_weaponselbkrnd);
    push_back(&m_selectionback2);
    push_back(&m_weaponicon[0]);
    push_back(&m_weaponicon[3]);
    push_back(&m_weaponicon[2]);
    push_back(&m_weaponicon[1]);
    m_arrow[0].Off();
    m_arrow[1].Off();
    m_arrow[0].SetUV(0.0f, 0.05f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.05f);
    m_arrow[0].SetTrans(70.0f);
    m_arrow[1].SetTrans(70.0f);
    m_arrow[0].SetColor(50.0f, 50.0f, 50.0f);
    m_arrow[1].SetColor(50.0f, 50.0f, 50.0f);
    m_arrowcolor = 50.0f;
    m_arrowincrement = 1;
    push_back(&m_arrow[0]);
    push_back(&m_arrow[1]);
    m_hotswapL1text.m_color[0] = 55.0f;
    m_hotswapL1text.m_color[1] = 140.0f;
    m_hotswapL1text.m_color[2] = 130.0f;
    m_hotswapL1text.m_color[3] = 80.0f;
    push_back(&m_hotswapL1text);
    m_hotswapL2text.m_color[0] = 55.0f;
    m_hotswapL2text.m_color[1] = 140.0f;
    m_hotswapL2text.m_color[2] = 130.0f;
    m_hotswapL2text.m_color[3] = 80.0f;
    push_back(&m_hotswapL2text);
    m_weaponname.m_color[0] = CHUD::m_hud_textcolor.x;
    m_weaponname.m_color[1] = CHUD::m_hud_textcolor.y;
    m_weaponname.m_color[2] = CHUD::m_hud_textcolor.z;
    m_weaponname.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_weaponname);
    m_weapondescription.m_color[0] = CHUD::m_hud_textcolor.x;
    m_weapondescription.m_color[1] = CHUD::m_hud_textcolor.y;
    m_weapondescription.m_color[2] = CHUD::m_hud_textcolor.z;
    m_weapondescription.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_weapondescription);
    m_ammotype.m_color[0] = CHUD::m_hud_textcolor.x;
    m_ammotype.m_color[1] = CHUD::m_hud_textcolor.y;
    m_ammotype.m_color[2] = CHUD::m_hud_textcolor.z;
    m_ammotype.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_ammotype);
    m_ammocount.m_color[0] = CHUD::m_hud_textcolor.x;
    m_ammocount.m_color[1] = CHUD::m_hud_textcolor.y;
    m_ammocount.m_color[2] = CHUD::m_hud_textcolor.z;
    m_ammocount.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_ammocount);
    m_magcount.m_color[0] = CHUD::m_hud_textcolor.x;
    m_magcount.m_color[1] = CHUD::m_hud_textcolor.y;
    m_magcount.m_color[2] = CHUD::m_hud_textcolor.z;
    m_magcount.m_color[3] = CHUD::m_hud_textcolor.w;
    push_back(&m_magcount);
    push_back(&m_selectionback0);
    push_back(&m_selectionback1);
    m_weaponselbkrnd.Off();
    m_selectionback0.Off();
    m_selectionback1.Off();
    for (int i = 0; i < 4; i++)
        m_weaponicon[i].Off();
    m_weaponname.Off();
    m_weapondescription.Off();
    m_magcount.Off();
    m_ammocount.Off();
    m_ammotype.Off();
    m_hotswapL1text.Off();
    m_hotswapL2text.Off();
    m_menuposx[1] = 30.0f;
    m_menuposy[1] = data_003E4A98;
    m_weaponselbkrnd.Load(0.0f, 5.0f + data_003E4AA0, data_0040C108, data_003E4A90 - 5.0f,
                          m_weaponselbkrndtex);
    m_selectionback0.Load(0.0f, data_003E4AA0, data_0040C108, 5.0f + data_003E4AA0, m_weaponselbkrndtex);
    m_selectionback1.Load(0.0f, data_003E4A90 - 5.0f, data_0040C108, data_003E4A90, m_weaponselbkrndtex);
    m_weaponicon[1].Load(m_menuposx[1] + m_iconoffsetx, m_menuposy[1] + m_iconoffsety, m_weapontex[1]);
    m_weaponname.Load(0.9f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx, m_menuposy[1] + m_nameoffsety,
                      true);
    m_weapondescription.Load(0.9f, "3", m_myfont, m_menuposx[1] + m_nameoffsetx,
                             m_menuposy[1] + m_nameoffsety, true);
    m_ammotype.Load(0.9f, "2", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    12.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_ammocount.Load(0.9f, "1", m_myfont, m_menuposx[1] + m_nameoffsetx,
                     24.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_magcount.Load(0.9f, "4testing", m_myfont, m_menuposx[1] + m_nameoffsetx,
                    36.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_weaponname.Off();
    m_weapondescription.Off();
    m_ammotype.Off();
    m_ammocount.Off();
    m_magcount.Off();
    m_hotswapL1text.Load(0.9f, "l1", m_myfont, m_menuposx[1] + m_nameoffsetx,
                         42.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_hotswapL2text.Load(0.9f, "l2", m_myfont, m_menuposx[1] + m_nameoffsetx,
                         50.0f + (m_menuposy[1] + m_nameoffsety), true);
    m_selectsound = CSnd::GetSoundByName(".TCM_SELECT");
    m_scrollupsound = CSnd::GetSoundByName(".TCM_SLIDE");
    m_scrolldownsound = CSnd::GetSoundByName(".TCM_SLIDE");
}

/* the menu opened to its middle row: the first and third icons on, the goal a row (62) down,
 * the outer rows started at the middle one's place and loaded (SOCOM 1's AfterOpenFirst, inline
 * in SOCOM II: only the opening state calls it, and no copy is linked) */
inline void CWeaponSel::AfterOpenFirst()
{
    m_weaponicon[0].On();
    m_weaponicon[2].On();
    m_openstate = 2;
    m_menugoaly = 62.0f;
    m_menugoalx = 0.0f;
    m_menuposx[0] = m_menuposx[2] = m_menuposx[1];
    m_menuposy[0] = m_menuposy[2] = m_menuposy[1];
    m_weaponicon[0].Load(m_menuposx[0] + m_iconoffsetx, m_menuposy[0] + m_iconoffsety, m_weapontex[0]);
    m_weaponicon[0].On();
    m_weaponicon[2].Load(m_menuposx[2] + m_iconoffsetx, m_menuposy[2] + m_iconoffsety, m_weapontex[2]);
    m_weaponicon[2].On();
}

/* open, it closes first; the texts let go of */
void CWeaponSel::UnInit()
{
    if (m_openstate)
        OnClose();
    TextClearAndWipe();
}

/* a value moved toward its goal by an increment, not past it (SOCOM 1's static
 * CWeaponSel::Increment, inline here: SOCOM II links no copy, and the increment's product is
 * substituted into both arms) */
static inline float Increment(float increment, float val, float goal)
{
    if (val < goal) {
        val += increment;
        if (val > goal)
            val = goal;
    } else if (val > goal) {
        val -= increment;
        if (val < goal)
            val = goal;
    }
    return val;
}

/* opening: the height grown with the middle row's slide, the row slid to its goal, the items
 * placed; there, the outer rows opened (SOCOM 1's HudTick's state 1, a function of its own in
 * SOCOM II: the name ours) */
void CWeaponSel::HandleOpenFirstState(float dT)
{
    m_curheight = m_goalheight * ((m_menuposx[1] - m_menustartx) / m_menugoalx - m_menustartx);
    m_menuposx[1] = Increment(475.0f * dT, m_menuposx[1], m_menugoalx);
    m_menuposy[1] = Increment(475.0f * dT, m_menuposy[1], m_menugoaly);
    SetItemPos(1);
    if (m_menuposx[1] == m_menugoalx && m_menuposy[1] == m_menugoaly)
        AfterOpenFirst();
}

/* the arrows' colours: the brighter one fades down to the other's (lit at 100 and the pulse
 * restarted from it when it gets there), else both pulse between 60 and 120 (SOCOM 1's) */
void CWeaponSel::PulseSelectionArrows(float dT)
{
    CPnt3D tempcolor1 = m_arrow[0].m_tris[0].GetColor();
    CPnt3D tempcolor2 = m_arrow[1].m_tris[0].GetColor();

    if (tempcolor1.x > tempcolor2.x) {
        CPnt3D step;

        step.x = step.y = step.z = 500.0f * dT;
        tempcolor1.Sub(&step, &tempcolor1);
        if (tempcolor1.x < tempcolor2.x) {
            tempcolor1.x = tempcolor2.x;
            tempcolor1.y = tempcolor2.y;
            tempcolor1.z = tempcolor2.z;
            m_arrow[0].SetTrans(100.0f);
            m_arrowincrement = 1;
            m_arrowcolor = tempcolor1.x;
        }
        m_arrow[0].SetColor(tempcolor1.x, tempcolor1.y, tempcolor1.z);
    } else if (tempcolor2.x > tempcolor1.x) {
        CPnt3D step;

        step.x = step.y = step.z = 500.0f * dT;
        tempcolor2.Sub(&step, &tempcolor2);
        if (tempcolor2.x < tempcolor1.x) {
            tempcolor2.x = tempcolor1.x;
            tempcolor2.y = tempcolor1.y;
            tempcolor2.z = tempcolor1.z;
            m_arrow[1].SetTrans(100.0f);
            m_arrowincrement = 1;
            m_arrowcolor = tempcolor2.x;
        }
        m_arrow[1].SetColor(tempcolor2.x, tempcolor2.y, tempcolor2.z);
    } else {
        if (m_arrowincrement) {
            m_arrowcolor += 250.0f * dT;
            if (m_arrowcolor > 120.0f) {
                m_arrowincrement = !m_arrowincrement;
                m_arrowcolor = 120.0f;
            }
        } else {
            m_arrowcolor -= 250.0f * dT;
            if (m_arrowcolor < 60.0f) {
                m_arrowincrement = !m_arrowincrement;
                m_arrowcolor = 60.0f;
            }
        }
        m_arrow[0].SetColor(m_arrowcolor, m_arrowcolor, m_arrowcolor);
        m_arrow[1].SetColor(m_arrowcolor, m_arrowcolor, m_arrowcolor);
    }
}

/* scrolling up: the second and third rows slid toward the fourth's place (the second twice as
 * far), their fades following, the scroll finished there; the items placed */
void CWeaponSel::HandleScrollUpState(float dT)
{
    m_menuposx[1] = Increment(2.0f * (float)(475.0f * dT), m_menuposx[1], m_menuposx[3] - m_menugoalx);
    m_menuposy[1] = Increment(2.0f * (float)(475.0f * dT), m_menuposy[1], m_menuposy[3] - 2.0f * m_menugoaly);
    m_menuposx[2] = Increment(2.0f * (float)(475.0f * dT), m_menuposx[2], m_menuposx[3] - m_menugoalx);
    m_menuposy[2] = Increment(2.0f * (float)(475.0f * dT), m_menuposy[2], m_menuposy[3] - m_menugoaly);

    float r = 1.0f + (m_menuposy[3] - m_menugoaly - m_menuposy[2]) / m_menugoaly;

    m_curtrans[0] = 40.0f + 87.0f * (1.0f - r);
    m_curtrans[1] = 40.0f + 87.0f * r;
    m_weaponicon[2].SetTrans(m_curtrans[1]);
    m_weaponicon[1].SetTrans(m_curtrans[0]);
    if (m_menuposy[2] == m_menuposy[3] - m_menugoaly)
        AfterScrollUp();
    SetItemPos(4);
}

/* scrolling down: the first two rows slid toward the fourth's place (the second twice as far),
 * their fades following, the scroll finished there; the items placed */
void CWeaponSel::HandleScrollDownState(float dT)
{
    m_menuposx[0] = Increment(2.0f * (float)(475.0f * dT), m_menuposx[0], m_menuposx[3] + m_menugoalx);
    m_menuposy[0] = Increment(2.0f * (float)(475.0f * dT), m_menuposy[0], m_menuposy[3] + m_menugoaly);
    m_menuposx[1] = Increment(2.0f * (float)(475.0f * dT), m_menuposx[1], m_menuposx[3] + 2.0f * m_menugoalx);
    m_menuposy[1] = Increment(2.0f * (float)(475.0f * dT), m_menuposy[1], m_menuposy[3] + 2.0f * m_menugoaly);

    float r = (m_menuposy[3] + m_menugoaly - m_menuposy[0]) / m_menugoaly;

    m_curtrans[0] = 40.0f + 87.0f * (1.0f - r);
    m_curtrans[1] = 40.0f + 87.0f * r;
    m_weaponicon[0].SetTrans(m_curtrans[0]);
    m_weaponicon[1].SetTrans(m_curtrans[1]);
    if (m_menuposy[0] == m_menuposy[3] + m_menugoaly)
        AfterScrollDown();
    SetItemPos(5);
}

/* the arrows, the backdrop and two backings off, closed (6), the HUD's reticule back and the
 * seal's, the texts off -- with a seal to own the HUD */
void CWeaponSel::OnClose()
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        m_arrow[0].Off();
        m_arrow[1].Off();
        m_weaponselbkrnd.Off();
        m_selectionback0.Off();
        m_selectionback1.Off();
        m_openstate = 6;
        m_parent->m_usehudret = false;
        seal->m_kit.SetCurReticule(false);
        TextOff();
    }
}

/* closing: the outer rows slid in to the middle one, the items placed; there, the menu folded
 * (SOCOM 1's HudTick's state 6, a function of its own in SOCOM II: the name ours; AfterCloseFirst
 * auto-inlined) */
void CWeaponSel::HandleCloseFirstState(float dT)
{
    m_menuposx[0] = Increment(475.0f * dT, m_menuposx[0], m_menuposx[1]);
    m_menuposy[0] = Increment(475.0f * dT, m_menuposy[0], m_menuposy[1]);
    m_menuposx[2] = Increment(475.0f * dT, m_menuposx[2], m_menuposx[1]);
    m_menuposy[2] = Increment(475.0f * dT, m_menuposy[2], m_menuposy[1]);
    SetItemPos(6);
    if (m_menuposx[0] == m_menuposx[1] && m_menuposy[0] == m_menuposy[1])
        AfterCloseFirst();
}

/* the menu folded to its second row: the height, its centre and the goal there; shut (7), the
 * first and third icons off */
void CWeaponSel::AfterCloseFirst()
{
    m_curheight = m_goalheight = 2.0f + (32.0f + m_menuposy[1]) - (m_menuposy[1] - 1.0f);
    m_menucentery = m_goalheight / 2.0f;
    m_menugoaly = m_menuposy[1];
    m_menugoalx = m_menuposx[1];
    m_menustartx = 0.0f;
    m_openstate = 7;
    m_weaponicon[0].Off();
    m_weaponicon[2].Off();
}

/* the item after `item` the HUD owner's kit can select, wrapping round (none without an owner) */
static inline int NextItem(int cur)
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return 0;

    int next = cur + 1;

    if (next >= seal->m_kit.GetNumItems())
        next = 0;
    while (!(bool)func_005BDC30(seal->GetKit(), next)) {
        next++;
        if (next >= seal->m_kit.GetNumItems())
            next = 0;
    }
    return next;
}

/* the item before `cur` the HUD owner's kit can select, wrapping round (none without an owner) */
static inline int PrevItem(int cur)
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return 0;

    int prev = cur - 1;

    if (prev < 0)
        prev = seal->m_kit.GetNumItems() - 1;
    while (!(bool)func_005BDC30(seal->GetKit(), prev)) {
        prev--;
        if (prev < 0)
            prev = seal->m_kit.GetNumItems() - 1;
    }
    return prev;
}

/* scrolling down: the fourth row brought in at the first's place with the item before it, the
 * arrows lit (the lower one brighter), the rows' fades set, the scroll sound played, the texts off */
void CWeaponSel::OnScrollDown()
{
    m_openstate = 5;
    m_menuposx[3] = m_menuposx[0];
    m_menuposy[3] = m_menuposy[0];
    m_menugoalx = 0.0f;
    m_menugoaly = m_menuposy[2] - m_menuposy[1];

    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return;

    int prev = PrevItem(m_weaponnum[0]);

    m_weapontex[3] = seal->m_kit.GetIcon(prev);
    m_weaponnum[3] = prev;
    m_weaponicon[3].Load(m_menuposx[3] + m_iconoffsetx, m_menuposy[3] + m_iconoffsety, m_weapontex[3]);
    m_weaponicon[3].On();
    m_arrow[1].SetTrans(128.0f);
    m_arrow[1].SetColor(100.0f, 100.0f, 100.0f);
    m_arrow[0].SetColor(60.0f, 60.0f, 60.0f);
    m_arrowcolor = 50.0f;
    m_arrowincrement = 1;
    m_goaltrans[0] = 127.0f;
    m_curtrans[0] = 40.0f;
    m_goaltrans[1] = 40.0f;
    m_curtrans[1] = 127.0f;
    if (m_scrolldownsound)
        m_scrolldownsound->PlaySound(0, 255);
    TextOff();
}

/* scrolling up: the fourth row brought in below the third with the item after it, the arrows lit
 * (the upper one brighter), the rows' fades set, the scroll sound played, the texts off */
void CWeaponSel::OnScrollUp()
{
    m_openstate = 4;
    m_menuposx[3] = m_menuposx[2];
    m_menuposy[3] = m_menuposy[2];
    m_menugoalx = 0.0f;
    m_menugoaly = m_menuposy[2] - m_menuposy[1];

    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return;

    int next = NextItem(m_weaponnum[2]);

    m_weaponnum[3] = next;
    m_weapontex[3] = seal->m_kit.GetIcon(next);
    m_weaponicon[3].Load(m_menuposx[3] + m_iconoffsetx, m_menuposy[3] + m_iconoffsety, m_weapontex[3]);
    m_weaponicon[3].On();
    m_arrow[0].SetTrans(128.0f);
    m_arrow[0].SetColor(100.0f, 100.0f, 100.0f);
    m_arrow[1].SetColor(60.0f, 60.0f, 60.0f);
    m_arrowcolor = 50.0f;
    m_arrowincrement = 1;
    m_goaltrans[0] = 40.0f;
    m_curtrans[0] = 127.0f;
    m_goaltrans[1] = 127.0f;
    m_curtrans[1] = 40.0f;
    if (m_scrollupsound)
        m_scrollupsound->PlaySound(0, 255);
    TextOff();
}

/* shut: the seal's menu state back to none, its pad's cross, circle and triangle released, the
 * icons, arrows and weapon texts off, closed (0) */
void CWeaponSel::AfterClose()
{
    int ii;
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        func_00597740(seal->m_ctrl, 0, 0);

        CPad *pad = GetPad(seal->m_ctrl->m_padindex);

        if (pad) {
            pad->SetButtonState(PAD_CROSS, KEY_UP);
            pad->SetButtonState(PAD_CIRCLE, KEY_UP);
            pad->SetButtonState(PAD_TRIANGLE, KEY_UP);
        }
        for (ii = 0; ii < 4; ii++)
            m_weaponicon[ii].Off();
        m_arrow[0].Off();
        m_arrow[1].Off();
        m_weaponname.Off();
        m_weapondescription.Off();
        m_ammotype.Off();
        m_ammocount.Off();
        m_magcount.Off();
        m_openstate = 0;
    }
}

/* scrolled down: each row takes the one above it (the top row the hidden fourth's), the fourth
 * icon off, the arrows dimmed to 70, the HUD's reticule on, the colour pulse restarted, the icons
 * dimmed but the middle one, the texts placed again (SOCOM 1's, the selection backings gone) */
void CWeaponSel::AfterScrollDown()
{
    if (CHUD::GetHUDOwner()) {
        m_openstate = 3;
        m_current_item = m_weaponnum[0];
        m_menuposx[2] = m_menuposx[1];
        m_menuposy[2] = m_menuposy[1];
        m_weapontex[2] = m_weapontex[1];
        m_weaponicon[2].Load(m_menuposx[2] + m_iconoffsetx, m_menuposy[2] + m_iconoffsety, m_weapontex[2]);
        m_weaponnum[2] = m_weaponnum[1];
        m_menuposx[1] = m_menuposx[0];
        m_menuposy[1] = m_menuposy[0];
        m_weapontex[1] = m_weapontex[0];
        m_weaponicon[1].Load(m_menuposx[1] + m_iconoffsetx, m_menuposy[1] + m_iconoffsety, m_weapontex[1]);
        m_weaponnum[1] = m_weaponnum[0];
        m_menuposx[0] = m_menuposx[3];
        m_menuposy[0] = m_menuposy[3];
        m_weapontex[0] = m_weapontex[3];
        m_weaponicon[0].Load(m_menuposx[0] + m_iconoffsetx, m_menuposy[0] + m_iconoffsety, m_weapontex[0]);
        m_weaponnum[0] = m_weaponnum[3];
        m_weaponicon[3].Off();
        m_arrow[0].SetTrans(70.0f);
        m_arrow[1].SetTrans(70.0f);
        m_parent->m_usehudret = true;
        m_curcolor = 1.0f;
        m_incCurcolor = 0;
        for (int i = 0; i < 4; i++)
            m_weaponicon[i].SetTrans(40.0f);
        m_weaponicon[1].SetTrans(128.0f);
        PlaceWeaponText();
        PlaceHotSwapLabels();
    }
}

/* scrolled up: each row takes the one below it, as AfterScrollDown the other way round */
void CWeaponSel::AfterScrollUp()
{
    if (CHUD::GetHUDOwner()) {
        m_openstate = 3;
        m_current_item = m_weaponnum[2];
        m_menuposx[0] = m_menuposx[1];
        m_menuposy[0] = m_menuposy[1];
        m_weapontex[0] = m_weapontex[1];
        m_weaponicon[0].Load(m_menuposx[0] + m_iconoffsetx, m_menuposy[0] + m_iconoffsety, m_weapontex[0]);
        m_weaponnum[0] = m_weaponnum[1];
        m_menuposx[1] = m_menuposx[2];
        m_menuposy[1] = m_menuposy[2];
        m_weapontex[1] = m_weapontex[2];
        m_weaponicon[1].Load(m_menuposx[1] + m_iconoffsetx, m_menuposy[1] + m_iconoffsety, m_weapontex[1]);
        m_weaponnum[1] = m_weaponnum[2];
        m_menuposx[2] = m_menuposx[3];
        m_menuposy[2] = m_menuposy[3];
        m_weapontex[2] = m_weapontex[3];
        m_weaponicon[2].Load(m_menuposx[2] + m_iconoffsetx, m_menuposy[2] + m_iconoffsety, m_weapontex[2]);
        m_weaponnum[2] = m_weaponnum[3];
        m_weaponicon[3].Off();
        m_arrow[0].SetTrans(70.0f);
        m_arrow[1].SetTrans(70.0f);
        m_parent->m_usehudret = true;
        m_curcolor = 1.0f;
        m_incCurcolor = 0;
        for (int i = 0; i < 4; i++)
            m_weaponicon[i].SetTrans(40.0f);
        m_weaponicon[1].SetTrans(128.0f);
        PlaceWeaponText();
        PlaceHotSwapLabels();
    }
}

/* opened: the arrows centred on the selector's width at its top and bottom rows (with an arrow
 * texture), the texts placed, scrolling (3), the select sound played (SOCOM 1's, the arrows'
 * placing reworked and the backings gone) */
void CWeaponSel::AfterOpenSecond()
{
    if (m_arrowtex) {
        float h = (int)m_arrowtex->m_height;
        float w = (int)m_arrowtex->m_width;

        m_arrow[0].On();

        /* the upper arrow's lower edge taken first: it is loaded ahead of the centring */
        float y = data_003E4AA0 + h;
        float right = (w + data_0040C108) / 2.0f;
        float left = right - w;

        m_arrow[0].Load(left, data_003E4AA0, left, y, right, y, right, data_003E4AA0, m_arrowtex);
        m_arrow[1].On();
        m_arrow[1].Load(right, data_003E4A90, right, data_003E4A90 - h, left, data_003E4A90 - h, left,
                        data_003E4A90, m_arrowtex);
    }
    PlaceHotSwapLabels();
    PlaceWeaponText();
    m_openstate = 3;
    if (m_selectsound)
        m_selectsound->PlaySound(0, 255);
}

/* the hot-swap highlight faded out and off, the middle icon off when asked or closed, the hot-swap
 * labels back to 80 (SOCOM II's: CHUD::Tick calls it with true after a close; the name ours) */
void CWeaponSel::ResetHotSwap(bool force)
{
    m_selectionback2.SetTrans(0.0f);
    m_selectionback2.m_fade_dx = 2.0f;
    m_selectionback2.Off();
    if (force || !m_openstate)
        m_weaponicon[1].Off();
    m_hotswapL1text.SetTrans(80.0f);
    m_hotswapL2text.SetTrans(80.0f);
}

/* the hot-swap highlight's pulse while the menu is shut: it fades in to 64 and out again,
 * the lit hot-swap label following it, and when gone the middle icon goes too; with the menu
 * open the highlight is put away (SOCOM II's: HudTick's last call, the name ours) */
void CWeaponSel::TickHotSwap(float dT)
{
    if (!m_selectionback2.IsOn())
        return;
    if (m_openstate == 0) {
        C2DString *label = 0;
        float speed;

        if (m_selectionback2.m_fade_dx > 0.0f)
            speed = 320.0f;
        else
            speed = -160.0f;
        if (m_hotswapL1text.IsOn())
            label = &m_hotswapL1text;
        else if (m_hotswapL2text.IsOn())
            label = &m_hotswapL2text;

        float dx = m_selectionback2.m_fade_dx;

        float trans = m_selectionback2.GetTrans();

        m_selectionback2.SetTrans(trans + 256.0f * dT * dx);
        if (label) {
            float step = dT * speed;

            label->SetTrans(step + label->m_color[3]);
        }
        if (!(m_selectionback2.GetTrans() < 64.0f)) {
            m_selectionback2.SetTrans(64.0f);
            m_selectionback2.m_fade_dx = -1.0f;
            if (label)
                label->SetTrans(80.0f);
        } else if (m_selectionback2.GetTrans() <= 0.0f) {
            m_weaponicon[1].Off();
            m_selectionback2.Off();
            m_selectionback2.m_fade_dx = 2.0f;
            if (label) {
                label->Off();
                label->SetTrans(80.0f);
            }
        }
    } else {
        m_selectionback2.Off();
        m_selectionback2.m_fade_dx = 2.0f;
        m_hotswapL1text.SetTrans(80.0f);
        m_hotswapL2text.SetTrans(80.0f);
    }
}

/* the menu's tick: closed, the weapon-select button opens it (with a seal not changing weapons);
 * opening, scrolling and closing run their states (SOCOM 1's, the opening and the first closing
 * states functions of their own); then the backdrops fade in over half a second */
void CWeaponSel::HudTick(CPad *pad, float dT)
{
    switch (m_openstate) {
    case 0: {
        CZSealBody *seal = CHUD::GetHUDOwner();

        /* the button's state compared as a bool (`!= false`: the target materialises the test) */
        if (seal && (func_002C64E0(4, pad) == 1) != false && !func_005A7980(seal, 1) && seal->m_socom2_e1_4)
            OnOpen();
        break;
    }
    case 1:
        HandleOpenFirstState(dT);
        break;
    case 2:
        m_menuposx[0] = m_menuposx[1] - m_menugoalx;
        m_menuposy[0] = m_menuposy[1] - m_menugoaly;
        m_menuposx[2] = m_menuposx[1] + m_menugoalx;
        m_menuposy[2] = m_menuposy[1] + m_menugoaly;
        SetItemPos(2);
        AfterOpenSecond();
        break;
    case 3:
        HandleOpenState(pad, dT);
        break;
    case 5:
        HandleScrollDownState(dT);
        break;
    case 4:
        HandleScrollUpState(dT);
        break;
    case 6:
        HandleCloseFirstState(dT);
        break;
    case 7:
        m_curheight = m_goalheight * ((m_menuposx[1] - m_menustartx) / m_menugoalx - m_menustartx);
        m_menuposx[1] = Increment(475.0f * dT, m_menuposx[1], m_menustartx);
        SetItemPos(7);
        if (m_menuposx[1] == m_menustartx)
            AfterClose();
        break;
    }
    func_0021A1F0(this);
    if (!(g_fFadeInTime < 0.0f)) {
        float t;

        g_fFadeInTime += dT;
        if (g_fFadeInTime < 0.5f)
            t = 2.0f * g_fFadeInTime;
        else
            t = 1.0f;
        m_weaponselbkrnd.SetTrans(t * data_003E3000);
        m_selectionback0.SetTrans(t * data_003E3000);
        m_selectionback1.SetTrans(t * data_003E3000);
        if (!(g_fFadeInTime < 0.5f))
            g_fFadeInTime = -1.0f;
    }
    TickHotSwap(dT);
}

/* the items placed for a state: opening and folding the middle icon sized to the menu's height,
 * the outer rows' icons where they slide (SOCOM 1's; its states 1 and 7 the middle icon's inline,
 * out of line in SOCOM II) */
void CWeaponSel::SetItemPos(int index)
{
    switch (index) {
    case 1:
        PlaceMiddleIcon();
        break;
    case 2:
        m_weaponicon[0].SetPos((int)(m_menuposx[0] + m_iconoffsetx), (int)(m_menuposy[0] + m_iconoffsety));
        m_weaponicon[2].SetPos((int)(m_menuposx[2] + m_iconoffsetx), (int)(m_menuposy[2] + m_iconoffsety));
        break;
    case 4:
        m_weaponicon[1].SetPos((int)(m_menuposx[1] + m_iconoffsetx), (int)(m_menuposy[1] + m_iconoffsety));
        m_weaponicon[2].SetPos((int)(m_menuposx[2] + m_iconoffsetx), (int)(m_menuposy[2] + m_iconoffsety));
        break;
    case 5:
        m_weaponicon[0].SetPos((int)(m_menuposx[0] + m_iconoffsetx), (int)(m_menuposy[0] + m_iconoffsety));
        m_weaponicon[1].SetPos((int)(m_menuposx[1] + m_iconoffsetx), (int)(m_menuposy[1] + m_iconoffsety));
        break;
    case 6:
        m_weaponicon[0].SetPos((int)(m_menuposx[0] + m_iconoffsetx), (int)(m_menuposy[0] + m_iconoffsety));
        m_weaponicon[2].SetPos((int)(m_menuposx[2] + m_iconoffsetx), (int)(m_menuposy[2] + m_iconoffsety));
        break;
    case 7:
        PlaceMiddleIcon();
        break;
    }
}

/* the middle icon sized to the menu's height as it opens and folds: its top no lower than the
 * row's centre, its bottom no higher, no taller than its texture (SOCOM 1's SetItemPos states 1
 * and 7, a function of its own in SOCOM II: the name ours) */
void CWeaponSel::PlaceMiddleIcon()
{
    m_curheight = m_goalheight * ((m_menuposx[1] - m_menustartx) / m_menugoalx - m_menustartx);

    float top = m_menuposy[1] + m_menucentery - m_curheight / 2.0f;

    if (top > m_menuposy[1] + m_menucentery - 1.0f)
        top = m_menuposy[1] - 1.0f + m_menucentery;

    float bottom = 0.5f * m_curheight + (m_menuposy[1] - 1.0f + m_menucentery) - 1.0f;

    if (bottom < m_menuposy[1] - 1.0f + m_menucentery)
        bottom = m_menuposy[1] - 1.0f + m_menucentery;
    if (m_weapontex[1]) {
        zdb::CTexture *tex = m_weapontex[1]->m_texture;
        float w = (int)tex->m_width;
        float h = (int)tex->m_height;

        if (bottom - top > h)
            bottom = top + h;

        m_weaponicon[1].SetPos((int)(m_menuposx[1] + m_iconoffsetx), (int)(top + m_iconoffsety),
                               (int)(w + (m_menuposx[1] + m_iconoffsetx)), (int)(bottom + m_iconoffsety));
    }
}

/* the middle row's texts: the weapon's name, upper-cased, its rounds over the magazine's size
 * (right-aligned at 148) and its spare magazines a line lower ("1 MAGAZINE" or n of them), the
 * last two off for an item of type 11 (SOCOM 1's, its locals' names from its DWARF; the strings
 * SOCOM II's string table's).
 * OPEN, 8 bytes short: the target, like SOCOM 1's, copies the seal into a saved register
 * (`move s1,v0; beqz s1`), forms the kit's address once in s0, and gives the seal's s1 to ammomax
 * (the seal dead after the kit). A kit local makes MW test v0 and drop the seal's copy (16 bytes
 * short); the seal read again at the end (pItem, as here) keeps the copy but holds it in s3, the
 * locals one register up and a load short; the calls written `seal->m_kit.X()` (SOCOM 1's DWARF
 * has no kit local) re-form the kit for every call. Tried also: early return, the declaration in
 * the condition, `(seal = f()) != 0`, GetKit() at each call. */
void CWeaponSel::PlaceWeaponText()
{
    static char tempstr[80];
    float topx = 4.0f + m_menuposx[1];
    float topy = 48.0f + m_menuposy[1];

    m_weaponname.On();
    m_weapondescription.Off();
    m_ammotype.Off();
    m_ammocount.On();
    m_magcount.On();

    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal) {
        CZKit *kit = seal->GetKit();
        int mags = kit->GetNumMags(m_current_item);
        int ammo = func_005C38A0(kit, m_current_item);
        int ammomax = func_005C3CF0(kit, m_current_item);
        const char *itemname = kit->GetItemName(m_current_item);

        strlen(itemname);
        strncpy(tempstr, itemname, 80);
        CHUD::HudText_to_upper(tempstr);
        m_weaponname.Load(0.9f, tempstr, m_myfont, topx, topy, true);
        if (ammomax) {
            sprintf(tempstr, "%d/%d", ammo, ammomax);
            m_ammocount.Load(0.9f, tempstr, m_myfont, 80.0f + topx, topy, true);
            m_ammocount.UpdatePos();
            m_ammocount.SetPos((int)(148.0f - m_ammocount.m_charheight), (int)topy);
            m_ammocount.UpdatePos();
        } else {
            m_ammocount.Off();
        }
        topy += 14.0f;
        if (mags)
            mags--;
        if (mags) {
            const char *text;

            if (mags > 1) {
                func_001FE120(tempstr, 80, 0xEC42, mags);
                text = tempstr;
            } else {
                text = func_00361B40(0xEC43);
            }
            m_magcount.Load(0.9f, text, m_myfont, topx, topy, true);
        } else {
            m_magcount.Off();
        }

        CZFTSWeapon *pItem = seal->m_kit.GetItemByIndex(m_current_item);

        if (pItem->m_type == 11) {
            m_ammotype.Off();
            m_ammocount.Off();
            m_magcount.Off();
        }
    }
}

/* the hot-swap labels, when the seal's two hot-swap slots differ: each shown beside the middle
 * row, right-aligned at 148, while that row holds its slot's item, else off (SOCOM 1's, SOCOM
 * II's strings from the string table and the slots its controller's floats) */
void CWeaponSel::PlaceHotSwapLabels()
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return;

    int slot1 = (int)seal->m_ctrl->m_hotswap1;
    int slot2 = (int)seal->m_ctrl->m_hotswap2;

    m_hotswapL1text.On();
    m_hotswapL2text.On();
    if (slot1 == slot2)
        return;
    if (m_weaponnum[1] == slot1) {
        C2DFont *font = m_myfont;
        int x, y;

        y = (int)(62.0f + m_menuposy[1]);
        x = (int)(80.0f + m_menuposx[1]);

        const char *str = func_00361B40(0xEC6D);

        m_hotswapL1text.Load(0.9f, str, font, x, y, true);
        m_hotswapL1text.UpdatePos();
        m_hotswapL1text.SetPos((int)(148.0f - m_hotswapL1text.m_charheight), (int)(62.0f + m_menuposy[1]));
        m_hotswapL1text.UpdatePos();
    } else {
        m_hotswapL1text.Off();
    }
    if (m_weaponnum[1] == slot2) {
        C2DFont *font = m_myfont;
        int x, y;

        y = (int)(62.0f + m_menuposy[1]);
        x = (int)(80.0f + m_menuposx[1]);

        const char *str = func_00361B40(0xEC6E);

        m_hotswapL2text.Load(0.9f, str, font, x, y, true);
        m_hotswapL2text.UpdatePos();
        m_hotswapL2text.SetPos((int)(148.0f - m_hotswapL2text.m_charheight), (int)(62.0f + m_menuposy[1]));
        m_hotswapL2text.UpdatePos();
    } else {
        m_hotswapL2text.Off();
    }
}
