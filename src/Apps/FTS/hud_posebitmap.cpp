/* hud_posebitmap.cpp -- the stance indicator (OVL1 0x221C40-0x22218F): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_posebitmap.cpp), Tick to the constructor. SOCOM II shows the
 * stance as a word (STAND, CROUCH, PRONE) where SOCOM 1 swapped textures: the pose is set by a
 * function of its own now (SetPose; the name is ours), placed differently in a network game,
 * and the text is reset by another (ReInit; ours too). Defined in reverse of their image order
 * (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"

namespace zdb {
class CTexture;
class CWorld;
}

class CEntityCtrl {
public:
    virtual ~CEntityCtrl();
    virtual void PreTick(float dT);
    virtual void Tick(float dT);
    virtual void PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual CEntityCtrl *GetRemoteControl();
    virtual void SetThrottle(unsigned int which, float value);
};

enum POSE {
    POSE_STAND = 0,
    POSE_CROUCH = 1,
    POSE_PRONE = 2,
};

class CZSealBody;

class CSealCtrl : public CEntityCtrl {
public:
    virtual void PlayAiSound(int sound);
    virtual bool AllowCommand(CZSealBody *seal, int command, void *data);
    virtual bool IsPlayer() const;
    virtual bool IsAi() const;
    virtual bool IsRemote() const;
};

/* only what the stance indicator reads: CEntity's control at 0xC0, SOCOM II's stance byte */
class CZSealBody {
public:
    bool StanceChanged();
    bool IsRemote() { return m_control ? ((CSealCtrl *)m_control)->IsRemote() : false; }

    unsigned char _0[0xc0];
    CEntityCtrl *m_control;   // 0xc0
    unsigned char _c4[0x374 - 0xc4];
    signed char m_stance;   // 0x374 (SOCOM 1 read m_desiredState, 0x304)
};

class CHUD {
public:
    static CZSealBody *GetHUDOwner();

    static PNT4D m_hud_textcolor;   /* the HUD's text colour */
};

class CZNetwork {
public:
    int m_SealIndex;   // 0x0
    int m_TerrIndex;   // 0x4
    unsigned char m_bSessionMaster;   // 0x8
    unsigned char m_bNetwork;   // 0x9
};

extern CZNetwork theNetwork;

zdb::CTexture *GetTextureByName(zdb::CWorld *world, const char *name);

class PoseBitmap : public C2DBitmap {
public:
    PoseBitmap();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Tick(float dT);
    void SetPose(int stance);
    void Clear();
    void ReInit();
    void Init(zdb::CWorld *world);

    int m_curpose;   // 0x80
    zdb::CTexture *m_posetex[3];   // 0x84
    float m_posx;   // 0x90
    float m_posy;   // 0x94
    unsigned char m_error;   // 0x98
    C2DString m_text;   // 0x9c
    float m_textposx;   // 0x134
    float m_textposy;   // 0x138
    C2DFont *m_myfont;   // 0x13c
    float m_trans;   // 0x140
};   // 0x150 bytes

/* where the word goes: a network game's place, then the single player's -- variables (each read
 * with lwc1; MW builds a constant like these with lui) */
static float s_netposx = 480.0f;
static float s_netposy = 431.0f;
static float s_posx = 545.0f;
static float s_posy = 380.0f;

PoseBitmap::PoseBitmap()
{
    m_curpose = 0;
    m_posx = 0.0f;
    m_posy = 0.0f;
    m_error = 0;
    m_posetex[0] = 0;
    m_posetex[1] = 0;
    m_posetex[2] = 0;
    m_trans = 127.0f;
}

/* the stance textures found, the word loaded and listed as a child, standing */
void PoseBitmap::Init(zdb::CWorld *world)
{
    m_posetex[0] = GetTextureByName(world, "stance_stand.tif");
    m_posetex[1] = GetTextureByName(world, "stance_crouch.tif");
    m_posetex[2] = GetTextureByName(world, "stance_prone.tif");
    m_text.Load(0.765f, "stand", m_myfont, m_textposx, m_textposy, 1);
    m_text.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text.m_color[3] = CHUD::m_hud_textcolor.w;
    m_posx = -1.0f;
    m_posy = -1.0f;
    push_back(&m_text);
    SetPose(POSE_STAND);
    SetTrans(80.0f);
}

void PoseBitmap::ReInit()
{
    m_text.Load(0.765f, "stand", m_myfont, m_textposx, m_textposy, 1);
    m_text.m_color[0] = CHUD::m_hud_textcolor.x;
    m_text.m_color[1] = CHUD::m_hud_textcolor.y;
    m_text.m_color[2] = CHUD::m_hud_textcolor.z;
    m_text.m_color[3] = CHUD::m_hud_textcolor.w;
    m_posx = -1.0f;
    m_posy = -1.0f;
    SetPose(POSE_STAND);
    SetTrans(80.0f);
}

void PoseBitmap::Clear()
{
    m_text.ClearAndWipe();
    clear();
}

/* the word for a stance, placed for the game's kind and made opaque again. The stance comes
 * in signed (Tick loads it with lb) and is compared unsigned (andi 0xff): an int narrowed here.
 * The places are read into locals first (both loads ahead of both stores) */
void PoseBitmap::SetPose(int stance)
{
    if (theNetwork.m_bNetwork) {
        float x = s_netposx, y = s_netposy;

        m_posx = x;
        m_posy = y;
        m_text.m_socom2_6a = 1;
    } else {
        float x = s_posx, y = s_posy;

        m_posx = x;
        m_posy = y;
        m_text.m_socom2_6a = 0;
    }
    POSE pose = (POSE)stance;

    m_trans = 127.0f;
    if (pose == POSE_STAND) {
        m_text.SetString("STAND", true);
        m_text.SetPos(m_posx, m_posy);
    } else if (pose == POSE_CROUCH) {
        m_text.SetString("CROUCH", true);
        m_text.SetPos(m_posx, m_posy);
    } else if (pose == POSE_PRONE) {
        m_text.SetString("PRONE", true);
        m_text.SetPos(m_posx, m_posy);
    }
}

void PoseBitmap::Draw(zdb::CCamera *cam)
{
    if (IsOn())
        m_text.Draw(cam);
}

/* a new stance shown at once (not for a remote seal); otherwise the word fades */
void PoseBitmap::Tick(float dT)
{
    CZSealBody *owner = CHUD::GetHUDOwner();

    if (!owner)
        return;
    if (owner->StanceChanged() && !owner->IsRemote()) {
        SetPose(owner->m_stance);
    } else if (m_trans > 0.0f) {
        m_trans -= 64.0f * dT;
        if (m_trans < 0.0f)
            m_trans = 0.0f;
        m_text.SetTrans(m_trans);
    }
}
