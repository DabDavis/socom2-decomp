/* zui_skbview.cpp -- the soft keyboard's shared display and text (OVL1 ~0x394420-~0x3990xx, its
 * initialiser 0x403800: four rows of ten CBmpWithNode at 0x49ED70 (0x6E0 bytes each, their
 * constructor/destructor 0x398AA0/0x398A40) and eight CBmpWithNode globals from 0x4A0A80): the text
 * being edited, the keyboards' look parser (0x3963F0) and their key props (0x38DA70's callees). SOCOM
 * 1 has none of it: the file's name and every name here are ours. Defined in reverse of their image
 * order (-inline deferred).
 *
 * The unit calls zui_skb.cpp's CBmpWithNode constructor and destructor (0x38E620, 0x38E490): SOCOM
 * II's CBmpWithNode is a shared class, not SOCOM 1's file-local one (the linker kept zui_skb.cpp's
 * copies of its inline members for both units) -- the keyboard's classes are zui_skb.h's.
 *
 * Round 11 wrote the locks and shift test (bitmaps 0 and 5, CPad's L1), the six sounds, the cursor,
 * the pair and bitmap-4 toggles, the bitmap loader, the caps' text look and the key props' spec.
 * OPEN: the tick 0x3949C0 (written round 32, 17 off: its note). The cap look reader
 * 0x397D30 (CSkbLook::ReadCapLook) is byte-exact since round 27, its POSITION pair assigned whole
 * into the cap's m_pos (zui_skb.h's CIntPair over two CInit<int, 0>: an assignment of class-typed
 * members copies each word through $f0 off an address register, plain ints copy with lw/sw).
 * Round 22: the key rows walked through CSkbBmpRows' and CSkbBmpRow's operator[]s (the nested
 * reference accessors keep MW from rotating the inner loop: entered at its bottom test through a
 * preheader after the return) -- SkbSetKeyTextures 0x395140 and SkbForgetDisplay 0x396F30 (open since
 * rounds 21 and 11), SkbShowKeyRows 0x394520, SkbReadUiParams 0x397070 and SkbPrepDisplay 0x3958D0
 * byte-exact, SkbScaling 0x396EC0 and SkbTwoEdits 0x396F10 from drafts; SkbStartDisplay 0x395540
 * written (byte-exact since round 73).
 * Round 21: the scaling step SkbScale 0x394DA0 (its state an unsigned char, tested `== 0` first: `!`
 * re-extends the byte for the next test; the t >= 1 block written as the if's body, the return 0
 * after it) and SkbLoadUiParams 0x3943E0 (from zui_hangul.cpp), byte-exact.
 * Round 20: the key props' SetShown 0x3941D0 and destructor 0x394370 (the unit's stretch opens at
 * 0x3941D0), the set-up from the open command SkbSetPurpose 0x397640 (SOCOM 1's UiSetSkbPurpose its
 * first part) and its two flags' readers 0x395110 and 0x396ED0, byte-exact.
 * Round 14 wrote the look parser 0x3963F0 (OPEN, below) with its out-of-line helpers (0x396C70,
 * 0x396C80: pinned) and the walker's constructor (zui_hangul.cpp's 0x3909F0): the template tail
 * 0x397FE0-0x398910 is the background pieces' vector (zui_skb.h's CSkbBgPiece, the parser's resize
 * of m_sources[i] its only request) -- resize 0x397FE0 and the piece's copy 0x398910 byte-exact
 * (the copy moves the rectangles word by word: CSkbRect has a user-declared copy constructor beside
 * its implicit, FPR block-copying assignment). Open in it: erase 0x3980D0 (0x150 against 0x220) and
 * insert 0x3982F0 (0x4A0 against 0x618) call std::copy's helpers out of line where the target
 * inlines the whole element assignment, the string's copy assignment included (its is_long tests,
 * then replace 0x22E690), with each rectangle a four-word block -- the element's assignment chain
 * is two levels too tall for include/vector, include/algorithm and include/string (the MSL headers'
 * round 9 note at include/vector's erase). */

/* (this file's end: THE LEVEL) */
#define __MSL_POD_DTOR_ARGS
#include <string>
#include "zui.h"
#include "zui_skb.h"
#include "../znode/znode.h"
#include "../zsnd/zsnd.h"

_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_flat_findtag_startidx(const _zrdr *rdr, const char *tag, int start);

/* an rdr array's item as an int, 0 for none (SOCOM 1's GetRdrArrayInt, zui_skb.cpp's there; inline
 * here) */
static inline int GetRdrArrayInt(_zrdr *rdr, int i)
{
    _zrdr *item = rdr ? rdr->Item(i) : 0;

    return item ? item->Integer() : 0;
}

/* ten bitmaps with their nodes: a row of the keyboards' shared keys (the name ours: its implicit
 * constructor and destructor, 0x398AA0 and 0x398A40, build and destroy the ten through zui_skb.cpp's
 * CBmpWithNode copies). The rows are walked `s_keyRows[i][j]`, through the two reference-returning
 * operator[]s (the UI's round 22): the nested accessors are what make MW enter each inner loop at its
 * bottom test through a preheader laid after the return path -- the shape of every row walk in the
 * unit (0x394520, 0x395140, 0x395540, 0x3958D0, 0x396F30, 0x397070); the members spelled out, or one
 * accessor alone, MW rotates the loop knowing j = 0 (round 21's forms, all short). It is the engine
 * core's round-11 rule: the row reference the outer operator[] returns is computed inside the inner
 * body and hoisted as an invariant, and MW lays such a preheader after the function's last block and
 * enters the loop with a `b` to it (a probe: one loop over `g[2][j]`, the row constant, rotates) */
struct CSkbBmpRow {
    CBmpWithNode &operator[](int j) { return m_bmps[j]; }

    CBmpWithNode m_bmps[10];
};   // 0x6e0 bytes

/* the four rows (the name ours: the initialiser inlines the implicit constructor, __construct_array
 * over the rows, and registers the implicit destructor 0x3989E0) */
struct CSkbBmpRows {
    CSkbBmpRow &operator[](int i) { return m_rows[i]; }

    CSkbBmpRow m_rows[4];
};   // 0x1b80 bytes

/* the keyboards' shared display (0x49ED70 on; the names ours) */
static CSkbBmpRows s_keyRows;
/* the caption's three texts: plain, and the two modes' (0x4A08F0, 0x4A0930, 0x4A09B0) */
static char s_captionText0[0x40];
static char s_captionText1[0x80];
static char s_captionText2[0x80];
/* the name of the UI variable the typed text goes to (0x4A0A30: the set-up's command copies a
 * 0x20-byte field into it) */
static char s_editText[0x20];
/* the set-up's two flags: an empty text may be entered (0x4A0A50) and a typed double quote becomes
 * an apostrophe (0x4A0A58; the names ours) */
static bool s_allowEmpty;
static bool s_noQuotes;
/* the caption's mode: 0 plain, 1 or 2 the modes 0x394840 and 0x394920 toggle (0x4A0A60) */
enum CAPTIONMODE { CAPTION_PLAIN, CAPTION_1, CAPTION_2 };
static CAPTIONMODE s_captionMode;
/* the caption's node (0x4A1118) */
static zdb::CNode *s_captionNode;
static CBmpWithNode s_bmp0;
static CBmpWithNode s_bmp1;
static CBmpWithNode s_bmp2;
static CBmpWithNode s_bmp3;
static CBmpWithNode s_bmp4;
static CBmpWithNode s_bmp5;
static CBmpWithNode s_bmp6;
static CBmpWithNode s_bmp7;
static CSkbRect s_rect;
static CRenderableString s_caption;
static CSaferStr s_str0;
static CSaferStr s_str1;
static CSaferStr s_str2;
static CSaferStr s_str3;
static CSaferStr s_str4;
static CSaferStr s_str5;
static CSaferStr s_str6;
/* the text the keyboard was opened with (0x4A1300) */
static CSaferStr s_openText;
static CSaferStr s_str8;
/* the display's root node (0x49ED50: zui_skb.cpp's 0x38D770 and 0x38D8F0 read it; the name ours) */
zdb::CNode *g_skbDisplayNode;
/* the display's six sounds, by SkbPlaySound's case (0x4A1110, 0x4A1108, 0x4A10F8, 0x4A10F0,
 * 0x4A10E8, 0x4A1100; the names ours) */
static CSnd *s_sound0;
static CSnd *s_sound1;
static CSnd *s_sound2;
static CSnd *s_sound3;
static CSnd *s_sound4;
static CSnd *s_sound5;
/* the look's limits on strings 0 and 1: bytes and characters (0x4A10C8-0x4A10E0; ours) */
static int s_maxBytes0;
static int s_maxChars0;
static int s_maxBytes1;
static int s_maxChars1;
/* the font a cap naming none is drawn in (0x4A1120: 0x3958D0 sets it; the name ours) */
static C2DFont *s_defaultFont;
/* bitmap 4's timer (0x3E04C8: -1 until SkbToggleBmp4 clears it; the name ours) */
static float s_bmp4Timer = -1.0f;
/* the key event the pad gave and how long it has been held (0x4A1070, 0x3978C0 returns it;
 * 0x4A10B8, the tick's repeat timer; the names ours) */
