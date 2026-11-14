/* hud_teammatenames.cpp -- the teammates' names (OVL1 0x226960-0x22755F): SOCOM 1's unit (its DWARF
 * names it, Z:\dev\Apps\FTS\hud_teammatenames.cpp), SOCOM II's CMPTeammateNames: On, Tick,
 * WordIntersectionTest, UnInit, Init and the font Init (CHUD::Init's) in the image; Tick written in round 65. Defined in reverse of their image order (-inline deferred). */

#include <list>
#include <stdio.h>
#include <string.h>

#include "hud_teammatenames.h"


/* the two bitmaps' sizes (variables: read with lwc1), and their halves, which the unit's
 * initialiser (0x3FE940) computes and Tick reads (the names ours) */
static float s_bitmap1Size = 32.0f;
static float s_bitmap2Size = 32.0f;
static float s_bitmap1Half = 0.5f * s_bitmap1Size;
static float s_bitmap2Half = 0.5f * s_bitmap2Size;

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

/* the HUD's phases (CHUD::Add2D) */
class CHUD {
public:
    void Add2D(C2D *p2d, int phase, bool add_to_back);
};

extern CHUD *theHUD;

/* a valve as the names read it: its value (zanim.h's CValve) */
enum VALVE_TYPE {};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* the names made: eight strings in the font, listed, and the two bitmaps (the ok-to-talk and the
 * off arrow textures) added to the HUD's phases off, grey and teal, half transparent; shown while
 * the DISPLAY_MP_TEAM_NAMES valve is on (CHUD::Init's; SOCOM 1's Init took the font too) */
void CMPTeammateNames::Init(C2DFont *font)
{
    m_socom2_5e0 = 0;
    for (unsigned int i = 0; i < 8; i++) {
        m_names[i].SetFont(font);
        m_names[i].m_scale = 0.9f;
        push_back(&m_names[i]);
    }
    m_bitmap1.Off();
    m_bitmap1.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap1.SetTrans(64.0f);
    m_bitmap1.m_pTexHandle = zdb::CAssetMgr::m_assets.GetTexHandle("action_ok_talk2.tif");
    m_bitmap1.SetPos(0, 0, (int)s_bitmap1Size, (int)s_bitmap1Size);
    theHUD->Add2D(&m_bitmap1, -1, true);
    m_bitmap2.Off();
    m_bitmap2.SetColor(40.0f, 96.0f, 110.0f);
    m_bitmap2.SetTrans(64.0f);
    m_bitmap2.m_pTexHandle = zdb::CAssetMgr::m_assets.GetTexHandle("hud_arrow_off2.tif");
    m_bitmap2.SetPos(0, 0, (int)s_bitmap2Size, (int)s_bitmap2Size);
    theHUD->Add2D(&m_bitmap2, -1, true);
    m_socom2_5e4 = CValve::Create("DISPLAY_MP_TEAM_NAMES", (VALVE_TYPE)1)->m_value != 0;
    theHUD->Add2D(this, 1, true);
}

/* both bitmaps off, grey and teal, half transparent, placed at the origin */
void CMPTeammateNames::Init()
{
    m_socom2_5e0 = 0;
    m_bitmap1.Off();
    m_bitmap1.SetColor(128.0f, 128.0f, 128.0f);
    m_bitmap1.SetTrans(64.0f);
    m_bitmap1.SetPos(0, 0, (int)s_bitmap1Size, (int)s_bitmap1Size);
    m_bitmap2.Off();
    m_bitmap2.SetColor(40.0f, 96.0f, 110.0f);
    m_bitmap2.SetTrans(64.0f);
    m_bitmap2.SetPos(0, 0, (int)s_bitmap2Size, (int)s_bitmap2Size);
}

/* the font as the test reads it: the glyphs' top and bottom (SOCOM 1's, 8 bytes on in SOCOM II) */
class C2DFont {
public:
    unsigned char _0[0x20];
    int m_displaytop;   // 0x20
    int m_displaybottom;   // 0x24
};

/* a string's width (the float at 0x3C, where SOCOM 1 read m_length): an inline accessor, which makes
 * the width the sum's right operand while it is loaded first, as the target has it */
static inline float StringWidth(const C2DString *s)
{
    return s->m_length;
}

/* whether a rectangle overlaps a string's: its corner the position (12.4 fixed point) moved by
 * its offset, as wide as StringWidth and as tall as its font. The last test is written
 * `!(ty2 <= y1)`: as `y1 < ty2` MW materialises it into v0 where the target branches on at (the
 * negated test's early return is the only form that gives the target's block order) */
