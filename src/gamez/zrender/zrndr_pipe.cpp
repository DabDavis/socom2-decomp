/* sc: as=RenderLineWorld__F5PNT4D5PNT4D5PNT4D5PNT4D RenderLineWorld__F6CPnt4D6CPnt4D6CPnt4D6CPnt4D */
/* zrndr_pipe.cpp -- CPipe, the renderer's packet pipe (OVL1 0x336230?-0x33CD4F): what it draws
 * and in which order, the texture loads queued in the GS FIFO between its packets. SOCOM 1's
 * class (its DWARF: 0x694 bytes; SOCOM II's thePipe runs to 0x720); SOCOM 1's file name (its
 * __sinit_zrndr_pipe.cpp builds thePipe as SOCOM II's 0x402E60 does). Defined in reverse of their
 * image order (-inline deferred), on node_world.h's real texture classes.
 * Written so far: the lens flares' system's Tick, Clear, Release and Add (0x336230-0x336650: the unit
 * opens with that system's code, SOCOM II's) and the pipe's TestFlare (0x336810), the opacity stack's
 * push and a node's (PushNodeOpacity, ours), the UI nodes' walk (RenderUiNodeRecursive), the alpha
 * pass's Add and AddLine and its constructor and destructor, the shadow pass (RenderMeshShadow,
 * RenderNodeShadow), the delayed nodes' draw (RenderDelayedNodes), the projected maps' list
 * (BuildProjectedMapList, ours: OPEN), a cell's pass (RenderCell, ours), the alpha pass's textures,
 * the noise overlay (its kick, its texture, its pixels),
 * Flush with the three texture-buffer inlines' copies after it, the flush FIFO packet, the FIFO
 * signal and load (FIFO_Load ours: SOCOM 1's two-argument FIFO_Signal less its argument), VIF1's interrupt, the destructor and the constructor -- whose resize of the
 * delayed nodes brings the template tail (0x33C2A0-0x33CCCF: resize, the deleters' clears and
 * destructors, erase, insert; ~CFlareSystem 0x33CCD0 last). 0x33CD50-0x33D22F is another unit,
 * zrndr_postfx.cpp (CPipeEffect's: the template tail closes this one, and its initialiser 0x402F80
 * follows this unit's).
 * The delayed nodes' insert 0x33C5C0 is byte-exact since include/vector's class insert takes its
 * end as an iterator object (the MSL headers' round 19; 109 instructions off before, the in-place
 * end pointer in s6 against the target's s0);
 * CDynTexList::Free 0x33BAA0 is byte-exact once 0x1FB3D0/0x1FB300 carry vector<CTexHandle *>'s
 * constructor and destructor names (they carry vector<CAssetLib *>'s) and 0x1FDDA0 vector<CTexPalette
 * *>'s destructor -- measured with the map so renamed: the pipe's Free and hud_main.cpp's own copies
 * of both destructors match there, nothing in the HUD, UI or 2D units regresses (the world lane r3's
 * report). */

#include <eetypes.h>
#include <vector>
#include <list>
#include <stack>
#include <iostream>
#include "../zsys/zsys_public.h"
#include "../zsys/zsys.h"
#include "../znode/node_world.h"
#include "../zcamera/zcamera.h"
#include "../zvid/zvid.h"

/* the system's packet ring (zsys_main.cpp): a packet in scratchpad, and the packet kicked */
u_long128 *zSysSprGetPacket(u_long128 **p);
void zSysFifoKick(u_long128 *p, unsigned int qwc);
void *_realloc(void *p, unsigned int size, const char *file, int line);


/* one texture command of the chain the FIFO handler walks (SOCOM 1's tag_TexLoadCmds; SOCOM II
 * keeps a buffer to load and one to signal apart -- the names ours) */
struct tag_TexLoadCmds {
    union {
        int m_cmd;   // 0x0
        struct {
            unsigned char m_flush : 1;   // 0x0: SOCOM II's, set by the empty FIFO packet (ours)
        } m_bits;
    };
    zdb::CGSTexBuffer *m_load;   // 0x4
    zdb::CGSTexBuffer *m_signal;   // 0x8

    void SetFlush(bool on) { m_bits.m_flush = on; }
};   // 0xc bytes

extern "C" void FlushCache(int op);
extern "C" int rand();
/* zdb::CTexture's, not named yet: its colours set (0x354A90, a tail call through its palette) */
extern "C" void func_00354A90(zdb::CTexture *tex, int r, int g, int b);
/* zvid_init.cpp's: the 21-quadword packet's four corners set to one point */
extern "C" void func_003B0CE0(unsigned int x, unsigned int y);
/* a pointer list's clear() out of line: an earlier unit's tail copy (0x1EA580, its row not named
 * yet), which the pipe's draw list links */
extern "C" void func_001EA580(std::list<void *> *list);

/* a node whose drawing waits (SOCOM 1's tag_DelayedNode, which its DWARF leaves empty: 0x70 bytes,
 * the delayed nodes' element -- SaveDelayedNode fills one; the matrix built unit by CMatrix(), the
 * rest left alone, as the resize's temporary shows; copied member by member, the matrix through
 * sceVu0CopyMatrix). SOCOM 1's SaveDelayedNode stores the FOV verdict at 0x48, the pipe's node at 0x4c,
 * the node at 0x50, a point at 0x54, a flag in bit 1 of 0x60 and the opacity at 0x64; the names are
 * ours */
struct tag_DelayedNode {
    CMatrix m_matrix;   // 0x0
    float m_40;   // 0x40
    zdb::CVisual *m_visual;   // 0x44: the visual CAlpha::Add keeps
    zdb::tag_ZVIS_FOV m_fov;   // 0x48 (a byte enum: passed on with lb)
    zdb::CNode *m_atom;   // 0x4c: the pipe's node when saved, its light lists rebuilt when it changes
    zdb::CNode *m_node;   // 0x50
    CPnt3D m_addColor;   // 0x54: CVisual::m_addColor when saved
    unsigned int m_lighting : 1;   // 0x60: CVisual::m_lightingEnable (CAlpha::Add)
    unsigned int m_fog : 1;   // 0x60: the pipe's m_doFog when saved, CVisual::m_fogEnable when added
    unsigned int m_2 : 1;   // 0x60
    unsigned int m_vid : 13;   // 0x60: the view id (CAlpha::Add)
    unsigned int m_flags : 16;   // 0x60
    float m_opacity;   // 0x64
    float m_lightScale;   // 0x68: CVisual::m_GlobalLightScale when saved
};   // 0x70 bytes

/* SOCOM II's effect the pipe keeps at 0x688, with a unit of its own after this one (0x33CD50-
 * 0x33D22F: its constructor 0x33D160 and destructor 0x33D110 called out of line, the unit's
 * initialiser 0x402F80 its iostream statics alone); the name is ours */
class CPipeEffect {
public:
    CPipeEffect();
    ~CPipeEffect();

    unsigned char _0[0x58];   // 0x0: the constructor sets 0x0-0x47
};   // 0x58 bytes (0x6e0 the pipe's next member)

/* a colour and a GS register word the pipe keeps two of at 0x6e0 and 0x6f0 (SOCOM II's; the names
 * ours) */
struct tag_PipeColor {
    unsigned int m_0;   // 0x0
    unsigned int m_rgba;   // 0x4
    u_long m_reg;   // 0x8
};   // 0x10 bytes

/* the cells the world pass walks (RenderWorld pushes GetCell's), a static of this unit built right
 * after its iostream objects; its count after it (the names ours) */
extern std::vector<zdb::CCell *> data_00488790;
/* the lens flares' system (below) */
class CFlareSystem;
extern CFlareSystem data_00488F10;

class CPipe {
public:
    /* the translucent objects drawn last (SOCOM 1's CPipe::CAlpha, 0x30 bytes in both) */
    class CAlpha {
    public:
        CAlpha();
        ~CAlpha();
        /* the draw list emptied and the textures' storage given up (SOCOM II's, inline: Flush
         * inlines it one level and calls both out of line -- SOCOM 1's Flush inlined both. The list's
         * clear is an earlier unit's copy, 0x1EA580, its row not named yet: called by its
         * placeholder; the name ours) */
        void Free()
        {
            func_001EA580(&m_drawList);
            m_dyntexlist.Free();
        }

        /* the next record of the buffer, none once its 2000 are used (SOCOM II's, inline in Add and
         * AddLine: the record handed out is the one after the cursor, the cursor left past it --
         * the name ours) */
        tag_DelayedNode *Next()
        {
            if (m_next >= m_visualList + 2000)
                return 0;
            m_next++;
            return m_next++;
        }
        /* a line kept for the pass (SOCOM II's; the name ours), a record put in the draw list by
         * its range (0x336F60: OPEN, see there) and the pass drawn (0x336CB0) */
        void AddLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1);
        void Add(zdb::CNode *atom, zdb::CNode *node, zdb::CVisual *visual, zdb::tag_ZVIS_FOV fov,
                 float opacity);
        void Insert(tag_DelayedNode *d);
        void Render();

