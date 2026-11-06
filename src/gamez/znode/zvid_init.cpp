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
/* the VIF packets zVid_Init hands the zVid (SOCOM 1's ztest_on_vif and ztest_off_vif: TEST register
 * packets, the z test on and off) */
extern u_long128 ztest_on_vif;
extern u_long128 ztest_off_vif;
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

/* a store image's BITBLTBUF and TRXREG as Sony's register structs (libgraph's sceGsBitbltbuf and
 * sceGsTrxreg bitfields: the fields' reads narrow to the byte or halfword holding them), over the
 * image at its own offsets -- the VIF codes and the GIF tag first (the view is ours) */
struct GS_BITBLTBUF {
    unsigned long SBP : 14;
    unsigned long : 2;
    unsigned long SBW : 6;
    unsigned long : 2;
    unsigned long SPSM : 6;
    unsigned long : 2;
    unsigned long DBP : 14;
    unsigned long : 2;
    unsigned long DBW : 6;
    unsigned long : 2;
    unsigned long DPSM : 6;
    unsigned long : 2;
};
struct GS_TRXREG {
    unsigned long RRW : 12;
    unsigned long : 20;
    unsigned long RRH : 12;
    unsigned long : 20;
};
struct GS_STOREIMAGE_REGS {
    u_long128 vifcode;   // 0x0
    u_long128 giftag;   // 0x10
    GS_BITBLTBUF bitbltbuf;   // 0x20
    unsigned long bitbltbufaddr;   // 0x28
    unsigned long trxpos;   // 0x30
    unsigned long trxposaddr;   // 0x38
    GS_TRXREG trxreg;   // 0x40
    unsigned long trxregaddr;   // 0x48
};

/* a GS-to-host transfer's split (new in SOCOM II, not named): the image's bytes by its pixel
 * format, the quadwords in whole 8-quadword blocks and the rest, the bytes past the last whole
 * quadword, and -- when there are some -- the height rounded up to whole quadwords and the
 * quadwords that adds past the blocks */
extern "C" int data_004A45A8;
extern "C" int data_004A45B0;
extern "C" int data_004A45B8;
extern "C" int data_004A45C0;

#define SCE_GS_SET_TRXREG(rrw, rrh) ((u_long)(rrw) | ((u_long)(rrh) << 32))

/* the transfer split worked out for a store image, its TRXREG's height rounded up in place
 * (through the uncached mirror) when the rows end inside a quadword (new in SOCOM II, not named;
 * the formats as libgraph's: 32, 24, 16, 8 and 4 bits; the video thread's body 0x3B1DD0 calls it on
 * the unit's store image). The case labels in libgraph's order give the target's descending compare
 * chain; the locals declared format, width, height, then the rounded height (the weapons' round 29:
 * `tools/igreplay.py --target` read the three volatile registers in descending number) */
