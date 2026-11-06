/* st_core.cpp -- the core state: the mission's load, its tick, its teardown (SOCOM 1's
 * Apps/FTS/st_core.cpp, its DWARF's line order; OVL1 0x1EB070-0x1EDFFF). Defined in reverse of
 * their image order (-inline deferred). */

#include "fts_state.h"
#include "../../gamez/zrdr/zrdr.h"
#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/ztwod/c2dfont.h"

#include <algorithm>
#include <iostream>

/* unnamed yet (SOCOM 1's names in the comments) */
extern "C" {
void exit(int code);
void func_002A6950(void *mission);
void func_002B6530(void *queue, const char *text, MSGTYPE type, int b, int c, float time);   /* C2DMessage_Q::Add_Message (the type a byte enum: CCoreState::DisplayMessage copies it lb) */
void func_005B61A0();

}

void ftsNetworkGameUninit();
void DMEDisconnectFromGameServer();

class CZProjectileList {
public:
    void UnInit();
};

class CZMarkList {
public:
    void UnInit();
};

class CZWeapon {
public:
    static void DestroyProjectileList();   /* 0x3D3930 */
    static void DestroyMarkList();   /* 0x3D37E0 */
    static bool PostTick(float dT, void *data);   /* 0x3D1F50 (the weapons lane's round 16) */

    static CZProjectileList *m_pProjectilelist;
    static CZMarkList *m_pMarklist;
};

class CTickNode;

class CEntity {
public:
    /* (zentity.h's returns its zdb::CNode; the tick's view of it) */
    CTickNode *GetNode();   /* 0x29ED20 */

    static bool PreTickAll(float dT, void *data);
    static bool TickAll(float dT, void *data);
    static bool PrePostTickAll(float dT, void *data);
    static bool PostTickAll(float dT, void *data);
};

/* the AI's event tick and route planner (the AI core's tick tasks) */
class CAiEvent {
public:
    static bool Tick(float dT, void *data);
};

namespace ai {
bool TickPathPlanner(float dT, void *data);
}

const char *GetDatabase();

/* a seal's controller (CEntity's at 0xC0) */
class CZSealBody : public CEntity {
public:
    unsigned char _0[0xc0];
    void *m_control;   // 0xc0
};

extern "C" {
void func_003B0B90(int a, int b, int w, int h);
void func_003B09E0(int a, int b, int w, int h);
void func_003B0B10(int *rect);
void func_003B0A70(int *rect);
void func_003B0660();
void func_00592CA0(void *control);

}

extern "C" {

/* the tick tasks (SOCOM 1's in the comments, where it had them) */
bool func_003D1F90(float dT, void *data);        /* CZWeapon::PreTick */
bool func_0054F570(float dT, void *data);
bool func_0060BC40(float dT, void *data);
bool func_0060B8F0(float dT, void *data);
bool func_005CCA30(float dT, void *data);        /* CSealCtrlAi::PostTick */

}

/* the film player, the bone motions' buffer, the sound instances and the valves (their units'; only
 * what the states call) */
class CMPEG {
public:
    static bool Done();   /* 0x3096C0 */
    static void End();   /* 0x3096F0 */
    static void Tick(int field);   /* 0x309850 */
    static void SetMaxHeight(int height);   /* 0x309550 */
    static int Run(const char *name, bool b1, bool b2, int i);   /* 0x3098E0 */
    static void Init(void *buf, int size);   /* 0x309B40 */
    static void Uninit();   /* 0x309A10 */
};

u_long128 *zBoneMotionRdrBuffer();   /* 0x288930 */
int zBoneMotionRdrSize();   /* 0x288920 */

class CSndInstance {
public:
    static void PauseAllSounds();   /* 0x346740 */
    static void ResumeAllSounds();   /* 0x3466F0 */
};

/* the headset (zsnd.h's): the one there is, and its queue flushed -- by the row's own name: the
 * film's Init hands the flush a flag (a1 = 1) its body never reads, where zsnd.h's Flush() takes
 * none */
class Headset {
public:
    static Headset *ftsGetHeadset();   /* 0x34BB80 */
};

extern "C" void Flush__7HeadsetFb(Headset *headset, bool b);   /* 0x34A6F0 (Flush(bool): the world's round 27) */
extern "C" int snd_StreamSafeCheckCDIdle(int block);   /* 0x33F550 */

/* the system's public state (zsys_public.h's _zsys_public: only its timer, as camera.cpp's) */
struct _zsys_public {
    unsigned char _0[0x24];
    unsigned int timerTicksPerSecond;   // 0x24
    float timerScale;   // 0x28
};

extern _zsys_public zSys;

/* the timer's count register and the GIF's mode (Sony's T0_COUNT and GIF_MODE) */
#define T0_COUNT ((volatile unsigned int *)0x10000000)
#define GIF_MODE ((volatile unsigned int *)0x10003010)

class CValve {
public:
    static void Reset();   /* 0x351CB0 */
};

/* the squads (seal_unit.cpp's; only their tick task) */
class CSealUnit {
public:
    static bool TickAll(float dT, void *data);
};

/* the AI SEALs' controller (zseal.h's; only its "ai_pre_tick" task: SOCOM 1's static
 * CSealCtrlAi::PreTick(float, void *), OVL2 0x5CCCD0, the kit lane's round 46) */
class CSealCtrlAi {
public:
    static bool PreTick(float dT, void *data);
};

extern CHUD *theHUD;

/* database.cpp's (SOCOM 1's) */
bool testLoad();
bool testUnload();

void ftsSetPlayer(CZSealBody *seal);
CZSealBody *ftsGetPlayer();

