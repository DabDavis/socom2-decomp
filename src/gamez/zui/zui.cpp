/* zui.cpp -- the game menu (OVL1 0x365200-0x36AF9F): CGameMenu, the dialog design it loads, the
 * buttons' actions and the menus' stick and sound helpers. The press beta's assert on ActionItem's
 * constructor names the unit; SOCOM 1's demo holds the same run in the same order, from
 * CDblBufTextureRelocator's inline virtuals (the unit's vtable comes first) and CGameMenu::Tick to
 * the ActionItem/CSaferStr/CDesignDlgObj/CDlgDesignModel vector instances closing it, the C2D
 * family's zui_2d.cpp after it. Its free helpers were defined alphabetically (SOCOM 1's and SOCOM
 * II's images hold them in reverse alphabetical order). Defined in reverse of their image order
 * (-inline deferred).
 *
 * WHOLE since the UI lane's round 31 (every row of 0x365200-0x36AF9F byte-exact): LoadFromDesign
 * 0x367950, the last, written then (its notes). Round 15 wrote RunEventAnim (0x367810) and
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

/* a list of libraries (node_world.h's CAssetList, 0x10 bytes: a pointer list, cleared as it dies,
 * and the model a lookup cached -- its size puts LoadFromDesign's local among the 16-byte slots) */
class CAssetLib;
class CModel;
class CAssetList : public std::list<CAssetLib *> {
public:
    CAssetList();
    ~CAssetList() { Clear(); }
    void Clear();
    CTexHandle *GetTexHandle(const char *name);

    CModel *m_cache_model;   // 0xc
};

class CAssetMgr {
public:
    /* a library's reference given back (Clear's, for the menu's libraries) */
    static void ReleaseLibRef(CAssetLib *lib);
    /* a library by name, or the named ones listed, loaded and referenced (LoadFromDesign's) */
    static CAssetLib *GetLoadedLibRef(const char *name);
    static void GetLoadedLibRefs(const char **names, int n, CAssetList *list);

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

/* a library (znode.h's CAssetLib: only its texture and palette lists and its autoload flag) */
class CAssetLib {
public:
    unsigned char _0[0x14];
    std::vector<CTexHandle *> m_textures;     // 0x14
    std::vector<CTexPalette *> m_palettes;    // 0x20
    unsigned char m_autoload;                 // 0x2c
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

void zSysFifoKick(u_long128 *packet, unsigned int qwc);   /* 0x350AB0 (zsys_public.h's) */

/* the menus' texture relocator (SOCOM 1's DWARF: two GS buffers, 0x1C bytes; SOCOM II's has three,
 * from 0x8) */
class CDblBufTextureRelocator : public zdb::CTextureRelocMgr {
public:
    void Init();
    void PreGetSprPacket(zdb::CTexHandle *tex);
    /* the packet kicked (an inline slot: its copy 0x365200 opens the unit, with the table) */
    void TakeDrawPacket(u_long128 *packet, unsigned int qwc) { zSysFifoKick(packet, qwc); }

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

    unsigned char _0[Z_OFS(0x5c, 0x68)];   /* the host's: after the vtable pointer, tag_NODE_PARAMS at 16 */
    unsigned int m_active : 1;   // 0x5c (host 0x6c) (znode.h's)
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
    float m_Opacity;   // 0x9c (host 0xa8)
    unsigned char _a0[0xc0 - 0xa0];
#if Z_PORT
} __attribute__((aligned(16)));   /* the host's: tag_NODE_PARAMS is 16-aligned, the class 0xd0 bytes */
#else
};
#endif

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
    /* the movie's name kept (ours: LoadFromDesign sets it through this -- assigned in place, its
     * erase()'s tag temporary trades frame slots with the script libraries' string's) */
    void SetMPEGName(const char *name) { m_MPEGName = name; }
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
    /* the menu's planes drawn (SOCOM 1's) */
    void Draw2dOnly();
    void DrawForeplane();
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

/* an animation set (zanim.h's CZAnimSet: what Clear and LoadFromDesign call, its scheduler) */
class CZAnimSet {
public:
    void Stop();
    void Unload();
    int Start();

