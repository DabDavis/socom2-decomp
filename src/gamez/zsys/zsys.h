/* The engine's system layer: the DMA FIFO and the VU units (OVL1 0x351xxx). Declarations only,
 * as far as callers need them; the header's name is ours. */
#ifndef ZSYS_H
#define ZSYS_H

#include <eetypes.h>

extern "C" void FlushCache(int mode);

/* the system allocator: zeroed, with a file and line SOCOM II passes as 0 (SOCOM 1's
 * _calloc__FUiUiPCci, 0x34EAE0) */
void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
void *_realloc(void *p, unsigned int size, const char *file, int line);

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

/* a vector emptied and its storage given up: cleared, then traded for an empty one's. SOCOM 1
 * writes it on one source line wherever it frees a vector (CFlashFXVector::Reset, CZObjectiveList::
 * FreeList -- its DWARF's line tables put the temporary, the clear, the swap and the temporary's
 * destructor on one line), and SOCOM II calls all four pieces out of line: an inline function
 * taller than the budget (the swap is four high), inlined one level. SOCOM 1 frees some thirty
 * vectors so (CMission::Close, CWorld's lists, Particle, CSnd, MapCompass's nav packages...).
 * The name is ours */
template <class V>
inline void FreeVector(V &v)
{
    V tmp;

    v.clear();
    v.swap(tmp);
}

#endif