#pragma auto_inline off
unsigned char WordIntersectionTest(int tx1, int ty1, int tx2, int ty2, C2DString *target)
{
    int x1 = (int)(target->m_posx + target->m_socom2_7c) >> 4;
    int y1 = (int)(target->m_posy + target->m_socom2_80) >> 4;
    int x2 = (int)(x1 + StringWidth(target));
    int y2 = y1 + (target->m_font->m_displaybottom - target->m_font->m_displaytop);

    if (!(tx1 < x2 && x1 < tx2 && ty1 < y2 && !(ty2 <= y1)))
        return false;
    return true;
}
#pragma auto_inline reset

/* every name off and emptied */
void CMPTeammateNames::UnInit()
{
    for (unsigned int i = 0; i < 8; i++) {
        m_names[i].Off();
        m_names[i].SetString(0, true);
    }
}

/* the tick's world (the owners' views): the network game, the radio (hud_mpinfobox.cpp's CZNetVoice), the
 * entity list, the SEAL read at its head and its teams, the persona's clan tag, the camera's matrix */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

struct CZNetVoice {
    bool IsReceiving();

    unsigned char _0[0x24];
    int m_slot;   // 0x24
    unsigned char _28[0x70 - 0x28];
    struct {
        int m_client;   // 0x0
        unsigned int m_4;   // 0x4
    } m_slots[12];   // 0x70
};

extern CZNetVoice *data_0045B718;

class CEntity;

namespace zdb {
class CNode {
public:
    bool Rendered(int frames);

    unsigned char _0[Z_OFS(0x5c, 0x6c)];   /* the host's: the vtable pointer first, tag_NODE_PARAMS at 16 */
    unsigned char m_5c;   // 0x5c (host 0x6c): bit 0, shown
};
}

class CEntity {
public:
    enum TYPE { ENT_TYPE_SEAL = 2 };

    zdb::CNode *GetNode();

    unsigned char _0[0x10];
    unsigned char m_type;   // 0x10

    static std::list<CEntity *> m_list;
};

class CZSealBody : public CEntity {
public:
    void GetHeadParams(CPnt3D *pos, CPnt3D *dir);

    unsigned char _11[0x14 - 0x11];
    const char *m_name;   // 0x14
    unsigned char _18[0xc8 - 0x18];
    unsigned int m_teams;   // 0xc8: the team bits
    unsigned char _cc[0xe1 - 0xcc];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4
    unsigned char _e1_5 : 3;
    unsigned char _e2[0xfc8 - 0xe2];
    int m_player;   // 0xfc8
};

CZSealBody *ftsGetPlayer();
extern "C" float func_00548050(CZSealBody *seal);

class CZPersonaState {
public:
    static const char *GetPersonaClanTag(int client);
};

/* the world's camera as the tick reads it: its node's matrix */
struct CTeamNamesCamNode {
    unsigned char _0[0x20];
    CMatrix m_matrix;   // 0x20
};

struct CTeamNamesCamera {
    unsigned char _0[0x3f0];
    CTeamNamesCamNode *m_node;   // 0x3f0
};

namespace zdb {
class CWorld {
public:
    unsigned char _0[Z_OFS(0x128, 0x134)];   /* the host's: CNode's vptr first, its 16-aligned base after */
    CTeamNamesCamera *m_camera;   // 0x128 (host 0x134)

    static CWorld *m_world;
};
}

/* the font's extent as the crowding test reads it */
struct CTeamNamesFont {
    unsigned char _0[0x20];
    int m_20;   // 0x20
    int m_24;   // 0x24
};

/* the bitmaps' gaps above a talker's head (OVL1 .data 0x3DC878, 0x3DC880; the names ours) */
static float s_bitmapGap = 10.0f;
static float s_bitmapRaise = -7.0f;

/* an element by index, read through its address (the target's `addiu r,sp,k; lwc1 f,0(r)`: a marked device
 * for SOCOM 1's accessor) */
static inline float &Elem(PNT3D &p, int i) { return (&p.x)[i]; }
static inline float &Elem(PNT4D &p, int i) { return (&p.x)[i]; }

/* the names (SOCOM 1's CTeamMateNames::Tick grown, its body rewritten): the entity list walked for up to
 * eight SEALs rendered within range of the player's team, each one's name (with the clan's tag online) put
 * over its head on screen, moved down until it crowds no earlier name, faded by distance while alive; online
 * the talker's two bitmaps over it; the strings left over off */
