/* zui.cpp -- the game menu (OVL1 0x365200-0x36AF9F): CGameMenu, the dialog design it loads, the
 * buttons' actions and the menus' stick and sound helpers. The press beta's assert on ActionItem's
 * constructor names the unit; SOCOM 1's demo holds the same run in the same order, from
 * CDblBufTextureRelocator's inline virtuals (the unit's vtable comes first) and CGameMenu::Tick to
 * the ActionItem/CSaferStr/CDesignDlgObj/CDlgDesignModel vector instances closing it, the C2D
 * family's zui_2d.cpp after it. Its free helpers were defined alphabetically (SOCOM 1's and SOCOM
 * II's images hold them in reverse alphabetical order). Defined in reverse of their image order
 * (-inline deferred).
 *
 * OPEN, not written yet: CGameMenu itself (Tick 0x3654C0?, SwitchMenu, LoadFromDesign 0x367950,
 * HandlePad 0x368650, Clear 0x369210, the constructor and destructor -- SOCOM 1's DWARF has the
 * layout, SOCOM II's grew the vtable pointer at 0x910), the design's loaders, MakeButtonActive and
 * its neighbours (0x366B60-0x367430), HookupLib 0x36A430 (its CGSTexBuffer local's constructor
 * 0x357BC0 and Hookup 0x3571D0 are rows outside the lane), 0x369880 (reads a string member of
 * theGame, +0x8C, for a sound), and the vector instances closing the unit (0x36AE50 among the
 * clears). Generated drafts stand for the rest. Round 15 wrote RunEventAnim (0x367810) and
 * CDblBufTextureRelocator's PreGetSprPacket (0x365210), both byte-exact. Round 18 wrote Init
 * (0x368570, SOCOM 1's) and the design loader it calls, LoadDesign (0x367430, ours: the block SOCOM
 * 1's Init and SwitchMenu inlined), whose design's inline constructor and destructor name the vector
 * copies after it (0x3676D0-0x3677F0) -- LoadDesign is byte-exact once 0x22BAD0, the copy of
 * ~vector<CSaferStr> its design's two string vectors are destroyed through (an earlier unit's, kept by
 * the linker; only LoadDesign calls it), carries that name (a row outside the lane: measured with it
 * renamed, 40 relocations resolved). */

#include "zui_2d.h"
#include <stdio.h>
#include <iostream>   /* (its pair opens __sinit_zui.cpp, 0x403620) */

/* the rdr reader (zrdr.h's: the file it returns is a _zrdr, CGameDlgDesign::LoadFromRdr's) */
class CRdrFile : public _zrdr {};
CRdrFile *zrdr_read(const char *name, const char *path, int flags);
void zrdr_free(CRdrFile *file);

class CZAnimZAR;

/* the menus' task scheduler (zanim.h's CSched_Manager, 0x20 bytes: a list of tasks; only its
 * constructor and destructor here, both out of line, 0x34E860/0x34EA70) */
class CSched_Manager {
public:
    CSched_Manager();
    ~CSched_Manager();

    unsigned char _0[0x20];
};

/* the menus' scheduler (SOCOM 1's zUIScheduler, a file static: 0x49EA50) -- __sinit_zui.cpp builds it
 * after <iostream>'s pair and registers its destructor; LoadFromDesign, Clear and 0x3654C0 run it */
static CSched_Manager zUIScheduler;

/* the batch relocator the menus prepare their controls with (SOCOM 1's g_relocBatch, 0x49EAA8: only
 * its vtable pointer, stored by __sinit_zui.cpp; no destructor registered) */
zdb::CBatchRelocator g_relocBatch;

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

/* a texture: its size and palette (SOCOM 1's DWARF, znode.h's CTexture; only what the relocator
 * reads), and the GS memory textures may use (its statics) */
class CTexPalette;
class CTexture {
public:
    static unsigned int m_baseAddress;
    static unsigned int m_endAddress;

