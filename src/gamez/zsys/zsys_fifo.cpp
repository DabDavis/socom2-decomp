/* zsys_fifo.cpp -- the DMA FIFO: packets built in the scratchpad's two buffers are drained from
 * SPR into the MFIFO ring (OVL1 0x350950-0x351133). SOCOM 1 links the same functions in the same
 * order after zsys_init.cpp; the file name is ours. Defined in reverse of their image order
 * (-inline deferred). */

#include <stdio.h>

#include "zsys_public.h"

extern "C" int func_00196D18(const char *s);   /* newlib's puts (MAIN) */
extern "C" void func_003AFBA0(void);            /* SOCOM II's, before the end packet (not named) */
extern "C" int sceGsSyncPath(int mode, unsigned short timeout);

/* the MFIFO ring's base (SOCOM 1's mfifoBase, 0x100000) */
extern u_long128 mfifoBase[];

/* Sony's register bit-fields */
typedef struct {
    unsigned int DIR : 1;
    unsigned int p0 : 1;
    unsigned int MOD : 2;
    unsigned int ASP : 2;
    unsigned int TTE : 1;
    unsigned int TIE : 1;
    unsigned int STR : 1;
    unsigned int p1 : 7;
    unsigned int TAG : 16;
} tD_CHCR;

typedef struct {
    unsigned int DMAE : 1;
    unsigned int RELE : 1;
    unsigned int MFD : 2;
    unsigned int STS : 2;
    unsigned int STD : 2;
    unsigned int RCYC : 3;
    unsigned int p0 : 21;
} tD_CTRL;

#define D1_CHCR ((volatile unsigned int *)0x10009000)
#define D1_MADR ((volatile unsigned int *)0x10009010)
#define D1_QWC ((volatile unsigned int *)0x10009020)
#define D1_TADR ((volatile unsigned int *)0x10009030)
#define D2_CHCR ((volatile unsigned int *)0x1000A000)
#define D2_TADR ((volatile unsigned int *)0x1000A030)
#define D8_CHCR ((volatile unsigned int *)0x1000D000)
#define D8_MADR ((volatile unsigned int *)0x1000D010)
#define D8_QWC ((volatile unsigned int *)0x1000D020)
#define D8_SADR ((volatile unsigned int *)0x1000D080)
#define D9_CHCR ((volatile unsigned int *)0x1000D400)
#define D9_MADR ((volatile unsigned int *)0x1000D410)
#define D9_QWC ((volatile unsigned int *)0x1000D420)
#define D9_SADR ((volatile unsigned int *)0x1000D480)
#define D_CTRL ((volatile unsigned int *)0x1000E000)
#define D_RBSR ((volatile unsigned int *)0x1000E040)
#define D_RBOR ((volatile unsigned int *)0x1000E050)

#define D8_CHCR_F ((tD_CHCR *)0x1000D000)
#define D_CTRL_F ((tD_CTRL *)0x1000E000)

/* a scratchpad buffer: 499 quadwords */
#define SPR_PACKET_SIZE 7984

volatile unsigned int *mfifoDrainTADR;  /* 0x49E2F8: the draining channel's TADR (SOCOM 1's) */
static bool s_fifoSync;                 /* 0x49E300: SOCOM II's, wait for the drain to empty */
u_long128 *sprBeingKicked;              /* 0x49E308 (SOCOM 1's) */
u_long128 *sprBeingWritten;             /* 0x49E310 (SOCOM 1's) */

_word128 *CSPR::m_sprPacket;            /* 0x49E330 (SOCOM 1's) */
_word128 *CSPR::m_sprPacketEnd;         /* 0x49E338 (SOCOM 1's) */

/* VIF1's channel as it was when the FIFO was paused (SOCOM II's; names ours) */
static unsigned int s_d1Chcr;           /* 0x49E340 */
static unsigned int s_d1Tadr;           /* 0x49E348 */
static unsigned int s_d1Qwc;            /* 0x49E350 */
static unsigned int s_d1Madr;           /* 0x49E358 */
static unsigned int s_dCtrl;            /* 0x49E360 */
static bool s_fifoPaused;               /* 0x49E368 */

/* SOCOM II's: VIF1's channel stopped and its registers kept, the cycle-steal off (name ours) */
void zSysFifoPause()
{
    s_fifoPaused = true;
    *D1_CHCR &= 0xfeff;
    asm volatile { sync }
    s_d1Chcr = *D1_CHCR;
    s_d1Tadr = *D1_TADR;
    s_d1Qwc = *D1_QWC;
    s_d1Madr = *D1_MADR;
    s_dCtrl = *D_CTRL;
    *D_CTRL = 1;
}

