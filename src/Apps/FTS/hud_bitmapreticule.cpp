/* hud_bitmapreticule.cpp -- the reticle (OVL1 ...-0x21938F): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_bitmapreticule.cpp). Written: UnInit, Draw, the constructor and destructor
 * (0x218D50-0x21932F), hudStartDefuseTimer (0x219330, the unit's last), On, Off and the pause test's
 * OnPauseTest (0x215180-0x215243), in SOCOM 1's order, HideTimer (0x213610, the unit's first) and
 * TimerProgressTick (0x213680, round 41) and SOCOM II's second bar after it, ReloadTimerTick
 * (0x213840), and SetGBarVal (0x213A60, round 57); still to write: 0x213E20-0x215170 (by SOCOM 1's
 * order ChangeReticule
 * -- UpdateGBarColor is not 0x213840), Init (0x2178C0) and the five functions 0x215250-0x2178BF. BitmapReticule is 0x12F0 bytes longer in SOCOM II, laid out whole
 * (SOCOM 1's names, SOCOM II's offsets; the reticle type is a byte now). Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mpinfobox.h"

class CHUD;
class CEntity;

namespace zdb {
/* a texture's size and a handle's texture (c2dbitmap.cpp's views) */
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};


/* the world's camera as the lased point is projected (c2dline.cpp's views: znode.h's
 * CWorld::m_world and m_camera at 0x128, the matrix set's VU block at 0x3F0, its world-to-screen
 * matrix at 0x20) */
class CFrameSetup {
public:
    unsigned char _0[0x20];
    sceVu0FMATRIX mtxWorldToScreen;   // 0x20
};

class CCamera {
public:
    unsigned char _0[0x3f0];
    CFrameSetup *m_vu;   // 0x3f0 (m_mtxSet.m_vu)
};

class CWorld {
public:
    unsigned char _0[0x128];
    CCamera *m_camera;   // 0x128
    static CWorld *m_world;
};
}

/* the seal's kit, as far as the lased point goes (kit.h's CZKit at the seal's 0x5E0: the lasing
 * flag, bit 16 of its first word, and the lased point at 0xC; the names ours) */
class CZKit {
public:
    /* the lased point (inline: read through the kit's own address, by value) */
    CPnt3D GetLasePoint() const { return m_lasePoint; }

    unsigned int m_bits0 : 16;
    unsigned int m_lasing : 1;   // 0x2 bit 0
    unsigned char _4[0xc - 0x4];
    CPnt3D m_lasePoint;   // 0xc
};

/* the seal the reticle is ticked for (zseal.h's CZSealBody, as far as this unit reads it) */
class CZSealBody {
public:
    bool IsReloading();

    unsigned char _0[0x5e0];
    CZKit m_kit;   // 0x5e0
    unsigned char _5f8[0xeb8 - 0x5f8];
    CEntity *m_eb8;   // 0xeb8: the turret it works
};

/* the HUD (hud_main.cpp's, theHUD 0x408E58): whether it shows */
class CHUD {
public:
    bool IsOn();
};

extern CHUD *theHUD;

/* the 2D library's screen line (c2dline.cpp's, 0x35FCE0) */
void Draw2DLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1);

/* the reticle's colours, built by the unit's initialiser (0x3FE490) in this order: SOCOM 1's
 * color_base (its symbol table; 12 bytes there too), then SOCOM II's two (the names ours: the
 * green and the grey 0x215C10 picks when the weapon is ready and when it has no ammo; SOCOM 1
 * computed its timer's image rectangle here instead) */
CPnt3D color_base(86.0f, 63.0f, 27.0f);
static CPnt3D s_colorReady(24.0f, 200.0f, 44.0f);
static CPnt3D s_colorNoAmmo(24.0f, 24.0f, 24.0f);

class BitmapReticule : public C2D {
public:
    BitmapReticule();
    virtual ~BitmapReticule();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Off();
    virtual void On();
    void OnPauseTest();
    void UnInit();
    void HideTimer();
    void TimerProgressTick(float dT);
    void ReloadTimerTick(float dT, CZSealBody *seal);
    void SetGBarVal(float val);
    /* the lased point marked on the screen (SOCOM II's, 0x217550: HudTick's call before
     * ThreatDirTick; the name ours) */
    void DrawLaseTarget(CZSealBody *seal);

