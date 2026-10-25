/* zparticle_buf.cpp -- the particle draw buffer (OVL1 0x324C70-0x326F4F): the frame's particles
 * gathered as buffer elements, sorted back to front and drawn as GS packets. The unit name is
 * SOCOM 1's (its __sinit_zparticle_buf.cpp; SOCOM II's 0x402BB0 builds the 5000 32-byte elements
 * at 0x45C4B0 with the constructor 0x326F40). SOCOM 1's demo holds DrawRange to the element's
 * constructor in this order. Defined in reverse of their image order (-inline deferred). Written
 * so far: the sort's comparison. */

#include "zparticle.h"

/* the renderer's pipe, as much as the buffer reads: its camera (zrndr_pipe.cpp's class; the
 * camera's matrix set's view at 0x3F0 -- SOCOM 1's 0x3C0, in its m_mtxSet -- the names ours) */
struct ZCAM_VIEW {
    unsigned char _0[0x20];   // 0x0
    float m[4][4];   // 0x20
};
class CPipeCamera {
public:
    unsigned char _0[0x3f0];   // 0x0
    ZCAM_VIEW *m_mtx;   // 0x3f0
};
class CPipe {
public:
    unsigned char _0[0x628];   // 0x0
    CPipeCamera *m_camera;   // 0x628
};
extern CPipe thePipe;   // 0x4887C0

namespace Particle {

/* one particle as drawn (SOCOM 1's BufferElement, 0x20 bytes; no DWARF): its instance at 0x10,
 * its depth at 0x1C (the members' names ours) */
class BufferElement {
public:
    void Draw(float (&m)[4][4]);
    void DrawRotated(float (&m)[4][4]);
    void DrawXZ(float (&m)[4][4]);
    void DrawStreaked(float (&m)[4][4]);

    unsigned char _0[0x10];   // 0x0
    Instance *m_instance;   // 0x10
    unsigned char _14[0x8];   // 0x14
    float m_depth;   // 0x1c
};   // 0x20 bytes

/* the sorted elements, drawn from the last back to the first (SOCOM 1's) */
extern BufferElement *bufferPtr[5000];   // 0x4835F0
extern BufferElement **pParticleBuf;   // 0x4835D8
extern zdb::CTexHandle *m_lastTexture;   // 0x4835D0

int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b);

/* SOCOM 1's: every buffered element drawn by its source's type, last first, through the pipe
 * camera's view matrix */
void DrawFlush()
{
    m_lastTexture = 0;
    while (pParticleBuf >= bufferPtr) {
        BufferElement *e = *pParticleBuf;
        float (&m)[4][4] = thePipe.m_camera->m_mtx->m;

        switch (e->m_instance->m_source->m_drawType) {
        case PT_BILLBOARD:
            e->Draw(m);
            break;
        case PT_ROTATED:
            e->DrawRotated(m);
            break;
        case PT_STREAKED:
            e->DrawStreaked(m);
            break;
        case PT_XZ:
            e->DrawXZ(m);
            break;
        }
        pParticleBuf--;
    }
    m_lastTexture = 0;
}

/* SOCOM 1's: two elements ordered by depth, the difference's bits returned as an int (its sign
 * the float's) */
int bufferPtrCompare(BufferElement *const &a, BufferElement *const &b)
{
    float d = a->m_depth - b->m_depth;

    return *(int *)&d;
}

}
