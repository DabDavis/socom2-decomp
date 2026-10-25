/* hud_teamnames.cpp -- the team's names (OVL1 ...-0x221BF0): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_teamnames.cpp), before hud_posebitmap.cpp as there: Clear, Init and the
 * constructor at its end (0x220FB0, 0x221070, 0x221B10), then the inline CHudBrightText
 * constructor's copy (0x221BF0). Written: Clear and the constructor (with that copy); Init and the
 * two functions after it (0x2216D0, 0x221970) not yet. CTeamNames is 0xF30 bytes in SOCOM II
 * (SOCOM 1's 0x770). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "hud_weaponsel.h"
#include "hud_mpinfobox.h"
#include "../../gamez/zvid/zvid.h"

/* the seal's controller as the names read it: two virtuals (slot 10, whether it has an order; slot
 * 12, the order) -- the vtable pointer first, as CSealCtrl's is */
class CSealCtrl {
public:
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual bool HasOrder();
    virtual void v11();
    virtual int GetOrder();
};

/* the kit (OVL2) */
class CZKit {
public:
    bool HasItemWithAmmo(EQUIP_ITEM item);
};

/* a SEAL as the names read it: its name (CEntity's, at 0x14), controller, the flag the compass tests
 * too, its kit and whether its order changed (cleared through an inline setter) */
class CZSealBody {
public:
    bool GetHudHitFlag();
    void SetOrderChanged(bool b) { m_orderchanged = b; }

    unsigned char _0[0x14];
    char *m_name;   // 0x14
    unsigned char _18[0xc0 - 0x18];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x5e0 - 0xe2];
    CZKit m_kit;   // 0x5e0
    unsigned char _5e1[0x105d - 0x5e1];
    unsigned char _105d_0 : 1;
    unsigned char m_orderchanged : 1;   // 0x105d bit 1
    unsigned char _105d_2 : 6;
};

/* an order's name (OVL2; no row named yet) */
extern "C" const char *func_005743C0(int order);

/* the HUD's layout (hud_main.cpp's data: the bottom block's top, the screen's last line) and the
 * corner boxes' texture (SOCOM 1's c_szCornerBoxesTx, a static of each unit there) */
extern float data_003E2FF8;
extern float data_003E2FE8;
extern const char *data_003E3048;
/* the HUD's bitmaps' alpha (hud_main.cpp's data: 76.8, SOCOM 1's c_fBracketTrans) */
extern float data_003E3000;

namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the HUD's text colour (SOCOM 1's static) and its upper-casing */
class CHUD {
public:
    static void HudText_to_upper(char *instr);
    static PNT4D m_hud_textcolor;
};

/* a string placed by its position and centring (no row named yet) */
extern "C" void func_00362B70(C2DString *s);

/* the BLUDSHOT name's scale (data 0x3DC720: a variable, read with lwc1) */
static float s_bludshotscale = 0.74f;

/* a squad by its team (OVL2), and its SEALs by their place (SOCOM 1's GetSealByIndex, a
 * second argument added; no row named yet) */
class CSealUnit {
public:
    static CSealUnit *GetUnitByTeam(unsigned int team);
};
extern "C" CZSealBody *func_005EDBB0(CSealUnit *unit, unsigned int index, int alive);

/* the team's names: SOCOM 1's DWARF moved by SOCOM II's sizes, with a health bar and a bitmap for
 * each of four teammates (laid out from the constructor) */
class CTeamNames : public C2D {
public:
    CTeamNames();
    void Clear();
    void Init();
    /* one teammate's name, order and health bar set up from its SEAL (SOCOM 1's, a flag added
     * that lists the bar's bitmaps too; 0x2216D0) */
    void SetupTeamNameEntry(int index, CZSealBody *seal, bool add);
    void SetOpacity(float fOpacity);
    virtual void Tick(float dT);

