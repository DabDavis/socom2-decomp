/* The movie player: CMPEG (SOCOM 1's zmpeg.cpp, its file name the demo's string) over Sony's
 * ezmpeg/playpss sample, compiled as C++ in both builds: playpss.cpp (the demo's string: the PSS
 * demuxer's callbacks and the player), and -- named after Sony's sample files, the units' extent
 * read off their calls, which -inline auto would have inlined within one unit -- audiodec.cpp (the
 * PCM streamed to the IOP, SOCOM II's through 989snd's movie-sound calls), ldimage.cpp (the
 * frame's upload) and strfile.cpp (the film read from the disc). SOCOM 1's demo names every
 * function and global; it has no DWARF for these units, so the structures are read off the code
 * (the names are Sony's sample's where the role is plain, ours otherwise). OVL1 0x309550-0x30BD3F,
 * after zMath. The header's name is ours. */
#ifndef ZMPEG_H
#define ZMPEG_H

/* libmpeg's decoder state and its callbacks' data (Sony's; only what is read here) */
struct sceMpeg {
    int width;   // 0x0
    int height;   // 0x4
    int frameCount;   // 0x8
    unsigned char _c[0x48 - 0xc];
};   // 0x48 (CMPEG::Run allocates 72 bytes)

struct sceMpegCbData {
    int type;   // 0x0
};

struct sceMpegCbDataStr {
    int type;   // 0x0
    int _4;   // 0x4
    unsigned char *header;   // 0x8: the packet's data
    unsigned int len;   // 0xc
};

/* the decoding buffers handed to the player (SOCOM II's 0x451CC0; Sony's playPssRsrcs) */
struct playPssRsrcs {
    unsigned char *mpegBuff;   // 0x0: sceMpegCreate's work area
    int mpegBuffSize;   // 0x4
    unsigned char *rgb32;   // 0x8: a frame's macroblocks
    unsigned int *path3tag;   // 0xc: their upload's DMA tags
    unsigned char *demuxBuff;   // 0x10
    int demuxBuffSize;   // 0x14
    unsigned char *audioBuff;   // 0x18
    int audioBuffSize;   // 0x1c
    int _20;   // 0x20
    int iopBuffSize;   // 0x24
};   // 0x28

/* the audio decoder: the stream's header, the PCM ring and its IOP side (Sony's AudioDec, SOCOM 1's
 * 0x5C bytes) */
enum {
    AU_STATE_INIT = 0,
    AU_STATE_PRESET = 1,
    AU_STATE_PLAYING = 2,
    AU_STATE_PAUSE = 3
};

#define AU_HDR_SIZE 0x28
#define UNIT_SIZE 1024

struct AudioDec {
    int state;   // 0x0
    unsigned char hdr[AU_HDR_SIZE];   // 0x4: 'SShd' and 'SSbd'
    unsigned int hdrCount;   // 0x2c
    unsigned char *data;   // 0x30: the ring
    int put;   // 0x34
    int count;   // 0x38
    int size;   // 0x3c
    int totalBytes;   // 0x40
    int iopBuff;   // 0x44
    int iopBuffSize;   // 0x48
    int iopLastPos;   // 0x4c
    int iopPausePos;   // 0x50
    int totalBytesSent;   // 0x54
    int iopZero;   // 0x58
};   // 0x5c

/* the stream's reader: called with a buffer and its size, the bytes read returned */
typedef int (*PssReadFunc)(void *buf, int size);

/* audiodec.cpp */
int audioDecCreate(AudioDec *ad, unsigned char *buff, int buffSize, int iopBuffSize);
int audioDecDelete(AudioDec *ad);
void audioDecStart(AudioDec *ad);
void audioDecReset(AudioDec *ad);
void audioDecBeginPut(AudioDec *ad, unsigned char **ptr0, int *len0, unsigned char **ptr1, int *len1);
void audioDecEndPut(AudioDec *ad, int size);
bool audioDecIsPreset(AudioDec *ad);
int audioDecSendToIOP(AudioDec *ad);

/* ldimage.cpp */
void loadImage(unsigned int *tags);
int setLoadImageTagsYX(unsigned int *tags, void *image, int bw, int x, int y, unsigned int dbp);
int setLoadImageTags(unsigned int *tags, void *image, int x, int y, unsigned int dbp);

/* strfile.cpp */
int strFileOpen(const char *name);
int strFileRewind();
int strFileClose();
int strFileRead(void *buf, int size);

/* playpss.cpp */
void initPssAudio(playPssRsrcs *rsrcs);
void uninitPssAudio();
sceMpeg *startPss(sceMpeg *mpeg, PssReadFunc read, playPssRsrcs *rsrcs, unsigned int width,
                  unsigned int height, int arg);
int tickPss(sceMpeg *mpeg, playPssRsrcs *rsrcs, unsigned int n);
bool testdonePss(sceMpeg *mpeg);
void stopPss(sceMpeg *mpeg);
void playPss(sceMpeg *mpeg, playPssRsrcs *rsrcs, unsigned int n);
int fillBuff(sceMpeg *mpeg, int wait);

/* zmpeg.cpp's globals playpss.cpp reads */
extern int _Max_Height;
extern bool BlockingPSSPlayback;

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
