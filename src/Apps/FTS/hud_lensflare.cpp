/* hud_lensflare.cpp -- the HUD's lens flares (OVL1 0x1FF060-0x2011EF): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_lensflare.cpp), between the letterbox and the pause menu as there. SOCOM
 * II replaced CLensFlareSaturate with a manager of flare entries, each a bitmap with elements (the
 * HUD lane's round 36: the unit ends at 0x2011EF -- 0x2011F0 ticks the pause menu's bitmaps and its
 * static 0x409080, so it is hud_pausemenu.cpp's first function; this unit's bss runs 0x408FD0-0x40901B,
 * the pause menu's from 0x409020). Written: the manager's Tick, PreDITick, Init, ReInit, Reset,
 * LoadRdr and LoadLightSource; the wildcard name pair; the entry's constructor, destructor and Init;
 * the element's Place and (round 54) its Load -- all byte-exact; unwritten: the entry's
 * Tick 0x1FF8A0 and LoadFlare 0x1FFF40. Defined in reverse of their image order (-inline deferred).
 * Its initialiser, 0x3FDAB0, builds only <iostream>'s two init objects (SOCOM 1's built only the
 * header CSTable SOCOM II dropped: SOCOM II's unit includes <iostream>). */

#include <iostream>

#include <string.h>

#include "../../gamez/znode/znode.h"
#include "../../gamez/zcamera/zcamera.h"
#include "../../gamez/zrdr/zrdr.h"
#include "hud_lensflare.h"

/* the flare system (OVL1 bss 0x488F10; no rows yet): a flare made at a point with a size (its
 * handle back), moved, moved and sized, asked whether it shows, and released */
struct CFlareSystem {
    int Add(const CPnt3D *pos, float size);
    void Release(int handle);
};
extern CFlareSystem data_00488F10;
extern "C" void func_00336580(CFlareSystem *system, int handle, CPnt3D *pos);
extern "C" void func_003365E0(CFlareSystem *system, int handle, CPnt3D *pos, float size);
extern "C" bool func_00336520(CFlareSystem *system, int handle, void *query);

extern "C" int sprintf(char *dst, const char *fmt, ...);

/* a node name with up to five '*' wildcards counted through as digits, last first -- "lf*" gives
 * lf0, lf1, ... lf9 (SOCOM II's; the names ours): the name being expanded, its wildcards from the
 * end backwards, each one's digit, and how many it has (OVL1 bss 0x408FD0-0x40901B) */
char *g_flareNamePattern;
char *g_flareNameWild[5];
int g_flareNameDigit[5];
int g_flareNameWilds;

/* the wildcards written with their digits, the first wildcard's last */
static inline char *FlareNameFormat()
{
    for (int i = g_flareNameWilds - 1; i >= 0; i--) {
        char digit[4];

        sprintf(digit, "%d", g_flareNameDigit[i]);
        *g_flareNameWild[i] = digit[0];
    }
    return g_flareNamePattern;
}

/* the next name: the digits counted up as an odometer, the first wildcard the fastest; none when
 * they have all come round (or there are no wildcards). Pinned: -inline auto would expand it into
 * the loader, which calls it */
#pragma auto_inline off
char *NextFlareName()
{
    bool wrapped = false;

    for (int i = 0; i < g_flareNameWilds; i++) {
        wrapped = true;
        if (g_flareNameDigit[i] < 9) {
            g_flareNameDigit[i]++;
            wrapped = false;
            break;
        }
        g_flareNameDigit[i] = 0;
    }
    if (wrapped || !g_flareNameWilds)
        return 0;
    return FlareNameFormat();
}
#pragma auto_inline reset

/* the first name: the pattern kept, its last five wildcards found from the end, all digits 0 -- the
 * name itself when it has none */
