/* playpss.cpp -- the PSS player: the stream demuxed into the IPU's DMA ring and the audio decoder,
 * the frames decoded and uploaded, the film started, ticked, played whole and stopped (Sony's ezmpeg
 * sample playpss.c, compiled as C++: SOCOM 1's demo names the file -- its string -- and the
 * functions). OVL1 0x30A860-0x30B73F, between audiodec.cpp and ldimage.cpp (it calls both out of
 * line). SOCOM II adds the stop flag, the frame-buffer pair startPss is given, the decode budget the
 * background callback spends, and playPss's own loop. Defined in reverse of their image order
 * (-inline deferred). */

#include <string.h>
#include "zmpeg.h"
#include "../zvid/zvid.h"
#include "../zinput/zinput.h"

/* libgraph's display environment (Sony's: the GS's display registers as bit-fields) */
struct tGS_PMODE { unsigned long v; };
struct tGS_SMODE2 { unsigned long v; };
struct tGS_DISPFB2 {
    unsigned long FBP : 9;
    unsigned long FBW : 6;
    unsigned long PSM : 5;
    unsigned long p0 : 12;
    unsigned long DBX : 11;
    unsigned long DBY : 11;
    unsigned long p1 : 10;
};
struct tGS_DISPLAY2 {
    unsigned int DX : 12;
    unsigned int DY : 11;
    unsigned int MAGH : 4;
    unsigned int MAGV : 2;
    unsigned int p0 : 3;
    unsigned int DW : 12;
    unsigned int DH : 11;
    unsigned int p1 : 9;
};
struct tGS_BGCOLOR { unsigned long v; };
struct sceGsDispEnv {
    tGS_PMODE pmode;
    tGS_SMODE2 smode2;
    tGS_DISPFB2 dispfb;
    tGS_DISPLAY2 display;
    tGS_BGCOLOR bgcolor;
};

extern "C" {
int sceGsSetDefDispEnv(sceGsDispEnv *disp, short psm, short w, short h, short dx, short dy);
void sceGsPutDispEnv(sceGsDispEnv *disp);
int func_001BDA78(int port, unsigned char *data, int n);   /* (libpad2: the pad read) */
void func_003B0660(void);   /* (zvid: the frame's wait) */

/* libmpeg (MAIN) */
void sceMpegCreate(sceMpeg *mp, unsigned char *work_area, int work_area_size);
int sceMpegAddStrCallback(sceMpeg *mp, int strType, int ch,
                          int (*cb)(sceMpeg *, sceMpegCbDataStr *, void *), void *anyData);
int sceMpegAddCallback(sceMpeg *mp, int type, int (*cb)(sceMpeg *, sceMpegCbData *, void *),
                       void *anyData);
int sceMpegDemuxPss(sceMpeg *mp, unsigned char *pss, int size);
int sceMpegGetPicture(sceMpeg *mp, void *rgb32, int mbcount);
void func_001BB688(void);   /* sceMpegInit */
int func_001BBAB8(sceMpeg *mp);   /* sceMpegIsEnd */
void func_001BBAD8(sceMpeg *mp);   /* sceMpegReset */
void func_001BB948(sceMpeg *mp);   /* sceMpegDelete */

int func_00198320(void *p);   /* (MAIN: the decoder's error test) */
void func_00198378(void *p, int v);   /* (MAIN: the error raised) */
int sceDmaSync(void *d, int mode, int timeout);
extern void *data_0049E2B8;   /* zSys's DMA channel (+8) */
extern unsigned char data_00455E50[];   /* the decoder's error state (SOCOM II's) */
}

extern int _Display_Height;   /* zmpeg.cpp's */

#define MUX_BUFF_SIZE 0x4000
#define D4_CHCR ((volatile unsigned int *)0x1000B400)
#define D4_MADR ((volatile unsigned int *)0x1000B410)
#define D4_QWC ((volatile unsigned int *)0x1000B420)
#define T0_COUNT ((volatile unsigned int *)0x10000000)
#define UNCACHED(p) ((void *)(((unsigned int)(p) & 0x0FFFFFFF) | 0x20000000))
#define PATH3TAG_HALF ((((_Max_Height / 16) * 0xF0 + 10) * 4 + 63) / 64 * 256)

