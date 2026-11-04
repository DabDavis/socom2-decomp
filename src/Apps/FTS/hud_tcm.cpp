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

/* an item's node made from its cache entry (0x236D20, this unit's -- not written: the HUD's round 39
 * had it at the target's size, 51 instructions off, and took it out). Read off the target: for a
 * non-null entry, the nodes named by the second argument (theTCM.Find((const char *)arg, found,
 * true)) or theTCM itself (found.push_back(&theTCM)); for each a new CTCMNode given SetCaption(a2,
 * 0), SetName(a3), SetText2(t0), SetEntry(entry), m_741 = the last argument's byte, the back and the
 * bar on GetTexHandle("TCM.tif") (an inline setting the poly's and both triangles' texture to the
 * handle's +0xC or 0, then SetHasTexture), UVs (1,0,0,1) and (0.7,0,0.3,1), the icon on
 * "TCMarrow.tif", AddChild under the found node and its m_root pushed onto theTCM.m_root (the list's
 * address hoisted into a saved register: a reference local gives the size); 0 returned on every
 * path. Open: the target homes t0 and the flag (`sb t1`) at 188/187 in a 224-byte frame where the
 * draft took 240 -- and with a `bool` last parameter tcmAddItem would convert its int (it passes t1
 * through untouched) */
extern "C" CTCMNode *func_00236D20(CTCMEntry *entry, int arg, const char *caption, const char *text2,
                                   const char *name, int flags);

/* the world's action points as the menu names them (zseal.h's CNodeAction: its prompt at 0x2C) and
 * the ones the radio menu lists (OVL1 0x2B4CD0: the enabled ones of the action list, pushed onto
 * the vector given) */
class CNodeAction {
public:
    unsigned char _0[0x2c];
    const char *m_uistring;   // 0x2c
};

extern "C" void func_002B4CD0(std::vector<CNodeAction *> *actions);

/* the menu's time constant (0.3; a variable: read with lwc1 -- the name ours) */
static float s_tcm_dd8 = 0.3f;

/* the layout's colours (OVL1 bss 0x4130C0-0x41310B, 16 bytes apart; the fifth a copy of the
 * fourth -- the initialiser copies it, 8 bytes and 4; func_00236430 reads them; the names ours) */
static CPnt3D s_tcmColor0(255.0f, 0.0f, 16.0f);
static CPnt3D s_tcmColor1(255.0f, 128.0f, 64.0f);
static CPnt3D s_tcmColor2(0.0f, 255.0f, 64.0f);
static CPnt3D s_tcmColor3(48.0f, 48.0f, 48.0f);
static CPnt3D s_tcmColor4 = s_tcmColor3;

/* the menu, and the item caches by name: the network game's and the single-player game's (the
 * tree's cache builders 0x232BC0/0x2344E0 fill them).
 * OPEN, 2 instructions (__sinit_hud_tcm.cpp, 0x160 against 0x168): the colours, theTCM and both
 * caches' vector stores are the target's, but for each cache's strcpy the target materialises the
 * object (`addiu v0,v0,lo(cache)`) and takes the name off it (`addiu a0,v0,12`), where MW folds the
 * name's address into one lui/addiu pair. Tried: the name through a pointer or reference local, a
 * cast of this, &m_name[0], an inline setter or accessor, a member class holding the name, the
 * constructor out of class, member-initialised, copy- or converting-initialised globals -- all fold */
CTCM theTCM;
CTCMCache mpCache("mpCache");
CTCMCache spCache("spCache");

/* the action points given to the items under each "NODEACTION ROOT" node, child by child, each
 * item's two lines set to its prompt (0x236260: the tree's builder calls it; the name ours). The
 * child and its action are declared ahead of the inner walk: inside it they take the walk's index's
 * saved register and it theirs (MW spills the node to the frame either way, as the target does) */
void tcmSetNodeActions()
{
    std::vector<CNodeAction *> actions;

    func_002B4CD0(&actions);

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
    func_002344E0();
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

/* an item's node, from the current cache's entry of this name (the captions default to it) */
CTCMNode *tcmAddItem(const char *name, int arg, const char *caption, const char *text2, const char *nodename,
                     int flags)
{
    CTCMEntry *entry = 0;

    if (theTCM.m_cache)
        entry = theTCM.m_cache->Find(name);
    return func_00236D20(entry, arg, caption ? caption : name, text2 ? text2 : name, nodename ? nodename : name,
                         flags);
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

void CTCMMode::Set(int mode)
{
    m_mode = mode;
}
