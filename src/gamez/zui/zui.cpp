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
 * HandlePad 0x368650 -- SOCOM 1's DWARF has the layout, SOCOM II's grew the vtable pointer at
 * 0x910), the design's loaders, HookupLib 0x36A430 (its CGSTexBuffer local's constructor
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
#include "../ztwod/c2dfont.h"
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
    /* the finished tasks freed (the scheduler's; Clear asks for all of them) */
    void ClearFreeList(bool all);
    /* the tasks run (CGameMenu::Tick's) */
    void Tick(float dT);

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

class CAssetLib;
class CAssetMgr {
public:
    /* a library's reference given back (Clear's, for the menu's libraries) */
    static void ReleaseLibRef(CAssetLib *lib);

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
    /* both lists freed (an inline's out-of-line copy, the world lane's: Clear calls it) */
    void Free();
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
    /* (inline copies the world lane names; Clear calls them out of line) */
    void ClearDynTex();
    void FreePktBuf();

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
    /* the packet buffer wiped (SwitchMenu's, SOCOM 1's) */
    static void WipePktBuf();

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
    unsigned char _c[0x1d - 0xc];
    unsigned char isCdBoot;   // 0x1d: a CD boot (SwitchMenu parks the head before a reboot)
};
extern _zsys_public zSys;

/* the game menu (SOCOM 1's DWARF: 0x890 bytes, no vtable pointer in its members; SOCOM II's members
 * come first and its vtable pointer follows them at 0x910). Only what is written so far is placed */
/* a node of the world (znode.h's zdb::CNode, 0xC0 bytes: only what the menu calls -- its vtable
 * pointer at 0x60, slots as SOCOM II's table 0x4067B0 holds them) and a visual (znode.h's
 * zdb::CVisual: its vtable pointer at 0x20, the 2D visual it is through slot 0x1C; C2DVisual, a 2D
 * object's visual, is 0x70 bytes as the menu news it, its 2D object at 0x68) */
class C2DVisual;
namespace zdb {
class CVisual {
public:
    unsigned char _0[0x20];
    virtual ~CVisual();                              /* +0x08 */
    virtual void Render(CMatrix *mtw);               /* +0x0C */
    virtual void ComputeBBox(void *box);             /* +0x10 */
    virtual void UpdateCentroid();                   /* +0x14 */
    virtual bool Read(void *zar);                    /* +0x18 */
    virtual C2DVisual *Get2DVisual() { return 0; }   /* +0x1C */
    unsigned char _24[0x68 - 0x24];
};

class CNode {
public:
    CNode();
    static CNode *Create(const char *name);
    static CNode *CreateInstance(const char *name, const CPnt3D *pos, const CQuat *rot);
    static void Delete(CNode *node);
    bool AddVisual(CVisual *visual);
    CVisual *GetFirstVisual();
    void SetDynamicLight(bool on, bool children);
    void SetDescendantVisuals(bool on);
    void SetRotation(const CPnt3D *rot);
    void SetPosition(float x, float y, float z);
    void SetName(const char *name);

    unsigned char _0[0x5c];
    unsigned int m_active : 1;   // 0x5c (znode.h's)
    unsigned int m_5c : 31;
    virtual ~CNode();                                     /* +0x08 */
    virtual CNode *_Copy(CNode *to) const;
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);                  /* +0x18 */
    virtual int DeleteChild(CNode *child);
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);   /* +0x24 */
    virtual CNode *FindChild(CNode *node, bool deep);     /* +0x28 */
    virtual CNode *FindChild(const char *name, int start, int *index, bool deep);
    virtual bool IsCell();
    virtual bool Read(void *io);
    virtual bool SetActive(bool on);                      /* +0x38 */
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

    C2D *m_c2d;   // 0x68
    zdb::CTextureRelocMgr *m_mgr;   // 0x6c
};