    /* SOCOM 1's DWARF names at SOCOM II's offsets (read off the constructor 0x218DE0 and the
     * destructor 0x219090); SOCOM II's own members named by offset */
    CHUD *m_parent;   // 0x18 (SOCOM 1: 0x1c)
    /* three groups of the reticle's pieces (SOCOM II's: plain C2Ds, the constructor builds them
     * first; the names ours) */
    C2D m_group0;   // 0x1c
    C2D m_group1;   // 0x34
    C2D m_group2;   // 0x4c
    unsigned char _64[0x70 - 0x64];
    C2DBitmap m_bitmap;   // 0x70
    C2DBitmap m_grenadebitmap;   // 0xf0
    C2DBitmap m_grenadebitmap2;   // 0x170
    C2DBitmap m_miss_x;   // 0x1f0
    C2DBitmapPoly m_grenadeactive[2];   // 0x270
    C2DBitmapPoly m_grenadeback[2];   // 0x650
    C2DBitmapPoly m_scopepoly[4];   // 0xa30
    C2DBitmapPoly m_socom2_11f0[4];   // 0x11f0
    C2DBitmapPoly m_threatdirpoly[8];   // 0x19b0
    C2DBitmapPoly m_floatingreticule[4];   // 0x2930
    unsigned char _30f0[0x3100 - 0x30f0];
    C2DBitmap m_nvgrange;   // 0x3100
    C2DBitmapPoly m_nvglens[4];   // 0x3180
    C2DBitmapPoly m_socom2_3940[4];   // 0x3940
    unsigned char m_nvgenabled;   // 0x4100
    float m_socom2_4104;   // 0x4104
    C2DString m_lase_update;   // 0x4108 (SOCOM 1: 0x3124)
    float m_prev_lase;   // 0x41a0
    char m_lase_str[32];   // 0x41a4
    unsigned char m_target_verified;   // 0x41c4
    C2DString m_detonation_update;   // 0x41c8 (SOCOM 1: 0x31d4)
    char m_detonation_str[32];   // 0x4260
    unsigned char m_detonation_verified;   // 0x4280
    float m_prev_deton;   // 0x4284
    unsigned char _4288[0x4290 - 0x4288];
    C2DBitmap m_timer_IMAGE;   // 0x4290
    C2DBitmap m_timer_BKGRD;   // 0x4310
    float *m_pCurval;   // 0x4390
    float m_maxval;   // 0x4394
    float m_maxval_frac;   // 0x4398
    unsigned char m_bIncrTimer;   // 0x439c
    /* a second timer (SOCOM II's) */
    C2DBitmap m_socom2_43a0;   // 0x43a0
    C2DBitmap m_socom2_4420;   // 0x4420
    float *m_pCurval2;   // 0x44a0
    float m_maxval2;   // 0x44a4
    unsigned char m_bIncrTimer2;   // 0x44a8
    float m_posx;   // 0x44ac
    float m_posy;   // 0x44b0
    float m_retsize;   // 0x44b4
    signed char m_rettype;   // 0x44b8 (SOCOM 1: an int at 0x33ac)
    /* eleven of each in SOCOM II (ten in SOCOM 1): the constructor's loop clears index 10 */
    zdb::CTexHandle *m_reticuleTex[11];   // 0x44bc (SOCOM 1: 0x33b0)
    zdb::CTexHandle *m_floatingreticuleTex[11];   // 0x44e8
    zdb::CTexHandle *m_grenadebartex_full;   // 0x4514
    zdb::CTexHandle *m_accuracyxtex;   // 0x4518
    zdb::CTexHandle *m_threatdirTex;   // 0x451c
    unsigned char m_error;   // 0x4520
    float m_minsize;   // 0x4524
    float m_maxsize;   // 0x4528
    float m_rettexsizex;   // 0x452c
    float m_rettexsizey;   // 0x4530
    float m_rettexsizex2;   // 0x4534
    float m_rettexsizey2;   // 0x4538
    float m_threattexsizex;   // 0x453c
    float m_threattexsizey;   // 0x4540
    C2DString m_enemy_name;   // 0x4544 (SOCOM 1: 0x3430)
    CHealthBar m_healthbar;   // 0x45e0 (SOCOM II's)
} __attribute__((aligned(16)));   // 0x47b0 bytes