/* the player's state (SOCOM 1's globals by name; SOCOM II's additions named ours) */
int isStarted;   /* 0x451D38: the audio started */
AudioDec audioDec;   /* 0x451D40 */
PssReadFunc fileRead;   /* 0x451DA0 */
int decodeBudget;   /* 0x451DA8: the background callback's timer ticks (SOCOM II's) */
unsigned char OKtoFillBuff;   /* 0x451DB0 */
int gFrameCount;   /* 0x451DB8 */
unsigned char audioStarted;   /* 0x451DC0: the first audio packet with its flag seen (SOCOM II's) */
unsigned char muxBuff[MUX_BUFF_SIZE];   /* 0x451E00 */
int muxBuffFullness;   /* 0x455E00 */
unsigned int *path3tag;   /* 0x455E08 */
unsigned char *demuxBuff;   /* 0x455E10 */
int demuxBuffSize;   /* 0x455E18 */
unsigned char *demuxBuffPast;   /* 0x455E20 */
unsigned char *demuxBuffPresent;   /* 0x455E28 */
unsigned char *demuxBuffFuture;   /* 0x455E30 */
unsigned char pssDone;   /* 0x455E38: stopped, or a button pressed (SOCOM II's) */
int dbp[2];   /* 0x455E40: the two frame buffers */

int videoCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data);
int audioCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data);
int nodataCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data);
int backgroundCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data);
int errorCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data);

/* the audio decoder made over the buffers (Sony's; OVL1 0x30B700) */
void initPssAudio(playPssRsrcs *rsrcs)
{
    audioDecCreate(&audioDec, rsrcs->audioBuff, rsrcs->audioBuffSize, rsrcs->iopBuffSize);
    isStarted = 0;
}

/* (Sony's; OVL1 0x30B6F0) */
void uninitPssAudio()
{
    audioDecDelete(&audioDec);
}

/* the film started: the decoder and its callbacks, the demux ring, the stream demuxed until the
 * decoder has its first data (Sony's, the frame buffers and the audio channel SOCOM II's; OVL1
 * 0x30B4D0) */
sceMpeg *startPss(sceMpeg *mp, PssReadFunc read, playPssRsrcs *rsrcs, unsigned int width,
                  unsigned int height, int arg)
{
    dbp[0] = width;
    dbp[1] = height;
    pssDone = 0;
    path3tag = rsrcs->path3tag;
    func_001BB688();
    audioDecReset(&audioDec);
    sceMpegCreate(mp, rsrcs->mpegBuff, rsrcs->mpegBuffSize);
    sceMpegAddStrCallback(mp, 0, 0, videoCallback, 0);
    sceMpegAddStrCallback(mp, 2, arg, audioCallback, 0);
    if (arg > 0)
        sceMpegAddStrCallback(mp, 2, 0, audioCallback, 0);
    audioStarted = 0;
    sceMpegAddCallback(mp, 1, nodataCallback, 0);
    sceMpegAddCallback(mp, 4, backgroundCallback, 0);
    sceMpegAddCallback(mp, 0, errorCallback, 0);
    muxBuffFullness = 0;
    demuxBuff = rsrcs->demuxBuff;
    demuxBuffSize = rsrcs->demuxBuffSize;
    fileRead = read;
    demuxBuffPresent = demuxBuff;
    demuxBuffPast = demuxBuff + 16;
    demuxBuffFuture = demuxBuff + 16;
    while (fillBuff(mp, 1))
        ;
    return mp;
}

/* a frame: decoded, uploaded to the frame buffer not shown, the audio sent on; the film stopped on
 * the decoder's error (Sony's; OVL1 0x30B1D0) */
