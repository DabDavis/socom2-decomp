/* reco.cpp -- the voice recogniser (OVL1 0x23A2E0-0x23B98F, its initialiser 0x3FF7B0): SOCOM 1's
 * unit name (Apps/FTS/reco.cpp: recoInit, recoTick, recoStartRecognizing ... as free functions),
 * SOCOM II's rewritten as one global object (the class name, the global's and every member's
 * ours; the two library callbacks keep SOCOM 1's names, cbrecoResult and cbrecoNotify, which both
 * builds hand to the same two library calls). The object is 0x40 bytes: the engine's handle at 0,
 * the language file at 4, three contexts' files and handles and their word files (0x8, 0x14, 0x20;
 * run/reco/<language>/...), the energy level and the last result's confidence (0x2C, 0x30, both
 * 0-1), the active context at 0x34 (0xDEADBEEF none), five flags at 0x38 (initialised, running, and
 * three requests the Tick serves one a frame: stop, set the context, start), the stop's delay at
 * 0x3C. main loads its files (Load), CHUD::Init starts the engine (Init), ~CHUD stops it (UnInit),
 * CCoreState::Tick ticks it (Tick); the radio command menu (OVL1 0x413120, the HUD lane's unwritten
 * 0x22EDB0-0x2375xx) hears each result through cbrecoResult and reads the levels.
 * Defined in reverse of their image order (-inline deferred). All written (round 20); open:
 * AdjustVolume (a stack slot).
 *
 * MW KNOWS WHAT AN ALREADY-GENERATED FUNCTION OF THE UNIT CLOBBERS: Load passes `lang` to the two
 * pinned language lookups, defined after it (so generated first), and keeps it in a0 across the
 * first call -- the target's second call sets no argument at all. Defined before Load, or in a
 * probe calling them from below, the value goes to a saved register and is moved back. */

#include <stdio.h>
#include <string.h>
#include <vector>

#include "../../gamez/zutil/zutil.h"

/* the speech library (OVL2, GCC: the recogniser lane's; called by placeholder where unnamed --
 * by SOCOM 1's recoInit2/recoSetupLanguage/recoSetContextByCol, which make the same calls at the
 * same places: 0x4C7328 gsapi_Init, 0x4C7508 gsapi_LanguageLoadBuffer, 0x4C6CD8 gsapi_EngineOpen,
 * 0x4C8128 gsapi_EngineSetMode, 0x4C7168 gsapi_EngineStop, 0x4C6078 gsapi_ErrorGetString) */
enum _GSAPI_RESULTTYPE {
    GSAPI_RESULTTYPE_FINAL,
    GSAPI_RESULTTYPE_PREMATURE,
    GSAPI_RESULTTYPE_TIMEOUT,
    GSAPI_RESULTTYPE_REJECTED,
    GSAPI_RESULT_ALIGNMENT_HACK = 0xFFFFFFFF
};

/* a recognition (SOCOM 1's DWARF) */
struct _GSAPI_RESULT {
    _GSAPI_RESULTTYPE Type;   // 0x0
    unsigned short u16NbrWords;   // 0x4
    unsigned short u16Conf;   // 0x6
    unsigned int *pWordIds;   // 0x8
};

/* a notification's kind (SOCOM 1's DWARF; SOCOM II's is int-sized: its switch takes it whole) */
enum _GSAPI_NOTIFY_TYPE {
    GSAPI_NOTIFY_ASRRESULT,
    GSAPI_NOTIFY_WAVEDATA,
    GSAPI_NOTIFY_STARTLISTENING,
    GSAPI_NOTIFY_STOPLISTENING,
    GSAPI_NOTIFY_ABNORMCOND,
    GSAPI_NOTIFY_ENERGYLEVEL,
    GSAPI_NOTIFY_SNRLEVEL,
    GSAPI_NOTIFY_SPEECHDETECTED,
    GSAPI_NOTIFY_TSDETECTED,
    GSAPI_NOTIFY_ALIGNMENT_HACK = 0x7FFFFFFF
};