void CMPTeammateNames::Tick(float dT)
{
    const std::list<CEntity *> &list = CEntity::m_list;
    std::list<CEntity *>::const_iterator it = list.begin();
    int talker = -1;

    if (theNetwork.m_bNetwork && data_0045B718->IsReceiving())
        talker = data_0045B718->m_slots[data_0045B718->m_slot].m_client;
    m_bitmap1.Off();
    m_bitmap2.Off();

    unsigned int count = 0;
    CZSealBody *player = ftsGetPlayer();
    bool show = true;

    for (; it != list.end() && count < 8; ++it) {
        CEntity *e = *it;
        CZSealBody *seal = e && e->m_type == CEntity::ENT_TYPE_SEAL ? (CZSealBody *)e : 0;
        zdb::CNode *node = seal ? seal->GetNode() : 0;

        if (!theNetwork.m_bNetwork) {
            bool vis = false;

            if (seal) {
                bool allied = true;
                unsigned int teams = seal->m_teams;

                if ((teams & 2) == 0 && (teams & 0x40000) == 0)
                    allied = false;

                if (allied)
                    vis = true;
            }
            show = vis;
        }
        if (seal && node && node->Rendered(2) && func_00548050(seal) < 5.0f && player != seal) {
            unsigned int teams = seal->m_teams;

            if ((teams & player->m_teams) && !(teams & 0x20000) && (node->m_5c & 1) && show) {
                CPnt3D head, dir;

                seal->GetHeadParams(&head, &dir);

                CMatrix view = zdb::CWorld::m_world->m_camera->m_node->m_matrix;
                CPnt4D in;

                in.x = head.x;
                Elem(in, 1) = 3.0f + Elem(head, 1);
                Elem(in, 2) = Elem(head, 2);
                Elem(in, 3) = 1.0f;

                CPnt4D out;

                view.Transform(&in, &out, 1);

                float w = Elem(out, 3);

                if (!(w <= 0.0001f)) {
                    const char *tag;
                    const char *name = seal->m_name;
                    float inv = 1.0f / w;
                    char buf[64];

                    if (name && strlen(name))
                        strcpy(buf, name);
                    if (theNetwork.m_bNetwork) {
                        tag = CZPersonaState::GetPersonaClanTag(seal->m_player);

                        if (tag && strlen(tag))
                            sprintf(buf, "[%s] %s", tag, name);
                    }
                    m_names[count].SetString(buf, true);
                    m_names[count].UpdatePos();

                    float half = 0.5f * StringWidth(&m_names[count]);
                    float x = 320.0f + out.x * inv - 2048.0f - half;
                    float oy = Elem(out, 1);
                    float y = 224.0f + oy * inv - 2048.0f;

                    if (x < 600.0f) {
                        float rowH = m_socom2_5e0;

                        if (y < 448.0f - rowH && !(x <= 0.0f) && !(y <= rowH)) {
                            if (!(x + half <= 600.0f))
                                x = 600.0f - half;
                            else if (x - half < 0.0f)
                                x = half;
                            for (unsigned int j = 0; j < count; j++) {
                                CTeamNamesFont *font = (CTeamNamesFont *)m_names[count].m_font;

                                if (WordIntersectionTest((int)(x - 10.0f), (int)(y - 10.0f),
                                                         (int)(10.0f + (x + m_names[count].m_length)),
                                                         (int)(10.0f + (y + (float)(font->m_24 - font->m_20))), &m_names[j])) {
                                    j = (unsigned int)-1;
                                    y += 5.0f;
                                }
                            }
                            if (!seal->m_socom2_e1_4)
                                m_names[count].SetTrans(100.0f - 20.0f * func_00548050(seal));
                            else
                                m_names[count].SetTrans(100.0f);
                            if (theNetwork.m_bNetwork && talker == seal->m_player) {
                                float sx = 320.0f + out.x * inv - 2048.0f;
                                float top = y - s_bitmapGap;

                                m_bitmap2.SetPos((int)(sx - s_bitmap2Half), (int)top, (int)(sx + s_bitmap2Half),
                                                 (int)(top - s_bitmap2Size));

                                float top1 = y - s_bitmapGap - s_bitmap2Size - s_bitmapRaise;

                                m_bitmap1.SetPos((int)(sx - s_bitmap1Half), (int)(top1 - s_bitmap1Size),
                                                 (int)(sx + s_bitmap1Half), (int)top1);
                                m_bitmap2.On();
                                m_bitmap1.On();
                            }
                            m_names[count].SetPos((int)x, (int)y);
                            m_names[count].On();
                            count++;
                        }
                    }
                }
            }
        }
    }
    for (; count < 8; count++)
        m_names[count].Off();
}

void CMPTeammateNames::On()
{
    if (m_socom2_5e4)
        C2D::On();
    else
        C2D::Off();
}