        /* set after each pass, so the next insert starts its walk at the list's start again (SOCOM
         * 1's static m_first_alpha_add, 1 byte: OVL1 0x3DFF68), and the draw list's last insert,
         * where the next one's walk starts (SOCOM 1's m_last_alpha_insert, 4 bytes: OVL1 bss
         * 0x488ED8, just past thePipe's flags) -- declared only: defined here they would join the
         * unit's data */
        static bool m_first_alpha_add;
        static std::list<void *>::iterator m_last_alpha_insert;

        zdb::CCamera *m_camera;   // 0x0
        tag_DelayedNode *m_visualList;   // 0x4
        tag_DelayedNode *m_next;   // 0x8
        std::list<void *> m_drawList;   // 0xc
        zdb::CDynTexList m_dyntexlist;   // 0x18
    };   // 0x30 bytes

    CPipe();
    ~CPipe();
    void Flush();
    void FIFO_Signal(zdb::CGSTexBuffer *buf);
    void FIFO_Load(zdb::CGSTexBuffer *buf);
    void PushOpacity(float opacity);
    void AddAlphaTextures(zdb::CNode *node);
    void KickOverlay();
    void FIFO_Flush();
    void RandomizeOverlay();
    void SetOverlayTexture(zdb::CTexHandle *tex);
    void RenderAtom(zdb::CNode *node);
    void SaveDelayedNode(CMatrix *matrix, zdb::CNode *node, zdb::tag_ZVIS_FOV fov);
    void RenderDelayedNodes();
    void RenderNodeShadow(zdb::CNode *node);
    void RenderMeshShadow(zdb::CNode *node, zdb::CMesh *mesh, CMatrix *matrix);
    bool PushNodeOpacity(zdb::CNode *node);
    bool TestFlare(const CPnt3D *pos, CPnt4D *out, float pull);
    void RenderUiNodeRecursive(zdb::CNode *node);
    /* SOCOM 1's RenderVisual (0x3374C0): whether every visual passed its level of detail */
    bool RenderVisual(zdb::CNode *node, zdb::tag_ZVIS_FOV fov);
    /* RenderAtom's callees (not written): SOCOM II's lighting and fog set-up (0x33B230, ours), SOCOM 1's
     * RenderMesh (0x338480) and RenderNode (0x3389C0) */
    void PrepareAtom(zdb::CNode *node);
    void BuildLightMapList(zdb::CNode *node);
    void RenderMesh(zdb::CNode *node, zdb::CMesh *mesh, CMatrix *matrix, zdb::tag_ZVIS_FOV fov);
    void RenderNode(zdb::CNode *node, zdb::tag_ZVIS_FOV fov);
    void RenderCell(zdb::CCell *cell);
    void BuildProjectedMapList(zdb::CNode *node);
    void BuildSpecMapList(zdb::CNode *node);

    unsigned int m_texIntIdx;   // 0x0
    unsigned int m_texLoadIdx;   // 0x4
    tag_TexLoadCmds m_texLoadChain[128];   // 0x8
    /* SOCOM 1's texture buffers, world, camera, cell, node and light map */
    zdb::CGSTexBuffer *m_assetAlpha;   // 0x608
    zdb::CGSTexBuffer *m_assetCommon;   // 0x60c
    zdb::CGSTexBuffer *m_assetCharacter[2];   // 0x610
    zdb::CGSTexBuffer *m_assetHUD1;   // 0x618
    zdb::CGSTexBuffer *m_assetHUD2;   // 0x61c
    zdb::CGSTexBuffer *m_assetHUDX;   // 0x620
    zdb::CWorld *m_world;   // 0x624
    zdb::CCamera *m_camera;   // 0x628
    zdb::CCell *m_cell;   // 0x62c
    zdb::CNode *m_node;   // 0x630
    zdb::CTexture *m_lightmap;   // 0x634
    CAlpha m_alpha;   // 0x638 (SOCOM 1: 0x654)
    /* the opacities pushed, each the product of those under it (SOCOM 1's at 0x648) */
    float *m_opacity_stack;   // 0x668
    int m_opacity_stack_size;   // 0x66c
    int m_opacity_stack_index;   // 0x670
    std::vector<tag_DelayedNode> m_DelayedNodes;   // 0x674 (SOCOM 1's at 0x684)
    unsigned int m_delayedNodeNum;   // 0x680
    unsigned int _684;   // 0x684
    CPipeEffect m_effect;   // 0x688
    tag_PipeColor m_color0;   // 0x6e0
    tag_PipeColor m_color1;   // 0x6f0
    /* the noise drawn over the frame (SOCOM II's; CCoreState::Init sets "noise50.tif" -- the
     * names ours) */
    unsigned int m_overlayOn;   // 0x700
    unsigned int m_overlayColor;   // 0x704: 0x40808080 at the start
    zdb::CTexHandle *m_overlay;   // 0x708
    /* SOCOM 1's flags (0x638 there), the first two set by the constructor's initialiser list */
    unsigned char m_wireframe;   // 0x70c
    unsigned char m_terrain;   // 0x70d
    /* the rest of SOCOM 1's flags, in its DWARF's order (0x63a-0x645 there): PrepareAtom reads
     * m_drawCharacters and sets m_doFog at SOCOM 1's offsets from m_wireframe */
    unsigned char m_polys;   // 0x70e
    unsigned char m_drawShadows;   // 0x70f
    unsigned char m_drawReflections;   // 0x710
    unsigned char m_simpletrans;   // 0x711
    unsigned char m_drawCharacters;   // 0x712
    unsigned char m_doFog;   // 0x713
    unsigned char m_doUI;   // 0x714
    /* RenderVisual reads +5, +7 and +8 as SOCOM 1's reads m_simpletrans, m_doFog and m_doUI, and
     * clears its result by +9 where SOCOM 1's cleared it by m_LODFilter (+12): SOCOM II's flags
     * hold none of SOCOM 1's m_drawDI, m_drawNormal and m_drawBBox there (round 27) */
    unsigned char m_LODFilter;   // 0x715
    unsigned char _716[0x720 - 0x716];   // 0x716
};   // 0x720 bytes

/* zvid_init.cpp's: the 21-quadword packet's word set, and the packet sent (it reads no argument;
 * the pipe passes one) */
extern "C" void func_003B0D50(unsigned int v);
extern "C" void func_003B0CC0(unsigned int v);

/* the lens flares' system (OVL1 bss 0x488F10, after thePipe; the HUD lane's name, hud_lensflare.cpp
 * calls it): its flares a pointer vector at 4, each with an active bit first (the names ours) */
struct CFlare {
    /* a new flare's three bits set in the initialiser list (a word read-mask-write each) */
    CFlare(const CPnt3D *pos, float size) : m_active(true), m_test(true), m_visible(true)
    {
        m_pos.x = pos->x;
        m_pos.y = pos->y;
        m_pos.z = pos->z;
        m_size = size;
    }
    bool IsActive() const { return m_active; }
    void SetActive(bool on) { m_active = on; }
    void SetVisible(bool on) { m_visible = on; }

    unsigned int m_active : 1;   // 0x0
    unsigned int m_test : 1;   // 0x0: tested against the depth buffer each tick
    unsigned int m_visible : 1;   // 0x0: the last test's verdict
    CPnt3D m_pos;   // 0x4
    float m_depth;   // 0x10: the projected point's w at the last test
    float m_size;   // 0x14: how far the test point is pulled toward the camera
};   // 0x18 bytes
class CFlareSystem {
public:
    /* the byte first cleared, then the vector built (the unit's initialiser; ours) */
    CFlareSystem() : m_0(0) {}
    void Tick();
    void Clear();
    void Release(int handle);
    int Add(const CPnt3D *pos, float size);

    unsigned char m_0;   // 0x0
    std::vector<CFlare *> m_flares;   // 0x4
};

/* the unit's statics, in the order its initialiser (0x402E60) builds and registers them after
 * <iostream>'s two: the cell list, the pipe, a stack that pushes nothing (grid_di.cpp's kind, its
 * destructor's linked copy 0x2D4FE0), the flare system (its implicit destructor the tail's last,
 * 0x33CCD0). The names are ours but thePipe's */
std::vector<zdb::CCell *> data_00488790;
CPipe thePipe;   // 0x4887C0
static CStack s_stack(false);   // 0x488EF8
CFlareSystem data_00488F10;
/* the thread a flushed FIFO wakes (SOCOM II's; not named) */
extern int data_003E1418;