static signed char s_keyEvent;
static float s_keyHeld;
/* the keyboard's scaling: up as it opens (0x395540 starts it), down as it closes (0x394420), and
 * the time it has run (0x4A1078, an unsigned char -- a byte enum adds an `andi 0xff` at the scaling
 * step's `== SKBSCALE_UP`; 0x4A10C0; the names ours) */
enum SKBSCALE { SKBSCALE_NONE, SKBSCALE_UP, SKBSCALE_DOWN };
static unsigned char s_scaling;
static float s_scaleTime;
/* the cursor's blink: the tick steps it, wrapping at a second, the cursor bitmap (1) shown for its
 * first half (0x4A1090; the name ours) */
static float s_blinkTime;
/* the key repeat (UiParams.rdr's RepeatDelay, RepeatSpeedStart, RepeatAccelTime, RepeatSpeedEnd:
 * 0x4A10A0, 0x4A10B0, 0x4A1098, 0x4A10A8) and the legend's slide times (SlideInSec, SlideOutSec:
 * 0x4A1080, 0x4A1088), read by 0x397070 (the names ours) */
static float s_repeatDelay;
static float s_repeatSpeedStart;
static float s_repeatAccelTime;
static float s_repeatSpeedEnd;
static float s_slideInSec;
static float s_slideOutSec;

/* the valves (zanim.h has CValve whole: the scaling's done valve here) */
enum VALVE_STATE { VALVE_SAME, VALVE_ON, VALVE_OFF };
class CValve {
public:
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);
    /* (zanim.h's inline) */
    bool SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME) {
            MakeCallbacks(state);
            return true;
        }
        return false;
    }

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
};

/* which way the menus' stick is pushed (zui.cpp's STICK_DIR: a byte enum) */
enum STICK_DIR { STICK_NONE, STICK_RIGHT, STICK_LEFT, STICK_UP, STICK_DOWN, STICK_CENTERED };
STICK_DIR UiGetDirStickMoved();

/* a key event (ours: a byte enum -- SkbHandleKey's 1-4 the cursor moves, 5 the cursor's key, 6
 * and 7 the edit keys) */
enum SKBKEY { SKBKEY_NONE, SKBKEY_1, SKBKEY_2, SKBKEY_3, SKBKEY_4, SKBKEY_5, SKBKEY_6, SKBKEY_7 };

/* the four cursor events by the stick's direction or the pad button pressed (0x3FB388; ours: the
 * event a SKBKEY field -- MW loads a byte enum with `lb` and narrows it at each read, the table's
 * load in SkbPadKeyEvent) */
struct SKBPADKEY {
    unsigned char m_dir;     // a STICK_DIR
    unsigned char m_button;  // a PAD_BUTTON
    SKBKEY m_event;
};
static SKBPADKEY s_padKeys[4] = {{1, 3, SKBKEY_2}, {2, 2, SKBKEY_3}, {3, 4, SKBKEY_1}, {4, 5, SKBKEY_4}};

/* the pad's key event: a cursor move by the stick or the pad, else cross, L2 or R2 (5, 6, 7) --
 * kept, its hold time restarted; with none, the kept event cleared once the stick is centred or
 * one of those keys is let go (0x394F60; the name ours). The release test is a bool accumulated
 * with `|=` (the target's branch-free sltu/or chain; `||` branches, `|` of the compares differs).
 * Byte-exact since the UI's round 24 (open since round 19, 16 bytes short): the table's event a
 * SKBKEY FIELD, read without a cast -- MW loads byte-enum memory with `lb` (sign-extending) and
 * narrows with `andi 0xff` at each read, the target's lb and andi's; a signed char field cast to
 * SKBKEY folds the conversion into an `lbu` in every form (`(SKBKEY)(signed char)`, `(SKBKEY)(int)`,
 * a char or int local first, an inline returning it). The tests are written `== SKBKEY_NONE` /
 * `!= SKBKEY_NONE`: `!event` / `if (event)` test the loaded byte directly (16 bytes short). Measured
 * on the way: a 2-byte enum loads a signed char field with `lb` and narrows with `andi 0xffff`, a
 * signed byte enum (an enumerator -1) sign-extends at each read; a signed char local tested
 * through casts has every instruction but the event in a3 (only a byte-enum local is coalesced
 * with the return register); the search as an inline returning the event is 24-29 off. */
SKBKEY SkbPadKeyEvent(CPad *pad)
{
    STICK_DIR dir = UiGetDirStickMoved();
    SKBKEY event = SKBKEY_NONE;

    for (int i = 0; i < 4; i++) {
        if (pad->m_state[s_padKeys[i].m_button] == KEY_FALLING || dir == s_padKeys[i].m_dir) {
            event = s_padKeys[i].m_event;
            break;
        }
    }
    if (event == SKBKEY_NONE) {
        if (pad->m_state[PAD_CROSS] == KEY_FALLING)
            event = SKBKEY_5;
        else if (pad->m_state[PAD_TRIGGER_L2] == KEY_FALLING)
            event = SKBKEY_6;
        else if (pad->m_state[PAD_TRIGGER_R2] == KEY_FALLING)
            event = SKBKEY_7;
    }
    if (event != SKBKEY_NONE) {
        s_keyEvent = event;
        s_keyHeld = 0.0f;
        return event;
    }
    bool released = dir == STICK_CENTERED;

    released |= pad->m_state[PAD_LEFT] == KEY_RISING;
    released |= pad->m_state[PAD_RIGHT] == KEY_RISING;
    released |= pad->m_state[PAD_UP] == KEY_RISING;
    released |= pad->m_state[PAD_DOWN] == KEY_RISING;
    released |= pad->m_state[PAD_CROSS] == KEY_RISING;
    released |= pad->m_state[PAD_TRIGGER_L2] == KEY_RISING;
    released |= pad->m_state[PAD_TRIGGER_R2] == KEY_RISING;
    if (released)
        s_keyEvent = 0;
    return SKBKEY_NONE;
}

/* a look's piece list by index (ours: the look's lists read through a byte index; through a pointer
 * to the lists, so the index is added before the look -- `look->m_sources[i]` adds the look first, as
 * an inline CSkbLook member does too) */
static inline CSkbBgPieces &SkbPieces(CSkbLook *look, unsigned char i)
{
    CSkbBgPieces *s = look->m_sources.m_a;

    return s[i];
}

/* the shared key bitmaps given their textures: the two bitmaps 6 and 7 theirs, and row i's ten keys
 * list i's handles in turn, each key's UVs the part of its texture that piece i of the look's list i
 * takes (0x395140; the name ours). Byte-exact since the UI's round 22: the bitmap read `s_keyRows[i][j]`
 * (the rows' operator[]s: the inner loop entered at its bottom test, as SkbForgetDisplay's), list i a
 * reference local (the s4 stepped by 12 numbered between i and j), the row index narrowed into a byte
 * local in the outer body (its `andi`, then the piece list's own in the hoisted preheader), the piece a
 * reference local read in place -- a reference to its rectangle renumbers the address and the two
 * positions (a3/a1/a2) -- and the texture's sizes converted in each argument (named float locals take
 * $f2/$f3 the other way round, or load the width first) */
void SkbSetKeyTextures(zdb::CTexHandle *tex6, zdb::CTexHandle *tex7, std::vector<zdb::CTexHandle *> *lists,
                       CSkbLook *look)
{
    s_bmp6.m_pTexHandle = tex6;
    s_bmp7.m_pTexHandle = tex7;
    for (int i = 0; i < 4; i++) {
        std::vector<zdb::CTexHandle *> &list = lists[i];
        unsigned char k = i;

        for (int j = 0; j < 10; j++) {
            CBmpWithNode &bmp = s_keyRows[i][j];
            zdb::CTexHandle *h = j < list.size() ? list[j] : 0;

            bmp.m_pTexHandle = h;
            zdb::CTexture *tex = h ? h->m_texture : 0;

            if (tex) {
                CSkbBgPiece &piece = SkbPieces(look, k)[j];

                bmp.SetUV(piece.m_source.m_pos[0] / (float)(int)tex->m_width,
                          piece.m_source.m_pos[1] / (float)(int)tex->m_height,
                          (piece.m_source.m_pos[0] + piece.m_source.m_size[0]) / (float)(int)tex->m_width,
                          (piece.m_source.m_pos[1] + piece.m_source.m_size[1]) / (float)(int)tex->m_height);
            }
        }
    }
}

