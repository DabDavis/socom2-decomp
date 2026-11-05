/* 989snd.c -- the EE side of 989 Studios' sound library: each call packs its arguments and
 * queues them as a command for 989SND.IRX on the IOP (OVL1 0x33F050-0x340F6F, the part from
 * 0x33F890 written here). The file name is SOCOM 1's (its "989snd.c: RPC collision!" message);
 * the API is extern "C" but the file was compiled as C++ -- the command senders carry mangled
 * names -- hence the pragma. Defined in reverse of their image order (-inline deferred). The
 * command numbers are the IOP dispatcher's; the names are SOCOM 1's where it has the function.
 * The command RPC (the world lane round 19): the two senders and the flush, SOCOM 1's with SOCOM
 * II's quiet flag, share two inlines SOCOM 1's code inlines as well -- the done test (returning an
 * int: an inlined bool's true is daddiu where the target's is li, and AndWait's `!` then converts
 * it, sltu/xori) and the fill buffer's send (its returns written through a local copy of the
 * return values' pointer, the busy word stored but not reread); the buffers' records are typed
 * (the commands' after the count, the return definitions 16 bytes: with an int view the record's
 * addresses are set up in another order). */
#pragma cplusplus on

#include "zsnd.h"

void snd_SendIOPCommandNoWait(int command, int size, char *data, SndCompleteProc callback,
                              unsigned long user_data);
int snd_SendIOPCommandAndWait(int command, int size, char *data);

extern "C" int snd_FlushSoundCommands(void);   /* 0x3408F0 (SOCOM 1's name, by position) */

extern "C" {
int printf(const char *fmt, ...);
char *strcpy(char *to, const char *from);
unsigned int strlen(const char *s);
void FlushCache(int op);
int sceSifCallRpc(void *cd, unsigned int fno, unsigned int mode, void *send, int ssize, void *recv,
                  int rsize, void (*end)(void *), void *end_param);
int sceSifCheckStatRpc(void *cd);
}

/* the bank loader's RPC (SOCOM 1's globals, its symbols): the client bound to 989snd's loader, the
 * name request (the offset, then the name), the request by disc location (sector and offset), the
 * reply (the bank, -1 while the IOP works) */
char gSyncStringBuffer[256];
int gSLClientLoaderData[10];   /* sceSifClientData */
int gLoadReturnValue[4];
int gLoadParams[8];
/* SOCOM II's (the names ours): the last load's error -- cleared by each load, 262 when its RPC call
 * failed -- and the messages switched off (snd_StartSoundSystem's flag 2) */
int gLastLoadError;
int gQuiet;
extern int gLoadBusy;
extern unsigned int gStats[16];

/* the command buffers, double-buffered (SOCOM 1's globals, its symbols): the commands queued, their
 * return definitions and the values the IOP returns, the pointers to each pair and the room left */
extern "C" {
void sceSifInitRpc(unsigned int mode);
int sceSifBindRpc(void *cd, unsigned int num, unsigned int mode);
}
int gSndCommandBuffer1[0x400];
int gSndCommandBuffer2[0x400];
/* a queued command's return: the callback and its data (16-byte records, 256 to a buffer as there
 * are commands; the name ours) */
struct SndReturnDef {
    SndCompleteProc callback;   /* 0x0 */
    unsigned long user_data;    /* 0x8 */
};
SndReturnDef gSndCommandReturnDef1[256];
SndReturnDef gSndCommandReturnDef2[256];
int gActualReturnValues1[0x110];
int gActualReturnValues2[0x110];
int *gSndCommandBuffePtr[2];
int gCommandBuffeBytesAvail[2];
SndReturnDef *gSndCommandReturnDefPtr[2];
int *gReturnValuesPtr[2];
/* the command client (SOCOM 1's gSLClientData, sceSifClientData: the server at 0x24) and the load
 * callback and its data (SOCOM 1's gLoadCB, gLoadUserData) */
int gSLClientData[10];
SndCompleteProc gLoadCB;
unsigned long gLoadUserData;

/* the flags and the status block, the streaming's (defined with it, below) */
extern int gCaching;
extern int gStreamingInited;
extern int gSSRead;
extern int gSSReadDone;
extern "C" {
int sceCdSync(int mode);
void InvalidDCache(void *start, void *end);
}
/* the stream-safe read's callback, and a file load in flight (SOCOM 1's globals, its symbols, by
 * their order beside gSSRead and gSSReadDone) and SOCOM II's own callback for that load and its data
 * (the names ours: SOCOM 1 gave the file load gLoadCB and gLoadUserData) -- only the flush reads them
 * here */
