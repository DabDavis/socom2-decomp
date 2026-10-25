/* zrndr_pipe.cpp -- CPipe, the renderer's packet pipe (OVL1 0x336230?-0x33D22F): what it draws
 * and in which order, the texture loads queued in the GS FIFO between its packets. SOCOM 1's
 * class (its DWARF: 0x694 bytes; SOCOM II's thePipe runs to 0x720); the unit's name is ours (no
 * initialiser or assert names it). Defined in reverse of their image order (-inline deferred).
 * Written so far: the two FIFO signals, the opacity stack's push, the alpha pass's textures and
 * the overlay's kick. */

#include <eetypes.h>
#include <vector>
#include <list>

namespace zdb {
class CGSTexBuffer;

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
    int m_cmd;   // 0x0
    zdb::CGSTexBuffer *m_load;   // 0x4
    zdb::CGSTexBuffer *m_signal;   // 0x8
};   // 0xc bytes

/* the overlay the pipe kicks over the frame (SOCOM II's; the class and its names ours): a packet
 * of its own at 0xC, its GIF tag 0x30 into it */
struct ZPIPE_OVERLAY {
    unsigned char _0[0xc];   // 0x0
    unsigned char *m_packet;   // 0xc
};

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
    unsigned int m_overlayOn;   // 0x700 (ours)
    unsigned int m_overlayColor;   // 0x704: 0x40808080 at the start (ours)
    ZPIPE_OVERLAY *m_overlay;   // 0x708 (ours)
    unsigned char _70c[0x720 - 0x70c];   // 0x70c
};   // 0x720 bytes

/* zvid_init.cpp's: the 21-quadword packet's word set, and the packet sent (it reads no argument;
 * the pipe passes one) */
extern "C" void func_003B0D50(unsigned int v);
extern "C" void func_003B0CC0(unsigned int v);

/* SOCOM II's: the overlay's tag kicked through the FIFO, then its colour and packet sent */
void CPipe::KickOverlay()
{
    if (m_overlayOn && m_overlay) {
        u_long128 *p = zSysSprGetPacket(0);

        *p = *(u_long128 *)(m_overlay->m_packet + 0x30);
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
