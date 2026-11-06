/* hud_tcm.cpp -- the radio command menu object (OVL1 0x236250-0x23775F; the name ours): CTCM's
 * own code, theTCM's mode setter, the item adder the cache builders call. SOCOM II's own -- no SOCOM
 * 1 counterpart (hud_tcm.h has the cut into three units: the tree calls this unit's eight-byte
 * mode setter 0x236250 out of line, while Reset here inlines SetCache).
 *
 * Its initialiser (0x3FF4A0) stores the layout's colours at 0x4130C0-0x41310B, builds theTCM
 * (0x413120) with CTCM's constructor 0x237390 and registers its implicit destructor (0x237580), and
 * the two item caches, mpCache (0x4144F0) and spCache (0x414550), registering ~CTCMCache 0x230620
 * (hud_tcmcmds.cpp's). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "hud_tcm.h"
#include "../../gamez/znode/znode.h"

/* an item's node made from its cache entry (0x236D20, below tcmAddItem: written in the HUD's round 41,
 * byte-exact since round 42) */
CTCMNode *tcmCreateItem(CTCMEntry *entry, const char *parent, const char *caption, const char *name,
                        const char *text2, bool flag);

/* the world's action points as the menu names them (zseal.h's CNodeAction: its prompt at 0x2C) and
 * the ones the radio menu lists (OVL1 0x2B4CD0: the enabled ones of the action list, pushed onto
 * the vector given) */
class CNodeAction {
public:
    /* every action Open tagged `tcm` (bit 2 of 0x3F), pushed onto the vector given (the game
     * states' name, OVL1 0x2B4CD0) */
    static void GetTCMActions(std::vector<CNodeAction *> *actions);

    unsigned char _0[0x2c];
    const char *m_uistring;   // 0x2c
};

/* the menu's time constant (0.3; a variable: read with lwc1 -- the name ours) */
static float s_tcm_dd8 = 0.3f;

/* the layout's colours (OVL1 bss 0x4130C0-0x41310B, 16 bytes apart; the fifth a copy of the
 * fourth -- the initialiser copies it, 8 bytes and 4; CTCM::Update 0x236430 reads them; the names ours) */
static CPnt3D s_tcmColor0(255.0f, 0.0f, 16.0f);
static CPnt3D s_tcmColor1(255.0f, 128.0f, 64.0f);
static CPnt3D s_tcmColor2(0.0f, 255.0f, 64.0f);
static CPnt3D s_tcmColor3(48.0f, 48.0f, 48.0f);
static CPnt3D s_tcmColor4 = s_tcmColor3;

/* the menu, and the item caches by name: the network game's and the single-player game's (the
 * tree's cache builders 0x232BC0/0x2344E0 fill them; __sinit_hud_tcm.cpp byte-exact since round 65: CTCMCache's constructor) */
CTCM theTCM;
CTCMCache mpCache("mpCache");
CTCMCache spCache("spCache");

/* the action points given to the items under each "NODEACTION ROOT" node, child by child, each
 * item's two lines set to its prompt (0x236260: the tree's builder calls it; the name ours). The
 * child and its action are declared ahead of the inner walk: inside it they take the walk's index's
 * saved register and it theirs (MW spills the node to the frame either way, as the target does) */
void CTCM::SetNodeActions()
{
    std::vector<CNodeAction *> actions;

    CNodeAction::GetTCMActions(&actions);

    std::vector<CTCMNode *> found;

    theTCM.Find("NODEACTION ROOT", found, true);

    unsigned int n = found.size();

    if (n && actions.size()) {
        for (unsigned int i = 0; i < n; i++) {
            CTCMNode *node = found[i];

            if (node) {
                unsigned int m = node->GetNumChildren();

                if (m) {
                    unsigned int e = actions.size();
                    CTCMNode *child;
                    CNodeAction *action;

                    for (unsigned int j = 0; j < m && j < e; j++) {
                        child = node->m_children[j];
                        action = actions[j];

                        if (child && action) {
                            child->m_action = action;
                            child->m_text.SetString(action->m_uistring, true);
                            child->m_text2.SetString(action->m_uistring, true);
                        }
                    }
                }
            }
        }
    }
}

/* the item list's bars: a dark one at 110 and a blue-grey one opaque, both untextured over the same
 * box, each turned on and added to the item list's bitmap, then the closing clock and its flag
 * cleared */
