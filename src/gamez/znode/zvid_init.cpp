/* zvid_init.cpp -- the video set-up: CVideo and the zVid_ calls (SOCOM 1's zvid_init.cpp, by its
 * static initialiser): OVL1 0x3AFBE0-..., after CTurret's unit, as SOCOM 1's 0x307790-0x308F3F
 * follows CMemCard. Defined in reverse of their image order (-inline deferred). */

#include <eetypes.h>

#include "../zsys/zsys_public.h"
#include "../zvid/zvid.h"

/* libgraph's image transfer (Sony's: two GIF tags around the BITBLTBUF/TRXPOS/TRXREG/TRXDIR
 * register pairs, 0x60 bytes) */
struct sceGsLoadImage {
    u_long128 giftag0;
    unsigned long regs[8];
    u_long128 giftag1;
};

/* and back (Sony's: a GIF tag and six register pairs, FINISH among them, 0x70 bytes) */
struct sceGsStoreImage {
    u_long128 giftag0;
    unsigned long regs[12];
};

extern "C" {
int sceGsSetDefLoadImage(sceGsLoadImage *lp, short dbp, short dbw, short dpsm, short x, short y,
                         short w, short h);
int sceGsExecLoadImage(sceGsLoadImage *lp, u_long128 *srcaddr);
int sceGsSetDefStoreImage(sceGsStoreImage *sp, short sbp, short sbw, short spsm, short x, short y,
                          short w, short h);
int sceGsExecStoreImage(sceGsStoreImage *sp, u_long128 *dstaddr);
int sceGsSyncPath(int mode, unsigned short timeout);
void sceDmaSendN(sceDmaChan *d, void *addr, int size);
}

/* the FIFO held while the GS is read (zsys_fifo.cpp's) */
void zSysFifoPause();
void zSysFifoResume(bool kick);

/* the stored image's width in pixels (256) and its pixel format (not named: SOCOM 1's reader
 * took both from its arguments) */
extern unsigned int data_003E1410;
extern short data_004A4540;

