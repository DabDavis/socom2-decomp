/* zrndr_pipe.cpp -- CPipe, the renderer's packet pipe (OVL1 0x336230?-0x33CD4F): what it draws
 * and in which order, the texture loads queued in the GS FIFO between its packets. SOCOM 1's
 * class (its DWARF: 0x694 bytes; SOCOM II's thePipe runs to 0x720); SOCOM 1's file name (its
 * __sinit_zrndr_pipe.cpp builds thePipe as SOCOM II's 0x402E60 does). Defined in reverse of their
 * image order (-inline deferred), on node_world.h's real texture classes.
 * Written so far: the lens flares' system's Clear and Release (0x336340, 0x3364D0: the unit opens
 * with that system's code, SOCOM II's), the opacity stack's push, the alpha pass's constructor and
 * destructor, the alpha pass's textures, the noise overlay (its kick, its texture, its pixels),
 * Flush with the three texture-buffer inlines' copies after it, the flush FIFO packet, the two
 * FIFO signals, VIF1's interrupt, the destructor and the constructor -- whose resize of the
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
#include <iostream>
#include "../zsys/zsys_public.h"
#include "../zsys/zsys.h"
#include "../znode/node_world.h"

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
 * sceVu0CopyMatrix). SOCOM 1's SaveDelayedNode stores the FOV verdict at 0x48, the camera at 0x4c,
 * the node at 0x50, a point at 0x54, a flag in bit 1 of 0x60 and the opacity at 0x64; the names are
 * ours */
struct tag_DelayedNode {
    CMatrix m_matrix;   // 0x0
    float m_40;   // 0x40
    unsigned int m_44;   // 0x44
    signed char m_fov;   // 0x48 (zdb::tag_ZVIS_FOV: copied with lb)
    zdb::CCamera *m_camera;   // 0x4c
    zdb::CNode *m_node;   // 0x50
    CPnt3D m_pos;   // 0x54
    unsigned int m_flags;   // 0x60
    float m_opacity;   // 0x64
    float m_68;   // 0x68
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
    void FIFO_Signal(zdb::CGSTexBuffer *buf, unsigned int *data);
    void PushOpacity(float opacity);
    void AddAlphaTextures(zdb::CNode *node);
    void KickOverlay();
    void FIFO_Flush();
    void RandomizeOverlay();
    void SetOverlayTexture(zdb::CTexHandle *tex);

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
    unsigned char _70e[0x720 - 0x70e];   // 0x70e
};   // 0x720 bytes

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
 * GS reaches it (its marker word 0x44444444); the second argument is SOCOM 1's, unused */
void CPipe::FIFO_Signal(zdb::CGSTexBuffer *buf, unsigned int *data)
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

/* SOCOM II's: the dynamic textures of a node's visuals added to the alpha pass's. The list's
 * address a local taken inside the guard (hoisted into a saved register; through the member, or
 * an inline's parameter or `this`, MW adds the offset at each call), the end before the first
 * element (i in s0) */
void CPipe::AddAlphaTextures(zdb::CNode *node)
{
    if (node->m_visual.size()) {
        zdb::CDynTexList *list = &m_alpha.m_dyntexlist;
        zdb::CVisualVector::iterator e = node->m_visual.end(), i = node->m_visual.begin();

        for (; i != e; ++i)
            list->Add(&(*i)->m_dynTexList);
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

/* an opacity pushed, multiplied into the one under it; the stack grown a slot at a time */
void CPipe::PushOpacity(float opacity)
{
    m_opacity_stack_index++;
    if (m_opacity_stack_index == m_opacity_stack_size) {
        m_opacity_stack = (float *)_realloc(m_opacity_stack, (m_opacity_stack_size + 1) * sizeof(float), 0, 0);
        m_opacity_stack_size++;
    }
    m_opacity_stack[m_opacity_stack_index] = opacity * m_opacity_stack[m_opacity_stack_index - 1];
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