    unsigned char _0[0x30];
    CSched_Manager *m_task_scheduler;   // 0x30
};

class CZAnimMain {
public:
    CZAnimSet *GetAnimSet(const char *name);
    CZAnim *GetAnim(const char *name, const char *set);
    int LoadZAR(CZAnimZAR *archive, const char *set_name);
    bool Load(const char *name, const char *set);

    unsigned char _0[0x10];
    bool m_LoadFromZAR;   // 0x10
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
    /* (the host's offsets: fts_state.h's class under the Itanium ABI, its CGame 0x80 bytes) */
    CGameState *MenuState() { return (CGameState *)((char *)this + Z_OFS(0x78, 0x80)); }
    CGameState *LoadState() { return (CGameState *)((char *)this + Z_OFS(0xec, 0xf4)); }
    CGameState *ShutdownState() { return (CGameState *)((char *)this + Z_OFS(0x298, 0x2a8)); }

    unsigned char _0[Z_OFS(0x8c, 0x94)];
    CSaferStr m_ttySoundString;   // 0x8c (host 0x94): m_menuState.TtySoundString
    unsigned char _98[Z_OFS(0xa4, 0xac) - Z_OFS(0x98, 0xa0)];
    CSnd *m_selectionSound;       // 0xa4 (host 0xac): m_menuState.SelectionSound
};
extern COurGame theGame;

/* the sound PlayCountSound plays (SOCOM 1's gp static; the name ours) */
static CSnd *s_countSound;

/* the menu's selection sound and the counters' sound, played once (SOCOM 1's; written for the port,
 * M3: their drafts called CSnd's slot 0x1C through MW's vtable layout) */
void PlaySelectionSound()
{
    if (theGame.m_selectionSound)
        theGame.m_selectionSound->PlaySound(true, 0, 0xFF);
}

void PlayCountSound()
{
    if (s_countSound)
        s_countSound->PlaySound(true, 0, 0xFF);
}

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

/* the magnitude of an axis, read straight off the static (ours: a macro, not an inline -- through an
 * inline's parameter the negation is of a copy assigned at each expansion and MW's loop optimiser
 * leaves it in the ternary's arm; on the static itself `-s_fStickX` is invariant and HandlePad's
 * loop hoists it beside GetStickDir's `-s_fStickY`, as the target's f6 / f4) */
#define STICK_ABS(v) ((v) >= 0.0f ? (v) : -(v))

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