void CTCM::SetupItemBars()
{
    m_11c0.SetHasTexture(false);
    m_11c0.SetColor(0.0f, 0.0f, 0.0f);
    m_11c0.SetTrans(110.0f);
    m_11c0.SetPos(136.0f, 100.0f, 140.0f, 132.0f);
    m_11c0.On();
    m_items.push_back(&m_11c0);
    m_fd0.SetHasTexture(false);
    m_fd0.SetColor(35.0f, 51.0f, 56.0f);
    m_fd0.SetTrans(128.0f);
    m_fd0.SetPos(136.0f, 100.0f, 140.0f, 132.0f);
    m_fd0.On();
    m_items.push_back(&m_fd0);
    m_dd0 = 0.0f;
    m_dce = 0;
}

/* the menu built: the root node, the mode 4, the three backing polys untextured, translucent and
 * black with two corners clear, the counters and the holds (SOCOM II's) */
CTCM::CTCM() : m_cache(0), m_da8(0), m_dac(0), m_db0(1.0f), m_hold(-1.0f)
{
    strcpy(m_cacheName, "No Cache Available");
    m_7d0.SetHasTexture(false);
    m_7d0.SetIsTrans(true);
    m_7d0.SetTrans(1, 0.0f);
    m_7d0.SetTrans(2, 0.0f);
    m_7d0.SetColor(0.0f, 0.0f, 0.0f);
    m_9c0.SetHasTexture(false);
    m_9c0.SetIsTrans(true);
    m_9c0.SetTrans(2, 0.0f);
    m_9c0.SetTrans(3, 0.0f);
    m_9c0.SetColor(0.0f, 0.0f, 0.0f);
    m_bb0.SetHasTexture(false);
    m_bb0.SetIsTrans(true);
    m_bb0.SetTrans(2, 0.0f);
    m_bb0.SetTrans(3, 0.0f);
    m_bb0.SetColor(0.0f, 0.0f, 0.0f);
    m_da0 = 0;
    m_da4 = -1.0f;
    m_dc8 = 0;
    m_dcd = 0;
    m_dcc = 0;
    m_dd4 = 0;
    m_dd8 = s_tcm_dd8;
    m_dd0 = 0;
    m_dce = 0;
    m_13b0 = 0;
}

void CTCM::Init()
{
    tcmBuildCaches();
    m_path.reserve(5);
    m_children.reserve(32);
}

void CTCM::ClearItems()
{
    m_items.clear();
}

void CTCM::Reset()
{
    DeleteChildren();
    SetCache(0);
    m_text.ClearAndWipe();
    m_text2.ClearAndWipe();
    /* through a reference (or the base): the end node is then computed off the list's own
     * address, as the target has it -- m_root.clear() folds both into this */
    C2D &root = m_root;

    root.clear();
    m_sounds.Clear();
    m_items.clear();
}

/* an item's node, from the current cache's entry of this name, under every node named `parent` (the
 * menu's root without one); the caption, the node's name and its second line default to the name.
 * The last parameter is a bool: every one of the ~100 calls in the tree's two cache builders passes a
 * constant 0 or 1, and the value goes through to the builder's byte store untouched */
CTCMNode *tcmAddItem(const char *name, const char *parent, const char *caption, const char *nodename,
                     const char *text2, bool flag)
{
    CTCMEntry *entry = 0;

    if (theTCM.m_cache)
        entry = theTCM.m_cache->Find(name);
    return tcmCreateItem(entry, parent, caption ? caption : name, nodename ? nodename : name,
                         text2 ? text2 : name, flag);
}

/* a poly textured from a handle, its two triangles with it (none without one), then told whether it
 * has one (ours: the three stores and the virtual call, three times in the builder). Through a local
 * copy of `this` defined after the texture's ternary: the target computes the poly (`addiu a0`)
 * right after the first store and the call's bool in its delay slot -- the copy, in the uses' block,
 * makes `this` a variable of its own numbered ahead of the argument (round 42: written through
 * `this`, a member, a free helper or a copy defined at the top, MW computes the bool first, or keeps
 * the copy as the stores' base) */
inline void C2DBitmapPoly::SetTexture(zdb::CTexHandle *h)
{
    zdb::CTexture *tex = h ? h->m_texture : 0;
    C2DBitmapPoly *self = this;

    self->m_texture = tex;
    self->m_tris[0].m_texture = tex;
    self->m_tris[1].m_texture = tex;
    self->SetHasTexture(tex != 0);
}

/* the item's node made under each node named `parent` (theTCM itself without a name), for a cache
 * entry -- none without one: captioned, named, its second line and entry set, the flag stored, the
 * back and the bar on "TCM.tif" (the bar's UVs the texture's right 70 %), the icon on "TCMarrow.tif",
 * then added under its parent and its root listed in the menu's (0x236D20; tcmAddItem's builder, the
 * name ours). 0 on every path. The handle's dead first definition after the count numbers it ahead
 * of the loop's counter, as the target's s4/s3 (declared at its first use the two swap); the list
 * reached through GetRoot() keeps &theTCM.m_root in a saved register from the loop's out-of-line
 * entry test without hoisting its end node (a reference local spills that, 240 bytes of frame).
 * Byte-exact since round 42 with SetTexture written through its copy of `this` (above); round 41's
 * six were each SetHasTexture's argument scheduled ahead of the poly */
