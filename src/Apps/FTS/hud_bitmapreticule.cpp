/* hud_bitmapreticule.cpp -- the reticle (OVL1 ...-0x21938F): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_bitmapreticule.cpp). Written: UnInit, Draw, the constructor and destructor
 * (0x218D50-0x21932F), hudStartDefuseTimer (0x219330, the unit's last), On, Off and the pause test's
 * OnPauseTest (0x215180-0x215243), in SOCOM 1's order, HideTimer (0x213610, the unit's first) and
 * TimerProgressTick (0x213680, round 41) and SOCOM II's second bar after it, ReloadTimerTick
 * (0x213840), SetGBarVal (0x213A60, round 57); ChangeReticule (0x213E20, the block to 0x215170), Init
 * (0x2178C0), UpdateAccuracy (0x215250) and ThreatDirTick (0x216DE0) byte-exact in round 66, HudTick
 * (0x216770: round 64) and DrawLaseTarget (0x217550: round 59, byte-exact since round 61) done;
 * UpdateColor (0x215C10) written and open. BitmapReticule is 0x12F0
 * bytes longer in SOCOM II, laid out whole
 * (SOCOM 1's names, SOCOM II's offsets; the reticle type is a byte now). Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zvid/zvid.h"
#include "hud_mpinfobox.h"

class CHUD;
class CEntity {
public:
    zdb::CNode *GetNode();
};

namespace zdb {
/* a node's name (znode.h's) */
class CNode {
public:
    char *GetName() { return m_name; }

    unsigned char _0[0x90];
    char *m_name;   // 0x90
};

/* a texture's size and a handle's texture (c2dbitmap.cpp's views) */
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
    unsigned char _4[0x18 - 0x4];
    unsigned char *m_pixels;   // 0x18
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

    /* the night vision's flag, the reticle's kind changed and the reticle to show, whether the weapon
     * is in its launcher mode (kit.h's, SOCOM 1's) */
    bool NVG_Enabled() const;
    bool CurReticuleTypeChanged();
    int GetCurItemReticule();
    bool CurWeaponIsLauncherMode();
    /* the reticle's blocked flag set (SOCOM 1's bit 3 of its first word, m_retblocked) */
    void SetRetBlocked(bool b) { m_retblocked = b; }

    unsigned int m_bits0_0 : 3;
    unsigned int m_retblocked : 1;   // bit 3
    unsigned int m_bits0 : 12;
    unsigned int m_lasing : 1;   // 0x2 bit 0
    unsigned char _4[0xc - 0x4];
    CPnt3D m_lasePoint;   // 0xc
    float m_retposx;   // 0x18 (SOCOM 1's four, at 0x4 there)
    float m_retposy;   // 0x1c
    float m_retoffsetx;   // 0x20
    float m_retoffsety;   // 0x24
    unsigned char _28[0x84c - 0x28];
    float m_retsize;   // 0x84c: the reticle's spread (the seal's 0xE2C)
    unsigned char _850[0x864 - 0x850];
    float m_retblockedX;   // 0x864 (SOCOM 1's 0x848, kit.h)
    float m_retblockedY;   // 0x868 (SOCOM 1's 0x84c)
    /* whether the seal looks through binoculars or a scope (kit.h's, SOCOM 1's) */
    bool IsUsingBinocs();
    bool IsUsingScope();
    int GetCurItemEnum() const;
    class CZWeapon *GetCurItem() const;
    bool CurItemHasAmmo();
    float GetReticuleSettle();
};

/* what the seal hit along the reticle: the point, and what it hit at 0x1c (zdb's CDI, as far as the
 * range reads it) */
class CReticuleDi : public CPnt3D {
public:
    unsigned char _c[0x1c - 0xc];
    void *m_hit;   // 0x1c
};

/* the seal's controller: its menu state (zseal.h's) */
class CSealCtrl {
public:
    unsigned char _0[0x221];
    unsigned char m_menustate;   // 0x221 (a byte enum, compared with lbu)
};

/* the view the player looks through (zseal.h's: a signed byte) */
enum ZOOMSTATE { ZOOMSTATE_UNKNOWN = -1, ZOOMSTATE_3RDPERSON = 0, ZOOMSTATE_1STPERSON = 1 };

/* the seal's target list, body and anim stack, as far as the colour goes (their classes': SOCOM 1's names) */
class CTarget {
public:
    unsigned char _0[0x2c];
    unsigned int m_flags;   // 0x2c
};
class CZSealBody;
class CTargetList {
public:
    CTarget *FindTarget(CEntity *e);
    unsigned char _0;
};
class CZBodyPart;
class CBody {
public:
    void PartInSpace(CZBodyPart *part, const CPnt3D *from, CPnt3D *out, CQuat *q) const;
    unsigned char _0[4];
};
class CZBodyAnimBlend {
public:
    unsigned char _0[0x2b];
    signed char m_2b;   // 0x2b
};
template <class T>
class RTStack {
public:
    T *Top();

    T **m_stack;   // 0x0
    int m_size;   // 0x4
    int m_top;   // 0x8
};

/* the seal the reticle is ticked for (zseal.h's CZSealBody, as far as this unit reads it) */
class CZSealBody {
public:
    bool IsReloading();
    /* the view's setter and what the reticle is on (zseal.h's, SOCOM 1's) */
    void SetCurZoomState(ZOOMSTATE state);
    CReticuleDi *GetReticuleDi();
    CZKit *GetKit() { return &m_kit; }
    float GetThreatScale(unsigned int i) const { return m_threatscale[i]; }
    CZSealBody *GetReticuleSeal();
    void GetFP(CPnt3D &fp);
    /* its name, else its node's (SOCOM 1's CEntity::GetName) */
    const char *GetName()
    {
        const char *n = m_name;

        if (n)
            return n;
        return ((CEntity *)this)->GetNode()->GetName();
    }
    /* the barrel's part (the entity's vtable slot +0x88: this view has no table) */
    CZBodyPart *GetBarrelPart();

    unsigned char _0[0x14];
    const char *m_name;   // 0x14
    unsigned char _18[0x1c - 0x18];
    CPnt3D m_posW;   // 0x1c
    unsigned char _28[0x50 - 0x28];
    CQuat m_rot;   // 0x50
    unsigned char _60[0xc0 - 0x60];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_flagsC8;   // 0xc8
    unsigned char _cc[0xd0 - 0xcc];
    CTargetList m_targets;   // 0xd0
    unsigned char _d1[0xe1 - 0xd1];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
    unsigned char _e2[0x170 - 0xe2];
    CBody m_body;   // 0x170
    short m_174;   // 0x174: 9 hides the error marker (SOCOM 1's byte at 0x11D)
    unsigned char _176[0x1c0 - 0x176];
    RTStack<CZBodyAnimBlend> m_animStack;   // 0x1c0
    unsigned char _1cc[0x200 - 0x1cc];
    ZOOMSTATE m_zoomstate;   // 0x200
    unsigned char _201[0x208 - 0x201];
    float m_208;   // 0x208: how far the reticle's point is, squared
    unsigned char _20c[0x258 - 0x20c];
    unsigned char m_258_0 : 1;   // 0x258 bit 0 (zseal.h's)
    unsigned char _259[0x3c0 - 0x259];
    CPnt3D m_target3c0;   // 0x3c0
    unsigned char _3cc[0x5e0 - 0x3cc];
    CZKit m_kit;   // 0x5e0, to 0xe4c
    unsigned char _e4c[0xeb8 - 0xe4c];
    CEntity *m_eb8;   // 0xeb8: the turret it works
    unsigned char _ebc[0xefc - 0xebc];
    /* where it was hit from, and each of the eight arrows' strength and angle (SOCOM 1's DWARF: E, N,
     * W, S at the even places, the diagonals between) */
    unsigned char m_threatdir[4];   // 0xefc
    float m_threatscale[8];   // 0xf00
    float m_threatangle[8];   // 0xf20
    unsigned char _f40[0xfc8 - 0xf40];
    int m_persona;   // 0xfc8
};

/* the asset manager's library list (node_world.h's) */
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

/* the HUD's lower block's top (hud_main.cpp's layout float, 364) and a texture's palette (0x354A90, ztex's) */
extern float g_fHudBottomBlockY;
extern "C" void func_00354A90(zdb::CTexture *tex, int r, int g, int b);
extern "C" int rand();
extern "C" void FlushCache(int op);
extern "C" int sprintf(char *buf, const char *fmt, ...);

/* the HUD (hud_main.cpp's, theHUD 0x408E58): whether it shows */
class CHUD {
public:
    static CPnt4D m_hud_textcolor;
    bool IsOn();
    static CZSealBody *GetHUDOwner();
    void SetRangeText(const char *text);
    void ShowRangeText();
    void HideRangeText();
    void SetRangeTextPos(float x, float y);