    unsigned short m_width;              // 0x0
    unsigned short m_height;             // 0x2
    unsigned int m_size;                 // 0x4
    unsigned int m_gsaddr;               // 0x8
    unsigned int m_texelBitSize : 8;     // 0xc
    unsigned int m_flags : 24;           // 0xc
    unsigned char _10[0x1c - 0x10];
    CTexPalette *m_palette;              // 0x1c
};

/* a GS buffer's textures and palettes (SOCOM 1's DWARF: two pointer vectors) */
class CDynTexList {
public:
    void Add(CTexPalette *pal, bool b);
    void Add(CTexHandle *tex, bool b);
    /* (ours: both lists emptied) */
    void Clear()
    {
        m_tex_list.clear();
        m_pal_list.clear();
    }

    std::vector<CTexHandle *> m_tex_list;    // 0x0
    std::vector<CTexPalette *> m_pal_list;   // 0xc
};

/* a GS buffer (SOCOM 1's DWARF, 0x40 bytes; only what the relocator reads) */
class CGSTexBuffer {
public:
    CGSTexBuffer();
    ~CGSTexBuffer();
    void Load(bool b);
    void Hookup(CAssetLib *lib);

    unsigned char _0[0x18];
    void *m_chainp;                  // 0x18
    unsigned char _1c[0x28 - 0x1c];
    CDynTexList m_dyntex_list;       // 0x28
};   // 0x40 bytes

/* a library (znode.h's CAssetLib: only its texture and palette lists) */
class CAssetLib {
public:
    unsigned char _0[0x14];
    std::vector<CTexHandle *> m_textures;     // 0x14
    std::vector<CTexPalette *> m_palettes;    // 0x20
};

/* the texture manager: its GS buffers first (SOCOM 1's; only what the relocator reads) */
class CTexManager {
public:
    int doAddBuffer(const char *name, float start, float end);

    std::vector<CGSTexBuffer *> m_buffers;   // 0x0

    static CTexManager m_texmanager;
};
}

/* the menus' texture relocator (SOCOM 1's DWARF: two GS buffers, 0x1C bytes; SOCOM II's has three,
 * from 0x8) */
class CDblBufTextureRelocator : public zdb::CTextureRelocMgr {
public:
    void Init();
    void PreGetSprPacket(zdb::CTexHandle *tex);

    int _iGsBufToFill;                    // 0x4
    zdb::CGSTexBuffer *_arrGsBufs[3];     // 0x8
    zdb::CTexHandle *_arrLastTex[3];      // 0x14: SOCOM II's, each buffer's last texture (the name ours)
    zdb::CTexPalette *_pSavedPalette;     // 0x20 (SOCOM 1's at 0x18)
};

/* the DMA channels (zsys_public.h's _zsys_public: only the GIF's) */
struct sceDmaChan;
extern "C" int sceDmaSync(sceDmaChan *d, int mode, int timeout);
struct _zsys_public {
    sceDmaChan *DmaToVif0;   // 0x0
    sceDmaChan *DmaToVif1;   // 0x4
    sceDmaChan *DmaToGif;    // 0x8
};
extern _zsys_public zSys;

/* the game menu (SOCOM 1's DWARF: 0x890 bytes, no vtable pointer in its members; SOCOM II's members
 * come first and its vtable pointer follows them at 0x910). Only what is written so far is placed */
/* a node of the world (znode.h's zdb::CNode, 0xC0 bytes: only what the menu calls -- its vtable
 * pointer at 0x60, slots as SOCOM II's table 0x4067B0 holds them) and a visual (znode.h's
 * zdb::CVisual; C2DVisual, a 2D object's visual, is 0x70 bytes as the menu news it) */
