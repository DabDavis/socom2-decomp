/* zsnd_csnd.cpp -- CSnd, a named sound out of sounds.rdr, and the sound system's statics (OVL1
 * 0x340F70-0x3462FF). The unit name is SOCOM 1's (its __sinit_zsnd_csnd.cpp and asserts). SOCOM
 * II's sounds-by-name map is gone, but the unit still has an initialiser (0x402FF0): the statics
 * below, in its order -- defined so, it is byte-exact, and its registered destructors are this
 * unit's tail (0x345F70-0x346260). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zsnd.h"

/* the finder (zrdr_main.cpp; zanim.h still calls it by its placeholder) */
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
#include "../zanim/zanim.h"
#include "../zutil/util_stable.h"
#include "zsnd_queue.h"
#include "zsnd_subtitles.h"

/* a loaded bank (SOCOM II's; the names ours): SOCOM 1's two vectors m_soundbank and m_bankloaded
 * made one of records -- the name LoadBank (0x343DD0) loaded it by, the bank, and 1 when it loaded
 * (-1 when it failed) */
struct CSndBank {
    char m_name[16];            /* 0x00 */
    SoundBankPtr m_bank;        /* 0x10 */
    int m_loaded;               /* 0x14 */
};   /* 0x18 bytes */

/* the sounds file read into a buffer whole (the RTTI's name; its vtable is CBufferIO's) */
class CSoundBufferIO : public CBufferIO {
};

/* The unit's statics, in the order its initialiser (0x402FF0) builds them. SOCOM II's reverb
 * tables, indoor and outdoor: depths and times per record (names not known) */
std::vector<int> data_0048DB68;
std::vector<int> data_0048DB88;
std::vector<int> data_0048DBA8;
std::vector<int> data_0048DBC8;
std::vector<CSndBank> data_0048DBE8;    /* the banks loaded */
std::vector<CSnd *> CSnd::m_soundlist;
CSubtitleTable data_0048DC30;
zar::CZAR CSnd::m_bnkArchive(0, 0);
CMatrix CSnd::m_listenermatrix;
CSTable data_0048DE60(0, 1024);         /* the subtitles' strings */
CSoundBufferIO data_0048DE90;           /* the file they are read from */
CSndVocalQueue data_0048DEE0;           /* SOCOM II's headset voice queue */
/* called through a reference local, as the target's calls dispatch through the vtable (on the
 * object itself MW calls the function directly) */

extern _zrdr *data_0048DF18;            /* the sounds file (GetSoundRdrFile's) */

extern int data_003DFFB0, data_003DFFB8, data_003DFFC0, data_003DFFC8;
static bool snd_system_initialized;     /* SOCOM 1's static (LOCAL in its symbols) */
int zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
extern "C" int toupper(int c);

_zrdr *zrdr_read(const char *name, const char *file, int line);

/* whether a voice other than `except` plays a sound with bit 4 of its flag byte set (OVL1 0x346270,
 * the unit's last row: ReleaseSoundInstance stops the streaming when none does; the name not known).
 * The iterator ASSIGNED from begin() and the end taken into a local first: MW keeps an assigned
 * wrapper iterator in its stack slot (re-read and re-stored at each step, the base iterator copied
 * in through $f0) -- initialised from begin() it lives in a register (0x68); with end() in the test
 * the address of the sentinel takes another register (the MSL headers round 15) */
extern "C" bool func_00346270(CSndInstance *except)
{
    std::list<CSndInstance *>::iterator e = CSndInstance::m_list.end();
    std::list<CSndInstance *>::iterator i;

    for (i = CSndInstance::m_list.begin(); i != e; ++i)
        if (except != *i && (*i)->m_parent->m_socom2_bit4)
            return true;
    return false;
}

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
static bool bnkArchiveIsOpen;           /* SOCOM 1's static */
extern char data_003E0520[];            /* CFileIO's root path ("host0:.\\") */
extern unsigned char data_0048DE48;     /* the subtitles' string table wanted (name not known) */
/* a file in the CD's table of contents (SOCOM 1's DWARF, as util_sonyio.cpp's) */
struct _FILECD {
    char *name;                 // 0x0
    unsigned int startSector;   // 0x4
    unsigned int endSector;     // 0x8
    unsigned int sectorSize;    // 0xc
};
extern "C" void func_00258920(void *zar, const char *name, int a, unsigned int b, unsigned int c,
                              int d);   /* zar::CZAR::Open, a fifth argument added */
extern "C" void func_00258E60(void *zar);           /* zar::CZAR::CloseKeepDir */
extern "C" void func_0034D180(void *p);
extern "C" void func_0034C4A0();

/* The initialised flag's 1 is the subtitles' file Open's mode argument too (a2, loaded once for
 * both: round 6's two-instruction miss was that argument left out). The file is a CBufferIO, read
 * whole: its buffer and the word after it are the table's */
