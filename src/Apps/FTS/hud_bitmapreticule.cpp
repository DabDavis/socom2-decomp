/* hud_bitmapreticule.cpp -- the reticle (OVL1 ...-0x21938F): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_bitmapreticule.cpp). Written: UnInit, Draw, the constructor and destructor
 * (0x218D50-0x21932F), hudStartDefuseTimer (0x219330, the unit's last), On, Off and the pause test's
 * OnPauseTest (0x215180-0x215243), in SOCOM 1's order, HideTimer (0x213610, the unit's first) and
 * TimerProgressTick (0x213680, round 41); still to write: 0x213840-0x215170 (by SOCOM 1's order
 * UpdateGBarColor, SetGBarVal, ChangeReticule), Init (0x2178C0) and the five functions
 * 0x215250-0x2178BF. BitmapReticule is 0x12F0 bytes longer in SOCOM II, laid out whole
 * (SOCOM 1's names, SOCOM II's offsets; the reticle type is a byte now). Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "hud_mpinfobox.h"

class CHUD;

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
