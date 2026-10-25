/* 989snd.c -- the EE side of 989 Studios' sound library: each call packs its arguments and
 * queues them as a command for 989SND.IRX on the IOP (OVL1 0x33F050-0x340F6F, the part from
 * 0x33F890 written here). The file name is SOCOM 1's (its "989snd.c: RPC collision!" message);
 * the API is extern "C" but the file was compiled as C++ -- the command senders carry mangled
 * names -- hence the pragma. Defined in reverse of their image order (-inline deferred). The
 * command numbers are the IOP dispatcher's; the names are SOCOM 1's where it has the function. */
#pragma cplusplus on

#include "zsnd.h"

void snd_SendIOPCommandNoWait(int command, int size, char *data, SndCompleteProc callback,
                              unsigned long user_data);
int snd_SendIOPCommandAndWait(int command, int size, char *data);

extern "C" int snd_FlushSoundCommands(void);   /* 0x3408F0 (SOCOM 1's name, by position) */

void snd_ResolveBankXREFS(void)
{
    snd_SendIOPCommandNoWait(8, 0, 0, 0, 0);
}

void snd_UnloadBank(SoundBankPtr bank)
{
    int data[1];

    data[0] = (int)bank;
    snd_SendIOPCommandNoWait(6, 4, (char *)data, 0, 0);
}

void snd_SetMasterVolume(int which, int vol)
{
    int data[2];

    data[0] = which;
    data[1] = vol;
    snd_SendIOPCommandNoWait(9, 8, (char *)data, 0, 0);
}

int snd_GetMasterVolume(int which)
{
    int data[1];

    data[0] = which;
    return snd_SendIOPCommandAndWait(10, 4, (char *)data);
}

void snd_SetPlaybackMode(int mode)
{
    int data[1];

    data[0] = mode;
    snd_SendIOPCommandNoWait(11, 4, (char *)data, 0, 0);
}

int snd_GetPlaybackMode(void)
{
    return snd_SendIOPCommandAndWait(12, 0, 0);
}

/* SOCOM II's: command 78, three words (name unknown) */
void func_00340430(int a, int b, int c)
{
    int data[3];

    data[0] = a;
    data[1] = b;
    data[2] = c;
    snd_SendIOPCommandNoWait(78, 12, (char *)data, 0, 0);
}

/* SOCOM II's: command 100, one word (name unknown) */
void func_00340400(int a)
{
    int data[1];

    data[0] = a;
    snd_SendIOPCommandNoWait(100, 4, (char *)data, 0, 0);
}

void snd_PlaySoundVolPanPMPB_A(SoundBankPtr bank, int sound, int vol, int pan, int pitch_mod,
                               int bend)
{
    int data[6];

    data[0] = (int)bank;
    data[1] = sound;
    data[2] = vol;
    data[3] = pan;
    data[4] = pitch_mod;
    data[5] = bend;
    snd_SendIOPCommandNoWait(18, 24, (char *)data, 0, 0);
}

void snd_PlaySoundVolPanPMPB_CB(SoundBankPtr bank, int sound, int vol, int pan, int pitch_mod,
                                int bend, SndCompleteProc callback, unsigned long user_data)
{
    int data[6];

    data[0] = (int)bank;
    data[1] = sound;
    data[2] = vol;
    data[3] = pan;
    data[4] = pitch_mod;
    data[5] = bend;
    snd_SendIOPCommandNoWait(17, 24, (char *)data, callback, user_data);
}

void snd_StopSound(unsigned int handle)
{
    int data[1];

    data[0] = handle;
    snd_SendIOPCommandNoWait(21, 4, (char *)data, 0, 0);
}

/* SOCOM II's: command 103, a word and a byte (name unknown) */
void func_003402F0(int a, char b)
{
    int data[2];

    data[0] = a;
    data[1] = b;
    snd_SendIOPCommandNoWait(103, 8, (char *)data, 0, 0);
}

void snd_PauseSound(unsigned int handle)
{
    int data[1];

    data[0] = handle;
    snd_SendIOPCommandNoWait(19, 4, (char *)data, 0, 0);
}

void snd_ContinueSound(unsigned int handle)
{
    int data[1];

    data[0] = handle;
    snd_SendIOPCommandNoWait(20, 4, (char *)data, 0, 0);
}

/* SOCOM II's: command 24, no arguments (name unknown) */
void func_00340270(void)
{
    snd_SendIOPCommandNoWait(24, 0, 0, 0, 0);
}

/* SOCOM II's: command 22, one word (name unknown) */
void func_00340240(int a)
{
    int data[1];

    data[0] = a;
    snd_SendIOPCommandNoWait(22, 4, (char *)data, 0, 0);
}

/* SOCOM II's: command 23, one word (name unknown) */
void func_00340210(int a)
{
    int data[1];

    data[0] = a;
    snd_SendIOPCommandNoWait(23, 4, (char *)data, 0, 0);
}

