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
 * snd_StartCaching and snd_SendCache, by position: SOCOM II's store a word) */
static int snd_caching;

void snd_StartCaching(void)
{
    snd_caching = 1;
}

int snd_SendCache(void)
{
    snd_caching = 0;
    return snd_FlushSoundCommands();
}
