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
#include "../zcamera/zcamera.h"
#include "../zutil/util_stable.h"
#include "zsnd_queue.h"
#include "zsnd_subtitles.h"
#include "../zsys/zsys_public.h"

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

/* SOCOM 1's statics of this unit (LOCAL in its symbols): the sounds file (GetSoundRdrFile's), its
 * SETS and the current mission's set (LoadSounds'), and that mission's name (0x80 bytes, "None" at
 * start in both builds) -- named by SOCOM 1's LoadSounds, which addresses each at the same
 * instruction (the world lane round 22). Declared, not defined: a definition would add them to the
 * unit's data, whose layout is not modelled yet (the code is the same either way; data.txt names
 * the addresses) */
extern _zrdr *_sound_rdr;               /* 0x48DF18 */
extern _zrdr *_sound_sets_rdr;          /* 0x48DF20 */
extern _zrdr *_sound_mission_rdr;       /* 0x48DF28 */
extern char cur_mission_name[];         /* 0x3DFFF0 */

extern int data_003DFFB0, data_003DFFB8, data_003DFFC0, data_003DFFC8;
static bool snd_system_initialized;     /* SOCOM 1's static (LOCAL in its symbols) */
int zrdr_findreal(const _zrdr *rdr, const char *tag, float *out, int n);
extern "C" int toupper(int c);
extern "C" int atoi(const char *s);

/* PlaySoundVolPanPitch's: the subtitles shown (GetSubtitles' byte), the network up (theNetwork's
 * m_bNetwork, znet.h's), the message queue the subtitle goes to (theMission + 0x6A0) and its
 * Add_Message (C2DMessage_Q's, 0x2B6530), and the stream held while every stream voice is busy (the
 * sound, its voice and a word; names not known) */
extern unsigned char data_003DFFD8;
extern unsigned char data_0045A0C1;
extern char data_00436B80[];
/* the HUD's message queue (fts_message.cpp's C2DMessage_Q, SOCOM 1's class): only its add declared,
 * the kind a byte enum (fts_state.h's MSGTYPE; 0 the player's) -- respelled from its placeholder
 * func_002B6530 (the world lane's round 31) */
enum MSGTYPE {
    MSGTYPE_PLAYER = 0
};
class C2DMessage_Q {
public:
    void Add_Message(const char *text, MSGTYPE type, bool b, int time, float delay);
};
extern CSnd *data_0048DE28;
extern CSndInstance *data_0048DE30;
extern const CPnt3D *data_0048DE38;
/* the position tracker's (zsnd_positions.cpp's, 0x34C2F0) */
extern "C" void func_0034C2F0(CSndInstance *inst);
const char *TranslateString(const char *name);

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
    return _sound_rdr;
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
        m_bnkArchive.Open("run/sounds/bnkstore.zar", 0, 33, 16, 0);
        CFileIO::SetRootPath(path);
        m_bnkArchive.CloseKeepDir();
    }
    data_0048DC30.Init();
    snd_StartSoundSystem(0);
    CSndInstance::Init();
    func_0034C4A0();
    if (!_sound_rdr)
        _sound_rdr = zrdr_read("sounds.rdr", 0, 0);
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
    if (!_sound_rdr)
        _sound_rdr = zrdr_read("sounds.rdr", 0, 0);
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
    if (!_sound_rdr)
        _sound_rdr = zrdr_read("sounds.rdr", 0, 0);
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

inline void CSnd::SetupSoundGroups(const char *name)
{
    if (!m_isDisabled)
        CSndGroup::LoadSoundGroups(name);
}

/* the mission named instead (a debug override: the flag and the name; names not known) */
extern unsigned char data_0048DEC8;
extern char data_0048DEB8[];

/* a mission's sound groups loaded (SOCOM II's; CMission::Open calls it after the mission's sounds,
 * 0x344450; the name not known): a multiplayer map's ("mp...") the common and hostage groups, a
 * campaign mission's the common groups and its teams' by the mission's number */
/* SOCOM II's: the sound list searched for a hash between two indices (the name not known) -- the
 * index's sound when the range is one and its hash is the key, else the half that holds it, the
 * middle through floor() of a double. Recursive: MW inlines it into itself once, the inner copy
 * calling the list's [], the hash and the two compares out of line (0x344E90-0x344ED0) */
extern "C" double floor(double x);

/* the hashes' order and equality (SOCOM II's; out of line in the search's inner copy, 0x344E90 and
 * 0x344EA0: two arguments, no object -- the names ours) */
inline bool HashLess(const unsigned int &a, const unsigned int &b) { return a < b; }
inline bool HashEqual(const unsigned int &a, const unsigned int &b) { return a == b; }

extern "C" CSnd *func_00344BF0(const unsigned int *key, int lo, int hi)
{
    if (lo == hi) {
        if (HashEqual(*key, *CSnd::m_soundlist[lo]->GetHash()))
            return CSnd::m_soundlist[lo];
        return 0;
    }
    int mid = (int)floor(lo + (hi - lo) / 2);
    unsigned int *k = CSnd::m_soundlist[mid]->GetHash();

    if (k && HashLess(*k, *key))
        return func_00344BF0(key, mid + 1, hi);
    return func_00344BF0(key, lo, mid);
}