void (*gCdCallback)(int);           /* 0x488FB8 */
int gLoadingFromFS;                 /* 0x488FD0 */
SndCompleteProc gFSLoadCB;          /* 0x48DB20 */
unsigned long gFSLoadUserData;      /* 0x48DB28 */

/* the command RPC's state (SOCOM 1's globals, its symbols): the reply a call in flight writes (the
 * busy word points at it, 0 when none is out), the commands whose returns it awaits, the reply and
 * the request, and the command buffer being filled */
int *gCommBusy;                /* 0x488F98 */
int gAwaitingInts;             /* 0x488FA0 */
int gSyncBuffer[16];           /* 0x489000 */
int gSyncSendBuffer[16];       /* 0x489040 */
int gCommandFillBuffer;        /* 0x48DA40 */

/* whether the call in flight is done: none out, or the RPC no longer busy and every return the
 * IOP owes written (-1 in the reply's first word and in the last awaited one) -- the busy word then
 * cleared (SOCOM 1's, inline in both its waits and its flush; the name ours) */
static inline int snd_CommandsReturned(void)
{
    if (!gCommBusy)
        return 1;
    if (sceSifCheckStatRpc(gSLClientData))
        return 0;
    if (gCommBusy[0] == -1 && gCommBusy[gAwaitingInts + 1] == -1) {
        gCommBusy = 0;
        return 1;
    }
    if (!gQuiet)
        printf("989snd.c: Sif says RPC isn't busy, but we still don't have returns from the IOP!\n");
    return 0;
}

/* the buffer being filled sent, unless it is empty or the commands are being cached: its returns
 * awaited in its return values, the buffers swapped and the new one emptied (SOCOM 1's, inline in
 * its wait and its flush; the name ours) */
static inline void snd_SendCommandBuffer(void)
{
    int count = gSndCommandBuffePtr[gCommandFillBuffer][0];

    if (count && !gCaching) {
        int *returns = gReturnValuesPtr[gCommandFillBuffer];

        gAwaitingInts = count;
        gCommBusy = returns;
        returns[count + 1] = 0;
        returns[0] = 0;
        while (sceSifCheckStatRpc(gSLClientData)) {
            if (!gQuiet)
                printf("989snd.c: RPC collision!\n");
            FlushCache(0);
        }
        sceSifCallRpc(gSLClientData, 77, 1, gSndCommandBuffePtr[gCommandFillBuffer],
                      4096 - gCommandBuffeBytesAvail[gCommandFillBuffer],
                      gReturnValuesPtr[gCommandFillBuffer],
                      gSndCommandBuffePtr[gCommandFillBuffer][0] * 4 + 8, 0, 0);
        gCommandFillBuffer = gCommandFillBuffer == 1 ? 0 : 1;
        gSndCommandBuffePtr[gCommandFillBuffer][0] = 0;
        gCommandBuffeBytesAvail[gCommandFillBuffer] = 4092;
    }
}


/* SOCOM 1's, with SOCOM II's quiet flag (flags bit 2) and the flags passed to the IOP after the
 * status block: the buffers set up, the command and loader clients bound -- a failed bind hangs --
 * and the IOP's side started */
int snd_StartSoundSystem(int flags)
{
    int data[2];

    gSndCommandBuffePtr[0] = gSndCommandBuffer1;
    gSndCommandBuffePtr[1] = gSndCommandBuffer2;
    gReturnValuesPtr[0] = gActualReturnValues1;
    gReturnValuesPtr[1] = gActualReturnValues2;
    gSndCommandReturnDefPtr[0] = gSndCommandReturnDef1;
    gSndCommandReturnDefPtr[1] = gSndCommandReturnDef2;
    if ((unsigned long)(unsigned int)flags & 2)
        gQuiet = 1;
    else
        gQuiet = 0;
    sceSifInitRpc(0);
    do {
        if (sceSifBindRpc(gSLClientData, 0x123456, 0) < 0) {
            printf("error: sceSifBindRpc in %s, at line %d\n", "989snd.c", 177);
            for (;;)
                ;
        }
    } while (!gSLClientData[9]);
    gLoadBusy = 0;
    gLoadCB = 0;
    gLoadUserData = 0;
    gLoadReturnValue[0] = 0;
    do {
        if (sceSifBindRpc(gSLClientLoaderData, 0x123457, 0) < 0) {
            printf("error: sceSifBindRpc in %s, at line %d\n", "989snd.c", 198);
            for (;;)
                ;
        }
    } while (!gSLClientLoaderData[9]);
    /* in SOCOM II's order (SOCOM 1's interleaved the byte counts with the buffers) */
    gStats[0] = 0;
    gStats[4] = 0;
    gSndCommandBuffer1[0] = 0;
    gSndCommandBuffer2[0] = 0;
    gCommandBuffeBytesAvail[0] = 4092;
    gCommandBuffeBytesAvail[1] = 4092;
    data[0] = (int)gStats;
    data[1] = flags;
    return snd_SendIOPCommandAndWait(0, 8, (char *)data);
}

