/* hud_mapcompass.cpp -- the HUD's compass (OVL1 0x2113D0-0x2135FF): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_mapcompass.cpp), its small functions in SOCOM 1's order after Tick. SOCOM
 * II's Draw is empty, SetPos scales the rectangle by ztwod's screen scale, and On shows the
 * compass only for the local seal while it lives. All byte-exact since round 65. Defined in reverse of
 * their image order (-inline deferred). */

#include <vector>

#include "hud_mapcompass.h"
#include "../../gamez/znode/znode.h"
#include "../../gamez/zsys/zsys.h"

class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

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

/* a cell of the AI maps (zentity.h's; its copy field by field) */
class CAiMapLoc {
public:
    CAiMapLoc(const CAiMapLoc &loc) : m_mapid(loc.m_mapid), x(loc.x), y(loc.y) {}

    unsigned int m_mapid : 6;
    unsigned int x : 13;
    unsigned int y : 13;
};

/* a node as the compass reads it: its matrix's forward and position rows (zdb::CNode's) */
struct CCompassNode {
    unsigned char _0[0x20];
    CPnt3D m_fwd;   // 0x20
    unsigned char _2c[0x30 - 0x2c];
    CPnt3D m_pos;   // 0x30
};

/* only what On and Tick read (names ours) */
class CZSealBody {
public:
    unsigned char _0[0x28];
    CCompassNode *m_node;   // 0x28: its node
    unsigned char _2c[0xe1 - 0x2c];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4: tested before the compass shows
    unsigned char _e1_5 : 3;
    unsigned char _e2[0xe8 - 0xe2];
    CAiMapLoc m_maploc;   // 0xe8 (SOCOM 1's at 0xe4)
    unsigned char _ec[Z_SEALBODY_OFS(0x200) - 0xec];
    signed char m_socom2_200;   // 0x200: under 2 while the seal lives
    bool IsDead() { return m_socom2_200 >= 2; }
};

/* a compass anchor as the colour reads it (zentity.h's CCompassAnchor: its type bits, and its
 * point's colour if it has one) */
class CCompassAnchor {
public:
    bool GetPoint(CPnt3D *p);

    unsigned int m_enabled : 1;   // 0x0
    unsigned int m_arrow_enabled : 1;
    unsigned int m_name_enabled : 1;
    unsigned int m_echo : 1;
    unsigned int m_type : 16;
    unsigned int m_locked : 1;
    unsigned int m_unk21 : 10;
    CPnt3D m_posW;   // 0x4
    char *m_name;   // 0x10
    CAiMapLoc *m_aiMapLoc;   // 0x14
};

/* every anchor (SOCOM 1's CCompass, a vector; its one object) */
class CCompass : public std::vector<CCompassAnchor *> {
};

extern CCompass theCompass;

/* the anchor following who is talking on the radio (a word of theMission's, no name yet), the voice
 * object it asks, the talker's player object by id (no names yet) */
class CMission;
extern CMission theMission;
#define data_00436604 (*(CCompassAnchor **)((char *)&::theMission + 0x124))   /* theMission+0x124 */

class CZNetVoice {
public:
    unsigned char _0[0x24];
    int m_current;   // 0x24
    unsigned char _28[0x70 - 0x28];
    struct {
        int m_id;
        int m_4;
    } m_talkers[1];   // 0x70
    bool IsReceiving();
};

extern CZNetVoice *data_0045B718;

struct CCompassTalker {
    unsigned char _0[0x1c];
    CPnt3D m_pos;   // 0x1c
};

extern "C" CCompassTalker *func_002BD420(int id);

class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

extern "C" {
void sceVu0OuterProduct(float *out, float *a, float *b);
float acosf(float x);
float sinf(float x);
float fabsf(float x);
}

/* ztwod's screen scale, x then y (no name yet) */
extern float data_0049E9F0[2];

/* the local player's seal (fts_player.cpp's ftsGetPlayer) */
CZSealBody *ftsGetPlayer();

/* the frame's anchors, nearest first (SOCOM 1's global; the unit's initialiser builds it) */
std::vector<nav_package> navpoint_vec;

/* an anchor's icon textured from a handle (none without one), sized to the texture (one line in
 * SOCOM 1's InitHUD_Compass: inline) */