namespace {

/* the defuse timer's valve and its time (SOCOM 1's, its symbol table: 0x3DC558 and 0x3DC560 here,
 * 8 bytes apart; hudStartDefuseTimer, 0x219330, sets the time and creates the valve) */
const char *c_szDefusingValve = "hud_defuse_timer";
float g_fDefuseTime = -1.0f;

}

/* a valve (zanim.h's: only its creation) */
enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };

class CValve {
public:
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);
};

/* the weapons' timer registration (SOCOM 1's CZWeapon statics; only the current value's pointer)
 * and the timer registered: its value, its maximum, whether it counts up (SOCOM 1's
 * CZWeapon::RegisterTimer(float *, float, bool), 0x3D0060, the same five stores) */
class CZWeapon {
public:
    static void RegisterTimer(float *pCurval, float maxval, bool bIncr);

    static float *m_timer_pCurval;
};

/* the binoculars' reticle texture (ret_binocs2.tif): Init loads it, MapCompass's binocular view
 * draws with it, the destructor forgets it (the name ours) */
zdb::CTexHandle *binocs_tex;

/* the defuse timer started: its time kept, the weapons' timer pointed at it and its valve made --
 * unless the timer already shows it (SOCOM 1's hudStartDefuseTimer(float); SOCOM II's takes whether
 * the timer counts up and registers it with the weapons' timer) */
void hudStartDefuseTimer(bool bIncr, float t)
{
    g_fDefuseTime = t;
    if (CZWeapon::m_timer_pCurval != &g_fDefuseTime) {
        CZWeapon::RegisterTimer(&g_fDefuseTime, t, bIncr);
        CValve::Create(c_szDefusingValve, 1, VALVE_TEMP);
    }
}

/* the pieces built, the texture tables cleared, no reticle type, the timers stopped (SOCOM 1's
 * order; SOCOM II adds the 64.0 at 0x4104, the second timer and a reticle type of -1) */
BitmapReticule::BitmapReticule()
{
    for (int i = 0; i < 11; i++) {
        m_floatingreticuleTex[i] = 0;
        m_reticuleTex[i] = 0;
    }
    m_grenadebartex_full = 0;
    m_threatdirTex = 0;
    m_accuracyxtex = 0;
    m_rettype = -1;
    m_posx = 0.0f;
    m_posy = 0.0f;
    m_retsize = 0.0f;
    m_minsize = 0.0f;
    m_maxsize = 2.0f;
    m_rettexsizex = 0.0f;
    m_rettexsizey = 0.0f;
    m_rettexsizex2 = 0.0f;
    m_rettexsizey2 = 0.0f;
    m_threattexsizex = 0.0f;
    m_threattexsizey = 0.0f;
    m_error = false;
    m_nvgenabled = false;
    m_socom2_4104 = 64.0f;
    m_target_verified = false;
    m_prev_lase = 0.0f;
    strcpy(m_lase_str, "UNUSED");
    strcpy(m_detonation_str, "UNUSED");
    m_detonation_verified = false;
    m_prev_deton = 0.0f;
    m_pCurval = 0;
    m_maxval = -1.0f;
    m_bIncrTimer = true;
    m_pCurval2 = 0;
    m_maxval2 = -1.0f;
    m_bIncrTimer2 = true;
    g_fDefuseTime = -1.0f;
}

/* the binoculars' texture forgotten; the members destroyed (SOCOM 1's, 0x3BCAD0 there) */
BitmapReticule::~BitmapReticule()
{
    binocs_tex = 0;
}

/* the reticle on: itself and its three groups (SOCOM 1's On, which only turned itself on) */
void BitmapReticule::On()
{
    C2D::On();
    m_group0.On();
    m_group1.On();
    m_group2.On();
}