char *FirstFlareName(char *name)
{
    g_flareNameWilds = 0;
    if (!name)
        return 0;
    g_flareNamePattern = name;
    for (int k = 0; k < 5; k++) {
        g_flareNameWild[k] = 0;
        g_flareNameDigit[k] = 0;
    }
    for (int i = strlen(name) - 1; i >= 0; i--) {
        if (g_flareNamePattern[i] == '*') {
            g_flareNameWild[g_flareNameWilds] = &g_flareNamePattern[i];
            g_flareNameWilds++;
            if (g_flareNameWilds == 5)
                i = -1;
        }
    }
    if (!g_flareNameWilds)
        return name;
    return FlareNameFormat();
}

/* the asset manager's library list (node_world.h's classes: units including znode.h declare their
 * own partial ones) */
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

/* the HUD the elements' bitmaps are added to */
class CHUD {
public:
    void Add2D(C2D *p2d, int phase, bool add_to_back);
};
extern CHUD *theHUD;

/* the element's settings: its texture loaded (if found) and its bitmap added to the HUD hidden, its
 * colour; its position scale (1 unless given); half its size (100 by 100 unless given); its opacity
 * and scale ranges, each kept as the second value and the first less it. Both ranges are read
 * into one THREE-float array (round 54): the target's "unused" sp+64-71 is that array's third float
 * and its padding -- MW rounds a stack object of 8-15 bytes to 8 and one of 16 or more to 16 (a
 * float[4] range sat at 48, a float[2] left the frame at 0x50), so only a 12-byte range lands at
 * 56 under the size (72) in a 0x60 frame (reco.cpp's AdjustVolume, the same rule) */
bool CFlareElement::Load(_zrdr *rdr)
{
    char *name = zrdr_findstring(rdr, "texture");

    if (name) {
        zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(name);

        if (tex) {
            m_bmp.Load(0.0f, 0.0f, tex);
            m_bmp.SetHasTexture(true);
            theHUD->Add2D(&m_bmp, -1, false);
            m_bmp.Off();
        }
    }
    float color[3];

    if (zrdr_findreal(rdr, "color", color, 3))
        m_bmp.SetColor(color[0], color[1], color[2]);
    m_posScale = 1.0f;
    zrdr_findreal(rdr, "position_scale", &m_posScale, 1);
    float size[2];

    if (!zrdr_findreal(rdr, "size", size, 2))
        size[0] = size[1] = 100.0f;
    m_halfWidth = 0.5f * size[0];
    m_halfHeight = 0.5f * size[1];
    float range[3];

    if (zrdr_findreal(rdr, "opacity", range, 2)) {
        m_opacity = range[1];
        m_opacityRange = range[0] - m_opacity;
    }
    if (zrdr_findreal(rdr, "scale", range, 2)) {
        m_scale = range[1];
        m_scaleRange = range[0] - m_scale;
    }
    return true;
}

/* the app camera: only its world camera (camera.h's CAppCamera, +0xB4), which the flare's points
 * are projected through by CCamera::GetScreenPos -- a point to its screen offsets (a 12-byte point:
 * the target's frame keeps it at sp+48), false when off screen */
class CAppCamera {
public:
    unsigned char _0[0xb4];
    zdb::CCamera *m_camera;   // 0xb4
};
extern CAppCamera *appCamera;

/* shown on screen at the point, centred on the screen less the point's offset scaled by the position
 * scale, sized and faded by `t` through the ranges -- or hidden, when not visible or off screen. The
 * size's two products are taken before the position's (the target's float order) */
void CFlareElement::Place(bool visible, const CPnt3D *pos, float t)
{
    if (visible) {
        CPnt3D xy;

        if (appCamera->m_camera->GetScreenPos(pos, &xy)) {
            m_bmp.On();
            float s = m_scale + t * m_scaleRange;
            float w = s * m_halfWidth;
            float h = s * m_halfHeight;
            float x = 320.0f - xy.x * m_posScale;
            float y = 224.0f - xy.y * m_posScale;

            m_bmp.SetPos((int)(x - w), (int)(y - h), (int)(x + w), (int)(y + h));
            m_bmp.SetTrans(128.0f * (m_opacity + t * m_opacityRange));
        } else
            m_bmp.Off();
    } else
        m_bmp.Off();
}

