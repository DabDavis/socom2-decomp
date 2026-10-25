/* hud_ammo.cpp -- the HUD's ammunition counts (OVL1 ...-0x23841B): SOCOM II's unit (no SOCOM 1
 * counterpart; the file name is ours), before the info box's: only the constructor (the unit's
 * last function in the image), the destructor and Reset so far. Defined in reverse of their image order
 * (-inline deferred). */

#include "hud_ammo.h"

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

/* only the bit the counts test (MapCompass::On tests it too) */
class CZSealBody {
public:
    unsigned char _0[0xe1];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
};

class CHUD {
public:
    static CZSealBody *GetHUDOwner();

    static PNT4D m_hud_textcolor;   /* the HUD's text colour */
};

CZSealBody *ftsGetPlayer();
/* whether the local player spectates (the network glue's; no name yet) */
extern "C" bool func_002C2FA0();

/* where the magazines' first bitmap sits (bss 0x4145C8, 0x4145D0: set by Init; no rows yet) */
extern float data_004145C8;
extern float data_004145D0;

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
    m_bitmaps[0].Load(data_004145C8, data_004145D0, own ? m_magtex : tex);
}