/* SOCOM 1's (0x33C1C0): the first two flags in the initialiser list, after the alpha pass and the
 * effect (both built out of line) and the delayed nodes' vector; the opacity stack of 32 with 1 at
 * its bottom, 48 delayed nodes, no texture commands -- then SOCOM II's two colours and its overlay */
CPipe::CPipe() : m_wireframe(false), m_terrain(true)
{
    m_opacity_stack = (float *)_calloc(sizeof(float), 32, 0, 0);
    m_opacity_stack[0] = 1.0f;
    m_opacity_stack_size = 32;
    m_opacity_stack_index = 0;
    m_DelayedNodes.resize(48);
    m_texIntIdx = 0;
    m_texLoadIdx = 0;
    m_color1.m_0 = 0;
    m_color1.m_rgba = 0x80808080;
    m_color1.m_reg = 0x80000000A8;
    m_color0.m_0 = 0;
    m_color0.m_rgba = 0x80808080;
    m_color0.m_reg = 0x80000000A8;
    m_overlayOn = 0;
    m_overlayColor = 0x40808080;
    m_overlay = 0;
}

/* SOCOM 1's (0x33C120): the opacity stack freed and the delayed nodes cleared; the members' own
 * destructors after (the effect's and the alpha pass's out of line) */
CPipe::~CPipe()
{
    if (m_opacity_stack)
        _free(m_opacity_stack, 0, 0);
    m_DelayedNodes.clear();
}

/* SOCOM 1's: VIF1's interrupt -- the next texture command taken: a flush wakes its thread, a load
 * sends the buffer's chain through DMA channel 2 once the GIF channel is idle and the buffer has
 * packet storage (SOCOM II tests its capacity, SOCOM 1 its size), a signal updates its buffer's
 * selects; VIF1 restarted */
int InterruptFromVIF1(int cause)
{
    if (cause == 5) {
        /* the index a local of its own (the target's registers); the command read through a
         * volatile pointer (the signal reloaded for the call after its test) */
        unsigned int i = thePipe.m_texIntIdx++;
        volatile tag_TexLoadCmds *c = &thePipe.m_texLoadChain[i];

        if (c->m_bits.m_flush)
            iWakeupThread(data_003E1418);
        else {
            if (c->m_load) {
                sceDmaSync(zSys.DmaToGif, 0, 0);
                if (c->m_load->m_pktbuf.capacity()) {
                    *(volatile unsigned int *)0x1000A020 = 0;
                    *(volatile unsigned int *)0x1000A030 = (unsigned int)c->m_load->m_chainp & 0x0FFFFFFF;
                    *(volatile unsigned int *)0x1000A000 = 325;
                    asm volatile { sync }
                }
            }
            if (c->m_signal)
                c->m_signal->UpdateTextureSelects();
            *(volatile unsigned int *)0x10003C10 = 8;
            asm volatile { sync }
        }
    }
    asm volatile { sync }
    asm volatile { ei }
    return 0;
}

/* a buffer uploaded, then a command queued and a FIFO packet kicked that interrupts when the
 * GS reaches it (its marker word 0x44444444). SOCOM 1's FIFO_Signal(CGSTexBuffer *, unsigned int *)
 * (0x2460A0: the same Load, the command's buffer to load set, the one to signal cleared) less its
 * wait-time pointer: no SOCOM II caller sets a2 (CCoreState::Tick's four calls, CMenuState::Tick's
 * one) and the body never reads it, so its signature is FIFO_Signal(CGSTexBuffer *)'s -- which
 * 0x33BE70 keeps, SOCOM 1's one-argument body (the signal-only 0x55555555 packet). So this one was
 * renamed; the name is ours */
void CPipe::FIFO_Load(zdb::CGSTexBuffer *buf)
{
    if (!buf)
        return;
    buf->Load(false);
    tag_TexLoadCmds &c = m_texLoadChain[m_texLoadIdx++];

    c.m_cmd = 0;
    c.m_load = buf;
    c.m_signal = 0;
    unsigned int *p = (unsigned int *)zSysSprGetPacket(0);

    p[0] = 0x10000002;
    p[1] = 0x44444444;
    p[2] = 0x11000000;
    p[3] = 0x50000001;
    ((u_long *)p)[2] = 0x0000400000008000;
    ((u_long *)p)[3] = 0;
    p[8] = 0x13000000;
    p[9] = 0;
    p[10] = 0;
    p[11] = 0x80000000;
    zSysFifoKick((u_long128 *)p, 3);
}

/* SOCOM 1's: a command queued for the buffer and the FIFO packet that interrupts when the GS
 * reaches it (its marker word 0x55555555) */
void CPipe::FIFO_Signal(zdb::CGSTexBuffer *buf)
{
    if (!buf)
        return;
    tag_TexLoadCmds &c = m_texLoadChain[m_texLoadIdx++];

    c.m_cmd = 0;
    c.m_load = 0;
    c.m_signal = buf;
    unsigned int *p = (unsigned int *)zSysSprGetPacket(0);

    p[0] = 0x10000002;
    p[1] = 0x55555555;
    p[2] = 0x11000000;
    p[3] = 0x50000001;
    ((u_long *)p)[2] = 0x0000400000008000;
    ((u_long *)p)[3] = 0;
    p[8] = 0x13000000;
    p[9] = 0;
    p[10] = 0;
    p[11] = 0x80000000;
    zSysFifoKick((u_long128 *)p, 3);
}

/* SOCOM II's: a command queued with its flush bit and an empty FIFO packet kicked that interrupts
 * when the GS reaches it (its marker word 0x66666666, fourteen zero quadwords after the tags) */
void CPipe::FIFO_Flush()
{
    tag_TexLoadCmds &c = m_texLoadChain[m_texLoadIdx++];

    c.SetFlush(true);
    c.m_load = 0;
    c.m_signal = 0;
    unsigned int *p = (unsigned int *)zSysSprGetPacket(0);

    p[0] = 0x1000000F;
    p[1] = 0x66666666;
    p[2] = 0x13000000;
    p[3] = 0;
    p[4] = 0x80000000;
    p[5] = 0;
    p[6] = 0;
    p[7] = 0;
    /* written out: a loop of fourteen is unrolled 7x, not whole */
    u_long128 *q = (u_long128 *)p;
    q[2] = 0;
    q[3] = 0;
    q[4] = 0;
    q[5] = 0;
    q[6] = 0;
    q[7] = 0;
    q[8] = 0;
    q[9] = 0;
    q[10] = 0;
    q[11] = 0;
    q[12] = 0;
    q[13] = 0;
    q[14] = 0;
    q[15] = 0;
    zSysFifoKick((u_long128 *)p, 16);
}

/* SOCOM 1's Flush (0x33B720): the deferred visuals deleted, the delayed nodes' and the alpha pass's
 * storage given up, the two alpha texture buffers emptied (each tested), the visuals' static list
 * flushed, no texture commands; SOCOM II's cell list freed, its flares cleared and its overlay off */
void CPipe::Flush()
{
    zdb::CNode::DeleteDeferredVisuals();
    FreeVector(m_DelayedNodes);
    m_alpha.Free();
    m_assetAlpha = zdb::CTexManager::m_texmanager[zdb::CTexManager::TexPhase[11][0]];
    if (m_assetAlpha)
        m_assetAlpha->Free();
    m_assetAlpha = zdb::CTexManager::m_texmanager[zdb::CTexManager::TexPhase[11][1]];
    if (m_assetAlpha)
        m_assetAlpha->Free();
    zdb::CVisual::Flush();
    m_texIntIdx = 0;
    m_texLoadIdx = 0;
    FreeVector(data_00488790);
    data_00488F10.Clear();
    m_overlayOn = 0;
    m_overlayColor = 0x40808080;
    m_overlay = 0;
}

/* c2dline.cpp's: the line clipped by the camera's planes and the line drawn (SOCOM 1's both) */
int clip3DLine(CPnt4D *planes, CPnt4D *p0, CPnt4D *p1);
void Draw3DLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1);

/* SOCOM 1's: a line in the world clipped to the view and drawn; SOCOM II defers a line either of
 * whose colours is translucent to the alpha pass (the opaque test first: the draw is the
 * fall-through, the alpha pass's call last). The four points are zmath.h's PNT4D by value (the
 * unit's `as=` line gives the map's CPnt4D name): 4-aligned with an implicit copy, the callee
 * copies each into its frame through four FPU registers (lwc1/swc1, as SOCOM 1's 0x245930 does)
 * -- zmath.h's 16-aligned CPnt4D copies with lq/sq (0xD4 against 0x12C), and with zparticle.h's
 * written copy constructor the callee copies nothing (the MSL headers' round 17 survey) */