extern "C" void func_00344910(const char *name)
{
    bool mp = false;
    const char *s = data_0048DEC8 ? data_0048DEB8 : name;

    strlen(s);
    if (s[1] == 'p')
        mp = true;
    if (mp) {
        CSnd::SetupSoundGroups("MP_COMMON");
        CSnd::SetupSoundGroups("HOSTAGE");
    } else {
        int n = atoi(s + 1);

        CSnd::SetupSoundGroups("SP_COMMON");
        switch (n) {
        case 51:
            CSnd::SetupSoundGroups("BRAVO_SEAL");
            CSnd::SetupSoundGroups("ALBANIAN");
            break;
        case 52:
        case 53:
            CSnd::SetupSoundGroups("BRAVO_SAS");
            CSnd::SetupSoundGroups("ALBANIAN");
            break;
        case 61:
        case 62:
        case 63:
            CSnd::SetupSoundGroups("BRAVO_SEAL");
            CSnd::SetupSoundGroups("BRAZILIAN");
            CSnd::SetupSoundGroups("HOSTAGE");
            break;
        case 71:
        case 72:
        case 73:
            CSnd::SetupSoundGroups("BRAVO_SEAL");
            CSnd::SetupSoundGroups("ALGERIAN");
            CSnd::SetupSoundGroups("HOSTAGE");
            break;
        case 81:
        case 83:
            CSnd::SetupSoundGroups("BRAVO_SPET");
            CSnd::SetupSoundGroups("RUSSIAN");
            break;
        case 82:
            CSnd::SetupSoundGroups("BRAVO_SPET");
            CSnd::SetupSoundGroups("RUSSIAN");
            CSnd::SetupSoundGroups("HOSTAGE");
            break;
        }
    }
}

inline void CSnd::LoadSounds(const char *name)
{
    if (!m_isDisabled)
        LoadSounds(name, name);
}

/* a mission's sounds loaded (SOCOM II's; CMission::Open calls it with the mission's name, the
 * groups after it; the name not known): the interface's, the music's and the radio's, the mission's
 * own five files (voice, effects, ambience, the SVO and music lines), then a multiplayer map's
 * shared sounds, or a campaign mission's headquarters' and ABLE's and its teams' by the mission's
 * number (as func_00344910's groups).
 * Round 18: byte-exact with the name chosen by an if/else into an uninitialised local and the flag
 * defined after it. The target keeps the name in s1 and the flag in s0: both have K = 25
 * neighbours, so they are coloured highest number first -- a name initialised by a ternary is
 * numbered after every named local and took s0 (16 off; func_00344910 above, where that is the
 * target's); the flag defined at its declaration, ahead of the if/else, 21 off; the parameter
 * overwritten in place 4 off, the override then landing in s1, not a0 (build/scratch/wld/v_snd*.py) */