/* SOCOM 1's, with SOCOM II's quiet flag and its own file load callback: the call in flight, once
 * its returns are in, finished -- a file load's callback given the reply, or each command's callback
 * its return value -- a bank load's callback given the bank once the loader replies, the buffer
 * being filled sent while no call is out, the stream-safe CD read's end noticed and its callback
 * told; whether a call or a load is still out */
int snd_FlushSoundCommands(void)
{
    if (gCommBusy) {
        FlushCache(0);
        if (snd_CommandsReturned()) {
            if (gLoadingFromFS) {
                if (gFSLoadCB)
                    gFSLoadCB(gSyncBuffer[1], gFSLoadUserData);
                gFSLoadCB = 0;
                gLoadingFromFS = 0;
            } else {
                int b = gCommandFillBuffer == 1 ? 0 : 1;

                for (int i = 0; i < gSndCommandBuffePtr[b][0]; i++)
                    if (gSndCommandReturnDefPtr[b][i].callback)
                        gSndCommandReturnDefPtr[b][i].callback(gReturnValuesPtr[b][i + 1],
                                                               gSndCommandReturnDefPtr[b][i].user_data);
            }
        }
    }
    if (gLoadBusy) {
        FlushCache(0);
        if (gLoadReturnValue[0] != -1) {
            if (gLoadCB) {
                SndCompleteProc cb = gLoadCB;
                unsigned long data = gLoadUserData;

                gLoadCB = 0;
                gLoadUserData = 0;
                cb(gLoadReturnValue[0], data);
            }
            gLoadReturnValue[0] = 0;
            gLoadBusy = 0;
        }
    }
    if (!gCommBusy)
        snd_SendCommandBuffer();
    if (gSSRead) {
        if (!gStreamingInited)
            sceCdSync(1);
        else {
            InvalidDCache(gStats, (char *)gStats + 63);
            gSSReadDone = !gStats[0];
        }
        if (gSSReadDone) {
            gSSRead = 0;
            gSSReadDone = 0;
            if (gCdCallback)
                gCdCallback(1);
        }
    }
    return gCommBusy || gLoadBusy;
}

/* SOCOM 1's, with SOCOM II's error and quiet flag: a bank loaded by its file name and offset; the
 * IOP's bank, 0 when a load is under way or the RPC call fails */