/* made off and transparent over the whole screen, then read from its record */
CLensFlareEntry::CLensFlareEntry(_zrdr *rdr, zdb::CNode *node)
{
    Init();
    SetOn(false);
    SetFlag1(false);
    SetFlag3(false);
    SetFlag4(false);
    m_node = 0;
    SetFlag6(false);
    SetHasTexture(false);
    Load(0.0f, 0.0f, 640.0f, 448.0f, 0);
    SetTrans(128.0f);
    SetIsTrans(true);
    LoadFlare(rdr, node);
}

/* every element hidden and deleted */
CLensFlareEntry::~CLensFlareEntry()
{
    if (m_elements.size()) {
        for (std::vector<CFlareElement *>::iterator i = m_elements.begin(); i != m_elements.end(); ++i)
            delete *i;
    }
}

/* off, no handle, no node; the glare's defaults; a pale yellow; hidden */
void CLensFlareEntry::Init()
{
    SetOn(false);
    SetFlag1(false);
    m_handle = -1;
    SetVisible(false);
    SetFlag3(false);
    m_9c = 0.0f;
    m_a0 = 0.0f;
    m_a4 = 0.5f;
    m_84 = 0.75f;
    m_88 = 1.0f / (1.0f - m_84);
    SetFlag4(false);
    m_node = 0;
    m_d0 = 1.0f;
    m_d4 = 1.0f;
    m_d8 = 0.0f;
    m_dc = 1.0f;
    SetColor(225.0f, 224.0f, 133.0f);
    Off();
}

/* the record's settings: its active item decides whether it is on; the angle's cosine and scale;
 * lens_flare, the node (the named child, or the given node if it has that name or no name is asked
 * for), mobile, a handle in the flare system with the visibility tolerance, the opacity and its
 * range; light_glare, the bitmap's colour, brightness, fade range and texture; lens_element, the
 * elements element1, element2 ... */
