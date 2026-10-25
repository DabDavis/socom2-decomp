/* audiodec.cpp -- the film's audio: the PSS stream's PCM kept in a ring and sent to the IOP buffer
 * 989snd plays it from (Sony's ezmpeg sample audiodec.c, compiled as C++ in both builds: SOCOM 1's
 * demo names the functions; SOCOM II's IOP side is 989snd's movie sound, where Sony's allocated an
 * IOP heap block and drove libsd's block transfer). OVL1 0x309F60-0x30A85F: playpss.cpp calls
 * audioDecIsPreset and audioDecReset out of line, so these are a unit of their own (the name
 * Sony's file's). Defined in reverse of their image order (-inline deferred). */

#include "zmpeg.h"

extern "C" {
void FlushCache(int mode);
int snd_GetTransStatus(void);
void snd_StartMovieSound(int buf, int size, int end, int freq, int channels);
void snd_CloseMovieSound(void);
void snd_ResetMovieSound(void);
int snd_InitMovieSound(int bytes, int a, int vol, int b, int c, int mode);

/* a SIF DMA transfer to the IOP (Sony's sceSifDmaData) */
struct sceSifDmaData {
    unsigned int data;
    unsigned int addr;
    unsigned int size;
    unsigned int mode;
};
unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);
int sceSifDmaStat(unsigned int id);

int func_00341A20(void);   /* (zsnd: a setting read, put back by audioDecDelete) */
void func_00341A00(int v);   /* (zsnd: the setting written) */
void func_00341060(void);   /* (zsnd) */
}

/* the setting audioDecCreate saves and audioDecDelete restores (SOCOM II's; the name ours) */
int savedSndSetting;   /* 0x451D30 */

#define min(a, b) ((a) < (b) ? (a) : (b))

/* the decoder made over the ring, its IOP buffer from 989snd (Sony's; OVL1 0x30A7C0) */
int audioDecCreate(AudioDec *ad, unsigned char *buff, int buffSize, int iopBuffSize)
{
    ad->state = AU_STATE_INIT;
    ad->hdrCount = 0;
    ad->data = buff;
    ad->put = 0;
    ad->count = 0;
    ad->size = buffSize;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopBuffSize = iopBuffSize;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
    savedSndSetting = func_00341A20();
    func_00341A00(0);
    func_00341060();
    ad->iopBuff = snd_InitMovieSound(iopBuffSize, 0, 0x366, 0, 2, 2);
    return ad->iopBuff >= 0;
}

/* the movie sound closed, the setting restored (Sony's; OVL1 0x30A790) */
int audioDecDelete(AudioDec *ad)
{
    func_00341A00(savedSndSetting);
    savedSndSetting = 0;
    snd_CloseMovieSound();
    return 1;
}

/* the IOP buffer started playing (Sony's; OVL1 0x30A730) */
void audioDecStart(AudioDec *ad)
{
    snd_StartMovieSound(ad->iopBuff, ad->iopBuffSize / UNIT_SIZE * UNIT_SIZE,
                        ad->iopBuff + ad->iopPausePos, 0, 0);
    ad->state = AU_STATE_PLAYING;
}

/* the decoder emptied (Sony's; OVL1 0x30A6E0) */
void audioDecReset(AudioDec *ad)
{
    snd_ResetMovieSound();
    ad->state = AU_STATE_INIT;
    ad->hdrCount = 0;
    ad->put = 0;
    ad->count = 0;
    ad->totalBytes = 0;
    ad->totalBytesSent = 0;
    ad->iopLastPos = 0;
    ad->iopPausePos = 0;
}

/* where the next PCM goes: the header first, then the ring's free areas (Sony's; OVL1 0x30A620) */
void audioDecBeginPut(AudioDec *ad, unsigned char **ptr0, int *len0, unsigned char **ptr1, int *len1)
{
    int len;

    if (ad->state == AU_STATE_INIT) {
        *ptr0 = (unsigned char *)&ad->hdr + ad->hdrCount;
        *len0 = AU_HDR_SIZE - ad->hdrCount;
        *ptr1 = ad->data;
        *len1 = ad->size;
        return;
    }
    len = ad->size - ad->count;
    if (ad->size - ad->put >= len) {
        *ptr0 = ad->data + ad->put;
        *len0 = len;
        *ptr1 = 0;
        *len1 = 0;
    } else {
        *ptr0 = ad->data + ad->put;
        *len0 = ad->size - ad->put;
        *ptr1 = ad->data;
        *len1 = len - (ad->size - ad->put);
    }
}

/* the PCM put counted, the header's first (Sony's; OVL1 0x30A590) */
void audioDecEndPut(AudioDec *ad, int size)
{
    if (ad->state == AU_STATE_INIT) {
        unsigned int hdr_add = AU_HDR_SIZE - ad->hdrCount;

        if ((unsigned int)size < hdr_add)
            hdr_add = size;

        ad->hdrCount += hdr_add;
        if (ad->hdrCount >= AU_HDR_SIZE)
            ad->state = AU_STATE_PRESET;
        size -= hdr_add;
    }
    ad->put = (ad->put + size) % ad->size;
    ad->count += size;
    ad->totalBytes += size;
}