namespace zdb {
class CVisual {
public:
    unsigned char _0[0x70];
};

class CNode {
public:
    static CNode *Create(const char *name);
    static CNode *CreateInstance(const char *name, const CPnt3D *pos, const CQuat *rot);
    bool AddVisual(CVisual *visual);
    void SetDynamicLight(bool on, bool children);
    void SetDescendantVisuals(bool on);
    void SetRotation(const CPnt3D *rot);
    void SetPosition(float x, float y, float z);
    void SetName(const char *name);

    unsigned char _0[0x60];
    virtual ~CNode();                                     /* +0x08 */
    virtual CNode *_Copy(CNode *to) const;
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);                  /* +0x18 */
    virtual int DeleteChild(CNode *child);
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);   /* +0x24 */
    virtual CNode *FindChild(CNode *node, bool deep);     /* +0x28 */
    unsigned char _64[0x9c - 0x64];
    float m_Opacity;   // 0x9c
    unsigned char _a0[0xc0 - 0xa0];
};

/* the world (znode.h's zdb::CWorld: only its root, the node models are added under) */
class CWorld : public CNode {
public:
    static CWorld *m_world;
};
}

class C2DVisual : public zdb::CVisual {
public:
    C2DVisual(C2D *c2d, zdb::CTextureRelocMgr *mgr);
};

class CGameMenu {
public:
    /* the background movie (SOCOM 1's TickMPEG/RestartBackgroundMPEG/HaltBackgroundMPEG, SOCOM II's
     * restart taking a name) */
    bool TickMPEG();
    bool RestartBackgroundMPEG(const char *name);
    bool HaltBackgroundMPEG();
    /* the actions on an event run, each with the argument given or its own (SOCOM 1's; SOCOM II
     * looks the animation up twice) */
    bool RunEventAnim(ActionItem::EVENT event, const char *arg);
    /* the menu emptied (0x369210, SOCOM 1's) and built from a design (0x367950, SOCOM 1's) */
    void Clear();
    void LoadFromDesign(const CGameDlgDesign &design, CZAnimZAR *zar);
    /* the design's models put into the world (0x366730, SOCOM 1's) */
    void PrepModels(const CGameDlgDesign &design);
    /* the design an rdr names read and the menu built from it (SOCOM 1's Init and SwitchMenu inlined
     * it; the name ours) */
    void LoadDesign(const char *name, CZAnimZAR *zar);
    /* the menu set up over a relocator, its own rdr's design loaded (SOCOM 1's) */
    bool Init(zdb::CTextureRelocMgr *mgr, CZAnimZAR *zar);
    /* a design object's control made in the menu's font, with a node of its own under the node it
     * names (else the backplane) (0x3696D0, ours) */
    C2D *Add2dNode(CDesignDlgObj *obj);

    zdb::CNode m_nodeTop;         // 0x0 (SOCOM 1's DWARF, as the members to 0x210)
    CZAnimZAR *m_zar;             // 0xc0 (SOCOM 1's at 0xc0 too)
    zdb::CNode *m_pBackplaneNode;   // 0xc4
    zdb::CNode *m_pForeplaneNode;   // 0xc8
    zdb::CNode *m_popupnode;      // 0xcc
    unsigned char _d0[0x210 - 0xd0];
    unsigned char m_font[0x64];   // 0x210: the menu's C2DFont (SOCOM 1's 0x30 bytes, SOCOM II's 0x64)
    unsigned char _274[0x278 - 0x274];
    bool m_movierunning;          // 0x278 (SOCOM 1's at 0x244)
    bool m_has_mpeg_background;   // 0x279
    bool m_loopMPEG;              // 0x27a: SOCOM II's (TickMPEG loops the movie when set; the name ours)
    CSaferStr m_MPEGName;         // 0x27c
    unsigned char _288[0x6f8 - 0x288];
    ActionItem m_commands[30];    // 0x6f8 (SOCOM 1's at 0x68c)
    int m_numCommands;            // 0x8d8
    zdb::CTextureRelocMgr *m_Reloc;   // 0x8dc
    CSaferStr m_ReaderName;     // 0x8e0: the menu's rdr
    std::vector<zdb::CNode *> m_modelNodes;   // 0x8ec: the design's models' nodes (PrepModels; SOCOM 1's at 0x878)
    unsigned char _8f8[0x910 - 0x8f8];