extern "C" void func_003B28D0(GS_STOREIMAGE_REGS *si)
{
    int psm = si->bitbltbuf.SPSM;
    int w = si->trxreg.RRW;
    int h = si->trxreg.RRH;
    int h2 = 0;

    switch (psm) {
    case 0:
    case 48: {
        int bytes = w * h * 4;

        data_004A45C0 = bytes & 0xF;
        data_004A45A8 = (bytes >> 4) & ~7;
        data_004A45B8 = (bytes >> 4) & 7;
        if (!data_004A45C0) {
            h2 = h;
            data_004A45B0 = 0;
        } else {
            h2 = (h + 3) & ~3;
            data_004A45B0 = ((w * h2) >> 2) - data_004A45A8 - data_004A45B8 - 1;
        }
        break;
    }
    case 1:
    case 49: {
        int bytes = w * h * 3;

        data_004A45C0 = bytes & 0xF;
        data_004A45A8 = (bytes >> 4) & ~7;
        data_004A45B8 = (bytes >> 4) & 7;
        if (!data_004A45C0) {
            h2 = h;
            data_004A45B0 = 0;
        } else {
            h2 = (h + 15) & ~15;
            data_004A45B0 = ((w * h2 * 3) >> 4) - data_004A45A8 - data_004A45B8 - 1;
        }
        break;
    }
    case 2:
    case 10:
    case 50:
    case 58: {
        int bytes = w * h * 2;

        data_004A45C0 = bytes & 0xF;
        data_004A45B8 = (bytes >> 4) & 7;
        data_004A45A8 = (bytes >> 4) & ~7;
        if (!data_004A45C0) {
            h2 = h;
            data_004A45B0 = 0;
        } else {
            h2 = (h + 7) & ~7;
            data_004A45B0 = ((w * h2) >> 3) - data_004A45A8 - data_004A45B8 - 1;
        }
        break;
    }
    case 19:
    case 27: {
        int bytes = w * h;

        data_004A45C0 = bytes & 0xF;
        data_004A45B8 = (bytes >> 4) & 7;
        data_004A45A8 = (bytes >> 4) & ~7;
        if (!data_004A45C0) {
            h2 = h;
            data_004A45B0 = 0;
        } else {
            h2 = (h + 7) & ~7;
            data_004A45B0 = ((w * h2) >> 4) - data_004A45A8 - data_004A45B8 - 1;
        }
        break;
    }
    case 20:
    case 36:
    case 44: {
        int bytes = (w * h) >> 1;

        data_004A45C0 = bytes & 0xF;
        data_004A45B8 = (bytes >> 4) & 7;
        data_004A45A8 = (bytes >> 4) & ~7;
        if (!data_004A45C0) {
            h2 = h;
            data_004A45B0 = 0;
        } else {
            h2 = (h + 7) & ~7;
            data_004A45B0 = ((w * h2) >> 5) - data_004A45A8 - data_004A45B8 - 1;
        }
        break;
    }
    }
    if (data_004A45C0)
        *(u_long *)UNCACHED(&si->trxreg) = SCE_GS_SET_TRXREG(si->trxreg.RRW, h2);
}

/* the EE's DMA, VIF, GIF and GS registers the transfer below touches (Sony's eeregs.h names) */
#define ZV_D1_CHCR ((volatile unsigned int *)0x10009000)
#define ZV_D1_MADR ((volatile unsigned int *)0x10009010)
#define ZV_D1_QWC ((volatile unsigned int *)0x10009020)
#define ZV_GIF_CTRL ((volatile unsigned int *)0x10003000)
#define ZV_VIF1_STAT ((volatile unsigned int *)0x10003C00)
#define ZV_VIF1_FBRST ((volatile unsigned int *)0x10003C10)
#define ZV_VIF1_FIFO ((volatile u_long128 *)0x10005000)
#define ZV_GS_CSR ((volatile unsigned long *)0x12001000)
#define ZV_GS_BUSDIR ((volatile unsigned long *)0x12001040)

/* the quadword written into VIF1's FIFO when the GS never finishes (0x3E1430, initialised data; not
 * named) */
extern "C" u_long128 data_003E1430;

/* the path reset after a timed-out read: the GS, the bus direction back to the host, GIF and VIF1
 * (inline; the name ours) */
static inline void StoreImageReset()
{
    *ZV_GS_CSR = 256;
    *ZV_GS_BUSDIR = 0;
    *ZV_GIF_CTRL = 1;
    *ZV_VIF1_FBRST = 1;
}