    unsigned char _0[0xb0];
    signed char m_hudrettype;   // 0xb0
    bool m_usehudret;   // 0xb1
    bool m_hudretchanged;   // 0xb2
    unsigned char _b3[0x17b64 - 0xb3];
    unsigned char m_17b64;   // 0x17b64
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
    void HudTick(float dT);
    void ThreatDirTick(CZSealBody *seal);
    void UpdateAccuracy();
    void UpdateColor();
    void ChangeReticule(int type);
    void Init(zdb::CWorld *world, C2DFont *font);

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
    zdb::CTexture *m_noisetex;   // 0x30f0: the night vision's noise, drawn anew each tick
    unsigned char _30f4[0x3100 - 0x30f4];
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

/* the entity's table as far as the barrel's part (slot +0x88; a view, the names ours): called as
 * a virtual, through t9, where a function pointer goes through v0 */
class CBarrelView {
public:
    virtual void V08();
    virtual void V0C();
    virtual void V10();
    virtual void V14();
    virtual void V18();
    virtual void V1C();
    virtual void V20();
    virtual void V24();
    virtual void V28();
    virtual void V2C();
    virtual void V30();
    virtual void V34();
    virtual void V38();
    virtual void V3C();
    virtual void V40();
    virtual void V44();
    virtual void V48();
    virtual void V4C();
    virtual void V50();
    virtual void V54();
    virtual void V58();
    virtual void V5C();
    virtual void V60();
    virtual void V64();
    virtual void V68();
    virtual void V6C();
    virtual void V70();
    virtual void V74();
    virtual void V78();
    virtual void V7C();
    virtual void V80();
    virtual void V84();
    virtual CZBodyPart *GetBarrelPart();   /* +0x88 */
};
inline CZBodyPart *CZSealBody::GetBarrelPart() { return ((CBarrelView *)this)->GetBarrelPart(); }

/* the weapons' timer registration (SOCOM 1's CZWeapon statics; only the current value's pointer)
 * and the timer registered: its value, its maximum, whether it counts up (SOCOM 1's
 * CZWeapon::RegisterTimer(float *, float, bool), 0x3D0060, the same five stores) */
class CZWeapon {
public:
    float GetArmingDistance();
    unsigned char _0[0x7c];
    unsigned char m_item;   // 0x7c
    static void RegisterTimer(float *pCurval, float maxval, bool bIncr);
    static bool TimerCleared();
    static bool TimerRegistered();
    static void RetrieveTimerRegistration(float **pCurval, float *maxval, bool *bIncr);
    static bool Timer2Registered();
    static void RetrieveTimer2Registration(float **pCurval, float *maxval, bool *bIncr);
    static void ClearTimer(float *pCurval);

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

/* a sum one inline down (its frame slot after the differences') */
static inline CPnt3D AddPos(const CPnt3D &a, const CPnt3D &b)
{
    return a + b;
}

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};
extern CZNetwork theNetwork;

/* an ally's name: blank where the HUD hides names */
static inline const char *MateName(CZSealBody *seal)
{
    const char *n;

    if (theHUD->m_17b64)
        n = " ";
    else
        n = seal->m_name;
    return n;
}

static inline const char *AllyName(CZSealBody *seal)
{
    const char *n;

    if (theNetwork.m_bNetwork)
        n = MateName(seal);
    else
        n = " ";
    return n;
}

/* the arming distance read through an inline of its own: the extra inline level's empty blocks keep
 * the `b` over the else arm and the block ahead of Top unfilled, as the target has them (round 74) */
static inline float ArmDistOf(CZWeapon *w) { return w->GetArmingDistance(); }

static inline float ArmDist(CZWeapon *w, float d)
{
    float r;

    if (w)
        r = ArmDistOf(w);
    else
        r = d;
    return r;
}

/* the reticle's blocked and foe colours (.data, in threes: 130, 130, 130 and 200, 200, 24; the names ours) */
float s_colorBlockedR = 130.0f;
float s_colorBlockedG = 130.0f;
float s_colorBlockedB = 130.0f;
float s_colorFoeR = 200.0f;
float s_colorFoeG = 200.0f;
float s_colorFoeB = 24.0f;

/* the seal's and the world's views UpdateColor needs: what the reticle hit, its owner (zseal.h's) */
class CDiTarget {
public:
    virtual ~CDiTarget();
    virtual void Slot0C();
    virtual bool CanLock(float range);   // +0x10
};
class CDiHit {
public:
    /* what a hit belongs to, called on a possibly-null hit (SOCOM 1's a member's inline) */
    CDiTarget *GetOwner()
    {
        CDiTarget *o = 0;

        if (this)
            o = m_owner;
        return o;
    }

    unsigned char _0[0x94];
    CDiTarget *m_owner;   // 0x94
};
class CNet111 {
public:
    unsigned char GetMode() const { return this ? m_111 : 0; }