int tickPss(sceMpeg *mp, playPssRsrcs *rsrcs, unsigned int fbp)
{
    OKtoFillBuff = 0;
    if (!func_00198320(data_00455E50)) {
        while (sceMpegGetPicture(mp, rsrcs->rgb32, _Max_Height / 16 * 40) < 0)
            ;
    } else {
        stopPss(mp);
        return 0;
    }
    if (mp->frameCount == 0) {
        int y = (_Display_Height - mp->height) * 10 / 2;
        unsigned int r = y % 320;

        if (r > 160)
            r -= 320;
        y -= r;
        if (y < 0)
            y = 0;
        setLoadImageTags(path3tag, rsrcs->rgb32, mp->width, mp->height, y + (dbp[0] << 5));
        setLoadImageTags((unsigned int *)((unsigned char *)path3tag + PATH3TAG_HALF), rsrcs->rgb32,
                         mp->width, mp->height, y + (dbp[1] << 5));
        decodeBudget = (int)(0.000151f * ((float)mp->width * (float)mp->height));
    }
    gFrameCount = mp->frameCount;
    sceDmaSync(data_0049E2B8, 0, 0);
    loadImage(fbp == dbp[0] ? path3tag : (unsigned int *)((unsigned char *)path3tag + PATH3TAG_HALF));
    audioDecSendToIOP(&audioDec);
    if (!isStarted && audioDecIsPreset(&audioDec)) {
        audioDecStart(&audioDec);
        isStarted = 1;
    }
    return 0;
}

/* the film over, or stopped (Sony's; OVL1 0x30B180) */
bool testdonePss(sceMpeg *mp)
{
    if (pssDone || func_001BBAB8(mp))
        return true;
    return false;
}

/* the film stopped (Sony's, the flag SOCOM II's; OVL1 0x30B120) */
void stopPss(sceMpeg *mp)
{
    pssDone = 1;
    audioDecReset(&audioDec);
    isStarted = 0;
    audioStarted = 0;
    func_001BBAD8(mp);
    func_001BB948(mp);
}

/* a background film played whole: the display set up, a frame each vertical blank into the frame
 * buffers in turn, until it ends or a button stops it (SOCOM II's, in SOCOM 1's playPss's place;
 * OVL1 0x30AF40) */
void playPss(sceMpeg *mp, playPssRsrcs *rsrcs, unsigned int n)
{
    sceGsDispEnv disp;
    int toggle = 1;

    sceGsSetDefDispEnv(&disp, 0, zVid.renderWidth, zVid.renderHeight, 0, 0);
    if (zVid.syncMode == 2 && zVid.renderHeight == 448)
        disp.display.DY += 18;
    disp.dispfb.FBW = 10;
    sceGsPutDispEnv(&disp);
    func_003B0660();
    while (!testdonePss(mp)) {
        if (toggle) {
            toggle = 0;
            disp.dispfb.FBP = dbp[1];
        } else {
            toggle = 1;
            disp.dispfb.FBP = dbp[0];
        }
        tickPss(mp, rsrcs, disp.dispfb.FBP);
        CMPEG::IdleTick();
        func_003B0660();
        sceGsPutDispEnv(&disp);
        CInput::Tick(1.0f / 30.0f);
        if (func_001BDA78(CInput::m_pads[0]->m_port, CInput::m_pads[0]->m_e4, 14))
            pssDone = 1;
    }
}

/* the video stream copied into the demux ring, when it fits (Sony's; OVL1 0x30ADD0) */
int videoCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data)
{
    int availableSize;
    int spill;

    availableSize = demuxBuffPresent - demuxBuffPast;
    if (availableSize < 0)
        availableSize += demuxBuffSize;
    if (availableSize == 0 && demuxBuffPresent == demuxBuffFuture)
        availableSize = demuxBuffSize;
    if ((unsigned int)availableSize < str->len)
        return 0;
    spill = str->len + (demuxBuffPast - demuxBuff) - demuxBuffSize;
    while ((*D4_CHCR & 0x100) == 1)
        ;
    if (spill > 0) {
        memcpy(UNCACHED(demuxBuffPast), str->header, str->len - spill);
        memcpy(UNCACHED(demuxBuff), str->header + (str->len - spill), spill);
        demuxBuffPast = demuxBuff + spill;
    } else {
        memcpy(UNCACHED(demuxBuffPast), str->header, str->len);
        demuxBuffPast += str->len;
    }
    return 1;
}