/* zdb's task scheduler: a list of tasks (SOCOM 1's DWARF: 0x20 bytes) */
class CSched_Manager {
public:
    void Tick(float dT);   /* 0x34E070 */
    bool AddTask(const char *name, bool (*task)(float, void *), float priority, void *data);
    bool RemoveTask(const char *name, bool all);
    void ClearFreeList(bool all);

    unsigned char _0[0x20];
};

extern CSched_Manager zTaskScheduler;


/* what the core state's tick reaches for */
namespace zdb {
/* the render phases (SOCOM 1's _RenderPhase; only the HUD's three, by number) */
enum _RenderPhase {
    RENDER_PHASE_12 = 12,
    RENDER_PHASE_13 = 13,
    RENDER_PHASE_14 = 14
};
}

class MapCompass {
public:
    void Draw(zdb::CCamera *cam);
};

class CFlashFXVector {
public:
    void Tick(float dT);
    void Draw(zdb::CCamera *cam);
};

/* a 2D item: its vtable pointer after a list (C2D's), Draw the second virtual */
class CTickItem {
public:
    unsigned char _0[0xc];
    virtual ~CTickItem();
    virtual void Draw(zdb::CCamera *cam);
};

/* the HUD as the core state ticks it (SOCOM 1's CHUD: its compass, its items, its flashes) */
class CHUD {
public:
    CHUD();   /* 0x1FCD80 */
    void Init(zdb::CWorld *world);   /* 0x1FA360 */
    void ReInit_Init();   /* 0x1FB420 */
    bool PauseGame();   /* 0x1FA300 */
    static void ReleaseHUD(CHUD **hud);   /* 0x1FCB70 */
    void PreDITick(float dT);
    void PostDITick(float dT);
    void Tick(float dT);
    void DrawPhase(zdb::_RenderPhase phase, zdb::CCamera *cam);

    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[0x4fe0 - 0x49c];
    MapCompass m_compass;   // 0x4fe0
    unsigned char _4fe1[0x18900 - 0x4fe1];
    CTickItem m_18900;   // 0x18900
    unsigned char _18910[0x1a2d0 - 0x18910];
    CFlashFXVector m_flashfx;   // 0x1a2d0
    unsigned char _1a2d1[0x1acd0 - 0x1a2d1];
};   // 0x1acd0 bytes (the core state's new)

/* the entity's node as the tick hides it: its active bit, SetActive the 13th virtual (CNode's,
 * its vtable pointer at 0x60) */
class CTickNode {
public:
    unsigned char _0[0x5c];
    unsigned int m_active : 1;   // 0x5c
    unsigned int _5c : 31;
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual bool SetActive(bool on);
};

/* SOCOM II's voice recogniser (reco.cpp's CReco, theReco: main loads its files, the tick ticks
 * it; only what the tick reads -- bit 1 at 0x38, the engine running, is SOCOM 1's
 * recoIsRecognizing; reco.cpp's word of bits read here as its first byte) */
class CReco {
public:
    void Tick(float dT);

    unsigned char _0[0x38];
    unsigned char m_init : 1;   // 0x38
    unsigned char m_running : 1;
    unsigned char _b2 : 6;
};

class CameraZoom {
public:
    static bool Tick(float dT);
    static void SetGoal(float goal);   /* 0x1F1820 */
    static void AttachHud(CHUD *hud);   /* 0x1F1840 */
    static void AttachSeal(CZSealBody *seal);   /* 0x1F1870 */
};

extern unsigned char OurGameName;

extern CReco theReco;

/* the lens flares' system (zrndr_pipe.cpp's, after thePipe: only its tick) */
class CFlareSystem {
public:
    void Tick();   /* 0x336230 */
};

extern CFlareSystem data_00488F10;

extern "C" {
extern int data_00408C08;                        /* SOCOM 1's oddeven, a static of Tick's */
void func_003B1160(int arg);
void func_0033B5B0(CPipe *pipe);
void func_0033B570(CPipe *pipe);
CTickItem *func_0022D590();
}
void zVid_Swap(bool flip, bool restore);   /* 0x3AFF30 */

static void LoadThreadEntry(void *arg);

/* MSL 3.0's basic_string members that close the unit in its template tail (OVL1 0x1EEB40,
 * 0x1EEB70, 0x1EF390; the template replace taking two iterators and their category, 0x1EE780, and
 * the assign(first, last) it calls for its temporary, 0x1EF400, are include/string's, emitted by
 * every unit that includes <string>): out-of-class template members, over a copy of include/
 * string's representation -- fts_state.h keeps its own std::basic_string, so <string> cannot come
 * in. assign, append and replace(pos, n1, s) are fts_state.h's stand-in's members under the map's
 * names (the states lane, round 21). The stand-in that copied assign(first, last) under its
 * placeholder is gone (round 34: nothing here called it, and its row carries the member
 * template's name since the MSL headers' round 21). Pinned, or one would take another in (the
 * same-unit stand-in rule) */
namespace {
/* include/string's twelve bytes: the long form's capacity | 1, size and text, the short form's
 * size in the first byte's upper seven bits */
struct CTailString {
    bool is_long() const { return w_[0] & 1; }
    unsigned int size() const { return is_long() ? w_[1] : (unsigned char)s_.size_; }

    struct short_rep {
        unsigned char is_long_ : 1;
        unsigned char size_ : 7;
        char data_[11];
    };
    union {
        unsigned int w_[3];
        short_rep s_;
    };
};
}