static inline void SetAnchorTexture(CHudCompassAnchor &anchor, zdb::CTexHandle *htex)
{
    anchor.m_texture = htex ? htex->m_texture : 0;
    if (anchor.m_texture) {
        anchor.m_width = (int)anchor.m_texture->m_width;
        anchor.m_height = (int)anchor.m_texture->m_height;
    }
}

/* the anchors' textures and colours, then every anchor and arrow loaded with the first texture
 * and listed under the anchor head, the head under the strip, the strip in the HUD's phases */
void MapCompass::InitHUD_Compass(zdb::CWorld *world)
{
    m_anchor_img[0] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_triangle.tif");
    m_anchor_img[1] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_bomb.tif");
    m_anchor_img[2] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_base.tif");
    m_anchor_img[3] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_base.tif");
    m_anchor_img[4] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_hostage.tif");
    m_anchor_img[5] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_friendly.tif");
    m_anchor_img[6] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_extraction.tif");
    m_anchor_img[7] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_objective.tif");
    m_anchor_img[8] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_objective.tif");
    m_anchor_img[9] = zdb::CAssetMgr::m_assets.GetTexHandle("hud_talk.tif");
    m_anchor_img[10] = zdb::CAssetMgr::m_assets.GetTexHandle("small_compass.tif");
    m_anchor_img[12] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_able.tif");
    m_anchor_img[13] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_bravo.tif");
    m_anchor_img[11] = zdb::CAssetMgr::m_assets.GetTexHandle("small_compass.tif");
    if (!m_anchor_img[10])
        m_anchor_img[10] = m_anchor_img[1];
    m_anchor_img[14] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_01.tif");
    m_anchor_img[15] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_02.tif");
    m_anchor_img[16] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_03.tif");
    m_anchor_img[17] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_04.tif");
    m_anchor_img[18] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_05.tif");
    m_anchor_img[19] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_06.tif");
    m_anchor_img[20] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_07.tif");
    m_anchor_img[21] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_08.tif");
    m_anchor_img[22] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_09.tif");
    m_anchor_img[23] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_10.tif");
    m_anchor_img[24] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_11.tif");
    m_anchor_img[25] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_12.tif");
    m_anchor_img[26] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_13.tif");
    m_anchor_img[27] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_14.tif");
    m_anchor_img[28] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_15.tif");
    m_anchor_img[29] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_16.tif");
    m_anchor_img[30] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_17.tif");
    m_anchor_img[31] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_18.tif");
    m_anchor_img[32] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_19.tif");
    m_anchor_img[33] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_20.tif");
    m_anchor_img[34] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_21.tif");
    m_anchor_img[35] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_22.tif");
    m_anchor_img[36] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_23.tif");
    m_anchor_img[37] = zdb::CAssetMgr::m_assets.GetTexHandle("ret_nav_24.tif");

    m_anchor_clr[0] = CPnt3D(86.0f, 63.0f, 27.0f);
    m_anchor_clr[1] = CPnt3D(62.0f, 62.0f, 32.0f);
    m_anchor_clr[3] = CPnt3D(62.0f, 37.0f, 36.0f);
    m_anchor_clr[2] = CPnt3D(40.0f, 62.0f, 36.0f);
    m_anchor_clr[4] = CPnt3D(62.0f, 49.0f, 36.0f);
    m_anchor_clr[5] = CPnt3D(32.0f, 49.0f, 128.0f);
    m_anchor_clr[6] = CPnt3D(36.0f, 40.0f, 62.0f);
    m_anchor_clr[7] = CPnt3D(76.0f, 70.0f, 32.0f);
    m_anchor_clr[8] = CPnt3D(70.0f, 76.0f, 76.0f);
    m_anchor_clr[9] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[10] = CPnt3D(64.0f, 12.0f, 12.0f);
    m_anchor_clr[12] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[13] = CPnt3D(32.0f, 62.0f, 62.0f);
    m_anchor_clr[11] = CPnt3D(100.0f, 64.0f, 32.0f);
    for (unsigned int jj = 14; jj < 38; jj++)
        m_anchor_clr[jj] = CPnt3D(32.0f, 62.0f, 32.0f);

    for (unsigned int ii = 0; ii < 38; ii++) {
        SetAnchorTexture(m_compass_anchors[ii], m_anchor_img[0]);
        SetAnchorTexture(m_compass_arrows[ii], m_anchor_img[0]);
        m_compass_anchors[ii].m_icon.SetUV(-0.01f, -0.01f, 1.02f, 1.02f);
        m_compass_arrows[ii].m_icon.SetUV(-0.01f, -0.01f, 1.02f, 1.02f);

        zdb::CTexture *tex = m_compass_anchors[ii].m_texture;

        if (tex) {
            m_compass_anchors[ii].m_icon.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex);
            m_anchor_head.push_back(&m_compass_anchors[ii].m_icon);
            m_compass_arrows[ii].m_icon.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex);
            m_compass_arrows[ii].m_icon.SetColor(m_anchor_clr[0].x, m_anchor_clr[0].y,
                                                 m_anchor_clr[0].z);
            m_anchor_head.push_back(&m_compass_arrows[ii].m_icon);
        }
    }
    m_poly.push_back(&m_anchor_head);
    theHUD->Add2D(&m_poly, -1, true);
}