void CSnd::Init()
{
    if (m_isDisabled)
        return;
    data_0048DBC8.reserve(10);
    data_0048DBA8.reserve(10);
    data_0048DB88.reserve(10);
    data_0048DB68.reserve(10);
    data_0048DBE8.reserve(10);
    if (!m_isDisabled && !bnkArchiveIsOpen) {
        bnkArchiveIsOpen = true;
        _FILECD *entry = CFileCD::Find("RUN\\SOUNDS\\BNKSTORE.ZAR;1");

        if (entry)
            m_bnkoffset = entry->startSector;
        char path[256];

        strcpy(path, data_003E0520);
        CFileIO::SetRootPath("cdrom0:\\");
        func_00258920(&m_bnkArchive, "run/sounds/bnkstore.zar", 0, 33, 16, 0);
        CFileIO::SetRootPath(path);
        func_00258E60(&m_bnkArchive);
    }
    func_0034D180(&data_0048DC30);
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
        CSoundBufferIO &f = data_0048DE90;

        f.Open("RUN\\SOUNDS\\SOUNDRDR.STR", 1);
        if (f.IsOpen())
            data_0048DE60.LoadTable(data_0048DE90.m_buffer, data_0048DE90.m_reserve, false);
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
    if (Headset::ftsGetHeadset())
        Headset::ftsGetHeadset()->InitStream();
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

extern int data_0048DB38, data_0048DB40;   /* (the loader's counts; names not known) */

/* every voice stopped, the banks unloaded and every sound deleted, both lists' storage given up
 * (SOCOM 1's, the banks now one vector of records): FreeVector's four pieces are calls, and
 * their out-of-line copies follow this function, the list's first (the image's order) */
void CSnd::UnloadAllSounds()
{
    if (m_isDisabled)
        return;
    CSndInstance::StopAllSounds();
    for (int i = 0; i < data_0048DBE8.size(); i++)
        if (data_0048DBE8[i].m_loaded == 1)
            snd_UnloadBank(data_0048DBE8[i].m_bank);
    FreeVector(data_0048DBE8);
    for (int i = 0; i < m_soundlist.size(); i++)
        if (m_soundlist[i])
            delete m_soundlist[i];
    FreeVector(m_soundlist);
    snd_ResolveBankXREFS();
    data_0048DB40 = 0;
    data_0048DB38 = 0;
    CSndInstance::Cleanup();
    func_0034BE10();
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
static inline void AddReverb(std::vector<int> &depths, std::vector<int> &times,
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
    _zrdr *list = zrdr_findtag(rdr, "IndoorReverb");

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
    _zrdr *list = zrdr_findtag(rdr, "OutdoorReverb");

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

/* SOCOM 1's: the groups' volumes lowered to a part of their own for a time -- while a fade runs
 * only its time lengthened; a reset still running finished first, then each group's own volume
 * kept and the lowered one set */
void CSnd::SetGroupVolTemp(float time, float pct, int g0, int g1, int g2, int g3, int g4, int g5, int g6)
{
    if (m_fadetime > 0.0f) {
        if (time > m_fadetime)
            m_fadetime = time;
        return;
    }
    if (m_fadeRestore < 1.0f) {
        m_fadeRestore = 1.0f;
        ResetGroupVolTemp(0.0f);
    }
    m_fadepct = pct;
    m_fadetime = time;
    m_fadegroup[0] = g0;
    m_fadegroup[1] = g1;
    m_fadegroup[2] = g2;
    m_fadegroup[3] = g3;
    m_fadegroup[4] = g4;
    m_fadegroup[5] = g5;
    m_fadegroup[6] = g6;
    m_fadeRestore = 0.0f;
    for (int i = 0; i < 7; i++) {
        if (m_fadegroup[i] != -1) {
            int vol = snd_GetMasterVolume(m_fadegroup[i]);
            int low = (int)(vol * m_fadepct);

            m_fade_newvol[i] = low;
            m_fade_oldvol[i] = vol;
            snd_SetMasterVolume(m_fadegroup[i], low);
        }
    }
}

/* SOCOM 1's ResetGroupVolTemp, SOCOM II's over five seconds: the reset's position stepped, each
 * group set between its lowered and its own volume by it, the fade forgotten once it is done. The
 * step's two products are named: MW orders a commutative float operation's operands its own way
 * (the global's load first, the converted int last) whatever the expression's order */
void CSnd::ResetGroupVolTemp(float dT)
{
    m_fadeRestore += dT / 5.0f;
    if (m_fadeRestore > 1.0f)
        m_fadeRestore = 1.0f;
    for (int i = 0; i < 7; i++) {
        if (m_fadegroup[i] != -1) {
            float d = m_fade_oldvol[i] - m_fade_newvol[i];
            float v = d * m_fadeRestore;

            SetGroupVol(m_fadegroup[i], (v + m_fade_newvol[i]) / 1024.0f);
        }
    }
    if (m_fadeRestore >= 1.0f) {
        m_fadepct = 0.0f;
        m_fadetime = 0.0f;
        for (int i = 0; i < 7; i++)
            m_fadegroup[i] = -1;
    }
}

/* SOCOM II's group-volume fade restarted when idle (name not known) */
extern "C" void func_00341060()
{
    if (CSnd::m_fadetime != 0.0f || CSnd::m_fadetime == 1.0e9f) {
        CSnd::m_fadeRestore = 1.0f;
        CSnd::ResetGroupVolTemp(0.01f);
    }
}

extern "C" double floor(double x);

/* a distance over a speed in miles per hour, in whole minutes (SOCOM II's; name not known) */
extern "C" int func_00340FA0(float a, float b, float mph)
{
    return (int)floor((a - b) / 1609.344f / (mph / 60.0f));
}
