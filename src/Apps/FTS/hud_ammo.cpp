/* hud_ammo.cpp -- the HUD's ammunition counts (OVL1 ...-0x23841B): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), before the info box's: the counts' update from the kit
 * (0x237760), SetMags, SetTrans, Off, On, Reset, Init (0x237FC0), the destructor and the
 * constructor (the unit's last function in the image). All written, byte-exact but SetMags
 * (OPEN). Defined in reverse of their image order (-inline deferred). */

#include "hud_ammo.h"

#include <stdio.h>

namespace zdb {
class CTexture;

/* a texture's handle (SOCOM 1's DWARF) */
class CTexHandle {
public:
    char *m_name;   // 0x0
    int m_count;   // 0x4
    unsigned int m_gsAddr;   // 0x8
    CTexture *m_texture;   // 0xc
    unsigned char m_libref;   // 0x10
};

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the backing's corners, the captions' and the bitmap's x (variables: read with lwc1), and the
 * two textures' names (pointers in data) */
static float s_polyx = -10.0f;
static float s_polyy = 364.0f;
static float s_polyw = 170.0f;
static float s_polyh = 75.0f;
static float s_ammox = 15.0f;
static float s_magsx = 95.0f;
static float s_bitmapx = 20.0f;
static const char *s_backtex = "newweapnbkrnd.tif";
static const char *s_magtexname = "firemode.tif";
/* the captions' y, the magazines' first bitmap's x and y and the bitmap's y, off the backing's
 * corner and the captions' x: computed by the unit's initialiser (0x3FF610), in this order */
static float s_texty = 18.0f + s_polyy;
static float s_magx = s_ammox - 5.0f;
static float s_magy = 58.0f + s_polyy;
static float s_bitmapy = 25.0f + s_polyy;

/* the magazines last shown: the texture past three (bss 0x4145E0) and the count (data 0x3DCBD8,
 * -1 for none); the unit's statics, no rows yet */
extern zdb::CTexHandle *data_004145E0;
extern int data_003DCBD8;
/* whether the magazines' string shows (bss 0x4145E8; no row yet) */
extern bool data_004145E8;

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the weapon in hand as the counts read it: its id (zweapon.h's m_ID) */
class CZFTSWeapon {
public:
    unsigned char _0[0x7c];
    unsigned char m_ID;   // 0x7c: 11 a weapon without counts
};

/* the fire modes (zweapon.h's FIREMODE, a byte enum): past 3, a texture of the mode's own */
enum FIREMODE { SAFETY = 0, SINGLE = 1, BURST = 2, AUTO = 3, FIREMODE_LAST = 255 };
enum EQUIP_IDTYPE { IDTYPE_PISTOL = 4, IDTYPE_LAST = 255 };

/* the seal's kit (OVL2's CZKit; the members the counts call) */
class CZKit {
public:
    zdb::CTexHandle *GetCurIcon();
    zdb::CTexHandle *GetFiremodeGraphic(int index);
    bool CurItemHasFiremodeGraphic();
    bool IsUsingScope();
    CZFTSWeapon *GetCurItem() const;
    FIREMODE GetCurFireMode();
    int GetCurNumMags();
    EQUIP_IDTYPE GetCurItemIDType();
    bool NVG_Enabled() const;
};

/* the kit's rounds left in the clip and in all (both pass the current item's index on; no names
 * yet), the index of a fire mode's graphic, and two tests of an item's type */
extern "C" int func_005C3CE0(CZKit *kit);
extern "C" int func_005C3890(CZKit *kit);
extern "C" int func_005C6530(CZKit *kit, FIREMODE mode);
extern "C" bool func_003D1DA0(EQUIP_IDTYPE type);
extern "C" bool func_003D1DB0(EQUIP_IDTYPE type);

/* only the bit the counts test (MapCompass::On tests it too) and the kit */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x5e0 - 0xe2];
    CZKit m_kit;   // 0x5e0
};

/* the network game's state (OVL1 bss 0x437CE8; hud_main.cpp's stand-in): whether it respawns */
class CNetGameState {
public:
    unsigned char _0[0xdc];
    unsigned char m_respawns;   // 0xdc
};

extern CNetGameState *data_00437CE8;

class CHUD {
public:
    static CZSealBody *GetHUDOwner();