extern "C" {
int func_004C7168(int engine);    /* the engine stopped */
int func_004C6B50(int engine);    /* the engine closed */
int func_004C7718(void);
int func_004C64C8(void);
void func_004C6078(int err);      /* an error reported */
int gsapi_ContextDeActivate(int engine);
int func_004C7328(void *callback, int arg);   /* the library opened, the result callback set */
int func_004C7508(void *language, int arg);   /* the language loaded from its file */
int func_004C6CD8(int arg, int *engine);      /* the engine opened into *engine */
int gsapi_NotifySetCallback(void *callback);
int func_004C8128(int engine, int arg);
int gsapi_EngineSetImmediateContext(int engine, void *context, int handle, void *words);
int func_004C6FF0(int engine);                /* the engine started */
int func_004C6E98(int engine, int param, int value);
int gsapi_ExecServer(int arg);
int func_004C82F0(void *engine, unsigned int id, char **word);   /* a word's text by its id */
}

/* the Logitech USB audio library (zsnd_headset.cpp's view: the description's fields read there,
 * the open parameters here; the layouts ours) */
struct lgAudDeviceDesc {
    unsigned char _0[0x150];
};

struct lgAudOpenParam {
    unsigned char mode;   // 0x0
    unsigned char _1;
    unsigned char channels;   // 0x2
    unsigned char bits;   // 0x3
    unsigned short rate;   // 0x4
    unsigned short bufferms;   // 0x6
    unsigned char _8[0x10 - 0x8];
};

extern "C" {
int lgAudEnumerate(int index, lgAudDeviceDesc *desc);
int lgAudOpen(int index, lgAudOpenParam *param, int *handle);
int lgAudSetRecordingVolume(int handle, int channel, unsigned char volume);
}

enum LANGUAGE {
    LANG_STATES,
    LANG_UK,
    LANG_FRANCE,
    LANG_GERMANY,
    LANG_SPAIN,
    LANG_ITALY,
    LANG_JAPAN,
    LANG_KOREA
};

LANGUAGE GetLanguage();

/* the network (its online flag at 1: the recogniser is single player's) */
class CZNetwork {
public:
    unsigned char m_8;   // 0x0
    unsigned char m_9;   // 0x1: an online game
};

extern CZNetwork theNetwork;

/* the radio command menu (OVL1 0x413120; only what the recogniser touches -- its listening states
 * 1 and 6, the column path whose depth picks the context, a heard word handled, the menu closed) */
class CRadioMenu {
public:
    int GetDepth() { return m_path.size(); }
    std::vector<int> &GetPath() { return m_path; }

    unsigned char _0[0x750];
    unsigned char m_state;   // 0x750
    std::vector<int> m_path;   // 0x754
};

extern CRadioMenu data_00413120;
extern "C" {
bool func_00234F40(CRadioMenu *menu, const char *word);   /* a heard word handled */
void func_00235300(CRadioMenu *menu, int arg);   /* the menu closed */
void func_0022F330();   /* (the radio menu's: called when a heard command was handled) */
}

/* the recogniser */
class CReco {
public:
    CReco();
    ~CReco();

    bool Load();
    bool Init();
    void UnInit();
    void Suspend(bool now);
    void Resume();
    void Tick(float dT);
    static void AdjustVolume(int delta);
    float GetLevel();
    void SetConfidence(int conf);
    float GetConfidence();

    /* the engine started or stopped, an error reported */
    void Start()
    {
        if (m_init && !m_running) {
            int err = func_004C6FF0(m_engine);

            if (err >= 0)
                m_running = 1;
            else
                func_004C6078(err);
        }
    }
    void Stop()
    {
        if (m_init && m_running) {
            int err = func_004C7168(m_engine);

            if (err >= 0)
                m_running = 0;
            else
                func_004C6078(err);
        }
    }
    /* a context made the engine's immediate one, and the engine started */
    int SetContext(int context)
    {
        int err;

        m_context = context;
        err = gsapi_EngineSetImmediateContext(m_engine, m_contexts[m_context], m_handles[m_context],
                                              m_words[m_context]);
        if (err >= 0)
            Start();
        else
            func_004C6078(err);
        return err;
    }