#pragma auto_inline off
namespace std {
/* replace(pos, n1, const char *): the range s..s+strlen(s) handed to the template replace
 * (0x1EF390) */
template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator> &basic_string<charT, traits, Allocator>::replace(unsigned int pos,
                                                                                    unsigned int n1, const charT *s)
{
    forward_iterator_tag category;

    return replace(pos, n1, s, s + strlen(s), category);
}

/* append(const char *): replace(size(), 0, s), a tail call (0x1EEB70) */
template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator> &basic_string<charT, traits, Allocator>::append(const charT *s)
{
    return replace(((CTailString *)this)->size(), 0, s);
}

/* assign(const char *): replace(0, size(), s), a tail call (0x1EEB40) */
template <class charT, class traits, class Allocator>
basic_string<charT, traits, Allocator> &basic_string<charT, traits, Allocator>::assign(const charT *s)
{
    return replace(0, ((CTailString *)this)->size(), s);
}
}
#pragma auto_inline reset

/* zsys.h's FreeVector (inline, taller than the budget): UnInit reaches it through FreeCues, an
 * inline helper MW inlines one level, so it is called and its out-of-line copy follows UnInit with
 * clear()'s (OVL1 0x1ECF60, 0x1ED000: reverse of their first use) -- the vector's constructor and
 * swap inlined whole, its clear (0x1EF310) and its temporary's deleter destructor (0x1EF320, with
 * 0) called, as a class vector's out-of-class members are in a unit's own code. Generated with
 * the unit's own code, it asks for the deleter's clear and destructor in the tail's first round,
 * ahead of the cue vector's insert and reserve, which then inline both in their temporary's
 * teardown (0x1EF190 byte-exact; as a plain template instance FreeVector is generated in the tail,
 * and reserve and insert called the destructor) */
template <class V>
inline void FreeVector(V &v)
{
    V tmp;

    v.clear();
    v.swap(tmp);
}

/* a film's cue (8 bytes: the vector's reserve copies a float and a word; built by 0x1EDEC0, the
 * float first): the time it is shown AT and the text it PRINTs (the subtitles rdr's tags; the
 * names ours) */
struct CMovieCue {
    CMovieCue(float time, char *text) : m_time(time), m_text(text) {}

    float m_time;
    char *m_text;
};

/* the film's cues (a class vector of them). OPEN, 49 instructions: its insert (OVL1 0x1EED40, the
 * tail's, from push_back's copy) is include/vector's insert to the register -- the in-place
 * branch's end pointer in v1 where the target has a2 (include/vector's open item, the 8-byte
 * element inserts); its temporary's teardown is the target's since FreeVector became own code */
typedef std::vector<CMovieCue> CMovieCueVector;

/* the cues in time order (std::sort's comparator, the name ours): SetMovie's is a default-
 * initialised local -- no store, no constructor call, its byte loaded as it is (0x1EE000's
 * instance and its two helpers, byte-exact) */
struct CueLess {
    bool operator()(CMovieCue a, CMovieCue b) const { return a.m_time < b.m_time; }
};

/* the subtitles (st_core.cpp's BSS, 0x408BD0-0x408BFF; the names ours): the film's cues -- the
 * unit's one dynamically initialised static after <iostream>'s two, so its initialiser
 * (0x3FD7B0) builds those, then zeroes the vector and registers its destructor -- then how they
 * are drawn: a scale, a position, the justification (0 left, 1 centre, 2 right), a colour, the
 * spacing and the width.
 * OPEN, 0x60 against 0x80: the registered destructor (0x1EF8C0, matched by its draft under an sc: as= line) is
 * the tail's last function in the target, generated after the deleter's clear and destructor, so it
 * inlines them whole (four this-tests); this unit's copy is generated earlier, with FreeVector's
 * temporary (whose teardown inlines it one level and calls the deleter's destructor, so its out-of-
 * line copy is emitted then, the deleter's call in it) -- no ~vector or ~__vector_imp copy follows
 * FreeVector in the target, so the original's FreeVector requested none */
static CMovieCueVector s_cues;
static float s_subScale;
static PNT2D s_subPos;
static int s_subJustify;
static PNT3D s_subColor;
static int s_subSpacing;
static int s_subWidth;

/* the film's first frame to come (OVL1 bss 0x408C00, the name ours): Init sets it, Tick's first
 * swap is made without the flip and clears it */
static unsigned char s_firstFrame;

/* the cues let go of (the name ours): an inline helper MW inlines one level (FreeVector makes it
 * taller than the budget), so UnInit calls clear() and FreeVector out of line, as the image does */
static inline void FreeCues()
{
    s_cues.clear();
    FreeVector(s_cues);
}

/* the load thread's stack (SOCOM 1's static array of it; SOCOM II's allocated once) */
static void *load_thread_stack;

/* the film's name let go of */
CCinematicState::~CCinematicState()
{
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
}

/* the subtitles read for a film: the defaults, then the rdr's settings and the film's own over
 * them (the second justification test is not an else-if, as the target tests it), its file and
 * its cues in time order (the name ours). An inline helper too big for MW's inline_max_size, so
 * SetMovie inlines it ONE level: its body comes in and every accessor it calls stays a call --
 * _zrdr's Count and Item, the cue's constructor, the vector's reserve forwarder, push_back, begin
 * and end -- their out-of-line copies following SetMovie in reverse of their first use (OVL1
 * 0x1EDE70-0x1EDF3C; Count and Item are the overlay's linked copies, which the zrdr units' walkers
 * call too). The sort's iterators are named locals built in place, the comparator declared after
 * them (its slot below theirs) */
