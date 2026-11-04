/* sc: as=RenderLineWorld__F8CPnt4D_48CPnt4D_48CPnt4D_48CPnt4D_4 RenderLineWorld__F6CPnt4D6CPnt4D6CPnt4D6CPnt4D */
/* zrndr_pipe.cpp -- CPipe, the renderer's packet pipe (OVL1 0x336230?-0x33CD4F): what it draws
 * and in which order, the texture loads queued in the GS FIFO between its packets. SOCOM 1's
 * class (its DWARF: 0x694 bytes; SOCOM II's thePipe runs to 0x720); SOCOM 1's file name (its
 * __sinit_zrndr_pipe.cpp builds thePipe as SOCOM II's 0x402E60 does). Defined in reverse of their
 * image order (-inline deferred), on node_world.h's real texture classes.
 * Written so far: the lens flares' system's Clear and Release (0x336340, 0x3364D0: the unit opens
 * with that system's code, SOCOM II's), the opacity stack's push, the alpha pass's constructor and
 * destructor, the alpha pass's textures, the noise overlay (its kick, its texture, its pixels),
 * Flush with the three texture-buffer inlines' copies after it, the flush FIFO packet, the FIFO
 * signal and load (FIFO_Load ours: SOCOM 1's two-argument FIFO_Signal less its argument), VIF1's interrupt, the destructor and the constructor -- whose resize of the
 * delayed nodes brings the template tail (0x33C2A0-0x33CCCF: resize, the deleters' clears and
 * destructors, erase, insert; ~CFlareSystem 0x33CCD0 last). 0x33CD50-0x33D22F is another unit,
 * zrndr_postfx.cpp (CPipeEffect's: the template tail closes this one, and its initialiser 0x402F80
 * follows this unit's).
 * OPEN: the delayed nodes' insert 0x33C5C0 (109 instructions, registers only: the in-place end
 * pointer in s6 against the target's s0, so every register after it shifts -- include/vector's
 * class insert end register, open for the MSL lane: its comment lists the shapes tried);
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
    unsigned int m_44;   // 0x44
    zdb::tag_ZVIS_FOV m_fov;   // 0x48 (a byte enum: passed on with lb)
    zdb::CNode *m_atom;   // 0x4c: the pipe's node when saved, its light lists rebuilt when it changes
    zdb::CNode *m_node;   // 0x50
    CPnt3D m_addColor;   // 0x54: CVisual::m_addColor when saved
    unsigned int m_flag0 : 1;   // 0x60
    unsigned int m_fog : 1;   // 0x60: the pipe's m_doFog when saved
    unsigned int m_flags : 30;   // 0x60
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
    bool PushNodeOpacity(zdb::CNode *node);
    void RenderUiNodeRecursive(zdb::CNode *node);
    /* SOCOM 1's RenderVisual (0x3374C0, not written) */
    void RenderVisual(zdb::CNode *node, zdb::tag_ZVIS_FOV fov);
    /* RenderAtom's callees (not written): SOCOM II's lighting and fog set-up (0x33B230, ours), SOCOM 1's
     * RenderMesh (0x338480) and RenderNode (0x3389C0) */
    void PrepareAtom(zdb::CNode *node);
    void BuildLightMapList(zdb::CNode *node);
    void RenderMesh(zdb::CNode *node, zdb::CMesh *mesh, CMatrix *matrix, zdb::tag_ZVIS_FOV fov);
    void RenderNode(zdb::CNode *node, zdb::tag_ZVIS_FOV fov);

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
    unsigned char m_drawDI;   // 0x715
    unsigned char m_drawNormal;   // 0x716
    unsigned char m_drawBBox;   // 0x717
    unsigned char m_LODFilter;   // 0x718
    unsigned char m_reticuleDraw;   // 0x719
    unsigned char _71a[0x720 - 0x71a];   // 0x71a
};   // 0x720 bytes

/* the world's lighting switch (0x3B7130: a byte set and its inverse beside it) and SOCOM II's second
 * light list (0x339470, not written), after BuildLightMapList's */
