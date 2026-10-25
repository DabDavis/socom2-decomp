/* zrndr_pipe.cpp -- CPipe, the renderer's packet pipe (OVL1 0x336230?-0x33D22F): what it draws
 * and in which order, the texture loads queued in the GS FIFO between its packets. SOCOM 1's
 * class (its DWARF: 0x694 bytes; SOCOM II's thePipe runs to 0x720); the unit's name is ours (no
 * initialiser or assert names it). Defined in reverse of their image order (-inline deferred).
 * Written so far: the two FIFO signals, the opacity stack's push, the alpha pass's textures and
 * the overlay's kick. */

#include <eetypes.h>
#include <vector>
#include <list>
#include "../zsys/zsys_public.h"
#include "../zsys/zsys.h"

namespace zdb {
/* a texture buffer the GS loads (SOCOM 1's DWARF, 0x40 bytes): its DMA chain at 0x18, the word
 * after it tested before the chain is sent */
class CGSTexBuffer {
public:
    unsigned char _0[0x18];   // 0x0
    void *m_chainp;   // 0x18
    unsigned int m_1c;   // 0x1c (SOCOM 1 tested its packet vector's size, at 0x20)
};

/* the dynamic textures a visual or a pass uses (znode.h's class; SOCOM II's Add of another
 * list, 0x357A00, which znode.h does not declare) */
class CDynTexList {
public:
    void Add(CDynTexList *list);

    std::vector<void *> m_tex_list;   // 0x0
    std::vector<void *> m_pal_list;   // 0xc
};   // 0x18 bytes

/* znode.h's classes, as much as the pipe reads (that header brings its own CDynTexList) */
class CVisual {
public:
    unsigned char _0[0x48];   // 0x0
    CDynTexList m_dynTexList;   // 0x48
};
class CVisualVector : public std::vector<CVisual *> {
};
class CNode {
public:
    unsigned char _0[0x84];   // 0x0
    CVisualVector m_visual;   // 0x84
};
}

/* the system's packet ring (zsys_main.cpp): a packet in scratchpad, and the packet kicked */
u_long128 *zSysSprGetPacket(u_long128 **p);
void zSysFifoKick(u_long128 *p, unsigned int qwc);
void *_realloc(void *p, unsigned int size, const char *file, int line);

/* zdb::CGSTexBuffer's upload into the GS, not named yet (0x356BE0) */
extern "C" void func_00356BE0(zdb::CGSTexBuffer *buf, int flags);

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

namespace zdb {
/* a texture (SOCOM 1's DWARF: TEXTURE_PARAMS's width and height first, its pixels at 0x18, the
 * DMA reference that sends it at 0x30) */
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
    unsigned char _4[0x18 - 0x4];   // 0x4
    unsigned char *m_buffer;   // 0x18
    unsigned char _1c[0x30 - 0x1c];   // 0x1c
    u_long128 m_dmaRef;   // 0x30
};
/* a texture by name (SOCOM 1's DWARF, 0x14 bytes) */
class CTexHandle {
public:
    char *m_name;   // 0x0
    int m_count;   // 0x4
    unsigned int m_gsAddr;   // 0x8
    CTexture *m_texture;   // 0xc
    unsigned char m_libref;   // 0x10
};
}

extern "C" void FlushCache(int op);
extern "C" int rand();
/* zdb::CTexture's, not named yet: its colours set (0x354A90, a tail call through its palette) */
extern "C" void func_00354A90(zdb::CTexture *tex, int r, int g, int b);
/* zvid_init.cpp's: the 21-quadword packet's four corners set to one point */
extern "C" void func_003B0CE0(unsigned int x, unsigned int y);

class CPipe {
public:
    /* the translucent objects drawn last (SOCOM 1's CPipe::CAlpha, 0x30 bytes in both) */
    class CAlpha {
    public:
        void *m_camera;   // 0x0
        void *m_visualList;   // 0x4
        void *m_next;   // 0x8
        std::list<void *> m_drawList;   // 0xc
        zdb::CDynTexList m_dyntexlist;   // 0x18
    };   // 0x30 bytes

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
    unsigned char _608[0x638 - 0x608];   // 0x608: SOCOM 1's assets, world, camera (0x628), cell, node
    CAlpha m_alpha;   // 0x638 (SOCOM 1: 0x654)
    /* the opacities pushed, each the product of those under it (SOCOM 1's at 0x648) */
    float *m_opacity_stack;   // 0x668
    int m_opacity_stack_size;   // 0x66c
    int m_opacity_stack_index;   // 0x670
    unsigned char _674[0x700 - 0x674];   // 0x674: the delayed nodes from 0x674
    /* the noise drawn over the frame (SOCOM II's; CCoreState::Init sets "noise50.tif" -- the
     * names ours) */
    unsigned int m_overlayOn;   // 0x700
    unsigned int m_overlayColor;   // 0x704: 0x40808080 at the start
    zdb::CTexHandle *m_overlay;   // 0x708
    unsigned char _70c[0x720 - 0x70c];   // 0x70c
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
    void Clear();
    void Release(int handle);

    unsigned int _0;   // 0x0
    std::vector<CFlare *> m_flares;   // 0x4
};

/* SOCOM II's: every flare deleted, the vector emptied and its storage given back (FreeVector
 * inlined one level: its pieces called) */
void CFlareSystem::Clear()
{
    for (std::vector<CFlare *>::iterator i = m_flares.begin(); i != m_flares.end(); ++i)
        delete *i;
    m_flares.clear();
    FreeVector(m_flares);
}

/* SOCOM II's: a flare let go of -- its active bit cleared, when the handle is one */
void CFlareSystem::Release(int handle)
{
    if (handle >= 0 && handle < m_flares.size())
        m_flares.at(handle)->SetActive(false);
}

extern CPipe thePipe;   // 0x4887C0
/* the thread a flushed FIFO wakes (SOCOM II's; not named) */
extern int data_003E1418;
/* zdb::CGSTexBuffer's, not named yet (SOCOM 1's handler calls UpdateTextureSelects here) */
extern "C" void func_00355DE0(zdb::CGSTexBuffer *buf);

/* SOCOM 1's: VIF1's interrupt -- the next texture command taken: a flush wakes its thread, a load
 * sends the buffer's chain through DMA channel 2 once the GIF channel is idle, a signal updates its
 * buffer; VIF1 restarted */
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
                if (c->m_load->m_1c) {
                    *(volatile unsigned int *)0x1000A020 = 0;
                    *(volatile unsigned int *)0x1000A030 = (unsigned int)c->m_load->m_chainp & 0x0FFFFFFF;
                    *(volatile unsigned int *)0x1000A000 = 325;
                    asm volatile { sync }
                }
            }
            if (c->m_signal)
                func_00355DE0(c->m_signal);
            *(volatile unsigned int *)0x10003C10 = 8;
            asm volatile { sync }
        }
    }
    asm volatile { sync }
    asm volatile { ei }
    return 0;
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
        unsigned char *pixels = t->m_buffer;

        for (unsigned int i = 0; i < n; i++) {
            float r = 4.656613e-10f * rand();

            pixels[i] = (int)(255.0f * r);
        }
        FlushCache(0);
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

/* a buffer uploaded, then a command queued and a FIFO packet kicked that interrupts when the
 * GS reaches it (its marker word 0x44444444); the second argument is SOCOM 1's, unused */
void CPipe::FIFO_Signal(zdb::CGSTexBuffer *buf, unsigned int *data)
{
    if (!buf)
        return;
    func_00356BE0(buf, 0);
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