/* the display scaled to t of its size, growing out of the screen's bottom-left corner: the two
 * shared bitmaps placed and sized, then the keys of each list of ten or fewer pieces placed over
 * their pieces' display rectangles (0x397950; SkbScale's, the name ours -- it takes the keyboard
 * SkbScale leaves in a0). The lerp an inline: its 0.0f through a parameter is kept, `sub.s` from a
 * zero register (written in place MW folds `0 - x` to neg.s); the list indexed `m_sources[(Uc)i]`
 * (the look added first, not SkbPieces' hoisted base).
 * Byte-exact since round 39 (0x268; rounds 22-38 had it 3 instructions short, 0x25C). The target keeps the key's
 * offset j*176 as an induction of its own (s1, from 0) and adds the row (s2, stepped by 1760) to it each pass, where
 * every plain spelling lets MW fold the row into one key pointer (rounds 22-37: row references and pointers, flat
 * indices, accessors, a do-while, byte offsets). TWO THINGS GIVE IT. The row through the weapons' LATE-FOLDING
 * GUARD (SkbFlag, vis_ripple.cpp's RipFlag): pass 0's loop phase sees a conditional `row = ...` and reduces only
 * j*176; pass 1 folds the test away. A MODELLING DEVICE, the original's shape is not known. And the list read
 * through an inline each time (SkbList), not held in a named reference: a named one is numbered low and coloured
 * after j (s5), the inline's temporary high and coloured first (s0, the target's); spelled out without the
 * inline it is 4 bytes longer, a pointer local or a reference bound inside the size test keep the rotation */
static inline float SkbLerp(float a, float b, float t)
{
    return a + t * (b - a);
}

struct SkbFlag { int m_on; };

static inline CSkbBgPieces &SkbList(CSoftKeyboard *skb, int i)
{
    return skb->m_sources[(unsigned char)i];
}

void SkbScaleDisplay(CSoftKeyboard *skb, float t)
{
    SkbFlag f;

    f.m_on = 1;
    float x = (float)-skb->m_5c;
    float y = 448.0f - (float)skb->m_60;
    float px = SkbLerp(x, 0.0f, t);
    float py = SkbLerp(y, 0.0f, t);

    s_bmp6.m_node->SetPosition(px, py, 0.0f);
    s_bmp7.m_node->SetPosition(px, py, 0.0f);
    int w = (int)(t * (float)skb->m_6c);
    int h = (int)(t * (float)skb->m_68);

    s_bmp6.Load(0.0f, 0.0f, (float)w, (float)h, s_bmp6.m_pTexHandle);
    s_bmp7.Load(0.0f, 0.0f, (float)w, (float)h, s_bmp7.m_pTexHandle);
    for (int i = 0; i < 4; i++) {
        if ((int)SkbList(skb, i).size() < 11) {
            for (int j = 0; j < (int)SkbList(skb, i).size(); j++) {
                CSkbBmpRow *row;
                SkbFlag &rf = f;

                if (rf.m_on)
                    row = &s_keyRows.m_rows[i];
                CBmpWithNode &bmp = row->m_bmps[j];
                CSkbBgPiece &piece = SkbList(skb, i)[j];

                bmp.m_node->SetPosition(SkbLerp(x, (float)piece.m_display.m_pos[0], t),
                                        SkbLerp(y, (float)piece.m_display.m_pos[1], t), 0.0f);
                bmp.Load(0.0f, 0.0f, (float)(int)(t * (float)piece.m_display.m_size[0]),
                         (float)(int)(t * (float)piece.m_display.m_size[1]), bmp.m_pTexHandle);
            }
        }
    }
}
/* the current menu's float at 0x9C set (zui.cpp's 0x36A210) */
extern "C" void func_0036A210(float f);

/* the keyboard's scaling stepped by dT, the display scaled to it: 0 while it runs (or with none),
 * 1 once scaled up -- the cursor, the caret and the caption shown, the blink restarted -- and 2 once
 * scaled down -- the skb_done valve set (0x394DA0; SkbTick's, the name ours) */
unsigned char SkbScale(CSoftKeyboard *skb, float dT)
{
    if (s_scaling == SKBSCALE_NONE)
        return 0;
    bool up = s_scaling == SKBSCALE_UP;

    s_scaleTime += dT;
    float t = s_scaleTime / (up ? skb->m_scaleUpSec : skb->m_scaleDownSec);
    float scale = up ? t : 1.0f - t;

    if (scale > 1.0f)
        scale = 1.0f;
    SkbScaleDisplay(skb, scale);
    if (t >= 1.0f) {
        s_scaling = SKBSCALE_NONE;
        if (up) {
            if (s_bmp2.m_node)
                s_bmp2.m_node->SetActive(true);
            if (s_bmp1.m_node)
                s_bmp1.m_node->SetActive(true);
            s_captionNode->SetActive(true);
            s_blinkTime = 0.0f;
            return 1;
        }
        CValve *done = CValve::GetByName("skb_done");

        if (done)
            done->SetValue(1);
        func_0036A210(0.0f);
        return 2;
    }
    return 0;
}

/* a 2D object given a node of its own under a parent (zui.cpp's, 0x36AB20) */
extern "C" zdb::CNode *func_0036AB20(C2D *obj, zdb::CTextureRelocMgr *mgr, zdb::CNode *parent);
int UTF8CharLen(const char *&s);

/* the caption's text set (a virtual call on the static: MW does not devirtualise it) */
static inline void SetCaption(const char *text)
{
    s_caption.SetString(text, true);
}

/* the display's sounds found by the names the look gave (0x397BC0, from 0x3958D0; the name ours;
 * its one caller calls it out of line: pinned) */
#pragma auto_inline off
void SkbFindSounds()
{
    /* three names taken first: the target holds them in saved registers across the first call */
    const char *n3 = s_str3.c_str(), *n4 = s_str4.c_str(), *n5 = s_str5.c_str();

    s_sound3 = CSnd::GetSoundByName(s_str2.c_str());
    s_sound2 = CSnd::GetSoundByName(n3);
    s_sound5 = CSnd::GetSoundByName(n4);
    s_sound1 = CSnd::GetSoundByName(n5);
    s_sound0 = CSnd::GetSoundByName(s_str6.c_str());
    s_sound4 = CSnd::GetSoundByName(s_str1.c_str());
}
#pragma auto_inline reset

/* the text: the edited buffer, or the one the keyboard opened with (0x3998A0 and 0x399D20 ask) */
const char *SkbGetText(bool edited)
{
    return edited ? s_editText : s_openText.c_str();
}

/* the display unit's reader of UiParams.rdr (0x397070, below) */
void SkbReadUiParams(CRdrFile *file);

/* the keyboards' display read again from UiParams.rdr (0x3943E0; CMission::Init calls it). This
 * unit's: it lies between ~CSkbKeyProps and 0x394420, and its two strings between s_padKeys and
 * 0x394DA0's valve name in the unit's data (zui_hangul.cpp's until round 21) */
void SkbLoadUiParams()
{
    CRdrFileHolder file("UiParams.rdr", "data/common");

    SkbReadUiParams(file);
}

/* the key's spec deleted through its virtual destructor (0x394370; ours: ~CSoftKeyboard deletes
 * every key's props) */
CSkbKeyProps::~CSkbKeyProps()
{
    delete m_spec;
}

/* the key's display forgotten: its node, and its button deleted (0x394320; ours: the keyboard's
 * InvalidateDisplay calls it for every key) */
void CSkbKeyProps::InvalidateDisplay()
{
    m_node = 0;
    delete m_button;
    m_button = 0;
}

/* the key's button made by its spec and put on a node of its own under the parent, at the key's
 * place (0x394290; ours: the keyboard's PrepForDisplay calls it for every key) */
void CSkbKeyProps::PrepForDisplay(zdb::CTextureRelocMgr *mgr, C2DFont *font, zdb::CNode *parent)
{
    if (m_spec) {
        m_button = (C2DButton *)m_spec->CreateDisplayObj(font, mgr);
        m_node = func_0036AB20(m_button, mgr, parent);
        m_node->SetPosition((float)m_x, (float)m_y, 0.0f);
    }
}

/* the key's button spec replaced (0x394210; ours) */
void CSkbKeyProps::SetSpec(_zrdr *rdr)
{
    delete m_spec;
    m_spec = rdr ? new CButtonSpec(rdr) : 0;
}

/* the key's node shown or hidden, if it has one (0x3941D0; ours: CBmpWithNode's SetShown, called
 * for every key by the keyboard's ShowElements) */
void CSkbKeyProps::SetShown(bool on)
{
    if (m_node)
        m_node->SetActive(on);
}

/* the animations (zanim.h's CZAnimMain, CZAnim and CZAnimArgList: only the set-up's reach -- a
 * name by index, the first argument's string; ZAnim OVL1 bss 0x414BB0) */
class CZAnimArgList {
public:
    char *GetCharP(int index);
};

class CZAnim {
public:
    const char *GetName(int index);
    int AddName(const char *name);

    unsigned char _0[0x18];
    CZAnimArgList *m_arg_list;   // 0x18
};

class CZAnimMain {
public:
    unsigned char _0[0x38];
    CZAnim *m_CurAnim;   // 0x38: the animation being run
};

extern CZAnimMain ZAnim;

/* a caption title: the anim's name at the index (none for a negative one), the message given when
 * it does not fit the title's 0x80 bytes, the empty string for none (ours) */
static inline const char *SkbTitle(CZAnim *anim, int index, const char *tooLong)
{
    const char *t = index >= 0 ? anim->GetName(index) : 0;

    if (t) {
        if (strlen(t) >= 0x80)
            t = tooLong;
    } else
        t = "";
    return t;
}

