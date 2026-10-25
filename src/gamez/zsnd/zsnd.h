/* The sound system: 989snd's EE API (989snd.c) and the game's classes over it -- CSnd (a named
 * sound out of sounds.rdr), CSndInstance (one playing voice), CSndGroup (a weighted choice of
 * sounds), CSndJukebox (the music), CSndSequence and the headset's voice queue (Headset). Layouts
 * are SOCOM 1's DWARF where it has the class, changed where SOCOM II's code says so; methods are
 * declared as the code calls them. The header's name is ours. */
#ifndef ZSND_H
#define ZSND_H

#include "../zmath/zmath.h"

/* --- 989snd's API (989snd.c): the names are SOCOM 1's symbols, the types ours --- */

typedef void (*SndCompleteProc)(unsigned int handle, unsigned long user_data);
typedef void *SoundBankPtr;

extern "C" {
void snd_AutoVol(unsigned int handle, int vol, int time, int final);
void snd_SetSoundParams_CB(unsigned int handle, unsigned int mask, int vol, int pan,
                           int pitch_mod, int bend, SndCompleteProc callback,
                           unsigned long user_data);
void snd_SetSoundPitch(unsigned int handle, int pitch);
void snd_SetSoundVolPan(unsigned int handle, int vol, int pan);
void snd_SoundIsStillPlaying_CB(unsigned int handle, SndCompleteProc callback,
                                unsigned long user_data);
void func_00340210(int a);
void func_00340240(int a);
void func_00340270(void);
void snd_ContinueSound(unsigned int handle);
void snd_PauseSound(unsigned int handle);
void func_003402F0(int a, char b);
void snd_StopSound(unsigned int handle);
void snd_PlaySoundVolPanPMPB_CB(SoundBankPtr bank, int sound, int vol, int pan, int pitch_mod,
                                int bend, SndCompleteProc callback, unsigned long user_data);
void snd_PlaySoundVolPanPMPB_A(SoundBankPtr bank, int sound, int vol, int pan, int pitch_mod,
                               int bend);
void func_00340400(int a);
void func_00340430(int a, int b, int c);
int snd_GetPlaybackMode(void);
void snd_SetPlaybackMode(int mode);
int snd_GetMasterVolume(int which);
void snd_SetMasterVolume(int which, int vol);
void snd_UnloadBank(SoundBankPtr bank);
void snd_ResolveBankXREFS(void);
void snd_StartCaching(void);
int snd_SendCache(void);
void snd_AutoReverb(int core, int depth, int delta, int channel);
void snd_StopAllStreams(void);
int snd_StreamSafeCheckCDIdle(int block);
}

#endif
