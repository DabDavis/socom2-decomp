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

class CZSealBody;

/* the HUD's layout (hud_main.cpp's data: the bottom block's top, the screen's last line) and the
 * corner boxes' texture (SOCOM 1's c_szCornerBoxesTx, a static of each unit there) */
extern float data_003E2FF8;
extern float data_003E2FE8;
extern const char *data_003E3048;

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

/* the HUD's text colour (SOCOM 1's static) */
class CHUD {
public:
    static PNT4D m_hud_textcolor;
};

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

    unsigned char m_18[0x10];   // 0x18: zeroed by the constructor
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
    memset(m_18, 0, sizeof(m_18));
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
