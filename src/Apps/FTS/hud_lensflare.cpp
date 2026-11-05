/* hud_lensflare.cpp -- the HUD's lens flares (OVL1 0x1FF060-0x2011EF): SOCOM 1's unit (its DWARF names
 * it, Z:\dev\Apps\FTS\hud_lensflare.cpp), between the letterbox and the pause menu as there. SOCOM
 * II replaced CLensFlareSaturate with a manager of flare entries, each a bitmap with elements (the
 * HUD lane's round 36: the unit ends at 0x2011EF -- 0x2011F0 ticks the pause menu's bitmaps and its
 * static 0x409080, so it is hud_pausemenu.cpp's first function; this unit's bss runs 0x408FD0-0x40901B,
 * the pause menu's from 0x409020). Written: the manager's Tick, PreDITick, Init, ReInit, Reset,
 * LoadRdr and LoadLightSource; the wildcard name pair; the entry's constructor, destructor and Init;
 * the element's Place -- all byte-exact; open: Clear, the element's Load (3); unwritten: the entry's
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
 * and scale ranges, each kept as the second value and the first less it.
 * OPEN, 3 instructions (one stack slot): the target reads both ranges into sp+56 and leaves
 * sp+64-71 unused (frame 0x60: colour 80, size 72). One shared range array gives the frame but swaps
 * `this` and `rdr` (s0/s1, 17 off); two arrays give the target's registers with opacity's at 64.
 * Tried: the two declared the other way round, each in a block of its own, an unused third array
 * between them, a CPnt3D or float[4] colour, a CPnt3D for every array (3 to 17 off). Round 48: one
 * shared array anywhere gives a 0x50 frame (17 off: the hole at 64 is a slot the target allots and
 * never touches), reading opacity into scale's array or a dead `if (0)` array get no slot either;
 * a float[4] or CPnt4D range gives the 0x60 frame with the range at 48 (6 off) */
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
    float opacity[2];
    float scale[2];

    if (zrdr_findreal(rdr, "opacity", opacity, 2)) {
        m_opacity = opacity[1];
        m_opacityRange = opacity[0] - m_opacity;
    }
    if (zrdr_findreal(rdr, "scale", scale, 2)) {
        m_scale = scale[1];
        m_scaleRange = scale[0] - m_scale;
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
void CFlareElement::Place(float t, bool visible, const CPnt3D *pos)
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

/* every flare deleted, the list emptied, inactive.
 * OPEN (4 bytes short): the target keeps three saved registers -- this, the end (this + 4) and
 * this + 8, the address begin() reads, taken before the loop for the final clear(), which
 * recomputes the end from this. MW here keeps two; the end taken after begin(), erase(begin(), e)
 * and end() in the test all miss too */
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
                if (func_00336520(&data_00488F10, flare->m_handle, flare->m_90))
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