    /* a row's height on the screen from the first row's: 15 apart, the second fire team 9
     * lower */
    static float RowY(float base, int index)
    {
        return base + 15.0f * index + (index < 2 ? 0 : 9);
    }
    static float RowY(int index)
    {
        return data_003E2FF8 + 15.0f + 15.0f * index + (index < 2 ? 0 : 9);
    }

    CZSealBody *m_ally[4];   // 0x18: zeroed by the constructor (SOCOM 1: 0x7c)
    CHudBrightText m_teamname[4];   // 0x28 (SOCOM 1: 0x8c)
    C2DString m_order[4];   // 0x288 (SOCOM 1: 0x2ac)
    C2DFont *m_myfont;   // 0x4e8 (SOCOM 1: 0x4dc)
    float m_increment;   // 0x4ec (SOCOM 1: 0x4e0)
    C2DBitmap m_bitmap1;   // 0x4f0
    C2DBitmap m_bitmap2;   // 0x570
    CHealthBar m_healthbar[4];   // 0x5f0
    C2DBitmap m_bitmaps[4];   // 0xd30
} __attribute__((aligned(16)));   // 0xf30 bytes

/* the arrays built, the first words zeroed (the unit's last function; the inline CHudBrightText
 * constructor's copy follows it, its address handed to the array constructor) */
CTeamNames::CTeamNames()
{
    memset(m_ally, 0, sizeof(m_ally));
}

/* two strings of the unit's own (OVL1 bss 0x40D7A0 and 0x40D850; no rows yet: SOCOM 1's Clear
 * empties two globals the same way) */
extern C2DString data_0040D7A0;
extern C2DString data_0040D850;
/* the dividers (SOCOM 1's g_lnDivider; SOCOM II has two) */
extern C2DBitmap data_0040D900;
extern C2DBitmap data_0040D990;
/* the dividers' line (the unit's initialiser computes it) */
extern float data_003E4F60;

/* SetOpacity's local static (SOCOM 1's c_fBracketTrans) and its guard, held to their addresses
 * (scdiff leaves a true function-local static unchecked). A `static float c_fBracketTrans =
 * BracketAlpha();` through an inline returning 48 matches too -- the guard says the initialiser was
 * not a constant to MW 3.0, which initialises `= 48.0f` (or a const float) statically */
extern signed char data_0040DA10;
extern float data_003E4F68;

/* everything faded to a fraction of the HUD's alpha: the bitmaps and bars to its bitmaps', the
 * names and orders to its text colour's, the dots to full, the brackets to theirs (SOCOM 1's) */
void CTeamNames::SetOpacity(float fOpacity)
{
    if (!data_0040DA10) {
        data_003E4F68 = 48.0f;
        data_0040DA10 = 1;
    }
    m_bitmap1.SetTrans(fOpacity * data_003E3000);
    m_bitmap2.SetTrans(fOpacity * data_003E3000);
    data_0040D900.SetTrans(fOpacity * data_003E3000);
    data_0040D990.SetTrans(fOpacity * data_003E3000);
    for (unsigned int i = 0; i < 4; i++) {
        m_teamname[i].SetTrans(fOpacity * CHUD::m_hud_textcolor.w);
        m_order[i].SetTrans(fOpacity * CHUD::m_hud_textcolor.w);
        m_healthbar[i].SetTrans(fOpacity * data_003E3000);
        m_bitmaps[i].SetTrans(128.0f * fOpacity);
    }
    data_0040D850.m_color[3] = data_0040D7A0.m_color[3] = 0.3f * fOpacity * data_003E4F68;
}


/* a string in the HUD's text colour (an inline: its pointer parameter keeps the reference's address
 * in a register for the four stores) */
static inline void SetTextColor(C2DString *s)
{
    s->m_color[0] = CHUD::m_hud_textcolor.x;
    s->m_color[1] = CHUD::m_hud_textcolor.y;
    s->m_color[2] = CHUD::m_hud_textcolor.z;
    s->m_color[3] = CHUD::m_hud_textcolor.w;
}