/* SOCOM II's: the channel's registers put back, and restarted if asked (name ours) */
void zSysFifoResume(bool restart)
{
    s_fifoPaused = false;
    *D_CTRL = s_dCtrl;
    *D1_MADR = s_d1Madr;
    *D1_QWC = s_d1Qwc;
    *D1_TADR = s_d1Tadr;
    *D1_CHCR = s_d1Chcr;
    if (restart)
        *D1_CHCR |= 0x100;
    asm volatile { sync }
}

void zSysFifoInit()
{
    *D_RBOR = (unsigned int)mfifoBase;
    *D_RBSR = 0x7FFF0;
    asm volatile { sync }
}

void zSysFifoDisable()
{
    sceGsSyncPath(0, 0);
    D_CTRL_F->MFD = 0;
    asm volatile { sync }
}

void zSysFifoEnable(unsigned int mode)
{
    *D_RBOR = (unsigned int)mfifoBase;
    *D_RBSR = 0x7FFF0;
    switch (mode) {
    case 1:
        while (*D1_CHCR & 0x100)
            ;
        D_CTRL_F->MFD = 2;
        *D8_MADR = (unsigned int)mfifoBase;
        *D1_TADR = (unsigned int)mfifoBase;
        *D1_CHCR = 0x145;
        mfifoDrainTADR = D1_TADR;
        break;
    case 2:
        while (*D2_CHCR & 0x100)
            ;
        D_CTRL_F->MFD = 3;
        *D8_MADR = (unsigned int)mfifoBase;
        *D2_TADR = (unsigned int)mfifoBase;
        *D2_CHCR = 0x144;
        mfifoDrainTADR = D2_TADR;
        break;
    default:
        zSysFifoDisable();
        break;
    }
    asm volatile { sync }
}

u_long128 *zSysSprGetPacket(u_long128 **end)
{
    sprBeingWritten = (u_long128 *)0x70000000 + zSys.sprPx * (SPR_PACKET_SIZE / 16);
    if (sprBeingWritten == sprBeingKicked) {
        zSys.sprPx ^= 1;
        sprBeingWritten = (u_long128 *)0x70000000 + zSys.sprPx * (SPR_PACKET_SIZE / 16);
    }
    if (end)
        *end = zSys.sprPacketEnd[zSys.sprPx] - 1;
    zSys.sprPx ^= 1;
    return sprBeingWritten;
}

void zSysFifoEnd()
{
    unsigned int *p;

    func_003AFBA0();
    p = (unsigned int *)zSysSprGetPacket(0);
    p[0] = 0x70000000;
    p[1] = 0;
    p[2] = 0x11000000;
    p[3] = 0x11000000;
    zSysFifoKick((u_long128 *)p, 1);
}

/* SOCOM II's (name ours) */
void zSysFifoSetSync(bool sync)
{
    s_fifoSync = sync;
}

void zSysFifoKick(u_long128 *packet, unsigned int qwc)
{
    static unsigned int tadr;
    static unsigned int madr;
    static unsigned int remain;
    unsigned int size;

    sprBeingKicked = packet;
    if ((unsigned int)packet < 0x70000000 + 2 * SPR_PACKET_SIZE) {
        u_long128 *tail = packet + qwc;

        if (((unsigned int *)tail)[-4] & 0x30000000) {
            *(unsigned long *)tail = 0x1111111110000000;
            ((unsigned long *)tail)[1] = 0;
            qwc++;
        }
    }
    size = qwc * 16;
    if (!s_fifoSync) {
        do {
            while ((tadr = *mfifoDrainTADR) == 0)
                ;
            madr = *D8_MADR;
            remain = tadr == madr ? 0x80000 : (tadr + 0x80000 - madr) & 0x7FFF0;
        } while (remain <= size);
    } else if (*D1_CHCR & 0x100) {
        do {
            while ((tadr = *mfifoDrainTADR) == 0)
                ;
            madr = *D8_MADR;
        } while (tadr != madr);
    }
    while (*D9_CHCR & 0x100)
        func_00196D18("D9 Busy");
    while (*D8_CHCR & 0x100)
        func_00196D18("D8 Busy");
    *D8_SADR = (unsigned int)packet;
    *D8_QWC = qwc;
    D8_CHCR_F->STR = 1;
}

_word128 *CSPR::Kick(_word128 *end, bool getNew)
{
    int n = end - m_sprPacket;

    if (n > 0) {
        zSysFifoKick((u_long128 *)m_sprPacket, n);
        if (getNew)
            m_sprPacket = (_word128 *)zSysSprGetPacket((u_long128 **)&m_sprPacketEnd);
    }
    return m_sprPacket;
}

_word128 *CSPR::Copy(_word128 *to, _word128 *from, int qwc)
{
    while (*D9_CHCR & 0x100)
        ;
    *D9_MADR = (unsigned int)from;
    *D9_SADR = (unsigned int)to;
    *D9_QWC = qwc;
    *D9_CHCR = 0x101;
    return to + qwc;
}