/* a store image's pixels read back from the GS into dst through VIF1 (new in SOCOM II, not named:
 * libgraph's sceGsExecStoreImage on the split 0x3B28D0 works out -- the video thread's body 0x3B1DD0
 * calls it twice): the packet sent once channel 1 is free, the GS waited on to finish, the bus
 * turned round, the whole blocks taken by DMA, the rest quadword by quadword out of the FIFO, the
 * last quadword's bytes copied and the padding read away; every wait counts on one counter and
 * gives up past 0x1000000 polls (-1), the later ones resetting the path first. Written in the
 * weapons' round 30: the register tests `(x & m) == 0` (`!(x & m)` materialises sltu/xori: 0x440).
 * BYTE-EXACT since the weapons' round 37, by three shapes:
 *  - the FIFO loop's second counter k is unsigned and indexes through `(int)k`: a same-size
 *    signedness cast of the induction variable is not a safe conversion to MW's strength reduction
 *    (PowerPC MW's IroLoop.c, IsSafeTypcon: only a widening, or unsigned to a LARGER signed type,
 *    passes), so the target's dst + (k << 4) is recomputed each pass; any uncast `dst[k++]` (int,
 *    unsigned, short, a value local, a char * or shifted address -- rounds 30-31) became a pointer
 *    stepped by 16 (45 off), `dst[base + i]` re-added base + i (15 off);
 *  - ONE int i, declared before k (the target's k t0, i t1), counts all three loops: the byte copy's
 *    and the padding loop's are later webs that IRO_SplitLifetimes renumbers after the loop pass's
 *    temporaries (the byte copy's index then takes a3 and its induction pointer t0, the padding
 *    loop's counter a1 and its bound a2, as the target's -- a j of their own trades each pair);
 *  - the byte copy reads the quadword as `((unsigned char *)&q)[i]`: the strength-reduced pointer
 *    MW makes is set in the rotated loop's preheader, after the `0 < count` guard as the target's,
 *    where a pointer of the source's own (`p = &q` in the for-init, `*p++`) was set before it */
extern "C" int func_003B24C0(GS_STOREIMAGE_REGS *si, u_long128 *dst)
{
    unsigned int count = 0;

    while (*ZV_D1_CHCR & 0x100) {
        if (count++ > 0x1000000)
            return -1;
    }
    *ZV_D1_QWC = 7;
    *ZV_D1_MADR = (unsigned int)si;
    *ZV_D1_CHCR = 0x101;
    while (*ZV_D1_CHCR & 0x100) {
        if (count++ > 0x1000000)
            return -1;
    }
    while ((*ZV_GS_CSR & 2) == 0) {
        if (count++ > 0x1000000) {
            *ZV_VIF1_FIFO = data_003E1430;
            return -1;
        }
    }
    *ZV_VIF1_STAT = 0x800000;
    *ZV_GS_BUSDIR = 1;
    if (data_004A45A8) {
        *ZV_D1_QWC = data_004A45A8;
        *ZV_D1_MADR = (unsigned int)dst;
        *ZV_D1_CHCR = 0x100;
        while (*ZV_D1_CHCR & 0x100) {
            if (count++ > 0x1000000) {
                StoreImageReset();
                return -1;
            }
        }
    }

    int i;
    unsigned int k = data_004A45A8;

    for (i = 0; i < data_004A45B8; i++) {
        while ((*ZV_VIF1_STAT & 0x1F000000) == 0) {
            if (count++ > 0x1000000) {
                StoreImageReset();
                return -1;
            }
        }
        dst[(int)k] = *ZV_VIF1_FIFO;
        k++;
    }
    if (data_004A45C0) {
        u_long128 q;

        while ((*ZV_VIF1_STAT & 0x1F000000) == 0) {
            if (count++ > 0x1000000) {
                StoreImageReset();
                return -1;
            }
        }
        q = *ZV_VIF1_FIFO;
        for (i = 0; i < data_004A45C0; i++)
            ((unsigned char *)&dst[data_004A45A8 + data_004A45B8])[i] = ((unsigned char *)&q)[i];
        for (i = 0; i < data_004A45B0; i++) {
            while ((*ZV_VIF1_STAT & 0x1F000000) == 0) {
                if (count++ > 0x1000000) {
                    StoreImageReset();
                    return -1;
                }
            }
            q = *ZV_VIF1_FIFO;
        }
    }
    *ZV_VIF1_STAT = 0;
    *ZV_GS_BUSDIR = 0;
    return 0;
}

/* the last sixteen brightness samples, a ring the video thread (func_003B1DD0) fills, and the
 * newest's place in it (new in SOCOM II, not named) */
extern float data_004A4560[16];
extern int data_004A45A0;

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

/* the screen's size (the 2D library's and the HUD's), the brightness sampler's mode (0 a grid, else
 * a cone; zrndr_postfx.cpp sets it), its window and the grid's phase (new in SOCOM II, not named) */
