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

/* SOCOM II's reverb tables, indoor and outdoor: depths and times per record, and two levels each
 * (names not known) */
extern std::vector<unsigned int> data_0048DB68, data_0048DB88, data_0048DBA8, data_0048DBC8;
extern int data_003DFFB0, data_003DFFB8, data_003DFFC0, data_003DFFC8;
static bool snd_system_initialized;     /* SOCOM 1's static (LOCAL in its symbols) */
int zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
extern "C" int toupper(int c);

_zrdr *zrdr_read(const char *name, const char *file, int line);
extern "C" void *func_0034BB80();       /* Headset::ftsGetHeadset, by its placeholder (as ZAnim calls it) */
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

/* the IOP manager's reboot callbacks (SOCOM 1's interface: the phase a byte enum) */
class CIOPManager {
public:
    enum RebootPhase { PHASE_STOP = 0, PHASE_START = 1 };
    enum RebootMode { MODE_0 = 0 };
    int RegisterRebootCallback(bool (*cb)(RebootPhase, RebootMode, void *), void *data);
};

extern int data_0048DE40;               /* the reverb's current depth (name not known) */

/* SOCOM 1's IOPRebootHandler: the sound system stopped before the IOP reboots and started after;
 * SOCOM II turns the reverb off first and sets the voice ranges and the reverb up again after */
static bool IOPRebootHandler(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    if (phase == CIOPManager::PHASE_STOP) {
        func_0033F260(2, 0, 0, 0, 0);
        snd_StopSoundSystem();
    } else {
        snd_StartSoundSystem(0);
        func_00340400(1);
        func_00340430(1, 24, 47);
        func_00340430(4, 24, 47);
        func_00340430(14, 24, 47);
        func_00340430(6, 24, 47);
        func_00340430(2, 24, 47);
        snd_SetReverbType(2, 3);
        if (CSnd::m_hasreverb) {
            snd_AutoReverb(2, 0, 240, 3);
            data_0048DE40 = 0;
            CSnd::m_hasreverb = 0;
            data_003DFFB0 = -1;
            data_003DFFC0 = -1;
        }
    }
    return true;
}

extern CIOPManager theIOPManager;
extern std::vector<unsigned int> data_0048DBE8;  /* a vector of a class (its reserve, 0x345DC0, closes
                                                   the unit); reserved by its placeholder */
extern "C" void func_00345DC0(void *v, unsigned int n);
static bool bnkArchiveIsOpen;           /* SOCOM 1's static */
extern char data_003E0520[];            /* CFileIO's root path ("host0:.\\") */
extern char data_0048DC30[];
extern unsigned char data_0048DE48;     /* the subtitles' string table wanted (name not known) */
extern "C" int *func_0039DEF0(const char *name);    /* CFileCD::Find (SOCOM 1's), by its placeholder */
extern "C" void func_00258920(void *zar, const char *name, int a, unsigned int b, unsigned int c,
                              int d);   /* zar::CZAR::Open, a fifth argument added */
extern "C" void func_00258E60(void *zar);           /* zar::CZAR::CloseKeepDir */
extern "C" void func_0034D180(void *p);
extern "C" void func_0034C4A0();

class CSTable {
public:
    void LoadTable(void *data, unsigned int size, bool copy);
};
extern CSTable data_0048DE60;           /* the subtitles' strings */

/* the file the subtitles' table is read from (a file object of the IO layer; names ours) */
class CSndStrFile {
public:
    virtual void Open(const char *name);
    virtual void Slot0C();
    virtual int Read();

    int m_4, m_8, m_c, m_10;
    void *m_data;               /* 0x14 */
    int m_size;                 /* 0x18 */
};
extern CSndStrFile data_0048DE90;

/* OPEN, 2 instructions: one register -- the target materialises the initialised flag's 1 in a2
 * (sb a2), MW here in v1. Tried: the flag static (SOCOM 1's) or extern, bool or unsigned char,
 * `= 1`; the store after the subtitles' block or after Headset::Create (worse: MW keeps it put);
 * the subtitles' flag bool; the file object through a pointer, a reference declared earlier.
 * m_size is an int (LoadTable takes unsigned: the converted argument loads first, as the target's) */