    int m_engine;   // 0x0
    void *m_language;   // 0x4: the language file
    void *m_contexts[3];   // 0x8: the contexts' files
    int m_handles[3];   // 0x14
    void *m_words[3];   // 0x20: the contexts' word files
    float m_level;   // 0x2c: the energy level, 0-1
    float m_confidence;   // 0x30: the last result's, 0-1
    unsigned int m_context;   // 0x34: the active context (0xDEADBEEF none)
    unsigned int m_init : 1;   // 0x38 bit 0
    unsigned int m_running : 1;   // bit 1
    unsigned int m_startReq : 1;   // bit 2: start the engine
    unsigned int m_stopReq : 1;   // bit 3: stop it
    unsigned int m_contextReq : 1;   // bit 4: set the context from the radio menu's depth
    unsigned int m_rest : 27;
    float m_timer;   // 0x3c: the stop's delay
};

/* the result printer's volume steps and confidence bounds */
static int s_volumeUp = 5;
static int s_volumeDown = -5;
static int s_confHigh = 7500;
static int s_confLow = 5000;
/* frames a listening radio menu waits without an energy level before the engine is restarted */
static int s_quietMax = 30;
/* the push-to-talk delay (1.0, in .data) */
static float s_delay = 1.0f;
/* the languages' directories and files (their index GetLanguage's) */
static const char *s_codeUK = "eng";
static const char *s_codeFrance = "frf";
static const char *s_codeGermany = "ged";
static const char *s_codeSpain = "spe";
static const char *s_codeItaly = "iti";
static const char *s_codeJapan = "jpj";
static const char *s_codeKorea = "kok";
static const char *s_codeStates = "enu";
static const char *s_fileUK = "2004b.lng";
static const char *s_fileFrance = "2004b.lng";
static const char *s_fileGermany = "3004b.lng";
static const char *s_fileSpain = "3004b.lng";
static const char *s_fileItaly = "3004b.lng";
static const char *s_fileJapan = "2004b.lng";
static const char *s_fileKorea = "2004b.lng";
static const char *s_fileStates = "3004b.lng";
/* the radio menu's root column, pushed when a command was not understood */
static const int s_root = 0;
/* the recording volume, clamped to 20-100 (the levels in .data) */
static int s_volume = 50;
static int s_volume_max = 100;
static int s_volume_min = 20;

/* the result kinds' names, for the printer */
static const char *s_resultTypes[] = { "final", "premature", "timeout", "rejected" };

/* frames since the last energy level */
static int s_quiet;

CReco theReco;

static const char *GetLangCode(LANGUAGE lang);
static const char *GetLangFile(LANGUAGE lang);
void cbrecoResult(void *hEngine, unsigned int action, _GSAPI_RESULT *pResult, unsigned int);
void cbrecoNotify(void *hEngine, _GSAPI_NOTIFY_TYPE type, unsigned int wParam, unsigned int);

CReco::CReco()
{
    m_engine = 0;
    m_language = 0;
    for (int i = 0; i < 3; i++) {
        m_contexts[i] = 0;
        m_handles[i] = 0;
        m_words[i] = 0;
    }
    m_context = 0xDEADBEEF;
    m_init = 0;
    m_running = 0;
    m_startReq = 0;
    m_stopReq = 0;
    m_contextReq = 0;
    m_rest = 0;
    m_level = 0.0f;
    m_confidence = 0.0f;
    m_timer = 0.0f;
}

CReco::~CReco()
{
}

/* a file read whole into a new buffer; the bytes read */
static inline int LoadFile(const char *name, void *&buf)
{
    CFileIO f;
    int n = 0;

    if (f.Open(name, 1)) {
        buf = _malloc(f.GetSize(), 0, 0);
        n = f.fread(buf, f.GetSize());
        f.Close();
    }
    return n;
}

/* the files read, once: the language, then each context's file and its word list (the 8-bit
 * list, else the plain one); true when the language was read */
