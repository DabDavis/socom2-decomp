/* The sound system: 989snd's EE API (989snd.c) and the game's classes over it -- CSnd (a named
 * sound out of sounds.rdr), CSndInstance (one playing voice), CSndGroup (a weighted choice of
 * sounds), CSndJukebox (the music), CSndSequence and the headset's voice queue (Headset). Layouts
 * are SOCOM 1's DWARF where it has the class, changed where SOCOM II's code says so; methods are
 * declared as the code calls them. The header's name is ours. */
#ifndef ZSND_H
#define ZSND_H

#include <list>
#include <vector>

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
int snd_FlushSoundCommands(void);
void snd_AutoReverb(int core, int depth, int delta, int channel);
/* SOCOM 1's snd_InitVAGStreamingEx (Open passes it SOCOM 1's arguments), outside this lane's rows */
int func_0033F720(int num_channels, int buffer_size, int a, int b);
void snd_StopAllStreams(void);
int snd_StreamSafeCheckCDIdle(int block);
}

/* --- the game's classes --- */

class CSndInstance;
class CValve;
struct _zrdr;

/* SOCOM 1's DWARF (bytes under -enum min) */
enum SOUND_MODE {
    SND_MODE_UNKNOWN = 0,
    SND_MODE_EFX = 1,
    SND_MODE_MUSIC = 2,
    SND_MODE_DIALOG = 3,
    SND_MODE_BANK_DEFINED = 4
};

enum SOUND_TYPE {
    SND_NONE = 0,
    SND_CONVERSATIONAL = 1,
    SND_SITREP = 2,
    SND_DEATH_DAMAGE = 3,
    SND_NUM_TYPES = 4
};

/* A sound: SOCOM 1's 0x44 bytes shrunk to 0x3C -- its range two shorts, bank and id narrowed, its
 * flag bytes packed into the bitfields at 0x1C. The first nine virtuals are SOCOM 1's, in its
 * vtable's order (the bool overloads play once with m_isOneShot set to the argument); SOCOM II
 * adds three that CSndGroup overrides (a group's members, their count, and a mode set through
 * every member). */