void CSnd::Init()
{
    if (m_isDisabled)
        return;
    data_0048DBC8.reserve(10);
    data_0048DBA8.reserve(10);
    data_0048DB88.reserve(10);
    data_0048DB68.reserve(10);
    func_00345DC0(&data_0048DBE8, 10);
    if (!m_isDisabled && !bnkArchiveIsOpen) {
        bnkArchiveIsOpen = true;
        int *entry = func_0039DEF0("RUN\\SOUNDS\\BNKSTORE.ZAR;1");

        if (entry)
            m_bnkoffset = entry[1];
        char path[256];

        strcpy(path, data_003E0520);
        CFileIO::SetRootPath("cdrom0:\\");
        func_00258920(&m_bnkArchive, "run/sounds/bnkstore.zar", 0, 33, 16, 0);
        CFileIO::SetRootPath(path);
        func_00258E60(&m_bnkArchive);
    }
    func_0034D180(data_0048DC30);
    snd_StartSoundSystem(0);
    CSndInstance::Init();
    func_0034C4A0();
    if (!data_0048DF18)
        data_0048DF18 = zrdr_read("sounds.rdr", 0, 0);
    func_00340400(1);
    func_00340430(1, 24, 47);
    func_00340430(4, 24, 47);
    func_00340430(14, 24, 47);
    func_00340430(6, 24, 47);
    func_00340430(2, 24, 47);
    snd_SetReverbType(2, 3);
    if (m_hasreverb) {
        snd_AutoReverb(2, 0, 240, 3);
        data_0048DE40 = 0;
        m_hasreverb = 0;
        data_003DFFB0 = -1;
        data_003DFFC0 = -1;
    }
    theIOPManager.RegisterRebootCallback(IOPRebootHandler, 0);
    snd_system_initialized = true;
    if (data_0048DE48) {
        CSndStrFile &f = data_0048DE90;

        f.Open("RUN\\SOUNDS\\SOUNDRDR.STR");
        if (f.Read())
            data_0048DE60.LoadTable(data_0048DE90.m_data, data_0048DE90.m_size, false);
    }
    Headset::Create();
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
    snd_InitVAGStreamingEx(m_max_num_vags, 0xB000, 0, 1);
}

void CSnd::Open()
{
    CSndVocalQueue &q = data_0048DEE0;

    q.Open();
    CFileCD::m_streamingSndActive = true;
    if (!data_0048DF18)
        data_0048DF18 = zrdr_read("sounds.rdr", 0, 0);
    m_max_num_vags = 6;
    snd_InitVAGStreamingEx(m_max_num_vags, 0xB000, 0, 1);
    if (func_0034BB80())
        func_0034B9E0(func_0034BB80());
}

extern "C" void func_0034BE10();        /* the vocal queues' entries deleted (not written) */

void CSnd::Close()
{
    CSndVocalQueue &q = data_0048DEE0;

    q.Close();
    m_max_num_vags = 0;
    Headset::Reset();
    snd_DoExternCall(0x12C4E67A, 6, 0, 0, 0, 0, 0);
    snd_StopAllStreams();
    snd_CloseVAGStreaming();
    CFileCD::m_streamingSndActive = false;
    func_0034BE10();
    data_003DFFB0 = -1;
    data_003DFFB8 = -1;
    data_003DFFC0 = -1;
    data_003DFFC8 = -1;
    snd_AutoReverb(2, 0, 240, 3);
    data_0048DE40 = 0;
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


/* a reverb record into its two tables: depth (to 32767) and time (to 240 ticks); the count
 * starts at 0 from -1 (ours: the loaders read both values before it, as arguments) */
static inline void AddReverb(std::vector<unsigned int> &depths, std::vector<unsigned int> &times,
                             int &count, float depth, float seconds)
{
    if (count == -1)
        count = 0;
    depths.push_back((int)(32767.0f * depth));
    times.push_back((int)(240.0f * seconds));
    count++;
}

/* a reverb record list read into its two tables. Pinned: LoadReverbs calls both */
#pragma auto_inline off
void CSnd::LoadIndoorReverbs(_zrdr *rdr)
{
    _zrdr *list = func_0032F0D0(rdr, "IndoorReverb");

    if (!list)
        return;
    int n = list->Count();

    for (unsigned int i = 0; (int)i < n; i++) {
        _zrdr *item = list->Item(i);
        float depth = 0.0f;

        zrdr_findreal(item, "Depth", &depth, 1);
        float seconds = 0.0f;

        zrdr_findreal(item, "Seconds", &seconds, 1);
        AddReverb(data_0048DB88, data_0048DB68, data_003DFFB8, depth, seconds);
    }
}

void CSnd::LoadOutdoorReverbs(_zrdr *rdr)
{
    _zrdr *list = func_0032F0D0(rdr, "OutdoorReverb");

    if (!list)
        return;
    int n = list->Count();

    for (unsigned int i = 0; (int)i < n; i++) {
        _zrdr *item = list->Item(i);
        float depth = 0.0f;

        zrdr_findreal(item, "Depth", &depth, 1);
        float seconds = 0.0f;

        zrdr_findreal(item, "Seconds", &seconds, 1);
        AddReverb(data_0048DBC8, data_0048DBA8, data_003DFFC8, depth, seconds);
    }
}
#pragma auto_inline reset

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
    LoadIndoorReverbs(rdr);
    LoadOutdoorReverbs(rdr);
}

void CSnd::SetGroupVol(int group, float vol)
{
    snd_SetMasterVolume(group, (int)(1024.0f * vol));
}

/* SOCOM II's group-volume fade (0x3410D0, not written) restarted when idle (names not known) */
extern float data_0048DE10;     /* the fade's time (1e9: held) */
extern float data_003DFFE0;     /* the fade's position, 0-1 */
extern "C" void func_003410D0(float step);

extern "C" void func_00341060()
{
    if (data_0048DE10 != 0.0f || data_0048DE10 == 1.0e9f) {
        data_003DFFE0 = 1.0f;
        func_003410D0(0.01f);
    }
}

extern "C" double floor(double x);

/* a distance over a speed in miles per hour, in whole minutes (SOCOM II's; name not known) */
extern "C" int func_00340FA0(float a, float b, float mph)
{
    return (int)floor((a - b) / 1609.344f / (mph / 60.0f));
}