void snd_SoundIsStillPlaying_CB(unsigned int handle, SndCompleteProc callback,
                                unsigned long user_data)
{
    int data[1];

    data[0] = handle;
    snd_SendIOPCommandNoWait(25, 4, (char *)data, callback, user_data);
}

void snd_SetSoundVolPan(unsigned int handle, int vol, int pan)
{
    int data[3];

    data[0] = handle;
    data[1] = vol;
    data[2] = pan;
    snd_SendIOPCommandNoWait(27, 12, (char *)data, 0, 0);
}

void snd_SetSoundPitch(unsigned int handle, int pitch)
{
    int data[2];

    data[0] = handle;
    data[1] = pitch;
    snd_SendIOPCommandNoWait(30, 8, (char *)data, 0, 0);
}

void snd_SetSoundParams_CB(unsigned int handle, unsigned int mask, int vol, int pan,
                           int pitch_mod, int bend, SndCompleteProc callback,
                           unsigned long user_data)
{
    int data[6];

    data[0] = handle;
    data[1] = mask;
    data[2] = vol;
    data[3] = pan;
    data[4] = pitch_mod;
    data[5] = bend;
    snd_SendIOPCommandNoWait(33, 24, (char *)data, callback, user_data);
}

void snd_AutoVol(unsigned int handle, int vol, int time, int final)
{
    int data[4];

    data[0] = handle;
    data[1] = vol;
    data[2] = time;
    data[3] = final;
    snd_SendIOPCommandNoWait(34, 16, (char *)data, 0, 0);
}

/* the flag the command senders test: set, a sender flushes before queueing (SOCOM 1's
 * snd_StartCaching and snd_SendCache, by position: SOCOM II's store a word; the flag is SOCOM 1's
 * global gCaching) */
int gCaching;

void snd_StartCaching(void)
{
    gCaching = 1;
}

int snd_SendCache(void)
{
    gCaching = 0;
    return snd_FlushSoundCommands();
}

/* --- streaming: while the IOP streams it owns the CD, so the game's reads go through it --- */

extern "C" {
int sceCdSync(int mode);
int sceCdRead(unsigned int lbn, unsigned int sectors, void *buf, void *mode);
int sceCdGetError(void);
void InvalidDCache(void *start, void *end);
void SyncDCache(void *start, void *end);
}

/* SOCOM 1's globals (its symbols); the status block is the one the IOP writes (64 bytes in SOCOM
 * II, 32 in SOCOM 1): word 0 busy while a stream-safe read runs, word 4 that read's error */
int gStreamingInited;
int gSSRead;
int gSSReadDone;
int gLoadBusy;
unsigned int gStats[16];

int snd_InitVAGStreamingEx(int num_channels, int buffer_size, int a, int b)
{
    int data[4];

    if (gStreamingInited == 1)
        return 0;
    if (gLoadBusy)
        while (snd_FlushSoundCommands())
            ;
    snd_StreamSafeCdSync(0);
    data[0] = num_channels;
    data[1] = buffer_size;
    data[2] = a;
    data[3] = b;
    gStreamingInited = snd_SendIOPCommandAndWait(42, 16, (char *)data);
    return gStreamingInited;
}

void snd_StopAllStreams(void)
{
    snd_SendIOPCommandNoWait(52, 0, 0, 0, 0);
}

void snd_CloseVAGStreaming(void)
{
    if (gStreamingInited) {
        if (gLoadBusy)
            while (snd_FlushSoundCommands())
                ;
        snd_StreamSafeCdSync(0);
        snd_SendIOPCommandAndWait(53, 0, 0);
        gStreamingInited = 0;
    }
}

/* SOCOM II adds a word (SOCOM 1's took eleven arguments) */
void snd_PlayVAGStreamByLoc_CB(int loc1, int loc2, int offset1, int offset2, int vol, int pan,
                               int group, int parent, int priority, int flags,
                               SndCompleteProc callback, unsigned long user_data)
{
    int data[8];

    data[0] = loc1;
    data[1] = loc2;
    data[2] = (vol << 16) | (offset1 & 0xFFFF);
    data[3] = (pan << 16) | (offset2 & 0xFFFF);
    data[4] = group;
    data[5] = parent;
    data[6] = priority;
    data[7] = flags;
    snd_SendIOPCommandNoWait(44, 32, (char *)data, callback, user_data);
}

int snd_StreamSafeCheckCDIdle(int block)
{
    int data[1];

    data[0] = block;
    return snd_SendIOPCommandAndWait(54, 4, (char *)data);
}

