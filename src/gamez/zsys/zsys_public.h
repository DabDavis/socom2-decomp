/* The system's public state (SOCOM 1's _zsys_public, 0x28 bytes; SOCOM II's 0x2C) and the EE
 * kernel and DMA calls the system units make. Apart from zsys.h because camera.cpp declares a
 * stand-in of its own for the one field it reads; the header's name is ours. */
#ifndef ZSYS_PUBLIC_H
#define ZSYS_PUBLIC_H

#include "zsys.h"

/* Sony's DMA channel registers (SOCOM 1's DWARF) */
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

struct sceDmaChan {
    tD_CHCR chcr;               /* 0x00 */
    unsigned int p0[3];
    void *madr;                 /* 0x10 */
    unsigned int p1[3];
    unsigned int qwc;           /* 0x20 */
    unsigned int p2[3];
    void *tadr;                 /* 0x30 */
    unsigned int p3[3];
    void *as0;                  /* 0x40 */
    unsigned int p4[3];
    void *as1;                  /* 0x50 */
    unsigned int p5[3];
    unsigned int p6[4];         /* 0x60 */
    unsigned int p7[4];         /* 0x70 */
    void *sadr;                 /* 0x80 */
    unsigned int p8[3];
};   /* 0x90 bytes */

extern "C" {
int DIntr(void);
int EIntr(void);
int AddIntcHandler(int cause, int (*handler)(int), int next);
int RemoveIntcHandler(int cause, int id);
int EnableIntc(int cause);
int DisableIntc(int cause);
int iWakeupThread(int thread);
int sceDmaSync(sceDmaChan *d, int mode, int timeout);
void sceDmaSend(sceDmaChan *d, void *tag);
}

/* SOCOM 1's DWARF; SOCOM II moved the timer 4 bytes on (CAppCamera::Tick reads the scale at 0x28)
 * -- what it added before 0x24 is not placed yet */
struct _zsys_public {
    sceDmaChan *DmaToVif0;              /* 0x00 */
    sceDmaChan *DmaToVif1;              /* 0x04 */
    sceDmaChan *DmaToGif;               /* 0x08 */
    sceDmaChan *DmaFromSPR;             /* 0x0C */
    u_long128 *sprPacketEnd[2];         /* 0x10 */
    unsigned int sprPx;                 /* 0x18 */
    unsigned char isT10K;               /* 0x1C */
    unsigned char isCdBoot;             /* 0x1D */
    unsigned char cdSounds;             /* 0x1E */
    unsigned char memIrxLoaded;         /* 0x1F */
    unsigned char mcIrxLoaded;          /* 0x20: SOCOM II's, the memory card's modules (name ours) */
    unsigned char hddIrxLoaded;         /* 0x21: SOCOM II's, the hard disk's (name ours) */
    unsigned int timerTicksPerSecond;   /* 0x24 (SOCOM 1: 0x20) */
    float timerScale;                   /* 0x28 (SOCOM 1: 0x24) */
};   /* 0x2C bytes */

extern _zsys_public zSys;   /* 0x49E2B0 */

/* a quadword seen every way (SOCOM 1's DWARF) */
union _word128 {
    u_long128 u128;
    float f32[4];
    unsigned long u64[2];
    unsigned int u32[4];
    unsigned short u16[8];
    unsigned char u8[16];
};

/* the scratchpad's packet (SOCOM 1's class: statics only) */
class CSPR {
public:
    static _word128 *Copy(_word128 *to, _word128 *from, int qwc);
    static _word128 *Kick(_word128 *end, bool getNew);

    static _word128 *m_sprPacket;       /* 0x49E330 */
    static _word128 *m_sprPacketEnd;    /* 0x49E338 */
};

/* the allocators (zsys_memory.cpp; _free and _strdup are MAIN's copies, 0x180280/0x1801E0) */
int zsys_FullAllocAndFree();
void *_malloc(unsigned int size, const char *file, int line);
void *_memalign(unsigned int align, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

/* the system's start (zsys_init.cpp) */
void zSysInit();
void zSysPostInit();
void zSysReset();
void zSysTimerInit();
void zSysSleep(int ticks);
int zSysLoadIRX(char *name, int argc, char *argv, bool retry);
void zSysLoadMcIRX();
void zSysUnloadMcIRX();
bool zSysHddInit();
bool zSysHddUnmount(const char *name);
void zSysShutdown();

/* the DMA FIFO (zsys_fifo.cpp) */
void zSysFifoKick(u_long128 *packet, unsigned int qwc);
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoEnd();
void zSysFifoEnable(unsigned int mode);
void zSysFifoDisable();
void zSysFifoInit();
void zSysFifoSetSync(bool sync);
void zSysFifoPause();
void zSysFifoResume(bool restart);

#endif