/* the reticle's groups off (SOCOM 1's Off; the reticle itself is left on) */
void BitmapReticule::Off()
{
    m_group0.Off();
    m_group1.Off();
    m_group2.Off();
}

/* the second group shown alone: CHUD::Off calls it in the pause test (SOCOM II's; the name ours) */
void BitmapReticule::OnPauseTest()
{
    m_group1.On();
}

/* the reticle drawn if its type has a texture */
void BitmapReticule::Draw(zdb::CCamera *cam)
{
    if (m_reticuleTex[m_rettype])
        C2D::Draw(cam);
}

void BitmapReticule::UnInit()
{
    m_lase_update.ClearAndWipe();
    m_detonation_update.ClearAndWipe();
    m_enemy_name.ClearAndWipe();
}

/* a bar's quad from its right and left, its far end and its base (the name ours): the inline's
 * arguments evaluate right to left, so the far end, the left and the right are computed in that
 * order, as the target's; the base by reference -- read through it, its load is not merged with the
 * far end's (the target loads the y twice for each bar; by value the two loads are one) */
/* the lased point's box: its size and its ticks' length (OVL1 .data 0x3DC568 and 0x3DC570, 10
 * and 4, after the defuse timer's statics; the names ours) */
namespace {
float s_laseBoxSize = 10.0f;
float s_laseTickSize = 4.0f;
}

/* the lased point marked (SOCOM II's; the name ours): with the HUD on and the seal's kit lasing,
 * the kit's point projected through the world camera's world-to-screen matrix onto the screen
 * (whole-screen coordinates less the 2048 offset, from the 640x448 frame's centre) and a red box
 * drawn round it -- its two uprights, then a tick at each corner fading to 0.2 toward the inside.
 * OPEN, 0x36C against 0x364 (round 59, written): the logic, the frame (the point, the 4D input, the
 * output, the matrix, the three colours and the three corners at the target's slots) and the copies
 * (the first corner copy a declaration -- lq / sq --, the later ones assignments, member by member)
 * are the target's; it reads the lased point through the kit's address (`addiu a0,s0,0x5E0` in the
 * lasing test's delay slot, then 0xC(a0)) where MW folds the offsets (0x5EC(s0)) whether the kit is
 * reached through a pointer or a reference local, before or after the test, or the by-value
 * GetLasePoint; and the projection's FPRs differ (the target: x f4, y f2, 2048 f3, 320 f1, 224 f0),
 * with two alignment nops more here -- the colours declared first, 1/w first or `out` scaled in
 * place change nothing (or grow it) */
void BitmapReticule::DrawLaseTarget(CZSealBody *seal)
{
    if (!theHUD->IsOn())
        return;
    if (!seal->m_kit.m_lasing)
        return;

    CPnt3D pos = seal->m_kit.GetLasePoint();
    CPnt4D in(pos.x, pos.y, pos.z, 1.0f);
    CPnt4D out;
    CMatrix m = *(CMatrix *)zdb::CWorld::m_world->m_camera->m_vu->mtxWorldToScreen;

    m.Transform(&in, &out, 1);

    float iw = 1.0f / out.w;
    float x = out.x * iw - 2048.0f + 320.0f;
    float y = out.y * iw - 2048.0f + 224.0f;
    CPnt4D red(1.0f, 0.0f, 0.0f, 1.0f);
    CPnt4D red2(1.0f, 0.0f, 0.0f, 1.0f);
    CPnt4D fade(1.0f, 0.0f, 0.0f, 0.2f);
    CPnt4D a(x, y, 0.0f, 1.0f);
    CPnt4D b(x, y, 0.0f, 1.0f);

    a.x -= s_laseBoxSize;
    a.y += s_laseBoxSize;
    b.x -= s_laseBoxSize;
    b.y -= s_laseBoxSize;
    Draw2DLine(&a, &b, &red, &red2);
    a.x += 2.0f * s_laseBoxSize;
    b.x += 2.0f * s_laseBoxSize;
    Draw2DLine(&a, &b, &red, &red2);

    CPnt4D c = a;

    c.x -= s_laseTickSize;
    Draw2DLine(&a, &c, &red, &fade);
    c = b;
    c.x -= s_laseTickSize;
    Draw2DLine(&c, &b, &fade, &red2);
    a.x -= 2.0f * s_laseBoxSize;
    c = a;
    c.x += s_laseTickSize;
    Draw2DLine(&a, &c, &red, &fade);
    b.x -= 2.0f * s_laseBoxSize;
    c = b;
    c.x += s_laseTickSize;
    Draw2DLine(&c, &b, &fade, &red2);
}