/* the keyboard set up by its command's record: the purpose (the argument's name -- "$1" the anim's
 * first argument, as UINetCensorText reads it), the UI variable the text goes to, the two strings'
 * limits, the caption and the two titles, the two flags; the pad's kept key cleared; the name of the
 * keyboard to open returned (0x397640; ours: SOCOM 1's UiSetSkbPurpose is its first part, SkbOpen
 * calls it with the command) */
const char *SkbSetPurpose(UISkbOpenCmd *cmd)
{
    CZAnim *anim = ZAnim.m_CurAnim;
    const char *name = anim->GetName(cmd->m_argument_index);

    if (name && !strcasecmp("$1", name))
        name = anim->m_arg_list->GetCharP(0);
    s_openText = name;
    strcpy(s_editText, cmd->m_uivar);
    s_maxBytes0 = cmd->m_maxBytes0;
    s_maxChars0 = cmd->m_maxChars0;
    s_maxBytes1 = cmd->m_maxBytes1;
    s_maxChars1 = cmd->m_maxChars1;
    strcpy(s_captionText0, cmd->m_caption);
    SetCaption(cmd->m_caption);
    strcpy(s_captionText1, SkbTitle(anim, cmd->m_title1, "MsgPlayer title too long!"));
    strcpy(s_captionText2, SkbTitle(anim, cmd->m_title2, "Action2 title too long!"));
    s_noQuotes = cmd->m_noQuotes;
    s_allowEmpty = cmd->m_allowEmpty;
    s_keyEvent = 0;
    s_keyHeld = 0.0f;
    return cmd->m_name;
}

/* whether an empty text may be entered (0x395110; ours: OnEnterKey asks) */
int SkbEmptyAllowed()
{
    return s_allowEmpty;
}

/* whether a double quote may be typed as it is (0x396ED0; ours: 0x399EC0 asks) */
bool SkbQuotesAllowed()
{
    return !s_noQuotes;
}

/* the cursor's bitmap (2) sized to a key and put on it, the key's button made active (0x397590;
 * the name ours) */
void SkbPlaceCursor(CSkbKeyProps *props)
{
    if (props) {
        s_bmp2.Load(0.0f, 0.0f, (float)props->m_w, (float)props->m_h, s_bmp2.m_pTexHandle);
        s_bmp2.m_node->SetPosition((float)props->m_x, (float)props->m_y, 0.0f);
        void (C2DButton::*active)() = &C2DButton::SetActive;

        if (props->m_button)
            (props->m_button->*active)();
    }
}

/* every node the display holds forgotten: the bitmaps', the key rows' (through the bitmaps'
 * SetNode), the sounds, the display's root node, the caption's node and text (0x396F30, from
 * InvalidateSkbsDisplay; the name ours). Byte-exact since the UI's round 22: the key rows walked
 * through both rows' operator[]s, which keep the inner loop's entry at its bottom test (one accessor
 * alone, a row reference, a while loop or the counter declared outside all rotate it: 0x11C) */
void SkbForgetDisplay()
{
    s_bmp1.m_node = 0;
    s_bmp2.m_node = 0;
    s_bmp3.m_node = 0;
    s_bmp6.m_node = 0;
    s_bmp7.m_node = 0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++)
            s_keyRows[i][j].Slot10(0);
    s_bmp0.m_node = 0;
    s_bmp5.m_node = 0;
    s_bmp4.m_node = 0;
    s_sound4 = 0;
    s_sound3 = 0;
    s_sound2 = 0;
    s_sound5 = 0;
    s_sound1 = 0;
    s_sound0 = 0;
    g_skbDisplayNode = 0;
    s_captionNode = 0;
    SetCaption(0);
}

/* the string has a character at p (zui_skbkeys.cpp's, ours) */
static inline bool HasChar(const char *p)
{
    return p && *p;
}

/* a character's length (a copy of the pointer handed on, zui_skbkeys.cpp's) */
static inline int CharLen(const char *s)
{
    return UTF8CharLen(s);
}

/* the string has a character at p, into the caller's flag (zui_skbkeys.cpp's TestChar: a variable of
 * the caller's, numbered where it is declared, where an inline bool's return is a temporary) */
static inline bool TestChar(bool &more, const char *p)
{
    more = false;
    if ((bool)p && *p)
        more = true;
    return more;
}

/* whether a text keeps within the limits the look sets for string 0 or 1: its bytes, then its
 * characters (ours: SkbTextFits' body, an inline -- its `return true` a bool assigned, daddiu, as an
 * inlined function's is). Open from round 11 to 17 on its saved registers: MW's colouring, read off
 * with the MSL lane's igdump, gives the target's cursor s0, limit s1 and count s2 only with the limit
 * numbered first -- a variable of its own, assigned in an if/else (a ternary initialiser folds it
 * into the ternary's temporary, numbered after the walk's) -- and the walk's flag numbered between
 * the cursor and the count, so the count goes in simplify's first sweep and the cursor last */
static inline bool TextFits(const std::string &s, bool second)
{
    int maxChars;
    int maxBytes = second ? s_maxBytes1 : s_maxBytes0;

    if (maxBytes > 0 && maxBytes < s.size())
        return false;
    if (second)
        maxChars = s_maxChars1;
    else
        maxChars = s_maxChars0;
    if (maxChars > 0) {
        const char *p = s.c_str();
        bool more = false;
        int n = 0;

        while (TestChar(more, p)) {
            n++;
            p += CharLen(p);
        }
        return n <= maxChars;
    }
    return true;
}

/* (0x396D40, 0x399EC0's test; the name ours) */
bool SkbTextFits(const std::string &s, bool second)
{
    return TextFits(s, second);
}

/* whether a bitmap's node is shown (ours) */
static inline bool SkbShown(const CBmpWithNode &b)
{
    return b.m_node && b.m_node->m_active;
}

/* L1 held or just pressed (ours) */
static inline bool SkbPadShift(const CPad *pad)
{
    return pad->m_state[PAD_TRIGGER_L1] == KEY_FALLING || pad->m_state[PAD_TRIGGER_L1] == KEY_DOWN;
}

/* whether the keys type shifted: L1 against the shift lock (bitmap 5); 0x399070 asks (0x396E60;
 * the names ours). The two bools are compared with `^`, which MW keeps an xor of its own and converts
 * after: inlined (0x394520's two arms) the xor writes its second operand's register and the sltu the
 * result's, where `!=` is one set-not-equal whose xor writes the result's (the out-of-line copies
 * are the same code either way) */
bool SkbShifted(CPad *pad)
{
    bool shift = SkbPadShift(pad);

    return shift ^ SkbShown(s_bmp5);
}

/* the same, against the second lock (bitmap 0) as well (0x38D200 asks; 0x396CB0, ours; `^` as
 * above) */
bool SkbShifted2(CPad *pad)
{
    return SkbShifted(pad) ^ SkbShown(s_bmp0);
}

/* a string given cap i's look -- the default font when the cap names none, its colour and scale --
 * and its node put at the cap's place raised by dy, hidden (0x395440: 0x398EA0's three; ours) */
void SkbApplyCapLook(C2DString *str, zdb::CNode *node, CSoftKeyboard *skb, unsigned char i, C2DFont *font,
                     float dy)
{
    /* the array into a local first: the index is added before the keyboard (`&skb->m_caps[i]`
     * adds the keyboard first) */
    CSkbCapLook *caps = skb->m_caps;
    CSkbCapLook *cap = &caps[i];
    /* the name through a reference: its address kept apart for the size test */
    const CSaferStr &name = cap->m_texture;

    if (!name.size())
        font = s_defaultFont;
    str->SetFont(font);
    str->SetColor((float)cap->m_r, (float)cap->m_g, (float)cap->m_b);
    str->m_scale = cap->m_scale;
    node->SetPosition((float)cap->m_pos.Get1st(), dy + (float)cap->m_pos.Get2nd(), 0.0f);
    node->SetActive(false);
}

/* a rectangle from a place and a size (0x396C80; the look parser calls it out of line: pinned) */
#pragma auto_inline off
CSkbRect::CSkbRect(const CIntPair &pos, const CIntPair &size)
{
    m_pos[0] = pos.Get1st();
    m_pos[1] = pos.Get2nd();
    m_size[0] = size.Get1st();
    m_size[1] = size.Get2nd();
}

/* the block a tag names (0x396C70; ours: the look parser's rectangles, out of line) */
_zrdr *SkbFindTag(_zrdr *rdr, const char *tag)
{
    return zrdr_flat_findtag_startidx(rdr, tag, 0);
}

/* a rectangle read from an rdr block's Pos and Size pairs (the look parser 0x3963F0 calls it) */
void ReadSkbRect(CSkbRect *r, _zrdr *rdr)
{
    r->Set(ReadIntPair(rdr, "Pos"), ReadIntPair(rdr, "Size"));
}

/* a look rectangle set by index (the look parser's five, out of line; 0x396BD0, ours) */
void CSkbLook::SetRect(unsigned char i, const CSkbRect &r)
{
    m_rects[i] = r;
}
#pragma auto_inline reset