void CLensFlareEntry::LoadFlare(_zrdr *rdr, zdb::CNode *node)
{
    if (rdr && rdr->type == ZRDR_ARRAY) {
        const _zrdr *active = zrdr_flat_findtag_startidx(rdr, "active", 0);

        if (active && active->Item(0)->type == ZRDR_STRING) {
            if (!strcmp(active->Item(0)->String(), "on"))
                SetOn(true);
            else
                SetOn(false);
        }
        if (m_on) {
            if (zrdr_findreal(rdr, "max_angle", &m_84, 1)) {
                m_84 = m_84 * 0.017453292f;
                m_84 = cosf(m_84);
                m_88 = 1.0f / (1.0f - m_84);
            }
            const _zrdr *name;
            _zrdr *tag = zrdr_findtag(rdr, "lens_flare");

            if (tag) {
                bool b;
                float v[3];

                m_node = 0;
                name = zrdr_findtag(tag, "node");

                if (name && name->type == ZRDR_ARRAY && name->Item(0)->type == ZRDR_STRING) {
                    zdb::CNode *child = node->FindChild(name->Item(0)->String(), true);

                    if (child)
                        m_node = child;
                    else if (!strcmp(name->Item(0)->String(), node->GetName()))
                        m_node = node;
                } else
                    m_node = node;
                zrdr_findbool(tag, "mobile", &b);
                m_tracking = b;
                if (m_node) {
                    m_pos = CPnt3D::zero;
                    m_node->TransformToWorld(m_pos);
                    m_size = 0.0f;
                    if (zrdr_findreal(tag, "visibility_tolerance", v, 1))
                        m_size = v[0];
                    m_handle = data_00488F10.Add(&m_pos, m_size);
                }
                if (zrdr_findreal(tag, "opacity", v, 2)) {
                    m_d8 = v[0];
                    m_dc = v[1] - m_d8;
                }
                if (zrdr_findbool(tag, "always_on", &b))
                    m_flag6 = b;
                zrdr_findbool(tag, "angle_opacity", &b);
                m_angleOpacity = b;
                if (zrdr_findtag(tag, "opacity_range_off"))
                    SetRangeOpacity(false);
                else
                    SetRangeOpacity(true);
                if (zrdr_findreal(tag, "opacity_range", v, 2)) {
                    m_c8 = v[0];
                    m_cc = 1.0f / (v[1] - m_c8);
                }
            }
            tag = zrdr_findtag(rdr, "light_glare");
            if (tag) {
                const _zrdr *on = zrdr_findtag(tag, "active");

                if (on && on->Item(0)->type == ZRDR_STRING) {
                    if (!strcmp(on->Item(0)->String(), "on"))
                        SetFlag3(true);
                    else
                        SetFlag3(false);
                }
                if (m_flag3) {
                    float c[3];

                    if (zrdr_findreal(tag, "color", c, 3))
                        SetColor(c[0], c[1], c[2]);
                    zrdr_findreal(tag, "max_brightness", &m_a4, 1);
                    if (zrdr_findreal(tag, "fade_range", c, 2)) {
                        m_fadeEnd = c[1];
                        m_fadeScale = 1.0f / (m_fadeEnd - c[0]);
                    }
                    SetHasTexture(false);
                    char *tex = zrdr_findstring(tag, "texture");

                    if (tex) {
                        zdb::CTexHandle *h = zdb::CAssetMgr::m_assets.GetTexHandle(tex);

                        if (h) {
                            Load(0.0f, 0.0f, 640.0f, 448.0f, h);
                            SetHasTexture(true);
                        }
                    }
                    if (m_flag3)
                        theHUD->Add2D(this, -1, false);
                }
            }
            tag = zrdr_findtag(rdr, "lens_element");
            if (tag) {
                const _zrdr *on = zrdr_findtag(tag, "active");

                if (on && on->Item(0)->type == ZRDR_STRING) {
                    if (!strcmp(on->Item(0)->String(), "on"))
                        SetFlag4(true);
                    else
                        SetFlag4(false);
                }
                if (m_flag4) {
                    int i = 0;
                    _zrdr *item;

                    do {
                        char key[16];

                        i++;
                        sprintf(key, "element%d", i);
                        item = zrdr_findtag(tag, key);
                        if (item) {
                            CFlareElement *elem = new CFlareElement;

                            if (elem->Load(item))
                                m_elements.push_back(elem);
                            else
                                delete elem;
                        }
                    } while (item);
                }
            }
        }
    }
}

/* each frame while on: how near the camera looks along the line to the flare; the node's scale and
 * opacity brought up or down by whether it shows; the glare's alpha moved towards its goal; the
 * elements placed */