extern "C" int func_00344450(const char *name)
{
    const char *s;
    bool mp;
    char buf[32];

    if (data_0048DEC8)
        s = data_0048DEB8;
    else
        s = name;
    mp = false;

    strlen(s);
    if (s[1] == 'p')
        mp = true;
    CSnd::LoadSounds("HUDUI");
    CSnd::LoadSounds("SMUS");
    CSnd::LoadSounds("TCM_ECHO");
    sprintf(buf, "%s_vc", s);
    CSnd::LoadSounds(s, buf);
    sprintf(buf, "%s_fx", s);
    CSnd::LoadSounds(s, buf);
    sprintf(buf, "%s_am", s);
    CSnd::LoadSounds(s, buf);
    sprintf(buf, "%s_svo", s);
    CSnd::LoadSounds(s, buf);
    sprintf(buf, "%s_smu", s);
    CSnd::LoadSounds(s, buf);
    if (mp) {
        CSnd::LoadSounds("MULTI");
        CSnd::LoadSounds("HOSTAGE");
    } else {
        int n = atoi(s + 1);

        CSnd::LoadSounds("HQ");
        CSnd::LoadSounds("ABLE");
        switch (n) {
        case 51:
            CSnd::LoadSounds("BRAVO_SEAL");
            CSnd::LoadSounds("ALBANIAN");
            break;
        case 52:
        case 53:
            CSnd::LoadSounds("BRAVO_SAS");
            CSnd::LoadSounds("ALBANIAN");
            break;
        case 61:
        case 62:
        case 63:
            CSnd::LoadSounds("BRAVO_SEAL");
            CSnd::LoadSounds("BRAZILIAN");
            CSnd::LoadSounds("HOSTAGE");
            break;
        case 71:
        case 72:
        case 73:
            CSnd::LoadSounds("BRAVO_SEAL");
            CSnd::LoadSounds("ALGERIAN");
            CSnd::LoadSounds("HOSTAGE");
            break;
        case 81:
        case 83:
            CSnd::LoadSounds("BRAVO_SPET");
            CSnd::LoadSounds("RUSSIAN");
            break;
        case 82:
            CSnd::LoadSounds("BRAVO_SPET");
            CSnd::LoadSounds("RUSSIAN");
            CSnd::LoadSounds("HOSTAGE");
            break;
        }
    }
    return 1;
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

/* SOCOM 1's: a set of sounds made from the sounds file -- the bank loaded, the mission's set found
 * under SETS (looked up again only for another mission, by its upper-cased name), the bank's own set
 * under it unless it is the mission's, each record read into a new CSnd and added; the error sound
 * looked up again. The bank's number returned, the count stepped on every way out. Byte-exact (the
 * world lane round 22): the loop's test signed (slt, as SOCOM 1's) with the item's index unsigned and
 * recomputed each pass -- an int counter against `(int)Count()` strength-reduces the item's offset
 * into a register of its own (+2 instructions), an unsigned one compared as an int does not */
int CSnd::LoadSounds(const char *name, const char *bank)
{
    if (m_isDisabled)
        return 0;
    LoadBank(bank);
    m_curID = 0;
    int id = m_curbank;

    if (!_sound_rdr) {
        m_curbank++;
        return id;
    }
    if (!_sound_sets_rdr) {
        _sound_sets_rdr = zrdr_findtag(_sound_rdr, "SETS");
        if (!_sound_sets_rdr) {
            m_curbank++;
            return id;
        }
    }
    char buf[128];

    if (!_sound_mission_rdr || strcmp(name, cur_mission_name)) {
        strcpy(cur_mission_name, name);
        CopyUpper(name, buf, 127);
        _sound_mission_rdr = zrdr_findtag(_sound_sets_rdr, buf);
        if (!_sound_mission_rdr) {
            if (!strcmp(name, bank)) {
                m_curbank++;
                return id;
            }
            _sound_mission_rdr = _sound_sets_rdr;
        }
    }
    _zrdr *set = _sound_mission_rdr;

    if (bank && strcmp(bank, name)) {
        CopyUpper(bank, buf, 127);
        set = zrdr_findtag(_sound_mission_rdr, buf);
        if (!set) {
            m_curbank++;
            return id;
        }
    }
    for (unsigned int i = 0; (int)i < (int)set->Count(); i++) {
        CSnd *sound = new CSnd;

        sound->LoadCSnd(set->Item(i));
        AddNewCSnd(sound);
    }
    m_curbank++;
    m_errorsound = GetSoundByName("SND_NEG");
    return id;
}

extern char data_004A1750[];            /* CFileIO's file-name suffix (SOCOM 1's m_file_version) */

/* the sound list searched by hash (0x344F20, a wrapper over the binary search 0x344EE0) */
extern "C" CSnd *func_00344F20(const unsigned int *hash);

/* SOCOM 1's: a sound added to the list -- SOCOM II's keeps it in hash order (the binary search's):
 * inserted before the first sound with a higher hash, else pushed at the end. The search's answer
 * is not used. MSL's insert(position, x) re-loads the data pointer right after its call (its
 * `begin() + n`), so every end() test reuses that one register (the MSL headers' round 17) */
void CSnd::AddNewCSnd(CSnd *sound)
{
    if (m_isDisabled)
        return;
    func_00344F20(&sound->m_18);
    bool done = false;
    std::vector<CSnd *>::iterator i;

    for (i = m_soundlist.begin(); i != m_soundlist.end() && !done; i++) {
        if (*i && sound->m_18 < (*i)->m_18) {
            m_soundlist.insert(i, sound);
            done = true;
        }
    }
    if (i == m_soundlist.end())
        m_soundlist.push_back(sound);
}

/* a key's data offset in the bank archive (SOCOM 1's code, inline in LoadBank in both builds; the
 * name ours): SOCOM II's offset counts from the archive's data start */
static inline bool GetBankKeyOffset(const char *name, unsigned int &offset)
{
    if (CSnd::m_isDisabled)
        return false;
    zar::CKey *key = CSnd::m_bnkArchive.FindKey(name);
    bool found = key != 0;

    if (key) {
        offset = key->m_offset + CSnd::m_bnkArchive.m_data_start;
        CSnd::m_bnkArchive.CloseKey(key);
    }
    return found;
}

/* SOCOM 1's: a bank loaded by name -- off the CD's bank archive when booted from the CD (its key's
 * offset in sectors past the archive's own, the stream synced first), else from its file -- and
 * recorded with whether it loaded; the bank cross-references resolved */
bool CSnd::LoadBank(const char *name)
{
    if (m_isDisabled)
        return false;
    char path[1024];
    bool ok = false;

    sprintf(path, "%sRUN\\SOUNDS\\%s.BNK%s", data_003E0520, name, data_004A1750);
    SoundBankPtr bank;

    if (zSys.isCdBoot) {
        sprintf(path, "%s.BNK", name);
        unsigned int offset = 0;

        int found = GetBankKeyOffset(path, offset);

        if (found) {
            int sector = m_bnkoffset + (offset >> 11);

            snd_StreamSafeCdSync(0);
            while (snd_StreamSafeCdSync(1) == 1)
                ;
            bank = snd_BankLoadByLoc(sector, offset & 0x7FF);
        } else
            bank = 0;
    } else
        bank = snd_BankLoad(path, 0);
    int loaded = -1;

    if (bank) {
        loaded = 1;
        ok = true;
    }
    CSndBank b;

    strncpy(b.m_name, name, 15);
    b.m_loaded = loaded;
    b.m_bank = bank;
    data_0048DBE8.push_back(b);
    snd_ResolveBankXREFS();
    return ok;
}

/* the record readers (zrdr_main.cpp) and a name's hash (0x3A59C0, not named yet) */
bool zrdr_findint(const _zrdr *rdr, const char *tag, int *out, int n);
char *zrdr_findstring(const _zrdr *rdr, const char *tag);
bool zrdr_findPNT2D(const _zrdr *rdr, const char *tag, PNT2D *out);
extern "C" unsigned int func_003A59C0(const char *name);

/* SOCOM 1's: a sound made from its sounds.rdr record -- the name (hashed into m_18), the type and
 * mode tags, a stream's ST block (its two names, priority, offsets, loop and quality flags), the
 * range and volume, a linked sound, a subtitle with its time and valve; numbered in its bank.
 * Byte-exact (the world lane round 23): the range brace-initialised (MW zeroes it in place, a byte
 * loop behind a null test; `PNT2D()` builds a temporary and copies it, a saved register more) */
void CSnd::LoadCSnd(_zrdr *rdr)
{
    if (m_isDisabled)
        return;
    m_soundmode = 4;
    m_isOneShot = false;
    m_socom2_bit7 = false;
    m_18 = rdr->Item(0)->Int();
    char *name = rdr->Item(0)->String();

    if (name) {
        m_name = name;
        m_18 = func_003A59C0(name);
    }
    m_socom2_bit9 = false;
    m_14 = 0;
    if (zrdr_findtag(rdr, "ONESHOT"))
        m_isOneShot = true;
    if (zrdr_findtag(rdr, "TYPE_CONVERSATIONAL"))
        m_mode = 1;
    if (zrdr_findtag(rdr, "TYPE_SITREP"))
        m_mode = 2;
    if (zrdr_findtag(rdr, "TYPE_DEATH_DAMAGE"))
        m_mode = 3;
    if (zrdr_findtag(rdr, "ONESHOT"))
        m_isOneShot = true;
    if (zrdr_findtag(rdr, "AMBIENT"))
        m_socom2_bit1 = true;
    if (zrdr_findtag(rdr, "DOPPLER"))
        m_socom2_bit8 = true;
    if (zrdr_findtag(rdr, "STREAMING_EFX"))
        m_soundmode = 1;
    if (zrdr_findtag(rdr, "MUS"))
        m_soundmode = 2;
    _zrdr *st = zrdr_findtag(rdr, "ST");

    if (st) {
        if (m_soundmode != 1 && m_soundmode != 2)
            m_soundmode = 3;
        m_isStreamed = true;
        _zrdr *t = zrdr_findtag(st, "N1");

        if (t) {
            char *s = t->Item(0)->String();

            if (s)
                m_20 = s;
        }
        t = zrdr_findtag(st, "N2");
        if (t) {
            char *s = t->Item(0)->String();

            if (s)
                m_24 = s;
        }
        if (zrdr_findtag(st, "PRIORITY"))
            m_socom2_bit7 = true;
        if (zrdr_findtag(st, "HS"))
            m_socom2_bit3 = true;
        zrdr_findint(st, "OFFSET1", (int *)&m_34, 1);
        zrdr_findint(st, "OFFSET2", (int *)&m_38, 1);
        if (zrdr_findtag(st, "LOOP"))
            m_socom2_bit9 = true;
        if (zrdr_findtag(st, "HQ"))
            m_socom2_bit4 = true;
        if (zrdr_findtag(st, "VQ"))
            m_socom2_bit5 = true;
        else
            m_socom2_bit5 = false;
        if (zrdr_findtag(st, "PD"))
            m_socom2_bit6 = true;
    }
    PNT2D range = { 0.0f, 0.0f };

    range.x = m_range_min;
    range.y = m_range_max;
    zrdr_findPNT2D(rdr, "RANGE", &range);
    m_range_min = (unsigned int)range.x;
    m_range_max = (unsigned int)range.y;
    zrdr_findreal(rdr, "VOLUME", &m_vol, 1);
    _zrdr *link = zrdr_findtag(rdr, "LINK");

    if (link) {
        char *s = link->Item(0)->String();

        if (s)
            m_14 = GetSoundByName(s);
    }
    char *sub = zrdr_findstring(rdr, "SUBTITLE");

    if (sub) {
        m_28 = sub;
        m_30 = 0.0f;
        zrdr_findreal(rdr, "SUBTITLE_TIME", &m_30, 1);
        char *valve = zrdr_findstring(rdr, "SUBTITLE_VALVE");

        if (valve)
            m_2c = CValve::Create(valve, (VALVE_TYPE)2);
    }
    m_ID = m_curID++;
    m_bank = m_curbank;
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

/* the sound played (SOCOM 1's, the same vtable slot; SOCOM II's adds the linked sound, the valve-gated
 * subtitle through the translated string table and the pending stream): its linked sound first, the
 * subtitle shown, then -- when its bank is loaded or it streams -- a dialog line stops the entity's
 * others, a headset sound is queued to the headset offline, a one-shot plays unowned, a stream plays
 * through StreamSoundVolPan (or is held while every stream voice is busy, when it may be) and anything
 * else plays with the voice's callback. The last argument is the entity the voice belongs to */
CSndInstance *CSnd::PlaySoundVolPanPitch(float vol, int pan, int pitch, CSndInstance *inst,
                                         unsigned int entityID)
{
    if (m_isDisabled)
        return 0;
    if (m_14)
        m_14->PlaySoundVolPanPitch(vol, pan, pitch, 0, 255);
    if (m_28 && data_003DFFD8 && (!m_2c || m_2c->GetValue()))
        ((C2DMessage_Q *)data_00436B80)->Add_Message(TranslateString(m_28), MSGTYPE_PLAYER, false, (int)m_30, 0.0f);
    if (m_bank < (int)data_0048DBE8.size()) {
        if (data_0048DBE8[m_bank].m_loaded == 1 || m_isStreamed) {
            if (m_mode == SND_MODE_DIALOG)
                CSndInstance::Remove(entityID, true, true, false);
            if (m_socom2_bit3 && !data_0045A0C1) {
                Headset::ftsGetHeadset()->Queue(entityID, this, (int)(1024.0f * vol));
            } else if (m_isOneShot && !m_isStreamed) {
                snd_PlaySoundVolPanPMPB_A(data_0048DBE8[m_bank].m_bank, m_ID,
                                          (int)(CSndInstance::GetMasterVol() * (1024.0f * vol * m_vol)), pan,
                                          pitch, 0);
            } else {
                if (!inst)
                    inst = CSndInstance::AcquireSoundInstance(-1, this);
                else
                    inst->m_parent = this;
                if (!inst)
                    return 0;
                inst->m_entityID = entityID;
                if (m_isStreamed) {
                    if (m_numStreams >= m_max_num_vags && m_socom2_bit7) {
                        data_0048DE28 = this;
                        data_0048DE30 = inst;
                        data_0048DE38 = 0;
                        return inst;
                    }
                    if (!StreamSoundVolPan((int)(CSndInstance::GetMasterVol() * (1024.0f * vol * m_vol)), pan,
                                           inst, 0)
                        && inst) {
                        inst->m_handle = 0;
                        CSndInstance::ReleaseSoundInstance(inst);
                        return 0;
                    }
                    return inst;
                }
                snd_PlaySoundVolPanPMPB_CB(data_0048DBE8[m_bank].m_bank, m_ID,
                                           (int)(CSndInstance::GetMasterVol() * (1024.0f * vol * m_vol)), pan,
                                           pitch, 0, CSndInstance::PlaySoundCallback, (unsigned long)inst);
                return inst;
            }
        }
    }
    return 0;
}

/* the sound played at a position (SOCOM 1's, the same vtable slot): as PlaySoundVolPanPitch, the volume
 * and pan from CalcVolPan (inlined: 1 and -1 while the system is off) -- the subtitle only when it is
 * heard at all, a one-shot heard nowhere not played, the commands cached from here on -- and a voice's
 * volume and position set; a stream held keeps its position, and a sound with bit 1 hands its voice
 * to the position tracker (0x34C2F0) and returns none */
CSndInstance *CSnd::PlaySound3D(const CPnt3D *pos, float vol, int pitch, CSndInstance *inst,
                                unsigned int entityID)
{
    if (m_isDisabled)
        return 0;
    if (m_14)
        m_14->PlaySound3D(pos, vol, pitch, 0, 255);
    if (m_bank < (int)data_0048DBE8.size()) {
        if (data_0048DBE8[m_bank].m_loaded == 1 || m_isStreamed) {
            int ivol = 1;
            int ipan = -1;

            CalcVolPan(pos, vol * m_vol, &ivol, &ipan);
            if (ivol && m_28 && data_003DFFD8 && (!m_2c || m_2c->GetValue()))
                ((C2DMessage_Q *)data_00436B80)->Add_Message(TranslateString(m_28), MSGTYPE_PLAYER, false, (int)m_30, 0.0f);
            if (m_isOneShot && !ivol)
                return 0;
            snd_StartCaching();
            if (m_mode == SND_MODE_DIALOG)
                CSndInstance::Remove(entityID, true, true, false);
            if (m_socom2_bit3 && !data_0045A0C1) {
                Headset::ftsGetHeadset()->Queue(entityID, this, ivol * 1024);
            } else if (m_isOneShot && !m_isStreamed) {
                snd_PlaySoundVolPanPMPB_A(data_0048DBE8[m_bank].m_bank, m_ID,
                                          (int)(CSndInstance::GetMasterVol() * ((float)ivol * m_vol)), ipan,
                                          pitch, 0);
            } else {
                if (!inst)
                    inst = CSndInstance::AcquireSoundInstance(-1, this);
                else
                    inst->m_parent = this;
                if (!inst)
                    return 0;
                inst->Set3D(vol);
                inst->SetPos(pos);
                inst->m_entityID = entityID;
                if (m_isStreamed) {
                    if (m_numStreams >= m_max_num_vags && m_socom2_bit7) {
                        data_0048DE28 = this;
                        data_0048DE30 = inst;
                        data_0048DE38 = pos;
                        return inst;
                    }
                    if (!StreamSoundVolPan((int)(CSndInstance::GetMasterVol() * ((float)ivol * m_vol)), ipan,
                                           inst, 1)
                        && inst) {
                        inst->m_handle = 0;
                        CSndInstance::ReleaseSoundInstance(inst);
                        return 0;
                    }
                    if (m_socom2_bit1) {
                        func_0034C2F0(inst);
                        return 0;
                    }
                    return inst;
                }
                snd_PlaySoundVolPanPMPB_CB(data_0048DBE8[m_bank].m_bank, m_ID,
                                           (int)(CSndInstance::GetMasterVol() * ((float)ivol * m_vol)), ipan,
                                           pitch, 0, CSndInstance::PlaySoundCallback, (unsigned long)inst);
                if (m_socom2_bit1) {
                    func_0034C2F0(inst);
                    return 0;
                }
                return inst;
            }
        }
    }
    return 0;
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

/* the volume at a distance (SOCOM 1's CalcVol, an out-of-line function there at 0x2EC780 that its
 * CalcVolPan inlines): full inside the near range, falling to none at the far one. SOCOM II's has no
 * out-of-line copy left, so it is inline */
inline int CSnd::CalcVol(float dist, float vol)
{
    float att;

    if (dist <= m_range_min)
        att = 1.0f;
    else if (dist <= m_range_max)
        att = 1.0f - (dist - m_range_min) / (m_range_max - m_range_min);
    else
        att = 0.0f;
    return (int)(vol * (1024.0f * att * CSndInstance::m_mastervol));
}

/* a 3D sound's volume and pan at a position (SOCOM 1's; SOCOM II's ranges are two shorts and it
 * returns the distance): nothing while the system is off, none without a listener; the volume
 * CalcVol's, the pan the position's bearing in degrees, 0-359.
 * The distance is CPnt3D::Length() (inline: the point's fields read off the frame, not through a
 * CSE'd address), the bearing's depth negated in place in a local of its own (`z *= -1.0f`: the
 * product takes the depth's register, the constant f0 -- `p.z * -1.0f` in the argument swaps them).
 * PlaySound3D auto-inlines it, as SOCOM 1's does: MW's auto-inliner counts statements (15 inline, 16
 * never -- README "Tells from the game states, round 29"), so the body is 15 --
 * CalcVol a call, the two zero stores chained, the depth's copy and negation one expression. Each
 * alone leaves it at 16 and PlaySound3D calls it (0x464 against 0x620); CalcVol's own statements do
 * not count (the world lane's round 17) */
float CSnd::CalcVolPan(const CPnt3D *pos, float vol, int *outVol, int *outPan)
{
    if (m_isDisabled)
        return 0.0f;
    if (!m_listenerIsValid) {
        *outPan = *outVol = 0;
        return 0.0f;
    }
    CPnt3D p;

    m_listenermatrix.Transform(pos, &p, 1);
    float dist = p.Length();

    *outVol = CalcVol(dist, vol);
    float z;

    (z = p.z) *= -1.0f;
    int pan = (int)(57.29578f * atan2f(p.x, z));

    if (pan < 0)
        pan += 360;
    *outPan = pan;
    return dist;
}

/* StreamSoundVolPan's: the stream's file name (a static buffer), the dialog streams allowed (a
 * byte) -- the group volumes lowered for a voice-over are CSnd::m_fadeHeld; the VAG index's lookup -- 1 when the
 * name is in the language's index, 2 in VAGSTORE's, 0 when in neither -- and the two archives' disc
 * sectors (zsnd_subtitles.cpp's CSubtitleTable, 0x34D480 / 0x34D470 / 0x34D460; names not known) */
extern char data_0048DF40[];
extern unsigned char data_003DFFA8;
enum VAG_WHERE { VAG_NONE = 0, VAG_INDEX = 1, VAG_STORE = 2 };   /* a byte (-enum min); names ours */
extern "C" VAG_WHERE func_0034D480(CSubtitleTable *table, const char *name, unsigned int *offset,
                                   int *size);
extern "C" int func_0034D470(CSubtitleTable *table);
extern "C" int func_0034D460(CSubtitleTable *table);

/* a streamed sound started on a voice (SOCOM 1's, its files found in the bank archive by key; SOCOM
 * II's in the VAG indexes): a one-shot voice let finish, a dialog stream refused offline unless
 * allowed or flagged, the sound's two files (.VAG, or .VPK music -- the second channel's names lower
 * case) looked up and, when either is found, the stream started on the music, voice-over or dialog
 * group with its flags -- a voice-over lowering the other groups once. -1 when it started */
int CSnd::StreamSoundVolPan(int vol, int pan, CSndInstance *inst, int flag)
{
    if (m_isDisabled)
        return 0;
    int loc1 = 0;
    int loc2 = 0;
    VAG_WHERE found1 = VAG_NONE;
    VAG_WHERE found2 = VAG_NONE;
    unsigned int offset1;
    unsigned int offset2;
    int size1;
    int size2;

    if (m_isOneShot)
        inst->TerminateOnFinish();
    if (m_soundmode == SND_MODE_DIALOG && !data_003DFFA8 && !data_0045A0C1 && !m_socom2_bit6)
        return 0;
    if (m_20) {
        if (m_soundmode == SND_MODE_MUSIC)
            sprintf(data_0048DF40, "%s.VPK", m_20);
        else
            sprintf(data_0048DF40, "%s.VAG", m_20);
        found1 = func_0034D480(&data_0048DC30, data_0048DF40, &offset1, &size1);
        if (found1 != VAG_NONE) {
            if (found1 == VAG_INDEX) {
                loc1 = (offset1 >> 11) + func_0034D470(&data_0048DC30);
                offset1 &= 0x7FF;
            } else if (found1 == VAG_STORE) {
                if (func_0034D460(&data_0048DC30)) {
                    loc1 = (offset1 >> 11) + func_0034D460(&data_0048DC30);
                    offset1 &= 0x7FF;
                }
            }
        }
    }
    if (m_24) {
        if (m_soundmode == SND_MODE_MUSIC)
            sprintf(data_0048DF40, "%s.vpk", m_24);
        else
            sprintf(data_0048DF40, "%s.vag", m_24);
        found2 = func_0034D480(&data_0048DC30, data_0048DF40, &offset2, &size2);
        if (found2 != VAG_NONE) {
            if (found2 == VAG_INDEX) {
                loc2 = (offset2 >> 11) + func_0034D470(&data_0048DC30);
                offset2 &= 0x7FF;
            } else if (found2 == VAG_STORE) {
                if (func_0034D460(&data_0048DC30)) {
                    loc2 = (offset2 >> 11) + func_0034D460(&data_0048DC30);
                    offset2 &= 0x7FF;
                }
            }
        }
    }
    if (found1 || found2) {
        m_numStreams++;
        int group = 0;

        if (m_soundmode == SND_MODE_MUSIC)
            group = 1;
        else if (m_socom2_bit4)
            group = 6;
        else if (m_soundmode == SND_MODE_DIALOG)
            group = 2;
        int flags = 0;

        if (m_socom2_bit9)
            flags |= 4;
        if (flag && !m_socom2_bit3)
            flags |= 2;
        if (m_socom2_bit4 && !m_fadeHeld) {
            SetGroupVolTemp(1e9f, 0.6f, 0, 1, 2, 3, 4, 5, -1);
            m_fadeHeld = 1;
        }
        snd_PlayVAGStreamByLoc_CB(loc1, loc2, offset1, offset2, vol, pan, group, 0, 0, flags,
                                  CSndInstance::PlaySoundCallback, (unsigned long)inst);
        return -1;
    }
    return 0;
}

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
    m_curbank = 0;
    m_curID = 0;
    CSndInstance::Cleanup();
    func_0034BE10();
}

/* PostTick's: the player indoors (camera.cpp's CftsPlayer), the music depth by the listener's height
 * through the mission's fade range (fts_mission.cpp's 0x2ACA30 over theMission) and the positions'
 * tick (zsnd_positions.cpp's 0x34BEC0); names not known */
class CftsPlayer {
public:
    bool IsInside();
};
extern CftsPlayer ftsPlayer;
extern char theMission[];
extern "C" float func_002ACA30(void *range, float x);
extern "C" void func_0034BEC0(float dT);
extern "C" double floor(double x);
void Vu0MatrixInverseWithUniformScale(CMatrix *dst, CMatrix *src);

/* SOCOM 1's post tick (SOCOM II's adds the outdoor reverbs, the held stream, the positions' tick and
 * the music depth): the listener made valid, then -- while there is a listener -- the reverb of the
 * camera's room set when it changes (the indoor table while the camera or the player is inside, the
 * outdoor one else; none past a table's end) and the listener's matrix inverted; the group fade
 * timed or eased back; the voices and the jukebox ticked; a held stream played once a stream voice is
 * free; the music depth from the listener's height; the vocal queue and, offline, the headset.
 * Round 18: byte-exact -- the room tests `room > -1` (slti/bnez; `room >= 0` branches bltz), the
 * music depth `floor(...) + -128.0` (dpadd with the constant first; `- 128.0` is dpsub) clamped by
 * `depth > 127` (slti at/bnez; `>= 128` puts the compare in v1). Round 17 had measured `> -1` alike
 * only because its tail was `- 128.0`: one size, two differences cancelling */
bool CSnd::PostTick(float dT, void *data)
{
    if (!m_listenerIsValid) {
        m_listenerIsValid = true;
        data_003DFFA8 = 1;
    }
    if (m_listener) {
        zdb::CCamera *cam = zdb::CWorld::m_world->m_camera;
        int room = cam->m_AbovePolyFlag;

        if (cam->m_IsInside || ftsPlayer.IsInside()) {
            if (room > -1 && (data_003DFFB0 != room || data_003DFFB0 == -1)) {
                data_003DFFB0 = room;
                data_003DFFC0 = -1;
                if ((unsigned int)room >= data_0048DB88.size()) {
                    if (m_hasreverb) {
                        snd_AutoReverb(2, 0, 240, 3);
                        data_0048DE40 = 0;
                        m_hasreverb = false;
                        data_003DFFB0 = -1;
                        data_003DFFC0 = -1;
                    }
                } else {
                    snd_AutoReverb(2, data_0048DB88[room], data_0048DB68[room], 3);
                    data_0048DE40 = data_0048DB88[data_003DFFB0];
                    m_hasreverb = true;
                }
            }
        } else if (room > -1 && (data_003DFFC0 != room || data_003DFFC0 == -1)) {
            data_003DFFC0 = room;
            data_003DFFB0 = -1;
            if ((unsigned int)room >= data_0048DBC8.size()) {
                if (m_hasreverb) {
                    snd_AutoReverb(2, 0, 240, 3);
                    data_0048DE40 = 0;
                    m_hasreverb = false;
                    data_003DFFB0 = -1;
                    data_003DFFC0 = -1;
                }
            } else {
                snd_AutoReverb(2, data_0048DBC8[room], data_0048DBA8[room], 3);
                data_0048DE40 = data_0048DBC8[data_003DFFC0];
                m_hasreverb = true;
            }
        }
        sceVu0CopyMatrix(m_listenermatrix.m_matrix, (float (*)[4])m_listener);
        Vu0MatrixInverseWithUniformScale(&m_listenermatrix, &m_listenermatrix);
    }
    if (m_fadetime > 0.0f && m_fadetime != 1e9f)
        m_fadetime -= dT;
    else if (m_fadeRestore < 1.0f && m_fadetime <= 0.0f)
        ResetGroupVolTemp(dT);
    CSndInstance::PostTick(dT, data);
    CSndJukebox::Tick(dT);
    if (m_numStreams < m_max_num_vags && data_0048DE28) {
        if (data_0048DE38)
            data_0048DE28->PlaySound3D(data_0048DE38, 1.0f, 0, data_0048DE30, 255);
        else
            data_0048DE28->PlaySoundVolPanPitch(1.0f, 0, 0, data_0048DE30, 255);
        data_0048DE28 = 0;
        data_0048DE30 = 0;
        data_0048DE38 = 0;
    }
    func_0034BEC0(dT);
    if (m_listener) {
        int depth = (int)(floor(255.0f * func_002ACA30(theMission, ((CMatrix *)m_listener)->m_matrix[3][1]))
                          + -128.0);

        if (depth > 127)
            depth = 127;
        if (depth < -128)
            depth = -128;
        func_003402F0(2, (char)depth);
    }
    if (&data_0048DEE0)
        data_0048DEE0.Tick(dT);
    if (!data_0045A0C1)
        Headset::ftsGetHeadset()->Tick(dT);
    return false;
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

/* SOCOM II's (names ours, after the calls they wrap): a group's master volume set and read straight */
void CSnd::SetMasterVolume(int group, int vol)
{
    snd_SetMasterVolume(group, vol);
}

int CSnd::GetMasterVolume(int group)
{
    return snd_GetMasterVolume(group);
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

/* SOCOM II's (name ours): PostTick's fade step for a caller without the post tick (CMenuState::Tick) */
void CSnd::TickGroupVolTemp(float dT)
{
    if (m_fadetime > 0.0f && m_fadetime != 1e9f)
        m_fadetime -= dT;
    else if (m_fadeRestore < 1.0f && m_fadetime <= 0.0f)
        ResetGroupVolTemp(dT);
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

/* SOCOM II's (name ours): a running group fade ended at once -- the reset's position set done and
 * one step taken, which sets every group back to its own volume and forgets the fade */
void CSnd::EndGroupVolTemp()
{
    if (m_fadetime != 0.0f || m_fadetime == 1.0e9f) {
        m_fadeRestore = 1.0f;
        ResetGroupVolTemp(0.01f);
    }
}

/* SOCOM II's (name ours): the fade StreamSoundVolPan holds for a radio voice (time 1e9) released --
 * left to end at the next step (CSndInstance::ReleaseSoundInstance, the last such voice gone) */
void CSnd::ReleaseGroupVolTemp()
{
    if (m_fadeHeld) {
        m_fadeHeld = 0;
        if (m_fadetime == 1e9f)
            m_fadetime = 0.01f;
    }
}

extern "C" double floor(double x);

/* a distance over a speed in miles per hour, in whole minutes (SOCOM II's; name not known) */
extern "C" int func_00340FA0(float a, float b, float mph)
{
    return (int)floor((a - b) / 1609.344f / (mph / 60.0f));
}