void RenderLineWorld(PNT4D p0, PNT4D p1, PNT4D c0, PNT4D c1)
{
    if (clip3DLine((CPnt4D *)&zdb::CWorld::m_world->m_camera->m_ClipSet, (CPnt4D *)&p0, (CPnt4D *)&p1)) {
        if (c0.w >= 0.9921875f && c1.w >= 0.9921875f)
            Draw3DLine((CPnt4D *)&p0, (CPnt4D *)&p1, (CPnt4D *)&c0, (CPnt4D *)&c1);
        else
            thePipe.m_alpha.AddLine((CPnt4D *)&p0, (CPnt4D *)&p1, (CPnt4D *)&c0, (CPnt4D *)&c1);
    }
}

/* SOCOM II's: the noise texture's DMA reference kicked through the FIFO, then the overlay's colour
 * and packet sent */
void CPipe::KickOverlay()
{
    if (m_overlayOn && m_overlay) {
        u_long128 *p = zSysSprGetPacket(0);

        *p = m_overlay->m_texture->m_dmaRef;
        zSysFifoKick(p, 1);
        func_003B0D50(m_overlayColor);
        func_003B0CC0(m_overlayOn);
    }
}

/* SOCOM II's: the noise texture taken, the overlay's corners set to its size, its colours set, its
 * pixels randomised (RandomizeOverlay inlined) */
void CPipe::SetOverlayTexture(zdb::CTexHandle *tex)
{
    m_overlay = tex;
    zdb::CTexture *t = tex->m_texture;

    func_003B0CE0(t->m_width, t->m_height);
    func_00354A90(t, 66, 73, 127);
    RandomizeOverlay();
}

/* SOCOM II's: every pixel of the noise texture a new random byte */
void CPipe::RandomizeOverlay()
{
    if (m_overlay) {
        zdb::CTexture *t = m_overlay->m_texture;
        unsigned int n = t->m_width * t->m_height;
        unsigned char *pixels = (unsigned char *)t->m_buffer;

        for (unsigned int i = 0; i < n; i++) {
            float r = 4.656613e-10f * rand();

            pixels[i] = (int)(255.0f * r);
        }
        FlushCache(0);
    }
}

/* the light lists (SOCOM 1's BuildLightMapList is likely 0x339660; neither written) and the
 * world's lighting switch (0x3B7130: a byte set and its inverse beside it), and four of the light
 * pass's counts (not named) */
extern int data_004B4DAC, data_004B4DE4, data_004B4E0C, data_004B4E34;

/* SOCOM II's (the name ours): what SOCOM 1's RenderAtom did between its FOV mask and its draw --
 * the fog flag cleared; a bump-mapped node only while the pass allows it; a landmark gets the
 * plain mask, no lights and the landmark fog, any other node its light lists and the fog */
void CPipe::PrepareAtom(zdb::CNode *node)
{
    m_doFog = false;
    if (node->m_bumpmap && !m_drawCharacters)
        return;
    if (node->m_landmark) {
        data_004B4DE4 = 0;
        data_004B4E34 = 0;
        data_004B4E0C = 0;
        data_004B4DAC = 0;
        m_camera->m_fovMask = 16175;
        m_doFog = node->m_fog ? m_camera->TestLandmarkFOG((const CPnt3D *)node->m_matrix.m_matrix[3], node->GetRadius()) : false;
        zdb::CVisual::LandmarkEnable(!node->m_socom2_5a_bit2);
    } else {
        zdb::CVisual::LandmarkEnable(false);
        BuildLightMapList(node);
        BuildProjectedMapList(node);
        m_doFog = node->m_fog ? m_camera->TestFOG((const CPnt3D *)node->m_matrix.m_matrix[3], node->GetRadius()) : false;
    }
}

/* SOCOM 1's: a node with visuals drawn -- unless it is all but transparent (SOCOM II's test) or
 * clipped to a region the camera cannot see -- its FOV mask chosen by the clip bit, its light maps
 * listed, then its mesh or its visuals rendered (SOCOM II dropped SOCOM 1's fog test) */
void CPipe::RenderAtom(zdb::CNode *node)
{
    m_node = node;
    if (node->m_hasVisuals) {
        if (node->m_Opacity < 0.0078125f)
            return;
        if (node->m_apply_clip && !m_camera->CanSeeRegion(1 << node->m_region_shift))
            return;
        if (!node->m_apply_clip)
            m_camera->m_fovMask = 16175;
        else
            m_camera->m_fovMask = 16191;
        PrepareAtom(node);
        if (node->m_hasMesh)
            RenderMesh(node, 0, 0, (zdb::tag_ZVIS_FOV)0);
        else
            RenderNode(node, (zdb::tag_ZVIS_FOV)0);
    }
}

/* SOCOM II's (the name ours; RenderWorld's three cell passes call it): a cell's fog flag from its
 * centre and the grid's two tolerances, its own visual drawn when it is reflective and its clutter
 * list says so (the type word -3 first), then its clutter */
void CPipe::RenderCell(zdb::CCell *cell)
{
    CPnt3D center;

    cell->GetBBox()->GetCenter(&center);
    m_doFog = m_camera->TestFOG(&center, zdb::CWorld::m_world->m_grid.m_ClutterTol +
                                             zdb::CWorld::m_world->m_grid.m_GridCellTol);
    zdb::CVisual *v = cell->GetFirstVisual();

    if (v && !cell->m_clutterList.m_socom2_18 && cell->m_reflective) {
        v->m_1c = -3;
        v->Render(CStack::m_top);
    }
    cell->RenderClutter(cell);
}

/* the specular fade a light map near enough gets in full (SOCOM II's, the name ours: 1.0 in the
 * image, read with lwc1 -- a variable) */
float s_specFadeNear = 1.0f;   /* 0x3DFF70 */

/* SOCOM II's (the name ours; RenderNode and RenderUiNodeRecursive call it before a node's visuals
 * are drawn, SOCOM 1 has no such list builder): the specular maps listed afresh -- for a node
 * marked to take them, each light map but a shadow within 250 units of the stack top's place (the
 * node's, pushed), its specular fade full within 225 and ramped down to nothing at 250. Each push
 * passes a copy of its own, taken after the fade's store (its stack store then fills Exists' delay
 * slot) while Exists is handed the light itself (a1 straight from s3), as the target's */
void CPipe::BuildSpecMapList(zdb::CNode *node)
{
    zdb::CVisual::m_specMapList.clear();
    if (node->m_socom2_a1_bits.bit7 == 1) {
        CPnt3D pos = *(const CPnt3D *)CStack::m_top->m_matrix[3];
        zdb::CLightMapVector &lights = m_world->m_lightMaps;

        for (zdb::CLightMapVector::iterator i = lights.begin(); i != lights.end(); ++i) {
            zdb::CLightMap *l = *i;

            if (!l->m_isShadow) {
                CPnt3D d = pos - *(const CPnt3D *)l->m_matrix.m_matrix[3];
                float d2 = d.x * d.x + d.y * d.y + d.z * d.z;

                if (d2 <= 50625.0f) {
                    l->m_SpecularFade = s_specFadeNear;
                    zdb::CLightMap *kept = l;

                    if (!zdb::CVisual::m_specMapList.Exists(l))
                        zdb::CVisual::m_specMapList.push_back(kept);
                } else if (d2 < 62500.0f) {
                    l->m_SpecularFade = (62500.0f - d2) * (1.0f / 11875.0f);
                    zdb::CLightMap *kept = l;

                    if (!zdb::CVisual::m_specMapList.Exists(l))
                        zdb::CVisual::m_specMapList.push_back(kept);
                }
            }
        }
    }
}

/* whether a light map reaches a sphere: its range and the sphere's radius against the distance
 * between them -- a shadow only on the side the world's shadow vector leaves lit (SOCOM 1's test,
 * in its BuildLightMapList's own inline; SOCOM II adds the shadow's side; the name ours) */
static inline bool LightReaches(zdb::CLightMap *l, const CPnt3D &center, float r)
{
    CPnt3D d = center - *(const CPnt3D *)l->m_matrix.m_matrix[3];
    float rr = r + l->m_maxRange;

    if (l->m_isShadow) {
        const CPnt3D &v = zdb::CWorld::m_world->m_shadow_vector;

        if (d.x * v.x + d.z * v.z < -r)
            return false;
    }
    if (d.x * d.x + d.y * d.y + d.z * d.z < rr * rr)
        return true;
    return false;
}

/* whether a light map's instance is the node's model -- the node itself when it is one, none
 * otherwise (ours: SOCOM 1's BuildLightMapList compares in place; SOCOM II's numbering needs the
 * instance as an inline's parameter, below) */
static inline bool IsModelOf(zdb::CNode *inst, zdb::CNode *node)
{
    return inst == (node->m_type == 2 ? node : 0);
}

