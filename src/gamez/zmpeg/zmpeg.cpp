/* zmpeg.cpp -- the movie player, CMPEG: the buffers carved from a block or allocated, a film
 * started and ticked, played whole in the background, looped and ended (SOCOM 1's zmpeg.cpp, its
 * name the demo's string; OVL1 0x309550-0x309F5F, after zMath and before playpss.cpp). SOCOM II
 * keeps SOCOM 1's functions but SetDisplayHeight, takes the buffers' height from the screen, gives
 * Run an argument it hands the player, and plays a background film through the player's own loop.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>
#include "zmpeg.h"
#include "../zvid/zvid.h"

void *_malloc(unsigned int size, const char *file, int line);
void *_memalign(unsigned int align, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

class CVideo {
public:
    static int MaxFrameRate(int rate);
    static int m_defaultFrameRate;
};

extern "C" {
extern unsigned char data_004A1790;   /* a film is running (another unit's flag) */
}

/* the buffers' height and the background flag, read by playpss.cpp too (SOCOM 1's globals, small
 * data there) */
int _Max_Height = 480;   /* 0x3DF348 */
int _Display_Height = 448;   /* 0x3DF350: tickPss centres the frame in it (SOCOM II drops its setter) */
bool BlockingPSSPlayback;   /* 0x451CB8: the film plays whole, through playPss */

/* the unit's own state (SOCOM 1's anonymous namespace; the Run argument SOCOM II's, the name ours) */
namespace {
playPssRsrcs rsrcs;   /* 0x451CC0 */
bool internal_init;   /* 0x451CF0: Run made the buffers, End frees them */
sceMpeg *p_mp;   /* 0x451CF8 */
int startArg;   /* 0x451D00: Run's argument, handed to startPss again by Loop */
bool mpegBuffAlloc;   /* 0x451D08: each buffer allocated, not carved from Init's block */
bool rgb32Alloc;   /* 0x451D10 */
bool path3tagAlloc;   /* 0x451D18 */
bool demuxBuffAlloc;   /* 0x451D20 */
bool audioBuffAlloc;   /* 0x451D28 */
}

#define MPEG_BUFF_SIZE (_Max_Height * 0x1680 / 2 + 0x176C)
#define RGB32_SIZE (_Max_Height / 16 * 0xA000)
#define PATH3TAG_SIZE ((((_Max_Height / 16) * 0xF0 + 10) * 4 + 63) / 64 * 512)

/* a buffer taken from the block when it holds the size and a 64-byte alignment, else allocated;
 * `own` set when it was allocated (the block's size unchanged) */
#define TAKE_BUFF(dst, size, own)                                                                   \
    {                                                                                               \
        unsigned int before = left;                                                                 \
        if ((size) + 64 < left) {                                                                   \
            dst = block + ((64 - (int)block) & 63);                                                 \
            left -= (size) + 64;                                                                    \
            block += (size) + 64;                                                                   \
        } else                                                                                      \
            dst = (unsigned char *)_memalign(64, size, 0, 0);                                       \
        own = before == left;                                                                       \
    }

/* the decoding buffers made, from `buf` as far as it goes (SOCOM 1's; OVL1 0x309B40).
 * OPEN, 0x370 against 0x418: the target recomputes each size from the loaded height in the test
 * and in both arms (no common subexpression across the branch), never folds the size's 0x176C and
 * the alignment's 64 into one add, and stores the pointer once after the join (a local or an
 * inline's result); the macro here, a cast to unsigned in the test, and an inline taking the block
 * and its size by reference (0x308) all fold and share. */
int CMPEG::Init(void *buf, int size)
{
    unsigned char *block = (unsigned char *)buf;
    unsigned int left = size;

    TAKE_BUFF(rsrcs.mpegBuff, MPEG_BUFF_SIZE, mpegBuffAlloc);
    rsrcs.mpegBuffSize = MPEG_BUFF_SIZE;
    TAKE_BUFF(rsrcs.rgb32, RGB32_SIZE, rgb32Alloc);
    TAKE_BUFF(*(unsigned char **)&rsrcs.path3tag, PATH3TAG_SIZE, path3tagAlloc);
    TAKE_BUFF(rsrcs.demuxBuff, 0x30000, demuxBuffAlloc);
    rsrcs.demuxBuffSize = 0x30000;
    TAKE_BUFF(rsrcs.audioBuff, 0xC000, audioBuffAlloc);
    rsrcs.audioBuffSize = 0xC000;
    rsrcs.iopBuffSize = 0x6000;
    initPssAudio(&rsrcs);
    return 1;
}

/* the buffers Init allocated freed, the audio stopped (SOCOM 1's End did it in place; OVL1
 * 0x309A10) */