static inline void LoadBar(C2DBitmapPoly &poly, float right, float left, float end, const float &base,
                           zdb::CTexture *tex)
{
    poly.Load(right, end, right, base, left, base, left, end, tex);
}

/* the grenade bar (SOCOM 1's SetGBarVal, 0x3CD490 there; 0x213A60): for the grenade reticle only --
 * a missing texture flags the error once -- the reticle's own picture left of the point, the full
 * bar's two halves grown out from it by the value and the empty bar's two drawn whole, the two
 * level bitmaps split at the value; the picture and the levels opaque. SOCOM 1's DWARF names the
 * floats (its two statics one flag here) */
void BitmapReticule::SetGBarVal(float val)
{
    static bool first = true;

    if (m_rettype == 4) {
        if (!m_reticuleTex[m_rettype] && first) {
            m_error = 1;
            first = false;
        }
        if (!m_grenadebartex_full && first) {
            m_error = 1;
            first = false;
        }

        zdb::CTexHandle *tex = m_reticuleTex[m_rettype];
        float y = m_posy;
        float x = m_posx;
        float height = (int)tex->m_texture->m_width;
        float half_height = 0.5f * height;
        float half_width = 0.5f * (int)tex->m_texture->m_height;

        m_bitmap.Load(x - half_width, y - half_height, x, y + half_height, tex);
        m_bitmap.SetUV(0.0f, 0.0f, 0.5f, 1.0f);
        m_bitmap.Off();

        float d = val * half_height;

        LoadBar(m_grenadeactive[0], m_posx + half_width, m_posx - half_width, m_posy - d, m_posy,
                m_grenadebartex_full->m_texture);
        float rest = 1.0f - val;

        m_grenadeactive[0].SetUV(0.0f, rest, 1.0f, 1.0f);
        LoadBar(m_grenadeactive[1], m_posx + half_width, m_posx - half_width, m_posy + d, m_posy,
                m_grenadebartex_full->m_texture);
        m_grenadeactive[1].SetUV(0.0f, rest, 1.0f, 1.0f);
        LoadBar(m_grenadeback[0], m_posx + half_width, m_posx - half_width, m_posy - half_height, m_posy,
                m_reticuleTex[m_rettype]->m_texture);
        m_grenadeback[0].SetUV(0.0f, 0.0f, 1.0f, 1.0f);
        LoadBar(m_grenadeback[1], m_posx + half_width, m_posx - half_width, m_posy + half_height, m_posy,
                m_reticuleTex[m_rettype]->m_texture);
        m_grenadeback[1].SetUV(0.0f, 0.0f, 1.0f, 1.0f);
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadebitmap.Load(m_posx, m_posy + half_height - val * height, m_posx + half_width,
                             m_posy + half_height, m_grenadebartex_full);
        m_grenadebitmap.SetUV(0.5f, 0.97f - val, 1.0f, 1.0f);
        m_grenadebitmap2.Load(m_posx, m_posy - half_height, m_posx + half_width,
                              m_posy - half_height + rest * height, m_reticuleTex[m_rettype]);
        m_grenadebitmap2.SetUV(0.5f, 0.0f, 1.0f, rest);
        m_bitmap.SetTrans(128.0f);
        m_grenadebitmap.SetTrans(128.0f);
        m_grenadebitmap2.SetTrans(128.0f);
    }
}

/* the second bar (SOCOM II's, below the timer's; the name ours): while the seal reloads at a turret,
 * shown with the second timer's fraction -- none without a maximum -- clamped to 0..1 (counting
 * down unless it counts up); otherwise both faded out at 400 a second and turned off at 0 (0x213840;
 * 0x216770 calls it after TimerProgressTick) */