    unsigned char _0[0x111];
    signed char m_111;   // 0x111
};
extern CNet111 *data_00437CE8;
extern "C" float func_005B82D0(CZKit *kit);
extern "C" bool func_002C30B0(CZSealBody *seal);
extern "C" bool func_003C5CA0(CZWeapon *w);
extern "C" bool func_003C5C70(CZWeapon *w);
extern "C" bool func_005A78D0(CZSealBody *seal);
class CZPersonaState {
public:
    static const char *GetPersonaClanTag(int client);
};

/* the reticle's colours and the name under it (SOCOM 1's UpdateColor, on SOCOM II's friend tests, ammo and health
 * bar): grey when blocked or the weapon can't reach, the no-ammo and ready colours; the seal it is on named in red,
 * green or blue by side, with the clan tag and an ally's health */
/* Byte-exact since the OVL1 lane's round 74: the arming distance's call one inline level deeper (ArmDistOf
 * inside ArmDist) gives the three nops round 73 left -- after mov.s and lwc1 f20, and the `b` before the
 * arming distance's join not filled from its target. Round 73: OPEN at 0xB48, 3 off.
 * Closed: the frame and the block copy of (2) and the madd.s nop of (1) by the two points INITIALISED from
 * CPnt3D::zero in one declaration (`CPnt3D tip = CPnt3D::zero, q = ...`: MW shares one temporary for the
 * repeated struct copy -- the target's 288, copied by block through a2 twice); the barrel's part called as a
 * virtual (CBarrelView: t9, where the function-pointer view went through v0). Was:
 * OPEN, 28 structural differences and 9 instructions differing in registers alone (0xB40 against
 * 0xB50; the GPRs and FPRs are right now: `scale` assigned, named and declared ahead of r, g, b, for
 * a `?:` value is a temporary numbered above them, coloured first and taking f20). What differs:
 * (1) three nops short, all between an FPR def ending a block and the label it falls to: `madd.s;
 * nop` after the muzzle's distance, `mov.s f0,f23; nop` in the arming distance's else, `lwc1 f20; nop`
 * ahead of Top -- and `b; nop` where mine steals the `mul.s` at the arm's join. Unused labels, gotos
 * to labels there and an inline's early return leave the size at 0xB40. (2) The frame: the zero copy
 * is the last object created (288, op+'s r 304), mine has them swapped whatever the depth of either's
 * wrappers (depth 1-4 tried); and the target copies it by block through a2 (`ld/lwc1 ...(a2)` twice),
 * mine reads it as scalars (a reference and a pointer copy alike; a by-value parameter as well).
 * (3) The last arm's two constants live together in the target (`lui v1; lui v0; mtc1 v1; mtc1 v0`). */
void BitmapReticule::UpdateColor()
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (seal && seal->m_socom2_e1_4) {
    float scale;
    float r, g, b;
    float RetSize = m_retsize;
    bool grenade = false;
    float x = seal->m_kit.m_retblockedX;
    float y = seal->m_kit.m_retblockedY;
    bool noammo = false;

    seal->m_208 = 100000.0f;
    if (seal->m_kit.m_retblocked && (!(x <= RetSize) || !(y <= RetSize))) {
        r = s_colorBlockedR;
        g = s_colorBlockedG;
        b = s_colorBlockedB;
        m_enemy_name.SetString("", false);
        m_enemy_name.Off();
        m_healthbar.Off();
    } else {
        bool isAlly;
        CZSealBody *pRetSeal;
        bool isMate;
        CReticuleDi *di;
        CZKit *kit;

        pRetSeal = seal->GetReticuleSeal();
        kit = &seal->m_kit;

        CZWeapon *item = kit ? kit->GetCurItem() : 0;
        bool show = false;

        isAlly = false;
        isMate = false;
        di = seal->GetReticuleDi();
        if (di)
            *di - seal->m_posW;
        CDiTarget *owner = di ? ((CDiHit *)di->m_hit)->GetOwner() : 0;

        if (!owner || !owner->CanLock(kit ? func_005B82D0(kit) : 0.0f))
            di = 0;
        float dist = 100000.0f;

        if (di) {
            CPnt3D muzzle;
            CPnt3D tip = CPnt3D::zero, q = CPnt3D::zero;

            seal->m_body.PartInSpace(seal->GetBarrelPart(), &q, &tip, 0);
            seal->m_rot.Apply(&tip, &tip, 1);
            muzzle = AddPos(tip, seal->m_posW);

            CPnt3D d = *di - muzzle;

            dist = d.x * d.x + d.y * d.y + d.z * d.z;
        }
        seal->m_208 = dist;
        if (!kit->CurItemHasAmmo()) {
            noammo = true;
            r = s_colorNoAmmo.x;
            g = s_colorNoAmmo.y;
            b = s_colorNoAmmo.z;
        }
        if (item && item->m_item == 0x97 && !noammo) {
            bool ready = seal->m_208 < 396.0f && seal->m_kit.GetReticuleSettle() == 0.0f;

            if (ready) {
                r = s_colorReady.x;
                g = s_colorReady.y;
                b = s_colorReady.z;
            } else {
                r = s_colorFoeR;
                g = s_colorFoeG;
                b = s_colorFoeB;
            }
            grenade = true;
        }
        const char *display_name = 0;

        if (!grenade && !noammo) {
            if (pRetSeal && pRetSeal->m_socom2_e1_4) {
                CPnt3D dxz;

                pRetSeal->m_posW.SubXZ(&seal->m_posW, &dxz);
                float d2 = dxz.x * dxz.x + dxz.z * dxz.z;
                float limit = m_rettype != 5 ? 102400.0f : 250000.0f;

                if (!(d2 <= limit))
                    show = false;
                else {
                    unsigned int flags = pRetSeal->m_flagsC8;

                    isAlly = (flags & seal->m_flagsC8) != 0;
                    if (theNetwork.m_bNetwork) {
                        if (flags & 0x20000) {
                            isMate = true;
                            isAlly = isMate;
                        }
                    } else if ((flags & 0x80000000) && !(flags & 2) && !(flags & 0x40000)) {
                        isMate = true;
                        isAlly = isMate;
                    }
                    if (isAlly)
                        show = true;
                    else {
                        CTarget *target = seal->m_targets.FindTarget((CEntity *)pRetSeal);

                        show = target ? (target->m_flags & 0x200) != 0 : false;
                    }
                }
            }
            if (!show) {
                r = s_colorFoeR;
                g = s_colorFoeG;
                b = s_colorFoeB;
                m_enemy_name.Off();
                m_healthbar.Off();
            } else if (isMate) {
                if (data_00437CE8->GetMode() == 3 && func_002C30B0(seal)) {
                    r = s_colorFoeR;
                    g = 24.0f;
                    b = 44.0f;
                } else {
                    r = 24.0f;
                    g = s_colorFoeG;
                    b = 44.0f;
                }
                display_name = pRetSeal->GetName();
            } else if (isAlly) {
                g = s_colorFoeG;
                r = 24.0f;
                b = 44.0f;
                display_name = AllyName(pRetSeal);
            } else {
                r = s_colorFoeR;
                g = 24.0f;
                b = 44.0f;
                display_name = pRetSeal->m_name;
            }
        }
        if (display_name) {
            const char *tag = pRetSeal ? CZPersonaState::GetPersonaClanTag(pRetSeal->m_persona) : 0;
            char text[64];

            if (tag && strlen(tag) > 1 && strcmp(display_name, " "))
                sprintf(text, "[%s] %s", tag, display_name);
            else
                strcpy(text, display_name);
            m_enemy_name.SetString(text, true);

            if (m_rettype == 5)
                scale = 0.5f;
            else
                scale = 1.0f;

            m_enemy_name.SetPos((int)(m_posx + (scale * m_rettexsizex) / 2.0f), (int)(m_posy - (scale * m_rettexsizey) / 2.0f));
            m_enemy_name.On();
            if (isMate || isAlly) {
                m_healthbar.SetPos(m_posx + (scale * m_rettexsizex) / 2.0f, (m_posy - (scale * m_rettexsizey) / 2.0f) - 15.0f);
                m_healthbar.Update(pRetSeal, true);
                m_healthbar.On();
            }
        } else {
            m_enemy_name.SetString("", false);
            m_enemy_name.Off();
            m_healthbar.Off();
        }
    }
    CZKit *curkit = &seal->m_kit;
    CZWeapon *cur = curkit ? curkit->GetCurItem() : 0;

    if (cur && (func_003C5CA0(cur) || func_003C5C70(cur))) {
        CPnt3D target(seal->m_target3c0);
        CPnt3D fp = target;

        seal->GetFP(fp);

        CPnt3D v = target - fp;
        float d2 = v.x * v.x + v.y * v.y + v.z * v.z;
        float arm = ArmDist(cur, d2);

        if (d2 < arm * arm) {
            r = s_colorBlockedR;
            g = s_colorBlockedG;
            b = s_colorBlockedB;
        }
    }
    CZBodyAnimBlend *blend = seal->m_animStack.Top();

    if (func_005A78D0(seal) || blend->m_2b == 5) {
        r = s_colorBlockedR;
        g = s_colorBlockedG;
        b = s_colorBlockedB;
    }
    if (grenade) {
        m_grenadeactive[0].SetColor(r, g, b);
        m_grenadeactive[1].SetColor(r, g, b);
        m_grenadeback[0].SetColor(r, g, b);
        m_grenadeback[1].SetColor(r, g, b);
    } else {
        m_floatingreticule[0].SetColor(r, g, b);
        m_floatingreticule[1].SetColor(r, g, b);
        m_floatingreticule[2].SetColor(r, g, b);
        m_floatingreticule[3].SetColor(r, g, b);
        if (m_rettype == 3) {
            m_scopepoly[0].SetColor(r, g, b);
            m_scopepoly[1].SetColor(r, g, b);
            m_scopepoly[2].SetColor(r, g, b);
            m_scopepoly[3].SetColor(r, g, b);
        } else {
            m_scopepoly[0].SetColor(128.0f, 128.0f, 128.0f);
            m_scopepoly[1].SetColor(128.0f, 128.0f, 128.0f);
            m_scopepoly[2].SetColor(128.0f, 128.0f, 128.0f);
            m_scopepoly[3].SetColor(128.0f, 128.0f, 128.0f);
        }
    }
}
}

/* the error marker and the floating reticle's four pieces placed round the point, and the threat
 * arrows round them (SOCOM 1's; a body of its order with SOCOM II's fade on the marker, its third kind of
 * reticle and its zoom's half size) */
