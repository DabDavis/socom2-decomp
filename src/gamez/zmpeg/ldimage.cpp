/* ldimage.cpp -- a decoded frame's upload: the DMA chain of GIF image transfers, one per macroblock,
 * built and kicked on the GIF channel (Sony's ezmpeg sample ldimage.c, compiled as C++: SOCOM 1's
 * demo names the functions). OVL1 0x30B740-0x30B9CF: tickPss calls loadImage and
 * setLoadImageTags out of line, so these are a unit of their own (the name Sony's file's). Defined
 * in reverse of their image order (-inline deferred). */

#include "zmpeg.h"

#define D2_CHCR ((volatile unsigned int *)0x1000A000)
#define D2_QWC ((volatile unsigned int *)0x1000A020)
#define D2_TADR ((volatile unsigned int *)0x1000A030)

/* the chain for a frame at (0, 0) of the frame buffer `dbp`, the macroblocks 1024 bytes apart
 * (Sony's; OVL1 0x30B9B0).
 * OPEN, 2 instructions, a register: the target swaps the height through v1 on its way to t0 where
 * MW takes v0 here, void or returning the call's int alike. */
int setLoadImageTags(unsigned int *tags, void *image, int width, int height, unsigned int dbp)
{
    return setLoadImageTagsYX(tags, image, 1024, width, height, dbp);
}

/* the chain started on the GIF channel (Sony's; OVL1 0x30B740) */
void loadImage(unsigned int *tags)
{
    *D2_TADR = (unsigned int)tags & 0x0FFFFFFF;
    *D2_QWC = 0;
    *D2_CHCR = 0x105;
}