void BitmapReticule::ReloadTimerTick(float dT, CZSealBody *seal)
{
    if (seal->IsReloading() && seal->m_eb8) {
        float t = m_pCurval2 ? *m_pCurval2 : 0.0f;
        float frac;

        t = t < 0.0f ? 0.0f : t;
        if (m_maxval2 != 0.0f)
            frac = 1.0f / m_maxval2;
        else
            frac = 0.0f;
        t *= frac;
        if (!m_bIncrTimer2)
            t = 1.0f - t;
        t = t < 0.0f ? 0.0f : t;
        if (t > 1.0f)
            t = 1.0f;
        m_socom2_4420.On();
        m_socom2_4420.SetTrans(38.4f);
        m_socom2_43a0.On();
        m_socom2_43a0.SetTrans(100.0f);
        m_socom2_43a0.SetPos(534, 208, (int)(534.0f + 50.0f * t), 220);
    } else {
        float trans_val = m_socom2_43a0.GetTrans() - 400.0f * dT;

        if (trans_val < 0.0f) {
            trans_val = 0.0f;
            m_socom2_43a0.Off();
            m_socom2_4420.Off();
        }
        m_socom2_43a0.SetTrans(trans_val);
        m_socom2_4420.SetTrans(trans_val);
    }
}

/* the timer bar: while a timer runs, its background and image shown, the image's right edge at the
 * time's fraction (counting down unless it counts up), the timer let go once it leaves 0..max; with
 * none, both faded out at 400 a second and turned off at 0 (SOCOM 1's TimerProgressTick, the bar's
 * rectangle constants now and the frame time a parameter, 0x213680; 0x216770 calls it). The clamp
 * at 0 is a ternary assigned back: as an `if` MW pads its join and not the count-down's */
void BitmapReticule::TimerProgressTick(float dT)
{
    if (m_pCurval) {
        float bomb_timer = *m_pCurval;

        bomb_timer = bomb_timer < 0.0f ? 0.0f : bomb_timer;
        bomb_timer *= m_maxval_frac;
        if (!m_bIncrTimer)
            bomb_timer = 1.0f - bomb_timer;
        m_timer_BKGRD.On();
        m_timer_BKGRD.SetTrans(38.4f);
        m_timer_IMAGE.On();
        m_timer_IMAGE.SetTrans(100.0f);
        m_timer_IMAGE.SetPos(534, 187, (int)(534.0f + 50.0f * bomb_timer), 199);
        if (!(*m_pCurval <= m_maxval))
            m_pCurval = 0;
        else if (*m_pCurval < 0.0f)
            m_pCurval = 0;
    } else {
        float trans_val = m_timer_IMAGE.GetTrans() - 400.0f * dT;

        if (trans_val < 0.0f) {
            trans_val = 0.0f;
            m_timer_IMAGE.Off();
            m_timer_BKGRD.Off();
        }
        m_timer_IMAGE.SetTrans(trans_val);
        m_timer_BKGRD.SetTrans(trans_val);
    }
}

/* the timer's four bitmaps faded out (SOCOM II's, the unit's first in the image: CHUD::ReInit_Init
 * calls it; the name ours) */
void BitmapReticule::HideTimer()
{
    m_timer_IMAGE.SetTrans(0.0f);
    m_timer_BKGRD.SetTrans(0.0f);
    m_socom2_43a0.SetTrans(0.0f);
    m_socom2_4420.SetTrans(0.0f);
}

/* the body's blend stack (zbody.h's RTStack, SOCOM 1's; only its Top here): this unit's tail holds
 * the one copy of Top for CZBodyAnimBlend (0x219390, after hudStartDefuseTimer), which body.cpp
 * calls -- the reticule's unwritten 0x215C10 instantiates it, explicitly instantiated here until
 * that is written */
class CZBodyAnimBlend;

template <class T>
class RTStack {
public:
    T *Top();

    T **m_stack;   // 0x0
    int m_size;   // 0x4
    int m_top;   // 0x8
};

/* the top item, none when empty (SOCOM 1's) */
template <class T>
T *RTStack<T>::Top()
{
    if (m_top < 0)
        return 0;
    return m_stack[m_top];
}

template class RTStack<CZBodyAnimBlend>;