void BitmapReticule::UpdateAccuracy()
{
    static float PT_NORTH;
    static float PT_EAST;
    static float PT_SOUTH;
    static float PT_WEST;
    static float ne_x = 0.9f;
    static float ne_y = 0.6f;
    static float se_x = 0.65f;
    static float se_y = 0.5f;
    static float nw_x = 0.9f;
    static float nw_y = 0.6f;
    static float sw_x = 0.65f;
    static float sw_y = 0.52f;

    /* a reference: the base is the add's first operand (an indexed read adds it last) */
    C2DBitmapPoly &ret = m_floatingreticule[m_rettype];

    if (!ret.m_texture)
        return;
    CZSealBody *seal = CHUD::GetHUDOwner();
    CZKit *kit = seal->GetKit();

    if (!seal)
        return;
    float RetSize;

    if (seal->m_zoomstate == ZOOMSTATE_3RDPERSON)
        RetSize = 0.5f * m_retsize;
    else
        RetSize = m_retsize;
    float rpos_offsetx, dpos_offsety, lpos_offsetx, upos_offsety, posx_ex, posy_ex;
    float x = kit->m_retblockedX;
    float y = kit->m_retblockedY;

    if ((kit->m_retblocked || !seal->m_258_0) && seal->m_174 != 9 && seal->m_socom2_e1_4 && !kit->IsUsingBinocs()
        && (!(x <= RetSize) || !(y <= RetSize))) {
        if (kit->IsUsingScope()) {
            float d = x * x + y * y;

            if (!(d <= 40012.8f)) {
                float k = 200.032f / sqrtf(d);

                x = x * k;
                y = y * k;
            }
        }
        m_miss_x.Load(x + m_posx - (int)m_accuracyxtex->m_texture->m_width / 2.0f,
                      y + m_posy - (int)m_accuracyxtex->m_texture->m_height / 2.0f, m_accuracyxtex);
        m_miss_x.On();
        m_miss_x.m_fade_dx = 32.0f;
        if (m_miss_x.GetTrans() < 128.0f) {
            float fade = m_miss_x.m_fade_dx;
            float t = m_miss_x.GetTrans();

            m_miss_x.SetTrans(t + fade);
        }
    } else {
        m_miss_x.m_fade_dx = -32.0f;

        float fade = m_miss_x.m_fade_dx;
        float t = m_miss_x.GetTrans();

        m_miss_x.SetTrans(t + fade);
        if (m_miss_x.GetTrans() <= 0.0f) {
            m_miss_x.Off();
            m_miss_x.SetTrans(0.0f);
        }
    }
    rpos_offsetx = m_posx + RetSize;
    dpos_offsety = m_posy + RetSize;
    lpos_offsetx = m_posx - RetSize;
    upos_offsety = m_posy - RetSize;
    /* held in a local: a literal comes first in the add (add.s f21,f0,f2 for the target's f2,f0) */
    float one = 1.0f;

    posx_ex = m_posx + one;
    posy_ex = m_posy - one;
    if (m_rettype == 9) {
        m_floatingreticule[0].SetPos(rpos_offsetx, posy_ex - m_rettexsizey, rpos_offsetx, posy_ex,
                                     rpos_offsetx + m_rettexsizex, posy_ex, rpos_offsetx + m_rettexsizex,
                                     posy_ex - m_rettexsizey);
        m_floatingreticule[0].FlipImage_Yaxis();
        m_floatingreticule[1].SetPos(posx_ex - m_rettexsizex, upos_offsety - m_rettexsizey, posx_ex - m_rettexsizex,
                                     upos_offsety, posx_ex, upos_offsety, posx_ex, upos_offsety - m_rettexsizey);
        m_floatingreticule[2].SetPos(lpos_offsetx - m_rettexsizex, posy_ex, lpos_offsetx - m_rettexsizex,
                                     posy_ex + m_rettexsizey, lpos_offsetx, posy_ex + m_rettexsizey, lpos_offsetx,
                                     posy_ex);
        m_floatingreticule[2].FlipImage_Xaxis();
        m_floatingreticule[3].SetPos(posx_ex, dpos_offsety, posx_ex, dpos_offsety + m_rettexsizey,
                                     posx_ex + m_rettexsizex, dpos_offsety + m_rettexsizey,
                                     posx_ex + m_rettexsizex, dpos_offsety);
        m_floatingreticule[3].FlipImage_XYaxis();
    } else if (m_rettype == 2 || m_rettype == 3) {
        if (m_rettype == 3) {
            m_rettexsizex *= 0.7f;
            m_rettexsizey *= 0.7f;
        }
        m_floatingreticule[0].SetPos(rpos_offsetx, upos_offsety - m_rettexsizey, rpos_offsetx, upos_offsety,
                                     rpos_offsetx + m_rettexsizex, upos_offsety, rpos_offsetx + m_rettexsizex,
                                     upos_offsety - m_rettexsizey);
        m_floatingreticule[0].FlipImage_Yaxis();
        m_floatingreticule[1].SetPos(lpos_offsetx - m_rettexsizex, upos_offsety - m_rettexsizey,
                                     lpos_offsetx - m_rettexsizex, upos_offsety, lpos_offsetx, upos_offsety,
                                     lpos_offsetx, upos_offsety - m_rettexsizey);
        m_floatingreticule[2].SetPos(lpos_offsetx - m_rettexsizex, dpos_offsety, lpos_offsetx - m_rettexsizex,
                                     dpos_offsety + m_rettexsizey, lpos_offsetx, dpos_offsety + m_rettexsizey,
                                     lpos_offsetx, dpos_offsety);
        m_floatingreticule[2].FlipImage_Xaxis();
        m_floatingreticule[3].SetPos(rpos_offsetx, dpos_offsety, rpos_offsetx, dpos_offsety + m_rettexsizey,
                                     rpos_offsetx + m_rettexsizex, dpos_offsety + m_rettexsizey,
                                     rpos_offsetx + m_rettexsizex, dpos_offsety);
        m_floatingreticule[3].FlipImage_XYaxis();
        if (m_rettype == 3) {
            m_rettexsizex /= 0.7f;
            m_rettexsizey /= 0.7f;
        }
    } else {
        m_floatingreticule[0].SetPos(rpos_offsetx, posy_ex, rpos_offsetx, posy_ex + m_rettexsizey,
                                     rpos_offsetx + m_rettexsizex, posy_ex + m_rettexsizey,
                                     rpos_offsetx + m_rettexsizex, posy_ex);
        m_floatingreticule[0].FlipImage_XYaxis();
        m_floatingreticule[1].SetPos(posx_ex - m_rettexsizex, upos_offsety - m_rettexsizey, posx_ex - m_rettexsizex,
                                     upos_offsety, posx_ex, upos_offsety, posx_ex, upos_offsety - m_rettexsizey);
        m_floatingreticule[2].SetPos(lpos_offsetx - m_rettexsizex, posy_ex, lpos_offsetx - m_rettexsizex,
                                     posy_ex + m_rettexsizey, lpos_offsetx, posy_ex + m_rettexsizey, lpos_offsetx,
                                     posy_ex);
        m_floatingreticule[2].FlipImage_XYaxis();
        m_floatingreticule[2].FlipImage_Xaxis();
        m_floatingreticule[3].SetPos(posx_ex - m_rettexsizex, dpos_offsety, posx_ex - m_rettexsizex,
                                     dpos_offsety + m_rettexsizey, posx_ex, dpos_offsety + m_rettexsizey, posx_ex,
                                     dpos_offsety);
        m_floatingreticule[3].FlipImage_Xaxis();
    }
    PNT2D center_pt;

    center_pt.x = m_rettexsizex2;
    center_pt.x += 15.0f;
    PT_EAST = center_pt.x;
    center_pt.x += rpos_offsetx;
    center_pt.y = posy_ex;
    m_threatdirpoly[0].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(0));
    m_threatdirpoly[0].FlipImage_XYaxis();
    center_pt.x = posx_ex;
    center_pt.y = -m_rettexsizey2;
    center_pt.y -= 15.0f;
    PT_NORTH = center_pt.y;
    center_pt.y += upos_offsety;
    m_threatdirpoly[2].SetCenterPos(center_pt, m_threattexsizex * seal->GetThreatScale(2), m_threattexsizey);
    center_pt.x = -m_rettexsizex2;
    center_pt.x -= 15.0f;
    PT_WEST = center_pt.x;
    center_pt.x += lpos_offsetx;
    center_pt.y = posy_ex;
    m_threatdirpoly[4].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(4));
    m_threatdirpoly[4].FlipImage_XYaxis();
    m_threatdirpoly[4].FlipImage_Xaxis();
    center_pt.x = posx_ex;
    center_pt.y = m_rettexsizey2;
    center_pt.y += 15.0f;
    PT_SOUTH = center_pt.y;
    center_pt.y += dpos_offsety;
    m_threatdirpoly[6].SetCenterPos(center_pt, m_threattexsizex * seal->GetThreatScale(6), m_threattexsizey);
    m_threatdirpoly[6].FlipImage_Xaxis();
    if (m_threatdirpoly[1].GetTrans() > 0.0f) {
        center_pt.x = PT_EAST * ne_x;
        center_pt.x += rpos_offsetx;
        center_pt.y = PT_NORTH * ne_y;
        center_pt.y += upos_offsety;
        m_threatdirpoly[1].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(1));
        m_threatdirpoly[1].FlipImage_XYaxis();
        m_threatdirpoly[1].Rotate_Zaxis(-45.0f);
    }
    if (m_threatdirpoly[7].GetTrans() > 0.0f) {
        center_pt.x = PT_EAST * se_x;
        center_pt.x += rpos_offsetx;
        center_pt.y = PT_SOUTH * se_y;
        center_pt.y += dpos_offsety;
        m_threatdirpoly[7].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(7));
        m_threatdirpoly[7].FlipImage_XYaxis();
        m_threatdirpoly[7].Rotate_Zaxis(45.0f);
    }
    if (m_threatdirpoly[3].GetTrans() > 0.0f) {
        center_pt.x = PT_WEST * nw_x;
        center_pt.x += lpos_offsetx;
        center_pt.y = PT_NORTH * nw_y;
        center_pt.y += upos_offsety;
        m_threatdirpoly[3].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(3));
        m_threatdirpoly[3].FlipImage_XYaxis();
        m_threatdirpoly[3].FlipImage_Xaxis();
        m_threatdirpoly[3].Rotate_Zaxis(45.0f);
    }
    if (m_threatdirpoly[5].GetTrans() > 0.0f) {
        center_pt.x = PT_WEST * sw_x;
        center_pt.x += lpos_offsetx;
        center_pt.y = PT_SOUTH * sw_y;
        center_pt.y += dpos_offsety;
        m_threatdirpoly[5].SetCenterPos(center_pt, m_threattexsizex, m_threattexsizey * seal->GetThreatScale(5));
        m_threatdirpoly[5].FlipImage_XYaxis();
        m_threatdirpoly[5].FlipImage_Xaxis();
        m_threatdirpoly[5].Rotate_Zaxis(-45.0f);
    }
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

/* the reticle's pieces made (SOCOM 1's Init, on SOCOM II's groups, scope lenses, noise and second bar): the three
 * groups under the reticle, the texts; the textures; each poly placed and listed under its group; the first kind */