class CGameMenu {
public:
    /* the menu an rdr names switched to (SOCOM 1's SwitchMenu; SOCOM II's fades first, and goes to a
     * game state for five names) */
    void SwitchMenu(char *name);
    /* the background movie (SOCOM 1's TickMPEG/RestartBackgroundMPEG/HaltBackgroundMPEG, SOCOM II's
     * restart taking a name) */
    bool TickMPEG();
    bool RestartBackgroundMPEG(const char *name);
    bool HaltBackgroundMPEG();
    /* the button a node names made the active one, and the previous or next one that takes input
     * (SOCOM 1's; SOCOM II's Prev and Next take whether to play the selection sound) */
    void MakeButtonActive(const char *name);
    void MakePrevButtonActive(bool sound);
    void MakeNextButtonActive(bool sound);
    /* the actions on an event run, each with the argument given or its own (SOCOM 1's; SOCOM II
     * looks the animation up twice) */
    bool RunEventAnim(ActionItem::EVENT event, const char *arg);
    /* (SOCOM 1's: 0x369B60, 0x3698C0) */
    CGameMenu();
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
    /* the menu's frame and the pad's input (SOCOM 1's) */
    void Tick(float dT);
    void HandlePad(const CPad *pad);
    /* the pop-up is showing (ours: the tick holds the keyboard's input while it is) */
    bool PopupShowing() { return m_popupnode && m_popupnode->m_active; }
    /* a design object's control made in the menu's font, with a node of its own under the node it
     * names (else the backplane) (0x3696D0, ours) */
    C2D *Add2dNode(CDesignDlgObj *obj);

    zdb::CNode m_nodeTop;         // 0x0 (SOCOM 1's DWARF, as the members to 0x210)
    CZAnimZAR *m_zar;             // 0xc0 (SOCOM 1's at 0xc0 too)
    zdb::CNode *m_pBackplaneNode;   // 0xc4
    zdb::CNode *m_pForeplaneNode;   // 0xc8
    zdb::CNode *m_popupnode;      // 0xcc
    CPlainBmp m_bmpd0;            // 0xd0 (the names to m_font ours, by the constructor's calls)
    CPlainBmp m_fader;            // 0x170: black, 640 by 448, half opaque
    C2DFont m_font;               // 0x210: the menu's font (SOCOM 1's 0x30 bytes, SOCOM II's 0x64)
    zdb::CAssetLib *m_274;        // 0x274: a library the menu holds (Clear gives it back)
    bool m_movierunning;          // 0x278 (SOCOM 1's at 0x244)
    bool m_has_mpeg_background;   // 0x279
    bool m_loopMPEG;              // 0x27a: SOCOM II's (TickMPEG loops the movie when set; the name ours)
    CSaferStr m_MPEGName;         // 0x27c
    C2DButton m_buttons;          // 0x290: the menu's buttons, as children (MakeButtonActive walks them)
    int m_activeButton;           // 0x6d0
    C2D m_6d4;                    // 0x6d4
    std::vector<zdb::CAssetLib *> m_libs;   // 0x6ec (reserved 32): the libraries the menu holds (Clear gives them back, last first)
    ActionItem m_commands[30];    // 0x6f8 (SOCOM 1's at 0x68c)
    int m_numCommands;            // 0x8d8
    zdb::CTextureRelocMgr *m_Reloc;   // 0x8dc
    CSaferStr m_ReaderName;     // 0x8e0: the menu's rdr
    std::vector<zdb::CNode *> m_modelNodes;   // 0x8ec: the design's models' nodes (PrepModels; SOCOM 1's at 0x878)
    bool m_8f8;                   // 0x8f8
    int m_fadeSteps;              // 0x8fc: -1, or the fade's steps left before SwitchMenu switches (the name ours)
    float m_900;                  // 0x900: a time the pad is ignored for (the tick counts it down)
    std::vector<CSaferStr> m_904; // 0x904

    virtual ~CGameMenu();
    virtual void MyTick(float dT);
    virtual const char *GetReaderFileName();
    /* slot 0x14: whether the menu holds a soft keyboard (ours: Clear forgets the keyboards' display
     * when it does) */
    virtual bool Slot14();
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
    unsigned char _38[0x60 - 0x38];
    float m_60;               // 0x60: the foreplane sits 1 nearer (the menu's constructor)
    unsigned char _64[0x7d - 0x64];
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

/* an animation set (zanim.h's CZAnimSet: only what Clear calls) */
class CZAnimSet {
public:
    void Stop();
    void Unload();
};

class CZAnimMain {
public:
    CZAnimSet *GetAnimSet(const char *name);
    CZAnim *GetAnim(const char *name, const char *set);
};

extern CZAnimMain ZAnim;

/* a sound (zsnd.h's CSnd: its lookup by name, and its table to the one-shot play at +0x1C) */
class CSndInstance;
class CSnd {
public:
    static CSnd *GetSoundByName(const char *name);