/* SOCOM 1's: the light maps and shadows that reach a node listed afresh, six at most -- the node's
 * bounding sphere (its box's half-diagonal about the box's centre, carried by its matrix) against
 * each active light map's range; a shadow only while shadows apply and never on the model it is the
 * shadow of (SOCOM II computes the sphere once, ahead of the walk). The count and the light are
 * declared uninitialised at the top (numbered there: the count s4 and the light s3 below the list's
 * s2, as the target's -- declared where they are set, 31 off), the count unsigned (sltiu). The world
 * lane's round 20, byte-exact: the model test's instance passed to an inline helper -- an inline's
 * parameter is numbered after the caller's named locals, below the node type's CSE temporary
 * (@N, shared by both ternaries), so the type is coloured first and takes a0, the instance a1, as
 * the target's (tools/igreplay.py --want: any number for the instance below the type's gives it;
 * compared in place, or the instance in a named local -- copy-propagated away -- the pair swaps:
 * 5 off) */
void CPipe::BuildLightMapList(zdb::CNode *node)
{
    unsigned int n;
    zdb::CLightMap *l;

    zdb::CVisual::m_lightMapList.clear();
    zdb::CVisual::m_shadowMapList.clear();
    zdb::CLightMapVector &lights = m_world->m_lightMaps;

    if (lights.size()) {
        CPnt3D half = (node->GetBBox()->m_max - node->GetBBox()->m_min) * 0.5f;
        float r = half.Length();
        CPnt3D center = node->GetBBox()->m_min + half;

        node->m_matrix.Transform(&center, 1);
        n = 0;
        for (zdb::CLightMapVector::iterator i = lights.begin(); i != lights.end() && n < 6; ++i) {
            l = *i;

            if (l->m_active) {
                if (l->m_isShadow) {
                    if (zdb::CVisual::m_applyShadow) {
                        zdb::CNode *model = node->m_type == 2 ? node : 0;

                        if (!model || !IsModelOf(l->m_instance, node)) {
                            if (LightReaches(l, center, r)) {
                                zdb::CLightMap *kept = l;

                                if (!zdb::CVisual::m_shadowMapList.Exists(l))
                                    zdb::CVisual::m_shadowMapList.push_back(kept);
                                n++;
                            }
                        }
                    }
                } else if (LightReaches(l, center, r)) {
                    zdb::CLightMap *kept = l;

                    if (!zdb::CVisual::m_lightMapList.Exists(l))
                        zdb::CVisual::m_lightMapList.push_back(kept);
                    n++;
                }
            }
        }
    }
}

/* a span about a value reaching into [lo, hi] (ours): the bounds by reference, so the upper one is
 * read only once the lower has passed; false preset and set true in the nested test */
static inline bool SpanMeets(float v, float r, const float &lo, const float &hi)
{
    bool t = false;

    if (!(v + r < lo))
        if (v - r <= hi)
            t = true;
    return t;
}

/* a sphere meeting a box, axis by axis (ours): x and y into one preset bool, z into another */
static inline bool SphereMeetsBox(const CPnt3D *p, float r, const CBBox *b)
{
    bool xyz = false;
    bool xy = false;

    if (SpanMeets(p->x, r, b->m_min.x, b->m_max.x))
        if (SpanMeets(p->y, r, b->m_min.y, b->m_max.y))
            xy = true;
    if (xy)
        if (SpanMeets(p->z, r, b->m_min.z, b->m_max.z))
            xyz = true;
    return xyz;
}

/* SOCOM II's (the name ours; RenderDelayedNodes and PrepareAtom call it after BuildLightMapList):
 * with any shadow casters the projected maps listed afresh -- for a pipe's node marked to take them,
 * each caster whose box the node's bounding sphere meets.
 * The box's address a named local of the walk, passed to the box test: MW then reads the box
 * through it (a2) after the first block, where `&c->m_bbox` passed straight is folded into every
 * load (s2 + 0x54...); each axis presets its false in the first compare's delay slot (the span a
 * preset bool set in nested ifs, the bounds by reference), x and y fold into one preset bool and z
 * into another -- the world lane's rounds 19 and 21 (build/scratch/wld/v_bpml.py) */
void CPipe::BuildProjectedMapList(zdb::CNode *node)
{
    if (m_world->m_shadow_casters.size()) {
        zdb::CVisual::m_projectedMapList.clear();
        if (m_node->m_socom2_a1_bits.bit6 == 1) {
            std::vector<zdb::CShadowCaster *> &casters = m_world->m_shadow_casters;

            for (std::vector<zdb::CShadowCaster *>::iterator i = casters.begin(); i != casters.end(); ++i) {
                zdb::CShadowCaster *c = *i;
                float r = node->GetRadius();
                const CPnt3D *pos = (const CPnt3D *)node->m_matrix.m_matrix[3];
                const CBBox *box = &c->m_bbox;

                if (SphereMeetsBox(pos, r, box)) {
                    zdb::CShadowCaster *kept = c;

                    zdb::CVisual::m_projectedMapList.push_back(kept);
                }
            }
        }
    }
}

/* SOCOM II's: the dynamic textures of a node's visuals added to the alpha pass's. The list's
 * address a local taken inside the guard (hoisted into a saved register; through the member, or
 * an inline's parameter or `this`, MW adds the offset at each call), the end before the first
 * element (i in s0). Pinned: SaveDelayedNode, below, calls it out of line */
#pragma auto_inline off
void CPipe::AddAlphaTextures(zdb::CNode *node)
{
    if (node->m_visual.size()) {
        zdb::CDynTexList *list = &m_alpha.m_dyntexlist;
        zdb::CVisualVector::iterator e = node->m_visual.end(), i = node->m_visual.begin();

        for (; i != e; ++i)
            list->Add(&(*i)->m_dynTexList);
    }
}
#pragma auto_inline reset

/* SOCOM 1's: an active node with visuals kept for later -- the delayed nodes grown by 8 when full,
 * the next one given the matrix on the stack's top times the node's, the pipe's node, the node and
 * its FOV verdict, the opacity on top of its stack, the visuals' added colour and light scale, the
 * fog flag; a translucent one's textures (and its active children's) go to the alpha pass
 * (SOCOM II's AddAlphaTextures, which SOCOM 1 wrote out here) */
void CPipe::SaveDelayedNode(CMatrix *matrix, zdb::CNode *node, zdb::tag_ZVIS_FOV fov)
{
    if (!node->m_active || !node->m_hasVisuals)
        return;
    if (m_delayedNodeNum >= m_DelayedNodes.size())
        m_DelayedNodes.resize(m_DelayedNodes.size() + 8);
    tag_DelayedNode *d = &m_DelayedNodes[m_delayedNodeNum++];

    sceVu0MulMatrix(d->m_matrix.m_matrix, matrix->m_matrix, CStack::m_top->m_matrix);
    d->m_atom = m_node;
    d->m_node = node;
    d->m_fov = fov;
    d->m_opacity = m_opacity_stack[m_opacity_stack_index];
    CPnt3D color(zdb::CVisual::m_addColor);

    d->m_addColor = color;
    d->m_lightScale = zdb::CVisual::m_GlobalLightScale;
    d->m_fog = m_doFog;
    if (d->m_opacity < 0.9921875f || node->m_transparent) {
        /* the children by reference, taken after the node's own call: the walk then keeps their
         * vector's address (s2) and every saved register is the target's -- walked through
         * node->m_child, `this` drops from s0 to s3 and the rest shift (taken before the call,
         * 8 bytes longer) */
        AddAlphaTextures(node);
        zdb::CNodeVector &c = node->m_child;
        for (zdb::CNodeVector::iterator i = c.begin(); i != c.end(); ++i)
            if ((*i)->m_active && (*i)->m_hasVisuals)
                AddAlphaTextures(*i);
    }
}

/* SOCOM 1's: the delayed nodes drawn in the order saved -- the light lists rebuilt when the node
 * saved with one changes (SOCOM II adds its second list), its matrix pushed, its opacity, added
 * colour, fog and light scale restored (a node's own scale byte when it has one), then drawn; SOCOM
 * II's altitude-fog switch: with the camera's on, the node is drawn with it as its first visual's
 * bit 19 says, and the camera's put back after */