static inline const char *ReadSubtitles(const char *name)
{
    char *file = 0;

    s_subJustify = 0;
    _zrdr *rdr = theMission.m_subtitles;
    s_subScale = 1.0f;
    s_subPos.x = 20.0f;
    s_subPos.y = 300.0f;
    s_subColor.x = s_subColor.y = s_subColor.z = 255.0f;
    s_subSpacing = 20;
    s_subWidth = zVid.m_44;
    if (rdr) {
        zrdr_findreal(rdr, "SCALE", &s_subScale, 1);
        zrdr_findPNT2D(rdr, "POS", &s_subPos);
        zrdr_findPNT3D(rdr, "COLOR", &s_subColor);
        zrdr_findint(rdr, "SPACING", &s_subSpacing, 1);
        zrdr_findint(rdr, "WIDTH", &s_subWidth, 1);
        if (zrdr_findtag(rdr, "LEFT_JUSTIFIED"))
            s_subJustify = 0;
        else if (zrdr_findtag(rdr, "CENTER_JUSTIFIED"))
            s_subJustify = 1;
        else if (zrdr_findtag(rdr, "RIGHT_JUSTIFIED"))
            s_subJustify = 2;
        _zrdr *movie = zrdr_findtag(rdr, name);
        if (movie) {
            file = zrdr_findstring(movie, "FILENAME");
            zrdr_findreal(movie, "SCALE", &s_subScale, 1);
            zrdr_findPNT2D(movie, "POS", &s_subPos);
            zrdr_findPNT3D(movie, "COLOR", &s_subColor);
            zrdr_findint(movie, "SPACING", &s_subSpacing, 1);
            zrdr_findint(movie, "WIDTH", &s_subWidth, 1);
            if (zrdr_findtag(movie, "LEFT_JUSTIFIED"))
                s_subJustify = 0;
            if (zrdr_findtag(movie, "CENTER_JUSTIFIED"))
                s_subJustify = 1;
            else if (zrdr_findtag(movie, "RIGHT_JUSTIFIED"))
                s_subJustify = 2;
            _zrdr *subs = zrdr_findtag(movie, "SUBTITLES");
            if (subs) {
                int n = subs->Count();

                s_cues.reserve(n);
                for (int i = 0; i < n; i++) {
                    float at = -1.0f;
                    _zrdr *cue = subs->Item(i);

                    zrdr_findreal(cue, "AT", &at, 1);
                    s_cues.push_back(CMovieCue(at, zrdr_findstring(cue, "PRINT")));
                }
                CMovieCueVector::iterator first = s_cues.begin();
                CMovieCueVector::iterator last = s_cues.end();
                CueLess less;

                std::sort(first, last, less);
            }
        }
    }
    return file;
}

/* the film to play next (OVL1 0x1EDA60): with subtitles, the film's file comes from the subtitles'
 * rdr, "INVALID" when it names none (the null test of the helper's result as written: `name = f;
 * if (!name)` puts the copy in the branch's delay slot, a ternary branches twice) */
void CCinematicState::SetMovie(const char *name, unsigned char flag, unsigned char subtitles)
{
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
    if (subtitles) {
        name = ReadSubtitles(name);
        if (!name)
            name = "INVALID";
    }
    m_moviename = _strdup(name, 0, 0);
    m_28 = flag;
}

/* a subtitle line centred, or set flush right (ours: C2DString's flags at 0x69 and 0x6A set through
 * an inline -- each a call whose object, the line reloaded, is numbered as the inline's variable,
 * after the store's value: written in place, the reloads take v0 and the values v1) */
static inline void SetCentered(C2DString *s, bool on)
{
    s->m_isCentered = on;
}

static inline void SetRight(C2DString *s, bool on)
{
    s->m_socom2_6a = on;
}

/* the film started: the sounds paused, the CD read let finish and the stream left idle for half a
 * second; with the film up the frame rate saved, the screen set up for it (the full height halved
 * for an interlaced mode, a window kept or the whole screen), path 3 masked, the bone motions'
 * buffer lent to the film's GS memory and the player given it, and -- with subtitles -- the film's
 * texture buffer loaded with the HUD's font and the three lines built; then the film run */
bool CCinematicState::Init()
{
    CSndInstance::PauseAllSounds();
    CSnd::WaitForCDRead();
    Headset *headset = Headset::ftsGetHeadset();

    if (headset)
        Flush__7HeadsetFb(headset, true);
    *T0_COUNT = 0;
    unsigned int start = 0;
    unsigned int now = start;

    while (now < start + zSys.timerTicksPerSecond / 2) {
        if (!snd_StreamSafeCheckCDIdle(1))
            now = start = *T0_COUNT;
        else
            now = *T0_COUNT;
    }
    m_movierunning = false;
    s_firstFrame = 1;
    m_time = 0.0f;
    if (m_2a) {
        m_savedFrameRate = CVideo::MaxFrameRate(-1);
        if (m_29) {
            if (zVid.m_75 == 2) {
                int h = zVid.m_48;

                func_003B0B90(0, 0, zVid.m_44, 224);
                func_003B09E0(0, 0, zVid.m_44, h);
            } else {
                m_1c = zVid.m_48;
                if (zVid.m_48 == 448) {
                    m_videoSave[0] = zVid.m_30;
                    m_videoSave[1] = zVid.m_34;
                    if (zVid.m_30 == 0) {
                        zVid.m_30 = 0;
                        zVid.m_34 = 70;
                    } else {
                        zVid.m_34 = 0;
                        zVid.m_30 = 70;
                    }
                }
            }
            zVid.m_7c = 2;
        } else {
            zVid.m_7c = 1;
            m_20 = zVid.m_92;
            zVid.m_92 = m_3d;
            if (m_3c) {
                func_003B0B10(m_40);
                func_003B0A70(m_50);
            } else {
                func_003B0B90(0, 0, 640, 448);
                func_003B09E0(0, 0, 640, 448);
            }
        }
        *GIF_MODE &= ~4;
        asm volatile {
            sync
        }
        u_long128 *buf = zBoneMotionRdrBuffer();
        zBoneMotionRdrSize();
        CVideo::WriteGsMemory(0x2400, buf, 0x1c0000);
        unsigned int height = m_29 ? (unsigned int)zVid.m_48 / 2 : zVid.m_48;

        CMPEG::SetMaxHeight(m_2a ? height / 2 : height);
        CMPEG::Init(buf, 0x1c0000);
        zVid.m_48 = height;
        if (s_cues.size()) {
            zdb::CGSTexBuffer *texbuf =
                zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[54]];
            CHUD *hud = theHUD;
            C2DFont *font = hud->m_myfont;

            texbuf->Setup(zVid.m_38 * 32, 0x2440);
            for (int i = 0; i < 1; i++) {
                texbuf->m_dyntex_list.Add(font->GetTexture(i), true);
                texbuf->m_chainp = 0;
            }
            texbuf->Load(true);
            for (int i = 0; i < 3; i++) {
                m_items[i] = new C2DString;
                int y;
                int off = i * s_subSpacing;
                y = (int)s_subPos.y;
                m_items[i]->Load("", font, (int)s_subPos.x, y - off, true);
                m_items[i]->m_scale = s_subScale;
                SetCentered(m_items[i], s_subJustify == 1);
                SetRight(m_items[i], s_subJustify == 2);
                m_items[i]->SetColor(s_subColor.x, s_subColor.y, s_subColor.z);
                m_items[i]->On();
            }
            zdb::CVisual::AlphaEnable(true);
        }
    }
    if (m_moviename)
        m_movierunning = CMPEG::Run(m_moviename, true, false, m_2c) != 0;
    m_fadeTime = -1.0f;
    func_003B0660();
    return true;
}