bool CReco::Load()
{
    bool ok = false;

    if (!m_language) {
        char path[256];
        LANGUAGE lang = GetLanguage();
        const char *code = GetLangCode(lang);
        const char *file = GetLangFile(lang);

        sprintf(path, "%s/%s/%s%s", "run/reco/", code, code, file);
        if (LoadFile(path, m_language))
            ok = true;
        for (unsigned int i = 0; i < 3; i++) {
            sprintf(path, "%s/%s/%s_RL%d.ctx", "run/reco/", code, code, i);
            LoadFile(path, m_contexts[i]);
            sprintf(path, "%s/%s/%s_RL%d.wr8", "run/reco/", code, code, i);
            if (!LoadFile(path, m_words[i])) {
                sprintf(path, "%s/%s/%s_RL%d.wrd", "run/reco/", code, code, i);
                LoadFile(path, m_words[i]);
            }
        }
    }
    return ok;
}

bool CReco::Init()
{
    bool ok = false;
    int err;

    if (!m_init && m_language && func_004C7328((void *)cbrecoResult, 0) >= 0) {
        err = func_004C7508(m_language, 0);
        if (err >= 0) {
            err = func_004C6CD8(0, &m_engine);
            if (err >= 0) {
                err = gsapi_NotifySetCallback((void *)cbrecoNotify);
                if (err >= 0) {
                    err = func_004C8128(m_engine, 5);
                    if (err < 0)
                        func_004C6078(err);
                }
            }
        }
        if (err >= 0) {
            int r;

            m_context = 0;
            r = gsapi_EngineSetImmediateContext(m_engine, m_contexts[m_context], m_handles[m_context],
                                                m_words[m_context]);
            if (r >= 0) {
                if (m_init && !m_running) {
                    int e = func_004C6FF0(m_engine);

                    if (e >= 0)
                        m_running = 1;
                    else
                        func_004C6078(e);
                }
            } else
                func_004C6078(r);
            if (r >= 0) {
                err = func_004C6E98(m_engine, 1, 0);
                if (err >= 0) {
                    err = func_004C6E98(m_engine, 0, 1);
                    if (err >= 0) {
                        err = func_004C6E98(m_engine, 8, 1);
                        if (err < 0)
                            func_004C6078(err);
                    }
                }
                if (err >= 0) {
                    gsapi_ExecServer(0);
                    ok = true;
                    m_init = 1;
                    Suspend(true);
                }
            }
        }
    }
    return ok;
}

void CReco::UnInit()
{
    int err;

    if (m_init && m_running) {
        err = func_004C7168(m_engine);
        if (err >= 0)
            m_running = 0;
        else
            func_004C6078(err);
    }
    gsapi_ContextDeActivate(m_engine);
    if (m_engine) {
        err = func_004C6B50(m_engine);
        if (err < 0)
            func_004C6078(err);
        m_engine = 0;
    }
    err = func_004C7718();
    if (err < 0)
        func_004C6078(err);
    err = func_004C64C8();
    if (err < 0)
        func_004C6078(err);
    m_init = 0;
}

/* a stop requested, now or after the push-to-talk delay */
void CReco::Suspend(bool now)
{
    m_stopReq = 1;
    m_startReq = 0;
    m_contextReq = 0;
    m_timer = now ? 0.0f : s_delay;
}

/* a stop, a new context and a start requested, in that order */
void CReco::Resume()
{
    m_stopReq = 1;
    m_startReq = 0;
    m_contextReq = 0;
    m_timer = 0.0f;
    m_contextReq = 1;
    m_startReq = 1;
}

/* a frame (single player only): the stop's delay run down, else one request served -- the stop,
 * the context the radio menu's depth picks, the start; then, initialised, a listening menu that
 * hears no energy for s_quietMax frames restarts the engine, and the library is served */