    virtual ~CSnd();
    virtual CSndInstance *PlaySound(CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVol(float vol, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound3D(const CPnt3D *pos, float vol, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVolPanPitch(float vol, int pan, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound(bool oneShot, CSndInstance *inst, unsigned int flags);
};

/* a game state, and the game's state switches (fts_state.h's CGameState and CGame: opaque here) */
class CGameState;
class CGame {
public:
    void Switch(CGameState *state, unsigned int flags);
    void Pop(unsigned int flags);
};

/* the game, as far as the menus' sounds read it (fts_state.h's COurGame: the menu state at 0x78,
 * its teletype sound's name at 0x14 there, its selection sound at 0x2C), and the states SwitchMenu
 * goes to by name (the menu state, the load state at 0xEC and the shutdown state at 0x298; ours) */
class COurGame : public CGame {
public:
    CGameState *MenuState() { return (CGameState *)((char *)this + 0x78); }
    CGameState *LoadState() { return (CGameState *)((char *)this + 0xec); }
    CGameState *ShutdownState() { return (CGameState *)((char *)this + 0x298); }

    unsigned char _0[0x8c];
    CSaferStr m_ttySoundString;   // 0x8c: m_menuState.TtySoundString
    unsigned char _98[0xa4 - 0x98];
    CSnd *m_selectionSound;       // 0xa4: m_menuState.SelectionSound
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

/* the menu's node table (SOCOM 1's RegisterMenuNode / UnRegisterMenuNode: zanim_menu.cpp's) and the
 * menu being built (UiGetCurMenu's static, 0x49EA90) */
void RegisterMenuNode(zdb::CNode *node);
void UnRegisterMenuNode();
extern CGameMenu *data_0049EA90;
extern unsigned char g_hasAnims;

/* the menu built: its nodes, its two bitmaps, its font, its button parent, its commands, its
 * strings and vectors, the backplane and foreplane under its node (the foreplane 1 nearer than the
 * video's 0x60), registered, the fader black and half opaque (SOCOM 1's) */
CGameMenu::CGameMenu() : m_popupnode(0), m_274(0), m_movierunning(false), m_has_mpeg_background(false),
                         m_8f8(false), m_fadeSteps(-1), m_900(0)
{
    data_0049EA90 = this;
    m_MPEGName.reserve(64);
    m_ReaderName.reserve(64);
    m_pBackplaneNode = zdb::CNode::Create("CGameMenu backplane");
    m_pForeplaneNode = zdb::CNode::Create("CGameMenu foreplane");
    m_nodeTop.AddChild(m_pBackplaneNode);
    m_nodeTop.AddChild(m_pForeplaneNode);
    m_pForeplaneNode->SetPosition(0.0f, 0.0f, zVid.m_60 - 1.0f);
    RegisterMenuNode(&m_nodeTop);
    m_fader.SetHasTexture(false);
    m_fader.Load(0.0f, 0.0f, 640.0f, 448.0f, 0);
    m_fader.SetColor(0.0f, 0.0f, 0.0f);
    m_fader.SetTrans(128.0f);
    m_numCommands = 0;
    m_activeButton = 0;
    g_hasAnims = 0;
    m_modelNodes.reserve(64);
    m_libs.reserve(32);
}

/* the menu emptied and unregistered, no menu being built, the CRTC flag set (SOCOM 1's) */
CGameMenu::~CGameMenu()
{
    Clear();
    UnRegisterMenuNode();
    data_0049EA90 = 0;
    zVid.pcrtcDo = 1;
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
    C2D *ctrl = obj->m_spec->CreateDisplayObj(&m_font, m_Reloc);
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

/* the menus' pop-up stack (zanim_menu.h's CMenuStack: its entries, the pop-up and the commands the
 * menu's tick hands it), the keyboards' display forgotten (zui_skb.cpp's) and the current menu's node
 * (0x49EA98: zui_skbview.cpp's 0x36A210 sets its opacity; the tick fades it; ours) */
struct CMenuEntry {
    int m_id;             // 0x0
    char m_argname[48];   // 0x4
};
class CMenuStack {
public:
    void CleanPopup();
    void AddToHistory(const char *name);
    /* the pop-up (zanim_menu.h's CPopUpDialog, a CPlainBmp: here its 2D object, whose Tick and
     * HandlePad the menu calls) */
    C2D *GetPopUp();
    void HandleCommand(const char *name);
    /* the top entry (ours: the tick reads its command and argument through it) */
    CMenuEntry *Top() { return &m_entries[m_top]; }

    CMenuEntry m_entries[40];   // 0x0
    int m_count;                // 0x820
    int m_top;                  // 0x824
};
CMenuStack *GetMenuStack();
void InvalidateSkbsDisplay();
extern zdb::CNode *data_0049EA98;
/* the pads (zinput.h's CInput: its pads only) and the soft keyboard's tick (zui_skb.cpp's) */
#ifndef ZINPUT_H
class CInput {
public:
    static CPad *m_pads[2];
};
#endif
void SkbTick(CGameMenu *menu, bool input, float dT);

/* a child taken off a 2D object's list (ours: Clear erases through it, the erase's temporaries a
 * level deeper than the walk's) */
static inline std::list<C2D *>::iterator EraseChild(std::list<C2D *> &l, std::list<C2D *>::iterator i)
{
    return l.erase(i);
}

/* the menu emptied: the pop-up off, the backplane and foreplane made again, the menu's GS buffer,
 * font and libraries freed, its animations unloaded, its buttons and its 2D objects deleted, the
 * first ten actions' strings freed, the movie ended, the models deleted, the scheduler and the UI
 * variables collected (0x369210: SOCOM 1's Clear, its font and buffer vectors' swaps SOCOM II's
 * Unload and the buffer's frees; the animations and the keyboard test SOCOM II's) */
void CGameMenu::Clear()
{
    m_900 = 0;
    if (m_popupnode) {
        m_popupnode->SetActive(false);
        m_popupnode = 0;
    }
    GetMenuStack()->CleanPopup();
    m_nodeTop.DeleteChild(m_pBackplaneNode);
    delete m_pBackplaneNode;
    m_pBackplaneNode = 0;
    m_nodeTop.DeleteChild(m_pForeplaneNode);
    data_0049EA98 = 0;
    if (Slot14())
        InvalidateSkbsDisplay();
    delete m_pForeplaneNode;
    m_pForeplaneNode = 0;
    m_pBackplaneNode = zdb::CNode::Create("CGameMenu backplane");
    m_pForeplaneNode = zdb::CNode::Create("CGameMenu foreplane");
    m_nodeTop.AddChild(m_pBackplaneNode);
    m_nodeTop.AddChild(m_pForeplaneNode);
    m_pForeplaneNode->SetPosition(0.0f, 0.0f, zVid.m_60 - 1.0f);
    zdb::CGSTexBuffer *buf = zdb::CTexManager::m_texmanager.m_buffers[0];

    buf->ClearDynTex();
    buf->m_dyntex_list.Free();
    buf->FreePktBuf();
    m_font.Unload();
    if (m_274)
        zdb::CAssetMgr::ReleaseLibRef(m_274);
    m_274 = 0;
    for (int k = m_libs.size() - 1; k >= 0; k--)
        zdb::CAssetMgr::ReleaseLibRef(m_libs[k]);
    m_libs.clear();
    if (g_hasAnims) {
        g_hasAnims = 0;
        for (std::vector<CSaferStr>::iterator a = m_904.begin(); a != m_904.end(); ++a) {
            CZAnimSet *set = ZAnim.GetAnimSet((*a).c_str());

            if (set) {
                set->Stop();
                set->Unload();
            }
        }
        m_904.clear();
    }
    std::list<C2D *>::iterator i, next;

    for (i = m_buttons.begin(); i != m_buttons.end(); i = next) {
        C2D *c = *i;

        next = EraseChild(m_buttons, i);
        delete c;
    }
    for (i = m_6d4.begin(); i != m_6d4.end(); i = next) {
        C2D *c = *i;

        next = EraseChild(m_6d4, i);
        delete c;
    }
    for (int j = 0; j < 10; j++) {
        if (m_commands[j].Animation)
            _free(m_commands[j].Animation, 0, 0);
        m_commands[j].Animation = 0;
        if (m_commands[j].Argument)
            _free(m_commands[j].Argument, 0, 0);
        m_commands[j].Argument = 0;
    }
    m_numCommands = 0;
    m_activeButton = 0;
    if (m_has_mpeg_background && m_movierunning)
        CMPEG::End();
    m_movierunning = false;
    for (int n = 0; n < m_modelNodes.size(); n++)
        zdb::CNode::Delete(m_modelNodes[n]);
    m_modelNodes.clear();
    s_countSound = 0;
    zUIScheduler.ClearFreeList(true);
    theUIVarManager.CollectGarbage();
    m_has_mpeg_background = false;
}

/* a pad (zinput.h's CPad, which this unit cannot include beside zui_spec.h: each button's state and
 * the sticks only) */
#ifndef ZINPUT_H
class CPad {
public:
    unsigned char m_0;                        // 0x0
    unsigned char m_state[PAD_MAX_BUTTONS];   // 0x1: KEY_STATE each
    unsigned char _11[0x220 - 0x11];
    float m_220[4];                           // 0x220: the left stick's x and y first
};
#endif
/* the soft keyboard's pad input taken (0x38DC00, a draft) */
extern "C" int func_0038DC00(const CPad *pad, CGameMenu *menu);
/* a key read this frame (zin_keyboard.cpp's KeyInput, 32 bytes) and its reader */
struct KeyInput {
    unsigned char code;
    int shift;
    int ctrl;
    int alt;
    int _10;
    int caps;
    int _18[2];
};
int GetKeyReadThisFrame(KeyInput *key);

/* the pad's input: the stick's centring and position kept, the pop-up's or the keyboard's or the
 * buttons' handling first, then the first button action whose button is in its state -- or, for
 * the four direction buttons falling, whose stick push came -- started, else the stick or the
 * pad's up and down stepping the active button, and a key 40 or 88 running event 44 (0x368650:
 * SOCOM 1's HandlePad; the stick's directions as buttons and the key SOCOM II's).
 * OPEN, 72 instructions at the target's size (0xB20; 334 before the button tests became bools of
 * their own, computed ahead of the four stick reads as the target computes them): the target's
 * loop hoists -s_fStickX into f6 beside -s_fStickY (f4), so each inlined StickAbs(x) takes the
 * negation with a mov.s and x sits in f7; ours hoists only -y (GetStickDir's) and negates x in the
 * branch, which moves x to f6 and reorders the preamble's bools. Tried: StickAbs as `f < 0 ? -f :
 * f` (0xAE0), with the negation in a local (0xB18 / 0xAD8 / 0xB28: the copies outside the loop
 * negate in the branch, as ours), DidStickMovePrev/Next for two of the reads, a dead `-x` local, the
 * item through a pointer, the count in a local, the type read through the array -- none hoists -x */
void CGameMenu::HandlePad(const CPad *pad)
{
    s_bStickCentered = IsStickNearCenter(s_bStickCentered ? 0.6f : 0.2f);
    s_fStickX = pad->m_220[0];
    s_fStickY = pad->m_220[1];
    if (m_popupnode && m_popupnode->m_active) {
        GetMenuStack()->GetPopUp()->HandlePad(pad, 0);
        return;
    }
    if (Slot14() && func_0038DC00(pad, this))
        return;
    if (m_buttons.HandlePad(pad, m_activeButton))
        return;
    for (int i = 0; i < m_numCommands; i++) {
        ActionItem &item = m_commands[i];

        if (item.Type == ACTION_TYPE_BUTTON) {
            bool isUp = item.PadButton == PAD_UP;
            bool isDown = item.PadButton == PAD_DOWN;
            bool isRight = item.PadButton == PAD_RIGHT;
            bool falling = item.State == KEY_FALLING;
            bool pressed = item.State == pad->m_state[item.PadButton];
            bool isLeft = item.PadButton == PAD_LEFT;
            bool up = UiGetDirStickMoved() == STICK_UP;
            bool down = UiGetDirStickMoved() == STICK_DOWN;
            bool right = UiGetDirStickMoved() == STICK_RIGHT;
            bool left = UiGetDirStickMoved() == STICK_LEFT;
            bool stickUp = falling && isUp && up;
            bool stickDown = falling && isDown && down;
            bool stickLeft = falling && isLeft && right;
            bool stickRight = falling && isRight && left;

            if (pressed || stickUp || stickDown || stickLeft || stickRight) {
                CZAnim *anim = ZAnim.GetAnim(item.Animation, 0);

                if (!anim)
                    return;
                if (item.Argument)
                    anim->ActivateArg(1, 8, item.Argument, 0);
                else
                    anim->Activate(0);
                return;
            }
        }
    }
    if (pad->m_state[PAD_DOWN] == KEY_FALLING || DidStickMoveNext())
        MakeNextButtonActive(true);
    else if (pad->m_state[PAD_UP] == KEY_FALLING || DidStickMovePrev())
        MakePrevButtonActive(true);
    KeyInput key;

    GetKeyReadThisFrame(&key);
    if (key.code == 40 || key.code == 88)
        RunEventAnim((ActionItem::EVENT)44, 0);
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

/* the next button that takes input made the active one, the selection sound played when the active
 * one changed and a sound is asked for (0x3670D0: SOCOM 1's MakeNextButtonActive, by its place
 * after MakePrevButtonActive and its code; SOCOM II's takes the sound flag, the recursion passing
 * false and playing the sound itself; SOCOM 1's divides signed too). The three Make*s' shapes, by
 * MW's colouring (tools/igreplay.py): ONE iterator walks every time -- initialised from begin(),
 * then assigned, which keeps it in memory, so even the first walk steps through its slot (its
 * unrolled copies each through a slot of their own) -- the end taken into a local first, one
 * counter for both walks (the second walk's range renumbered late: v0/v1/a0 as the target), and
 * the index saved AFTER the first walk: a copy of that walk's hoisted bound, numbered among the
 * loop optimiser's temporaries after the end's (declared ahead of the walk, old and the end trade
 * s1/s2). A plain `return;` after the search makes its empty test jump to that return's `b`
 * instead of the epilogue: the else-if */
void CGameMenu::MakeNextButtonActive(bool sound)
{
    if (m_buttons.size() == 0)
        return;

    std::list<C2D *>::iterator e = m_buttons.end();
    std::list<C2D *>::iterator i = m_buttons.begin();
    int n;

    for (n = 0; n < m_activeButton; n++)
        ++i;

    int old = m_activeButton;

    (*i)->SetNormal();
    m_activeButton++;
    m_activeButton %= (int)m_buttons.size();

    i = m_buttons.begin();
    for (n = 0; n < m_activeButton; n++)
        ++i;

    C2D *button = *i;

    if (!button->CanHandleInput()) {
        for (i = m_buttons.begin(); i != e; ++i) {
            if ((*i)->CanHandleInput()) {
                MakeNextButtonActive(false);
                if (m_activeButton != old && sound) {
                    CSnd *snd = theGame.m_selectionSound;

                    if (snd)
                        snd->PlaySound(true, 0, 255);
                }
                return;
            }
        }
    } else if (m_activeButton != old) {
        button->SetActive();
        if (sound) {
            CSnd *snd = theGame.m_selectionSound;

            if (snd)
                snd->PlaySound(true, 0, 255);
        }
    }
}

/* the previous button that takes input made the active one (0x366D70: SOCOM 1's
 * MakePrevButtonActive, by its place and its code; the sound flag SOCOM II's, as Next's) */
void CGameMenu::MakePrevButtonActive(bool sound)
{
    if (m_buttons.size() == 0)
        return;

    std::list<C2D *>::iterator e = m_buttons.end();
    std::list<C2D *>::iterator i = m_buttons.begin();
    int n;

    for (n = 0; n < m_activeButton; n++)
        ++i;

    int old = m_activeButton;

    (*i)->SetNormal();
    m_activeButton--;
    if (m_activeButton < 0)
        m_activeButton = m_buttons.size() - 1;

    i = m_buttons.begin();
    for (n = 0; n < m_activeButton; n++)
        ++i;

    C2D *button = *i;

    if (!button->CanHandleInput()) {
        for (i = m_buttons.begin(); i != e; ++i) {
            if ((*i)->CanHandleInput()) {
                MakePrevButtonActive(false);
                if (m_activeButton != old && sound) {
                    CSnd *snd = theGame.m_selectionSound;

                    if (snd)
                        snd->PlaySound(true, 0, 255);
                }
                return;
            }
        }
    } else if (m_activeButton != old) {
        button->SetActive();
        if (sound) {
            CSnd *snd = theGame.m_selectionSound;

            if (snd)
                snd->PlaySound(true, 0, 255);
        }
    }
}

/* the button the named node draws made the active one, the active one set back to normal first and
 * the selection sound played, and its place in the menu's buttons kept (0x366B60: SOCOM 1's
 * MakeButtonActive, by its place after HaltBackgroundMPEG and its code) */
void CGameMenu::MakeButtonActive(const char *name)
{
    if (m_buttons.size() == 0)
        return;

    std::list<C2D *>::iterator i = m_buttons.begin();

    for (int n = 0; n < m_activeButton; n++)
        ++i;
    (*i)->SetNormal();

    C2D *button;
    std::list<C2D *>::iterator e = m_buttons.end();
    zdb::CNode *node = m_nodeTop.FindChild(name, true);

    if (node) {
        zdb::CVisual *visual = node->GetFirstVisual();

        if (visual) {
            C2DVisual *v2d = visual->Get2DVisual();

            if (v2d)
                button = v2d->m_c2d;
        }
        button->SetActive();

        CSnd *snd = theGame.m_selectionSound;

        if (snd)
            snd->PlaySound(true, 0, 255);
    }

    int n = 0;

    for (i = m_buttons.begin(); i != e; ++i, n++) {
        if (*i == button) {
            m_activeButton = n;
            return;
        }
    }
}

/* the CD's head parked (util_sonyio.cpp's 0x39DFC0: the lowest file sector read, ours) */
extern "C" void func_0039DFC0();
void RebootAfterError();
void Unload_MissionReader();
/* the menu an rdr names switched to, or the game state a name stands for (0x365A00: SOCOM 1's
 * SwitchMenu -- the menu cleared, the packet buffer wiped, the mission reader unloaded, the fader
 * set black and the design loaded inside one batch; SOCOM II's fades first: the first call keeps
 * the name and starts three steps, each later one steps while the fader is opaque, the last
 * switches -- to the load screen, the menu state (popped first, or not), shutdown, a reboot, or the
 * menu) */
void CGameMenu::SwitchMenu(char *name)
{
    if (m_fadeSteps <= -1) {
        if (!strcmp(name, "REBOOT") && zSys.isCdBoot)
            func_0039DFC0();
        m_fadeSteps = 3;
        m_ReaderName = name;
        return;
    }
    if (m_fadeSteps > 0) {
        if (m_fader.GetTrans() == 128.0f || !strcmp(name, "REBOOT")) {
            m_pBackplaneNode->SetActive(false);
            m_fadeSteps--;
        }
        return;
    }
    m_pBackplaneNode->SetActive(true);
    m_fadeSteps = -1;
    if (!strcmp(name, "LOAD_SCREEN")) {
        theGame.Switch(theGame.LoadState(), 0);
        return;
    }
    if (!strcmp(name, "POP_TO_MENU_STATE")) {
        theGame.Pop(0);
        theGame.Switch(theGame.MenuState(), 0);
        return;
    }
    if (!strcmp(name, "MENU_SCREEN")) {
        theGame.Switch(theGame.MenuState(), 0);
        return;
    }
    if (!strcmp(name, "SHUTDOWN")) {
        theGame.Switch(theGame.ShutdownState(), 0);
        return;
    }
    if (!strcmp(name, "REBOOT")) {
        RebootAfterError();
        return;
    }
    Clear();
    zdb::CTexManager::WipePktBuf();
    Unload_MissionReader();
    if (name && strstr(name, "dlgHype"))
        printf("");
    if (name && strstr(name, "dlgNetLogin"))
        printf("");
    m_fader.SetHasTexture(false);
    m_fader.Load(0.0f, 0.0f, 640.0f, 448.0f, 0);
    m_fader.SetColor(0.0f, 0.0f, 0.0f);
    m_fader.SetTrans(128.0f);
    m_Reloc->StartBatchReloc();
    LoadDesign(name, m_zar);
    m_Reloc->EndBatchReloc();
}

/* a node's 2D object, through its first visual (ours: the original leaves the result unset for a
 * node without a 2D visual -- the target calls through whatever the register held, which is what
 * keeps each copy's result in a saved register) */
static inline C2D *NodeC2D(zdb::CNode *node)
{
    C2D *c2d;
    zdb::CVisual *visual = node->GetFirstVisual();

    if (visual) {
        C2DVisual *v = visual->Get2DVisual();

        if (v)
            c2d = v->m_c2d;
    }
    return c2d;
}

/* the menu's frame: a fade's switch made once the fader is opaque, the pad read unless a wait runs or
 * a fade does, the step clamped to 1/20 s, the scheduler run, a fade's menu node brought up, the
 * menu stack's commands carried out -- a switch with its history, a switch, a button made active,
 * a control's input taken or given back, each then handed back to the stack -- and the pop-up, the
 * 2D objects, the buttons, the keyboard and the menu's own tick run (0x3654C0: SOCOM 1's Tick; the
 * fade, the wait and the history switch's early return SOCOM II's). The visibility switches call
 * through NodeC2D's unset result when the node has no 2D visual (the target does: each copy's result
 * lives from the entry, so in a saved register); the fade's opacity clamped before the fader's call
 * (the 0.99 the compare made moves in the call's delay slot); each walk's end taken first (the
 * iterator numbered after it: s0 / s1 as the target) */
void CGameMenu::Tick(float dT)
{
    float opacity = data_0049EA98 ? data_0049EA98->m_Opacity : 0.99f;

    if (m_fadeSteps >= 0 && m_fader.GetTrans() == 128.0f) {
        SwitchMenu((char *)m_ReaderName.c_str());
        return;
    }
    if (m_900 > 0.0f)
        m_900 -= dT;
    else if (m_fadeSteps < 0)
        HandlePad(CInput::m_pads[0]);
    if (dT > 0.05f)
        dT = 0.05f;
    if (g_hasAnims)
        zUIScheduler.Tick(dT);
    if (m_fadeSteps >= 0) {
        opacity += 2.0f * dT;
        if (!(opacity < 0.99f)) {
            opacity = 0.99f;
            m_fader.SetTrans(128.0f);
        }
        if (data_0049EA98)
            data_0049EA98->m_Opacity = opacity;
    }
    while (GetMenuStack()->m_count) {
        switch (GetMenuStack()->Top()->m_id) {
        case 2:
            GetMenuStack()->AddToHistory(m_ReaderName.c_str());
            SwitchMenu(GetMenuStack()->Top()->m_argname);
            GetMenuStack()->HandleCommand(m_ReaderName.c_str());
            return;
        case 5:
            SwitchMenu(GetMenuStack()->Top()->m_argname);
            break;
        case 6:
            MakeButtonActive(GetMenuStack()->Top()->m_argname);
            break;
        case 7: {
            zdb::CNode *node = m_nodeTop.FindChild(GetMenuStack()->Top()->m_argname, true);

            if (node)
                NodeC2D(node)->SetCanHandleInput(true);
            break;
        }
        case 8: {
            zdb::CNode *node = m_nodeTop.FindChild(GetMenuStack()->Top()->m_argname, true);

            if (node)
                NodeC2D(node)->SetCanHandleInput(false);
            break;
        }
        }
        GetMenuStack()->HandleCommand(m_ReaderName.c_str());
    }
    GetMenuStack()->GetPopUp()->Tick(dT);
    for (std::list<C2D *>::iterator e = m_6d4.end(), i = m_6d4.begin(); i != e; ++i)
        (*i)->Tick(dT);
    for (std::list<C2D *>::iterator f = m_buttons.end(), j = m_buttons.begin(); j != f; ++j)
        (*j)->Tick(dT);
    SkbTick(this, !PopupShowing(), dT);
    MyTick(dT);
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