extern "C" unsigned int data_004A44C4;
extern "C" unsigned int data_004A44C8;
extern "C" int data_004A4550;
extern "C" int data_004A44CC;
extern "C" int data_004A44D0;
extern "C" int data_004A44D4;
extern "C" int data_004A44D8;
extern "C" int data_004B45D0;
extern "C" int data_004B45D8;
/* the brightness measured last (new in SOCOM II, not named) */
extern "C" float data_003E1420;
extern sceGsStoreImage data_004A43E0;
/* libgraph's GS interrupt-mask pair (MAIN 0x1A3058 / 0x1A3098: src/lib/sdk/gs_imr.c) */
extern "C" u_long sceGsGetIMR(void);
extern "C" u_long sceGsPutIMR(u_long imr);
extern "C" int SleepThread(void);
extern "C" float cosf(float);

#define ZV_ANGLE 1.4922565f

/* a point in floats (SOCOM 1's PNT2D, zmath.h's) */
struct PNT2D {
    float x, y;
};

/* the video thread's body (new in SOCOM II, not named): woken once a frame, it holds the
 * interrupts and the FIFO, stops VIF1 and reads single pixels back from the GS through the unit's
 * store image -- on a grid stepped 40 pixels over the window, its phase moved on each pass, or along
 * a cone of rows and columns (85.5 degrees each way) -- and keeps their weighted mean as the
 * brightness, also in the ring func_003B1BF0 averages. BYTE-EXACT in the weapons' round 34 (unwritten
 * before): the grid arm's two emptied if-bodies (`srl; bltz +1; nop`, the AI core's round 34) are the
 * cone arm's half-size locals declared there too and never read -- an unsigned-to-float conversion
 * assigned to a dead local keeps its sign test and loses both arms; the pixel position and the two
 * angles are IPNT2D / PNT2D locals, which MW keeps in the frame (each read once per use, the angle
 * shared by the compare and cosf's argument -- volatile scalars give the frame but re-read it); the
 * two steps' divisions named locals (hoisted ahead of the loops; in the steps they stay in them) */
extern "C" void func_003B1DD0(void *arg)
{
    for (;;) {
        SleepThread();

        unsigned int sum[3] = {0};
        IPNT2D pos;
        PNT2D ang;
        u_long128 pix;
        float weight = 0.0f;
#define si ((GS_STOREIMAGE_REGS *)&data_004A43E0)

        DIntr();
        zSysFifoPause();
        *ZV_VIF1_FBRST = 8;
        asm volatile { sync }
        while (*ZV_VIF1_STAT & 0xC00)
            ;
        u_long imr = sceGsPutIMR(sceGsGetIMR() | 0x200);

        *ZV_GS_CSR = 2;
        func_003B28D0(si);
        if (!data_004A4550) {
            float hw = (float)(data_004A44C4 >> 1);
            float hh = (float)(data_004A44C8 >> 1);

            for (pos.y = data_004A44D0 + data_004B45D8; pos.y < data_004A44D8; pos.y += 40) {
                for (pos.x = data_004A44CC + data_004B45D0; pos.x < data_004A44D4; pos.x += 40) {
                    *(u_long *)UNCACHED(&si->trxpos) = (u_long)pos.x | ((u_long)pos.y << 16);
                    func_003B24C0(si, &pix);
                    weight += 256.0f;
                    sum[0] += ((unsigned char *)&pix)[0];
                    sum[1] += ((unsigned char *)&pix)[1];
                    sum[2] += ((unsigned char *)&pix)[2];
                }
            }
            data_004B45D0 += 10;
            if (data_004B45D0 == 40) {
                data_004B45D0 = 0;
                data_004B45D8 += 7;
                if (data_004B45D8 == 28)
                    data_004B45D8 = 0;
            }
        } else {
            float hw = (float)(data_004A44C4 >> 1);
            float hh = (float)(data_004A44C8 >> 1);
            float sx = 59.69026f / hw;
            float sy = 41.783184f / hh;

            for (ang.y = -ZV_ANGLE; ang.y < ZV_ANGLE; ang.y += sy) {
                if (ang.y < 0.0f)
                    pos.y = (int)(hh * cosf(ang.y));
                else
                    pos.y = (int)((float)data_004A44C8 - hh * cosf(ang.y));

                float lim = ZV_ANGLE * cosf(ang.y);

                for (ang.x = -lim; ang.x < lim; ang.x += sx) {
                    if (ang.x < 0.0f)
                        pos.x = (int)(hw * cosf(ang.x));
                    else
                        pos.x = (int)((float)data_004A44C4 - hw * cosf(ang.x));
                    *(u_long *)UNCACHED(&si->trxpos) = (u_long)pos.x | ((u_long)pos.y << 16);
                    func_003B24C0(si, &pix);
                    weight += 256.0f;
                    sum[0] += ((unsigned char *)&pix)[0];
                    sum[1] += ((unsigned char *)&pix)[1];
                    sum[2] += ((unsigned char *)&pix)[2];
                }
            }
        }

        float br = (0.4901f * sum[0] + 0.4657f * sum[1] + 0.0441f * sum[2]) / weight;

        data_003E1420 = br;
        data_004A4560[data_004A45A0] = br;
        data_004A45A0++;
        if ((unsigned int)data_004A45A0 >= 16)
            data_004A45A0 = 0;
        *ZV_VIF1_FIFO = data_003E1430;
        sceGsPutIMR(imr);
        *ZV_GS_CSR = 2;
        zSysFifoResume(true);
        EIntr();
    }
#undef si
}