void CLensFlareEntry::Tick(float dT)
{
    if (m_on) {
        float dot = 0.0f;

        if (m_visible)
            SetFlag1(true);
        if (m_flag1 && (m_angleOpacity || m_flag3 || m_flag4)) {
            CMatrix *camera_mtx = (CMatrix *)appCamera->m_camera;
            CPnt3D camera_pos;
            CPnt3D camera_dir, light_dir;

            camera_pos.x = camera_mtx->m_matrix[3][0];
            camera_pos.y = camera_mtx->m_matrix[3][1];
            camera_pos.z = camera_mtx->m_matrix[3][2];
            camera_dir = *(CPnt3D *)camera_mtx->m_matrix[2];
            light_dir.x = camera_pos.x - m_pos.x;
            light_dir.y = camera_pos.y - m_pos.y;
            light_dir.z = camera_pos.z - m_pos.z;
            camera_dir.Normalize();
            light_dir.Normalize();
            float d = Dot(camera_dir, light_dir);

            if (d <= m_84)
                dot = 0.0f;
            else
                dot = m_88 * (d - m_84);
        }
        if (m_visible) {
            if (!m_flag6) {
                m_node->SetActive(true);
                if (m_d4 < 0.9999f) {
                    m_d4 += 25.0f * dT;
                    if (!(m_d4 <= 1.0f))
                        m_d4 = 1.0f;
                }
                if (m_d0 < 0.9999f) {
                    float rate = 1.0f;

                    m_d0 += rate * dT;
                    if (!(m_d0 <= 1.0f))
                        m_d0 = 1.0f;
                    m_node->SetScale(m_d0);
                }
            }
        } else if (m_flag1) {
            SetFlag1(false);
            if (!m_flag6) {
                float rate = 1.0f;

                m_d0 -= rate * dT;
                if (m_d0 < 0.01f)
                    m_d0 = 0.01f;
                m_node->SetScale(m_d0);
                m_d4 -= 25.0f * dT;
                if (m_d4 < 0.0f)
                    m_d4 = 0.0f;
                if (m_d0 < 0.011f && m_d4 < 0.001f)
                    m_node->SetActive(false);
            }
        }
        if (m_flag1) {
            float a = 1.0f;
            unsigned char angle = m_angleOpacity;

            if (angle)
                a = m_d8 + dot * m_dc;
            float o = 1.0f;
            unsigned char range = m_rangeOpacity;

            if (range) {
                o = m_d8;
                if (!(m_90 <= m_c8)) {
                    o += m_cc * (m_90 - m_c8);
                    if (!(o <= 1.0f))
                        o = 1.0f;
                }
            }
            if (!m_flag6 || angle || range)
                m_node->m_Opacity = o * (m_d4 * a);
        }
        if (m_flag3) {
            if (m_flag1) {
                if (!m_visible)
                    m_a0 = 0.0f;
                else
                    m_a0 = dot * m_a4;
                float glare_delta = m_a0 - m_9c;
                float abs_delta = fabsf(glare_delta);
                float step = 3.0f * dT;

                if (!(abs_delta <= step))
                    abs_delta = step;
                if (glare_delta < 0.0f)
                    abs_delta = abs_delta * -1.0f;
                m_9c += abs_delta;
                float fade;

                if (!(m_90 < m_fadeEnd))
                    fade = 0.0f;
                else {
                    fade = m_fadeScale * (m_fadeEnd - m_90);
                    if (!(fade <= 1.0f))
                        fade = 1.0f;
                }
                float opacity = m_9c * fade;

                if (!(opacity <= 1.0f))
                    opacity = 1.0f;
                else if (opacity < 0.0f)
                    opacity = 0.0f;
                SetTrans(128.0f * opacity);
                if (opacity < 0.025f)
                    Off();
                else
                    On();
            } else {
                SetTrans(0.0f);
                Off();
            }
        }
        if (m_flag4 && m_elements.size()) {
            bool visible = IsSeen() && IsVisible();
            for (std::vector<CFlareElement *>::iterator i = m_elements.begin(); i != m_elements.end(); ++i)
                (*i)->Place(visible, &m_pos, dot);
        }
    }
}

/* when the record's active item is "on": the group_node name copied and expanded through its
 * wildcards, each name looked up as a landmark of the world and then as a node anywhere under it,
 * and a flare made on each node found */
void CLensFlareManager::LoadLightSource(_zrdr *def)
{
    if (def && def->type == ZRDR_ARRAY) {
        _zrdr *active = zrdr_flat_findtag_startidx(def, "active", 0);

        if (active && active->Item(0)->type == ZRDR_STRING && !strcmp(active->Item(0)->String(), "on")) {
            /* const: its type byte is read once for both of the array tests (a plain pointer
             * reloads it after the second null test, 4 bytes long) */
            const _zrdr *group = zrdr_findtag(def, "group_node");

            if (group && group->type == ZRDR_ARRAY && group && group->Item(0)->type == ZRDR_STRING) {
                char *names = _strdup(group->Item(0)->String(), 0, 0);

                char *name = FirstFlareName(names);

                do {
                    zdb::CNode *node = zdb::CWorld::m_world->FindLandmark(name);

                    if (!node)
                        node = zdb::CWorld::m_world->FindChild(name, true);
                    if (node) {
                        CLensFlareEntry *flare = new CLensFlareEntry(def, node);

                        if (flare)
                            push_back(flare);
                    }
                    name = NextFlareName();
                } while (name);
                _free(names, 0, 0);
            }
        }
    }
}

