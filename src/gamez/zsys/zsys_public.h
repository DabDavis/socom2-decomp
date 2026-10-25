/* The system's public state (SOCOM 1's _zsys_public, 0x28 bytes; SOCOM II's 0x2C) and the EE
 * kernel and DMA calls the system units make. Apart from zsys.h because camera.cpp declares a
 * stand-in of its own for the one field it reads; the header's name is ours. */
#ifndef ZSYS_PUBLIC_H
#define ZSYS_PUBLIC_H

#include "zsys.h"

struct sceDmaChan;

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
    unsigned int _20;                   /* 0x20: SOCOM II's */
    unsigned int timerTicksPerSecond;   /* 0x24 (SOCOM 1: 0x20) */
    float timerScale;                   /* 0x28 (SOCOM 1: 0x24) */
};   /* 0x2C bytes */

extern _zsys_public zSys;   /* 0x49E2B0 */

#endif
