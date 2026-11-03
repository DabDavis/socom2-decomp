/* hud_tcm.h -- the radio command menu ("TCM": its textures TCM.tif, TCMarrow.tif and
 * action_TCM.tif, its sounds .TCM_SELECT and .TCM_SLIDE, mission.rdr's tcm::nodeaction_root):
 * SOCOM II's own (SOCOM 1's orders.cpp OrdersMenu is another design), so every name here is ours.
 *
 * Three units share it, cut where the code stops inlining (auto-inlining never crosses units):
 *  - hud_tcmcmds.cpp (OVL1 0x22EDB0-0x231A2F, initialiser 0x3FF430): the commands an item runs
 *    and the tests that show or hide it, the item caches, the layout;
 *  - 0x231A30-0x23624F: the menu tree (CTCMNode's own code: its constructor 0x232660 and
 *    destructor 0x232490), the cache builders, the menu's input -- it calls hud_tcmcmds.cpp's two
 *    eight-byte node setters (0x230550/0x230540) out of line;
 *  - 0x236250-0x23775F (initialiser 0x3FF4A0: the menu theTCM 0x413120 and the caches
 *    "mpCache" 0x414550 and "spCache" 0x4144F0): the menu object -- the tree calls its
 *    eight-byte byte setter 0x236250 out of line. */
#ifndef HUD_TCM_H
#define HUD_TCM_H

#include <vector>

#include "../../gamez/ztwod/ztwod.h"

class CZSealBody;
class CNodeAction;

/* what an item's command and its test return: the test's value is the node's state (a byte enum:
 * the tree copies it with lb, compares it with lbu) */
enum TCM_STATE {
    TCM_SHOWN = 0,
    TCM_HIDDEN = 1
};

/* a command or a test: the player and the item's argument */
typedef int (*TCMFunc)(CZSealBody *player, int arg);

/* an item the caches hold: its name, its command and its test, each with its argument (0x70
 * bytes: the cache's Add news them) */
class CTCMEntry {
public:
    CTCMEntry()
    {
        strcpy(m_name, "No Cache Name");
        m_action = 0;
        m_actionArg = 0;
        m_update = 0;
        m_updateArg = 0;
    }

    char m_name[0x60];   // 0x0
    TCMFunc m_action;   // 0x60
    TCMFunc m_update;   // 0x64
    int m_actionArg;   // 0x68
    int m_updateArg;   // 0x6c
};   // 0x70 bytes

/* the items by name (two, the multiplayer and the single-player game's: their initialiser builds
 * them with their names) */
class CTCMCache {
public:
    CTCMEntry *Find(const char *name);
    CTCMEntry *Add(const char *name, TCMFunc action, int actionArg, TCMFunc update, int updateArg);
    ~CTCMCache();

    std::vector<CTCMEntry *> m_entries;   // 0x0
    char m_name[0x40];   // 0xc
};

/* a node of the menu (0x750 bytes: the tree's constructor 0x232660 builds it, its destructor
 * 0x232490 tears it down) */
class CTCMNode {
public:
    /* whether the node is still waiting out its last message's delay (ours: the commands test the
     * flag through it, materialised) */
    bool IsWaiting() const { return !m_ready; }

    std::vector<CTCMNode *> m_children;   // 0x0
    CTCMNode *m_parent;   // 0xc
    CTCMEntry *m_entry;   // 0x10
    C2D m_root;   // 0x14
    C2DBitmapPoly m_back;   // 0x30
    C2DBitmapPoly m_bar;   // 0x220
    C2DBitmapPoly m_icon;   // 0x410
    C2DString m_text;   // 0x600
    char *m_name;   // 0x698
    C2DString m_text2;   // 0x69c
    int m_734;   // 0x734
    TCM_STATE m_state;   // 0x738
    float m_delay;   // 0x73c
    bool m_ready;   // 0x740
    bool m_741;   // 0x741
    CNodeAction *m_action;   // 0x744
} __attribute__((aligned(16)));   // 0x750 bytes

/* the tree's (0x231A30-0x23624F, not written yet): the caption cut to a length, the name, the
 * second string, the message delay */
extern "C" {
void func_00232170(CTCMNode *node, const char *caption, int len);
void func_00232100(CTCMNode *node, const char *name);
void func_002320E0(CTCMNode *node, const char *s);
void func_00232240(CTCMNode *node, float delay);
}

#endif
