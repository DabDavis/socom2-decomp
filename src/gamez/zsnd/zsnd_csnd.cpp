/* zsnd_csnd.cpp -- CSnd, a named sound out of sounds.rdr, and the sound system's statics (OVL1
 * 0x340F70-0x3462FF). The unit name is SOCOM 1's (its __sinit_zsnd_csnd.cpp and asserts; SOCOM
 * II's has no static initialiser: its sounds-by-name map is gone). Defined in reverse of their
 * image order (-inline deferred). */

#include <string.h>

#include "zsnd.h"
#include "../zanim/zanim.h"

/* SOCOM II's headset voice queue, a global (CSndVocalQueue by the functions its calls land in;
 * the slots' names ours) */
class CSndVocalQueue {
public:
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Open();
    virtual void Close();
};
extern CSndVocalQueue data_0048DEE0;
/* called through a reference local, as the target's calls dispatch through the vtable (on the
 * object itself MW calls the function directly) */

extern _zrdr *data_0048DF18;            /* the sounds file (GetSoundRdrFile's) */
extern bool snd_system_initialized;
extern "C" int toupper(int c);

_zrdr *zrdr_read(const char *name, const char *file, int line);
extern "C" void *func_0034BB80();       /* Headset::ftsGetHeadset (not named yet) */
extern "C" void func_0034B9E0(void *headset);

_zrdr *GetSoundRdrFile()
{
    return data_0048DF18;
}

CSnd::CSnd()
    : m_18(0)
{
    if (m_isDisabled)
        return;
    m_vol = 1.0f;
    m_name = 0;
    m_range_min = 160;
    m_range_max = 430;
    m_bank = 255;
    m_ID = 0xFFFF;
    m_isStreamed = false;
    m_socom2_bit1 = false;
    m_socom2_bit8 = false;
    m_20 = 0;
    m_24 = 0;
    m_socom2_bit3 = false;
    m_34 = 0.0f;
    m_38 = 0.0f;
    m_isOneShot = false;
    m_mode = 0;
    m_soundmode = 0;
    m_socom2_bit4 = false;
    m_socom2_bit5 = false;
    m_socom2_bit6 = false;
    m_28 = 0;
    m_30 = 0;
    m_2c = 0;
    m_mode = 1;
    CSndInstance::m_pool.reserve(48);
}

CSnd::~CSnd()
{
}

void CSnd::NetOpen()
{
    CSndVocalQueue &q = data_0048DEE0;

    q.Open();
    if (!data_0048DF18)
        data_0048DF18 = zrdr_read("sounds.rdr", 0, 0);
    snd_SetMasterVolume(16, 1024);
    CFileCD::m_streamingSndActive = true;
    m_max_num_vags = 4;
    func_0033F720(m_max_num_vags, 0xB000, 0, 1);
}

void CSnd::Open()
{
    CSndVocalQueue &q = data_0048DEE0;

    q.Open();
    CFileCD::m_streamingSndActive = true;
    if (!data_0048DF18)
        data_0048DF18 = zrdr_read("sounds.rdr", 0, 0);
    m_max_num_vags = 6;
    func_0033F720(m_max_num_vags, 0xB000, 0, 1);
    if (func_0034BB80())
        func_0034B9E0(func_0034BB80());
}

/* a name copied upper-cased into a buffer of max characters (SOCOM II's; name ours) */
void CopyUpper(const char *src, char *dst, int max)
{
    int n = strlen(src);
    int i;

    for (i = 0; i < n && i < max; i++)
        dst[i] = toupper(src[i]);
    dst[i] = 0;
}

CSndInstance *CSnd::PlaySound(CSndInstance *inst, unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    return PlaySoundVolPanPitch(1.0f, -1, 0, inst, flags);
}

CSndInstance *CSnd::PlaySoundVol(float vol, CSndInstance *inst, unsigned int flags)
{
    if (m_isDisabled)
        return 0;
    return PlaySoundVolPanPitch(vol, -1, 0, inst, flags);
}

CSndInstance *CSnd::PlaySound(bool oneShot, CSndInstance *inst, unsigned int flags)
{
    bool old = m_isOneShot;

    m_isOneShot = oneShot;
    CSndInstance *r = PlaySound(inst, flags);
    m_isOneShot = old;
    return r;
}

CSndInstance *CSnd::PlaySoundVol(float vol, bool oneShot, CSndInstance *inst, unsigned int flags)
{
    bool old = m_isOneShot;

    m_isOneShot = oneShot;
    CSndInstance *r = PlaySoundVol(vol, inst, flags);
    m_isOneShot = old;
    return r;
}

CSndInstance *CSnd::PlaySound3D(const CPnt3D *pos, bool oneShot, float vol, int pitch,
                                CSndInstance *inst, unsigned int flags)
{
    bool old = m_isOneShot;

    m_isOneShot = oneShot;
    CSndInstance *r = PlaySound3D(pos, vol, pitch, inst, flags);
    m_isOneShot = old;
    return r;
}

CSndInstance *CSnd::PlaySoundVolPanPitch(float vol, int pan, int pitch, bool oneShot,
                                         CSndInstance *inst, unsigned int flags)
{
    bool old = m_isOneShot;

    m_isOneShot = oneShot;
    CSndInstance *r = PlaySoundVolPanPitch(vol, pan, pitch, inst, flags);
    m_isOneShot = old;
    return r;
}

void CSnd::PlayErrorSound()
{
    if (m_errorsound)
        m_errorsound->PlaySound(0, 255);
}

void CSnd::WaitForCDRead()
{
    if (m_max_num_vags && snd_system_initialized) {
        snd_StopAllStreams();
        while (snd_FlushSoundCommands())
            ;
        while (!(bool)snd_StreamSafeCheckCDIdle(1))
            ;
    }
}

/* SOCOM II's reverb tables, indoor and outdoor: depths and times per record, and two levels each
 * (names not known) */
extern std::vector<unsigned int> data_0048DB68, data_0048DB88, data_0048DBA8, data_0048DBC8;
extern int data_003DFFB0, data_003DFFB8, data_003DFFC0, data_003DFFC8;
extern "C" void func_003416B0(_zrdr *rdr);     /* the IndoorReverb records */
extern "C" void func_00341500(_zrdr *rdr);     /* the OutdoorReverb records */

void CSnd::LoadReverbs(_zrdr *rdr)
{
    data_0048DB88.clear();
    data_0048DB68.clear();
    data_0048DBC8.clear();
    data_0048DBA8.clear();
    data_003DFFB8 = -1;
    data_003DFFB0 = -1;
    data_003DFFC8 = -1;
    data_003DFFC0 = -1;
    func_003416B0(rdr);
    func_00341500(rdr);
}

void CSnd::SetGroupVol(int group, float vol)
{
    snd_SetMasterVolume(group, (int)(1024.0f * vol));
}