/* the mean of the last n brightness samples: all sixteen in order, or n back from the newest (new in
 * SOCOM II, not named). BYTE-EXACT since the weapons' round 37: ONE unsigned variable at function
 * scope is the first loop's counter AND the second loop's walk index -- IRO_SplitLifetimes gives the
 * later web (the walk) a temporary of its own, made after the loop unroller's bound (`n - 8`), so the
 * walk's index is numbered after the bound and takes the target's v1 (the bound a1). Before: OPEN, 17
 * instructions, the same size -- idx (v35) and the unroller's bound (v36) traded v1 and a1, every
 * placement numbering the bound BEFORE idx giving the target's registers (round 29's igreplay); a
 * declared idx (signed or unsigned, at the top or in the branch), an inline's local or parameter, a
 * dead first definition, casts and template accessors (rounds 29-31) all kept idx a declared-local
 * number; the loop counter i shared by both loops instead (18 off) or a dead `idx = 0` first (22) do
 * not reach it -- the first web must be live */
extern "C" float func_003B1BF0(unsigned int n)
{
    float sum = 0.0f;
    unsigned int idx;

    if (n == 16) {
        for (idx = 0; idx < 16; idx++)
            sum += data_004A4560[idx];
        sum /= 16.0f;
    } else {
        unsigned int i;

        idx = data_004A45A0;
        for (i = 0; i < n; i++)
            sum += data_004A4560[idx--];
        sum /= n;
    }
    return sum;
}


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

/* (a marked device, zVid_Init's note: a byte-sized value IRO does not propagate) */
enum ZviByte {
    ZVI_0 = 0,
    ZVI_FF = 255
};

