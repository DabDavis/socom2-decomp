/* zmath_frect.cpp -- CFRect, the grid's x/z rectangle: its two constructors (OVL1 0x306620-0x30666F),
 * a unit of their own (the online lane's round 27: nothing here inlines either, and the unit before
 * ends with the tree instance 0x3065A0). THE NAME IS OURS: SOCOM 1 names no file for it (no DWARF
 * unit, no assert), but its demo links CFRect() in the same place, right before zmath_main.cpp's
 * zMath_Init (0x220590, after int_circleclip.cpp's CircleSegmentClip) -- and the zmath units link
 * alphabetically (zmath_main, zmath_matrix, zmath_pnt3d, zmath_pnt4d), so this one sorts before
 * zmath_main.cpp: a global class like the zmath types, with zmath's prefix. The class is znode.h's
 * (SOCOM 1's DWARF: top, bottom, left, right). Defined in reverse of their image order (-inline
 * deferred). */

#include "../znode/znode.h"

/* an empty rectangle (SOCOM 1's, the same 0x18 bytes) */
CFRect::CFRect()
{
    top = 0.0f;
    bottom = 0.0f;
    left = 0.0f;
    right = 0.0f;
}

/* from two corners: top and bottom their z, left and right their x */
CFRect::CFRect(const CPnt3D *a, const CPnt3D *b)
{
    top = a->z;
    bottom = b->z;
    left = a->x;
    right = b->x;
}