void CPipe::RenderDelayedNodes()
{
    m_node = 0;
    zdb::CVisual::LandmarkEnable(false);
    tag_DelayedNode *d = &m_DelayedNodes[0];

    for (unsigned int i = 0; i < m_delayedNodeNum; i++, d++) {
        if (m_node != d->m_atom) {
            m_node = d->m_atom;
            BuildLightMapList(m_node);
            BuildProjectedMapList(m_node);
        }
        s_stack.Push(&d->m_matrix);
        PushOpacity(d->m_opacity);
        CPnt3D color(d->m_addColor);

        zdb::CVisual::m_addColor = color;
        m_doFog = d->m_fog != 0;
        if (m_node->m_socom2_a1 & 4) {
            signed char scale = m_node->m_socom2_a0;

            if (scale < 0)
                zdb::CVisual::m_GlobalLightScale = 1.0f;
            else
                zdb::CVisual::m_GlobalLightScale = (1.0f / 127.0f) * scale;
        } else
            zdb::CVisual::m_GlobalLightScale = d->m_lightScale;
        bool altFog = m_camera->m_fog_alt_enabled;

        if (!altFog)
            RenderNode(d->m_node, d->m_fov);
        else {
            zdb::CVisual *v = m_node->GetFirstVisual();

            m_camera->m_fog_alt_enabled = v && v->m_bits.m_b19;
            RenderNode(d->m_node, d->m_fov);
            m_camera->m_fog_alt_enabled = altFog;
        }
        zdb::CVisual::m_GlobalLightScale = 1.0f;
        m_opacity_stack_index--;
        s_stack.Pop();
    }
}

/* zmath's: whether a matrix is the unit one (0x3084F0, not named); CVisual's light set-up
 * (0x3B6B20, not named) */
extern "C" bool func_003084F0(const CMatrix *m);
extern "C" void func_003B6B20(zdb::CNode *node, int n, float scale);

/* SOCOM 1's: an active node with visuals drawn into the shadow pass with its children -- its matrix
 * multiplied onto the stack unless it is the unit one (SOCOM II's test), its visuals set up and each
 * drawn unless its level of detail says not at the camera's range */
void CPipe::RenderNodeShadow(zdb::CNode *node)
{
    if (!node->m_active || !node->m_hasVisuals)
        return;
    bool identity = func_003084F0(&node->m_matrix);
    CMatrix *m;

    if (identity)
        m = CStack::m_top;
    else
        m = s_stack.Multiply(&node->m_matrix, true);
    if (node->GetFirstVisual()) {
        zdb::CVisual::NodeSetup(m_node, node, m);
        func_003B6B20(node, 1, 1.0f);
        zdb::CVisualVector &v = node->m_visual;

        for (zdb::CVisualVector::iterator i = v.begin(); i != v.end(); ++i) {
            zdb::CVisual *vis = *i;

            unsigned int lod = vis->m_5;

            if (lod) {
                zdb::CLOD_band *band = m_world->m_LOD_Object.GetBand(lod);

                if (band) {
                    float fade;

                    if (!vis->DrawLOD(band, m_camera->GetScaledRangeSquared((CPnt3D *)m->m_matrix[3]), &fade))
                        continue;
                }
            }
            vis->Render(m);
        }
    }
    zdb::CNodeVector &c = node->m_child;

    for (zdb::CNodeVector::iterator i = c.begin(); i != c.end(); ++i)
        RenderNodeShadow(*i);
    if (&node->m_matrix && !identity)
        s_stack.Pop();
}

/* SOCOM 1's: a node of a skinned mesh drawn into the shadow pass -- its matrix multiplied onto the
 * stack; a mesh of its own taken over (the unit matrix pushed for it); a sub-mesh's matrix written
 * into its mesh's palette (uncached); each child a sub-mesh again or a plain node drawn under the
 * mesh's matrix; its own mesh set up and drawn at its first level of detail; everything popped */
void CPipe::RenderMeshShadow(zdb::CNode *node, zdb::CMesh *mesh, CMatrix *matrix)
{
    zdb::CMesh *own = node->GetMesh();

    node->GetFirstVisual();
    zdb::CSubMesh *sub = node->GetSubMesh();
    CMatrix *m = s_stack.Multiply(&node->m_matrix, true);

    if (own) {
        mesh = own;
        s_stack.Push(&CMatrix::identity);
        matrix = m;
    }
    if (sub)
        sceVu0CopyMatrix((float (*)[4])((unsigned int)mesh->m_palette[sub->m_index] | 0x20000000), m->m_matrix);
    zdb::CNodeVector &c = node->m_child;

    for (zdb::CNodeVector::iterator i = c.begin(); i != c.end(); ++i) {
        zdb::CNode *child = *i;

        if (child->GetSubMesh())
            RenderMeshShadow(child, mesh, matrix);
        else {
            CMatrix t;

            sceVu0MulMatrix(t.m_matrix, matrix->m_matrix, CStack::m_top->m_matrix);
            s_stack.Push(&t);
            RenderNodeShadow(child);
            s_stack.Pop();
        }
    }
    if (own) {
        zdb::CVisual::NodeSetup(m_node, node, m);
        func_003B6B20(node, 1, 1.0f);
        own->SetLOD(0u, 1u);
        own->Render(m);
        s_stack.Pop();
    }
    if (&node->m_matrix)
        s_stack.Pop();
}

/* SOCOM 1's (inline there; 0x3379A0 out of line here, pinned: CPipe() calls it): the draw list and
 * the texture lists built, the delayed nodes' buffer allocated, the walk at its start */
#pragma auto_inline off
CPipe::CAlpha::CAlpha()
{
    m_next = m_visualList = (tag_DelayedNode *)_malloc(0x36B00, 0, 0);
}
#pragma auto_inline reset

/* SOCOM 1's (inline there; 0x3378A0 out of line here, pinned: ~CPipe calls it): the walk back at the
 * start, the draw list and the texture lists emptied, the buffer freed */
#pragma auto_inline off
CPipe::CAlpha::~CAlpha()
{
    m_next = m_visualList;
    m_drawList.clear();
    m_dyntexlist.Clear();
    _free(m_visualList, 0, 0);
}
#pragma auto_inline reset

/* the next visual drawn is a node's first since the last walk ended: its node is set up first
 * (SOCOM 1's global, a byte -- OVL1 .data 0x3DFF80, 1 in the image) */
extern bool FirstVisDrawn;
/* the local lights' set-up of a lit node (0x338FA0, below) */
void BuildNodeLocalLightList(zdb::CNode *node);

/* SOCOM 1's: a node's visuals drawn -- each one its level of detail rejects at the camera's range
 * skipped (and, with the LOD filter on, the walk's result false), its fog switch set, its detail
 * layers' range taken; one the frame's opacity or its flags make translucent added to the alpha
 * pass with its textures and its material's texture, any other drawn at once, the node set up before
 * the first; the node stamped with the frame when any was. SOCOM II drops SOCOM 1's bench counts
 * and its bilinear-range test, tests the atom's mesh bit and the visual's bits 0 and 8, adds the
 * material's texture, and sets the lights up with the view and the atom's opacity. Written in the
 * world lane's round 27 (build/scratch/wld/v_rv*.py, vscore.py): the stack's top through an inline
 * wrapper (StackTop, ours -- a local from an inline's result is numbered after the front-end
 * temporaries, which gives m s1 and the visual s2; read as `CStack::m_top` straight into the local
 * the two swap, 16 off), the visuals' vector and the alpha pass's texture list as references, the
 * material index a local of its own (read twice, the target's one lbu), bit 8's test `== 0`, the
 * detail layers through a local declared after the counter (the counter, layers, count and test
 * then take the target's a2 / a1 / a0 / v1).
 * THE DETAIL LOOP IS NOT STRENGTH-REDUCED in the target (sll/addu of the index each pass, as SOCOM 1's
 * loop): no loop form tried changes MW's pointer walk (an inline test by pointer or reference, an int
 * counter, the increment in the body, a while, the layers through a local) -- only the
 * function-scoped `#pragma opt_strength_reduction off` gives the target's size (0x3DC: a stand-in,
 * its original spelling unknown).
 * OPEN, 24 instructions at the target's size, every register the target's -- placement only: the
 * vector's address (s6) is computed at the top and the texture list's (s7) fills the entry test's
 * delay slot, the layers' load precedes the detail loop's entry test, where the target computes all
 * three in the loops' preheaders as hoisted invariants (the vector's in the slot, then the fov
 * narrowing, then the list's; the layers after the test). Named locals keep their definitions'
 * place, and every form that leaves them to LICM (the vector read in place, the list or the layers
 * assigned inside the loop, accessors, pointers, other orders) loses the registers (22-36) */
static inline CMatrix *StackTop()
{
    return CStack::m_top;
}

