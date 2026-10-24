/* The engine's system layer: the DMA FIFO and the VU units (OVL1 0x351xxx). Declarations only,
 * as far as callers need them; the header's name is ours. */
#ifndef ZSYS_H
#define ZSYS_H

#include <eetypes.h>

extern "C" void FlushCache(int mode);

/* the system allocator: zeroed, with a file and line SOCOM II passes as 0 (SOCOM 1's
 * _calloc__FUiUiPCci, 0x34EAE0) */
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);

/* A vector unit: CVU(0) is VU0, built and destroyed around an upload (zMath_Init). */
class CVU {
public:
    CVU(unsigned int unit);
    ~CVU();
    void Code(u_long128 *code);

    unsigned int m_unit;        /* 0x00 */
    void *m_vif;                /* 0x04 */
    u_long128 *m_code;          /* 0x08 */
    int m_memsize;              /* 0x0C: 256 (VU0) or 1024 qwords */
    int m_unk10;
};

#endif