/* a word the film's tick zeroes each frame (OVL1 0x49E9E8, among C2D's statics; not named) */
extern int data_0049E9E8;

/* a frame of the film: the cross button fades it out, the fade run down (the film ended when it is
 * out and the film does not hold its last frame), the film ended and the state popped when it is
 * done -- the one return after the if/else; else its time counted, the film ticked, and the
 * subtitle line due drawn: the latest cue at or before the film's time, split at the first space
 * past its middle when it is wider than the subtitles' width (its second half on the line, its first
 * on the next, drawn in the same pass). The tells: the cue's index `line` is a counter of its own,
 * stepped in the for's increment (MW strength-reduces &lines[line] to s3, `addiu s3,s3,4` in the
 * latch; indexed by i -- which the split's i++ also steps -- it is recomputed); the line read through
 * a reference bound before the null test (s8, computed in the test's delay slot) for the split's
 * text and its SetString, the rest written m_items[i]; `more` set before the next line's SetString;
 * the film's time read into a float ahead of the cue array (hoisted out of the cue walk); `half`
 * named (the loop's bound before its counter's zero) */
bool CCinematicState::Tick(float dT)
{
    CInput::Tick(dT);
    CPad *pad = CInput::m_pads[0];

    if (pad && pad->m_state[9] == 3 && m_fadeTime < 0.0f)   /* PAD_CROSS, KEY_RISING */
        m_fadeTime = 1.1f;
    if (m_fadeTime > 0.0f) {
        m_fadeTime -= dT;
        if (m_fadeTime < 0.1f || !m_28) {
            CVideo::Fade(0.0f, 0, 0, 0);
            if (m_fadeTime < 0.0f || !m_28) {
                m_fadeTime = -1.0f;
                m_movierunning = false;
            }
        } else {
            CVideo::Fade(m_fadeTime - 0.1f, 0, 0, 0);
        }
    }
    if (!m_movierunning || CMPEG::Done()) {
        CMPEG::End();
        m_movierunning = false;
        theGame.Pop(0);
    } else {
        m_time += dT;
        CMPEG::Tick(zVid.m_34);
        data_0049E9E8 = 0;
        int line = 0;

        for (int i = 0; i < 1; i++, line++) {
            C2DString *&cur = m_items[i];

            if (cur) {
                bool more = false;
                float t = m_time;
                const char *lines[3] = {"", "", ""};

                for (CMovieCueVector::iterator c = s_cues.begin(); c != s_cues.end(); c++) {
                    if (c->m_time <= t) {
                        lines[2] = lines[1];
                        lines[1] = lines[0];
                        lines[0] = c->m_text;
                    }
                }
                m_items[i]->SetString(lines[line], true);
                m_items[i]->UpdatePos();
                if (m_items[i]->m_length > s_subWidth) {
                    char first[800];
                    char rest[800];

                    memset(first, 0, 800);
                    char *text = cur->m_string + cur->m_firstCharPos;
                    int len = strlen(text);
                    int half = len / 2;
                    int j;

                    for (j = 0; j < half; j++)
                        first[j] = text[j];
                    for (; j < len && text[j] != ' '; j++)
                        first[j] = text[j];
                    for (; j < len && text[j] == ' '; j++)
                        ;
                    sprintf(rest, "%s", text + j);
                    cur->SetString(rest, true);
                    if (i < 2) {
                        more = true;
                        m_items[i + 1]->SetString(first, true);
                    }
                }
                m_items[i]->Tick(dT);
                m_items[i]->Draw(appCamera->m_camera);
                if (more) {
                    i++;
                    m_items[i]->Tick(dT);
                    m_items[i]->Draw(appCamera->m_camera);
                }
            }
        }
        zSysFifoEnd();
        zVid_Swap(!s_firstFrame, true);
        s_firstFrame = 0;
    }
    return false;
}

/* the film let go of: the film player, its items, its texture buffer and list, and -- if it was
 * up -- the GS memory it borrowed restored, the frame rate, the screen's mode or clear, path 3;
 * the film's name, the fade, the player's controller told */
