/* int_shapes.h -- int_shapes.cpp's inline for other units: GetBBoxCorners, a box's eight corners
 * (the header's name is ours). AN INLINE IN THE ORIGINAL (the zdb world's round 31): 0x2DCCD0 is
 * int_shapes.cpp's out-of-line copy of it -- SOCOM 1's CBBox8::Build store for store, the copy
 * generated right after SetCDIBBox, whose corners' constructor it makes too big to expand whole
 * (inlined one level: the member array's CPnt3D() constructions a loop, this called). A unit
 * building the corners through that device needs the body visible: node_world.cpp and
 * zrndr_pipe.cpp each defined it again for that until this header (the engine core's round 26,
 * asked by the world's round 31). node_world.h and zparticle.h declare it; a unit including both a
 * declaration and this header sees the declaration first, then this definition. */
#ifndef INT_SHAPES_H
#define INT_SHAPES_H

#include "../zmath/zmath.h"

namespace zdb {
/* a corner set from three values (the arguments evaluated right to left, so z, y and x are loaded
 * before the stores; an inline leaves no symbol: the name is ours) */
inline void BoxCornerSet(CPnt3D &p, float x, float y, float z)
{
    p.x = x;
    p.y = y;
    p.z = z;
}

/* a box's eight corners: the bottom four, then the top four, each ring from the low x, high z
 * corner (new in SOCOM II, after SOCOM 1's SetCDIBBox; the name is ours) */
inline void GetBBoxCorners(CPnt3D *out, const CBBox *b)
{
    BoxCornerSet(out[0], b->m_min.x, b->m_min.y, b->m_max.z);
    out[1] = b->m_min;
    BoxCornerSet(out[2], b->m_max.x, b->m_min.y, b->m_min.z);
    BoxCornerSet(out[3], b->m_max.x, b->m_min.y, b->m_max.z);
    BoxCornerSet(out[4], b->m_min.x, b->m_max.y, b->m_max.z);
    BoxCornerSet(out[5], b->m_min.x, b->m_max.y, b->m_min.z);
    BoxCornerSet(out[6], b->m_max.x, b->m_max.y, b->m_min.z);
    out[7] = b->m_max;
}
}

#endif