void CReco::Tick(float dT)
{
    if (theNetwork.m_9)
        return;
    if (m_timer > 0.0f) {
        m_timer -= dT;
        if (m_timer < 0.0f)
            m_timer = 0.0f;
    } else if (m_stopReq) {
        Stop();
        m_stopReq = 0;
        s_quiet = 0;
    } else if (m_contextReq) {
        int depth = data_00413120.GetDepth();

        if (depth > 0)
            theReco.SetContext(depth - 1);
        m_contextReq = 0;
    } else if (m_startReq) {
        Start();
        m_startReq = 0;
        s_quiet = 0;
    }
    if (m_init || m_contextReq) {
        if (data_00413120.m_state == 1 || data_00413120.m_state == 6)
            s_quiet++;
        if (s_quiet > s_quietMax) {
            s_quiet = 0;
            Resume();
        }
        gsapi_ExecServer(0);
    }
}

/* the language's directory name (called out of line by Load, as are the files: pinned) */
#pragma auto_inline off
static const char *GetLangCode(LANGUAGE lang)
{
    switch (lang) {
    case LANG_UK:
        return s_codeUK;
    case LANG_FRANCE:
        return s_codeFrance;
    case LANG_GERMANY:
        return s_codeGermany;
    case LANG_SPAIN:
        return s_codeSpain;
    case LANG_ITALY:
        return s_codeItaly;
    case LANG_JAPAN:
        return s_codeJapan;
    case LANG_KOREA:
        return s_codeKorea;
    default:
        return s_codeStates;
    }
}

/* the language's file */
static const char *GetLangFile(LANGUAGE lang)
{
    switch (lang) {
    case LANG_UK:
        return s_fileUK;
    case LANG_FRANCE:
        return s_fileFrance;
    case LANG_GERMANY:
        return s_fileGermany;
    case LANG_SPAIN:
        return s_fileSpain;
    case LANG_ITALY:
        return s_fileItaly;
    case LANG_JAPAN:
        return s_fileJapan;
    case LANG_KOREA:
        return s_fileKorea;
    default:
        return s_fileStates;
    }
}
#pragma auto_inline reset

static void PrintResult(void *hEngine, _GSAPI_RESULT *pResult, int action);

/* a recognition (the library's result callback): printed, its words after the first handed to a
 * listening radio menu until one is refused; a menu not listening is closed (a command ran when
 * one was handled) and the engine stopped; a listening one either runs what it heard, or, nothing
 * understood, goes back to its root column and the engine restarts */
void cbrecoResult(void *hEngine, unsigned int action, _GSAPI_RESULT *pResult, unsigned int)
{
    bool closed = false;
    unsigned int n;
    char *word;
    bool more;
    bool heard;
    unsigned int i;

    if (data_00413120.m_state != 1 && data_00413120.m_state != 6)
        closed = 1;
    PrintResult(hEngine, pResult, action);
    n = pResult->u16NbrWords;
    if ((int)n > 0)
        n--;
    word = 0;
    i = 1;
    more = true;
    heard = false;
    while (i < n && more) {
        if (func_004C82F0(hEngine, pResult->pWordIds[i], &word) >= 0 && word && strlen(word)) {
            heard = true;
            more = func_00234F40(&data_00413120, word);
        }
        i++;
    }
    if (closed) {
        if ((!more || n < 3) && heard)
            func_0022F330();
        func_00235300(&data_00413120, 0);
        theReco.Suspend(true);
    } else if (data_00413120.m_state == 1) {
        theReco.Resume();
    } else if (data_00413120.m_state == 6) {
        if (more && n > 2) {
            func_00235300(&data_00413120, 0);
        } else if (heard) {
            func_0022F330();
            func_00235300(&data_00413120, 0);
        } else {
            data_00413120.m_path.clear();
            data_00413120.GetPath().push_back(s_root);
            theReco.Resume();
        }
    }
}

/* the recording volume stepped by delta, clamped to 20-100, and set on the first device that opens
 * (the levels in .data) */
/* OPEN, 6 instructions: the open parameters' stack slot -- the target's at sp+456 (its fields at
 * 458-462), here at sp+448: the frame (480), the registers and the code agree. A 16-byte struct
 * gives the frame; 8 bytes put it above the description but shrink the frame to 464, 20 bytes
 * move it under the description, a 0x158-byte description grows the frame to 496; every order of
 * the three locals tried (perm_lines). Something 8 bytes wide sits between the two in the
 * original. */
