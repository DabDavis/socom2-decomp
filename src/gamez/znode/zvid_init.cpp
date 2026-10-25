/* zvid_init.cpp -- the video set-up: CVideo and the zVid_ calls (SOCOM 1's zvid_init.cpp, by its
 * static initialiser): OVL1 0x3AFBE0-..., after CTurret's unit, as SOCOM 1's 0x307790-0x308F3F
 * follows CMemCard. Defined in reverse of their image order (-inline deferred). */

#include <eetypes.h>

#include "../zsys/zsys_public.h"

/* libgraph's image transfer (Sony's: two GIF tags around the BITBLTBUF/TRXPOS/TRXREG/TRXDIR
 * register pairs, 0x60 bytes) */
struct sceGsLoadImage {
    u_long128 giftag0;
    unsigned long regs[8];
    u_long128 giftag1;
};

extern "C" {
int sceGsSetDefLoadImage(sceGsLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y,
                         short w, short h);
int sceGsExecLoadImage(sceGsLoadImage *lp, u_long128 *srcaddr);
}

/* the video's statics (SOCOM 1's class; only what this unit's matched code touches) */
class CVideo {
public:
    static void Brightness(float b);
    static void WriteGsMemory(unsigned int addr, u_long128 *data, unsigned int size);
    static void Fade(float t, unsigned char r, unsigned char g, unsigned char b);
    static int MaxFrameRate(int rate);

    static int m_defaultFrameRate;
    static int m_maxFrameRate;
    static unsigned int m_brightness;
};

/* the DMA references that switch z writes on and off (SOCOM 1's zon_writeon_dmaref; SOCOM II adds
 * the other: SOCOM 1's code loaded zon_writeon_dmaref on both arms) */
extern u_long128 zon_writeon_dmaref;
extern u_long128 zon_writeoff_dmaref;
/* the full-screen brightness quad, a GIF packet (SOCOM 1's; SOCOM II adds a ZBUF pair after the
 * TEST one, so its ALPHA pair is the fourth quadword and its RGBAQ the sixth) */
extern u_long128 brightnesscode[];
/* the fade colour (SOCOM 1's zvid + 0x260, the private state's; its start is not placed here) */
extern unsigned char zvid_fade[4];

/* an address through the EE's uncached mirror */
#define UNCACHED(p) ((unsigned int)(p) | 0x20000000)

int CVideo::MaxFrameRate(int rate)
{
    if (rate == 60)
        rate = -1;
    int old = m_maxFrameRate;
    m_maxFrameRate = rate;
    return old;
}

void CVideo::Fade(float t, unsigned char r, unsigned char g, unsigned char b)
{
    zvid_fade[0] = r;
    zvid_fade[1] = g;
    zvid_fade[2] = b;
    zvid_fade[3] = (unsigned int)(255.0f * t);
}

/* the image written to GS memory in 256x256 blocks of 32 bits (SOCOM 1's, byte for byte) */
void CVideo::WriteGsMemory(unsigned int addr, u_long128 *data, unsigned int size)
{
    sceGsLoadImage li;

    DIntr();
    sceDmaSync(zSys.DmaToGif, 0, 0);
    do {
        sceGsSetDefLoadImage(&li, addr, 4, 0, 0, 0, 256, 256);
        FlushCache(0);
        sceGsExecLoadImage(&li, data);
        sceDmaSync(zSys.DmaToGif, 0, 0);
        size -= 0x40000;
        data += 0x4000;
        addr += 1024;
    } while (size);
    EIntr();
}

void zVid_ZTestOnWriteSet(bool on)
{
    u_long128 *p = zSysSprGetPacket(0);

    *p = *(on ? &zon_writeon_dmaref : &zon_writeoff_dmaref);
    zSysFifoKick(p, 1);
}

/* 0.5 is neutral; below it the screen is darkened, above it brightened, by a full-screen quad of
 * that alpha (SOCOM 1's, with 127 where SOCOM II scales by 204.8) */
void CVideo::Brightness(float b)
{
    if (b == 0.5f) {
        m_brightness = 128;
    } else {
        b -= 0.5f;
        if (b < 0.0f) {
            m_brightness = (unsigned int)(204.8f * -b);
            *(unsigned long *)UNCACHED(&brightnesscode[3]) = 70;
            if (m_brightness == 0)
                m_brightness = 128;
        } else {
            m_brightness = (unsigned int)(204.8f * b);
            *(unsigned long *)UNCACHED(&brightnesscode[3]) = 73;
        }
    }
    *(unsigned long *)UNCACHED(&brightnesscode[5]) = (unsigned long)m_brightness << 24;
}