/* the anchor tables built 38 to a table, everything placed at zero, room for 48 anchors */
MapCompass::MapCompass()
{
    m_newcompasstex = 0;
    m_compasstopx = 0.0f;
    m_compasstopy = 0.0f;
    m_compassbotx = 0.0f;
    m_compassboty = 0.0f;
    m_compasscenterx = 0.0f;
    m_compasscentery = 0.0f;
    m_scale = 0.0f;
    m_tex_width = 0.0f;
    m_tex_height = 0.0f;
    navpoint_vec.reserve(48);
}

MapCompass::~MapCompass()
{
    FreeVector(navpoint_vec);
}

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
        bool local = owner == ftsGetPlayer();
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

/* the anchors' pulse: its speed, its limits; the strip's centre on the screen; the anchor's scale as
 * last set (written here, its 2.0 never read); the distance past which only the kept types show;
 * the fade's floor (OVL1 .data 0x3DC4F0-0x3DC520; SOCOM 1's base_trans and the local static scalars
 * collapsed; the names ours) */
static float s_fPulseSpeed = 250.0f;
static float s_fPulseMin = 64.0f;
static float s_fPulseMax = 128.0f;
static PNT2D s_ptCompassCenter = {565.0f, 90.0f};
static float s_fAnchorScale = 2.0f;
static float s_fDistReject = 900.0f;
static float s_fFadeFloor = 0.4f;
/* the pulse now and its direction (bss 0x40C070, 0x40C078: downwards when set) */
static float s_fPulse;
static bool s_bPulseDown;

/* the cross product through VU0, w 1 (CPnt3D::Cross's body, inline here as in the SEAL units) */
static inline void CompassCross(const CPnt3D *a, const CPnt3D *b, CPnt3D *out)
{
    float r[4] __attribute__((aligned(16)));
    float va[4] __attribute__((aligned(16)));
    float vb[4] __attribute__((aligned(16)));

    va[0] = a->x;
    va[1] = a->y;
    va[2] = a->z;
    va[3] = 1.0f;
    vb[0] = b->x;
    vb[1] = b->y;
    vb[2] = b->z;
    vb[3] = 1.0f;
    sceVu0OuterProduct(r, va, vb);
    out->x = r[0];
    out->y = r[1];
    out->z = r[2];
}

/* the compass (SOCOM 1's MapCompass::Tick, 0x3CB160 there; SOCOM II's for the HUD's owner, the talker's
 * anchor, the kept types): the strip turned to the owner's look; every anchor and arrow off, the frame's
 * list emptied; each enabled compass anchor found on the strip, its arrow when it is off the end --
 * the far ones listed with their distance on the AI's cells, of which only the nearest stays lit */