#pragma opt_strength_reduction off
bool CPipe::RenderVisual(zdb::CNode *node, zdb::tag_ZVIS_FOV fov)
{
    bool drawn = true;
    int count = 0;
    CMatrix *m = StackTop();
    float opacity = m_opacity_stack[m_opacity_stack_index];
    zdb::CVisualVector &v = node->m_visual;
    zdb::CDynTexList &dyn = m_alpha.m_dyntexlist;

    for (zdb::CVisualVector::iterator i = v.begin(); i != v.end(); ++i) {
        zdb::CVisual *vis = *i;

        if (vis->m_5) {
            float d = m_camera->GetScaledRangeSquared((CPnt3D *)m->m_matrix[3]);
            zdb::CLOD_band *band = m_world->m_LOD_Object.GetBand(vis->m_5);

            if (band && !vis->DrawLOD(band, d, &opacity)) {
                if (m_LODFilter)
                    drawn = false;
                continue;
            }
        }
        zdb::CVisual::m_fogEnable = m_doFog && (vis->m_flags & 2);
        if (vis->m_numDetails) {
            CPnt3D c;

            CStack::m_top->Transform(&vis->m_centroid, &c, 1);
            float d = m_camera->GetScaledRangeSquared(&c);

            zdb::CVisual::m_rangeSqdToCamera = d;
            zdb::CVisual::m_applyDetailTexture = false;
            unsigned int k;
            zdb::CVisual::DETAIL *det = vis->m_details;

            for (k = 0; k < vis->m_numDetails; k++)
                if (d <= det[k].m_rangeSq) {
                    zdb::CVisual::m_applyDetailTexture = true;
                    break;
                }
        }
        count++;
        if (!m_node->m_hasMesh && !m_doUI &&
            (((vis->m_flags & 1) & ((vis->m_flags & 0x100) == 0)) || opacity < 1.0f) && !m_simpletrans) {
            if (opacity <= 1.0f / 128)
                continue;
            m_alpha.Add(m_node, node, vis, fov, opacity);
            dyn.Add(&vis->m_dynTexList);
            unsigned int mat = vis->m_7;

            if (mat) {
                zdb::CTexHandle *tex = zdb::CWorld::m_world->m_Material_Object.GetMaterial(mat)->m_tex;

                if (tex)
                    dyn.Add(tex, true);
            }
        } else {
            if (FirstVisDrawn) {
                FirstVisDrawn = false;
                zdb::CVisual::NodeSetup(m_node, node, m);
                if (zdb::CVisual::m_applyLocalLights)
                    BuildNodeLocalLightList(node);
                func_003B6B20(node, fov, m_node->m_Opacity);
            }
            vis->Render(m);
        }
    }
    FirstVisDrawn = true;
    if (count)
        node->m_frameRendered = zVid.frameNumber;
    return drawn;
}
#pragma opt_strength_reduction reset

/* the view id in use, the stack's top -- a `break` if the stack is empty (vis_vu.cpp's inline,
 * CVisual::GetVid: six levels high, so Add inlines it one level and calls empty() and top()) */
inline unsigned int zdb::CVisual::GetVid()
{
    if (m_stack_vid.empty()) {
        __I_break(0);
        __I_nop();
    }
    return (unsigned int)m_stack_vid.top();
}

/* a point copied a float at a time (ours) */
static inline void CopyPnt4D(PNT4D *d, const CPnt4D *s)
{
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
    d->w = s->w;
}

/* SOCOM II's: a line with translucent colour kept for the alpha pass -- its two ends and their
 * colours where a delayed node's matrix is, no node (the record's mark of a line), its range from
 * the camera for the draw order */
void CPipe::CAlpha::AddLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1)
{
    tag_DelayedNode *d = Next();

    if (d) {
        PNT4D *line = (PNT4D *)&d->m_matrix;

        CopyPnt4D(&line[0], p0);
        CopyPnt4D(&line[1], p1);
        CopyPnt4D(&line[2], c0);
        CopyPnt4D(&line[3], c1);
        d->m_atom = 0;
        d->m_40 = m_camera->GetScaledRangeSquared((CPnt3D *)p0);
        Insert(d);
    }
}

/* SOCOM 1's: a translucent visual kept for the pass -- the next record given the stack's top matrix,
 * the node saved with it and the node, the added colour and light scale, the FOV verdict, the visual,
 * the lighting and fog switches, the view id and the opacity -- its range from the camera (an
 * always-last visual far away; SOCOM II's test) for the draw order */
void CPipe::CAlpha::Add(zdb::CNode *atom, zdb::CNode *node, zdb::CVisual *visual, zdb::tag_ZVIS_FOV fov,
                        float opacity)
{
    tag_DelayedNode *d = Next();

    if (d) {
        sceVu0CopyMatrix(d->m_matrix.m_matrix, CStack::m_top->m_matrix);
        d->m_atom = atom;
        d->m_node = node;
        CPnt3D color(zdb::CVisual::m_addColor);

        d->m_addColor = color;
        d->m_lightScale = zdb::CVisual::m_GlobalLightScale;
        d->m_fov = fov;
        d->m_visual = visual;
        d->m_lighting = zdb::CVisual::m_lightingEnable;
        d->m_fog = zdb::CVisual::m_fogEnable;
        d->m_vid = zdb::CVisual::GetVid();
        d->m_opacity = opacity;
        CPnt3D pos;

        CStack::m_top->Transform(&visual->m_centroid, &pos, 1);
        if (d->m_visual->m_flags & 0x40000)
            d->m_40 = 100000000.0f;
        else
            d->m_40 = m_camera->GetScaledRangeSquared(&pos);
        Insert(d);
    }
}


/* a view id given back -- a `break` if none is out (SOCOM 1's test, as CVisual::GetVid's; the name
 * ours) */
static inline void PopVid()
{
    if (zdb::CVisual::m_stack_vid.empty()) {
        __I_break(0);
        __I_nop();
    }
    zdb::CVisual::m_stack_vid.pop();
}

/* the particle buffer's pass (zparticle_buf.cpp's, SOCOM 1's names): its set-up, which returns the
 * depth the first range ends at, a range drawn up to a depth, the rest flushed, and the whole pass
 * when no translucent object waits */
namespace Particle {
float DrawPrep();
float DrawRange(float depth, float end);
void DrawFlush();
void Draw();
}

/* the local lights' set-up of a lit node (SOCOM 1's BuildNodeLocalLightList, which also took a
 * second node; 0x338FA0, not written: RenderVisual calls it too) */
void BuildNodeLocalLightList(zdb::CNode *node);

/* SOCOM 1's: the translucent objects drawn in their depth order between the particle ranges -- each
 * with its lighting and fog switches, added colour and view id restored, a node set up, lit and drawn
 * through its visual, a line drawn as one; the particles left over flushed, or the whole particle
 * pass when nothing waits; the added colour cleared after. The world lane round 18, byte-exact: the
 * zero colour declared at the top and assigned at the end (its frame slot above the loop's colour;
 * built where it is declared, the function is 0x2A8, as a temporary 0x274), and the list's end in a
 * named local beside the cursor (in the test, the end is numbered after the loop's hoisted colour
 * addresses and takes s0 from them: 11 off) */
void CPipe::CAlpha::Render()
{
    CPnt3D zero;

    zdb::CVisual::AlphaEnable(true);
    zdb::CVisual::m_applyShadow = 0;
    zdb::CVisual::m_applyMultipass = 1;
    zdb::CVisual::LandmarkEnable(false);
    data_004B4E34 = 0;
    if (m_drawList.size()) {
        float depth = Particle::DrawPrep();
        std::list<void *>::iterator i, e = m_drawList.end();

        for (i = m_drawList.begin(); i != e; ++i) {
            tag_DelayedNode *d = (tag_DelayedNode *)*i;

            if (!(depth <= d->m_40))
                depth = Particle::DrawRange(depth, d->m_40);
            zdb::CVisual::m_lightingEnable = d->m_lighting;
            zdb::CVisual::m_fogEnable = d->m_fog;
            CPnt3D color(d->m_addColor);

            zdb::CVisual::m_addColor = color;
            zdb::CVisual::m_stack_vid.push((void *)d->m_vid);
            if (d->m_atom) {
                zdb::CVisual::NodeSetup(d->m_atom, d->m_node, &d->m_matrix);
                if (zdb::CVisual::m_applyLocalLights)
                    BuildNodeLocalLightList(d->m_node);
                func_003B6B20(d->m_node, d->m_fov, d->m_opacity);
                zdb::CVisual::m_GlobalLightScale = d->m_lightScale;
                d->m_visual->VuUpdate();
            } else
                Draw3DLine((CPnt4D *)d, (CPnt4D *)d + 1, (CPnt4D *)d + 2, (CPnt4D *)d + 3);
            PopVid();
        }
        zdb::CVisual::m_GlobalLightScale = 1.0f;
        Particle::DrawFlush();
    } else
        Particle::Draw();
    zero = CPnt3D(0.0f, 0.0f, 0.0f);
    CPnt3D color(zero);

    zdb::CVisual::m_addColor = color;
    zdb::CVisual::AlphaEnable(false);
    zdb::CVisual::m_applyMultipass = 1;
    m_first_alpha_add = 1;
}