/* cap i's look read from the block a tag names: its SCALE (1 by default), its COLOR's three ints,
 * its FONT's name and its POSITION pair (0x397D30; the look parser calls it out of line; ours) */
void CSkbLook::ReadCapLook(unsigned char i, _zrdr *rdr, const char *tag)
{
    _zrdr *block = zrdr_findtag(rdr, tag);
    float scale = 1.0f;

    zrdr_findreal(block, "SCALE", &scale, 1);
    CSkbCapLook &cap = m_caps[i];

    cap.m_scale = scale;
    _zrdr *color = zrdr_findtag(block, "COLOR");
    int r = GetRdrArrayInt(color, 0);
    int g = GetRdrArrayInt(color, 1);
    int b = GetRdrArrayInt(color, 2);

    cap.m_r = r;
    cap.m_g = b;
    cap.m_b = g;
    cap.m_texture = zrdr_findstring(block, "FONT");
    cap.m_pos = ReadIntPair(block, "POSITION");
}
/* the keyboard's default width (0x4A44C4; ours) */
extern int data_004A44C4;

/* a rectangle from a block's Pos and Size, read in that order (ours) */
static inline CSkbRect SkbReadRect(_zrdr *rdr)
{
    CIntPair pos = ReadIntPair(rdr, "Pos");
    CIntPair size = ReadIntPair(rdr, "Size");

    return CSkbRect(pos, size);
}

/* an int or a real the rdr may give, else the default (ours: the look parser's scalars, each read
 * into a local of the helper's -- the target's frame holds them after the parser's own) */
static inline int SkbFindInt(_zrdr *rdr, const char *tag, int def)
{
    int i = def;

    zrdr_findint(rdr, tag, &i, 1);
    return i;
}

static inline float SkbFindReal(_zrdr *rdr, const char *tag, float def)
{
    float f = def;

    zrdr_findreal(rdr, tag, &f, 1);
    return f;
}

/* the look a keyboard's rdr block gives: its name and backgrounds, the four background piece lists
 * (each piece a Source -- Texture, Pos, Size -- and a Display -- Pos, Size), its place, size, edit
 * strings, cursor and caps, the five toggles' rectangles, the scaling times and the keyboards it
 * switches to (0x3963F0; CreateSoftKeyboard calls it; the name ours).
 * OPEN, 0x810 against 0x7D8 (written round 14 for its template tail: the pieces' resize and the
 * piece's copy match through it): the calls, their order and the code up to the first loop's body
 * are the target's; what differs is the frame and one variable. The scalars through the inline
 * readers (theirs a variable each, numbered after the parser's own lh) and the pieces' pairs
 * through SkbReadRect give the target's slots for both (444-412, 328-304); still off: the second
 * walker (the target's 376, just under the first: declared before start?), the five toggle
 * rectangles (240-176, never zeroed, under the loop's two rectangle temporaries 272/256 -- not
 * default-constructed CSkbRect locals: ReadSkbRect returning by value?), and the list index, which
 * the target narrows into a variable of its own (`andi s1,s5,0xff` in the walker's constructor's
 * delay slot, `andi` again at the read, the index added before `this`: SetRect's shape, an inline
 * taking an unsigned char?). Tried: an `unsigned char` index local (0x818) */
void CSoftKeyboard::ParseLook(_zrdr *rdr)
{
    m_name = zrdr_findstring(rdr, "Name");
    m_background = zrdr_findstring(rdr, "Background");
    m_shiftBackground = zrdr_findstring(rdr, "ShiftBackground");
    const char *tags[4] = {"BackgroundPieces", "ShiftBgPieces", "TwoEditsBgPieces", "TwoEditsShiftBgPieces"};

    for (unsigned int i = 0; i < 4; i++) {
        CSkbRdrIter it(rdr, tags[i]);

        if (it.More()) {
            CSkbBgPieces &pieces = m_sources[(unsigned char)i];

            do {
                pieces.resize(pieces.size() + 1);
                CSkbBgPiece &piece = pieces.back();
                _zrdr *item = it.Item();
                _zrdr *src = zrdr_flat_findtag_startidx(item, "Source", 0);

                piece.m_texture = zrdr_findstring(src, "Texture");
                piece.m_source = SkbReadRect(src);
                _zrdr *disp = zrdr_flat_findtag_startidx(item, "Display", 0);

                piece.m_display = SkbReadRect(disp);
                it.Next();
            } while (it.More());
        }
    }
    *(float *)&m_64 = -SkbFindReal(rdr, "TwoEditsGrowUp", 0.0f);
    CIntPair start = ReadIntPair(rdr, "StartGridPos");

    m_54 = start.Get1st();
    m_58 = start.Get2nd();
    CIntPair pos = ReadIntPair(rdr, "SkbPos");

    m_5c = pos.Get1st();
    m_60 = pos.Get2nd();
    m_68 = SkbFindInt(rdr, "SkbHeight", 0);
    m_6c = SkbFindInt(rdr, "SkbWidth", data_004A44C4);
    m_c0 = SkbFindInt(rdr, "UserStringWidth", 0);
    ReadCapLook(0, rdr, "UserStringProps");
    float lh = 0.0f;

    zrdr_findreal(rdr, "UserStringLineHeight", &lh, 1);
    m_c4 = lh;
    m_c8 = SkbFindInt(rdr, "CursorY", 0);
    m_cc = SkbFindInt(rdr, "CursorY2nd", 0);
    CIntPair csize = ReadIntPair(rdr, "CursorSize");

    m_d0 = csize.Get1st();
    m_d4 = csize.Get2nd();
    ReadCapLook(1, rdr, "PurposeStringProps");
    ReadCapLook(2, rdr, "MsgPlayerStringProps");
    CSkbRect r0, r1, r2, r3, r4;

    ReadSkbRect(&r0, SkbFindTag(rdr, "CapsLockOn"));
    SetRect(0, r0);
    ReadSkbRect(&r1, SkbFindTag(rdr, "ShiftOn"));
    SetRect(1, r1);
    ReadSkbRect(&r2, SkbFindTag(rdr, "LegendOn"));
    SetRect(2, r2);
    ReadSkbRect(&r3, SkbFindTag(rdr, "UsbKbdOn"));
    SetRect(3, r3);
    ReadSkbRect(&r4, SkbFindTag(rdr, "UsbKbdMode"));
    SetRect(4, r4);
    m_scaleUpSec = SkbFindReal(rdr, "ScaleUpSec", 0.25f);
    m_scaleDownSec = SkbFindReal(rdr, "ScaleDownSec", 0.25f);
    CSkbRdrIter it(rdr, "SwitchTargets");

    if (it.More()) {
        do {
            m_strings.push_back(CSaferStr(it.Item()->String()));
            it.Next();
        } while (it.More());
    }
}

/* a sound played once, if it was found (ours) */
static inline void PlayOnce(CSnd *snd)
{
    if (snd)
        snd->PlaySound(true, 0, 255);
}

/* one of the display's six sounds (0x395DE0; the name ours) */
void SkbPlaySound(unsigned char which)
{
    switch (which) {
    case 0:
        PlayOnce(s_sound0);
        break;
    case 1:
        PlayOnce(s_sound1);
        break;
    case 2:
        PlayOnce(s_sound2);
        break;
    case 3:
        PlayOnce(s_sound3);
        break;
    case 4:
        PlayOnce(s_sound4);
        break;
    case 5:
        PlayOnce(s_sound5);
        break;
    }
}

/* bitmap 4's node toggled, bitmap 3's shown when it comes on, its timer cleared (0x396300; the name
 * ours) */
void SkbToggleBmp4()
{
    bool on = !SkbShown(s_bmp4);

    if (s_bmp4.m_node)
        s_bmp4.m_node->SetActive(on);
    if (SkbShown(s_bmp4) && s_bmp3.m_node)
        s_bmp3.m_node->SetActive(true);
    s_bmp4Timer = 0.0f;
    SkbPlaySound(3);
}

/* a bitmap's node toggled (ours) */
static inline void SkbToggle(CBmpWithNode *b)
{
    b->m_node->SetActive(!b->m_node->m_active);
}

/* whether a bitmap has its node (ours) */
static inline bool SkbHasNode(const CBmpWithNode *b)
{
    return b->m_node != 0;
}

/* two bitmaps' nodes toggled together, the callback told whether the second is on now (0x396220:
 * 0x38D200 and 0x38C630 pass the keyboard's 0x1C0 and 0x270; the name ours) */
void SkbTogglePair(CBmpWithNode *a, CBmpWithNode *b, void (*onToggle)(bool))
{
    /* through a bool inline: tested in a temporary, the node is read again for the toggle (a plain
     * `a->m_node` test keeps it in a0) */
    if (!SkbHasNode(a) || !SkbHasNode(b))
        return;
    SkbToggle(a);
    SkbToggle(b);
    onToggle(SkbShown(*b));
    SkbPlaySound(3);
}

void *_calloc(unsigned int n, unsigned int size, const char *file, int line);
int GetUICommandType(const char *name);

/* a string an rdr tag names copied into a field of n bytes, else the field emptied (ours: the
 * set-up's parser) */
