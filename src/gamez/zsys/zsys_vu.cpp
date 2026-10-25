/* zsys_vu.cpp -- the vector units: CVU uploads microcode by DMA, and the first VU0 user installs
 * the VIF0 interrupt handler and fills VU0's data memory with 0xCD; SOCOM II adds a VU0 register
 * context switch (OVL1 0x351140-0x3515CF). SOCOM
 * 1 links these after the FIFO code and before the valves; the file name is ours. Defined in
 * reverse of their image order (-inline deferred). */

#include "zsys_public.h"

#define VIF0_FBRST ((volatile unsigned int *)0x10003810)
#define VU0_MEM ((u_long128 *)0x11004000)

int CVU::m_refCount;        /* 0x49E370 */
int CVU::m_VifHandlerID;    /* 0x49E378 */

/* VU0's registers as a context switch keeps them: vf1-vf31, then vi1-vi15 four to a quadword
 * (SOCOM II's; names ours) */
struct VU0Context {
    u_long128 vf[31];           /* 0x000 */
    u_long128 vi[4];            /* 0x1F0 */
};   /* 0x230 bytes */

static VU0Context s_vu0Context[2];  /* 0x49E380 */

/* VIF0 stopped: SOCOM 1's file static */
static int m_running;   /* 0x49E7E0 */

int InterruptFromVIF0(int cause)
{
    if (cause == 4) {
        m_running = 0;
        *VIF0_FBRST = 8;
        asm volatile { sync }
    }
    asm volatile { sync }
    asm volatile { ei }
    return 0;
}

CVU::CVU(unsigned int unit)
{
    if (m_refCount == 0 && unit == 0) {
        u_long128 *p;
        unsigned int i;

        m_VifHandlerID = AddIntcHandler(4, InterruptFromVIF0, 0);
        EnableIntc(4);
        p = VU0_MEM;
        for (i = 0; i < 256; i++)
            *p++ = 0xCDCDCDCDCDCDCDCDCDCDCDCDCDCDCDCD;
    }
    m_refCount++;
    m_unit = unit;
    if (unit == 0) {
        m_memsize = 256;
        m_vif = zSys.DmaToVif0;
    } else {
        m_memsize = 1024;
        m_vif = zSys.DmaToVif1;
    }
}

CVU::~CVU()
{
    unsigned int unit = m_unit;

    m_refCount--;
    if (m_refCount == 0 && unit == 0) {
        DisableIntc(4);
        RemoveIntcHandler(4, m_VifHandlerID);
    }
}

int CVU::Code(u_long128 *code)
{
    m_code = code;
    sceDmaSync(m_vif, 0, 0);
    sceDmaSend(m_vif, code);
    sceDmaSync(m_vif, 0, 0);
    return 0;
}

/* SOCOM II's: VU0's registers saved to a context (name ours). Each block ends in a nop: the
 * target keeps EIntr's delay slot empty, where MW would move the block's last instruction in */
void zSysVu0Save(int n)
{
    VU0Context *ctx = &s_vu0Context[n];
    u_long128 *vi = ctx->vi;

    DIntr();
    asm volatile {
        sqc2 vf1, 0(ctx)
        sqc2 vf2, 16(ctx)
        sqc2 vf3, 32(ctx)
        sqc2 vf4, 48(ctx)
        sqc2 vf5, 64(ctx)
        sqc2 vf6, 80(ctx)
        sqc2 vf7, 96(ctx)
        sqc2 vf8, 112(ctx)
        sqc2 vf9, 128(ctx)
        sqc2 vf10, 144(ctx)
        sqc2 vf11, 160(ctx)
        sqc2 vf12, 176(ctx)
        sqc2 vf13, 192(ctx)
        sqc2 vf14, 208(ctx)
        sqc2 vf15, 224(ctx)
        sqc2 vf16, 240(ctx)
        sqc2 vf17, 256(ctx)
        sqc2 vf18, 272(ctx)
        sqc2 vf19, 288(ctx)
        sqc2 vf20, 304(ctx)
        sqc2 vf21, 320(ctx)
        sqc2 vf22, 336(ctx)
        sqc2 vf23, 352(ctx)
        sqc2 vf24, 368(ctx)
        sqc2 vf25, 384(ctx)
        sqc2 vf26, 400(ctx)
        sqc2 vf27, 416(ctx)
        sqc2 vf28, 432(ctx)
        sqc2 vf29, 448(ctx)
        sqc2 vf30, 464(ctx)
        sqc2 vf31, 480(ctx)
        vmfir.x vf1x, vi1
        vmfir.y vf1y, vi2
        vmfir.z vf1z, vi3
        vmfir.w vf1w, vi4
        sqc2 vf1, 0(vi)
        vmfir.x vf1x, vi5
        vmfir.y vf1y, vi6
        vmfir.z vf1z, vi7
        vmfir.w vf1w, vi8
        sqc2 vf1, 16(vi)
        vmfir.x vf1x, vi9
        vmfir.y vf1y, vi10
        vmfir.z vf1z, vi11
        vmfir.w vf1w, vi12
        sqc2 vf1, 32(vi)
        vmfir.x vf1x, vi13
        vmfir.y vf1y, vi14
        vmfir.z vf1z, vi15
        sqc2 vf1, 48(vi)
        nop
    }
    EIntr();
}

/* SOCOM II's: VU0's registers loaded from a context (name ours) */
void zSysVu0Restore(int n)
{
    VU0Context *ctx = &s_vu0Context[n];
    u_long128 *vi = ctx->vi;

    DIntr();
    asm volatile {
        lqc2 vf1, 0(vi)
        vmtir vi1, vf1x
        vmtir vi2, vf1y
        vmtir vi3, vf1z
        vmtir vi4, vf1w
        lqc2 vf1, 16(vi)
        vmtir vi5, vf1x
        vmtir vi6, vf1y
        vmtir vi7, vf1z
        vmtir vi8, vf1w
        lqc2 vf1, 32(vi)
        vmtir vi9, vf1x
        vmtir vi10, vf1y
        vmtir vi11, vf1z
        vmtir vi12, vf1w
        lqc2 vf1, 48(vi)
        vmtir vi13, vf1x
        vmtir vi14, vf1y
        vmtir vi15, vf1z
        lqc2 vf1, 0(ctx)
        lqc2 vf2, 16(ctx)
        lqc2 vf3, 32(ctx)
        lqc2 vf4, 48(ctx)
        lqc2 vf5, 64(ctx)
        lqc2 vf6, 80(ctx)
        lqc2 vf7, 96(ctx)
        lqc2 vf8, 112(ctx)
        lqc2 vf9, 128(ctx)
        lqc2 vf10, 144(ctx)
        lqc2 vf11, 160(ctx)
        lqc2 vf12, 176(ctx)
        lqc2 vf13, 192(ctx)
        lqc2 vf14, 208(ctx)
        lqc2 vf15, 224(ctx)
        lqc2 vf16, 240(ctx)
        lqc2 vf17, 256(ctx)
        lqc2 vf18, 272(ctx)
        lqc2 vf19, 288(ctx)
        lqc2 vf20, 304(ctx)
        lqc2 vf21, 320(ctx)
        lqc2 vf22, 336(ctx)
        lqc2 vf23, 352(ctx)
        lqc2 vf24, 368(ctx)
        lqc2 vf25, 384(ctx)
        lqc2 vf26, 400(ctx)
        lqc2 vf27, 416(ctx)
        lqc2 vf28, 432(ctx)
        lqc2 vf29, 448(ctx)
        lqc2 vf30, 464(ctx)
        lqc2 vf31, 480(ctx)
        nop
    }
    EIntr();
}