/* the video system set up: the display 640x448 in the GS's 1728,1824 window, its buffers, the z
 * range, the fade cleared, the LOD valve made, the video thread started and the mode set (SOCOM 1's,
 * with SOCOM II's window, second buffer and thread). BYTE-EXACT since round 40 (below). Before: OPEN, 11 instructions, the same size: the two
 * zero byte stores (0x7C, 0x7D) sink with the fade alpha's 255 (in v0) to the 0x90 flags, where the
 * target keeps them in source order with 255 hoisted into v1 at the top, as SOCOM 1's does -- every
 * position of the two stores (a 23x23 sweep), bool/char/bitfield/volatile fields, chained and 0.0f
 * forms leave it at 11; the 255's register looks like the cause (an alpha set some other way?).
 * Round 31: 31 off at the round's start (not 11: an earlier round's header change); the target
 * keeps 1 in a1 from the top for flag91 AND both of Create's arguments (`move a2,a1`: one shared
 * constant) and 255 in v1 -- the fade's two stores right after swapMode 21 off, at the top 23,
 * the alpha alone at the top 22, through an unsigned char or int local 31.
 * Round 37 (igdump --iro / --pcode, build/scratch/wep/vorder.py, v_zvi37*.py): IRO hands the stores
 * to the back end in SOURCE order (its last dump), and the target keeps that order; our back end
 * moves every store of a constant 0 but the first to the function's last run of zero stores
 * (swapMode / pcrtcDo to flag90's; with the flags first, finalBlend and runTime move too), and
 * `swapMode = 1` or `= mode` stays put. `#pragma scheduling off` changes nothing, `#pragma peephole
 * off` scatters the zero stores up instead (the pre-pass scheduler then places them freely) -- the
 * grouping is the peephole's; false, a zero local (char or int), casts, an enum 0 and field types
 * bool / char (swapMode's or the flags') all group the same.
 * Round 39 (igdump --pcode): the PCode at colouring already has swapMode's and pcrtcDo's zero stores
 * just ahead of flag90's, with `li 1` (a1) and `li 255` between them -- so 255 is live only there and
 * takes v0; the target's 255 is live from the top (v1), its zero stores where the source writes them.
 * SOCOM 1's zVid_Init (the demo's 0x308690) has the same shape: 255 into v1 near the top, its two zero
 * byte stores in source order among the constants -- a code-generation property of both builds.
 * Round 40 (build/scratch/wep/v_zvi40*.py): BYTE-EXACT, by two things. THE DATA: every earlier round's
 * diff hid that the two pointers are SOCOM 1's ztest_on_vif and ztest_off_vif, not the DMA refs (the
 * relocation mismatch shows only once the code is aligned: `try.py` printed WRONG REFERENCE, d=0).
 * THE ENUM LEVER (the game states' round 53): 1 and 255 in enum-typed locals, which IRO does not
 * propagate, so each is one `li` where it is defined. WHERE they are defined is what orders the stores:
 * the pre-pass scheduler issues a `li` whose store is ready at once and the store next, so a run of
 * such pairs leaves a zero byte store no slot, while a `li` whose store is not ready (flag91's and the
 * fade's follow other stores) lets the first ready zero store in behind it. So one such `li` ahead of
 * 640 -- exactly one -- lets swapMode's store in at the top, the other just before pcrtcDo's store
 * lets that store in behind ztest_off's; two at the top put pcrtcDo at the top too, none sinks both
 * (11 / 31 off in the earlier rounds). And 255 lives from there to its store, so it takes v1, and
 * the post-pass scheduler hoists its `li` (it has a height, its store being in the block) over the
 * parameter's move. Either of the two may come first (v_zvi40t.py): one marked device, what the
 * original held the two constants in is not known. On the way, recorded for other functions: a `.set`
 * after each store from 640 to ztest_off gave 2 off (the short blocks issue every `li` at once
 * otherwise) */
