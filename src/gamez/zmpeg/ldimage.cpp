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
 * MW takes v0 here, void or returning the call's int alike; round 17: a local copy of each argument
 * (height, dbp, width, image, tags, a named 1024, width and height in either order) changes nothing --
 * the node holding v0 in the target is not a local the source can name. Round 18: MW's own colouring
 * (the igdump tools) has the height's copy with neighbours zero, a0-a3, t0, t1 only, so it takes the
 * lowest free, v0: the target's v1 needs a node coloured v0 live across the shuffle that leaves no
 * instruction. Const locals of each argument (the engine core's round 7 rule), a dead first
 * definition, `-O4,s`, `-O3,p`, `-opt nopeephole`/`noschedule`: all 2 (SOCOM 1's 2.4 build has v1 too).
 * Round 22 (build/scratch/onl/ldi1-3.cpp): the PCode has the call define v0/v1 and the return use v0,
 * so nothing the call or the return does keeps v0 live across the shuffle -- an int function falling
 * off its end (v0 read, never set), an inline wrapper, the arguments copied into locals, 1024 named,
 * a function-pointer call, a dead test of the width, signedness casts on each argument (the radio
 * tick's tell): all 2 */
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
 * keeps `& 0x0FFFFFFF` and `& -16` apart); the position a block-local of its own */
void setLoadImageTagsYX(unsigned int *tags, void *image, int bw, int x, int y, unsigned int dbp)
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