/* SOCOM 1's: a record put in the draw list in depth order, deepest first -- the walk starts at the
 * last insert (at the list's start again after each pass: Render sets 0x3DFF68), forward while the
 * record is nearer than the one under the cursor, else backward while it is deeper, before the first
 * when it is deeper than every one.
 * OPEN, 0x248 of 0x244 (the world lane round 18; build/scratch/wld/v_ins*.py): the logic and the
 * three inserts are the target's, the cursor in memory as there (its address taken by the by-value
 * copy for insert), the record homed for insert's `const T &`. Left: (1) the target keeps the
 * list's first node in a3 from the head through the backward walk and builds the ternary's other
 * arm from it (`sw a3,64(sp)`, copied through $f0) -- `b` there takes b's address (the conditional
 * an lvalue: b in memory, 0x224), `iterator(b)` 0x21C, a fresh begin() (as here) keeps `this` in s0
 * for it; (2) the first walk reloads the record through its home every pass where MW here hoists
 * `&d->m_40` (the third walk hoists it in the target too; a for/break first walk changes nothing) */
void CPipe::CAlpha::Insert(tag_DelayedNode *d)
{
    if (m_first_alpha_add) {
        m_first_alpha_add = 0;
        m_last_alpha_insert = m_drawList.begin();
    }
    std::list<void *>::iterator it = m_last_alpha_insert;
    std::list<void *>::iterator b = m_drawList.begin(), e = m_drawList.end();

    if (it == b) {
        while (it != e && d->m_40 < ((tag_DelayedNode *)*it)->m_40)
            ++it;
        m_last_alpha_insert = m_drawList.insert(it, (void *const &)d);
    } else if (!(d->m_40 < ((tag_DelayedNode *)*it)->m_40)) {
        while (it != b && !(d->m_40 <= ((tag_DelayedNode *)*it)->m_40))
            --it;
        m_last_alpha_insert = m_drawList.insert(d->m_40 < ((tag_DelayedNode *)*b)->m_40 ? ++it : m_drawList.begin(), (void *const &)d);
    } else {
        while (it != e && d->m_40 < ((tag_DelayedNode *)*it)->m_40)
            ++it;
        m_last_alpha_insert = m_drawList.insert(it, (void *const &)d);
    }
}

/* SOCOM 1's: an active UI node drawn with its children -- its view id pushed, its matrix multiplied
 * onto the stack, its opacity pushed (SOCOM II's helper, out of line), its visuals' lights listed and
 * its visuals rendered, each child a mesh or a node of its own, then everything popped */
void CPipe::RenderUiNodeRecursive(zdb::CNode *node)
{
    if (node->m_active) {
        zdb::CVisual::m_stack_vid.push((void *)node->m_vid);
        s_stack.Multiply(&node->m_matrix, true);
        bool pushed = PushNodeOpacity(node);

        if (node->m_visual.size()) {
            BuildSpecMapList(node);
            RenderVisual(node, (zdb::tag_ZVIS_FOV)0);
        }
        zdb::CNodeVector &c = node->m_child;
        for (zdb::CNodeVector::iterator i = c.begin(); i != c.end(); ++i) {
            if ((*i)->m_hasMesh)
                RenderMesh(*i, 0, 0, (zdb::tag_ZVIS_FOV)0);
            else
                RenderUiNodeRecursive(*i);
        }
        if (&node->m_matrix)
            s_stack.Pop();
        if (pushed)
            m_opacity_stack_index--;
        PopVid();
    }
}

/* SOCOM II's (the name ours; 0x336A10 out of line, pinned: RenderUiNodeRecursive, RenderMesh and
 * RenderNode call it): a translucent node's opacity pushed, and whether it was */
#pragma auto_inline off
bool CPipe::PushNodeOpacity(zdb::CNode *node)
{
    if (node->m_Opacity < 0.9921875f) {
        PushOpacity(node->m_Opacity);
        return true;
    }
    return false;
}
#pragma auto_inline reset

/* an opacity pushed, multiplied into the one under it; the stack grown a slot at a time (pinned:
 * PushNodeOpacity and RenderDelayedNodes call it out of line) */
#pragma auto_inline off
void CPipe::PushOpacity(float opacity)
{
    m_opacity_stack_index++;
    if (m_opacity_stack_index == m_opacity_stack_size) {
        m_opacity_stack = (float *)_realloc(m_opacity_stack, (m_opacity_stack_size + 1) * sizeof(float), 0, 0);
        m_opacity_stack_size++;
    }
    m_opacity_stack[m_opacity_stack_index] = opacity * m_opacity_stack[m_opacity_stack_index - 1];
}
#pragma auto_inline reset

/* zvid_init.cpp's: the depth under a screen point, false off the screen (the point's projection is
 * zdb::CCamera::GetScreenPosPers, 0x290450: false behind the camera) */
extern "C" bool func_003B0E70(const float *p, unsigned int *z);

/* SOCOM II's (the name ours; pinned: CFlareSystem::Tick calls it out of line): whether a point is in
 * front of what the depth buffer holds under it -- pulled first toward the camera by `pull` -- its
 * projection left in `out` */
#pragma auto_inline off
bool CPipe::TestFlare(const CPnt3D *pos, CPnt4D *out, float pull)
{
    bool visible = false;
    CPnt4D local;
    CPnt3D moved;
    CPnt4D *o = out ? out : &local;

    if (pull != 0.0f) {
        CPnt3D dir = *(CPnt3D *)m_camera->m_matrix.m_matrix[3] - *pos;

        dir.Normalize();
        dir.Scale(pull);
        CPnt3D p = *pos + dir;

        moved = p;
        pos = &moved;
    }
    if (m_camera->GetScreenPosPers(pos, o)) {
        unsigned int z;

        if (func_003B0E70((float *)o, &z))
            visible = o->z >= (float)z;
    }
    return visible;
}
#pragma auto_inline reset

/* SOCOM II's (the name ours; hud_lensflare.cpp's): a flare added -- the first inactive one taken
 * over, else a new one pushed (the storage grown by ten) -- its handle given back */
int CFlareSystem::Add(const CPnt3D *pos, float size)
{
    int handle = -1;

    if (m_flares.size()) {
        for (unsigned int i = 0; i < m_flares.size() && handle < 0; i++) {
            CFlare *f = m_flares.at(i);

            if (f->IsActive() == false) {
                handle = i;
                f->m_pos.x = pos->x;
                f->m_pos.y = pos->y;
                f->m_pos.z = pos->z;
                f->m_size = size;
                f->SetActive(true);
            }
        }
    }
    if (handle < 0) {
        CFlare *f = new CFlare(pos, size);

        if (m_flares.size() == m_flares.capacity())
            m_flares.reserve(m_flares.size() + 10);
        m_flares.push_back(f);
        handle = m_flares.size() - 1;
    }
    m_0 = 1;
    return handle;
}

/* SOCOM II's: a flare let go of -- its active bit cleared, when the handle is one */
void CFlareSystem::Release(int handle)
{
    if (handle >= 0 && handle < m_flares.size())
        m_flares.at(handle)->SetActive(false);
}

/* SOCOM II's: every flare deleted, the vector emptied and its storage given back (FreeVector
 * inlined one level: its pieces called) */
void CFlareSystem::Clear()
{
    for (std::vector<CFlare *>::iterator i = m_flares.begin(); i != m_flares.end(); ++i)
        delete *i;
    m_flares.clear();
    FreeVector(m_flares);
}

/* zmath's CPnt4D() out of line: the row 0x2110E0 is __ct__6CPnt4DFv, the weak copy hud_linemap.cpp
 * emits for its initialiser's __construct_array (fts_message.cpp's initialiser and Props::Create use
 * the same copy). The flare's projection is built through it -- called here by its symbol, the
 * original's spelling unknown (zmath.h's constructor is inline: `CPnt4D p;`, `CPnt4D p = CPnt4D();`
 * and a one-element array all inline it, 0x104 against 0x10C) */
extern "C" void __ct__6CPnt4DFv(CPnt4D *p);

/* SOCOM II's (the name ours; CCoreState::Tick calls it): with any flare added, each one's verdict
 * cleared and, for an active one to test, taken again with its projection's depth */
void CFlareSystem::Tick()
{
    if (m_0) {
        for (std::vector<CFlare *>::iterator i = m_flares.begin(); i != m_flares.end(); ++i) {
            CFlare *f = *i;

            f->SetVisible(false);
            if (f->m_active && f->m_test) {
                CPnt4D p;

                __ct__6CPnt4DFv(&p);
                f->SetVisible(thePipe.TestFlare(&f->m_pos, &p, f->m_size));
                f->m_depth = p.w;
            }
        }
    }
}