void zVid_Init(_zvid_mode mode)
{
    ZviByte one = (ZviByte)1;
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
    zVid.ztest_on = &ztest_on_vif;
    zVid.ztest_off = &ztest_off_vif;
    ZviByte ff = (ZviByte)255;
    zVid.pcrtcDo = 0;
    zVid.iMinZ = 10;
    zVid.iMaxZ = 0xFFFF;
    zVid.textureBaseAddr = 11200;
    zVid.textureEndAddr = 16384;
    zVid.flag90 = 0;
    zVid.flag91 = one;
    zVid.finalBlend = 0;
    zVid.displayBuf = 0;
    zVid.psm = 0;
    *(unsigned long *)zvid_fade = 0;
    zvid_fade[3] = ff;
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

/* func_003B09E0 of a rectangle, written out (new in SOCOM II, not named). BYTE-EXACT since the weapons'
 * round 40 (build/scratch/wep/v_rect40*.py), with no device: the scaled width and height held in two
 * locals ahead of the first store -- NOT func_003B09E0 inlined, whose own body, the same words, keeps the
 * subtractions after it. The colours gave it away: the target's registers ask for numbers w, 640, h,
 * 448, y, x (each node's colour needs a coloured neighbour for every lower register), so w's and h's
 * parts are generated first, and h is no inline's temporary. The way there: moving the setter's
 * subtractions ahead of its first store in a throwaway copy fell from 35 to 24 off, and the same with the
 * fields read in place matched. Before: OPEN, 35
 * instructions, the same size: scheduling -- the target loads h, y, w, x and keeps more of the
 * products live (t3-t5); the inline call, the fields into locals (both orders) and the two words
 * built first all schedule as the by-value setter does. Round 36 (build/scratch/wep/v_rect*.py):
 * the setter's two stores swapped, each OR's operands swapped, the high halves into locals first
 * (33-37), and any of the fields through an inline with a body of its own (ApplyDecalRender's
 * argument rule: 35-36) -- the target's load order h, y, w, x is not an argument order. Round 37
 * (igdump --pcode; v_rect37.py): the inliner substitutes x and y (used before the first store) and
 * gives only w and h argument temporaries, so the PCode already reads h, w, y, x before colouring
 * (igreplay --target: 14 PCode instructions unmatched -- the schedule, not a numbering); the body
 * written in place with either OR's halves in either order and either store first, and wrapper
 * inlines taking the fields as (x, w, y, h), (h, w, y, x) or (w, x, h, y): 35-38 */
extern "C" void func_003B0A70(ZVID_RECT *r)
{
    unsigned int w = (640 - r->w) * 16;
    unsigned int h = (448 - r->h) * 16;

    *(unsigned long *)UNCACHED(&finalcode[14]) =
        (unsigned long)(r->x * 16 + 0x6C00) | ((unsigned long)(r->y * 16 + 0x7200) << 16);
    *(unsigned long *)UNCACHED(&finalcode[16]) = (unsigned long)(0x9400 - w) | ((unsigned long)(0x8E00 - h) << 16);
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

/* libgraph's display register as bit-fields (Sony's tGS_DISPLAY2: unsigned int fields, so the
 * register copies as two words) and the display environment two of which open the double buffer
 * (sceGsDispEnv, 0x28 bytes; the GIF tags and drawing environments follow in sceGsDBuff) */
struct tGS_DISPLAY {
    unsigned int DX : 12;
    unsigned int DY : 11;
    unsigned int MAGH : 4;
    unsigned int MAGV : 2;
    unsigned int p0 : 3;
    unsigned int DW : 12;
    unsigned int DH : 11;
    unsigned int p1 : 9;
};
struct sceGsDispEnv {
    unsigned long pmode;
    unsigned long smode2;
    unsigned long dispfb;
    tGS_DISPLAY display;
    unsigned long bgcolor;
};
struct sceGsDBuff {
    sceGsDispEnv disp[2];
};

/* the GS's SMODE2, DISPFB1, DISPLAY1, DISPFB2, DISPLAY2 and BGCOLOR registers */
#define GS_SMODE2 ((volatile unsigned long *)0x12000020)
#define GS_DISPFB1 ((volatile unsigned long *)0x12000070)
#define GS_DISPLAY1 ((volatile unsigned long *)0x12000080)
#define GS_DISPFB2 ((volatile unsigned long *)0x12000090)
#define GS_DISPLAY2 ((volatile unsigned long *)0x120000A0)
#define GS_BGCOLOR ((volatile unsigned long *)0x120000E0)

/* the MPEG player's tick between frames (zmpeg.h's class: only the call) */
class CMPEG {
public:
    static void IdleTick();
};

/* the GS's set-up packet, sixteen quadwords, sent when the video mode changes (SOCOM 1's initcode,
 * which its swap sent the same way), and the frame restore's packet (SOCOM 1's framecode: SOCOM II
 * sends its first eight quadwords, then the next seven when the second pass is on) */
extern u_long128 initcode[16];
extern u_long128 framecode[];
/* whether the last vertical blank had passed when the frame closed (SOCOM 1's zvid + 0x230) */
extern int data_004A43B0;
/* the frame restore packet's FBP and coordinate halfwords (SOCOM 1's zvid + 0x234 / 0x238, pointers
 * into a packet) */
extern unsigned short *data_004A43B4;
extern unsigned short *data_004A43B8;

/* whether one vertical blank more still ends inside the frame rate's period (the name and the
 * inline ours: the swap's wait loop tests it -- the rate passed by value is read first, so the
 * division's 1.0 is built before the blank's 1/60, as the target builds them; written in place, or
 * with the limit or the sum passed instead, 1/60 comes first) */
static inline bool FrameEarly(float t, int rate)
{
    return t + 1.0f / 60.0f < 1.0f / rate;
}

/* the frame closed and shown: the frame rate capped by vertical blanks, the level of detail set by
 * the rate, the video mode reset when it changed; the frame's time, number and rate; then, to flip,
 * the buffers swapped and the PCRTC set -- the frame buffers, the display rectangle moved by the
 * mode's offset, the fade's background or the PCRTC offset; then, to restore, the frame restore
 * packet sent (SOCOM 1's zVid_Swap, SOCOM II's taking both flags: the second mode's packet, the
 * mode offsets, the frame restore's second pass) */
void zVid_Swap(bool flip, bool restore)
{
    static unsigned int t0Count;

    sceGsSyncPath(0, 0);
    if (CVideo::m_maxFrameRate != -1) {
        for (float t = zVid.procTime; FrameEarly(t, CVideo::m_maxFrameRate); t += 1.0f / 60.0f)
            sceGsSyncV(0);
    }
    SetDynamicLOD((int)(1.0f / zVid.procTime));
    if (zVid.syncMode != zVid.videoMode) {
        zvid_SetVideoMode(*(_zvid_mode *)&zVid.videoMode, true);
        zvid_KickPacket((unsigned long *)initcode, 16, false, false);
    }
    CMPEG::IdleTick();
    zVid.procTime = (float)*T0_COUNT * zSys.timerScale;
    data_004A43B0 = !sceGsSyncV(0);
    t0Count = *T0_COUNT;
    *T0_COUNT = 0;
    zVid.frameTime = (float)t0Count * zSys.timerScale;
    zVid.frameRate = (int)(0.5f + 1.0f / zVid.frameTime);
    zVid.runTime += zVid.frameTime;
    zVid.frameNumber++;
    if (flip) {
        if ((zVid.syncMode != 2 || !zVid.swapMode) && zVid.swapMode != 1) {
            unsigned int buf = zVid.displayBuf;

            zVid.displayBuf = zVid.renderBuf;
            zVid.renderBuf = buf;
        }
        if (zVid.swapMode == 2)
            *GS_SMODE2 = (unsigned long)(unsigned int)data_004A43C8 | 2;
        else
            *GS_SMODE2 = (unsigned long)(unsigned int)data_004A43C8 |
                         (unsigned long)(unsigned int)data_004A43CC << 1;

        tGS_DISPLAY d = data_004A4180.disp[1].display;

        d.DX += zVid.modeOffset[0] * zVid.pcrtcOffset[0];
        d.DY += zVid.modeOffset[1];
        *GS_DISPFB1 = (unsigned long)zVid.displayBuf | 0x1400 | (unsigned long)zVid.framePsm << 15;
        *GS_DISPFB2 = (unsigned long)zVid.displayBuf | 0x1400 | (unsigned long)zVid.framePsm << 15;
        *GS_DISPLAY2 = *(unsigned long *)&d;
        if (zvid_fade[3] < 255) {
            *GS_BGCOLOR = *(unsigned long *)zvid_fade;
            *GS_DISPLAY1 = *(unsigned long *)&d;
            *GS_PMODE = (unsigned long)zvid_fade[3] << 8 | 0xE1;
        } else if (zVid.pcrtcDo) {
            d.DX += zVid.pcrtcOffset[0];
            d.DY += zVid.pcrtcOffset[1];
            *GS_DISPLAY1 = *(unsigned long *)&d;
            *GS_PMODE = 0x7F23;
        } else {
            *GS_PMODE = 0x62;
        }
        asm volatile { sync }
    } else if (zVid.flag90) {
        *GS_PMODE = 96;
        asm volatile { sync }
    }
    if (restore) {
        zSysFifoEnable(1);
        if (zVid.swapMode == 0) {
            *data_004A43B4 = zVid.renderBuf;
            *data_004A43B8 = 0x7200;
            FlushCache(0);
            zvid_KickPacket((unsigned long *)framecode, 8, true, false);
            if (zVid.flag91)
                zvid_KickPacket((unsigned long *)&framecode[8], 7, true, false);
        } else {
            func_003B07B0(true);
        }
    }
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