class CSnd {
public:
    CSnd();
    virtual ~CSnd();
    virtual CSndInstance *PlaySound(CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVol(float vol, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound3D(const CPnt3D *pos, float vol, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVolPanPitch(float vol, int pan, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound(bool oneShot, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVol(float vol, bool oneShot, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound3D(const CPnt3D *pos, bool oneShot, float vol, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVolPanPitch(float vol, int pan, int pitch, bool oneShot, CSndInstance *inst, unsigned int flags);
    /* SOCOM II's (names ours): a single sound has no members */
    virtual CSnd *GetSound(unsigned int i) { return 0; }
    virtual unsigned int GetNumSounds() { return 0; }
    virtual void SetDialogMode() { m_mode = 3; }

    static CSnd *GetSoundByName(const char *name);
    static void SetGroupVol(int group, float vol);
    static void LoadReverbs(_zrdr *rdr);
    static void Close();
    static void Open();
    static void UIOpen();
    static void NetOpen();
    static void WaitForCDRead();
    static void PlayErrorSound();
    static void OnListenerDeath();
    static bool GetSubtitles();
    static void SetSubtitles(bool on);

    static bool m_isDisabled;           /* 0x48DD28 */
    static CSnd *m_errorsound;          /* 0x48DD90 */
    static int m_max_num_vags;          /* 0x48DD98 */
    static int m_numStreams;            /* 0x48DD80: streamed voices playing (name ours) */
    static int m_numActive;             /* 0x48DD88: voices acquired (name ours) */

    float m_vol;                        /* 0x04 */
    char *m_name;                       /* 0x08 */
    unsigned short m_range_min;         /* 0x0C: SOCOM 1's PNT2D m_range, now two shorts */
    unsigned short m_range_max;         /* 0x0E */
    unsigned char m_bank;               /* 0x10: 255 for none (SOCOM 1's int m_bank) */
    unsigned short m_ID;                /* 0x12: 0xFFFF for none (SOCOM 1's int m_ID) */
    int m_14;                           /* 0x14 */
    int m_18;                           /* 0x18 */
    unsigned int m_isStreamed : 1;      /* 0x1C bit 0 (SOCOM 1's byte at 0x1C) */
    unsigned int m_socom2_bit1 : 1;     /* bit 1 */
    unsigned int m_isOneShot : 1;       /* bit 2 (SOCOM 1's byte at 0x1D: the bool overloads') */
    unsigned int m_socom2_bit3 : 1;     /* bit 3: CZAnim::StopSounds stops such a sound's stream by
                                           hand (SOCOM 1's m_isHeadsetStream?) */
    unsigned int m_socom2_bit4 : 1;     /* bits 4, 5: the SOUND tick plays a sound with either */
    unsigned int m_socom2_bit5 : 1;     /*   through a streamer of its own (names unknown) */
    unsigned int m_socom2_bit6 : 1;     /* bit 6 */
    unsigned int m_socom2_bit7 : 1;     /* bit 7 */
    unsigned int m_socom2_bit8 : 1;     /* 0x1D bit 0 */
    unsigned int m_socom2_bit9 : 1;     /* bit 1 */
    unsigned int m_soundmode : 3;       /* bits 2-4 (SOCOM 1's SOUND_MODE byte?) */
    unsigned int m_mode : 3;            /* bits 5-7: 1 when built, 3 by SetDialogMode */
    char *m_20;                         /* 0x20 */
    char *m_24;                         /* 0x24 */
    int m_28;                           /* 0x28 */
    int m_2c;                           /* 0x2C */
    int m_30;                           /* 0x30 */
    float m_34;                         /* 0x34 */
    float m_38;                         /* 0x38 */
};   /* 0x3C bytes */

/* A weighted choice of sounds (sounds.rdr's groups; SOCOM 1's class, 0x60 bytes there): each play
 * picks a member by weight, then lowers the winner's weight by m_decay and renormalises. */
class CSndGroup : public CSnd {
public:
    virtual ~CSndGroup();
    virtual CSndInstance *PlaySound(CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVol(float vol, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySound3D(const CPnt3D *pos, float vol, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSndInstance *PlaySoundVolPanPitch(float vol, int pan, int pitch, CSndInstance *inst, unsigned int flags);
    virtual CSnd *GetSound(unsigned int i)
    {
        if (i > m_sounds.size())
            return 0;
        return m_sounds[i];
    }
    virtual unsigned int GetNumSounds() { return m_sounds.size(); }
    virtual void SetDialogMode()
    {
        for (std::vector<CSnd *>::iterator i = m_sounds.begin(); i != m_sounds.end(); ++i)
            (*i)->SetDialogMode();
    }
    void NormalizeWeights();
    /* the member to play (ours: the four plays inline it) */
    CSnd *PickSound()
    {
        if (m_isDisabled)
            return 0;
        float r = (float)rand() * 4.656613e-10f;
        float sum = 0.0f;

        for (int i = 0; i < m_sounds.size(); i++) {
            sum += m_weights[i];
            if (sum > r) {
                m_weights[i] *= m_decay;
                NormalizeWeights();
                return m_sounds[i];
            }
        }
        return 0;
    }

    std::vector<CSnd *> m_sounds;       /* 0x3C */
    std::vector<float> m_weights;       /* 0x48 */
    float m_decay;                      /* 0x54 */
};   /* 0x58 bytes */

/* the CD file system's flag that a sound stream owns the drive (SOCOM 1's static) */
class CFileCD {
public:
    static bool m_streamingSndActive;
};

/* the sounds file, read once (SOCOM 1's GetSoundRdrFile) */
_zrdr *GetSoundRdrFile();

/* the instances' pool (SOCOM 1's class, a vector of pointers) */
class CSndInstanceVec : public std::vector<CSndInstance *> {
};

/* One playing voice: SOCOM 1's 0x28 bytes, a word added at 0xC and the flag bytes packed. */
class CSndInstance {
public:
    CSndInstance()
    {
        if (!CSnd::m_isDisabled) {
            m_handle = 0;
            m_parent = 0;
            m_is3D = false;
            m_forcedLoop = false;
            m_terminateOnFinish = false;
            m_terminateASAP = false;
            m_pauseASAP = false;
            m_continueASAP = false;
            m_socom2_bit6 = false;
            m_c = 0;
        }
    }
    CSndInstance(unsigned int handle, CSnd *parent)
    {
        if (CSnd::m_isDisabled)
            return;
        InitInstance(handle, parent);
    }

    /* SOCOM II's (names ours): a voice whose play command is still queued is stopped later */
    void TerminateASAP()
    {
        m_terminateASAP = true;
        m_terminateOnFinish = true;
    }
    bool StopIfPending()
    {
        if (m_handle == -1) {
            m_terminateASAP = true;
            m_terminateOnFinish = true;
            return true;
        }
        return false;
    }
    /* the 989snd volume of a level (name ours): FadeOut's call with 0 keeps its products */
    int CalcVol(float vol) { return (int)(m_vol * (1024.0f * vol * m_mastervol)); }
    static void PlaySoundCallback(unsigned int handle, unsigned long user_data);
    static void InitInstancePool(int n);
    static void RemoveFromList();
    static void RemoveFromList(CSndInstance *instance);
    static void DebugSoundList();
    static void Init();
    static void Cleanup();
    void TerminateOnFinish();
    void Set3D(float vol);
    bool IsPlaying();
    bool IsFading();
    void FadeOut(float time);
    void SetPos(const CPnt3D *pos);
    void SetPitch(int pitch);
    void SetPan(int pan);
    void SetVol(float vol);
    void Stop();
    void InitInstance(unsigned int handle, CSnd *parent);
    static void ReleaseSoundInstance(CSndInstance *instance);
    static CSndInstance *AcquireSoundInstance(unsigned int handle, CSnd *parent);

    static float m_mastervol;                       /* 0x3E0070 */
    static std::list<CSndInstance *> m_list;        /* 0x48DF98: the voices playing */
    static CSndInstanceVec m_pool;                  /* 0x48DFB8: the free ones */
    static std::vector<CSndInstance *> m_vec;       /* 0x48DFD8 (name ours) */
    static int m_numAllocated;                      /* 0x48DFE8 (name ours) */
    static int m_numInits;                          /* 0x48DFF0 (name ours) */
    static bool m_paused;                           /* 0x48DF80 (name ours) */

    unsigned int m_handle;                  /* 0x00: -1 while the play command is pending */
    CSnd *m_parent;                         /* 0x04 */
    float m_vol;                            /* 0x08 */
    int m_c;                                /* 0x0C: SOCOM II's */
    CPnt3D m_pos;                           /* 0x10 */
    unsigned int m_is3D : 1;                /* 0x1C bit 0 */
    unsigned int m_forcedLoop : 1;          /* bit 1 */
    unsigned int m_terminateOnFinish : 1;   /* bit 2 */
    unsigned int m_terminateASAP : 1;       /* bit 3 */
    unsigned int m_pauseASAP : 1;           /* bit 4 */
    unsigned int m_continueASAP : 1;        /* bit 5 */
    unsigned int m_socom2_bit6 : 1;         /* bit 6: playing whatever the handle says */
    float m_fadeTime;                       /* 0x20 */
    unsigned int m_entityID;                /* 0x24 */
};   /* 0x28 bytes */

/* one step of a sequence: a sound played to its end, or a pause (SOCOM 1's class) */
class CSndSequenceEntry {
public:
    CSndSequenceEntry(CSnd *sound, float pause);
    bool IsFading();
    void FadeOut();
    void Stop();
    void Play();
    bool Tick(float dT);

    CSnd *m_sound;              /* 0x0: none for a pause */
    CSndInstance *m_instance;   /* 0x4 */
    float m_pause;              /* 0x8 */
    float m_timer;              /* 0xC */
};   /* 0x10 bytes */

/* sounds played one after another (sndjukeboxm.rdr's SEQ_NAME records; SOCOM 1's class) */
class CSndSequence {
public:
    CSndSequence();
    ~CSndSequence();
    void FadeOut();
    bool IsFading();
    void Stop();
    void Play();
    bool Tick(float dT);
    void CreateSequence(_zrdr *rdr);

    std::vector<CSndSequenceEntry> m_entries;   /* 0x00 */
    int m_current;                              /* 0x0C: -1 before the first */
    char *m_name;                               /* 0x10 */
    bool m_done;                                /* 0x14 */
    bool m_fading;                              /* 0x15 */
};   /* 0x18 bytes */

enum MUSIC_MODE {};   // its values are not in SOCOM 1's DWARF; the SOUND tick passes 1 and 3

class CSndJukebox {
public:
    static void Play(MUSIC_MODE mode, CSnd *sound);
    static void FadeOut();
};

#endif