void CMPEG::Uninit()
{
    if (mpegBuffAlloc)
        _free(rsrcs.mpegBuff, 0, 0);
    mpegBuffAlloc = rsrcs.mpegBuff = 0;
    if (rgb32Alloc)
        _free(rsrcs.rgb32, 0, 0);
    rgb32Alloc = rsrcs.rgb32 = 0;
    if (path3tagAlloc)
        _free(rsrcs.path3tag, 0, 0);
    path3tagAlloc = rsrcs.path3tag = 0;
    if (demuxBuffAlloc)
        _free(rsrcs.demuxBuff, 0, 0);
    demuxBuffAlloc = rsrcs.demuxBuff = 0;
    if (audioBuffAlloc)
        _free(rsrcs.audioBuff, 0, 0);
    audioBuffAlloc = rsrcs.audioBuff = 0;
    uninitPssAudio();
}

/* a film opened and started: the buffers made first if nobody gave Init a block, socom.pss always
 * played whole (SOCOM 1's, the argument SOCOM II's; OVL1 0x3098E0) */
int CMPEG::Run(const char *name, bool a, bool background, int arg)
{
    data_004A1790 = 1;
    if (strstr(name, "socom.pss"))
        background = true;
    BlockingPSSPlayback = background;
    if (!strFileOpen(name)) {
        data_004A1790 = 0;
        return 0;
    }
    if (!rsrcs.mpegBuff) {
        _Max_Height = zVid.renderHeight;
        Init(0, 0);
        internal_init = 1;
    } else
        internal_init = 0;
    p_mp = (sceMpeg *)_malloc(0x48, 0, 0);
    startArg = arg;
    startPss(p_mp, strFileRead, &rsrcs, zVid.renderBuf, zVid.displayBuf, arg);
    CVideo::MaxFrameRate(30);
    return 1;
}

/* a frame of the film, or the whole of a background one (SOCOM 1's; OVL1 0x309850) */
void CMPEG::Tick(int n)
{
    if (!BlockingPSSPlayback) {
        tickPss(p_mp, &rsrcs, n);
        return;
    }
    playPss(p_mp, &rsrcs, n);
    stopPss(p_mp);
    if (p_mp)
        _free(p_mp, 0, 0);
    p_mp = 0;
}

/* the stream kept fed until the beam reaches the frame's last lines (SOCOM 1's, the line counts
 * per video mode SOCOM II's; OVL1 0x309780).
 * OPEN, 0xB4 against 0xC4, every instruction alike but the layout: the target keeps two jump-only
 * blocks after the loop (the early exit's `b` to the epilogue, and the chain's join, the loop's
 * entry `b` to its test) where MW threads both here; tried an enclosing `if (p_mp)`, `for`, a
 * do-while with a goto into its test, a switch (cases 2, 1, 0: the tests come first). */
void CMPEG::IdleTick()
{
    unsigned int lines, last;

    if (!p_mp)
        return;
    if (zVid.syncMode == 0) {
        lines = 525;
        last = 500;
    } else if (zVid.syncMode == 1) {
        lines = 625;
        last = 600;
    } else if (zVid.syncMode == 2) {
        lines = 1050;
        last = 910;
    }
    while (*(volatile unsigned int *)0x10000000 % lines < last)
        fillBuff(p_mp, 0);
}

/* the film stopped and freed, the stream closed, the buffers Run made freed (SOCOM 1's; OVL1
 * 0x3096F0) */
void CMPEG::End()
{
    CVideo::MaxFrameRate(CVideo::m_defaultFrameRate);
    if (p_mp) {
        stopPss(p_mp);
        _free(p_mp, 0, 0);
        p_mp = 0;
    }
    strFileClose();
    if (internal_init) {
        Uninit();
        internal_init = 0;
    }
    data_004A1790 = 0;
}

/* the film over (SOCOM 1's; OVL1 0x3096C0) */
bool CMPEG::Done()
{
    bool done = testdonePss(p_mp);

    return done;
}

/* the film started again from the top, unless it plays in the background (SOCOM 1's; OVL1
 * 0x309560) */
int CMPEG::Loop()
{
    if (!BlockingPSSPlayback) {
        stopPss(p_mp);
        strFileRewind();
        memset(rsrcs.mpegBuff, 0, rsrcs.mpegBuffSize);
        memset(rsrcs.rgb32, 0, RGB32_SIZE);
        memset(rsrcs.path3tag, 0, PATH3TAG_SIZE);
        memset(rsrcs.demuxBuff, 0, 0x30000);
        memset(rsrcs.audioBuff, 0, rsrcs.audioBuffSize);
        memset(p_mp, 0, 0x48);
        startPss(p_mp, strFileRead, &rsrcs, zVid.renderBuf << 5, zVid.displayBuf << 5, startArg);
    }
    return 1;
}

/* the height the buffers are sized for (SOCOM 1's; OVL1 0x309550) */
void CMPEG::SetMaxHeight(int height)
{
    _Max_Height = height;
}