/* the video's statics (SOCOM 1's class; only what this unit's matched code touches) */
class CVideo {
public:
    static void Brightness(float b);
    static void WriteGsMemory(unsigned int addr, u_long128 *data, unsigned int size);
    static void ReadGsMemory(unsigned int addr, u_long128 *data, unsigned int size);
    static void Fade(float t, unsigned char r, unsigned char g, unsigned char b);
    static void Clear(bool all, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
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

/* the clear's GIF packet, ten quadwords: FRAME the third, RGBAQ the eighth, the sprite's far XYZ2 the
 * tenth (SOCOM 1's vramcode) */
extern u_long128 vramcode[10];

/* the final copy to the display, a GIF packet: its sprite's two XYZ2 words are the fifteenth and
 * seventeenth quadwords (SOCOM 1's finalcode, 0x40 bytes shorter) */
extern u_long128 finalcode[];
/* the clear colour's GS register word (SOCOM 1's zvid + 0x244, a pointer into a packet) */
extern unsigned long *zvid_clearColor;

/* a valve (zanim.h's class: only the value and the callbacks' call) */
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
    unsigned int m_type : 8;   // 0x4
    unsigned int m_flags : 8;   // 0x4
};

/* CValve::SetValue, inlined in both builds (zanim_menu.h's SetValveValue) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* the particles' rates (SOCOM 1's Particle::Instance statics) */
namespace Particle {
class Instance {
public:
    static int spawnFactor;
    static float ageFactor;
};
}

/* the level of detail by frame rate: the thresholds, and per level the LOD valve's value and the
 * particles' spawn and age rates (SOCOM 1's tables) */
extern int frameRateLimits[5];
extern int lodLevelLimits[5];
extern int spawnRateLimits[5];
extern float ageRateLimits[5];
extern CValve *lodLevel;

/* an address through the EE's uncached mirror */
#define UNCACHED(p) ((unsigned int)(p) | 0x20000000)

/* GS memory read back in 256x256 blocks, the FIFO paused (SOCOM 1's took the blocks' size and
 * format from its arguments) */
void CVideo::ReadGsMemory(unsigned int addr, u_long128 *data, unsigned int size)
{
    sceGsStoreImage si;

    DIntr();
    zSysFifoPause();
    sceGsSyncPath(0, 0);
    do {
        sceGsSetDefStoreImage(&si, addr, data_003E1410 >> 6, data_004A4540, 0, 0, 256, 256);
        FlushCache(0);
        sceGsExecStoreImage(&si, data);
        size -= 0x40000;
        data += 0x4000;
        addr += 1024;
        sceGsSyncPath(0, 0);
    } while (size);
    zSysFifoResume(true);
    EIntr();
}

/* a GS packet sent: through the FIFO, by reference or copied into the scratchpad packet behind its
 * DMA tag; or straight down the GIF channel, waited on (SOCOM 1's took no copy flag) */
void zvid_KickPacket(unsigned long *packet, int qwc, bool fifo, bool copy)
{
    if (fifo) {
        _word128 *p = (_word128 *)zSysSprGetPacket(0);

        p->u32[2] = 0x13000000;
        p->u32[3] = qwc | 0x50000000;
        if (copy) {
            CSPR::Copy(p + 1, (_word128 *)packet, qwc);
            p->u32[0] = qwc | 0x10000000;
            zSysFifoKick((u_long128 *)p, qwc + 1);
        } else {
            p->u32[0] = qwc | 0x30000000;
            p->u32[1] = (unsigned int)packet;
            zSysFifoKick((u_long128 *)p, 1);
        }
    } else {
        sceDmaSendN(zSys.DmaToGif, packet, qwc);
        sceGsSyncPath(0, 0);
    }
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

/* one GS register set through the GIF's A+D path: a DMA tag and two VIF codes, the GIF tag, the
 * pair -- one cursor step per quadword (SOCOM 1's wrote them all off the packet's start) */
void SetGSReg(short reg, unsigned long data)
{
    _word128 *packet = (_word128 *)zSysSprGetPacket(0);
    _word128 *p = packet;

    p->u32[0] = 0x10000002;
    p->u32[1] = 0;
    p->u32[2] = 0x11000000;
    p->u32[3] = 0x50000002;
    p++;
    p->u64[0] = 0x1000000000008001;
    p->u64[1] = 0xE;
    p++;
    p->u64[0] = data;
    p->u64[1] = reg;
    p++;
    zSysFifoKick((u_long128 *)packet, 3);
}

/* the scissor rectangle, its y scaled by the display's aspect (new in SOCOM II: SOCOM 1's took the
 * four as given) */
void zVid_Scissor(int x0, int y0, int x1, int y1)
{
    y0 = y0 * zVid.aspect[0];
    y1 = y1 * zVid.aspect[1];
    _word128 *packet = (_word128 *)zSysSprGetPacket(0);
    _word128 *p = packet;

    p->u32[0] = 0x10000002;
    p->u32[1] = 0;
    p->u32[2] = 0x11000000;
    p->u32[3] = 0x50000002;
    p++;
    p->u64[0] = 0x1000000000008001;
    p->u64[1] = 0xE;
    p++;
    p->u64[0] = (unsigned long)x0 | ((unsigned long)(x1 - 1) << 16) | ((unsigned long)y0 << 32) |
                ((unsigned long)(y1 - 1) << 48);
    p->u64[1] = 0x40;
    p++;
    zSysFifoKick((u_long128 *)packet, 3);
}

/* the final sprite's rectangle within the 640x448 display, in the GS's 12.4 coordinates offset to
 * 1728,1824 (new in SOCOM II; the name is not known) */
extern "C" void func_003B09E0(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    *(unsigned long *)UNCACHED(&finalcode[14]) =
        (unsigned long)(x * 16 + 0x6C00) | ((unsigned long)(y * 16 + 0x7200) << 16);
    *(unsigned long *)UNCACHED(&finalcode[16]) =
        (unsigned long)(0x9400 - (640 - w) * 16) | ((unsigned long)(0x8E00 - (448 - h) * 16) << 16);
}

/* the clear colour, opaque, Q 1.0 */
void zVid_ClearColor(unsigned int *rgb)
{
    *(unsigned long *)UNCACHED(zvid_clearColor) =
        (unsigned long)rgb[0] | ((unsigned long)rgb[1] << 8) | ((unsigned long)rgb[2] << 16) |
        ((unsigned long)0x80 << 24) | ((unsigned long)0x3F800000 << 32);
}

/* the frame rate's level of detail (SOCOM 1's) */
void SetDynamicLOD(int rate)
{
    int i;

    if (rate >= frameRateLimits[0])
        i = 0;
    else if (rate >= frameRateLimits[1])
        i = 1;
    else if (rate >= frameRateLimits[2])
        i = 2;
    else if (rate >= frameRateLimits[3])
        i = 3;
    else
        i = 4;
    SetValveValue(lodLevel, lodLevelLimits[i]);
    Particle::Instance::spawnFactor = spawnRateLimits[i];
    Particle::Instance::ageFactor = ageRateLimits[i];
}

void zVid_ZTestOnWriteSet(bool on)
{
    u_long128 *p = zSysSprGetPacket(0);

    *p = *(on ? &zon_writeon_dmaref : &zon_writeoff_dmaref);
    zSysFifoKick(p, 1);
}

/* the screen cleared by the clear packet: the frame buffer drawn to, 640x448 in its pixel format, or
 * the whole of GS memory (1024 wide from page 0) in the colour given (SOCOM 1's cleared the frame
 * buffer in the colour, sending the packet itself) */
void CVideo::Clear(bool all, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    if (all == false) {
        *(unsigned long *)UNCACHED(&vramcode[2]) =
            (unsigned long)zVid.frameBuf | 0xA0000 | ((unsigned long)zVid.framePsm << 24);
        *(unsigned long *)UNCACHED(&vramcode[9]) = 0x1C002800;
        zvid_KickPacket((unsigned long *)vramcode, 10, true, false);
    } else {
        *(unsigned long *)UNCACHED(&vramcode[2]) = 0x100000;
        *(unsigned long *)UNCACHED(&vramcode[9]) = 0x40004000;
        *(unsigned long *)UNCACHED(&vramcode[7]) =
            (unsigned long)r | ((unsigned long)g << 8) | ((unsigned long)b << 16) |
            ((unsigned long)a << 24) | ((unsigned long)0x3F800000 << 32);
        zvid_KickPacket((unsigned long *)vramcode, 10, true, false);
    }
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
