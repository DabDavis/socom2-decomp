/* zrndr_init.cpp -- the renderer's start: VU1's data memory filled with a marker, its microcode and
 * data uploaded, the two alpha environments the pipe sends built (OVL1 0x335FD0-0x33622F). The
 * unit name is SOCOM 1's (its __sinit_zrndr_init.cpp; SOCOM II's 0x402DF0, which builds only
 * <iostream>'s two init objects, 0x488660/0x488678, ZDB's neighbours in the BSS). zRndrInit is
 * SOCOM 1's, the same 0x258 bytes. */

#include <eetypes.h>
#include "../zsys/zsys_public.h"

/* Sony's GS packet types (libgraph.h's, in our own words) */
struct sceGifTag {
    unsigned long NLOOP : 15;
    unsigned long EOP : 1;
    unsigned long pad16 : 16;
    unsigned long id : 14;
    unsigned long PRE : 1;
    unsigned long PRIM : 11;
    unsigned long FLG : 2;
    unsigned long NREG : 4;
    unsigned long REGS0 : 4;
    unsigned long REGS1 : 4;
    unsigned long REGS2 : 4;
    unsigned long REGS3 : 4;
    unsigned long REGS4 : 4;
    unsigned long REGS5 : 4;
    unsigned long REGS6 : 4;
    unsigned long REGS7 : 4;
    unsigned long REGS8 : 4;
    unsigned long REGS9 : 4;
    unsigned long REGS10 : 4;
    unsigned long REGS11 : 4;
    unsigned long REGS12 : 4;
    unsigned long REGS13 : 4;
    unsigned long REGS14 : 4;
    unsigned long REGS15 : 4;
} __attribute__((aligned(16)));

struct sceGsAlphaEnv {
    unsigned long alpha1;   // 0x0
    long alpha1addr;   // 0x8
    unsigned long pabe;   // 0x10
    long pabeaddr;   // 0x18
    unsigned long texa;   // 0x20
    long texaaddr;   // 0x28
    unsigned long fba1;   // 0x30
    long fba1addr;   // 0x38
};   // 0x40 bytes

extern "C" int sceGsSetDefAlphaEnv(sceGsAlphaEnv *ap, int ctxt);

/* an alpha environment as the pipe sends it: its size in quadwords, the GIF tag, the registers
 * (ours) */
struct ZDB_ALPHA {
    int qwc;   // 0x0
    sceGifTag tag;   // 0x10
    sceGsAlphaEnv env;   // 0x20
};   // 0x60 bytes

/* the renderer's state (SOCOM 1's ZDB, 0xD0 bytes; no DWARF -- the members' names ours) */
struct ZDB_T {
    unsigned char m_flag0;   // 0x0
    unsigned char m_flag1;   // 0x1
    ZDB_ALPHA m_alpha[2];   // 0x10
};   // 0xd0 bytes

ZDB_T ZDB;   // 0x488680

namespace zdb {
class CVisual {
public:
    static void Init();
};
}

/* VU1's microcode and the data its DMA chain starts with (in the microcode blob) */
extern u_long128 vu1_DataSegment1;
extern u_long128 vu1_CodeSegment1;

/* SOCOM 1's */
void zRndrInit()
{
    ZDB.m_flag0 = 0;
    ZDB.m_flag1 = 0;
    /* VU1's 1024 data quadwords marked: eight a pass, a pair at a time -- MW unrolls the inner
     * loop whole and leaves the outer one as written (a plain loop over pairs it unrolls 8x) */
    u_long128 *p = (u_long128 *)0x1100C000;
    for (unsigned int i = 0; i < 1024; i += 8) {
        for (int k = 0; k < 4; k++) {
            *p++ = (u_long128)0xCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD;
            *p++ = (u_long128)0xCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD;
        }
    }
    FlushCache(0);
    sceDmaSend(zSys.DmaToVif1, (u_long128 *)((u_int)&vu1_DataSegment1 & 0x0FFFFFFF));
    sceDmaSync(zSys.DmaToVif1, 0, 0);
    sceDmaSend(zSys.DmaToVif1, (u_long128 *)((u_int)&vu1_CodeSegment1 & 0x0FFFFFFF));
    sceDmaSync(zSys.DmaToVif1, 0, 0);

    ZDB.m_alpha[0].qwc = sceGsSetDefAlphaEnv(&ZDB.m_alpha[0].env, 0);
    ZDB.m_alpha[0].env.alpha1 = 136;
    *(u_long128 *)&ZDB.m_alpha[0].tag = 0;
    ZDB.m_alpha[0].tag.NLOOP = ZDB.m_alpha[0].qwc;
    ZDB.m_alpha[0].tag.EOP = 1;
    ZDB.m_alpha[0].tag.NREG = 1;
    ZDB.m_alpha[0].tag.REGS0 = 0xE;

    ZDB.m_alpha[1].qwc = sceGsSetDefAlphaEnv(&ZDB.m_alpha[1].env, 0);
    *(u_long128 *)&ZDB.m_alpha[1].tag = 0;
    ZDB.m_alpha[1].tag.NLOOP = ZDB.m_alpha[1].qwc;
    ZDB.m_alpha[1].tag.EOP = 1;
    ZDB.m_alpha[1].tag.NREG = 1;
    ZDB.m_alpha[1].tag.REGS0 = 0xE;
    zdb::CVisual::Init();
}
