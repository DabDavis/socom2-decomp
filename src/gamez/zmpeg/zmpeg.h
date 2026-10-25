/* The movie player: CMPEG (SOCOM 1's zmpeg.cpp, its file name the demo's string) over Sony's
 * playpss sample (SOCOM 1's playpss.cpp: the PSS demuxer's callbacks, the ADPCM-less audio decoder
 * that streams PCM to the IOP, the image loader and the stream file), compiled as C++ in both builds.
 * SOCOM 1's demo names every function; it has no DWARF for these units, so the structures are
 * read off the code (the names are Sony's sample's where the role is plain, ours otherwise). OVL1
 * 0x309550-0x30BD3F, after zMath. The header's name is ours. */
#ifndef ZMPEG_H
#define ZMPEG_H

/* libmpeg's decoder state (only its address is passed here) */
struct sceMpeg;
struct sceMpegCbData;
struct sceMpegCbDataStr;

/* the decoding buffers handed to the player (SOCOM II's 0x451CC0; Sony's playPssRsrcs) */
struct playPssRsrcs {
    unsigned char *mpegBuff;   // 0x0: sceMpegCreate's work area
    int mpegBuffSize;   // 0x4
    unsigned char *rgb32;   // 0x8: a frame's macroblocks
    unsigned int *path3tag;   // 0xc: their upload's GIF tags
    unsigned char *demuxBuff;   // 0x10
    int demuxBuffSize;   // 0x14
    unsigned char *audioBuff;   // 0x18
    int audioBuffSize;   // 0x1c
    int _20;   // 0x20
    int iopBuffSize;   // 0x24
};   // 0x28

/* the stream's reader: called with a buffer and its size, the bytes read returned */
typedef int (*PssReadFunc)(void *buf, int size);

/* playpss.cpp */
int strFileOpen(const char *name);
void strFileRewind();
void strFileClose();
int strFileRead(void *buf, int size);
void initPssAudio(playPssRsrcs *rsrcs);
void uninitPssAudio();
int startPss(sceMpeg *mpeg, PssReadFunc read, playPssRsrcs *rsrcs, unsigned int width,
             unsigned int height, int arg);
void tickPss(sceMpeg *mpeg, playPssRsrcs *rsrcs, unsigned int n);
bool testdonePss(sceMpeg *mpeg);
void stopPss(sceMpeg *mpeg);
void playPss(sceMpeg *mpeg, playPssRsrcs *rsrcs, unsigned int n);
void fillBuff(sceMpeg *mpeg, int n);

/* the movie player: all static (SOCOM 1's class) */
class CMPEG {
public:
    static int Init(void *buf, int size);
    static void Uninit();
    static int Run(const char *name, bool a, bool background, int arg);
    static void Tick(int n);
    static void IdleTick();
    static void End();
    static bool Done();
    static int Loop();
    static void SetMaxHeight(int height);
};

#endif
