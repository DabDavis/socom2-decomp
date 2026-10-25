/* zparticle_buf.cpp -- the particle draw buffer (OVL1 0x324C70-0x326F4F): the frame's particles
 * gathered as buffer elements, sorted back to front and drawn as GS packets. The unit name is
 * SOCOM 1's (its __sinit_zparticle_buf.cpp; SOCOM II's 0x402BB0 builds the 5000 32-byte elements
 * at 0x45C4B0 with the constructor 0x326F40). SOCOM 1's demo holds DrawRange to the element's
 * constructor in this order. Defined in reverse of their image order (-inline deferred). Written
 * so far: the sort's comparison. */

#include "zparticle.h"

namespace Particle {

/* one particle as drawn (SOCOM 1's BufferElement, 0x20 bytes; no DWARF): its depth at 0x1C (the
 * members' names ours) */
class BufferElement {
public:
    unsigned char _0[0x1c];   // 0x0
    float m_depth;   // 0x1c
};   // 0x20 bytes

int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b);

/* SOCOM 1's: two elements ordered by depth, the difference's bits returned as an int (its sign
 * the float's) */
int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b)
{
    float d = a->m_depth - b->m_depth;

    return *(int *)&d;
}

}