    if (STICK_ABS(s_fStickX) <= r && STICK_ABS(s_fStickY) <= r)
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
 * ...` on one variable keeps it in a saved register from the search on: 8 bytes short). Its one
 * caller, LoadFromDesign, calls it out of line (MW takes it whole under -inline auto): pinned */
#pragma auto_inline off
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
#pragma auto_inline reset

#if Z_PORT
/* the port: a child of the menu being shown found by name. UiGetCurMenu's static holds the menu
 * (`data_0049EA90 = this` above), and CHProgressBar::Draw (zui_2d.cpp) searches it as the menu's
 * node: under MW m_nodeTop is at the menu's 0, on the host the vtable pointer is, so the node is
 * reached through the member here, where the real class is */
zdb::CNode *UiCurMenuFindChild(const char *name, bool deep)
{
    return data_0049EA90 ? data_0049EA90->m_nodeTop.FindChild(name, deep) : 0;
}
#endif

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
/* the pop-up (zanim_menu.h's CPopUpDialog, a plain bitmap: the menu calls its Tick and HandlePad and
 * gives it its font) */
class CPopUpDialog : public CPlainBmp {
public:
    /* its textures prepared with the menu's relocator (0x370610, zui_2d.cpp's: SOCOM 1's
     * CPopUpDialog::Load, there called in the same place; the HUD's round 58 named it) */
    bool Load(zdb::CTextureRelocMgr *mgr);

#if Z_PORT
    /* zui_2d.cpp's members up to the font, so the host's offsets follow (MW's padding put the font
     * 0x20 past the class's own, and Load read a null font) */
    zdb::CTextureRelocMgr *m_mgr;
    CWrappedText m_text;
    C2DFont *m_font;
#else
    unsigned char _a0[0x300 - 0xa0];
    C2DFont *m_font;   // 0x300
#endif
};
class CMenuStack {
public:
    void CleanPopup();
    void AddToHistory(const char *name);
    CPopUpDialog *GetPopUp();
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

/* the render pipe, as far as the menu's planes go (zrndr_pipe.cpp's CPipe: its node at 0x630 and
 * its simpletrans flag at 0x711, which fts_state.h calls the UI node and the UI's rendering; at the
 * same offsets on the host -- the class has no virtuals) */
class CPipe {
public:
    void RenderUiNode(zdb::CNode *node);

    unsigned char _0[0x630];
    zdb::CNode *m_uiNode;   // 0x630
    unsigned char _634[0x711 - 0x634];
    unsigned char m_renderingUi;   // 0x711
};
extern CPipe thePipe;

/* the menu's 2D only: its own node drawn as the UI, the backplane the pipe's node (SOCOM 1's;
 * written for the port, M3: its draft passed `this` as the node, MW's m_nodeTop at 0) */
void CGameMenu::Draw2dOnly()
{
    thePipe.m_uiNode = m_pBackplaneNode;
    thePipe.m_renderingUi = 1;
    thePipe.RenderUiNode(&m_nodeTop);
    thePipe.m_renderingUi = 0;
}

/* the foreplane drawn as the UI (SOCOM 1's; written for the port, M3: its draft read MW's 0xC8) */
void CGameMenu::DrawForeplane()
{
    thePipe.m_uiNode = m_pForeplaneNode;
    thePipe.m_renderingUi = 1;
    thePipe.RenderUiNode(m_pForeplaneNode);
    thePipe.m_renderingUi = 0;
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

/* a button's state on the pad (ours: through an inline returning the enum, the state read is the
 * inline's result -- a temporary numbered ahead of the caller's code-generation temporaries, so
 * HandlePad's item loads take t0 / a3 and the state t1, as the target; read in place it is numbered
 * after them) */
static inline KEY_STATE PadButtonState(const CPad *pad, int b)
{
    return (KEY_STATE)pad->m_state[b];
}

/* the pad's input: the stick's centring and position kept, the pop-up's or the keyboard's or the
 * buttons' handling first, then the first button action whose button is in its state -- or, for
 * the four direction buttons falling, whose stick push came -- started, else the stick or the
 * pad's up and down stepping the active button, and a key 40 or 88 running event 44 (0x368650:
 * SOCOM 1's HandlePad; the stick's directions as buttons and the key SOCOM II's).
 * Byte-exact since round 29 (open since round 28 at 72): THE STICK'S MAGNITUDE READS THE STATIC
 * ITSELF (STICK_ABS, a macro) -- through an inline's parameter `-f` negates a copy assigned at each
 * expansion and stays in the ternary's arm, on `s_fStickX` it is a loop invariant MW hoists into f6
 * beside GetStickDir's `-s_fStickY` (f4), each arm then a mov.s; the six button tests declared
 * pressed, falling, up, down, left, right (the target's t7 down to t2: the first declared takes the
 * highest scratch register); the pad's state through an inline returning the enum (PadButtonState:
 * its result numbered ahead of the item loads, `igreplay --move 110=103.5` -- read in place the
 * three loads take t1 / t0 / a3 against the target's t0 / a3 / t1) */
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
            bool pressed = item.State == PadButtonState(pad, item.PadButton);
            bool falling = item.State == KEY_FALLING;
            bool isUp = item.PadButton == PAD_UP;
            bool isDown = item.PadButton == PAD_DOWN;
            bool isLeft = item.PadButton == PAD_LEFT;
            bool isRight = item.PadButton == PAD_RIGHT;
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
        RunEventAnim(ActionItem::USBKBD_ENTER, 0);
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

/* the counters' and teletypes' speeds (zanim_menu.h's, SOCOM 1's), the mission's fonts rdr
 * (zanim_menu.cpp's), the keyboards' display prepared (zui_skb.cpp's) */
void UiSetTtySpeedFactor(float f);
void UiSetCounterSpeedFactor(float f);
_zrdr *FetchMissionFontsRdr();
void PrepSkbsForDisplay(zdb::CNode *parent, zdb::CTextureRelocMgr *mgr, C2DFont *font);

/* a 2D object given a node of its own under the parent ("Add2dNode": the object told of it, its
 * visual added), the node returned (ours: LoadFromDesign's background, fader and pop-up, the
 * arguments loaded parent first) */
static inline zdb::CNode *AddNode(C2D *obj, zdb::CTextureRelocMgr *mgr, zdb::CNode *parent)
{
    zdb::CNode *node = zdb::CNode::Create("Add2dNode");

    obj->Slot10(node);
    node->AddVisual(new C2DVisual(obj, mgr));
    parent->FindChild(node, true);
    parent->AddChild(node);
    return node;
}

/* 0x36AB20: AddNode's out-of-line copy (the soft keyboard's key and text nodes are made through it:
 * zui_skbview.cpp's CSkbKeyProps::PrepForDisplay, zui_skbkeys.cpp's 0x398FC0). Byte-exact (the port's M3: its
 * draft's calls went through MW's vtable layout, the object's table at +0xC, the node's at +0x60) */
extern "C" zdb::CNode *func_0036AB20(C2D *obj, zdb::CTextureRelocMgr *mgr, zdb::CNode *parent)
{
    return AddNode(obj, mgr, parent);
}

/* a design's script library by index, or none (ours) */
static inline const char *ScriptLibrary(const CGameDlgDesign &design, unsigned int i)
{
    return i < design.m_scriptLibraries.size() ? design.m_scriptLibraries[i].c_str() : 0;
}

/* the menu built from a design (0x367950, SOCOM 1's LoadFromDesign): emptied first, the memory card
 * brought up or down as the design uses it (SOCOM II's), the speeds reset, the design's libraries
 * referenced and hooked up (SOCOM II's in one call), the font loaded with its textures batched, the
 * background image placed under the backplane, the actions copied, the fader under the foreplane,
 * the controls (with their children) and the objects made, the controls prepared, the models placed,
 * the fader put back on top, the keyboards and the pop-up prepared, the script libraries' animations
 * loaded and started (else the fader cleared), the first button made active, the background movie
 * started and the count sound looked up.
 * Byte-exact on the UI lane's round 31 (written then; SOCOM 1's 0xBA4 inlined Add2dNode's block
 * three times): Add2dNode pinned out of line; the library walk's iterator ASSIGNED from begin()
 * (kept in memory, a begin() temporary copied in through $f0 -- initialised it lives in a register);
 * the 16-byte CAssetList (its cached model at 0xC) laid among the 16-byte slots, between the name
 * array (12 entries: the script string's temporary sits right above it) and the path; the background,
 * fader and pop-up nodes through one inline taking the parent last (loaded first: arguments right to
 * left); the movie's name through SetMPEGName, a level deeper (assigned in place its erase() tag and
 * the script string's swap sp+780 / sp+784); the active index cleared after the front is read (its
 * store in the null test's delay slot) */
void CGameMenu::LoadFromDesign(const CGameDlgDesign &design, CZAnimZAR *zar)
{
    Clear();
    if (m_8f8 && !design.m_usesMemcard) {
        CMemCard::Uninit();
        m_8f8 = false;
    } else if (!m_8f8 && design.m_usesMemcard) {
        CMemCard::Init();
        m_8f8 = true;
    }
    UiSetCounterSpeedFactor(1.0f);
    UiSetTtySpeedFactor(1.0f);

    const char *names[12];

    for (int i = 0; i < (int)design.m_libraries.size(); i++)
        names[i] = design.m_libraries[i].c_str();
    zdb::CAssetList libs;

    zdb::CAssetMgr::GetLoadedLibRefs(names, design.m_libraries.size(), &libs);
    zdb::CAssetList::iterator it;

    for (it = libs.begin(); it != libs.end(); ++it) {
        zdb::CAssetLib *lib = *it;

        lib->m_autoload = 0;
        HookupLib(lib);
        m_libs.push_back(lib);
    }
    libs.clear();

    const char *font = design.m_font.c_str();

    if (design.m_fontLib) {
        m_274 = zdb::CAssetMgr::GetLoadedLibRef("common/assetlib/font");
        m_274->m_autoload = 0;
    } else
        m_274 = 0;
    m_font.Load(FetchMissionFontsRdr(), font);
    for (int i = 0; i < (int)m_font.m_textures.size(); i++) {
        if (!m_font.GetTexture(i)) {
            printf("*** ERROR *** Cannot find texture for font - %s\n", font);
            printf("*** ERROR *** Recovering - for now\n");
        } else
            m_Reloc->AddToBatch(m_font.GetTexture(i));
    }

    const char *bg = design.m_background.c_str();

    if (design.m_backgroundType == 0) {
        zdb::CTextureRelocMgr *mgr = m_Reloc;
        zdb::CTexHandle *tex = PrepTexture(bg, mgr);

        if (tex ? tex->m_texture : 0) {
            m_bmpd0.Load(0.0f, 0.0f, 640.0f, 448.0f, tex);
            AddNode(&m_bmpd0, mgr, m_pBackplaneNode);
        }
    }
    for (int i = 0; i < (int)design.m_animations.size(); i++) {
        m_commands[i] = design.m_animations[i];
        g_hasAnims = 1;
        m_numCommands++;
    }

    zdb::CNode *fader = AddNode(&m_fader, m_Reloc, m_pForeplaneNode);

    data_0049EA98 = fader;
    fader->SetName("MENUFADER");
    m_fader.SetTrans(128.0f);
    for (int i = 0; i < (int)design.m_controls.size(); i++) {
        CDesignDlgObj *obj = (CDesignDlgObj *)&design.m_controls[i];
        C2D *ctrl = Add2dNode(obj);

        m_buttons.push_back(ctrl);
        CDlgObjSpec *spec = obj->m_spec;

        if (spec->HasChildren()) {
            for (int j = 0; j < spec->NumChildren(); j++)
                ctrl->push_back(Add2dNode(spec->GetChild(j)));
        }
    }
    for (int i = 0; i < (int)design.m_objects.size(); i++)
        m_6d4.push_back(Add2dNode((CDesignDlgObj *)&design.m_objects[i]));
    PrepControls(m_Reloc);
    PrepModels(design);
    m_pForeplaneNode->DeleteChild(data_0049EA98);
    m_pForeplaneNode->AddChild(data_0049EA98);
    if (Slot14())
        PrepSkbsForDisplay(m_pForeplaneNode, m_Reloc, &m_font);

    CPopUpDialog *popup = GetMenuStack()->GetPopUp();

    popup->m_font = &m_font;
    popup->Load(m_Reloc);
    m_popupnode = AddNode(popup, m_Reloc, m_pForeplaneNode);
    m_popupnode->SetName("POPUP");
    m_popupnode->SetActive(false);
    if (g_hasAnims) {
        for (int i = 0; i < (int)design.m_scriptLibraries.size(); i++) {
            const char *name = ScriptLibrary(design, i);
            char path[512];

            sprintf(path, "data/common/zrdr/dialog/%s", name);
            if (zar && ZAnim.m_LoadFromZAR)
                ZAnim.LoadZAR(zar, name);
            else
                ZAnim.Load(path, name);
            CZAnimSet *set = ZAnim.GetAnimSet(name);

            if (set) {
                m_904.push_back(CSaferStr(name));
                set->m_task_scheduler = &zUIScheduler;
                set->Start();
            }
        }
        RunEventAnim(ActionItem::ONSTART, 0);
    } else
        m_fader.SetTrans(0.0f);
    if (m_buttons.size()) {
        C2D *button = m_buttons.front();

        m_activeButton = 0;
        if (!button || !button->CanHandleInput())
            MakeNextButtonActive(false);
        else
            button->SetActive();
    }

    unsigned char type = design.m_backgroundType;

    if (type == 1 || type == 2) {
        bool loop = type == 2;

        SetMPEGName(bg);
        m_has_mpeg_background = true;
        m_loopMPEG = loop;
        zSysFifoEnd();
        zSysFifoDisable();
        m_movierunning = CMPEG::Run(bg, true, false, 0) != 0;
        zSysFifoEnable(1);
    }
    if (m_has_mpeg_background && !m_movierunning)
        RunEventAnim(ActionItem::ONMPEGEND, 0);
    UiLoadCountSound(this);
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

/* an action read from its rdr block: its ANIMATION and ARGUMENT, and a BUTTON with the STATE it
 * waits for (UP, FALLING, DOWN or RISING; FALLING for none or another), or an EVENT by name (0x365D50:
 * SOCOM 1's ParseButtonAnim; SOCOM II's 63 event names, unknown ones ONSTART) */
void ParseButtonAnim(_zrdr *rdr, ActionItem *item)
{
    const char *anim = zrdr_findstring(rdr, "ANIMATION");
    const char *arg = zrdr_findstring(rdr, "ARGUMENT");
    const char *state = zrdr_findstring(rdr, "STATE");

    if (zrdr_findtag(rdr, "BUTTON")) {
        const char *button = zrdr_findstring(rdr, "BUTTON");
        KEY_STATE s;

        item->Type = ACTION_TYPE_BUTTON;
        item->PadButton = GetButton(button);
        if (!state)
            s = KEY_FALLING;
        else if (!strcmp(state, "UP"))
            s = KEY_UP;
        else if (!strcmp(state, "FALLING"))
            s = KEY_FALLING;
        else if (!strcmp(state, "DOWN"))
            s = KEY_DOWN;
        else if (!strcmp(state, "RISING"))
            s = KEY_RISING;
        else
            s = KEY_FALLING;
        item->State = s;
        if (anim)
            item->Animation = _strdup(anim, 0, 0);
        if (arg)
            item->Argument = _strdup(arg, 0, 0);
    } else if (zrdr_findtag(rdr, "EVENT")) {
        const char *name = zrdr_findstring(rdr, "EVENT");
        int e;

        item->Type = ACTION_TYPE_EVENT;
        if (!strcmp(name, "ONSTART"))
            e = 0;
        else if (!strcmp(name, "ONEXIT"))
            e = 1;
        else if (!strcmp(name, "ONACTIVATE"))
            e = 2;
        else if (!strcmp(name, "ONMPEGEND"))
            e = 3;
        else if (!strcmp(name, "MP_JOIN_ROOM"))
            e = 4;
        else if (!strcmp(name, "ON_SKB_APPLY"))
            e = 14;
        else if (!strcmp(name, "ON_SKB_APPLY2"))
            e = 15;
        else if (!strcmp(name, "ON_STATE_POP"))
            e = 13;
        else if (!strcmp(name, "ON_SKB_CLOSE"))
            e = 16;
        else if (!strcmp(name, "MP_PLAYER_NOT_FOUND"))
            e = 5;
        else if (!strcmp(name, "MP_SUCCESS"))
            e = 6;
        else if (!strcmp(name, "MP_CREATE_CLAN_SUCCESS"))
            e = 7;
        else if (!strcmp(name, "MP_CREATE_CLAN_FAILURE"))
            e = 8;
        else if (!strcmp(name, "MP_DISBAND_CLAN_SUCCESS"))
            e = 9;
        else if (!strcmp(name, "MP_DISBAND_CLAN_FAILURE"))
            e = 10;
        else if (!strcmp(name, "MP_MEDIUS_ERROR"))
            e = 11;
        else if (!strcmp(name, "MP_MEDIUS_HAS_INVITES"))
            e = 12;
        else if (!strcmp(name, "ON_LOAD_COMPLETE"))
            e = 17;
        else if (!strcmp(name, "ON_CLIENT_CONNECT"))
            e = 18;
        else if (!strcmp(name, "ON_FORCE_NEW_DB"))
            e = 19;
        else if (!strcmp(name, "ON_LADDER_REFRESH"))
            e = 20;
        else if (!strcmp(name, "ON_TTY_DONE"))
            e = 21;
        else if (!strcmp(name, "MP_MEDIUS_JOIN_GAME_FAILURE"))
            e = 22;
        else if (!strcmp(name, "MP_MEDIUS_READY_TO_CONNECT"))
            e = 23;
        else if (!strcmp(name, "MP_PLAYER_ALREADY_MEMBER"))
            e = 24;
        else if (!strcmp(name, "MP_VERSION_NUMBER_ERROR"))
            e = 25;
        else if (!strcmp(name, "MP_LEADERSHIP_TRANSFER_SUCCESS"))
            e = 26;
        else if (!strcmp(name, "MEMCARD_ERROR"))
            e = 27;
        else if (!strcmp(name, "MP_MEDIUS_BECAME_LEADER"))
            e = 28;
        else if (!strcmp(name, "ON_SKB_CANCEL"))
            e = 29;
        else if (!strcmp(name, "NETCNF_ERROR"))
            e = 30;
        else if (!strcmp(name, "MP_MEDIUS_JOIN_ROOM_FAILURE"))
            e = 31;
        else if (!strcmp(name, "MP_PLAYER_IS_LEADER"))
            e = 32;
        else if (!strcmp(name, "MP_CLAN_DISBANDED"))
            e = 33;
        else if (!strcmp(name, "MP_INVALID_PASSWORD"))
            e = 34;
        else if (!strcmp(name, "MP_PLAYER_ALREADY_INVITED"))
            e = 35;
        else if (!strcmp(name, "MP_PLAYER_LOGGED_IN"))
            e = 36;
        else if (!strcmp(name, "MP_FILTER_FAILURE"))
            e = 37;
        else if (!strcmp(name, "ONFOCUS"))
            e = 43;
        else if (!strcmp(name, "MP_GAME_NAME_EXISTS"))
            e = 38;
        else if (!strcmp(name, "LOADED_FROM_MEMCARD"))
            e = 39;
        else if (!strcmp(name, "SLIDER_MOVED"))
            e = 40;
        else if (!strcmp(name, "ONSCROLL"))
            e = 41;
        else if (!strcmp(name, "ONSCROLL_END"))
            e = 42;
        else if (!strcmp(name, "USBKBD_ENTER"))
            e = 44;
        else if (!strcmp(name, "MP_BANNED_FROM_GAME"))
            e = 45;
        else if (!strcmp(name, "ON_UPDATE_UIVAR"))
            e = 46;
        else if (!strcmp(name, "CLAN_MESSAGE_ID_ERR"))
            e = 47;
        else if (!strcmp(name, "CLAN_MESSAGE_EMPTY"))
            e = 48;
        else if (!strcmp(name, "MP_PLAYER_IS_FRIEND"))
            e = 49;
        else if (!strcmp(name, "MP_FRIEND_CONFIRMATION_REQUESTED"))
            e = 50;
        else if (!strcmp(name, "MP_EJECT_SPECTATOR"))
            e = 51;
        else if (!strcmp(name, "MP_EIGHTY_READY"))
            e = 52;
        else if (!strcmp(name, "MP_DNAS_ERROR"))
            e = 53;
        else if (!strcmp(name, "MP_DOWNLOAD_ERROR"))
            e = 54;
        else if (!strcmp(name, "MP_MEDIUS_FILE_DOES_NOT_EXIST"))
            e = 55;
        else if (!strcmp(name, "MP_CLAN_FULL"))
            e = 56;
        else if (!strcmp(name, "MEMCARD_STATUS"))
            e = 57;
        else if (!strcmp(name, "MP_SYSTEM_MESSAGE"))
            e = 58;
        else if (!strcmp(name, "PLAYER_IS_IGNORED"))
            e = 59;
        else if (!strcmp(name, "MP_RANDOM_GAME_FOUND"))
            e = 60;
        else if (!strcmp(name, "HDD_ERROR"))
            e = 61;
        else if (!strcmp(name, "MP_IGNORE_LIST_FULL"))
            e = 62;
        else
            e = 0;
        item->Event = e;
        if (anim)
            item->Animation = _strdup(anim, 0, 0);
        if (arg)
            item->Argument = _strdup(arg, 0, 0);
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