/* a teammate's name upper-cased and loaded, placed right-aligned on its row (BLUDSHOT's smaller and a
 * line lower), its order coloured and off, its dot off; both listed when asked (SOCOM 1's, the
 * flag SOCOM II's). The scale is 0.9f * 0.95f (0x3F5AE147; 0.855f rounds to ...148).
 * OPEN, 2 instructions: SetPos's arguments are scheduled differently -- the target loads the 510
 * into a1 ahead of the y expression's constants and does the `bludshot ? 2 : 1` movz after the row's
 * operands, where MW here does the movz first and loads 510 into v0 after it, leaving a nop before
 * the sub.s (and, two words on, a label pad after the second push_back). Tried: x or y (int or
 * float) in a local first, the offset in a local, a float cast on it, `!bludshot ? 1 : 2`, the width
 * through an inline accessor, the bool assigned apart, the locals swapped. Everything else (the
 * `= {0}` buffer, the reference in memory, the colour through an inline) matches */
void CTeamNames::SetupTeamNameEntry(int index, CZSealBody *seal, bool add)
{
    if (!seal)
        return;
    m_ally[index] = seal;

    C2DString &strName = m_teamname[index];
    char name_upper[32] = {0};

    if (seal->m_name) {
        strncpy(name_upper, seal->m_name, 31);
        CHUD::HudText_to_upper(name_upper);
    }
    strName.Load(0.9f * 0.95f, name_upper, m_myfont, 0.0f, 0.0f, true);

    bool bludshot = false;

    if (strlen(name_upper) && !strcasecmp(name_upper, "BLUDSHOT"))
        bludshot = true;
    if (bludshot)
        strName.m_scale = s_bludshotscale;
    func_00362B70(&strName);
    strName.SetPos((int)(510.0f - strName.m_charheight), (int)(RowY(index) - (bludshot ? 2 : 1)));
    SetTextColor(&strName);
    if (add) {
        push_back(&strName);
        push_back(&m_order[index]);
    }
    m_order[index].m_color[0] = CHUD::m_hud_textcolor.x;
    m_order[index].m_color[1] = CHUD::m_hud_textcolor.y;
    m_order[index].m_color[2] = CHUD::m_hud_textcolor.z;
    m_order[index].m_color[3] = CHUD::m_hud_textcolor.w;
    m_order[index].Off();
    m_bitmaps[index].Off();
}

/* the background, the dividers, the bars and the dots placed and listed; both fire teams' names
 * set up; the brackets loaded in the HUD's colour */