void CCinematicState::UnInit()
{
    CMPEG::Uninit();
    zdb::CVisual::AlphaEnable(false);
    for (int i = 0; i < 3; i++) {
        if (m_items[i]) {
            delete m_items[i];
            m_items[i] = 0;
        }
    }
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[54]]->WipePktBuf();
    FreeCues();
    if (m_2a) {
        zSysFifoEnd();
        zSysFifoDisable();
        CVideo::ReadGsMemory(0x2400, zBoneMotionRdrBuffer(), 0x1c0000);
        CVideo::MaxFrameRate(m_savedFrameRate);
        if (m_29) {
            if (zVid.m_75 == 2) {
                func_003B0B90(0, 0, zVid.m_44, zVid.m_48);
                func_003B09E0(0, 0, zVid.m_44, zVid.m_48);
            } else {
                if (m_1c != zVid.m_48) {
                    zVid.m_30 = 0;
                    zVid.m_34 = m_videoSave[0] > m_videoSave[1] ? m_videoSave[0] : m_videoSave[1];
                    zVid.m_48 = m_1c;
                }
                zVid.m_90 = 1;
            }
        } else {
            zVid.m_92 = m_20 != 0;
        }
        zVid.m_7c = 0;
        *(volatile unsigned int *)0x10003010 |= 4;
        asm volatile {
            sync
        }
    }
    if (m_moviename)
        _free(m_moviename, 0, 0);
    m_moviename = 0;
    CVideo::Fade(1.0f, 0, 0, 0);
    CZSealBody *player = ftsGetPlayer();
    void *control = player ? player->m_control : 0;

    if (control)
        func_00592CA0(control);
    CSndInstance::ResumeAllSounds();
}

/* a message for the player: the sender's name and the text built into a string (and not used),
 * the text queued on the mission's first message queue with the message's type (OVL1 0x1ECC40,
 * slot 0x2C) */
void CCoreState::DisplayMessage(const char *text, const char *who, int a, int b, int c, unsigned char d,
                                unsigned char e, MSGTYPE type)
{
    CSaferStr s;

    s = who;
    s += ": ";
    s += text;
    func_002B6530(&theMission.m_messages, text, type, 0, 0, 0.0f);
}

/* a pad's actuators stopped (the first pad's) */
void CCoreState::OnPush(int userdata)
{
    zSysFifoEnd();
    sceGsSyncPath(0, 0);
    CPad *pad = CInput::m_pads[0];

    pad->m_ActData[1] = 0;
    pad->m_ActData[0] = 0;
}

/* back on top: the two texture phases' buffers loaded again, and from a sub-state (1) the mission
 * reopened and restarted, the camera's zoom reset onto the player, the HUD set up again */
void CCoreState::OnPop(int userdata)
{
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[36]]->Load(true);
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[40]]->Load(true);
    zVid_Sync0();
    theMission.m_108 = 10.0f;
    if (userdata == 1) {
        CameraZoom::SetGoal(1.0f);
        theMission.ReOpen_Load();
        theMission.OnMissionStart();
        CameraZoom::AttachSeal(ftsGetPlayer());
        m_hud->ReInit_Init();
    }
    zVid_Sync0();
    m_skip_swap = 2;
}

CCoreState::CCoreState() : CGameState("CoreState")
{
    m_loading = false;
    m_loadstarted = false;
    m_skip_swap = 0;
    m_c0 = 1.0f;
    m_c4 = 1.0f / m_c0;
    m_c8 = m_c0;
    m_LoadThreadId = -1;
}

/* the base's destructor called in the body, so it runs twice: SOCOM 1's and SOCOM II's both inline
 * CGameState's teardown (vtable, name freed) two times over */
CCoreState::~CCoreState()
{
    CGameState::~CGameState();
}

/* the asset manager's handle for a texture by name (out of line: every caller calls it) */
#pragma auto_inline off
zdb::CTexHandle *zdb::CWorld::GetTexHandle(const char *name)
{
    return CAssetMgr::m_assets.GetTexHandle(name);
}

#pragma auto_inline reset

/* the mission set up: the world and its camera, the HUD, the player under the camera, the
 * reader archives, the tick tasks, the camera's far plane, room for 300 more nodes, the texture
 * buffers, the mission started, the pipe's noise texture, the GIF's path 3 opened, the frame rate
 * back to the default */
bool CCoreState::Init()
{
    m_world = zdb::CWorld::m_world;
    zdb::CVisual::m_camera = thePipe.m_camera = m_world->m_camera;
    CHUD *hud = new CHUD;

    theHUD = m_hud = hud;
    if (gopt[0]) {
        CHUD &h = *m_hud;

        h.Init(m_world);
        CameraZoom::AttachHud(&h);
    }
    CameraZoom::AttachSeal(ftsGetPlayer());
    CRdrArchive::CloseAll();
    CRdrArchive::RemoveArchive("readerm.zar", GetDatabase());
    zTaskScheduler.AddTask("UnitTick", CSealUnit::TickAll, 0.8f, 0);
    zTaskScheduler.AddTask("ai_pre_tick", CSealCtrlAi::PreTick, 0.9f, 0);
    zTaskScheduler.AddTask("entity_pre_tick", CEntity::PreTickAll, 1.0f, 0);
    zTaskScheduler.AddTask("weapon_pre_tick", func_003D1F90, 3.0f, 0);
    zTaskScheduler.AddTask("CAiEventTick", CAiEvent::Tick, 4.5f, 0);
    zTaskScheduler.AddTask("entity_tick", CEntity::TickAll, 4.6f, 0);
    zTaskScheduler.AddTask("body_collide_tick", func_0054F570, 4.65f, 0);
    zTaskScheduler.AddTask("entity_preposttick", CEntity::PrePostTickAll, 4.7f, 0);
    zTaskScheduler.AddTask("target_pretick", func_0060BC40, 4.8f, 0);
    zTaskScheduler.AddTask("entity_post_tick", CEntity::PostTickAll, 6.0f, 0);
    zTaskScheduler.AddTask("weapon_post_tick", CZWeapon::PostTick, 8.0f, 0);
    zTaskScheduler.AddTask("ai_route_planner", ai::TickPathPlanner, 10.5f, 0);
    zTaskScheduler.AddTask("target_post_tick", func_0060B8F0, 10.6f, 0);
    zTaskScheduler.AddTask("ai_posttick", func_005CCA30, 10.7f, 0);
    zTaskScheduler.AddTask("sound_post_tick", CSnd::PostTick, 11.0f, 0);
    zTaskScheduler.AddTask("appCamera_tick", CAppCamera::TickTask, 12.0f, 0);
    appCamera->m_camera->GetLandmarkFarPlane(m_world);
    std::vector<zdb::CNode *> &nodes = m_world->m_nodes;
    unsigned int n = nodes.size() + 300;

    if (nodes.capacity() < n)
        nodes.reserve(n);
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[33]]->ReservePktBuf(2048);
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[34]]->ReservePktBuf(2048);
    m_world->ReleaseTextureAssetCharacters();
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[36]]->Load(true);
    zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[40]]->Load(true);
    theMission.OnMissionStart();
    theMission.m_e8 = -1.0f;
    thePipe.SetOverlayTexture(zdb::CWorld::GetTexHandle("noise50.tif"));
    *(volatile unsigned int *)0x10003010 |= 4;
    zSysFifoEnd();
    zVid.m_91 = 0;
    CVideo::MaxFrameRate(CVideo::m_defaultFrameRate);
    zVid_Sync0();
    return true;
}