static inline void SkbCopyField(char *dst, _zrdr *rdr, const char *tag, unsigned int n)
{
    const char *s = zrdr_findstring(rdr, tag);

    if (s && strlen(s) < n)
        strcpy(dst, s);
    else
        dst[0] = 0;
}

/* an anim name an rdr tag names, added to the running anim's names, else -1 (ours) */
static inline int SkbFindName(_zrdr *rdr, const char *tag)
{
    const char *s = zrdr_findstring(rdr, tag);

    return s ? ZAnim.m_CurAnim->AddName(s) : -1;
}

/* the keyboard's set-up command's parser (0x395F40; SOCOM II's, the name ours): its record built --
 * the purpose from UiVar, the two strings' limits (MaxBytes, MaxChars, MaxBytes2, MaxChars2), the
 * caption (Purpose), the two titles (MsgPlayerTitle, Action2Title), the keyboard (SkbName), the
 * variable the text goes to (UiVar2) and the two flags (NoDQuote, AllowEmpty), as zanim_menu.cpp's
 * parsers build theirs */
UISkbOpenCmd *SkbOpenParse(const char *name, _zrdr *rdr)
{
    UISkbOpenCmd *cmd = (UISkbOpenCmd *)_calloc(1, sizeof(UISkbOpenCmd), 0, 0);
    const char *s;

    cmd->header.timeless = 1;
    cmd->header.data_size = sizeof(UISkbOpenCmd);
    cmd->node_ref_index = 0;
    cmd->m_command_index = GetUICommandType(name);
    s = zrdr_findstring(rdr, "UiVar");
    if (s && *s) {
        cmd->m_argument_index = ZAnim.m_CurAnim->AddName(s);
        cmd->m_c = 0;
    }
    int maxBytes0 = 0;

    zrdr_findint(rdr, "MaxBytes", &maxBytes0, 1);
    cmd->m_maxBytes0 = maxBytes0;
    int maxChars0 = 0;

    zrdr_findint(rdr, "MaxChars", &maxChars0, 1);
    cmd->m_maxChars0 = maxChars0;
    int maxBytes1 = 0;

    zrdr_findint(rdr, "MaxBytes2", &maxBytes1, 1);
    cmd->m_maxBytes1 = maxBytes1;
    int maxChars1 = 0;

    zrdr_findint(rdr, "MaxChars2", &maxChars1, 1);
    cmd->m_maxChars1 = maxChars1;
    SkbCopyField(cmd->m_caption, rdr, "Purpose", sizeof(cmd->m_caption));
    cmd->m_title1 = SkbFindName(rdr, "MsgPlayerTitle");
    cmd->m_title2 = SkbFindName(rdr, "Action2Title");
    SkbCopyField(cmd->m_name, rdr, "SkbName", sizeof(cmd->m_name));
    SkbCopyField(cmd->m_uivar, rdr, "UiVar2", sizeof(cmd->m_uivar));
    cmd->m_noQuotes = zrdr_flat_findtag_startidx(rdr, "NoDQuote", 0) != 0;
    cmd->m_allowEmpty = zrdr_flat_findtag_startidx(rdr, "AllowEmpty", 0) != 0;
    return cmd;
}

/* a bitmap loaded with a texture by name at a size, and hung under a node (0x395D50: PrepForDisplay
 * and 0x38B8F0 call it; the name ours) */
zdb::CNode *SkbLoadBitmap(CBmpWithNode *bmp, zdb::CNode *parent, int w, int h, const char *tex,
                          zdb::CTextureRelocMgr *mgr)
{
    bmp->Load(0.0f, 0.0f, (float)w, (float)h, PrepTexture(tex, mgr));
    return func_0036AB20(bmp, mgr, parent);
}

/* the pair of ints a tag names (SOCOM 1's ReadIntPair; the look parser and zui_skb.cpp's 0x38DCB0
 * call it out of line: pinned) */
#pragma auto_inline off
CIntPair ReadIntPair(_zrdr *rdr, const char *tag)
{
    _zrdr *t = zrdr_findtag(rdr, tag);
    int a = GetRdrArrayInt(t, 0);
    int b = GetRdrArrayInt(t, 1);

    return CIntPair(a, b);
}
#pragma auto_inline reset

/* the caption's node placed at the keyboard's second cap's position, raised by dy (ours: the
 * 0.0f a caller passes is still added) */
static inline void PlaceCaption(CSoftKeyboard *skb, float dy)
{
    s_captionNode->SetPosition((float)skb->m_caps[1].m_pos.Get1st(), dy + (float)skb->m_caps[1].m_pos.Get2nd(), 0.0f);
}

/* the key repeats a key event held so long gives (ours: the tick's two copies, before and after
 * the frame's time): none until the delay, then the distance covered at a speed ramping from the
 * start to the end speed over the acceleration time and the end speed after it, plus one */
static inline int SkbRepeats(float held)
{
    float frac, a, rest;
    float d = s_repeatSpeedEnd - s_repeatSpeedStart;
    float t = held - s_repeatDelay;
    bool past = t > 0.0f;
    float e = past ? t : 0.0f;
    bool full = e >= s_repeatAccelTime;

    e = full ? s_repeatAccelTime : e;
    frac = e / s_repeatAccelTime;

    frac = full ? 1.0f : frac;
    a = e * (s_repeatSpeedStart + 0.5f * d * frac);
    rest = t - s_repeatAccelTime;

    rest = rest >= 0.0f ? rest : 0.0f;
    float b = s_repeatSpeedEnd * rest;

    return past ? (int)(a + b) + 1 : 0;
}

/* the display ticked by dT (0x3949C0; the name ours: SkbTick calls it): bitmap 4's legend slid in
 * while bitmap 4 shows, else out and hidden, by its timer (stopped at -1 once done); then, unless
 * the keyboard is scaling, the cursor's blink stepped (bitmap 1 shown for the first half second)
 * and, while a key event is held, the number of its repeats the frame adds returned.
 * Written in the UI lane's round 32 (unwritten before); byte-exact in round 36. Three changes, each
 * read off MW's colouring (igdump / igreplay --target): (1) the legend's x and width, $f0/$f1 in the
 * target, need the width's nodes numbered ahead of x's -- the product wrapped in a no-op `(float)`
 * cast (the SEALs' round-55 lever: the cast node makes it the heavier operand; `(float)t * w` does it
 * too), 17 -> 11; (2) the repeat inline's two clamps as ternaries, `e = full ? accel : e` and `frac =
 * full ? 1.0f : frac` (the if form pads each then-arm with the join's nop and schedules the 0.5f
 * constant early: if versus ternary, the UI's round 31), 11 -> 7; (3) the second copy's `rest` in
 * e's register: no numbering gives it (igreplay: rest interfered with e), so the graph differed --
 * `a` computed BEFORE `rest`, declared uninitialised at the inline's top between frac and rest (or
 * first), so the first copy keeps its numbering; computed after rest, or declared at its statement
 * or after rest, it stays 7 off (round 35: 72 with rest computed after `a` and declared there).
 * Measured before: the repeat statics load up front and `end - start` is formed first only with the
 * range computed at the inline's top (start/end locals, parameters: 120); `a + b` from two locals
 * with the clamp between keeps the target's separate mul.s / add.s (one expression fuses to mula.s /
 * madd.s); the clamp as `rest >= 0.0f ? rest : 0.0f` gives the target's bc1t with the product
 * duplicated into both arms (`if (rest < 0.0f)` a bc1f, 6 short); the held event compared as
 * `(SKBKEY)s_keyEvent == SKBKEY_NONE` loads with lbu (the plain signed char lb) */
int SkbTickDisplay(CSoftKeyboard *skb, float dT)
{
    if (s_bmp4Timer >= 0.0f) {
        s_bmp4Timer += dT;
        bool shown = SkbShown(s_bmp4);
        float t = s_bmp4Timer / (shown ? s_slideInSec : s_slideOutSec);

        if (t > 1.0f) {
            t = 1.0f;
            s_bmp4Timer = -1.0f;
            if (!shown && s_bmp3.m_node)
                s_bmp3.m_node->SetActive(false);
        }
        int x = skb->m_6c;

        if (shown)
            t = 1.0f - t;
        s_bmp3.m_node->SetPosition((float)x - (float)(t * (float)s_rect.m_size[0]), 0.0f, 0.0f);
    }
    if (s_scaling)
        return 0;
    s_blinkTime += dT;
    if (s_blinkTime > 1.0f)
        s_blinkTime -= 1.0f;
    bool on = s_blinkTime < 0.5f;

    if (s_bmp1.m_node)
        s_bmp1.m_node->SetActive(on);
    if ((SKBKEY)s_keyEvent == SKBKEY_NONE)
        return 0;
    int before = SkbRepeats(s_keyHeld);

    s_keyHeld += dT;
    return SkbRepeats(s_keyHeld) - before;
}

/* the caption's mode 2 toggled: its text, then its place (0x394920; the name ours) */
void SkbToggleCaption2(CSoftKeyboard *skb)
{
    bool on = s_captionMode != CAPTION_2;

    s_captionMode = on ? CAPTION_2 : CAPTION_PLAIN;
    SetCaption(on ? s_captionText2 : s_captionText0);
    PlaceCaption(skb, 0.0f);
}