void CReco::AdjustVolume(int delta)
{
    lgAudOpenParam param;
    lgAudDeviceDesc desc;
    int handle = -1;
    int index;
    int volume = s_volume + delta;

    index = 0;

    if (volume > s_volume_max)
        volume = s_volume_max;
    else if (volume < s_volume_min)
        volume = s_volume_min;
    if (volume != s_volume && lgAudEnumerate(0, &desc) == 0) {
        do {
            param.mode = 2;
            param.channels = 1;
            param.bits = 16;
            param.rate = 11025;
            param.bufferms = 500;
            if (lgAudOpen(index, &param, &handle) == 0) {
                lgAudSetRecordingVolume(handle, 0, volume);
                s_volume = volume;
                return;
            }
            index++;
        } while (lgAudEnumerate(index, &desc) == 0);
    }
}

/* the library's notifications: a result printed, the energy level scaled into 0-1 (its quiet count
 * restarted) */
void cbrecoNotify(void *hEngine, _GSAPI_NOTIFY_TYPE type, unsigned int wParam, unsigned int)
{
    switch (type) {
    case GSAPI_NOTIFY_ENERGYLEVEL:
        {
            float level = (float)(int)wParam - -6000.0f;

            if (level > 7000.0f)
                level = 1.0f;
            else if (level > 0.0f)
                level /= 7000.0f;
            else
                level = 0.0f;
            theReco.m_level = level;
            s_quiet = 0;
        }
        break;
    case GSAPI_NOTIFY_ASRRESULT:
        PrintResult(hEngine, (_GSAPI_RESULT *)wParam, 0);
        break;
    case GSAPI_NOTIFY_WAVEDATA:
    case GSAPI_NOTIFY_STARTLISTENING:
    case GSAPI_NOTIFY_STOPLISTENING:
    case GSAPI_NOTIFY_ABNORMCOND:
    case GSAPI_NOTIFY_SNRLEVEL:
    case GSAPI_NOTIFY_SPEECHDETECTED:
    case GSAPI_NOTIFY_TSDETECTED:
        break;
    }
}

/* a recognition printed (its word ids and texts), its confidence kept, and the recording volume
 * stepped up under s_confLow, down over s_confHigh */
static void PrintResult(void *hEngine, _GSAPI_RESULT *pResult, int action)
{
    char *word;

    if (pResult) {
        int i;
        int first = 1;
        char words[2000] = {0};
        char ids[2000] = {0};
        char *p;

        (void)*(volatile int *)0x10000800;
        p = ids;
        if (pResult->u16NbrWords > 0) {
            for (i = 0; i < pResult->u16NbrWords; i++) {
                unsigned int id = pResult->pWordIds[i];

                if (func_004C82F0(hEngine, id, &word) < 0)
                    return;
                if (first)
                    p += sprintf(p, "(");
                first = 0;
                p += sprintf(p, "%d%c", id, (char)(i == pResult->u16NbrWords - 1 ? ')' : ','));
                strcat(words, word);
                strcat(words, " ");
            }
        }
        printf("Result: Action %d '%s' (Conf.%d)\n", action, words, pResult->u16Conf);
        theReco.SetConfidence(pResult->u16Conf);
        if (pResult->u16Conf < s_confLow)
            CReco::AdjustVolume(s_volumeUp);
        else if (pResult->u16Conf > s_confHigh)
            CReco::AdjustVolume(s_volumeDown);
        printf("result of %d words\n", pResult->u16NbrWords);
        printf("%s   %s\n", words, ids);
        printf("%d (type %d - %s)\n", pResult->u16Conf, pResult->Type, s_resultTypes[pResult->Type]);
        printf("%d\n", action);
    }
}

float CReco::GetLevel()
{
    return m_level;
}

/* the last result's confidence, 0-5000, kept as 0-1 */
void CReco::SetConfidence(int conf)
{
    if (conf < 0.0f)
        conf = 0;
    else if (conf > 5000.0f)
        conf = 5000;
    m_confidence = conf / 5000.0f;
}

float CReco::GetConfidence()
{
    return m_confidence;
}