/* a frame of the mission. While the mission is not yet running (its m_18 at 1, its flag at 0xFC,
 * or online) the fade-in waits; otherwise it counts down and, done, starts the mission. Input;
 * while running the voice, the HUD's lens flare, the tasks (unless paused) and -- for SOCOM 2 --
 * the zoom; the frame swapped (or not, while swaps are skipped); the camera updated; the world
 * drawn with the camera's own entity hidden in first person or while its controller holds the
 * zoom; the HUD drawn between texture loads (with the option), its compass, flashes and items.
 * SOCOM 1's Tick without its benchmark timers (line order kept) */
bool CCoreState::Tick(float dT)
{
    bool running;
    bool hide;
    CTickNode *pEntNode;
    bool camera_entity_active;
    zdb::CCamera *cam;

    cam = thePipe.m_camera;
    running = true;
    if (theMission.m_18 != 1 && !(theMission.m_fc & 1) && !theNetwork.m_9) {
        m_c8 -= dT;
        float fade = m_c8 * m_c4;

        if (fade < 0.0f) {
            fade = 1.0f;
            theMission.EndMission();
        }
        CVideo::Fade(fade, 0, 0, 0);
        running = false;
    } else {
        m_c8 = m_c0;
    }
    CInput::Tick(dT);
    if (running) {
        theReco.Tick(dT);
        m_hud->PreDITick(dT);
        if (!theReco.m_running && !m_hud->PauseGame())
            zTaskScheduler.Tick(dT);
        if (OurGameName == 1)
            CameraZoom::Tick(dT);
    }
    zSysFifoEnd();
    if (m_skip_swap > 0) {
        m_skip_swap--;
        zVid_Swap(false, true);
        zVid.m_90 = 0;
    } else {
        zVid_Swap(true, true);
    }
    data_00488F10.Tick();
    cam->Update(zdb::ZCAM_TYPE_0);
    m_hud->PostDITick(dT);
    if (running && !m_hud->PauseGame())
        zdb::theWaterRippleManager.Tick(dT);
    func_003B1160(1);
    hide = appCamera->m_camera_mode == 3;
    pEntNode = appCamera->m_bc->GetNode();
    CZSealBody *player = ftsGetPlayer();
    void *control = player ? player->m_control : 0;

    if (!hide && control && ((unsigned char *)control)[0x221] == 5)
        hide = true;
    if (hide) {
        camera_entity_active = pEntNode->m_active;
        pEntNode->SetActive(false);
    }
    data_00408C08 = thePipe.RenderWorld(m_world);
    if (hide)
        pEntNode->SetActive(camera_entity_active);
    if (running && !m_hud->PauseGame()) {
        thePipe.m_effect.Tick();
        thePipe.RandomizeOverlay();
    }
    func_0033B5B0(&thePipe);
    func_0033B570(&thePipe);
    thePipe.KickOverlay();
    if (gopt[0]) {
        if (running)
            m_hud->Tick(dT);
        zdb::CVisual::AlphaEnable(true);
        thePipe.m_assetHUD2 =
            zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[39 + data_00408C08]];
        thePipe.FIFO_Load(thePipe.m_assetHUD2);
        thePipe.FIFO_Signal(thePipe.m_assetHUD1);
        m_hud->DrawPhase(zdb::RENDER_PHASE_12, cam);
        thePipe.m_assetHUDX = zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[42]];
        thePipe.FIFO_Load(thePipe.m_assetHUDX);
        thePipe.FIFO_Signal(thePipe.m_assetHUD2);
        m_hud->DrawPhase(zdb::RENDER_PHASE_13, cam);
        thePipe.FIFO_Load(zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[0]]);
        thePipe.FIFO_Signal(thePipe.m_assetHUDX);
        m_hud->DrawPhase(zdb::RENDER_PHASE_14, cam);
        m_hud->m_compass.Draw(cam);
        if (!theReco.m_running && !m_hud->PauseGame())
            m_hud->m_flashfx.Tick(dT);
        m_hud->m_flashfx.Draw(cam);
        m_hud->m_18900.Draw(cam);
        func_0022D590()->Draw(cam);
        zdb::CVisual::AlphaEnable(false);
    } else {
        thePipe.FIFO_Load(zdb::CTexManager::m_texmanager.m_buffer.data_[zdb::CTexManager::TexPhase[0]]);
    }
    if (!theNetwork.m_9 && theGame.m_6c)
        CVideo::Clear(true, 0, 0, 0, 0);
    return false;
}

