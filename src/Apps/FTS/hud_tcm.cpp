/* hud_tcm.cpp -- the radio command menu object (OVL1 0x236250-0x23775F; the name ours): CTCM's
 * own code, theTCM's mode setter, the item adder the cache builders call. SOCOM II's own -- no SOCOM
 * 1 counterpart (hud_tcm.h has the cut into three units: the tree calls this unit's eight-byte
 * mode setter 0x236250 out of line, while Reset here inlines SetCache).
 *
 * Its initialiser (0x3FF4A0, not written yet) builds theTCM (0x413120) with CTCM's constructor
 * 0x237390, the two item caches spCache (0x4144F0) and mpCache (0x414550), and the layout's
 * colours at 0x4130C0-0x413108. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "hud_tcm.h"

/* the tree's (0x231A30-0x23624F): an item's node made from its cache entry */
extern "C" CTCMNode *func_00236D20(CTCMEntry *entry, int arg, const char *caption, const char *text2,
                                   const char *name, int flags);

/* the menu's time constant (0.3; a variable: read with lwc1 -- the name ours) */
static float s_tcm_dd8 = 0.3f;

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
    func_00231FD0(this);
    SetCache(0);
    m_text.ClearAndWipe();
    m_text2.ClearAndWipe();
    /* through a reference (or the base): the end node is then computed off the list's own
     * address, as the target has it -- m_root.clear() folds both into this */
    C2D &root = m_root;

    root.clear();
    func_00232950(&m_sounds);
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