    static PNT4D m_hud_textcolor;   /* the HUD's text colour */

    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

CZSealBody *ftsGetPlayer();
/* whether the local player spectates (the network glue's; no name yet) */
extern "C" bool func_002C2FA0();


CAmmoDisplay::CAmmoDisplay()
{
    m_magtex = 0;
}

/* the cached counts forgotten, as Reset does */
CAmmoDisplay::~CAmmoDisplay()
{
    data_004145E0 = 0;
    data_003DCBD8 = -1;
}

/* the backing and its bitmap, the counts' captions and the four magazine bitmaps, placed and listed
 * (the name is ours) */
void CAmmoDisplay::Init(C2DFont *font)
{
    zdb::CTexHandle *back = zdb::CAssetMgr::m_assets.GetTexHandle(s_backtex);

    m_poly.Load(s_polyx, s_polyy, s_polyx, s_polyy + s_polyh, s_polyx + s_polyw, s_polyy + s_polyh,
                s_polyx + s_polyw, s_polyy, back ? back->m_texture : 0);
    theHUD->Add2D(&m_poly, -1, true);
    m_bitmap.Load(s_bitmapx, s_bitmapy, back);
    m_poly.push_back(&m_bitmap);
    theHUD->Add2D(&m_head, 1, true);
    m_ammo.m_color[0] = 128.0f;
    m_ammo.m_color[1] = 128.0f;
    m_ammo.m_color[2] = 128.0f;
    m_ammo.m_color[3] = 80.0f;
    m_ammo.Load(0.9f, "AMMO", font, s_ammox, s_texty, true);
    m_head.push_back(&m_ammo);
    m_mags.m_color[0] = 128.0f;
    m_mags.m_color[1] = 128.0f;
    m_mags.m_color[2] = 128.0f;
    m_mags.m_color[3] = 80.0f;
    m_mags.Load(0.9f, "MAGS", font, s_magsx, s_texty, true);
    m_head.push_back(&m_mags);
    m_magtex = zdb::CAssetMgr::m_assets.GetTexHandle(s_magtexname);
    for (unsigned int i = 0; i < 4; i++) {
        m_bitmaps[i].SetUV(0.01f, 0.0f, 1.0f, 1.0f);
        m_bitmaps[i].SetColor(128.0f, 128.0f, 128.0f);
        m_bitmaps[i].Load(s_ammox + 36.0f * (float)i, s_magy, m_magtex);
        m_head.push_back(&m_bitmaps[i]);
    }
}

/* both counts wiped and forgotten (CHUD's ReInit_Init and ReInit_Uninit call it; the name ours) */
void CAmmoDisplay::Reset()
{
    m_ammo.ClearAndWipe();
    m_mags.ClearAndWipe();
    data_004145E0 = 0;
    data_003DCBD8 = -1;
}

/* shown unless spectating a network game (or, offline, while the player's seal has bit 4 of its
 * byte at 0xE1 clear): the last counts shown again, the magazines' string as it was, and the
 * head only for the player's own seal */
void CAmmoDisplay::On()
{
    if (theNetwork.m_bNetwork && func_002C2FA0())
        return;
    if (!theNetwork.m_bNetwork && !ftsGetPlayer()->m_socom2_e1_4)
        return;
    m_poly.On();
    m_head.On();
    SetMags(data_003DCBD8, data_004145E0);
    if (data_004145E8)
        m_mags.On();
    else
        m_mags.Off();
    if (CHUD::GetHUDOwner() != ftsGetPlayer())
        m_head.Off();
}

/* the poly, the head and the four bitmaps off */
void CAmmoDisplay::Off()
{
    m_poly.Off();
    m_head.Off();
    for (unsigned int i = 0; i < 4; i++)
        m_bitmaps[i].Off();
}

/* faded together: the poly to 0.6 of the bitmaps' alpha, the strings by the HUD text's own (the
 * head is not faded) */
void CAmmoDisplay::SetTrans(float trans)
{
    m_poly.SetTrans(76.8f * trans);
    float t = 128.0f * trans;
    m_bitmap.SetTrans(t);
    m_ammo.SetTrans(trans * CHUD::m_hud_textcolor.w);
    m_mags.SetTrans(trans * CHUD::m_hud_textcolor.w);
    for (unsigned int i = 0; i < 4; i++)
        m_bitmaps[i].SetTrans(t);
}

/* the magazines: one to three lit among the four bitmaps; otherwise all off, and a texture given
 * (the count's) shown in the first -- the first bitmap's texture is the magazines' own unless so.
 * OPEN, 7 instructions, all the switch's registers: the target narrows the count into a0 after
 * loading the cases' 3 and 1 into v0 and v1, where this narrows it into v1 first and loads the 1
 * into a0. The argument is an int narrowed inside (On passes the cached word whole; an unsigned
 * char parameter makes On narrow it), and the load ternary (where an `if (own) tex = ...` pads its
 * join with a nop) fixes the tail; tried: `mags & 0xff`, a byte enum, an unsigned char local, a
 * signed char -- each leaves the same swap. */
void CAmmoDisplay::SetMags(int mags, zdb::CTexHandle *tex)
{
    bool own = true;

    switch ((unsigned char)mags) {
    case 1:
        m_bitmaps[0].On();
        m_bitmaps[1].Off();
        m_bitmaps[2].Off();
        m_bitmaps[3].Off();
        break;
    case 2:
        m_bitmaps[0].On();
        m_bitmaps[1].On();
        m_bitmaps[2].On();
        m_bitmaps[3].Off();
        break;
    case 3:
        m_bitmaps[0].On();
        m_bitmaps[1].On();
        m_bitmaps[2].On();
        m_bitmaps[3].On();
        break;
    default:
        for (unsigned int i = 0; i < 4; i++)
            m_bitmaps[i].Off();
        if (tex) {
            m_bitmaps[0].On();
            own = false;
        }
        break;
    }
    m_bitmaps[0].Load(s_magx, s_magy, own ? m_magtex : tex);
}

/* the counts from the HUD owner's kit: the clip's rounds over all the rounds, the magazines left
 * past the one in (hidden while hiding, and for a network game's pistols), the fire mode's
 * magazines (none for a weapon without counts), and the item's icon. The name is ours */
void CAmmoDisplay::Update()
{
    CZSealBody *owner = CHUD::GetHUDOwner();
    CZKit *kit = owner ? &owner->m_kit : 0;
    CZFTSWeapon *item = kit ? kit->GetCurItem() : 0;
    int clip = func_005C3CE0(kit);
    int rounds = func_005C3890(kit);
    int mags = kit->GetCurNumMags();
    char text[32] = {0};
    bool hide = !kit;

    if (theHUD && !m_poly.IsOn())
        hide = true;
    else if (kit && (kit->IsUsingScope() || kit->NVG_Enabled() || !item))
        hide = true;
    bool pistol = false;

    if (theNetwork.m_bNetwork && kit) {
        pistol = func_003D1DA0(kit->GetCurItemIDType());
        if (!pistol && data_00437CE8->m_respawns)
            pistol = func_003D1DB0(kit->GetCurItemIDType());
    }
    sprintf(text, "%d/%d", rounds, clip);
    m_ammo.SetString(text, true);
    int left = mags - 1;

    if (left <= 0 || hide || pistol) {
        m_mags.Off();
        data_004145E8 = false;
    } else {
        sprintf(text, "%d MAG%c", left, left != 1 ? 'S' : ' ');
        m_mags.SetString(text, true);
        m_mags.On();
        data_004145E8 = true;
    }
    if (item && item->m_ID == 11 && !hide) {
        m_ammo.Off();
        m_mags.Off();
        for (unsigned int i = 0; i < 4; i++)
            m_bitmaps[i].Off();
        data_004145E0 = 0;
        data_003DCBD8 = -1;
    } else if (kit && kit->CurItemHasFiremodeGraphic() && !hide) {
        int mode = kit->GetCurFireMode();
        zdb::CTexHandle *tex = 0;

        if (mode > 3)
            tex = kit->GetFiremodeGraphic(func_005C6530(kit, (FIREMODE)mode));
        SetMags(mode, tex);
        data_003DCBD8 = mode;
        data_004145E0 = tex;
        m_ammo.On();
    } else {
        for (unsigned int i = 0; i < 4; i++)
            m_bitmaps[i].Off();
        data_004145E0 = 0;
        data_003DCBD8 = -1;
    }
    m_bitmap.Load(s_bitmapx, s_bitmapy, kit ? kit->GetCurIcon() : 0);
}