/* the audio stream put into the decoder's ring, from the first packet that carries the flag (Sony's,
 * the flag SOCOM II's; OVL1 0x30ACA0) */
int audioCallback(sceMpeg *mp, sceMpegCbDataStr *str, void *data)
{
    unsigned char *ptr0, *ptr1;
    int len0, len1;
    int spill;
    unsigned char flag = str->header[3];

    if (flag)
        audioStarted = 1;
    if (audioStarted && !flag)
        return 1;
    audioDecBeginPut(&audioDec, &ptr0, &len0, &ptr1, &len1);
    str->len -= 4;
    str->header += 4;
    if (len0 + len1 < str->len)
        return 0;
    spill = str->len - len0;
    if (spill > 0) {
        memcpy(ptr0, str->header, str->len - spill);
        memcpy(ptr1, str->header + (str->len - spill), spill);
    } else
        memcpy(ptr0, str->header, str->len);
    audioDecEndPut(&audioDec, str->len);
    return 1;
}

/* the decoder starved: the stream demuxed until the ring holds a block, which goes to the IPU by DMA
 * (Sony's; OVL1 0x30AAA0) */
int nodataCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    int availableSize;
    int size;

    availableSize = demuxBuffPast - demuxBuffFuture;
    demuxBuffPresent = demuxBuffFuture;
    if (availableSize < 0)
        availableSize += demuxBuffSize;
    while (availableSize < 0x1000) {
        if (!fillBuff(mp, 1)) {
            availableSize += 15;
            break;
        }
        availableSize = demuxBuffPast - demuxBuffFuture;
        if (availableSize < 0)
            availableSize += demuxBuffSize;
    }
    size = availableSize & ~15;
    if (size > 0x1000)
        size = 0x1000;
    if (demuxBuffPresent + size > demuxBuff + demuxBuffSize)
        size = demuxBuff + demuxBuffSize - demuxBuffPresent;
    *D4_QWC = size / 16;
    *D4_MADR = (unsigned int)demuxBuffPresent;
    *D4_CHCR = 0x101;
    demuxBuffFuture = demuxBuffPresent + size;
    if (demuxBuffFuture >= demuxBuff + demuxBuffSize)
        demuxBuffFuture -= demuxBuffSize;
    return 1;
}

/* the decoder idle: a blocking film demuxes for the frame's budget, else the tick may (Sony's, the
 * budget SOCOM II's; OVL1 0x30A970) */
int backgroundCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    if (BlockingPSSPlayback) {
        unsigned int end = *T0_COUNT + decodeBudget;

        while (*T0_COUNT < end)
            fillBuff(mp, 0);
    } else
        OKtoFillBuff = 1;
    return 1;
}

/* the decoder's error raised (SOCOM II's; OVL1 0x30A940) */
int errorCallback(sceMpeg *mp, sceMpegCbData *cbdata, void *data)
{
    func_00198378(data_00455E50, 1);
    return 1;
}

/* a stream demuxed from the mux buffer, refilled first when empty; 0 when nothing can be read and
 * `wait` is not set (Sony's; OVL1 0x30A860) */
int fillBuff(sceMpeg *mp, int wait)
{
    unsigned char *p;
    int n;

    if (muxBuffFullness)
        p = muxBuff + MUX_BUFF_SIZE - muxBuffFullness;
    else {
        do {
            muxBuffFullness = fileRead(muxBuff, MUX_BUFF_SIZE);
            if (!muxBuffFullness && !wait)
                return 0;
        } while (muxBuffFullness == 0);
        p = muxBuff;
    }
    while ((*D4_CHCR & 0x100) == 1)
        ;
    n = sceMpegDemuxPss(mp, p, muxBuffFullness);
    muxBuffFullness -= n;
    return n;
}