/* the second lock (bitmap 0) toggled (0x3948F0; the name ours) */
void SkbToggleLock2()
{
    s_bmp0.m_node->SetActive(!s_bmp0.m_node->m_active);
}

/* how far mode 1 raises the caption: the look's float at 0x64, or nothing (ours) */
static inline float CaptionRaise(CSoftKeyboard *skb, bool on)
{
    return on ? *(float *)&skb->m_64 : 0.0f;
}

/* the caption's mode 1 toggled: its place (raised by the look's float at 0x64 when on), then its
 * text; whether it is on now (0x394840; the name ours) */
bool SkbToggleCaption1(CSoftKeyboard *skb)
{
    bool on = s_captionMode != CAPTION_1;

    s_captionMode = on ? CAPTION_1 : CAPTION_PLAIN;
    int x = skb->m_caps[1].m_pos.Get1st(), y = skb->m_caps[1].m_pos.Get2nd();
    float dy = CaptionRaise(skb, on);

    s_captionNode->SetPosition((float)x, dy + (float)y, 0.0f);
    SetCaption(on ? s_captionText1 : s_captionText0);
    return on;
}

/* the shift lock (bitmap 5) toggled, or hidden (0x3947D0; the name ours) */
void SkbToggleShiftLock(bool toggle)
{
    if (toggle)
        s_bmp5.m_node->SetActive(!s_bmp5.m_node->m_active);
    else if (s_bmp5.m_node)
        s_bmp5.m_node->SetActive(false);
}

/* the background piece lists, as the look names them: BackgroundPieces, ShiftBgPieces,
 * TwoEditsBgPieces, TwoEditsShiftBgPieces (a byte enum: the choice below is assigned with `li`, an
 * unsigned char's with `daddiu`; the names ours) */
enum SKBLIST { SKBLIST_PLAIN, SKBLIST_SHIFT, SKBLIST_TWO, SKBLIST_TWOSHIFT };

/* the key rows shown for the background piece list in use: 3 the two edits' shifted list, 2 the
 * two edits' list (caption mode 1 with a list), 1 the shifted list (shifted, with a shift background
 * or a list), else 0; bitmaps 6 and 7 shown with lists 0 and 1, and row i's keys j shown while i is
 * the list's index and j under its count -- the shift by the pad and the shift lock alone when the
 * keyboard has a row-2 key, else against the second lock too (0x394520: 0x38C990 and 0x38D200 call it
 * after the locks toggle, with the keyboard's 0x174; the name ours). Byte-exact (the UI's round 22):
 * the last list's test a bool local (its load and compare ahead of the first branch), declared before
 * `two`; the counts compared as ints (slt/blez); the key's flag a ternary (slt into v1, no branch),
 * `list == i` (the list first in the bne); the rows walked through their operator[]s */

void SkbShowKeyRows(CPad *pad, CSoftKeyboard *skb, bool row2)
{
    bool shifted = row2 ? SkbShifted(pad) : SkbShifted2(pad);
    SKBLIST list = SKBLIST_PLAIN;
    bool has3 = (int)skb->m_sources[3].size() > 0;
    bool two = s_captionMode == CAPTION_1;

    if (two && shifted && has3)
        list = SKBLIST_TWOSHIFT;
    else if (two && (int)skb->m_sources[2].size() > 0)
        list = SKBLIST_TWO;
    else if (shifted && (*skb->m_shiftBackground.c_str() || (int)skb->m_sources[1].size() > 0))
        list = SKBLIST_SHIFT;
    if (s_bmp6.m_node)
        s_bmp6.m_node->SetActive(list == 0);
    if (s_bmp7.m_node)
        s_bmp7.m_node->SetActive(list == 1);
    int count = SkbPieces(skb, list).size();

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++) {
            CBmpWithNode &bmp = s_keyRows[i][j];
            bool on = list == i ? j < count : false;

            if (bmp.m_node)
                bmp.m_node->SetActive(on);
        }
}

/* the Korean input's rules, read from the same file (zui_hangul.cpp's) */
void SkbParseKoreanRules(_zrdr *rdr);

/* the soft keyboards' common props, UiParams.rdr's CommonSoftKeyboardProps: the key repeat's delay
 * and speeds, the six sounds' names (the scaling's two, the key presses', the navigation's and the
 * refusal's), the legend's texture, rectangle and slide times, the USB keyboard's icon, room for
 * the typed text, the Korean rules, and the backgrounds' opacity given to bitmaps 6 and 7 and every
 * key (0x397070; SkbLoadUiParams calls it with the file; the name ours) */
void SkbReadUiParams(CRdrFile *file)
{
    _zrdr *props = zrdr_findtag(file, "CommonSoftKeyboardProps");

    s_repeatDelay = SkbFindReal(props, "RepeatDelay", 0.0f);
    s_repeatSpeedStart = SkbFindReal(props, "RepeatSpeedStart", 0.0f);
    s_repeatAccelTime = SkbFindReal(props, "RepeatAccelTime", 0.0f);
    s_repeatSpeedEnd = SkbFindReal(props, "RepeatSpeedEnd", 0.0f);
    s_str6 = zrdr_findstring(props, "ScaleUpSound");
    s_str5 = zrdr_findstring(props, "ScaleDownSound");
    s_str2 = zrdr_findstring(props, "KeyPressSound");
    s_str1 = zrdr_findstring(props, "KeyPressEnterSound");
    s_str3 = zrdr_findstring(props, "NavigationSound");
    s_str4 = zrdr_findstring(props, "NegativeSound");
    _zrdr *legend = zrdr_findtag(props, "Legend");

    s_str0 = zrdr_findstring(legend, "Texture");
    s_rect = SkbReadRect(legend);
    s_slideInSec = SkbFindReal(legend, "SlideInSec", 0.0f);
    s_slideOutSec = SkbFindReal(legend, "SlideOutSec", 0.0f);
    s_str8 = zrdr_findstring(props, "UsbKbdOnIcon");
    s_openText.reserve(800);
    SkbParseKoreanRules(file);
    float opacity = SkbFindReal(props, "BackgroundsOpacity", 1.0f);

    s_bmp6.SetTrans(128.0f * opacity);
    float trans = 128.0f * opacity;

    s_bmp7.SetTrans(trans);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++)
            s_keyRows[i][j].SetTrans(trans);
}

/* a key read this frame (zin_keyboard.cpp's KeyInput, 32 bytes) and its reader */
struct KeyInput {
    unsigned char code;
    int shift;
    int ctrl;
    int alt;
    int _10;
    int caps;
    int _18[2];
};
int GetKeyReadThisFrame(KeyInput *key);

/* the three toggles' bitmaps and the look rectangles they take (a table 0x395540 copies from
 * 0x3E0500 into its frame; ours) */
struct SKBTOGGLE {
    unsigned char m_rect;
    CBmpWithNode *m_bmp;
};

/* the display set going as a keyboard opens: the caption plain, two keys read away, the display
 * placed at the look's position, the cursor (bitmap 2) hidden, the legend (bitmap 3) placed slid in,
 * the plain background's keys shown, the caption given cap 1's look, the caret (bitmap 1) sized and
 * hidden, the scaling up started, and the three toggles sized, placed at their rectangles and hidden
 * (0x395540; ShowSkb calls it with the font; the name ours).
 * Byte-exact since round 73 (the width through SkbRectWidth, above). Was OPEN, 1 instruction: the legend's `mul.s $f0,$f0,$f1` (1.0 times the width, the 1.0 in f0) comes
 * out `mul.s $f0,$f1,$f0`: written `t * w` MW numbers the 1.0 before the width and colours the
 * width into f0, written `w * t` (as below) the registers are the target's and the operands
 * swapped -- the target wants the 1.0 first in the product but numbered after the width (tried: the
 * width or the x in a named local, an int, an inline accessor, a const or swapped parameter, the
 * comma form, a negated product, t as a local or a const local -- that one folds). The caption's
 * look is written here, not through SkbApplyCapLook (0x395440): the target converts the colour's
 * three components each into f0 at its store, which only int components read into locals (b, g,
 * r) give -- SkbApplyCapLook inlined keeps SetColor's g and b as named variables, converted in
 * place; the font choice `font = name.size() == 0 ? s_defaultFont : font` leaves the join unpadded
 * as the target (`if (!name.size())` pads it: the game states' round-34 ternary rule; `!size() ?`
 * gives two arms); the node read into a local first (the target keeps it in s1 across the calls).
 * Round 23: 0x398 -> 0x390, 18 lines off -> 1.
 * Round 24, read off MW's float colouring (igdump): MW numbers the product's operands in TREE order
 * and the colouring follows the numbers -- the target's registers (x f2, width f1, the 1.0 f0) are
 * exactly `x - w * t`'s numbering, yet its product reads `t * w`, so the 1.0 is materialised after
 * the width although it is the left operand; the tick 0x3949C0 places the same legend as `x - t *
 * w` with a variable t in f21. Every `t * w` form numbers the 1.0 first (4 off), every `w * t` form
 * keeps the swap (1 off): the width or x as int, float or dead-first-defined locals, an implicit
 * int-to-float conversion, a product local, a by-value or by-reference t (by reference 0x398), an
 * int, double or self-cast argument, `1.0f - 0.0f`, an extra inline level, x / w / t as the inline's
 * three parameters (int or float), a multiply inline, the node in a local, t a caller local defined
 * before the calls (the constant is propagated before numbering in each).
 * Round 38, read in MW's own code generator (mweppc's fp_binary_operator, a prediction for mwcps2):
 * `if (right->hascall) { right first, then left } else { left first, then right }`, then `op reg, left,
 * right` -- the target's word is exactly that branch's output, `t * w` with the width generated first:
 * its width operand carries hascall although no call survives in its code. 14 casts and spellings of
 * the operands and product move nothing (4 off or 1 off: build/scratch/ui/v_ssd1.py); what sets the
 * flag without leaving a call is not found (the pass that computes it is not among the sources). */