/* the IOP buffer filled once (Sony's; OVL1 0x30A570) */
bool audioDecIsPreset(AudioDec *ad)
{
    return ad->totalBytesSent >= ad->iopBuffSize;
}

/* a block sent to the IOP and waited for (Sony's; inlined) */
static inline void sendToIOP(int dst, unsigned char *src, int size)
{
    sceSifDmaData transData;
    unsigned int did;

    if (size <= 0)
        return;
    transData.data = (unsigned int)src;
    transData.addr = (unsigned int)dst;
    transData.size = size;
    transData.mode = 0;
    FlushCache(0);
    did = sceSifSetDma(&transData, 1);
    while (sceSifDmaStat(did) >= 0)
        ;
}

/* the two source areas sent into the two destination areas, as much as fits (Sony's; OVL1
 * 0x309F60) */
int sendToIOP2area(int pd0, int d0, int pd1, int d1, unsigned char *ps0, int s0, unsigned char *ps1,
                   int s1)
{
    if (d0 + d1 < s0 + s1) {
        int diff = (s0 + s1) - (d0 + d1);

        if (diff >= s1) {
            s0 -= diff - s1;
            s1 = 0;
        } else
            s1 -= diff;
    }
    if (s0 >= d0) {
        sendToIOP(pd0, ps0, d0);
        sendToIOP(pd1, ps0 + d0, s0 - d0);
        sendToIOP(pd1 + s0 - d0, ps1, s1);
    } else if (s1 >= d0 - s0) {
        sendToIOP(pd0, ps0, s0);
        sendToIOP(pd0 + s0, ps1, d0 - s0);
        sendToIOP(pd1, ps1 + d0 - s0, s1 - (d0 - s0));
    } else {
        sendToIOP(pd0, ps0, s0);
        sendToIOP(pd0 + s0, ps1, s1);
    }
    return s0 + s1;
}

/* the IOP buffer's free areas behind the play position, in whole units (Sony's; out of line in
 * SOCOM II, OVL1 0x30A2D0) */
void iopGetArea(int *pd0, int *d0, int *pd1, int *d1, AudioDec *ad, int pos)
{
    int len = (pos + ad->iopBuffSize - ad->iopLastPos - UNIT_SIZE) % ad->iopBuffSize;

    if (pos - ad->iopLastPos >= 0 && pos - ad->iopLastPos < UNIT_SIZE) {
        *pd0 = ad->iopBuff;
        *d0 = 0;
        *pd1 = ad->iopBuff;
        *d1 = 0;
        return;
    }
    len = len / UNIT_SIZE * UNIT_SIZE;
    if (ad->iopBuffSize - ad->iopLastPos >= len) {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = len;
        *pd1 = 0;
        *d1 = 0;
    } else {
        *pd0 = ad->iopBuff + ad->iopLastPos;
        *d0 = ad->iopBuffSize - ad->iopLastPos;
        *pd1 = ad->iopBuff;
        *d1 = len - (ad->iopBuffSize - ad->iopLastPos);
    }
}

/* the ring's PCM sent to the IOP buffer: all of it before playing, behind the play position after
 * (Sony's; OVL1 0x30A3B0) */
int audioDecSendToIOP(AudioDec *ad)
{
    int pd0, pd1, d0, d1;
    unsigned char *ps0, *ps1;
    int s0, s1;
    int count_sent = 0;
    int countAdj;
    int pos;

    switch (ad->state) {
    case AU_STATE_INIT:
        return 0;
    case AU_STATE_PRESET:
        pd0 = ad->iopBuff + ad->totalBytesSent % ad->iopBuffSize;
        d0 = ad->iopBuffSize - ad->totalBytesSent;
        pd1 = 0;
        d1 = 0;
        break;
    case AU_STATE_PLAYING:
        pos = (snd_GetTransStatus() & 0x00FFFFFF) - ad->iopBuff;
        iopGetArea(&pd0, &d0, &pd1, &d1, ad, pos);
        break;
    case AU_STATE_PAUSE:
        return 0;
    }
    ps0 = ad->data + (ad->size + (ad->put - ad->count)) % ad->size;
    ps1 = ad->data;
    countAdj = ad->count / UNIT_SIZE * UNIT_SIZE;
    s0 = min(ad->data + ad->size - ps0, countAdj);
    s1 = countAdj - s0;
    if (d0 + d1 >= UNIT_SIZE && s0 + s1 >= UNIT_SIZE)
        count_sent = sendToIOP2area(pd0, d0, pd1, d1, ps0, s0, ps1, s1);
    ad->count -= count_sent;
    ad->totalBytesSent += count_sent;
    ad->iopLastPos = (ad->iopLastPos + count_sent) % ad->iopBuffSize;
    return count_sent;
}
