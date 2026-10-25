/* zsys_vu.cpp -- the vector units: CVU uploads microcode by DMA, and the first VU0 user installs
 * the VIF0 interrupt handler and fills VU0's data memory with 0xCD (OVL1 0x351380-0x3515CF). SOCOM
 * 1 links these after the FIFO code and before the valves; the file name is ours. Defined in
 * reverse of their image order (-inline deferred). */

#include "zsys_public.h"

#define VIF0_FBRST ((volatile unsigned int *)0x10003810)
#define VU0_MEM ((u_long128 *)0x11004000)

/* VIF0 stopped: SOCOM 1's file static */
static int m_running;   /* 0x49E7E0 */

int CVU::m_refCount;        /* 0x49E370 */
int CVU::m_VifHandlerID;    /* 0x49E378 */

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