/* the flares read from an rdr: every LightSourceDefinition record in it. The counter an int (an
 * unsigned one is strength-reduced into a fourth saved register), the item const (its type byte
 * read once for String()'s test). Pinned: Init calls it, where -inline auto would expand it */
#pragma auto_inline off
void CLensFlareManager::LoadRdr(const char *name)
{
    _zrdr *rdr = zrdr_read(name, 0, 0);

    if (rdr && rdr->type == ZRDR_ARRAY) {
        for (int i = 0; i < rdr->Count(); i++) {
            const _zrdr *item = rdr->Item(i);

            if (item->type == ZRDR_STRING && !strcasecmp(item->String(), "LightSourceDefinition"))
                LoadLightSource(rdr->Item(i + 1));
        }
    }
    if (rdr)
        zrdr_free((CRdrFile *)rdr);
}
#pragma auto_inline reset

/* every flare deleted, the list emptied, inactive (byte-exact since the list headers settled: the
 * target's three saved registers -- this, the end (this + 4) and this + 8 -- come from include/list's
 * begin(), the end taken first) */
void CLensFlareManager::Clear()
{
    if (size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i)
            delete *i;
        clear();
    }
    m_active = false;
}

/* the flares read from lensflare.rdr, active if there were any */
void CLensFlareManager::Init()
{
    m_active = false;
    LoadRdr("lensflare.rdr");
    if (size())
        m_active = true;
}

/* every flare on a node placed at the node's origin and handed to the flare system: made if it has
 * no handle yet, else moved (CHUD::ReInit_Init's last call on the manager; the name ours) */
void CLensFlareManager::ReInit()
{
    std::list<CLensFlareEntry *>::iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CLensFlareEntry *flare = *i;

        if (flare->m_node) {
            flare->m_pos = CPnt3D::zero;
            flare->m_node->TransformToWorld(flare->m_pos);
            if (flare->m_handle < 0)
                flare->m_handle = data_00488F10.Add(&flare->m_pos, flare->m_size);
            else
                func_003365E0(&data_00488F10, flare->m_handle, &flare->m_pos, flare->m_size);
        }
    }
}

/* every flare's handle given back to the flare system */
void CLensFlareManager::Reset()
{
    std::list<CLensFlareEntry *>::iterator i, e = end();

    for (i = begin(); i != e; ++i) {
        CLensFlareEntry *flare = *i;

        if (flare->m_handle >= 0)
            data_00488F10.Release(flare->m_handle);
        flare->m_handle = -1;
    }
}

/* while active, before the world draws: each flare that is on asked whether it shows, and a
 * tracking one moved to its node's origin */
void CLensFlareManager::PreDITick(float dT)
{
    if (m_active && size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i) {
            CLensFlareEntry *flare = *i;

            if (flare->m_on && flare->m_handle >= 0) {
                if (func_00336520(&data_00488F10, flare->m_handle, &flare->m_90))
                    flare->m_visible = true;
                else
                    flare->m_visible = false;
                if (flare->m_tracking && flare->m_node) {
                    flare->m_pos = CPnt3D::zero;
                    flare->m_node->TransformToWorld(flare->m_pos);
                    func_00336580(&data_00488F10, flare->m_handle, &flare->m_pos);
                }
            }
        }
    }
}

/* while active, every flare ticked. The iterator is assigned from begin(), so it lives in memory
 * (the copy is bitwise), as CZNewHudMap's finders'; the end is taken once into a local (the target
 * keeps it in a saved register, where end() in the test recomputes it from `this`) */
void CLensFlareManager::Tick(float dT)
{
    if (m_active && size()) {
        std::list<CLensFlareEntry *>::iterator i, e = end();

        for (i = begin(); i != e; ++i)
            (*i)->Tick(dT);
    }
}