int snd_StreamSafeCdRead(unsigned int lbn, unsigned int sectors, void *buf, void *mode)
{
    int data[3];

    if (!gStreamingInited)
        return sceCdRead(lbn, sectors, buf, mode);
    if (snd_StreamSafeCdSync(1) == 1)
        return 0;
    gStats[0] = 1;
    gStats[4] = 0;
    SyncDCache(gStats, (char *)gStats + 63);
    gSSRead = 1;
    data[0] = lbn;
    data[1] = sectors;
    data[2] = (int)buf;
    gSSReadDone = 0;
    snd_SendIOPCommandNoWait(56, 12, (char *)data, 0, 0);
    return 1;
}

/* SOCOM 1's (0x33F320 by position, between the error query and the read in both builds); SOCOM
 * II invalidates the status block where SOCOM 1 flushed the whole cache */
int snd_StreamSafeCdSync(int mode)
{
    if (!gStreamingInited)
        return sceCdSync(mode);
    InvalidDCache(gStats, (char *)gStats + 63);
    gSSReadDone = gStats[0] ? 0 : 1;
    if (gSSReadDone == 1)
        return 0;
    if (mode == 1)
        return 1;
    while (gSSReadDone == 0) {
        snd_FlushSoundCommands();
        InvalidDCache(gStats, (char *)gStats + 63);
        gSSReadDone = gStats[0] ? 0 : 1;
    }
    return 0;
}

int snd_StreamSafeCdGetError(void)
{
    if (gStreamingInited)
        return gStats[4];
    return sceCdGetError();
}

void snd_SetReverbType(int core, int type)
{
    int data[2];

    data[0] = core;
    data[1] = type;
    snd_SendIOPCommandNoWait(14, 8, (char *)data, 0, 0);
}

/* SOCOM II's: command 80, five words (989SND.IRX's reverb setter: core, type, depth, delay,
 * feedback -- socom-unzipped's reading; name unknown) */
void func_0033F260(int core, int type, int depth, int delay, int feedback)
{
    int data[5];

    data[0] = core;
    data[1] = type;
    data[2] = depth;
    data[3] = delay;
    data[4] = feedback;
    snd_SendIOPCommandNoWait(80, 20, (char *)data, 0, 0);
}

void snd_AutoReverb(int core, int depth, int delta, int channel)
{
    int data[4];

    data[0] = core;
    data[1] = depth;
    data[2] = delta;
    data[3] = channel;
    snd_SendIOPCommandNoWait(16, 16, (char *)data, 0, 0);
}

/* SOCOM II passes six words (SOCOM 1 four) */
int snd_InitMovieSound(int bytes, int a, int vol, int b, int c, int mode)
{
    int data[6];

    data[0] = bytes;
    data[1] = a;
    data[2] = vol;
    data[3] = b;
    data[4] = c;
    data[5] = mode;
    return snd_SendIOPCommandAndWait(59, 24, (char *)data);
}

void snd_ResetMovieSound(void)
{
    snd_SendIOPCommandAndWait(61, 0, 0);
}

void snd_CloseMovieSound(void)
{
    snd_SendIOPCommandAndWait(60, 0, 0);
}

/* SOCOM II passes five words (SOCOM 1 three) */
void snd_StartMovieSound(void *buf, int size, int end, int freq, int channels)
{
    int data[5];

    data[0] = (int)buf;
    data[1] = size;
    data[2] = end;
    data[3] = freq;
    data[4] = channels;
    snd_SendIOPCommandAndWait(62, 20, (char *)data);
}

int snd_GetTransStatus(void)
{
    return snd_SendIOPCommandAndWait(64, 0, 0);
}

/* the extern calls: a function of another IRX that registered with 989SND (the headset's stream
 * module, id 0x12C4E67A) */
void snd_DoExternCall(unsigned int id, int func, int a, int b, int c, int d, int e)
{
    int data[7];

    data[0] = id;
    data[1] = func;
    data[2] = a;
    data[3] = b;
    data[4] = c;
    data[5] = d;
    data[6] = e;
    snd_SendIOPCommandAndWait(76, 28, (char *)data);
}

void snd_DoExternCall_A(unsigned int id, int func, int a, int b, int c, int d, int e)
{
    int data[7];

    data[0] = id;
    data[1] = func;
    data[2] = a;
    data[3] = b;
    data[4] = c;
    data[5] = d;
    data[6] = e;
    snd_SendIOPCommandNoWait(76, 28, (char *)data, 0, 0);
}

void snd_DoExternCall_CB(unsigned int id, int func, int a, int b, int c, int d, int e,
                         SndCompleteProc callback, unsigned long user_data)
{
    int data[7];

    data[0] = id;
    data[1] = func;
    data[2] = a;
    data[3] = b;
    data[4] = c;
    data[5] = d;
    data[6] = e;
    snd_SendIOPCommandNoWait(76, 28, (char *)data, callback, user_data);
}

/* SOCOM II's, before the API (name unknown): a scale by 1524/741, which the instances' tick
 * applies */
int func_0033F050(int x)
{
    return x * 1524 / 741;
}