extern "C" void func_003B7130(bool on);
extern "C" void func_00339470(CPipe *pipe, zdb::CNode *node);

/* zvid_init.cpp's: the 21-quadword packet's word set, and the packet sent (it reads no argument;
 * the pipe passes one) */
extern "C" void func_003B0D50(unsigned int v);
extern "C" void func_003B0CC0(unsigned int v);

/* the lens flares' system (OVL1 bss 0x488F10, after thePipe; the HUD lane's name, hud_lensflare.cpp
 * calls it): its flares a pointer vector at 4, each with an active bit first (the names ours) */
struct CFlare {
    unsigned char m_active : 1;   // 0x0

    void SetActive(bool on) { m_active = on; }
};
class CFlareSystem {
public:
    /* the byte first cleared, then the vector built (the unit's initialiser; ours) */
    CFlareSystem() : m_0(0) {}
    void Clear();
    void Release(int handle);

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

/* c2dline.cpp's: the line clipped by the camera's planes and the line drawn (SOCOM 1's both), and
 * the alpha pass's line (0x3373B0, not written) */
int clip3DLine(CPnt4D *planes, CPnt4D *p0, CPnt4D *p1);
void Draw3DLine(CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1);
extern "C" void func_003373B0(CPipe::CAlpha *alpha, CPnt4D *p0, CPnt4D *p1, CPnt4D *c0, CPnt4D *c1);

/* a CPnt4D aligned to 4 with no written copy constructor -- what RenderLineWorld's four by-value
 * points are (STAND-IN, the unit's `as=` line gives the function its real name): the target copies
 * each into its frame through four FPU registers (lwc1/swc1, as SOCOM 1's 0x245930 does), which only
 * a 4-aligned point with an implicit copy gives -- zmath.h's 16-aligned CPnt4D copies with lq/sq
 * (0xD4 against 0x12C), and with zparticle.h's written copy constructor the callee copies nothing.
 * zmath.h's CPnt4D cannot be both (SeeFarther needs its 128-bit implicit copies, the particles their
 * written one): the original likely had two 4D point classes; this one is ours */
struct CPnt4D_4 : public PNT4D {
};

/* SOCOM 1's: a line in the world clipped to the view and drawn; SOCOM II defers a line either of
 * whose colours is translucent to the alpha pass (the opaque test first: the draw is the
 * fall-through, the alpha pass's call last) */
void RenderLineWorld(CPnt4D_4 p0, CPnt4D_4 p1, CPnt4D_4 c0, CPnt4D_4 c1)
{
    if (clip3DLine((CPnt4D *)&zdb::CWorld::m_world->m_camera->m_ClipSet, (CPnt4D *)&p0, (CPnt4D *)&p1)) {
        if (c0.w >= 0.9921875f && c1.w >= 0.9921875f)
            Draw3DLine((CPnt4D *)&p0, (CPnt4D *)&p1, (CPnt4D *)&c0, (CPnt4D *)&c1);
        else
            func_003373B0(&thePipe.m_alpha, (CPnt4D *)&p0, (CPnt4D *)&p1, (CPnt4D *)&c0, (CPnt4D *)&c1);
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
        func_003B7130(!node->m_socom2_5a_bit2);
    } else {
        func_003B7130(false);
        BuildLightMapList(node);
        func_00339470(this, node);
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
    func_003B7130(false);
    tag_DelayedNode *d = &m_DelayedNodes[0];

    for (unsigned int i = 0; i < m_delayedNodeNum; i++, d++) {
        if (m_node != d->m_atom) {
            m_node = d->m_atom;
            BuildLightMapList(m_node);
            func_00339470(this, m_node);
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

/* the free view ids' stack (znode.h's CVisual::m_stack_vid; vis_vu.cpp's class) */
namespace zdb {
class CVidStack : public std::stack<void *> {
};
}

/* SOCOM II's light lists for a node's visuals (0x339AD0, not written: Exists and the pod's insert
 * twice, as BuildLightMapList) */
extern "C" void func_00339AD0(CPipe *pipe, zdb::CNode *node);

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
            func_00339AD0(this, node);
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