CTCMNode *tcmCreateItem(CTCMEntry *entry, const char *parent, const char *caption, const char *name,
                        const char *text2, bool flag)
{
    if (entry) {
        std::vector<CTCMNode *> found;

        if (parent)
            theTCM.Find(parent, found, true);
        else
            found.push_back(&theTCM);

        unsigned int n = found.size();
        zdb::CTexHandle *tex = 0;

        if (n) {
            for (unsigned int i = 0; i < n; i++) {
                CTCMNode *at = found[i];
                CTCMNode *node = new CTCMNode;

                node->SetCaption(caption, 0);
                node->SetName(name);
                node->SetText2(text2);
                node->SetEntry(entry);
                node->m_741 = flag;

                tex = zdb::CWorld::GetTexHandle("TCM.tif");

                node->m_back.SetTexture(tex);
                node->m_back.SetUV(1.0f, 0.0f, 0.0f, 1.0f);
                node->m_bar.SetTexture(tex);
                node->m_bar.SetUV(0.7f, 0.0f, 0.3f, 1.0f);
                node->m_icon.SetTexture(zdb::CWorld::GetTexHandle("TCMarrow.tif"));
                at->AddChild(node);
                theTCM.GetRoot()->push_back(&node->m_root);
            }
        }
    }
    return 0;
}

void CTCM::SetCache(CTCMCache *cache)
{
    if (CacheName(cache))
        strcpy(m_cacheName, CacheName(cache));
    else
        strcpy(m_cacheName, "No Cache Available");
    m_cache = cache;
}

/* the node the path selects, walked down from the root -- the path started at the first child when
 * there is none yet and the root has children; none for an empty path (the walk's count taken once,
 * so MW unrolls it 8x as the target does) */
CTCMNode *CTCM::GetSelected()
{
    CTCMNode *node = 0;

    if (!m_path.size() && GetNumChildren())
        m_path.push_back(0);

    unsigned int n = m_path.size();

    if (n) {
        node = this;
        for (unsigned int i = 0; i < n; i++)
            node = node->m_children[m_path[i]];
    }
    return node;
}

/* the node of a depth's column: the path walked down from the root */
CTCMNode *CTCM::GetColumn(unsigned int depth)
{
    unsigned int n = m_path.size();
    CTCMNode *found = 0;

    if (n) {
        CTCMNode *node = this;

        for (unsigned int i = 0; i < n && i <= depth && !found; i++) {
            if (i == depth)
                found = node;
            else
                node = node->m_children[m_path[i]];
        }
    }
    return found;
}

/* a hold on the menu's input: added to while it lasts, unless reset */
void CTCM::Hold(bool reset, float t)
{
    if (!(t <= 0.0f)) {
        if (!reset && !(m_hold <= 0.0f))
            m_hold += t;
        else
            m_hold = t;
    } else {
        m_hold = 0.0f;
    }
}

