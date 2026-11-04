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
 * OPEN, not written yet: the tick 0x3949C0, the display's builders 0x394520, 0x395140, 0x395540,
 * 0x3958D0, 0x397070, 0x397950, the cap look reader 0x397D30.
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
 * unit (0x394520, 0x395140, 0x395540, 0x3958D0, 0x396F30, 0x397070); the members spelled out, MW
 * rotates the loop knowing j = 0 (round 21's forms, all short) */
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

/* the four cursor events by the stick's direction or the pad button pressed (0x3FB388; ours) */
struct SKBPADKEY {
    unsigned char m_dir;     // a STICK_DIR
    unsigned char m_button;  // a PAD_BUTTON
    signed char m_event;
};
static SKBPADKEY s_padKeys[4] = {{1, 3, 2}, {2, 2, 3}, {3, 4, 1}, {4, 5, 4}};

/* the pad's key event: a cursor move by the stick or the pad, else cross, L2 or R2 (5, 6, 7) --
 * kept, its hold time restarted; with none, the kept event cleared once the stick is centred or
 * one of those keys is let go (0x394F60; the name ours). The release test is a bool accumulated
 * with `|=` (the target's branch-free sltu/or chain; `||` branches, `|` of the compares differs).
 * OPEN, 16 bytes short (0x198 against 0x1A8): the event. The target loads the table's event with
 * `lb`, keeps it in v0 (the return value's register) and narrows it with `andi 0xff` at each test;
 * a byte-enum local folds the narrowing into an `lbu` and drops the tests' andi, a char local gives
 * the lb but no andi (0x1A0), an int local tested through a byte cast reaches the size but takes a3
 * and moves every volatile register (35 off): the shape that keeps all three is not found */
SKBKEY SkbPadKeyEvent(CPad *pad)
{
    STICK_DIR dir = UiGetDirStickMoved();
    SKBKEY event = SKBKEY_NONE;

    for (int i = 0; i < 4; i++) {
        if (pad->m_state[s_padKeys[i].m_button] == KEY_FALLING || dir == s_padKeys[i].m_dir) {
            event = (SKBKEY)s_padKeys[i].m_event;
            break;
        }
    }
    if (!event) {
        if (pad->m_state[PAD_CROSS] == KEY_FALLING)
            event = SKBKEY_5;
        else if (pad->m_state[PAD_TRIGGER_L2] == KEY_FALLING)
            event = SKBKEY_6;
        else if (pad->m_state[PAD_TRIGGER_R2] == KEY_FALLING)
            event = SKBKEY_7;
    }
    if (event) {
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
    CSkbBgPieces *s = look->m_sources;

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

/* the display scaled to a fraction of its size (0x397950, not written yet) */
extern "C" void func_00397950(float scale);
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
    func_00397950(scale);
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

/* the display's sounds found by the names the look gave (0x397BC0, from 0x3958D0; the name ours) */
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

/* the text: the edited buffer, or the one the keyboard opened with (0x3998A0 and 0x399D20 ask) */
const char *SkbGetText(bool edited)
{
    return edited ? s_editText : s_openText.c_str();
}

/* the display unit's reader of UiParams.rdr (0x397070, not written yet) */
extern "C" void func_00397070(CRdrFile *file);

/* the keyboards' display read again from UiParams.rdr (0x3943E0; CMission::Init calls it). This
 * unit's: it lies between ~CSkbKeyProps and 0x394420, and its two strings between s_padKeys and
 * 0x394DA0's valve name in the unit's data (zui_hangul.cpp's until round 21) */
void SkbLoadUiParams()
{
    CRdrFileHolder file("UiParams.rdr", "data/common");

    func_00397070(file);
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
 * the names ours) */
bool SkbShifted(CPad *pad)
{
    bool shift = SkbPadShift(pad);

    return shift != SkbShown(s_bmp5);
}

/* the same, against the second lock (bitmap 0) as well (0x38D200 asks; 0x396CB0, ours) */
bool SkbShifted2(CPad *pad)
{
    return SkbShifted(pad) != SkbShown(s_bmp0);
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
    node->SetPosition((float)cap->m_0, dy + (float)cap->m_4, 0.0f);
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

/* a cap's look read from the block a tag names (0x397D30, not written yet) */
extern "C" void func_00397D30(CSkbLook *look, unsigned char i, _zrdr *rdr, const char *tag);
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
    func_00397D30(this, 0, rdr, "UserStringProps");
    float lh = 0.0f;

    zrdr_findreal(rdr, "UserStringLineHeight", &lh, 1);
    m_c4 = lh;
    m_c8 = SkbFindInt(rdr, "CursorY", 0);
    m_cc = SkbFindInt(rdr, "CursorY2nd", 0);
    CIntPair csize = ReadIntPair(rdr, "CursorSize");

    m_d0 = csize.Get1st();
    m_d4 = csize.Get2nd();
    func_00397D30(this, 1, rdr, "PurposeStringProps");
    func_00397D30(this, 2, rdr, "MsgPlayerStringProps");
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
    s_captionNode->SetPosition((float)skb->m_caps[1].m_0, dy + (float)skb->m_caps[1].m_4, 0.0f);
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
    int x = skb->m_caps[1].m_0, y = skb->m_caps[1].m_4;
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