void BitmapReticule::Init(zdb::CWorld *world, C2DFont *font)
{
    push_back(&m_group0);
    push_back(&m_group1);
    push_back(&m_group2);
    if (font) {
        sprintf(m_lase_str, "NA");
        m_lase_update.Load(1.0f, m_lase_str, font, 10.0f, 52.0f + g_fHudBottomBlockY, true);
        m_lase_update.Off();
        m_group0.push_back(&m_lase_update);
        sprintf(m_detonation_str, "NA");
        m_detonation_update.Load(1.0f, m_detonation_str, font, 466.0f, g_fHudBottomBlockY - 8.0f, true);
        m_detonation_update.Off();
        m_group0.push_back(&m_detonation_update);
        m_enemy_name.Load(0.7f, " ", font, 0.0f, 0.0f, true);
        m_enemy_name.Off();
        m_group0.push_back(&m_enemy_name);
        m_healthbar.SetSize(60.0f, 5.0f);
        m_healthbar.SetPos(-100.0f, -100.0f);
        m_group0.push_back(&m_healthbar);
    }
    m_reticuleTex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_sidearm_01.tif");
    m_reticuleTex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_rifle_01.tif");
    m_reticuleTex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_shotgun_01.tif");
    m_reticuleTex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_rocket_01.tif");
    m_reticuleTex[4] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_grenade_02.tif");
    m_reticuleTex[5] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_scope_02.tif");
    m_reticuleTex[6] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_scope_01.tif");
    m_reticuleTex[7] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_binocs.tif");
    m_reticuleTex[8] = zdb::CAssetMgr::m_assets.GetTexHandle("nvg_part.tif");
    m_reticuleTex[9] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_sidearm_01.tif");
    m_floatingreticuleTex[0] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_sidearm_02.tif");
    m_floatingreticuleTex[1] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_rifle_02.tif");
    m_floatingreticuleTex[2] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_shotgun_02.tif");
    m_floatingreticuleTex[3] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_rocket_02.tif");
    m_floatingreticuleTex[4] = 0;
    m_floatingreticuleTex[5] = 0;
    m_floatingreticuleTex[6] = 0;
    m_floatingreticuleTex[7] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_binocs2.tif");
    m_floatingreticuleTex[8] = 0;
    m_floatingreticuleTex[9] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_laser_designator.tif");
    binocs_tex = zdb::CAssetMgr::m_assets.GetTexHandle("ret_binocs2.tif");
    m_threatdirTex = zdb::CAssetMgr::m_assets.GetTexHandle("ret_threat.tif");
    m_grenadebartex_full = zdb::CAssetMgr::m_assets.GetTexHandle("ret_grenade_01.tif");
    m_accuracyxtex = zdb::CAssetMgr::m_assets.GetTexHandle("ret_accuracy.tif");
    m_miss_x.Load(50.0f, 50.0f, m_accuracyxtex);
    m_miss_x.Off();
    m_miss_x.SetTrans(0.0f);
    m_scopepoly[0].Load(0.0f, -96.0f, 0.0f, 224.0f, 320.0f, 224.0f, 320.0f, -96.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
    m_scopepoly[0].SetTrans(128.0f);
    m_scopepoly[1].Load(320.0f, -96.0f, 320.0f, 224.0f, 640.0f, 224.0f, 640.0f, -96.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
    m_scopepoly[1].SetTrans(128.0f);
    m_scopepoly[2].Load(0.0f, 224.0f, 0.0f, 544.0f, 320.0f, 544.0f, 320.0f, 224.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
    m_scopepoly[2].SetTrans(128.0f);
    m_scopepoly[3].SetTrans(128.0f);
    m_scopepoly[3].Load(320.0f, 224.0f, 320.0f, 544.0f, 640.0f, 544.0f, 640.0f, 224.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
    m_scopepoly[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
    m_scopepoly[1].SetUV(0.99f, 0.01f, 0.99f, 0.99f, 0.01f, 0.99f, 0.01f, 0.01f);
    m_scopepoly[2].SetUV(0.01f, 0.99f, 0.01f, 0.01f, 0.99f, 0.01f, 0.99f, 0.99f);
    m_scopepoly[3].SetUV(0.99f, 0.99f, 0.99f, 0.01f, 0.01f, 0.01f, 0.01f, 0.99f);
    m_socom2_11f0[0].Load(0.0f, -96.0f, 0.0f, 224.0f, 320.0f, 224.0f, 320.0f, -96.0f, m_reticuleTex[6] ? m_reticuleTex[6]->m_texture : 0);
    m_socom2_11f0[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
    m_socom2_11f0[0].SetTrans(128.0f);
    m_socom2_11f0[1].Load(320.0f, -96.0f, 320.0f, 224.0f, 640.0f, 224.0f, 640.0f, -96.0f, m_reticuleTex[6] ? m_reticuleTex[6]->m_texture : 0);
    m_socom2_11f0[1].SetTrans(128.0f);
    m_socom2_11f0[2].Load(0.0f, 224.0f, 0.0f, 544.0f, 320.0f, 544.0f, 320.0f, 224.0f, m_reticuleTex[6] ? m_reticuleTex[6]->m_texture : 0);
    m_socom2_11f0[2].SetTrans(128.0f);
    m_socom2_11f0[3].SetTrans(128.0f);
    m_socom2_11f0[3].Load(320.0f, 224.0f, 320.0f, 544.0f, 640.0f, 544.0f, 640.0f, 224.0f, m_reticuleTex[6] ? m_reticuleTex[6]->m_texture : 0);
    m_socom2_11f0[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
    m_socom2_11f0[1].SetUV(0.99f, 0.01f, 0.99f, 0.99f, 0.01f, 0.99f, 0.01f, 0.01f);
    m_socom2_11f0[2].SetUV(0.01f, 0.99f, 0.01f, 0.01f, 0.99f, 0.01f, 0.99f, 0.99f);
    m_socom2_11f0[3].SetUV(0.99f, 0.99f, 0.99f, 0.01f, 0.01f, 0.01f, 0.01f, 0.99f);
    m_group0.push_back(&m_bitmap);
    m_group0.push_back(&m_grenadebitmap);
    m_group0.push_back(&m_grenadebitmap2);
    m_group0.push_back(&m_grenadeactive[0]);
    m_group0.push_back(&m_grenadeactive[1]);
    m_group0.push_back(&m_grenadeback[0]);
    m_group0.push_back(&m_grenadeback[1]);
    m_group0.push_back(&m_miss_x);
    int i;

    for (i = 0; i < 4; i++) {
        m_group1.push_back(&m_scopepoly[i]);
        m_group1.push_back(&m_socom2_11f0[i]);
        m_floatingreticule[i].SetTrans(100.0f);
        m_floatingreticule[i].SetUV(0.03f, 0.03f, 1.0f, 1.0f);
        m_group2.push_back(&m_floatingreticule[i]);
        m_floatingreticule[i].Off();
    }
    for (int j = 0; j < 8; j++) {
        m_threatdirpoly[j].SetTrans(0.0f);
        m_threatdirpoly[j].SetUV(0.03f, 0.03f, 1.0f, 1.0f);
        m_group2.push_back(&m_threatdirpoly[j]);
        m_threatdirpoly[j].On();
        if (m_threatdirTex) {
            zdb::CTexture *pic = m_threatdirTex->m_texture;

            if (pic)
                m_threatdirpoly[j].Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, pic);
        }
    }
    if (m_threatdirTex->m_texture) {
        m_threattexsizex = (int)m_threatdirTex->m_texture->m_width;
        m_threattexsizey = (int)m_threatdirTex->m_texture->m_height;
    }
    for (unsigned int k = 0; k < 4; k++)
        m_group1.push_back(&m_nvglens[k]);
    for (i = 0; i < 4; i++)
        m_nvglens[i].On();
    m_nvglens[0].Load(0.0f, 0.0f, 0.0f, 224.0f, 320.0f, 224.0f, 320.0f, 0.0f, m_reticuleTex[8] ? m_reticuleTex[8]->m_texture : 0);
    m_nvglens[1].Load(320.0f, 0.0f, 320.0f, 224.0f, 640.0f, 224.0f, 640.0f, 0.0f, m_reticuleTex[8] ? m_reticuleTex[8]->m_texture : 0);
    m_nvglens[2].Load(0.0f, 224.0f, 0.0f, 448.0f, 320.0f, 448.0f, 320.0f, 224.0f, m_reticuleTex[8] ? m_reticuleTex[8]->m_texture : 0);
    m_nvglens[3].Load(320.0f, 224.0f, 320.0f, 448.0f, 640.0f, 448.0f, 640.0f, 224.0f, m_reticuleTex[8] ? m_reticuleTex[8]->m_texture : 0);
    m_nvglens[0].SetUV(0.01f, 0.01f, 0.01f, 0.995f, 0.995f, 0.995f, 0.995f, 0.01f);
    m_nvglens[1].SetUV(0.995f, 0.01f, 0.995f, 0.995f, 0.01f, 0.995f, 0.01f, 0.01f);
    m_nvglens[2].SetUV(0.01f, 0.995f, 0.01f, 0.01f, 0.995f, 0.01f, 0.995f, 0.995f);
    m_nvglens[3].SetUV(0.995f, 0.995f, 0.995f, 0.01f, 0.01f, 0.01f, 0.01f, 0.995f);
    zdb::CTexHandle *noise = zdb::CAssetMgr::m_assets.GetTexHandle("noise50.tif");

    m_noisetex = noise ? noise->m_texture : 0;
    if (m_noisetex) {
        for (unsigned int k = 0; k < 4; k++)
            m_group1.push_back(&m_socom2_3940[k]);
        for (unsigned int l = 0; l < 4; l++) {
            m_socom2_3940[l].On();
            m_socom2_3940[l].SetTrans(64.0f);
        }
        m_socom2_3940[0].Load(5.0f, 75.0f, 5.0f, 224.0f, 320.0f, 224.0f, 320.0f, 75.0f, m_noisetex);
        m_socom2_3940[1].Load(320.0f, 75.0f, 320.0f, 224.0f, 635.0f, 224.0f, 635.0f, 75.0f, m_noisetex);
        m_socom2_3940[2].Load(5.0f, 224.0f, 5.0f, 373.0f, 320.0f, 373.0f, 320.0f, 224.0f, m_noisetex);
        m_socom2_3940[3].Load(320.0f, 224.0f, 320.0f, 373.0f, 635.0f, 373.0f, 635.0f, 224.0f, m_noisetex);
        m_socom2_3940[0].SetUV(0.01f, 0.01f, 0.01f, 0.995f, 0.995f, 0.995f, 0.995f, 0.01f);
        m_socom2_3940[1].SetUV(0.995f, 0.01f, 0.995f, 0.995f, 0.01f, 0.995f, 0.01f, 0.01f);
        m_socom2_3940[2].SetUV(0.01f, 0.995f, 0.01f, 0.01f, 0.995f, 0.01f, 0.995f, 0.995f);
        m_socom2_3940[3].SetUV(0.995f, 0.995f, 0.995f, 0.01f, 0.01f, 0.01f, 0.01f, 0.995f);
        func_00354A90(m_noisetex, 0x42, 0x49, 0x7F);
        if (m_noisetex) {
            unsigned int count = m_noisetex->m_width * m_noisetex->m_height;
            unsigned char *pixels = m_noisetex->m_pixels;

            for (unsigned int n = 0; n < count; n++)
                pixels[n] = (int)(255.0f * (4.656613e-10f * (float)rand()));
            FlushCache(0);
        }
    }
    ChangeReticule(0);
    m_posx = 320.0f;
    m_posy = 224.0f;
    m_timer_BKGRD.SetHasTexture(false);
    m_timer_BKGRD.SetColor(0.0f, 0.0f, 0.0f);
    m_timer_BKGRD.SetPos(0x214, 0xB9, 0x24A, 0xC9);
    m_timer_BKGRD.SetTrans(0.0f);
    m_group2.push_back(&m_timer_BKGRD);
    m_timer_IMAGE.SetHasTexture(false);
    m_timer_IMAGE.SetColor(CHUD::m_hud_textcolor.x, CHUD::m_hud_textcolor.y, CHUD::m_hud_textcolor.z);
    m_timer_IMAGE.SetPos(0x216, 0xBB, 0x216, 0xC7);
    m_timer_IMAGE.SetTrans(0.0f);
    m_group2.push_back(&m_timer_IMAGE);
    m_socom2_4420.SetHasTexture(false);
    m_socom2_4420.SetColor(0.0f, 0.0f, 0.0f);
    m_socom2_4420.SetPos(0x214, 0xCE, 0x24A, 0xDE);
    m_socom2_4420.SetTrans(0.0f);
    m_group2.push_back(&m_socom2_4420);
    m_socom2_43a0.SetHasTexture(false);
    m_socom2_43a0.SetColor(CHUD::m_hud_textcolor.x, CHUD::m_hud_textcolor.y, CHUD::m_hud_textcolor.z);
    m_socom2_43a0.SetPos(0x216, 0xD0, 0x216, 0xDC);
    m_socom2_43a0.SetTrans(0.0f);
    m_group2.push_back(&m_socom2_43a0);
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
 * The first corner copy a declaration (lq / sq), the later ones assignments, member by member.
 * Round 61 (written in round 59, 0x36C of 0x364): the kit is a pointer an inline returns, taken before
 * the lasing test -- the test's load folds into the seal's offset in the same block (`lbu 0x5E2(s0)`),
 * the address goes into the test's delay slot and the lased point is read at offsets from it in the
 * next (`0xC(a0)`), where a plain pointer or reference local to the kit, before or after the test, or
 * the inline returning the kit called after it, folds every offset (0x5EC(s0)); and the screen offset
 * is added in a statement of its own (`x += 320`: the target's add.s keeps the product's register, x
 * f4 and y f2), one expression puts the constant first (two FPRs and two alignment nops off) */

/* the lifted night-vision mode's global flag (OVL1 bss 0x4B4A70: the world's renderer reads it; no
 * row yet) */
extern unsigned char data_004B4A70;
/* whether the bomb game's defuse clock runs here (fts_mission.cpp's, 0x2AA890) */
extern "C" bool func_002AA890();
extern "C" int rand();
extern "C" int sprintf(char *buf, const char *fmt, ...);
extern "C" void FlushCache(int op);

/* a diagonal threat arrow's strength shared with the side it leans to: the angle wrapped below 90,
 * the side's arrow gets its remainder, the diagonal its part (ours: ThreatDirTick's eight arms) */
#define THREAT_SPLIT(side, diag, a)                                                  \
    {                                                                                \
        float angle = fabsf(a);                                                      \
                                                                                     \
        while (angle >= 90.0f)                                                       \
            angle -= 90.0f;                                                          \
        m_threatdirpoly[side].SetTrans(50.0f * ((90.0f - angle) * ONE_OVER_90));     \
        const float k50 = 50.0f;                                                     \
                                                                                     \
        m_threatdirpoly[diag].SetTrans(k50 * (angle * ONE_OVER_90));                 \
    }

/* the threat arrows (SOCOM 1's ThreatDirTick, 0x3CF360 there): the seal's four hit directions kept;
 * where two neighbours were hit, the diagonal between gets half their strength and the two arrows the
 * angle's share (SOCOM 1 set them full); every arrow faded by 120 a second; the flags cleared. The
 * names SOCOM 1's DWARF's (SOCOM 1's DIAGONAL_SCALAR was made under a guard, SOCOM II's is data).
 * The diagonal's 50 a const local (round 66, open since round 64 at 32 off): a literal is numbered last and a plain
 * local folds back; a const one keeps its number ahead of the product, the target's f1 and f0. The first test reads
 * the seal's two flags through locals (a0, v1 kept
 * across the statics' stores), the later ones the statics */
void BitmapReticule::ThreatDirTick(CZSealBody *pSeal)
{
    static float DIAGONAL_SCALAR = 0.5f;
    static float ONE_OVER_90 = 1.0f / 90.0f;
    static unsigned char dirE;
    static unsigned char dirN;
    static unsigned char dirW;
    static unsigned char dirS;

    if (!pSeal)
        return;
    unsigned char e = pSeal->m_threatdir[0];

    dirE = e;

    unsigned char n = pSeal->m_threatdir[1];

    dirN = n;
    dirW = pSeal->m_threatdir[2];
    dirS = pSeal->m_threatdir[3];
    if (e && n) {
        pSeal->m_threatscale[1] = DIAGONAL_SCALAR * (pSeal->m_threatscale[0] + pSeal->m_threatscale[2]);
        if (pSeal->m_threatangle[0] > pSeal->m_threatangle[2]) {
            THREAT_SPLIT(0, 1, pSeal->m_threatangle[0]);
        } else {
            THREAT_SPLIT(2, 1, pSeal->m_threatangle[2]);
        }
    }
    if (dirW && dirN) {
        pSeal->m_threatscale[3] = DIAGONAL_SCALAR * (pSeal->m_threatscale[4] + pSeal->m_threatscale[2]);
        if (pSeal->m_threatangle[4] > pSeal->m_threatangle[2]) {
            THREAT_SPLIT(4, 3, pSeal->m_threatangle[4]);
        } else {
            THREAT_SPLIT(2, 3, pSeal->m_threatangle[2]);
        }
    }
    if (dirE && dirS) {
        pSeal->m_threatscale[7] = DIAGONAL_SCALAR * (pSeal->m_threatscale[0] + pSeal->m_threatscale[6]);
        if (pSeal->m_threatangle[0] > pSeal->m_threatangle[6]) {
            THREAT_SPLIT(0, 7, pSeal->m_threatangle[0]);
        } else {
            THREAT_SPLIT(6, 7, pSeal->m_threatangle[6]);
        }
    }
    if (dirW && dirS) {
        pSeal->m_threatscale[5] = DIAGONAL_SCALAR * (pSeal->m_threatscale[4] + pSeal->m_threatscale[6]);
        if (pSeal->m_threatangle[4] > pSeal->m_threatangle[6]) {
            THREAT_SPLIT(4, 5, pSeal->m_threatangle[4]);
        } else {
            THREAT_SPLIT(6, 5, pSeal->m_threatangle[6]);
        }
    }
    for (unsigned int ii = 0; ii < 8; ii++) {
        float trans_val = m_threatdirpoly[ii].GetTrans();

        if (trans_val != 0.0f) {
            trans_val -= 120.0f * zVid.frameTime;
            if (trans_val < 0.0f)
                trans_val = 0.0f;
            m_threatdirpoly[ii].SetTrans(trans_val);
        }
    }
    pSeal->m_threatdir[0] = 0;
    pSeal->m_threatdir[1] = 0;
    pSeal->m_threatdir[2] = 0;
    pSeal->m_threatdir[3] = 0;
}
#undef THREAT_SPLIT

/* the reticle's tick (SOCOM 1's HudTick, 0x3CF070 there): the health bar, the lased point and the
 * threat arrows; the weapons' timers taken up and shown; the defuse clock counted down under the three
 * menu states (SOCOM 1's DefuseTick, its clock's text gone); the night vision's flag kept, and with it
 * the lenses' alpha and new noise in the noise texture; a launcher's first person forced; the reticle
 * changed when its kind does (SOCOM II: the HUD's own override first); it placed by the kit's position
 * and spread (SOCOM 1's), but for the fixed kinds; the miss mark faded under three kinds; the range
 * text; the colour */
void BitmapReticule::HudTick(float dT)
{
    CZSealBody *seal = CHUD::GetHUDOwner();

    if (!seal)
        return;
    m_healthbar.Tick(dT);
    DrawLaseTarget(seal);
    ThreatDirTick(seal);
    if (CZWeapon::TimerCleared())
        m_pCurval = 0;
    if (CZWeapon::TimerRegistered()) {
        CZWeapon::RetrieveTimerRegistration(&m_pCurval, &m_maxval, (bool *)&m_bIncrTimer);
        if (m_pCurval && m_maxval != 0.0f) {
            m_maxval_frac = 1.0f / m_maxval;
        } else {
            m_maxval = -1.0f;
            m_maxval_frac = -1.0f;
        }
    }
    if (CZWeapon::Timer2Registered())
        CZWeapon::RetrieveTimer2Registration(&m_pCurval2, &m_maxval2, (bool *)&m_bIncrTimer2);
    TimerProgressTick(dT);
    ReloadTimerTick(dT, seal);
    if (func_002AA890() && !(g_fDefuseTime < 0.0f)) {
        if (CHUD::GetHUDOwner()->m_ctrl->m_menustate == 0 || CHUD::GetHUDOwner()->m_ctrl->m_menustate == 1
            || CHUD::GetHUDOwner()->m_ctrl->m_menustate == 5) {
            g_fDefuseTime -= dT;
            if (g_fDefuseTime <= 0.0f) {
                CZWeapon::ClearTimer(0);
                g_fDefuseTime = -1.0f;
                CValve::Create(c_szDefusingValve, 0, VALVE_TEMP);
            }
        }
    }
    m_nvgenabled = seal->m_kit.NVG_Enabled();
    data_004B4A70 = m_nvgenabled;
    if (m_nvgenabled) {
        unsigned int i;

        for (i = 0; i < 4; i++)
            m_socom2_3940[i].SetTrans(m_socom2_4104);
        zdb::CTexture *noise = m_noisetex;

        if (noise) {
            unsigned int count = noise->m_width * noise->m_height;
            unsigned char *pixels = noise->m_pixels;

            for (i = 0; i < count; i++)
                pixels[i] = (int)(255.0f * (4.6566128730773926e-10f * (float)rand()));
            FlushCache(0);
        }
    }
    if (seal->GetKit() && seal->GetKit()->CurWeaponIsLauncherMode() && (unsigned int)seal->m_zoomstate > 1)
        seal->SetCurZoomState(ZOOMSTATE_1STPERSON);
    CHUD *hud;

    if (!seal->m_socom2_e1_4) {
        ChangeReticule(-1);
    } else if ((hud = m_parent)->m_usehudret) {
        if (hud->m_hudretchanged) {
            hud->m_hudretchanged = false;
            ChangeReticule(hud->m_hudrettype);
        } else if (seal->m_kit.CurReticuleTypeChanged()) {
            ChangeReticule(seal->m_kit.GetCurItemReticule());
        }
    } else if (seal->m_kit.CurReticuleTypeChanged()) {
        ChangeReticule(seal->m_kit.GetCurItemReticule());
    }
    if (m_rettype != 4) {
        m_retsize = seal->m_kit.m_retsize;

        float x = seal->m_kit.m_retposx + seal->m_kit.m_retoffsetx;
        float y = seal->m_kit.m_retposy + seal->m_kit.m_retoffsety;

        if (m_reticuleTex[m_rettype]) {
            float py;
            float px = 320.0f + x;

            py = 224.0f + y;
            m_posx = px;
            m_posy = py;
            if (m_rettype != 4 && m_rettype != 5 && m_rettype != -1 && m_rettype != 7 && m_rettype != 8
                && m_rettype != 9) {
                zdb::CTexture *tex = m_reticuleTex[m_rettype]->m_texture;

                m_bitmap.SetPos((int)(px - 0.5f * (float)(int)tex->m_width), (int)(py - 0.5f * (float)(int)tex->m_height));
            }
        }
        UpdateAccuracy();
    } else {
        seal->m_kit.SetRetBlocked(false);
    }
    if (m_rettype == 4 || m_rettype == 10 || m_rettype == 3) {
        m_miss_x.m_fade_dx = -32.0f;

        float fade = m_miss_x.m_fade_dx;

        float t = m_miss_x.GetTrans();

        m_miss_x.SetTrans(t + fade);
        if (m_miss_x.GetTrans() <= 0.0f) {
            m_miss_x.Off();
            m_miss_x.SetTrans(0.0f);
        }
    }

    char text[32];
    CReticuleDi *di = seal->GetReticuleDi();

    if (di && di->m_hit) {
        CPnt3D delta = *di - seal->m_posW;

        sprintf(text, "RANGE(m): %.0f", delta.Length() / 10.0f);
    } else {
        sprintf(text, "RANGE(m): ----");
    }
    theHUD->SetRangeText(text);
    UpdateColor();
}

/* (the seal's kit through an inline returning its address -- the name ours; zseal.h's accessor
 * is likely one) */
static inline CZKit *KitOf(CZSealBody *s)
{
    return &s->m_kit;
}

void BitmapReticule::DrawLaseTarget(CZSealBody *seal)
{
    if (!theHUD->IsOn())
        return;

    CZKit *kit = KitOf(seal);

    if (!kit->m_lasing)
        return;

    CPnt3D pos = kit->GetLasePoint();
    CPnt4D in(pos.x, pos.y, pos.z, 1.0f);
    CPnt4D out;
    CMatrix m = *(CMatrix *)zdb::CWorld::m_world->m_camera->m_vu->mtxWorldToScreen;

    m.Transform(&in, &out, 1);

    float iw = 1.0f / out.w;
    float x = out.x * iw - 2048.0f;
    float y = out.y * iw - 2048.0f;

    x += 320.0f;
    y += 224.0f;
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

/* the range text's place for the scope, the binoculars and the night vision, the scope's range box and the bars'
 * and binoculars' alphas (OVL1 .data; the names ours) */
float s_scopeRangeX = 415.0f;
float s_scopeRangeY = 215.0f;
float s_binocRangeX = 515.0f;
float s_binocRangeY = 40.0f;
float s_nvgRangeX = 515.0f;
float s_nvgRangeY = 70.0f;
float s_scopeRangeSize = 32.0f;
float s_barBackTrans = 36.0f;
float s_barTrans = 64.0f;
float s_binocTrans = 128.0f;

/* the reticle's kind changed: nothing for a kind with no texture; the range text on for the scopes and the night
 * vision; the floating pieces textured and placed; each kind's own pieces shown and the others hidden (SOCOM 1's;
 * kinds -1 none, 0-3 and 9 a bitmap or the floating reticle's, 4 the grenade's bar, 5 the scope, 7 the binoculars) */
void BitmapReticule::ChangeReticule(int type)
{
    signed char t = type;

    if (t != -1 && !m_reticuleTex[t])
        return;
    m_rettype = type;
    if (t == 5 || t == 7 || t == 8 || m_nvgenabled)
        theHUD->ShowRangeText();
    else
        theHUD->HideRangeText();
    if (t != -1) {
        if (m_floatingreticuleTex[m_rettype]) {
            for (int i = 0; i < 4; i++)
                m_floatingreticule[i].Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                           m_floatingreticuleTex[m_rettype]->m_texture);
        }
    }
    if (m_rettype != -1) {
        zdb::CTexHandle *tex = 0;

        if (m_floatingreticuleTex[m_rettype])
            tex = m_floatingreticuleTex[m_rettype];
        else if (m_reticuleTex[m_rettype])
            tex = m_reticuleTex[m_rettype];
        if (tex) {
            m_rettexsizex = (int)tex->m_texture->m_width;
            m_rettexsizey = (int)tex->m_texture->m_height;
            m_rettexsizex2 = m_rettexsizex;
            m_rettexsizey2 = m_rettexsizey;
            if (m_rettype == 4)
                m_rettexsizex2 = m_rettexsizex2 * 0.5f;
            else if (m_rettype == 5) {
                m_rettexsizex2 = m_rettexsizey2 = s_scopeRangeSize;
            }
        }
        UpdateAccuracy();
        if (m_rettype == 4) {
            for (int i = 0; i < 4; i++)
                m_floatingreticule[i].Off();
        } else {
            for (int i = 0; i < 4; i++)
                m_floatingreticule[i].On();
        }
    }
    if (m_rettype == -1) {
        for (int i = 0; i < 4; i++) {
            m_floatingreticule[i].Off();
            m_threatdirpoly[i].Off();
            m_scopepoly[i].Off();
            m_socom2_11f0[i].Off();
        }
    }
    if (m_nvgenabled) {
        for (unsigned int k = 0; k < 4; k++) {
            m_nvglens[k].On();
            m_socom2_3940[k].On();
        }
    } else {
        for (unsigned int k = 0; k < 4; k++) {
            m_nvglens[k].Off();
            m_socom2_3940[k].Off();
        }
    }
    int i;

    if (t == -1) {
        m_bitmap.Off();
        for (i = 0; i < 4; i++) {
            m_scopepoly[i].Off();
            m_threatdirpoly[i].Off();
            m_socom2_11f0[i].Off();
        }
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadeactive[0].Off();
        m_grenadeactive[1].Off();
        m_grenadeback[0].Off();
        m_grenadeback[1].Off();
        m_enemy_name.Off();
        m_healthbar.Off();
    } else if ((t >= 0 && t < 4) || t == 9) {
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadeactive[0].Off();
        m_grenadeactive[1].Off();
        m_grenadeback[0].Off();
        m_grenadeback[1].Off();
        m_enemy_name.On();
        m_healthbar.On();
        m_bitmap.On();
        for (i = 0; i < 4; i++) {
            m_scopepoly[i].Off();
            m_socom2_11f0[i].Off();
            m_threatdirpoly[i].On();
            m_floatingreticule[i].On();
        }
        if (t != 3) {
            zdb::CTexHandle *tex = m_reticuleTex[t];
            zdb::CTexture *pic = tex->m_texture;
            float h = (int)pic->m_height / 2.0f;
            float w = (int)pic->m_width / 2.0f;

            m_bitmap.Load(m_posx - w, m_posy - h, m_posx + w, m_posy + h, tex);
            m_bitmap.SetUV(0.0f, 0.0f, 1.0f, 1.0f);
            m_bitmap.SetTrans(128.0f);

            CZSealBody *seal = CHUD::GetHUDOwner();

            if (t == 0 && seal->m_zoomstate == 2) {
                for (int n = 0; n < 4; n++)
                    m_socom2_11f0[n].On();
            }
        } else {
            zdb::CTexture *pic = m_reticuleTex[t]->m_texture;
            float w = 0.7f * (int)pic->m_width;
            float h = 0.7f * (int)pic->m_height;
            float left = 320.0f - w;
            float right = 320.0f + w;
            float top = 224.0f - h;
            float bottom = 224.0f + h;

            m_bitmap.Off();
            m_scopepoly[0].On();
            m_scopepoly[0].SetTrans(128.0f);
            m_scopepoly[0].Load(left, top, 320.0f, top, 320.0f, 224.0f, left, 224.0f, m_reticuleTex[t] ? m_reticuleTex[t]->m_texture : 0);
            m_scopepoly[1].On();
            m_scopepoly[1].SetTrans(128.0f);
            m_scopepoly[1].Load(right, top, 320.0f, top, 320.0f, 224.0f, right, 224.0f, m_reticuleTex[t] ? m_reticuleTex[t]->m_texture : 0);
            m_scopepoly[2].On();
            m_scopepoly[2].SetTrans(128.0f);
            m_scopepoly[2].Load(left, bottom, 320.0f, bottom, 320.0f, 224.0f, left, 224.0f, m_reticuleTex[t] ? m_reticuleTex[t]->m_texture : 0);
            m_scopepoly[3].On();
            m_scopepoly[3].SetTrans(128.0f);
            m_scopepoly[3].Load(right, bottom, 320.0f, bottom, 320.0f, 224.0f, right, 224.0f, m_reticuleTex[t] ? m_reticuleTex[t]->m_texture : 0);
            m_scopepoly[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
            m_scopepoly[1].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
            m_scopepoly[2].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
            m_scopepoly[3].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
        }
    } else if (t == 4) {
        static CPnt3D s_barColor(128.0f, 70.0f, 32.0f);

        SetGBarVal(0.0f);
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadeactive[0].On();
        m_grenadeactive[1].On();
        m_grenadeback[0].On();
        m_grenadeback[1].On();
        m_grenadeactive[0].SetColor(s_barColor.x, s_barColor.y, s_barColor.z);
        m_grenadeactive[1].SetColor(s_barColor.x, s_barColor.y, s_barColor.z);
        m_grenadeback[0].SetColor(200.0f, 200.0f, 0.0f);
        m_grenadeback[1].SetColor(200.0f, 200.0f, 0.0f);
        m_grenadeactive[0].SetTrans(s_barTrans);
        m_grenadeactive[1].SetTrans(s_barTrans);
        m_grenadeback[0].SetTrans(s_barBackTrans);
        m_grenadeback[1].SetTrans(s_barBackTrans);
        m_enemy_name.Off();
        m_healthbar.Off();
        m_bitmap.Off();
        for (i = 0; i < 4; i++) {
            m_scopepoly[i].Off();
            m_socom2_11f0[i].Off();
            m_threatdirpoly[i].On();
        }
        zdb::CTexHandle *tex = m_reticuleTex[t];
        zdb::CTexture *pic = tex->m_texture;
        float h = (int)pic->m_height / 2.0f;
        float w = (int)pic->m_width / 2.0f;

        m_bitmap.Load(m_posx - w, m_posy - h, m_posx + w, m_posy + h, tex);
        m_bitmap.SetUV(0.0f, 0.0f, 1.0f, 1.0f);
        m_bitmap.SetTrans(128.0f);
    } else if (t == 5) {
        bool laser = false;
        CZSealBody *seal = CHUD::GetHUDOwner();
        CZKit *kit = seal->GetKit();

        if (seal)
            laser = kit != 0 && (kit->GetCurItemEnum() & 0xff) == 0x3f;
        m_enemy_name.Off();
        m_healthbar.Off();
        m_bitmap.Off();
        for (i = 0; i < 4; i++) {
            m_threatdirpoly[i].On();
            m_floatingreticule[i].Off();
            m_socom2_11f0[i].On();
            if (laser)
                m_scopepoly[i].Off();
            else {
                m_scopepoly[i].On();
                m_scopepoly[i].SetTrans(128.0f);
            }
        }
        SetGBarVal(0.0f);
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadeactive[0].Off();
        m_grenadeactive[1].Off();
        m_grenadeback[0].Off();
        m_grenadeback[1].Off();
        if (!laser) {
            m_scopepoly[0].Load(0.0f, -96.0f, 0.0f, 224.0f, 320.0f, 224.0f, 320.0f, -96.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
            m_scopepoly[1].Load(320.0f, -96.0f, 320.0f, 224.0f, 640.0f, 224.0f, 640.0f, -96.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
            m_scopepoly[2].Load(0.0f, 224.0f, 0.0f, 544.0f, 320.0f, 544.0f, 320.0f, 224.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
            m_scopepoly[3].Load(320.0f, 224.0f, 320.0f, 544.0f, 640.0f, 544.0f, 640.0f, 224.0f, m_reticuleTex[5] ? m_reticuleTex[5]->m_texture : 0);
        }
        m_scopepoly[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.99f, 0.99f, 0.99f, 0.01f);
        m_scopepoly[1].SetUV(0.99f, 0.01f, 0.99f, 0.99f, 0.01f, 0.99f, 0.01f, 0.01f);
        m_scopepoly[2].SetUV(0.01f, 0.99f, 0.01f, 0.01f, 0.99f, 0.01f, 0.99f, 0.99f);
        m_scopepoly[3].SetUV(0.99f, 0.99f, 0.99f, 0.01f, 0.01f, 0.01f, 0.01f, 0.99f);
        theHUD->SetRangeTextPos(s_scopeRangeX, s_scopeRangeY);
    } else if (t == 7) {
        m_enemy_name.Off();
        m_healthbar.Off();
        m_bitmap.Off();
        for (i = 0; i < 4; i++) {
            m_floatingreticule[i].Off();
            m_scopepoly[i].On();
            m_scopepoly[i].SetTrans(128.0f);
            m_threatdirpoly[i].On();
        }
        SetGBarVal(0.0f);
        m_grenadebitmap.Off();
        m_grenadebitmap2.Off();
        m_grenadeactive[0].Off();
        m_grenadeactive[1].Off();
        m_grenadeback[0].Off();
        m_grenadeback[1].Off();
        m_scopepoly[0].Load(0.0f, 0.0f, 0.0f, 448.0f, 320.0f, 448.0f, 320.0f, 0.0f, m_reticuleTex[7] ? m_reticuleTex[7]->m_texture : 0);
        m_scopepoly[1].Load(319.0f, 0.0f, 320.0f, 448.0f, 640.0f, 448.0f, 640.0f, 0.0f, m_reticuleTex[7] ? m_reticuleTex[7]->m_texture : 0);
        m_scopepoly[2].Load(0.0f, 0.0f, 0.0f, 448.0f, 320.0f, 448.0f, 320.0f, 0.0f, binocs_tex ? binocs_tex->m_texture : 0);
        m_scopepoly[3].Load(319.0f, 0.0f, 320.0f, 448.0f, 640.0f, 448.0f, 640.0f, 0.0f, binocs_tex ? binocs_tex->m_texture : 0);
        m_scopepoly[0].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.97f, 0.99f, 0.97f, 0.01f);
        m_scopepoly[1].SetUV(0.97f, 0.01f, 0.97f, 0.99f, 0.01f, 0.99f, 0.01f, 0.01f);
        m_scopepoly[2].SetUV(0.01f, 0.01f, 0.01f, 0.99f, 0.97f, 0.99f, 0.97f, 0.01f);
        m_scopepoly[3].SetUV(0.97f, 0.01f, 0.97f, 0.99f, 0.01f, 0.99f, 0.01f, 0.01f);
        m_scopepoly[2].SetTrans(s_binocTrans);
        m_scopepoly[3].SetTrans(s_binocTrans);
        theHUD->SetRangeTextPos(s_binocRangeX, s_binocRangeY);
    }
    if (m_nvgenabled)
        theHUD->SetRangeTextPos(s_nvgRangeX, s_nvgRangeY);
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
 * calls and UpdateColor instantiates
 *
 * the top item, none when empty (SOCOM 1's) */
template <class T>
T *RTStack<T>::Top()
{
    if (m_top < 0)
        return 0;
    return m_stack[m_top];
}

