/* ldimage.cpp -- a decoded frame's upload: the DMA chain of GIF image transfers, one per macroblock,
 * built and kicked on the GIF channel (Sony's ezmpeg sample ldimage.c, compiled as C++: SOCOM 1's
 * demo names the functions). OVL1 0x30B740-0x30B9CF: tickPss calls loadImage and
 * setLoadImageTags out of line, so these are a unit of their own (the name Sony's file's). Defined
 * in reverse of their image order (-inline deferred). */

#include "zmpeg.h"

#define D2_CHCR ((volatile unsigned int *)0x1000A000)
#define D2_QWC ((volatile unsigned int *)0x1000A020)
#define D2_TADR ((volatile unsigned int *)0x1000A030)

static void setLoadImageTagsYX(unsigned int *tags, void *image, int bw, int x, int y, unsigned int dbp);

/* the chain for a frame at (0, 0) of the frame buffer `dbp`, the macroblocks 1024 bytes apart
 * (Sony's; OVL1 0x30B9B0). Byte-exact since the online lane's round 23 (open since round 17, 2 off:
 * the height's copy took v0 where the target has v1): setLoadImageTagsYX is STATIC. MW knows the
 * registers an internal-linkage callee generated earlier in the unit writes (-inline deferred
 * generates in emission order, so setLoadImageTagsYX first) and kills only those at the call; it
 * never writes v0, so the return's read of v0 keeps v0 live from the entry across the shuffle and
 * the copy takes the next free register, v1 (as c2dfont.cpp's ReadPlacement keeps a value in a2
 * across its anonymous-namespace callees). An external callee kills every volatile register */
void setLoadImageTags(unsigned int *tags, void *image, int width, int height, unsigned int dbp)
{
    setLoadImageTagsYX(tags, image, 1024, width, height, dbp);
}

/* the chain for an x by y frame of 16x16 macroblocks `bw` bytes apart, written through the uncached
 * mapping: a DMA cnt with the GIF A+D tag setting BITBLTBUF (frame buffer `dbp`, 640 wide, CT32) and
 * TRXREG (16 x 16); then per macroblock, column by column, a cnt with TRXPOS and TRXDIR, the IMAGE
 * tag (64 qwords, EOP on the last) and a ref to the block; a DMA end closing it (Sony's; OVL1
 * 0x30B770: it sets no v0, so void). The cursor declared first (s4) but set after the counts -- x's
 * register then free for the conversion; the block address masked through a pointer cast (the target
 * keeps `& 0x0FFFFFFF` and `& -16` apart); the position a block-local of its own. Static: its one
 * caller's register shows it (setLoadImageTags above) */
static void setLoadImageTagsYX(unsigned int *tags, void *image, int bw, int x, int y, unsigned int dbp)
{
    unsigned int *t;
    int mbx = x >> 4;
    int mby = y >> 4;
    int i, j;
    unsigned long v;

    t = (unsigned int *)(((unsigned int)tags & 0x0FFFFFFF) | 0x20000000);
    *(unsigned long *)t = 0x10000003;
    t[4] = 2;
    t[5] = 0x10000000;
    t[6] = 0xE;
    t[7] = 0;
    v = (unsigned long)dbp << 32 | (unsigned long)10 << 48;
    t[8] = (unsigned int)(v & 0xFFFFFFFF);
    t[9] = (unsigned int)(v >> 32);
    t[10] = 0x50;
    t[11] = 0;
    t[12] = 16;
    t[13] = 16;
    t[14] = 0x52;
    t[15] = 0;
    t += 16;
    for (i = 0; i < mbx; i++) {
        for (j = 0; j < mby; j++) {
            bool eop = j == mby - 1 && i == mbx - 1;

            *(unsigned long *)t = 0x10000004;
            t[4] = 2;
            t[5] = 0x10000000;
            t[6] = 0xE;
            t[7] = 0;
            unsigned long p = (unsigned long)(unsigned int)(i * 16) << 32 | (unsigned long)(unsigned int)(j * 16) << 48;
            t[8] = (unsigned int)(p & 0xFFFFFFFF);
            t[9] = (unsigned int)(p >> 32);
            t[10] = 0x51;
            t[11] = 0;
            t[12] = 0;
            t[13] = 0;
            t[14] = 0x53;
            t[15] = 0;
            t[16] = eop << 15 | 0x40;
            t[17] = 0x08000000;
            t[18] = 0;
            t[19] = 0;
            *(unsigned long *)&t[20] =
                (unsigned long)((unsigned int)(void *)((unsigned int)image & 0x0FFFFFFF) & ~0xF) << 32 | 0x30000040;
            image = (char *)image + bw;
            t += 24;
        }
    }
    *(unsigned long *)t = 0x70000000;
}

/* the chain started on the GIF channel (Sony's; OVL1 0x30B740) */
void loadImage(unsigned int *tags)
{
    *D2_TADR = (unsigned int)tags & 0x0FFFFFFF;
    *D2_QWC = 0;
    *D2_CHCR = 0x105;
}
