/* zui.cpp -- the game menu (OVL1 0x365200-0x36AF9F): CGameMenu, the dialog design it loads, the
 * buttons' actions and the menus' stick and sound helpers. The press beta's assert on ActionItem's
 * constructor names the unit; SOCOM 1's demo holds the same run in the same order, from
 * CDblBufTextureRelocator's inline virtuals (the unit's vtable comes first) and CGameMenu::Tick to
 * the ActionItem/CSaferStr/CDesignDlgObj/CDlgDesignModel vector instances closing it, the C2D
 * family's zui_2d.cpp after it. Its free helpers were defined alphabetically (SOCOM 1's and SOCOM
 * II's images hold them in reverse alphabetical order). Defined in reverse of their image order
 * (-inline deferred).
 *
 * OPEN, not written yet: CGameMenu itself (Tick, SwitchMenu, LoadFromDesign 0x367950, HandlePad
 * 0x368650, Clear 0x369210, the constructor and destructor -- SOCOM 1's DWARF has the layout,
 * SOCOM II's grew the vtable pointer at 0x910), the design's loaders, MakeButtonActive and its
 * neighbours, RunEventAnim, GetButton, HookupLib, PrepTexture and CDblBufTextureRelocator's
 * PreGetSprPacket. Generated drafts stand for the rest. */

#include "zui_2d.h"

/* the asset manager's texture lookup (zdb's; SOCOM 1's DWARF layouts) */
namespace zdb {
class CTexHandle {
public:
    char *m_name;              // 0x0
    int m_count;               // 0x4
    unsigned int m_gsAddr;     // 0x8
    CTexture *m_texture;       // 0xc
    unsigned char m_libref;    // 0x10
};   // 0x14 bytes

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);

    unsigned char _0[0x10];
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the game menu (SOCOM 1's DWARF: 0x890 bytes, no vtable pointer in its members; SOCOM II's members
 * come first and its vtable pointer follows them at 0x910). Only what is written so far is placed */
class CGameMenu {
public:
    unsigned char _0[0x8e0];
    CSaferStr m_ReaderName;     // 0x8e0: the menu's rdr
    unsigned char _8ec[0x910 - 0x8ec];

    virtual ~CGameMenu();
    virtual void MyTick(float dT);
    virtual const char *GetReaderFileName();
    virtual void Slot14();
    virtual void PrepControls(zdb::CTextureRelocMgr *mgr);
};

/* a pad button (SOCOM 1's DWARF, zinput.h's) */
enum PAD_BUTTON {
    PAD_START, PAD_SELECT, PAD_RIGHT, PAD_LEFT, PAD_UP, PAD_DOWN, PAD_TRIANGLE, PAD_CIRCLE,
    PAD_SQUARE, PAD_CROSS, PAD_TRIGGER_R1, PAD_TRIGGER_R2, PAD_TRIGGER_L1, PAD_TRIGGER_L2,
    PAD_RIGHT_STICK, PAD_LEFT_STICK, PAD_MAX_BUTTONS
};

/* which way the stick was pushed (ours; the menus' Prev and Next are up and down) */
enum STICK_DIR {
    STICK_NONE = 0,
    STICK_RIGHT = 1,
    STICK_LEFT = 2,
    STICK_UP = 3,
    STICK_DOWN = 4,
    STICK_CENTERED = 5
};

/* the left stick as the menu last read it, -1 to 1 (SOCOM 1's gp statics; the names ours), and
 * whether it has come back to the centre since it last moved */
static float s_fStickX;
static float s_fStickY;
static bool s_bStickCentered = true;

STICK_DIR UiGetDirStickMoved();
bool IsStickNearCenter(float r);
STICK_DIR GetStickDir();

/* (ours: the helpers' magnitude, a ternary) */
static inline float StickAbs(float f)
{
    return f >= 0.0f ? f : -f;
}

/* the stick moved down, once per push (SOCOM 1's) */
bool DidStickMoveNext()
{
    return UiGetDirStickMoved() == STICK_DOWN;
}

/* the stick moved up, once per push (SOCOM 1's) */
bool DidStickMovePrev()
{
    return UiGetDirStickMoved() == STICK_UP;
}

/* the pad button an rdr names, START for none (SOCOM 1's) */
PAD_BUTTON GetButton(const char *name)
{
    if (!name)
        return PAD_START;
    if (!strcmp(name, "START"))
        return PAD_START;
    if (!strcmp(name, "SELECT"))
        return PAD_SELECT;
    if (!strcmp(name, "TRIANGLE"))
        return PAD_TRIANGLE;
    if (!strcmp(name, "CROSS"))
        return PAD_CROSS;
    if (!strcmp(name, "SQUARE"))
        return PAD_SQUARE;
    if (!strcmp(name, "CIRCLE"))
        return PAD_CIRCLE;
    if (!strcmp(name, "UP"))
        return PAD_UP;
    if (!strcmp(name, "DOWN"))
        return PAD_DOWN;
    if (!strcmp(name, "LEFT"))
        return PAD_LEFT;
    if (!strcmp(name, "RIGHT"))
        return PAD_RIGHT;
    if (!strcmp(name, "TRIGGER_L1"))
        return PAD_TRIGGER_L1;
    if (!strcmp(name, "TRIGGER_L2"))
        return PAD_TRIGGER_L2;
    if (!strcmp(name, "TRIGGER_R1"))
        return PAD_TRIGGER_R1;
    return !strcmp(name, "TRIGGER_R2") ? PAD_TRIGGER_R2 : PAD_START;
}

/* the stick's direction, by its larger axis (SOCOM 1's) */
STICK_DIR GetStickDir()
{
    if (s_fStickX < s_fStickY)
        return s_fStickX < -s_fStickY ? STICK_LEFT : STICK_UP;
    return s_fStickX < -s_fStickY ? STICK_DOWN : STICK_RIGHT;
}

/* a texture by name, batched for relocation if it is loaded, or 0 (SOCOM 1's) */
zdb::CTexHandle *PrepTexture(const char *name, zdb::CTextureRelocMgr *mgr)
{
    if (!name)
        return 0;
    zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(name);
    if (!(tex ? tex->m_texture : 0))
        return 0;
    mgr->AddToBatch(tex);
    return tex;
}

/* both axes within r of the centre (SOCOM 1's) */
bool IsStickNearCenter(float r)
{
    bool near = false;

    if (StickAbs(s_fStickX) <= r && StickAbs(s_fStickY) <= r)
        near = true;
    return near;
}

/* the stick's news: back at the centre once it was pushed, or pushed out past 0.6 once it was
 * centred, else nothing (SOCOM 1's) */
STICK_DIR UiGetDirStickMoved()
{
    if (!s_bStickCentered && IsStickNearCenter(0.2f))
        return STICK_CENTERED;
    if (s_bStickCentered && !IsStickNearCenter(0.6f))
        return GetStickDir();
    return STICK_NONE;
}

/* slot 0x10: the menu's rdr (SOCOM 1's) */
const char *CGameMenu::GetReaderFileName()
{
    return m_ReaderName.c_str();
}
