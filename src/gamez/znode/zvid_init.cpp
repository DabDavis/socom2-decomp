/* zvid_init.cpp -- the video set-up: CVideo and the zVid_ calls (SOCOM 1's zvid_init.cpp, by its
 * static initialiser): OVL1 0x3AFBE0-0x3B2CA3, after CTurret's unit, as SOCOM 1's 0x307790-0x308F3F
 * follows CMemCard. Defined in reverse of their image order (-inline deferred). The unit's last
 * function is zVid_Assert 0x3B2C80 (SOCOM 1's zvid_init.cpp holds it too, 0x307A40, and so does
 * MAIN's copy of the unit, src/main/zvid_init.cpp): it was src/gamez/zvid/zvid.cpp until the weapons'
 * round 21, a file holding nothing else. */

#include <eetypes.h>
#include <iostream>

#include "../zsys/zsys_public.h"
#include "../zutil/zutil.h"
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
int sceGsSyncV(int mode);
}

/* a point or a size in pixels (SOCOM 1's IPNT2D) */
struct IPNT2D {
    int x;
    int y;
};

/* the FIFO held while the GS is read (zsys_fifo.cpp's) */
void zSysFifoPause();
void zSysFifoResume(bool kick);
void zSysFifoEnd();

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
    static void RestoreImage(char *name);

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

enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };

class CValve {
public:
    void MakeCallbacks(VALVE_STATE state);
    static CValve *Create(const char *name, unsigned int value, VALVE_TYPE type);

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

/* the machine's reset (OVL1 0x22ED10, st_reboot.cpp's) */
void RebootAfterError();

/* a failed check stops the machine. SOCOM 1's prints the file and line in the colour given first;
 * SOCOM II's callers pass neither, and it only resets (the unit's last function: nothing here
 * calls it, the allocators' check in zsys_memory.cpp does) */
void zVid_Assert(bool ok, unsigned int color, char *file, int line)
{
    if (!ok)
        RebootAfterError();
}

/* the last sixteen frames' times, a ring, and the newest's place in it (new in SOCOM II, not
 * named) */
extern float data_004A4560[16];
extern int data_004A45A0;

/* the mean of the last n frames' times: all sixteen in order, or n back from the newest (new in
 * SOCOM II, not named). OPEN, 17 instructions, the same size: the unrolled walk's index and its
 * bound (n - 8) trade v1 and a1; the single exit gave the sum $f0 and declaring i first its a2 (the
 * index declared before i, at the top, unsigned, a while loop: no better). Round 29, by
 * `tools/igreplay.py --target`: those two are the only differing nodes, idx (v35) and the unroller's
 * bound, a front-end temporary (v36, `n - 8`, defined right after idx's load) -- every placement
 * numbering the bound BEFORE idx gives the target's registers (91 of 3,481). Not found in source:
 * the step in the for, two statements, the for-init, idx unsigned, a named count, `last - i` (0x1F0),
 * a pointer walk (0x188), a while (no unrolling) */
extern "C" float func_003B1BF0(unsigned int n)
{
    float sum = 0.0f;

    if (n == 16) {
        for (unsigned int i = 0; i < 16; i++)
            sum += data_004A4560[i];
        sum /= 16.0f;
    } else {
        unsigned int i;
        int idx = data_004A45A0;

        for (i = 0; i < n; i++)
            sum += data_004A4560[idx--];
        sum /= n;
    }
    return sum;
}

/* the EE kernel's (eekernel.h) */
struct ThreadParam {
    int status;
    void (*entry)(void *);
    void *stack;
    int stackSize;
    void *gpReg;
    int initPriority;
    int currentPriority;
    unsigned int attr;
    unsigned int option;
    int waitType;
    int waitId;
    int wakeupCount;
};
extern "C" {
int CreateThread(ThreadParam *param);
int StartThread(int id, void *arg);
}
extern char _gp[];

/* the video thread's body, its stack and its id (new in SOCOM II, not named) */
extern "C" void func_003B1DD0(void *arg);
extern unsigned char data_004A45D0[];
extern int data_003E1418;

/* the video thread started at priority 4 on its 64 KB stack (new in SOCOM II, not named; pinned:
 * zVid_Init calls it out of line) */
#pragma auto_inline off
extern "C" void func_003B2450()
{
    ThreadParam param;

    param.entry = func_003B1DD0;
    param.stack = data_004A45D0;
    param.stackSize = 0x10000;
    param.initPriority = 4;
    param.gpReg = _gp;
    data_003E1418 = CreateThread(&param);
    StartThread(data_003E1418, 0);
}
#pragma auto_inline reset

/* a saved screen image read back into the render buffer, a band of 64 rows at a time (SOCOM 1's
 * RestoreImage read the file whole and sent it in four fixed blocks) */
void CVideo::RestoreImage(char *name)
{
    CFileIO file;

    if (file.Open(name, 1)) {
        int addr;
        unsigned int block;
        u_long128 *buf;
        unsigned int step;
        int size;

        size = file.GetSize();
        block = zVid.renderWidth << 8;
        step = block >> 8;
        buf = (u_long128 *)_memalign(128, block, 0, 0);
        addr = zVid.renderBuf << 5;

        if (size > 0) {
            do {
                sceGsLoadImage li;
                unsigned int n = file.fread(buf, block);

                sceGsSetDefLoadImage(&li, addr, zVid.renderWidth >> 6, zVid.psm, 0, 0,
                                     zVid.renderWidth, n / 2560);
                FlushCache(0);
                sceDmaSync(zSys.DmaToGif, 0, 0);
                sceGsExecLoadImage(&li, buf);
                sceDmaSync(zSys.DmaToGif, 0, 0);
                size -= block;
                addr += step;
            } while (size > 0);
        }
        file.Close();
        _free(buf, 0, 0);
    }
}

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

/* a rectangle of GS memory read back at once, the FIFO paused, in the render width (new in SOCOM
 * II, not named; pinned: the depth read calls it out of line) */
#pragma auto_inline off
extern "C" void func_003B1800(int addr, IPNT2D *pos, IPNT2D *size, int psm, u_long128 *data)
{
    sceGsStoreImage si;

    DIntr();
    zSysFifoPause();
    sceGsSyncPath(0, 0);
    sceGsSetDefStoreImage(&si, addr, zVid.renderWidth >> 6, psm, pos->x, pos->y, size->x, size->y);
    FlushCache(0);
    sceGsExecStoreImage(&si, data);
    zSysFifoResume(true);
    EIntr();
}
#pragma auto_inline reset

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

/* the video private state's GS modes (SOCOM 1's zvid + 0x248/0x24c/0x250, the same offsets from
 * SOCOM II's 0x4A4180), the display's height, its double buffer and its one-pixel store image */
extern int data_004A43C8;   /* interlace */
extern int data_004A43CC;   /* field/frame */
extern int data_004A43D0;   /* the video mode */
extern int data_004A43D4;   /* the display's height */
struct sceGsDBuff;
extern sceGsDBuff data_004A4180;
extern sceGsStoreImage data_004A43E0;
extern IPNT2D data_003E1370;   /* the store image's size: one pixel */

extern "C" {
int sceGsResetGraph(short mode, short inter, short omode, short ffmode);
int sceGsSetDefDBuff(sceGsDBuff *dp, short psm, short w, short h, short ztest, short zpsm,
                     short clear);
}

/* the GS's PMODE and EXTWRITE registers */
#define GS_PMODE ((volatile unsigned long *)0x12000000)
#define GS_EXTWRITE ((volatile unsigned long *)0x120000D0)

/* the display mode set: NTSC, PAL or 480p, the GS reset on request, the double buffer and the
 * one-pixel store image set up (SOCOM 1's, with SOCOM II's aspect and the 480p offsets) */
void zvid_SetVideoMode(_zvid_mode mode, bool reset)
{
    zVid.syncMode = mode;
    switch (mode) {
    case ZVID_MODE_NTSC:
        data_004A43D0 = 2;
        data_004A43C8 = 1;
        data_004A43D4 = 448;
        zVid.pcrtcOffset[1] = 1;
        zVid.hblnkRate = 15734.25f;
        zVid.pcrtcOffset[0] = 4;
        data_004A43CC = 0;
        zVid.framePsm = 0;
        zVid.aspect[0] = 1.0f;
        zVid.aspect[1] = 1.0f;
        zVid.modeOffset[0] = 0;
        zVid.modeOffset[1] = 0;
        break;
    case ZVID_MODE_PAL:
        data_004A43C8 = 1;
        zVid.pcrtcOffset[1] = 1;
        data_004A43D0 = 3;
        data_004A43D4 = 512;
        data_004A43CC = 0;
        zVid.hblnkRate = 15549.999f;
        zVid.pcrtcOffset[0] = 4;
        zVid.aspect[0] = 1.0f;
        zVid.framePsm = 0;
        zVid.aspect[1] = 1.1428572f;
        zVid.modeOffset[0] = 0;
        zVid.modeOffset[1] = 0;
        break;
    case ZVID_MODE_DTV480P:
        zVid.pcrtcOffset[0] = 2;
        data_004A43C8 = 0;
        data_004A43D0 = 80;
        data_004A43CC = 1;
        data_004A43D4 = 448;
        zVid.pcrtcOffset[1] = 1;
        zVid.hblnkRate = 31468.5f;
        zVid.framePsm = 0;
        zVid.aspect[0] = 1.0f;
        zVid.aspect[1] = 1.0f;
        zVid.modeOffset[1] = 18;
        zVid.modeOffset[0] = 0;
        break;
    }
    if (reset) {
        sceGsSyncV(0);
        *GS_PMODE = 0;
        asm volatile { sync }
        sceGsResetGraph(0, data_004A43C8, data_004A43D0, data_004A43CC);
        *GS_PMODE = 0;
        asm volatile { sync }
    }
    sceGsSetDefDBuff(&data_004A4180, zVid.psm, zVid.renderWidth, zVid.renderHeight, 2, 58, 1);
    zSysTimerInit();
    sceGsSetDefStoreImage(&data_004A43E0, zVid.renderBuf << 5, zVid.renderWidth >> 6, 0, 0, 0,
                          data_003E1370.x, data_003E1370.y);
    FlushCache(0);
}

/* the video system set up: the display 640x448 in the GS's 1728,1824 window, its buffers, the z
 * range, the fade cleared, the LOD valve made, the video thread started and the mode set (SOCOM 1's,
 * with SOCOM II's window, second buffer and thread). OPEN, 11 instructions, the same size: the two
 * zero byte stores (0x7C, 0x7D) sink with the fade alpha's 255 (in v0) to the 0x90 flags, where the
 * target keeps them in source order with 255 hoisted into v1 at the top, as SOCOM 1's does -- every
 * position of the two stores (a 23x23 sweep), bool/char/bitfield/volatile fields, chained and 0.0f
 * forms leave it at 11; the 255's register looks like the cause (an alpha set some other way?) */
void zVid_Init(_zvid_mode mode)
{
    zVid.runTime = 0;
    zVid.swapMode = 0;
    zVid.renderWidth = 640;
    zVid.renderHeight = 448;
    zVid.screenLeft = 1728;
    zVid.screenRight = 2368;
    zVid.screenTop = 1824;
    zVid.screenBottom = 2272;
    zVid.renderBuf = 140;
    zVid.renderBuf2 = 280;
    zVid.ztest_on = &zon_writeon_dmaref;
    zVid.ztest_off = &zon_writeoff_dmaref;
    zVid.pcrtcDo = 0;
    zVid.iMinZ = 10;
    zVid.iMaxZ = 0xFFFF;
    zVid.textureBaseAddr = 11200;
    zVid.textureEndAddr = 16384;
    zVid.flag90 = 0;
    zVid.flag91 = 1;
    zVid.finalBlend = 0;
    zVid.displayBuf = 0;
    zVid.psm = 0;
    *(unsigned long *)zvid_fade = 0;
    zvid_fade[3] = 255;
    zVid.minZ = zVid.iMinZ;
    zVid.maxZ = zVid.iMaxZ;
    if (!lodLevel)
        lodLevel = CValve::Create("lodLevel", 1, VALVE_PERM);
    func_003B2450();
    zvid_SetVideoMode(mode, false);
    zVid.videoMode = mode;
    *GS_EXTWRITE = 0;
    zVid.frameNumber = 0;
}

/* the video system shut down: nothing left to do (SOCOM 1's deleted its CTTY; CGame::StopEngine
 * calls it at the same place SOCOM 1's calls zVid_Uninit, after DeletePad(1)) */
void zVid_Uninit()
{
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

/* the two GS packets new in SOCOM II (not named): 31 quadwords sent n times (its FRAME words the
 * eleventh and nineteenth, its two coordinates the twenty-fourth and twenty-sixth), and 21 sent once
 * (a word the fourth, its four corners' coordinates every fourth from the eighth) */
extern u_long128 data_003E0C40[31];
extern u_long128 data_003E0E40[21];

/* a rectangle of the display (x, y, width, height: the name ours) */
struct ZVID_RECT {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
};

/* one pixel read back (1 by 1, from page 0x2300) */
extern IPNT2D data_003E1370;

/* the point within the 640x448 display, in the GS's coordinates (inline; the name ours) */
static inline bool OnScreen(const float *p)
{
    if (p[0] < 1728.0f || !(p[0] <= 2368.0f) || p[1] < 1824.0f || !(p[1] <= 2272.0f))
        return false;
    return true;
}

/* the depth under a point of the display, read back from the z buffer: false off the screen (new in
 * SOCOM II, not named) */
extern "C" bool func_003B0E70(const float *p, unsigned int *z)
{
    if (OnScreen(p)) {
        IPNT2D pos = {0};
        u_long128 buf;

        pos.x = (int)(p[0] - 1728.0f);
        pos.y = (int)(p[1] - 1824.0f);
        func_003B1800(0x2300, &pos, &data_003E1370, 58, &buf);
        *z = *(unsigned short *)&buf;
        return true;
    }
    return false;
}

/* the 31-quadword packet's two coordinate words (new in SOCOM II, not named) */
extern "C" void func_003B0E50(unsigned int v)
{
    *(unsigned long *)UNCACHED(&data_003E0C40[25]) = v;
}

extern "C" void func_003B0E30(unsigned long v)
{
    *(unsigned long *)UNCACHED(&data_003E0C40[23]) = v;
}

/* the 31-quadword packet aimed at the frame buffer (its FRAME words: width 640, the format's and
 * the buffer's base) and sent n times (new in SOCOM II, not named) */
extern "C" void func_003B0D70(int n)
{
    *(unsigned long *)UNCACHED(&data_003E0C40[10]) =
        (unsigned long)(zVid.renderBuf << 5) | ((unsigned long)0xA802 << 16 | 0x8000 | (unsigned long)6 << 32);
    *(unsigned long *)UNCACHED(&data_003E0C40[18]) = (unsigned long)zVid.renderBuf | 0xA0000;
    while (n--)
        zvid_KickPacket((unsigned long *)data_003E0C40, 31, true, true);
}

/* the 21-quadword packet's word (new in SOCOM II, not named) */
extern "C" void func_003B0D50(unsigned int v)
{
    *(unsigned long *)UNCACHED(&data_003E0E40[3]) = v;
}

/* the 21-quadword packet's four corners set to one point, in 12.4 (new in SOCOM II, not named) */
extern "C" void func_003B0CE0(unsigned int x, unsigned int y)
{
    unsigned long xy = (unsigned long)(x * 16) | ((unsigned long)(y * 16) << 16);

    *(unsigned long *)UNCACHED(&data_003E0E40[7]) = xy;
    *(unsigned long *)UNCACHED(&data_003E0E40[11]) = xy;
    *(unsigned long *)UNCACHED(&data_003E0E40[15]) = xy;
    *(unsigned long *)UNCACHED(&data_003E0E40[19]) = xy;
}

/* the 21-quadword packet sent, copied (new in SOCOM II, not named) */
extern "C" void func_003B0CC0()
{
    zvid_KickPacket((unsigned long *)data_003E0E40, 21, true, true);
}

/* the final copy sent: the frame buffer read at its base and width, the texture's format and the
 * flag's alpha (new in SOCOM II, not named) */
extern "C" void func_003B0C00(bool fifo)
{
    *(unsigned long *)&finalcode[1] = (unsigned long)zVid.displayBuf | 0xA0000;
    *(unsigned long *)&finalcode[5] =
        (unsigned long)(zVid.renderBuf << 5) | ((unsigned long)0xA802 << 16 | 0x8000 | (unsigned long)6 << 32);
    if (zVid.finalBlend)
        *(unsigned long *)&finalcode[6] = 96;
    else
        *(unsigned long *)&finalcode[6] = 0;
    FlushCache(0);
    zvid_KickPacket((unsigned long *)finalcode, 17, fifo, false);
}

/* the final sprite's texture rectangle, in 12.4 (new in SOCOM II, not named) */
extern "C" void func_003B0B90(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
    *(unsigned long *)UNCACHED(&finalcode[13]) = (unsigned long)(x * 16) | ((unsigned long)(y * 16) << 16);
    *(unsigned long *)UNCACHED(&finalcode[15]) = (unsigned long)(w * 16) | ((unsigned long)(h * 16) << 16);
}

/* the same of a rectangle, through it (inline) */
extern "C" void func_003B0B10(ZVID_RECT *r)
{
    func_003B0B90(r->x, r->y, r->w, r->h);
}

extern "C" void func_003B09E0(unsigned int x, unsigned int y, unsigned int w, unsigned int h);

/* func_003B09E0 of a rectangle, through it (inline; new in SOCOM II, not named). OPEN, 35
 * instructions, the same size: scheduling -- the target loads h, y, w, x and keeps more of the
 * products live (t3-t5); the inline call, the fields into locals (both orders) and the two words
 * built first all schedule as the by-value setter does */
extern "C" void func_003B0A70(ZVID_RECT *r)
{
    func_003B09E0(r->x, r->y, r->w, r->h);
}

/* the GS's FRAME register (Sony's sceGsFrame) */
struct sceGsFrame {
    unsigned long FBP : 9;
    unsigned long pad09 : 7;
    unsigned long FBW : 6;
    unsigned long pad22 : 2;
    unsigned long PSM : 6;
    unsigned long pad30 : 2;
    unsigned long FBMSK : 32;
};

/* the two ten-quadword packets the swap sends by reference (new in SOCOM II, not named): FRAME the
 * second quadword, a coordinate pair the third */
extern u_long128 data_003E0FC0[10];
extern u_long128 data_003E1080[10];

/* a packet sent through the FIFO by reference (inline: SOCOM 1's zvid_KickPacket's FIFO arm, the
 * name ours) */
static inline void KickRef(u_long128 *packet, int qwc)
{
    _word128 *p = (_word128 *)zSysSprGetPacket(0);

    p->u32[0] = qwc | 0x30000000;
    p->u32[1] = (unsigned int)packet;
    p->u32[2] = 0x13000000;
    p->u32[3] = qwc | 0x50000000;
    zSysFifoKick((u_long128 *)p, 1);
}

/* the frame presented: in the timer's sync or the whole-copy mode, the display cleared and the
 * final copy sent; otherwise one of the two packets aimed at the render buffer; then that packet
 * sent (new in SOCOM II, not named) */
extern "C" void func_003B07B0(bool fifo)
{
    u_long128 *pkt = data_003E0FC0;

    if (zVid.syncMode == 2 || zVid.swapMode == 1) {
        CVideo::Clear(false, 0, 0, 0, 0);
        ((sceGsFrame *)&data_003E0FC0[1])->FBP = zVid.renderBuf;
        func_003B0C00(fifo);
    } else {
        if (zVid.swapMode == 2) {
            pkt = data_003E1080;
            *(unsigned long *)&data_003E1080[1] = (unsigned long)zVid.renderBuf | 0xA0000;
            *(unsigned long *)&data_003E1080[2] = 0x6C00 | ((unsigned long)0x7900 << 32);
        } else {
            *(unsigned long *)&data_003E0FC0[1] = (unsigned long)zVid.renderBuf | 0xA0000;
            *(unsigned long *)&data_003E0FC0[2] = 0x6C00 | ((unsigned long)0x7200 << 32);
        }
        FlushCache(0);
    }
    KickRef(pkt, 10);
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

/* the timer's count register (T0_COUNT) */
#define T0_COUNT ((volatile unsigned int *)0x10000000)

/* the frame waited for: a thirtieth of a second on the timer, or the next vertical blank; the timer
 * restarted (new in SOCOM II, not named) */
extern "C" void func_003B0660()
{
    if (zVid.syncMode == 2) {
        unsigned int t = zSys.timerTicksPerSecond / 30;

        do
            sceGsSyncV(0);
        while (*T0_COUNT < t);
    } else {
        while (sceGsSyncV(0) == 0)
            ;
    }
    *T0_COUNT = 0;
}

/* the frame closed: its time from the timer, the frame counted, the FIFO ended and drained (new in
 * SOCOM II, not named) */
extern "C" void func_003B05E0()
{
    zVid.procTime = (float)*T0_COUNT * zSys.timerScale;
    zVid.frameNumber++;
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
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
            (unsigned long)zVid.displayBuf | 0xA0000 | ((unsigned long)zVid.framePsm << 24);
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