    virtual ~CGameMenu();
    virtual void MyTick(float dT);
    virtual const char *GetReaderFileName();
    virtual void Slot14();
    virtual void PrepControls(zdb::CTextureRelocMgr *mgr);
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

/* the movie player (zmpeg.h's CMPEG: all static) and the DMA FIFO it takes over (zsys_public.h) */
class CMPEG {
public:
    static int Run(const char *name, bool a, bool background, int arg);
    static void Tick(int n);
    static void End();
    /* (bools: TickMPEG tests the one and stores the other unconverted, as SOCOM 1's) */
    static bool Done();
    static bool Loop();
};
void zSysFifoEnd();
void zSysFifoEnable(unsigned int mode);
void zSysFifoDisable();
extern "C" int sceGsSyncPath(int mode, unsigned short timeout);

/* the video state (zvid.h's _zvid_public: the run time a load's message stamps, the page TickMPEG
 * hands the movie, the CRTC flag Init clears) */
struct _zvid_public {
    unsigned char _0[0x20];
    float runTime;            // 0x20
    unsigned char _24[0x34 - 0x24];
    unsigned int renderBuf;   // 0x34
    unsigned char _38[0x7d - 0x38];
    unsigned char pcrtcDo;    // 0x7d
};
extern _zvid_public zVid;

/* the animations (zanim.h's CZAnimMain and CZAnim: only what an action calls; ZAnim OVL1 bss
 * 0x414BB0) */
class CZAnim {
public:
    CZAnim *ActivateArg(int type, ...);
    void Activate(CZAnim *parent);
};

class CZAnimMain {
public:
    CZAnim *GetAnim(const char *name, const char *set);
};

extern CZAnimMain ZAnim;

/* a sound (zsnd.h's CSnd: only its lookup by name) */
class CSnd {
public:
    static CSnd *GetSoundByName(const char *name);
};

/* the game, as far as the menus' sounds read it (fts_state.h's COurGame: the menu state at 0x78,
 * its teletype sound's name at 0x14 there) */
class COurGame {
public:
    unsigned char _0[0x8c];
    CSaferStr m_ttySoundString;   // 0x8c: m_menuState.TtySoundString
};
extern COurGame theGame;

/* the sound PlayCountSound plays (SOCOM 1's gp static; the name ours) */
static CSnd *s_countSound;

/* the flags a design's rdr is read with (SOCOM 1's global, a byte there: 0x49EA70, loaded as a word) */
int LoadDialogsFromReader;

/* the message a design's load prints into (0x49EAB0, 0x100 bytes up to zui_2d.cpp's statics; the
 * name ours) */
static char s_loadMsg[0x100];

/* a dialog design read from the rdr it is named by, through the reader's flags, its message printed
 * first (ours: SOCOM II's, inlined into CGameMenu::LoadDesign) */
static inline bool LoadDlgDesign(CGameDlgDesign &design, const char *name)
{
    sprintf(s_loadMsg, "(%.2f)CGameDlgDesign::LoadFromRdr(): %s\n", zVid.runTime, name);
    CRdrFile *rdr = zrdr_read(name, "data/common/zrdr/dialog", LoadDialogsFromReader);

    if (!rdr)
        return false;
    design.LoadFromRdr(rdr);
    zrdr_free(rdr);
    return true;
}

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

/* a library's palettes and textures hooked up to the GS through a buffer of its own (SOCOM 1's;
 * SOCOM II's lists one word further on) */
void HookupLib(zdb::CAssetLib *lib)
{
    zdb::CGSTexBuffer buf;
    std::vector<zdb::CTexPalette *> &pals = lib->m_palettes;

    for (std::vector<zdb::CTexPalette *>::iterator i = pals.begin(); i != pals.end(); ++i) {
        buf.m_dyntex_list.Add(*i, true);
        buf.m_chainp = 0;
    }
    std::vector<zdb::CTexHandle *> &texs = lib->m_textures;

    for (std::vector<zdb::CTexHandle *>::iterator i = texs.begin(); i != texs.end(); ++i) {
        buf.m_dyntex_list.Add(*i, true);
        buf.m_chainp = 0;
    }
    buf.Hookup(0);
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

/* a copy of another's: its strings duplicated, the button (or event) before the state (SOCOM 1's
 * lines 404-407: each string freed and cleared, then duplicated) */
void ActionItem::operator=(const ActionItem &a)
{
    Type = a.Type;
    if (Animation)
        _free(Animation, 0, 0);
    Animation = 0;
    Animation = a.Animation ? _strdup(a.Animation, 0, 0) : 0;
    if (Argument)
        _free(Argument, 0, 0);
    Argument = 0;
    Argument = a.Argument ? _strdup(a.Argument, 0, 0) : 0;
    PadButton = a.PadButton;
    State = a.State;
}

/* the count sound looked up again by the menu state's teletype sound name (SOCOM II's: CMenuState::
 * Init calls it; the name ours). It takes the menu, unread: the caller loads m_menu into a0 for
 * the call (the game states' round 35). Through a reference: the string's address built once, its
 * words read off it (written out, MW addresses each absolutely) */
void UiLoadCountSound(CGameMenu *)
{
    CSaferStr &name = theGame.m_ttySoundString;

    s_countSound = CSnd::GetSoundByName(name.c_str());
}

/* a design object's control made in the menu's font, given a node of its own ("Add2dNode": the
 * control told of it, its visual added) under the menu's node the object names, else under the
 * backplane, placed at the object's position with its opacity and named after it; the control
 * returned (0x3696D0, ours: SOCOM 1's LoadFromDesign inlined the same block at each of its three
 * places, without the control's node hand-over and the opacity, which SOCOM II's adds). The node
 * found and the parent are two variables, the backplane chosen by a ternary (`if (!parent) parent =
 * ...` on one variable keeps it in a saved register from the search on: 8 bytes short) */
C2D *CGameMenu::Add2dNode(CDesignDlgObj *obj)
{
    C2D *ctrl = obj->m_spec->CreateDisplayObj((C2DFont *)m_font, m_Reloc);
    zdb::CNode *found = *obj->m_childOf.c_str() ? m_nodeTop.FindChild(obj->m_childOf.c_str(), true) : 0;
    zdb::CNode *parent = found ? found : m_pBackplaneNode;
    zdb::CTextureRelocMgr *reloc = m_Reloc;
    zdb::CNode *node = zdb::CNode::Create("Add2dNode");

    ctrl->Slot10(node);
    node->AddVisual(new C2DVisual(ctrl, reloc));
    parent->FindChild(node, true);
    parent->AddChild(node);
    node->SetPosition(obj->m_xpos, obj->m_ypos, 0.0f);
    node->m_Opacity = obj->m_opacity;
    node->SetName(obj->m_name.c_str());
    return ctrl;
}

/* slot 0x10: the menu's rdr (SOCOM 1's) */
const char *CGameMenu::GetReaderFileName()
{
    return m_ReaderName.c_str();
}

/* the menu emptied, set up over the relocator given (else the batch relocator) and its own rdr's
 * design loaded inside one batch (0x368570, SOCOM 1's Init: the load is SOCOM II's LoadDesign) */
bool CGameMenu::Init(zdb::CTextureRelocMgr *mgr, CZAnimZAR *zar)
{
    Clear();
    m_zar = zar;
    zVid.pcrtcDo = 0;
    if (!mgr)
        m_Reloc = &g_relocBatch;
    else
        m_Reloc = mgr;
    m_Reloc->StartBatchReloc();
    LoadDesign(GetReaderFileName(), m_zar);
    m_Reloc->EndBatchReloc();
    return true;
}

/* the actions on an event run (0x367810, SOCOM 1's RunEventAnim): each event action of the event
 * starts its animation with the argument given, else its own, else bare; whether one ran. SOCOM II
 * looks the animation up twice, the first result unused (SOCOM 1 asked the menu's set first) */
bool CGameMenu::RunEventAnim(ActionItem::EVENT event, const char *arg)
{
    bool ran = false;

    for (int i = 0; i < m_numCommands; i++) {
        if (m_commands[i].Type == ACTION_TYPE_EVENT && m_commands[i].Event == event) {
            ZAnim.GetAnim(m_commands[i].Animation, 0);
            CZAnim *anim = ZAnim.GetAnim(m_commands[i].Animation, 0);

            if (anim) {
                if (arg)
                    anim->ActivateArg(1, 8, arg, 0);
                else if (m_commands[i].Argument)
                    anim->ActivateArg(1, 8, m_commands[i].Argument, 0);
                else
                    anim->Activate(0);
                ran = true;
            }
        }
    }
    return ran;
}

/* the design an rdr names read, and once it is the menu's reader is that name and the menu is built
 * from it (0x367430: SOCOM 1's Init and SwitchMenu inlined this; SOCOM II's call it, Init with its
 * own rdr's name) */
void CGameMenu::LoadDesign(const char *name, CZAnimZAR *zar)
{
    CGameDlgDesign design;

    if (!LoadDlgDesign(design, name))
        return;
    m_ReaderName = name;
    LoadFromDesign(design, zar);
}

/* the background movie stopped, if one runs (SOCOM 1's HaltBackgroundMPEG, 0x366B10) */
bool CGameMenu::HaltBackgroundMPEG()
{
    bool halted = false;

    if (m_has_mpeg_background && m_movierunning) {
        CMPEG::End();
        m_movierunning = false;
        halted = true;
    }
    return halted;
}

/* the background movie started again -- stopped first if it runs -- under a new name when one is
 * given (SOCOM 1's RestartBackgroundMPEG, SOCOM II's taking the name: CMenuState::OnPop passes
 * none; 0x366A00) */
bool CGameMenu::RestartBackgroundMPEG(const char *name)
{
    bool running = false;

    if (m_has_mpeg_background) {
        if (m_movierunning)
            HaltBackgroundMPEG();
        if (!m_movierunning) {
            zSysFifoEnd();
            zSysFifoDisable();
            if (name)
                m_MPEGName = name;
            running = CMPEG::Run(m_MPEGName.c_str(), true, false, 0) != 0;
            m_movierunning = running;
            zSysFifoEnable(1);
        }
    }
    return running;
}

/* the background movie ticked while it plays; once done, the menu's event 3 run and the movie
 * looped or ended (SOCOM 1's TickMPEG, which always looped; 0x366940) */
bool CGameMenu::TickMPEG()
{
    if (m_has_mpeg_background && m_movierunning) {
        if (!CMPEG::Done()) {
            unsigned int buf = zVid.renderBuf;

            zSysFifoEnd();
            zSysFifoDisable();
            CMPEG::Tick(buf);
            sceGsSyncPath(0, 0);
            zSysFifoEnable(1);
        } else {
            RunEventAnim(ActionItem::ONMPEGEND, 0);
            if (m_loopMPEG) {
                m_movierunning = CMPEG::Loop();
            } else {
                m_movierunning = false;
                CMPEG::End();
            }
        }
    }
    return m_movierunning;
}

/* the design's models put into the world: each an instance of its model -- by name, or by the
 * string variable its name names ("" for none) -- at the place its azimuth, elevation and distance
 * give, dynamically lit, named, added under the world, its visuals on, turned by its rotation, and
 * kept (SOCOM 1's; 0x366730). The count compared as an int (slt), the "" preset ahead of the
 * variable test (its addiu in that branch's delay slot), the rotation's three floats read into
 * locals z first (the constructor's arguments straight give y and z each other's register) */
void CGameMenu::PrepModels(const CGameDlgDesign &design)
{
    if (!zdb::CWorld::m_world)
        return;
    for (int i = 0; i < (int)design.m_models.size(); i++) {
        const CDlgDesignModel &m = design.m_models[i];
        CPnt3D pos(320.0f + m.m_azimuth, m.m_elev, 320.0f - m.m_distance);
        const char *name = "";

        if (m.m_useModelUIVar) {
            CUIVariable *var = theUIVarManager.Find(m.m_model.c_str());

            if (var)
                name = var->String();
        } else {
            name = m.m_model.c_str();
        }
        zdb::CNode *node = zdb::CNode::CreateInstance(name, &pos, 0);

        if (node) {
            node->SetDynamicLight(true, true);
            node->SetName(m.m_name.c_str());
            zdb::CWorld::m_world->AddChild(node);
            node->SetDescendantVisuals(true);
            float rz = m.m_rotZ, ry = m.m_rotY, rx = m.m_rotX;
            CPnt3D rot(rx, ry, rz);

            node->SetRotation(&rot);
            m_modelNodes.push_back(node);
        }
    }
}

void CDblBufTextureRelocator::Init()
{
    _arrGsBufs[0] = zdb::CTexManager::m_texmanager.m_buffers[zdb::CTexManager::m_texmanager.doAddBuffer("Reloc0", 0.0f, 0.4f)];
    _arrGsBufs[1] = zdb::CTexManager::m_texmanager.m_buffers[zdb::CTexManager::m_texmanager.doAddBuffer("Reloc1", 0.4f, 0.8f)];
    _arrGsBufs[2] = zdb::CTexManager::m_texmanager.m_buffers[zdb::CTexManager::m_texmanager.doAddBuffer("Reloc2", 0.8f, 1.0f)];
}

/* a texture's GS buffer filled before its sprite packet is built: nothing when the texture is one of
 * the three buffers' last, else it goes into buffer 2 -- or, too big for the memory less a
 * megabyte, into the buffer to fill (0 or 1), the fill toggled -- and the buffer is loaded; the GIF is synced
 * unless a texture was loaded on a palette other than the last (SOCOM 1's had two buffers and no
 * cache) */
void CDblBufTextureRelocator::PreGetSprPacket(zdb::CTexHandle *tex)
{
    bool found = false;
    bool samePal = false;

    _pSavedPalette = 0;
    for (int i = 0; i < 3; i++) {
        if (tex == _arrLastTex[i]) {
            found = true;
            break;
        }
    }
    bool load = tex && !found;

    if (load) {
        int idx = 2;
        zdb::CTexture *t = tex->m_texture;

        if ((unsigned int)(t->m_height * t->m_width * t->m_texelBitSize / 8) >
            (zdb::CTexture::m_endAddress - zdb::CTexture::m_baseAddress) * 256 - 0x100000) {
            idx = _iGsBufToFill;
            _iGsBufToFill = !idx;
        }
        zdb::CGSTexBuffer *buf = _arrGsBufs[idx];

        buf->m_dyntex_list.Add(tex, true);
        buf->m_chainp = 0;
        zdb::CTexPalette *pal = tex->m_texture->m_palette;

        if (pal && pal == _pSavedPalette)
            samePal = true;
        _pSavedPalette = pal;
        _arrGsBufs[idx]->Load(true);
        _arrLastTex[idx] = tex;
        buf = _arrGsBufs[idx];
        buf->m_dyntex_list.Clear();
        buf->m_chainp = 0;
    }
    if (!load || samePal)
        sceDmaSync(zSys.DmaToGif, 0, 0);
}