/* a rectangle's width, through an inline taking the rectangle (ours). Round 73: the width operand
 * read through an inline WITH AN ARGUMENT is round 38's `hascall` -- the product then generates the
 * width first and reads `t * w` (`mul.s f0,f0,f1`), which the plain member read, an argumentless
 * inline returning it (int or float) and every round-24 spelling do not give */
static inline int SkbRectWidth(const CSkbRect &r)
{
    return r.m_size[0];
}

/* the legend (bitmap 3) placed slid in by t, a fraction of its width (ours: the 1.0f a caller
 * passes is still multiplied) */
static inline void SkbPlaceLegend(CSoftKeyboard *skb, float t)
{
    s_bmp3.m_node->SetPosition((float)skb->m_6c - (float)SkbRectWidth(s_rect) * t, 0.0f, 0.0f);
}

/* a node put at a place raised by dy (ours: the 0.0f a caller passes is still added) */
static inline void SkbPlaceNode(zdb::CNode *node, int x, int y, float dy)
{
    node->SetPosition((float)x, dy + (float)y, 0.0f);
}

void SkbStartDisplay(CSoftKeyboard *skb, C2DFont *font)
{
    KeyInput key;

    s_captionMode = CAPTION_PLAIN;
    GetKeyReadThisFrame(&key);
    GetKeyReadThisFrame(&key);
    SkbPlaceNode(g_skbDisplayNode, skb->m_5c, skb->m_60, 0.0f);
    if (s_bmp2.m_node)
        s_bmp2.m_node->SetActive(false);
    SkbPlaceLegend(skb, 1.0f);
    if (s_bmp6.m_node)
        s_bmp6.m_node->SetActive(true);
    if (s_bmp7.m_node)
        s_bmp7.m_node->SetActive(false);
    int count = SkbPieces(skb, SKBLIST_PLAIN).size();

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++) {
            CBmpWithNode &bmp = s_keyRows[i][j];
            bool on = i == 0 ? j < count : false;

            if (bmp.m_node)
                bmp.m_node->SetActive(on);
        }
    {
        zdb::CNode *node = s_captionNode;
        CSkbCapLook *caps = skb->m_caps;
        CSkbCapLook *cap = &caps[1];
        const CSaferStr &name = cap->m_texture;

        font = name.size() == 0 ? s_defaultFont : font;
        s_caption.SetFont(font);
        int b = cap->m_b, g = cap->m_g, r = cap->m_r;

        s_caption.SetColor(r, g, b);
        s_caption.m_scale = cap->m_scale;
        SkbPlaceNode(node, cap->m_pos.Get1st(), cap->m_pos.Get2nd(), 0.0f);
        node->SetActive(false);
    }
    s_bmp1.Load(0.0f, 0.0f, (float)skb->m_d0, (float)skb->m_d4, s_bmp1.m_pTexHandle);
    if (s_bmp1.m_node)
        s_bmp1.m_node->SetActive(false);
    s_scaleTime = 0.0f;
    s_scaling = SKBSCALE_UP;
    SKBTOGGLE toggles[3] = {{0, &s_bmp0}, {1, &s_bmp5}, {2, &s_bmp4}};

    for (unsigned int k = 0; k < 3; k++) {
        CSkbRect r = skb->m_rects[toggles[k].m_rect];
        CBmpWithNode *bmp = toggles[k].m_bmp;
        const CSkbRect *p = &r;

        bmp->Load(0.0f, 0.0f, (float)p->m_size[0], (float)p->m_size[1], bmp->m_pTexHandle);
        SkbPlaceNode(bmp->m_node, p->m_pos[0], p->m_pos[1], 0.0f);
        if (bmp->m_node)
            bmp->m_node->SetActive(false);
    }
}

/* the keyboards' shared display built under the menu's node: the sounds found, the display's root
 * node (hidden), the legend (bitmap 3: its texture and the legend rectangle's size, hidden), the
 * cursor's highlight (bitmap 2), bitmaps 6 and 7 and the forty keys untextured, the default font
 * kept, the key units' own pieces made through the callback, the caption's node, the caret (bitmap
 * 1: no texture, white, its node hidden) and the three toggles' highlights (0x3958D0;
 * PrepSkbsForDisplay calls it; the name ours). Byte-exact (the UI's round 22): SkbLoadBitmap inlined
 * (SkbFindSounds pinned: called out of line), the legend's size read into int locals first (the
 * target loads it ahead of the texture name's c_str()), each toggle taken into a local (kept in a
 * saved register for the SetNode slot's call) */
void SkbPrepDisplay(zdb::CNode *parent, zdb::CTextureRelocMgr *mgr, C2DFont *font,
                    void (*prep)(zdb::CNode *node, zdb::CTextureRelocMgr *mgr))
{
    SkbFindSounds();
    g_skbDisplayNode = zdb::CNode::Create("AllSkbsRoot");
    parent->AddChild(g_skbDisplayNode);
    g_skbDisplayNode->SetActive(false);
    int w = s_rect.m_size[0], h = s_rect.m_size[1];

    s_bmp3.Slot10(SkbLoadBitmap(&s_bmp3, g_skbDisplayNode, w, h, s_str0.c_str(), mgr));
    if (s_bmp3.m_node)
        s_bmp3.m_node->SetActive(false);
    s_bmp2.Slot10(SkbLoadBitmap(&s_bmp2, g_skbDisplayNode, 0, 0, "SkbHighlight.tif", mgr));
    s_bmp6.Slot10(SkbLoadBitmap(&s_bmp6, g_skbDisplayNode, 0, 0, 0, mgr));
    s_bmp7.Slot10(SkbLoadBitmap(&s_bmp7, g_skbDisplayNode, 0, 0, 0, mgr));
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 10; j++) {
            CBmpWithNode &bmp = s_keyRows[i][j];

            bmp.Slot10(SkbLoadBitmap(&bmp, g_skbDisplayNode, 0, 0, 0, mgr));
        }
    s_defaultFont = font;
    prep(g_skbDisplayNode, mgr);
    s_captionNode = func_0036AB20(&s_caption, mgr, g_skbDisplayNode);
    s_bmp1.SetHasTexture(false);
    s_bmp1.SetColor(255.0f, 255.0f, 255.0f);
    s_bmp1.m_node = func_0036AB20(&s_bmp1, mgr, g_skbDisplayNode);
    if (s_bmp1.m_node)
        s_bmp1.m_node->SetActive(false);
    CBmpWithNode *toggles[3] = {&s_bmp0, &s_bmp5, &s_bmp4};

    for (unsigned int k = 0; k < 3; k++) {
        CBmpWithNode *bmp = toggles[k];

        bmp->Slot10(SkbLoadBitmap(bmp, g_skbDisplayNode, 0, 0, "SkbHighlight.tif", mgr));
    }
}

/* whether the keyboard is scaling up or down (0x396EC0; HandlePad asks; the name ours) */
bool SkbScaling()
{
    return s_scaling != SKBSCALE_NONE;
}

/* whether the caption shows mode 1, the two edit strings (0x396F10; 0x399100 asks; the name ours) */
bool SkbTwoEdits()
{
    return s_captionMode == CAPTION_1;
}

/* insert<CSkbBgPiece> 0x3982F0's end numbered before m: include/vector's __insert_end_pointer, a marked modelling device (the AI core's round 37) */
namespace std { template <> struct __insert_end_pointer<CSkbBgPiece> { enum { value = 1 }; }; }

/* A MODELLING DEVICE, a "Level 6, or 8?" witness, with `__MSL_POD_DTOR_ARGS` at this file's head (include/vector; the MSL headers round 40): copy's tree
 * over the pieces is eight high (include/vector's erase note), so at level 7 and with the pieces' copy taller the insert 0x3982F0 and the erase 0x3980D0 are
 * byte-exact. The erase: the target colours the end after `this` (s3, this s2, first s1, last s0) where a named end goes first; `igreplay --want` puts it past __copy's two
 * argument temporaries, so an object end handed to a copy whose LAST parameter it is (__erase_end_object's value 2: the object alone 45 off) */
namespace std { template <> struct __erase_end_object<CSkbBgPiece> { enum { value = 2 }; }; }

#pragma inline_depth(7)