void CTeamNames::Init()
{
    m_increment = 500.0f;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(data_003E3048);

    m_bitmap1.Load(435.0f, data_003E2FF8, zVid.renderWidth, data_003E4F60 - 2.0f, tex);
    m_bitmap2.Load(435.0f, 6.0f + data_003E4F60, zVid.renderWidth, data_003E2FE8, tex);
    m_bitmap1.SetUV(1.0f, 0.0f, 0.0f, 0.5f);
    m_bitmap2.SetUV(1.0f, 0.5f, 0.0f, 1.0f);
    push_back(&m_bitmap1);
    push_back(&m_bitmap2);
    data_0040D900.Load(435.0f, data_003E4F60 - 2.0f, zVid.renderWidth, data_003E4F60, tex);
    data_0040D900.SetUV(0.8f, 0.0f, 0.0f, 0.01f);
    data_0040D990.Load(435.0f, 4.0f + data_003E4F60, zVid.renderWidth, 6.0f + data_003E4F60, tex);
    data_0040D990.SetUV(0.8f, 0.0f, 0.0f, 0.01f);
    push_back(&data_0040D900);
    push_back(&data_0040D990);
    for (int i = 0; i < 4; i++) {
        float top = RowY(i) - 13.0f;
        float bottom = 14.0f + top;

        m_healthbar[i].SetSize(65.0f, bottom - top);
        m_healthbar[i].SetPos(450.0f, top);
        push_back(&m_healthbar[i]);
        m_bitmaps[i].Load(522.0f, 2.0f + top, 536.0f, bottom,
                          zdb::CAssetMgr::m_assets.GetTexHandle("hud_satchel.tif"));
        push_back(&m_bitmaps[i]);
    }

    CSealUnit *unit;

    if ((unit = CSealUnit::GetUnitByTeam(2)) != 0) {
        SetupTeamNameEntry(0, func_005EDBB0(unit, 0, 0), true);
        SetupTeamNameEntry(1, func_005EDBB0(unit, 1, 0), true);
    }
    if ((unit = CSealUnit::GetUnitByTeam(3)) != 0) {
        SetupTeamNameEntry(2, func_005EDBB0(unit, 0, 0), true);
        SetupTeamNameEntry(3, func_005EDBB0(unit, 1, 0), true);
    }
    float base = 15.0f + data_003E2FF8;

    data_0040D7A0.Load(1.2f, "[", m_myfont, 435.0f, 10.0f + RowY(base, 0), true);
    data_0040D850.Load(1.2f, "[", m_myfont, 435.0f, 10.0f + RowY(base, 2), true);
    data_0040D7A0.m_color[0] = CHUD::m_hud_textcolor.x;
    data_0040D7A0.m_color[1] = CHUD::m_hud_textcolor.y;
    data_0040D7A0.m_color[2] = CHUD::m_hud_textcolor.z;
    data_0040D7A0.m_color[3] = CHUD::m_hud_textcolor.w;
    data_0040D850.m_color[0] = CHUD::m_hud_textcolor.x;
    data_0040D850.m_color[1] = CHUD::m_hud_textcolor.y;
    data_0040D850.m_color[2] = CHUD::m_hud_textcolor.z;
    data_0040D850.m_color[3] = CHUD::m_hud_textcolor.w;
}

/* every name and order wiped, the two shared strings emptied, the children let go */
void CTeamNames::Clear()
{
    for (int index = 0; index < 4; index++) {
        m_teamname[index].ClearAndWipe();
        m_order[index].ClearAndWipe();
    }
    data_0040D7A0.SetString(0, true);
    data_0040D850.SetString(0, true);
    clear();
}

/* each teammate's bar updated and ticked, its dot shown while it has the satchel's ammo, its order
 * hidden when hit or not marked, or reloaded from its controller when it changed (SOCOM 1's,
 * the unit's first function in the image) */
void CTeamNames::Tick(float dT)
{
    for (int i = 0; i < 4; i++) {
        CZSealBody *seal = m_ally[i];

        if (!seal)
            continue;
        if (seal->GetHudHitFlag() && !seal->m_socom2_e1_4) {
            m_order[i].Off();
            m_bitmaps[i].Off();
        }
        m_healthbar[i].Update(seal, true);
        m_healthbar[i].Tick(dT);
        if (seal->m_kit.HasItemWithAmmo((EQUIP_ITEM)152))
            m_bitmaps[i].On();
        else
            m_bitmaps[i].Off();
        if (!seal->m_socom2_e1_4) {
            m_order[i].Off();
            continue;
        }

        bool changed = seal->m_orderchanged;

        seal->SetOrderChanged(false);
        if (!changed)
            continue;

        bool hasorder = seal->m_ctrl ? seal->m_ctrl->HasOrder() : false;

        if (!hasorder)
            continue;

        const char *order = func_005743C0(seal->m_ctrl->GetOrder());

        if (!order)
            continue;
        m_order[i].Load(0.9f * 0.95f, order, m_myfont, 545.0f, RowY(i), true);
        m_order[i].On();
        m_order[i].SetTrans(80.0f);
    }
}