SoundBankPtr snd_BankLoad(char *name, int offset)
{
    gLastLoadError = 0;
    if (gLoadBusy) {
        if (!gQuiet)
            printf("snd_BankLoad: Load already in progress!\n");
        return 0;
    }
    ((int *)gSyncStringBuffer)[0] = offset;
    strcpy(gSyncStringBuffer + 4, name);
    gLoadReturnValue[0] = -1;
    while (sceSifCheckStatRpc(gSLClientLoaderData)) {
        if (!gQuiet)
            printf("989snd.c: RPC collision!\n");
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    if (sceSifCallRpc(gSLClientLoaderData, 2, 1, gSyncStringBuffer, strlen(name) + 5, gLoadReturnValue, 4, 0, 0) < 0) {
        if (!gQuiet)
            printf("989snd.c: RPC call has failed inside snd_BankLoad\n");
        gLastLoadError = 262;
        return 0;
    }
    while (gLoadReturnValue[0] == -1)
        FlushCache(0);
    return (SoundBankPtr)gLoadReturnValue[0];
}

/* SOCOM 1's, as snd_BankLoad: a bank loaded by its disc sector and the offset in it, refused while
 * the stream-safe CD read is busy */
SoundBankPtr snd_BankLoadByLoc(int loc, int offset)
{
    gLastLoadError = 0;
    if (gLoadBusy) {
        if (!gQuiet)
            printf("snd_BankLoadByLoc: Load already in progress!\n");
        return 0;
    }
    if (snd_StreamSafeCdSync(1) == 1) {
        if (!gQuiet)
            printf("snd_BankLoadByLoc: CD BUSY!\n");
        return 0;
    }
    gLoadParams[0] = loc;
    gLoadParams[1] = offset;
    gLoadReturnValue[0] = -1;
    while (sceSifCheckStatRpc(gSLClientLoaderData)) {
        if (!gQuiet)
            printf("989snd.c: RPC collision!\n");
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    if (sceSifCallRpc(gSLClientLoaderData, 3, 1, gLoadParams, 8, gLoadReturnValue, 4, 0, 0) < 0) {
        if (!gQuiet)
            printf("989snd.c: RPC call has failed inside snd_BankLoadByLoc\n");
        gLastLoadError = 262;
        return 0;
    }
    while (gLoadReturnValue[0] == -1)
        FlushCache(0);
    return (SoundBankPtr)gLoadReturnValue[0];
}

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

/* a queued command: its number, its data's size and the data (SOCOM 1's records, after the
 * buffer's count; the name ours) */
struct SndCommand {
    short command;   /* 0x0 */
    short size;      /* 0x2 */
    char data[4];    /* 0x4 */
};


/* SOCOM 1's, with SOCOM II's quiet flag: a command sent at once, its data copied into the request,
 * once the call before it is done -- the reply awaited, the buffer being filled sent after it, the
 * reply's second word returned */
int snd_SendIOPCommandAndWait(int command, int size, char *data)
{
    for (int i = 0; i < size; i++)
        ((char *)gSyncSendBuffer)[i] = data[i];
    while (gCommBusy) {
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    gSyncBuffer[2] = 0;
    gSyncBuffer[0] = 0;
    gAwaitingInts = 1;
    gCommBusy = gSyncBuffer;
    while (sceSifCheckStatRpc(gSLClientData)) {
        if (!gQuiet)
            printf("989snd.c: RPC collision!\n");
        snd_FlushSoundCommands();
        FlushCache(0);
    }
    if (size)
        sceSifCallRpc(gSLClientData, command, 1, gSyncSendBuffer, size, gSyncBuffer, 12, 0, 0);
    else
        sceSifCallRpc(gSLClientData, command, 1, 0, 0, gSyncBuffer, 12, 0, 0);
    do
        FlushCache(0);
    while (!snd_CommandsReturned());
    int result = gSyncBuffer[1];

    snd_SendCommandBuffer();
    return result;
}

/* SOCOM 1's, with SOCOM II's quiet flag: a command with no data and no callback, while nothing is
 * cached or in flight, sent at once; any other queued in the buffer being filled -- flushing until
 * the buffer has room for it -- its return definition recorded, and the buffers flushed */
void snd_SendIOPCommandNoWait(int command, int size, char *data, SndCompleteProc callback,
                              unsigned long user_data)
{
    int recache = 0;

    if (!gCaching && !gCommBusy && !size && !callback) {
        gSyncBuffer[2] = 0;
        gAwaitingInts = 1;
        gCommBusy = gSyncBuffer;
        gSyncBuffer[0] = 0;
        while (sceSifCheckStatRpc(gSLClientData)) {
            if (!gQuiet)
                printf("989snd.c: RPC collision!\n");
            snd_FlushSoundCommands();
            FlushCache(0);
        }
        sceSifCallRpc(gSLClientData, command, 1, 0, 0, gSyncBuffer, 12, 0, 0);
    } else {
        int stalls = 0;
        int bytes = size + 4;

        if (bytes % 4)
            bytes += 4 - bytes % 4;
        while (gSndCommandBuffePtr[gCommandFillBuffer][0] == 256 ||
               gCommandBuffeBytesAvail[gCommandFillBuffer] < bytes) {
            if (gCaching) {
                recache = 1;
                gCaching = 0;
            }
            snd_FlushSoundCommands();
            if (stalls == 1 && !gQuiet)
                printf("snd_SendIOPCommandNoWait: BUFFER %d FULL(%d)! Sound stalled waiting for IOP comands to finish!\n",
                       gCommandFillBuffer, gSndCommandBuffePtr[gCommandFillBuffer][0]);
            stalls++;
        }
        if (stalls && !gQuiet)
            printf("snd_SendIOPCommandNoWait: continuing (%d).\n", stalls);
        if (recache)
            gCaching = 1;
        char *rec = (char *)gSndCommandBuffePtr[gCommandFillBuffer] +
                    (4092 - gCommandBuffeBytesAvail[gCommandFillBuffer]);
        SndCommand *cmd = (SndCommand *)(rec + 4);

        cmd->command = command;
        cmd->size = size;
        for (int i = 0; i < size; i++)
            cmd->data[i] = data[i];
        gCommandBuffeBytesAvail[gCommandFillBuffer] -= bytes;
        gSndCommandReturnDefPtr[gCommandFillBuffer][gSndCommandBuffePtr[gCommandFillBuffer][0]].callback = callback;
        gSndCommandReturnDefPtr[gCommandFillBuffer][gSndCommandBuffePtr[gCommandFillBuffer][0]].user_data = user_data;
        gSndCommandBuffePtr[gCommandFillBuffer][0]++;
        snd_FlushSoundCommands();
    }
}

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
