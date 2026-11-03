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

class CSnd;

/* the menu's sounds (0x10 bytes; the tree's code: its constructor 0x2329D0, Load 0x232970 --
 * .TCM_SLIDE, .TCM_SELECT twice -- Play 0x232890, Clear 0x232950) */
class CTCMSounds {
public:
    bool m_loaded;   // 0x0
    CSnd *m_back;   // 0x4: Play(2)
    CSnd *m_slide;   // 0x8: Play(0)
    CSnd *m_select;   // 0xc: Play(1)
};

/* the menu (theTCM, OVL1 bss 0x413120: the menu object's initialiser 0x3FF4A0 builds it with its
 * constructor 0x237390 and registers its destructor 0x237580) -- its root node, then its own */
class CTCM : public CTCMNode {
public:
    unsigned char m_state;   // 0x750: the recogniser's listening states 1 and 6 (reco.cpp's)
    std::vector<int> m_path;   // 0x754: the open column's selections, one per depth
    int m_760;   // 0x760
    char m_cacheName[0x6c];   // 0x764: "No Cache Available"
    unsigned char _7d0[0xdb8 - 0x7d0];
    CTCMSounds m_sounds;   // 0xdb8
    unsigned char _dc8[0x13c0 - 0xdc8];

    /* the menu's depth and its path (reco.cpp's: a push through the reference loads the vector's
     * words off its address, the target's tcmOpenNode) */
    int GetDepth() { return m_path.size(); }
    std::vector<int> &GetPath() { return m_path; }
};

extern CTCM theTCM;

/* a column of the menu, one per depth (OVL1 0x413040, hud_tcmcmds.cpp's: its initialiser 0x3FF430
 * constructs the eight, 0x231A10 their constructor): which way it slides, 1 in, -1 out */
class CTCMColumn {
public:
    CTCMColumn() : m_dir(1.0f), m_4(0) {}

    float m_dir;   // 0x0
    int m_4;   // 0x4
};

extern CTCMColumn g_tcmColumns[8];

/* the tree's (0x231A30-0x23624F, not written yet): the caption cut to a length, the name, the
 * second string, the message delay, the subtree's display */
extern "C" {
void func_00232170(CTCMNode *node, const char *caption, int len);
void func_00232100(CTCMNode *node, const char *name);
void func_002320E0(CTCMNode *node, const char *s);
void func_00232240(CTCMNode *node, float delay);
void func_00231BA0(CTCMNode *node);
void func_002356D0(CTCM *tcm);
void func_00232890(CTCMSounds *sounds, int which);
}

#endif