/* torn down: the GIF's path 3 masked again, the frame rate freed, the mission unloaded, the
 * valves reset and the scheduler's free list cleared */
void CCoreState::UnInit()
{
    *(volatile unsigned int *)0x10003010 &= ~4;
    asm volatile {
        sync
    }
    CVideo::MaxFrameRate(-1);
    CVideo::Fade(1.0f, 0, 0, 0);
    zVid.m_91 = 1;
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
    CValve::Reset();
    zTaskScheduler.ClearFreeList(true);
    func_005B61A0();
}

/* the load thread: the sky's colour, the texture buffers, the load itself (exiting when it
 * fails), an online game's exit screens, then the mission's own */
static void LoadThreadEntry(void *arg)
{
    theGame.m_coreState.skyColor[0] = 190;
    theGame.m_coreState.skyColor[1] = 220;
    theGame.m_coreState.skyColor[2] = 250;
    zdb::CTexManager::m_texmanager.doSetupBuffers();
    if (!testLoad())
        exit(0);
    if (theNetwork.m_9)
        theGame.m_MPexitState.LoadLibs();
    theGame.m_coreState.m_loading = false;
    func_002A6950(&theMission);
    ExitDeleteThread();
}

/* the load thread started on a stack of its own (allocated once, its ends marked). Written so
 * it is 15 statements and Load would inline it; the original's had more (Load calls it) */
#pragma auto_inline off
void CCoreState::StartLoadThread()
{
    ThreadParam param;

    if (!load_thread_stack)
        load_thread_stack = _memalign(16, 0x11000, 0, 0);
    memset(load_thread_stack, 250, 2048);
    memset((char *)load_thread_stack + 0x10800, 250, 2048);
    memset((char *)load_thread_stack + 2048, 234, 0x10000);
    param.entry = LoadThreadEntry;
    param.option = 0;
    param.stackSize = 0x10000;
    param.initPriority = 6;
    param.gpReg = _gp;
    param.stack = (char *)load_thread_stack + 2048;
    m_LoadThreadId = CreateThread(&param);
    if (m_LoadThreadId)
        StartThread(m_LoadThreadId, this);
}

#pragma auto_inline reset

/* the load thread's stack let go of (the name ours) */
void CCoreState::FreeLoadThread()
{
    m_LoadThreadId = -1;
    if (load_thread_stack)
        _free(load_thread_stack, 0, 0);
    load_thread_stack = 0;
}

/* the mission loaded: on its own thread, or here and now */
void CCoreState::Load(bool async)
{
    m_loading = true;
    m_loadstarted = true;
    zsys_FullAllocAndFree();
    if (async) {
        StartLoadThread();
        return;
    }
    skyColor[0] = 190;
    skyColor[1] = 220;
    skyColor[2] = 250;
    zdb::CTexManager::m_texmanager.doSetupBuffers();
    if (!testLoad())
        exit(0);
    if (theNetwork.m_9)
        theGame.m_MPexitState.LoadLibs();
    m_loading = false;
}

void CCoreState::Unload()
{
    m_loading = false;
    m_loadstarted = false;
    auxUnload();
}

/* the mission let go of: an online game's network, the tick tasks, the projectiles and the marks,
 * the loaded data (exiting if that fails), the world's texture buffers, the HUD, the player, the
 * camera's seal, the weapons' lists, the mission's reader archive, the scheduler's free list */
void CCoreState::auxUnload()
{
    if (theNetwork.m_9 && theMission.m_18 == 4) {
        ftsNetworkGameUninit();
        DMEDisconnectFromGameServer();
    }
    zTaskScheduler.RemoveTask("CAiEventTick", false);
    zTaskScheduler.RemoveTask("UnitTick", false);
    zTaskScheduler.RemoveTask("ai_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_posttick", false);
    zTaskScheduler.RemoveTask("entity_pre_tick", false);
    zTaskScheduler.RemoveTask("target_pretick", false);
    zTaskScheduler.RemoveTask("target_post_tick", false);
    zTaskScheduler.RemoveTask("weapon_pre_tick", false);
    zTaskScheduler.RemoveTask("ai_post_tick", false);
    zTaskScheduler.RemoveTask("ai_direg_tick", false);
    zTaskScheduler.RemoveTask("entity_post_tick", false);
    zTaskScheduler.RemoveTask("entity_tick", false);
    zTaskScheduler.RemoveTask("body_collide_tick", false);
    zTaskScheduler.RemoveTask("entity_preposttick", false);
    zTaskScheduler.RemoveTask("weapon_post_tick", false);
    zTaskScheduler.RemoveTask("ai_route_planner", false);
    zTaskScheduler.RemoveTask("sound_post_tick", false);
    zTaskScheduler.RemoveTask("light_tick", false);
    zTaskScheduler.RemoveTask("appCamera_tick", false);
    CZProjectileList *proj = CZWeapon::m_pProjectilelist;
    CZMarkList *marks = CZWeapon::m_pMarklist;

    if (proj)
        proj->UnInit();
    if (!testUnload())
        exit(0);
    if (marks)
        marks->UnInit();
    m_world = 0;
    zdb::CTexManager::DestroyBuffers(zdb::CTexManager::m_texmanager, false);
    CameraZoom::AttachHud(0);
    CHUD::ReleaseHUD(&m_hud);
    ftsSetPlayer(0);
    CameraZoom::AttachSeal(0);
    CZWeapon::DestroyProjectileList();
    CZWeapon::DestroyMarkList();
    CRdrArchive::RemoveArchive("readerm.zar", 0);
    zTaskScheduler.ClearFreeList(true);
}