/* the network game (fts_net_game's theNetwork, 0x45A0C0): whether it is one */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the voice recogniser (reco.cpp's theReco, 0x414680): its engine running, its context asked for, the
 * input's level and the last result's confidence */
class CReco {
public:
    float GetLevel();
    void SetConfidence(int conf);
    float GetConfidence();

    unsigned char _0[0x38];
    unsigned int m_init : 1;   // 0x38 bit 0
    unsigned int m_running : 1;   // bit 1
    unsigned int m_startReq : 1;   // bit 2
    unsigned int m_stopReq : 1;   // bit 3
    unsigned int m_contextReq : 1;   // bit 4
};

extern CReco theReco;

/* the display's rates and the microphone meter's place (OVL1 .data 0x3DCB10-0x3DCB8F; the names ours) */
static float s_tcmUpdatePulseRate = 8.0f;
static float s_tcmColumnRate = 7.0f;
static float s_tcmFadeRate = 2.0f;
static float s_tcmMicFadeRate = 100.0f;
static float s_tcmMicX = 288.0f;
static float s_tcmMicY = 390.0f;
static float s_tcmMicW = 42.0f;
static float s_tcmMicH = 42.0f;
static float s_tcmMicGap = 4.0f;
static float s_tcmLevelW = 4.0f;
static float s_tcmLevelH = 42.0f;
static float s_tcmConfLow = 0.5f;
static float s_tcmConfMid = 1.0f;
static float s_tcmConfDecay = 0.5f;

/* the menu's per-frame display (no SOCOM 1 counterpart; the name ours): the layout and the talk button,
 * the clocks, the pulse, the columns' fade to the menu's mode, the tree; offline with the recogniser
 * running or asked for a context, the microphone meter placed, its colour flashed by the last result's
 * confidence and fading; otherwise the meter fading out.
 * OPEN, 2 instructions (round 65): the column's rate arm opens `lui at; lwc1 f3,0(v1); lwc1 f2,rate(at)` in the
 * target, `lwc1 f3,0(v1); lui at; lwc1 f2,rate(at)` here: every register agrees, the longer path from the direction's
 * load keeps it first. Tried, none moves the lui ahead: the column through a pointer, a reference or a walker (an
 * indexed store lets the rate's load be hoisted out of the loop, which the target does not); step, product, direction
 * or rate in a local, plain or const (const or a cast rate only renumbers: 5) */
void CTCM::Update(float dt)
{
    Layout(dt);
    TalkTick(dt);
    m_dc8 += dt;
    m_dac += s_tcmUpdatePulseRate * (dt * m_db0);
    if (1.0f == m_db0) {
        if (!(m_dac <= 1.0f)) {
            m_dac = 1.0f;
            m_db0 *= -1.0f;
        }
    } else if (m_dac < 0.0f) {
        m_dac = 0.0f;
        m_db0 *= -1.0f;
    }

    for (unsigned int i = 0; i < 8; i++) {
        CTCMColumn &c = g_tcmColumns[i];
        float cur = c.m_4;
        float v;

        if (m_mode.m_mode == 0)
            v = 0.0f;
        else
            v = cur + s_tcmColumnRate * (dt * c.m_dir);
        if (!(v <= 1.0f))
            v = 1.0f;
        else if (v < 0.0f)
            v = 0.0f;
        if (v != cur)
            c.m_4 = v;
    }

    float fade = m_da0 + s_tcmFadeRate * (dt * m_da4);

    m_da0 = fade;
    if (!(fade <= 1.0f))
        m_da0 = 1.0f;
    else if (fade < 0.0f)
        m_da0 = 0.0f;
    CTCMNode::Tick(dt);
    if (!theNetwork.m_bNetwork && (theReco.m_running || theReco.m_contextReq)) {
        static float x = s_tcmMicGap + (s_tcmMicX + s_tcmMicW);
        static float y = s_tcmMicY + s_tcmMicH;

        m_items.SetPos(s_tcmMicX, s_tcmMicY, s_tcmMicW, s_tcmMicH);
        m_11c0.SetPos(x, y, s_tcmLevelW, -1.0f * s_tcmLevelH);
        m_fd0.SetPos(x, y, s_tcmLevelW, -1.0f * s_tcmLevelH * theReco.GetLevel());

        float conf = theReco.GetConfidence();

        if (!(conf <= 0.0f)) {
            if (conf < s_tcmConfLow) {
                CPnt3D d = s_tcmColor0 - s_tcmColor3;

                s_tcmColor4.x = d.x;
                s_tcmColor4.y = d.y;
                s_tcmColor4.z = d.z;
            } else if (conf < s_tcmConfMid) {
                CPnt3D d = s_tcmColor1 - s_tcmColor3;

                s_tcmColor4.x = d.x;
                s_tcmColor4.y = d.y;
                s_tcmColor4.z = d.z;
            } else {
                CPnt3D d = s_tcmColor2 - s_tcmColor3;

                s_tcmColor4.x = d.x;
                s_tcmColor4.y = d.y;
                s_tcmColor4.z = d.z;
            }
            m_13b0 = 1.0f;
            theReco.SetConfidence(0);
        }

        float flash = m_13b0 - s_tcmConfDecay * dt;

        m_13b0 = flash;
        if (flash < 0.0f)
            m_13b0 = 0.0f;
        m_items.SetColor(s_tcmColor3.x + s_tcmColor4.x * m_13b0, s_tcmColor3.y + s_tcmColor4.y * m_13b0,
                         s_tcmColor3.z + s_tcmColor4.z * m_13b0);
        m_items.SetTrans(128.0f);
        m_11c0.SetTrans(128.0f);
        m_fd0.SetTrans(128.0f);
        m_items.On();
        m_11c0.On();
        m_fd0.On();
    } else {
        float trans = m_items.GetTrans();

        if (!(trans <= 0.0f)) {
            trans -= s_tcmMicFadeRate * dt;
            if (trans < 0.0f)
                trans = 0.0f;
        }
        m_items.SetTrans(trans);
        m_11c0.SetTrans(trans);
        m_fd0.SetTrans(trans);
    }
}

void CTCMMode::Set(int mode)
{
    m_mode = mode;
}
