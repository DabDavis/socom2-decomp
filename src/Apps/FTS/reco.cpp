/* reco.cpp -- the voice recogniser (OVL1 0x23A2E0-0x23B98F, its initialiser 0x3FF7B0): SOCOM 1's
 * unit name (Apps/FTS/reco.cpp: recoInit, recoTick, recoStartRecognizing ... as free functions),
 * SOCOM II's rewritten as one global object (the class name, the global's and every function's
 * ours). The object is 0x40 bytes: the engine's handle at 0, the language file at 4, three
 * contexts' files and handles and their word files (0x8, 0x14, 0x20; run/reco/<language>/...),
 * two levels (0x2C, 0x30), 0xDEADBEEF at 0x34 and five flags at 0x38 (initialised, recognising,
 * three of the push-to-talk state), the push-to-talk timer at 0x3C. main loads its files
 * (0x23B540), CHUD::Init starts the engine (0x23B300), ~CHUD stops it (UnInit), CCoreState::Tick
 * ticks it (0x23AD90). Written: the constructor and destructor (the initialiser registers the
 * destructor), Init, UnInit, the two push-to-talk setters and the recording volume's step; the rest
 * are the m2c drafts or unwritten (the result and notify callbacks 0x23A860/0x23A640, their
 * helper 0x23A380, the Tick and the files' loader). Defined in reverse of their image order
 * (-inline deferred). */

/* the speech library (OVL2, GCC: the recogniser lane's; unnamed yet) */
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
/* the recogniser's two callbacks (this unit's, not written yet) */
void func_0023A860();
void func_0023A640();
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

/* the recogniser */
class CReco {
public:
    CReco();
    ~CReco();

    bool Init();
    void UnInit();
    void Suspend(bool now);
    void Resume();
    static void AdjustVolume(int delta);

    int m_engine;   // 0x0
    void *m_language;   // 0x4: the language file
    void *m_contexts[3];   // 0x8: the contexts' files
    int m_handles[3];   // 0x14
    void *m_words[3];   // 0x20: the contexts' word files
    float m_level;   // 0x2c
    float m_threshold;   // 0x30
    unsigned int m_context;   // 0x34: the active context (0xDEADBEEF none)
    unsigned int m_init : 1;   // 0x38 bit 0
    unsigned int m_running : 1;   // bit 1
    unsigned int m_listening : 1;   // bit 2
    unsigned int m_stopping : 1;   // bit 3
    unsigned int m_held : 1;   // bit 4
    unsigned int m_rest : 27;
    float m_timer;   // 0x3c
};

/* the push-to-talk delay (1.0, in .data) */
static float s_delay = 1.0f;

CReco theReco;

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
    m_listening = 0;
    m_stopping = 0;
    m_held = 0;
    m_rest = 0;
    m_level = 0.0f;
    m_threshold = 0.0f;
    m_timer = 0.0f;
}

CReco::~CReco()
{
}

bool CReco::Init()
{
    bool ok = false;
    int err;

    if (!m_init && m_language && func_004C7328((void *)func_0023A860, 0) >= 0) {
        err = func_004C7508(m_language, 0);
        if (err >= 0) {
            err = func_004C6CD8(0, &m_engine);
            if (err >= 0) {
                err = gsapi_NotifySetCallback((void *)func_0023A640);
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

void CReco::Suspend(bool now)
{
    m_stopping = 1;
    m_listening = 0;
    m_held = 0;
    m_timer = now ? 0.0f : s_delay;
}

void CReco::Resume()
{
    m_stopping = 1;
    m_listening = 0;
    m_held = 0;
    m_timer = 0.0f;
    m_held = 1;
    m_listening = 1;
}

/* the recording volume stepped by delta, clamped to 20-100, and set on the first device that opens
 * (the levels in .data) */
static int s_volume = 50;
static int s_volume_max = 100;
static int s_volume_min = 20;

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