void MapCompass::Tick(float dT)
{
    CZSealBody *pSeal = CHUD::GetHUDOwner();

    if (!pSeal)
        return;

    CHudCompassAnchor *pHudAnchor;
    CHudCompassAnchor *pHudArrow;
    CCompassAnchor *pThisAnchor;
    unsigned int ii;
    CPnt3D lookDir = pSeal->m_node->m_fwd;
    CPnt3D *pSealPos = &pSeal->m_node->m_pos;
    CAiMapLoc seal_maploc(pSeal->m_maploc);

    if (lookDir.y != 0.0f) {
        lookDir.y = 0.0f;
        lookDir.Normalize();
    }
    m_poly.SetCenterPos(s_ptCompassCenter, 0.75f * m_tex_width, 0.75f * m_tex_height);
    m_poly.Rotate_Zaxis(lookDir.x, lookDir.z);

    s_fPulse += s_fPulseSpeed * dT * (s_bPulseDown ? -1.0f : 1.0f);
    if (s_fPulse < s_fPulseMin) {
        s_fPulse = s_fPulseMin;
        s_bPulseDown = false;
    } else if (s_fPulse > s_fPulseMax) {
        s_fPulse = s_fPulseMax;
        s_bPulseDown = true;
    }
    for (unsigned int ii = 0; ii < 38; ii++) {
        m_compass_anchors[ii].m_icon.Off();
        m_compass_arrows[ii].m_icon.Off();
    }
    navpoint_vec.clear();

    CCompassAnchor *pTalk = data_00436604;

    if (pTalk && theNetwork.m_bNetwork) {
        CCompassTalker *pTalker = func_002BD420(data_0045B718->m_talkers[data_0045B718->m_current].m_id);

        if (data_0045B718->IsReceiving() && pTalker) {
            pTalk->m_posW = pTalker->m_pos;
            if (!pTalk->m_locked)
                pTalk->m_enabled = true;
        } else if (!pTalk->m_locked)
            pTalk->m_enabled = false;
    }

    unsigned int count = theCompass.size();

    for (ii = 0; ii < count; ii++) {
        pThisAnchor = theCompass[ii];
        pHudAnchor = &m_compass_anchors[ii];
        pHudArrow = &m_compass_arrows[ii];
        CPnt3D color;

        if (!pThisAnchor || (pThisAnchor && !pThisAnchor->m_enabled)) {
            pHudAnchor->m_icon.Off();
            pHudArrow->m_icon.Off();
            continue;
        }

        unsigned int img_type = pThisAnchor->m_type;
        nav_package *pNavPkg = 0;

        if (img_type >= 14 && img_type < 38 && pThisAnchor->m_aiMapLoc) {
            CAiMapLoc *loc = pThisAnchor->m_aiMapLoc;
            int dx = seal_maploc.x - loc->x;

            dx = seal_maploc.x < loc->x ? loc->x - seal_maploc.x : dx;

            int dy = seal_maploc.y < loc->y ? loc->y - seal_maploc.y : seal_maploc.y - loc->y;
            nav_package nav_pkg;

            nav_pkg.m_dist = dx + dy;
            nav_pkg.m_this_anchor = pThisAnchor;
            nav_pkg.m_hud_anchor = pHudAnchor;
            nav_pkg.m_hud_arrow = pHudArrow;
            navpoint_vec.push_back(nav_pkg);
            pNavPkg = &navpoint_vec.back();
        }
        if (img_type && img_type < 14)
            pHudAnchor->m_icon.SetTrans(s_fPulse);
        else
            pHudAnchor->m_icon.SetTrans(s_fPulseMax);
        pHudAnchor->m_icon.On();

        CPnt3D seal_to_anchor = pThisAnchor->m_posW - *pSealPos;
        CPnt3D up_vec = CPnt3D::unit_y;

        seal_to_anchor.y = 0.0f;

        float dist = seal_to_anchor.Normalize();
        float sin_theta;
        float cos_theta;
        float theta;
        float dist_fix;
        bool bKeep = false;

        if (theNetwork.m_bNetwork) {
            switch (img_type) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 12:
            case 13:
                bKeep = true;
                break;
            }
        } else {
            switch (img_type) {
            case 7:
            case 8:
            case 12:
            case 13:
            case 2:
            case 3:
                bKeep = true;
                break;
            }
        }
        if (!bKeep && dist > s_fDistReject) {
            pHudAnchor->m_icon.Off();
            pHudArrow->m_icon.Off();
            if (img_type < 14 || img_type >= 38)
                continue;
        }
        seal_to_anchor.x = -seal_to_anchor.x;
        seal_to_anchor.y = -seal_to_anchor.y;
        seal_to_anchor.z = -seal_to_anchor.z;

        cos_theta = (float)Dot(seal_to_anchor, lookDir);
        theta = acosf(cos_theta);
        CPnt3D cross;

        CompassCross(&lookDir, &up_vec, &cross);

        sin_theta = sinf(theta * (Dot(seal_to_anchor, cross) < 0.0f ? -1.0f : 1.0f));
        PNT2D custom_pt = {0};

        custom_pt.x = s_ptCompassCenter.x;
        custom_pt.y = s_ptCompassCenter.y;
        if (img_type >= 38)
            img_type = 0;

        if (m_anchor_img[img_type]) {
            if (pThisAnchor->GetPoint(&color))
                pHudAnchor->m_icon.SetColor(color.x, color.y, color.z);
            else
                m_compass_anchors[ii].m_icon.SetColor(m_anchor_clr[img_type].x, m_anchor_clr[img_type].y,
                                                      m_anchor_clr[img_type].z);
            m_compass_anchors[ii].m_icon.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, m_anchor_img[img_type]->m_texture);
        }

        dist_fix = fabsf(dist);

        if (dist_fix < 200.0f)
            dist_fix *= 0.005f;
        else
            dist_fix = 1.0f;
        if (dist_fix < 1.0f) {
            float k = 65.0f * dist_fix;

            s_fAnchorScale = 1.0f;
            custom_pt.x += k * sin_theta;
            custom_pt.y -= k * cos_theta;
            pHudAnchor->m_icon.SetCenterPos(custom_pt, pHudAnchor->m_width * s_fAnchorScale,
                                            pHudAnchor->m_height * s_fAnchorScale);
            dist_fix = 1.0f - dist_fix;

            float trans_val_temp = pHudAnchor->m_icon.GetTrans();

            trans_val_temp = trans_val_temp * (dist_fix + s_fFadeFloor);
            pHudAnchor->m_icon.SetTrans(trans_val_temp);
            pHudArrow->m_icon.Off();
            if (pNavPkg)
                pNavPkg->m_override_showme = true;
        } else {
            PNT2D custom_retainer = custom_pt;

            if (pHudArrow) {
                custom_pt.x += 50.0f * sin_theta;
                custom_pt.y -= 50.0f * cos_theta;
                s_fAnchorScale = 0.6f;
                pHudArrow->m_icon.SetCenterPos(custom_pt, pHudAnchor->m_width * s_fAnchorScale,
                                               pHudAnchor->m_height * s_fAnchorScale);
                pHudArrow->m_icon.Rotate_Zaxis(sin_theta, cos_theta);
                pHudArrow->m_icon.On();
                if (pNavPkg)
                    pNavPkg->m_show_arrow = true;
            }
            s_fAnchorScale = 1.2f;
            custom_pt = custom_retainer;
            custom_pt.x += 64.0f * sin_theta;
            custom_pt.y -= 64.0f * cos_theta;
            pHudAnchor->m_icon.SetCenterPos(custom_pt, pHudAnchor->m_width * s_fAnchorScale,
                                            pHudAnchor->m_height * s_fAnchorScale);
        }
    }

    unsigned int nav_count = navpoint_vec.size();

    if (nav_count) {
        int cur_dist = 1000;
        unsigned int cur_idx = (unsigned int)-1;

        for (unsigned int ii = 0; ii < nav_count; ii++) {
            nav_package *pNavPkg = &navpoint_vec[ii];

            if (pNavPkg->m_dist < cur_dist) {
                cur_dist = pNavPkg->m_dist;
                cur_idx = ii;
            }
            if (!pNavPkg->m_override_showme) {
                pNavPkg->m_hud_anchor->m_icon.Off();
                pNavPkg->m_hud_arrow->m_icon.Off();
            }
        }
        if (cur_idx != (unsigned int)-1) {
            nav_package *pNavPkg = &navpoint_vec[cur_idx];

            pNavPkg->m_hud_anchor->m_icon.On();
            if (pNavPkg->m_show_arrow)
                pNavPkg->m_hud_arrow->m_icon.On();
        }
    }
}

/* an anchor type's colour (a ternary of lvalues: the pointer is selected, the copy made after the
 * join; past the last, an unset temporary's) */
CPnt3D MapCompass::GetAnchorColor(unsigned int type)
{
    return type < 38 ? m_anchor_clr[type] : CPnt3D();
}

/* an anchor's colour: what its point gives, else its type's, else black */
CPnt3D MapCompass::GetAnchorColor(CCompassAnchor *anchor)
{
    CPnt3D color(0.0f, 0.0f, 0.0f);

    if (!anchor->GetPoint(&color)) {
        unsigned int type = anchor->m_type;

        if (type < 38)
            color = m_anchor_clr[type];
    }
    return color;
}

/* an anchor type's texture (SOCOM II's: the map's legend and items read it) */
zdb::CTexHandle *MapCompass::GetAnchorImg(unsigned int type)
{
    return type < 38 ? m_anchor_img[type] : 0;
}
