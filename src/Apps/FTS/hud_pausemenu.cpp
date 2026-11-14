/* hud_pausemenu.cpp -- the HUD's pause menu (CZPauseTest, OVL1 0x2011F0-0x205C3F: round 36 put its start at 0x2011F0, its last function is the lines' test 0x205A70): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_pausemenu.cpp), after the lens flares and before the
 * objectives as there; its first functions in the image are the vtable's On, Off and Draw, its
 * last the destructor and the constructor. SOCOM II's CZPauseTest is 0x13C0 bytes (SOCOM 1's
 * 0x380), laid out from its destructor. Written: On, Off, Draw, Clear, ReInit, the constructor, the
 * lines' test and the line's constructor and destructor (round 37); ClosePauseMenu (round 51: its old
 * menu state SOCOM 1's MENU_STATE m_oldmenustate -- a signed char field cast to the enum loads with lbu); TickOffState (round 59: the
 * unit's first function, 0x2011F0, as SOCOM 1's; HudTick 0x201690, Init 0x202520, HandleSelectionInput
 * 0x2041E0 and OpenPauseMenu 0x204A40 named by SOCOM 1's calls, unwritten); HudTick, HandleSelectionInput and OpenPauseMenu (round 65); Init 0x202520 (round 71, ReInit inlined into it). Defined in reverse of their
 * image order (-inline deferred): the unit ends with the lines' test 0x205A70, the line's
 * constructor 0x205970 (generated before the menu's constructor, which takes its address) and the
 * menu's constructor 0x205800. */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zinput/zinput.h"

/* the HUD's menu state (hud_missioncams.h's; a byte under -enum min) */
enum MENU_STATE {
    stateNone = 0,
    stateWeaponSel = 1,
    stateOrders = 2,
    statePauseTestMenu = 3,
    stateFullScreenMapMenu = 4,
    stateFullScreenMapAnim = 5,
    stateLast = 6,
};

/* a menu line (0xA4 bytes: the menu's array of twelve is new'ed): a string first -- a member, not a
 * base: the line's implicit destructor (0x2057A0, which new[] and delete[] hand to the array
 * helpers) destroys the string with -1 -- then a word (0xDEADBEEF until set: 0x202520 stores each
 * line's string id there), a byte (12 until set) and a bit-field group at 0x9D (a word at a byte
 * offset: a 19-bit field there is written with lwr/lwl) whose first two flags the lines' test reads,
 * the first offline, the second online; the 27-bit field at 0xA0 only the constructor touches. The
 * names ours */
struct CPauseItem {
    CPauseItem();
    /* a line set up (Init's: the string id, the kind, the offline and online flags; a line with no
     * string gets the third flag; the names ours) */
    void Set(unsigned int id, unsigned char kind, bool f0, bool f1)
    {
        m_id = id;
        m_kind = kind;
        m_flag0 = f0;
        m_flag1 = f1;
        if (m_id == (unsigned int)-1)
            m_flag2 = true;
    }
    void SetFlags(bool f2, bool f3, bool f4)
    {
        m_flag2 = f2;
        m_flag3 = f3;
        m_flag4 = f4;
    }

    C2DString m_text;   // 0x0
    unsigned int m_id;   // 0x98
    unsigned char m_kind;   // 0x9c
    unsigned int m_flag0 : 1;   // 0x9d: selectable offline
    unsigned int m_flag1 : 1;   // selectable online
    unsigned int m_flag2 : 1;
    unsigned int m_flag3 : 1;
    unsigned int m_flag4 : 1;
    unsigned int _9d_5 : 19;
    unsigned int m_a0 : 27;   // 0xa0
    unsigned int _a0_27 : 5;
};

/* an object the menu keeps at 0xBD0 (no name yet): told when the menu opens, slot +0xC */
class CPauseSub {
public:
    virtual ~CPauseSub();
    virtual void Notify(int a, int b);
};

/* the pause menu, laid out from its destructor (0x205480): the lines through a pointer at 0x18, the
 * background (a bitmap of two triangles), bitmaps, strings and three more two-triangle bitmaps, the
 * open flag that On and Off keep and a flag that swaps the lines for the third panel; the members'
 * names are ours. Each of the three bitmap-polys and seven strings at 0xCF0 is destroyed on its
 * own, so they are members, not arrays */
class CZPauseTest : public C2D {
public:
    CZPauseTest();
    virtual ~CZPauseTest();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Off();
    virtual void On();
    void ReInit();
    /* the prompts', questions' and answers' texts (ReInit's; inline, the name ours: written out in
     * ReInit's body, ReInit is past -inline auto's fifteen statements, and the target inlines ReInit
     * into Init) */
    void SetStrings()
    {
        m_texta90.SetString("\xa6 ACCEPT", true);
        m_textb28.SetString("\xac BACK", true);
        m_textcf0.SetString("Are you sure you want", true);
        m_textd88.SetString("to abort the game?", true);
        m_texte20.SetString(" ", true);
        m_texteb8.SetString("Are you sure you want", true);
        m_textf50.SetString("to replay the mission?", true);
        m_textfe8.SetString(" ", true);
        m_text1080.SetString("YES", true);
        m_text1220.SetString("NO", true);
    }
    void Clear();
    void ClosePauseMenu();
    bool IsSelectable(signed char line);
    /* the menu closed: the start button opens it (SOCOM 1's TickOffState, 0x2011F0), the faded
     * parts fade out; and the menu's tick, set-up, pad and opening (SOCOM 1's HudTick, Init,
     * HudTick 0x201690, HandleSelectionInput 0x2041E0, OpenPauseMenu 0x204A40) */
    void TickOffState(const CPad &pad, float dT);
    void HudTick(CPad *pad, float dT);
    void Init();
    void HandleSelectionInput(const CPad &pad);
    void OpenPauseMenu();
    /* a line by its index (an int: the test re-extends the char line after the valve lookup, where
     * m_items[line] keeps the extension across the call in a saved register) */
    CPauseItem &Item(int i) { return m_items[i]; }

    CPauseItem *m_items;   // 0x18
    signed char m_sel;   // 0x1c: the line selected
    C2DFont *m_font;   // 0x20: the lines' font (Init loads every string with it)
    unsigned char _24[0x30 - 0x24];
    C2DBitmapPoly m_background;   // 0x30
    C2DBitmap m_panel;   // 0x220
    C2DBitmap m_bitmap2a0;   // 0x2a0
    C2DBitmap m_bitmap320;   // 0x320
    C2DBitmap m_bitmap3a0;   // 0x3a0
    C2DString m_text420;   // 0x420
    C2DBitmapPoly m_poly4c0;   // 0x4c0
    C2DBitmapPoly m_poly6b0;   // 0x6b0
    C2DBitmapPoly m_poly8a0;   // 0x8a0
    C2DString m_texta90;   // 0xa90
    C2DString m_textb28;   // 0xb28
    unsigned char m_open;   // 0xbc0
    unsigned char m_socom2_bc1;   // 0xbc1: cleared by ReInit
    float m_bc4;   // 0xbc4
    float m_bc8;   // 0xbc8
    MENU_STATE m_oldmenustate;   // 0xbcc: SOCOM 1's (0x364 there), put back on closing
    CPauseSub *m_bd0;   // 0xbd0
    CPauseSub *m_bd4;   // 0xbd4
    CPauseSub *m_bd8;   // 0xbd8
    unsigned char m_socom2_bdc;   // 0xbdc: the third panel drawn instead of the lines
    int m_be0;   // 0xbe0
    unsigned char m_be4;   // 0xbe4
    unsigned char _be5[0xbf0 - 0xbe5];
    C2DBitmap m_bitmapbf0;   // 0xbf0
    C2DBitmap m_panel2;   // 0xc70
    C2DString m_textcf0;   // 0xcf0
    C2DString m_textd88;   // 0xd88
    C2DString m_texte20;   // 0xe20
    C2DString m_texteb8;   // 0xeb8
    C2DString m_textf50;   // 0xf50
    C2DString m_textfe8;   // 0xfe8
    C2DString m_text1080;   // 0x1080
    C2DBitmap m_bitmap1120;   // 0x1120
    C2DBitmap m_bitmap11a0;   // 0x11a0
    C2DString m_text1220;   // 0x1220
    C2DBitmap m_bitmap12c0;   // 0x12c0
    C2DBitmap m_bitmap1340;   // 0x1340
} __attribute__((aligned(16)));   // 0x13c0 bytes (CHUD's member at 0x18900)

/* the menu's five colours (OVL1 bss 0x409020-0x40906B, 16 bytes apart: the unit's initialiser
 * 0x3FDB20 stores their fifteen words; SOCOM 1's pause menu had none; the names ours) */
static CPnt3D s_pauseColor0(11.0f, 21.0f, 24.0f);
static CPnt3D s_pauseColor1(35.0f, 51.0f, 56.0f);
static CPnt3D s_pauseColor2(11.0f, 45.0f, 64.0f);
static CPnt3D s_pauseColor3(35.0f, 51.0f, 56.0f);
static CPnt3D s_pauseColor4(128.0f, 128.0f, 128.0f);

/* the menu drawn while open, or while 0x409080 holds (a byte; no row named yet): the background,
 * then the third panel or the second and the twelve lines */
extern unsigned char data_00409080;
/* a second byte the menu keeps (OVL1 bss 0x409070; no row named yet) */
extern unsigned char data_00409070;

/* a word the menu keeps (OVL1 bss 0x409088; no row named yet): the seconds since the mission's
 * set-up (ReInit zeroes it, TickOffState counts it up to s_pauseDelay) */
extern float data_00409088;

/* the HUD (hud_main.cpp's, theHUD 0x408E58): turned back on once the menu has closed */
class CZNewHudMap {
public:
    void SwitchToCompassMap();

    unsigned char _0[0xdc];
    bool m_isFullScreen;   // 0xdc
};

class CHUD {
public:
    void On();
    void Off();
    static class CZSealBody *GetHUDOwner();

    unsigned char _0[0x11c90];
    CZNewHudMap *m_map;   // 0x11c90
};

extern CHUD *theHUD;

/* the network game (fts_net_game's theNetwork, 0x45A0C0): whether it is one */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* ClosePauseMenu's world (SOCOM 1's, grown for SOCOM II as hud_missioncams.cpp's): the menu state
 * put back, the voices resumed, the player's pad, controller, zoom state and kit, the lens effects
 * and the camera's zoom */
enum ZOOMSTATE {
    ZOOMSTATE_UNKNOWN = -1, ZOOMSTATE_3RDPERSON = 0, ZOOMSTATE_1STPERSON = 1, ZOOMSTATE_THERMAL = 2,
    ZOOMSTATE_NVG = 3, ZOOMSTATE_BINOCS = 4, ZOOMSTATE_WEAPONDEF1 = 5, ZOOMSTATE_WEAPONDEF2 = 6,
    ZOOMSTATE_WEAPONDEF3 = 7, ZOOMSTATE_WEAPONDEF4 = 8, ZOOMSTATE_WEAPONDEF5 = 9, ZOOMSTATE_WEAPONDEF6 = 10,
    ZOOMSTATE_WEAPONDEF7 = 11, ZOOMSTATE_WEAPONDEF8 = 12, NUM_ZOOMSTATES = 13
};

/* the controller (zseal.h's CSealCtrl: IsPlayer its slot +0x2c; the pad's index at 0x114) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool flag);
    void StopRumbler();
    virtual ~CSealCtrl();
    virtual bool PreTick(float dT);
    virtual bool Tick(float dT);
    virtual bool PostMortemPreTick(float dT);
    virtual void OnDeath();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void PlayAiSound();
    virtual void AllowCommand();
    virtual bool IsPlayer() const;   /* +0x2c */

    /* the menu state (SOCOM 1's GetMenuState, out of line there: SOCOM II reads it inline) */
    MENU_STATE GetMenuState() const { return m_menustate; }

    unsigned char _4[0x114 - 0x4];
    unsigned int m_port;   // 0x114: its pad
    unsigned char _118[0x170 - 0x118];
    unsigned int m_flags0 : 11;   // 0x170
    unsigned int m_input_enabled : 1;   // bit 11 (zseal.h's)
    unsigned char _172[0x221 - 0x172];
    MENU_STATE m_menustate;   // 0x221
};

class CZKit {
public:
    void Apply_LensEffects(bool on, CPnt3D *color);

    unsigned int m_bits0 : 15;
    unsigned int m_heartbeat_enabled : 1;
};

class CZSealBody {
public:
    /* whether a player's controller drives it (inline: both builds call through a null test) */
    bool IsPlayer() { return m_ctrl ? m_ctrl->IsPlayer() : false; }

    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0x200 - 0xc4];
    ZOOMSTATE m_zoomstate;   // 0x200
    unsigned char _201[0x5e0 - 0x201];
    CZKit m_kit;   // 0x5e0
};

CZSealBody *ftsGetPlayer();

class CSndInstance {
public:
    static void ResumeAllSounds();
    static void PauseAllSounds();
};

class CSnd {
public:
    static void PostTick(float dT, void *arg);
    static CPauseSub *GetSoundByName(const char *name);
};

class CameraZoom {
public:
    static void ReleaseLensFx(unsigned char hold);
    static void HoldLensFx(unsigned char hold);
    static unsigned char LensFxOn();
};

namespace zdb {
class CWorld {
public:
    static CWorld *m_world;

    unsigned char _0[Z_OFS(0x5dc, 0x5e8)];   /* the host's: CNode's vptr first, its 16-aligned base after */
    unsigned char m_night_mission;   // 0x5dc (host 0x5e8)
    unsigned char _5dd[3];
    PNT4D m_lensfx_nvg;   // 0x5e0
    PNT4D m_lensfx_starlightscope;   // 0x5f0
};
}

class CCameraZoomView {
public:
    unsigned char _0[0x470];
    float m_zoom;   // 0x470
};

class CAppCamera {
public:
    void SetZoom(float zoom);

    unsigned char _0[0xb4];
    CCameraZoomView *m_camera;   // 0xb4: the camera, its zoom
};

extern CAppCamera *appCamera;

/* the camera's zoom the menu put aside (OVL1 .data 0x3DC410, 1.0; the name ours) */
static float s_pauseCamZoom = 1.0f;

/* how long the menu waits after a mission's set-up before the start button opens it (OVL1 .data
 * 0x3DC420, 2.0 seconds; TickOffState's; the name ours) */
static float s_pauseDelay = 2.0f;

/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* whether the network game is the online one (OVL1 0x2C2FA0, no row named yet) */
extern "C" bool func_002C2FA0();

/* the LAN lobby (zNetLANLobby.cpp's, 0x44FE10): whether the menus came from the LAN menu */
class CZLANLobby {
public:
    bool IsActive();
};

extern CZLANLobby data_0044FE10;

/* a valve (zvalve.h's): its value a 16-bit field */
enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };
enum VALVE_STATE { VALVE_SAME, VALVE_ON, VALVE_OFF };

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    static CValve *GetByName(const char *name);
    void MakeCallbacks(VALVE_STATE state);
    /* the value set, the callbacks told when it turned on or off (zanim.h's) */
    void SetValue(int value)
    {
        VALVE_STATE state = VALVE_SAME;

        if (m_value != value)
            state = value == 0 ? VALVE_OFF : VALVE_ON;
        m_value = value;
        if (state != VALVE_SAME)
            MakeCallbacks(state);
    }

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
};

/* whether a line can be chosen (0x205A70, the unit's last function; the name ours): online, the
 * line's second flag -- not the fifth and sixth lines in the online game, nor the eleventh in
 * the LAN game, nor the twelfth while the menus came from the LAN menu; offline its first flag,
 * and the eighth line only while the NoAutoAim valve is off */
bool CZPauseTest::IsSelectable(signed char line)
{
    bool r;

    if (theNetwork.m_bNetwork) {
        if (func_002C2FA0()) {
            switch (line) {
            case 5:
            case 4:
                r = false;
                break;
            default:
                r = Item(line).m_flag1;
                break;
            }
        } else {
            switch (line) {
            case 10:
                r = false;
                break;
            default:
                r = Item(line).m_flag1;
                break;
            }
        }
        if (line == 11 && data_0044FE10.IsActive())
            r = false;
    } else {
        switch (line) {
        case 7: {
            CValve *valve = CValve::GetByName("NoAutoAim");

            r = Item(line).m_flag0;
            if (r && valve)
                r = valve->m_value == 0;
            break;
        }
        default:
            r = Item(line).m_flag0;
            break;
        }
    }
    return r;
}

/* the flag group as the line's constructor writes it (ours: A MODELLING DEVICE, the HUD's round 64).
 * MW narrows a bit-field's read-modify-write to the byte by the field's DECLARATION, unless the
 * declared type is template-dependent (MAIN's runtime, round 7: `typename Allocator::size_type`
 * fields stay words) -- then the access is the type's whole word at the group's byte offset, here
 * the unaligned word at 0x9D (lwr/lwl 157..160, swr/swl). The target's constructor is the only
 * read-modify-write of that shape in the image (build/scratch/hud/swrscan.py), while its readers
 * (IsSelectable) and 0x202520's line set-up narrow the same flags (lbu/sb 157) -- so one declaration
 * cannot give both: CPauseItem keeps the plain fields, the constructor goes through this view. Whatever
 * in the original gave the constructor its own type is not known */
template <class T> struct CPauseItemBits {
    void SetFlag0(bool b) { m_flag0 = b; }
    void SetFlag1(bool b) { m_flag1 = b; }
    void SetFlag2(bool b) { m_flag2 = b; }
    void SetFlag3(bool b) { m_flag3 = b; }
    void SetFlag4(bool b) { m_flag4 = b; }
    void SetA0(unsigned int v) { m_a0 = v; }

    unsigned char _0[0x9d];
    T m_flag0 : 1;   // 0x9d
    T m_flag1 : 1;
    T m_flag2 : 1;
    T m_flag3 : 1;
    T m_flag4 : 1;
    T _9d_5 : 19;
    T m_a0 : 27;   // 0xa0
    T _a0_27 : 5;
};

/* a line made empty (0x205970: its address handed to __construct_new_array by the menu's
 * constructor; generated before it, so not an inline's copy). Open from round 37 to 64 at 10
 * instructions (0xD0 vs 0xF8): every plain spelling narrows -- int, unsigned long, unsigned short, a
 * 4-byte enum and volatile fields; group sizes 5-32 bits; packed structs and #pragma pack(1); nested
 * structs; template setters over a plain field (the narrowing is the declaration's); #pragma
 * opt_scalarizebitfields, opt_lifetimes; a whole template base holding the fields matches this row but
 * loses IsSelectable and ~CPauseItem */
CPauseItem::CPauseItem()
{
    m_id = 0xdeadbeef;
    m_kind = 12;
    CPauseItemBits<unsigned int> *bits = (CPauseItemBits<unsigned int> *)this;

    bits->SetFlag0(false);
    bits->SetFlag1(false);
    bits->SetFlag2(false);
    bits->SetFlag3(false);
    bits->SetFlag4(false);
    bits->SetA0(0);
}

/* the menu made closed (CHUD's constructor builds CHUD+0x18900 with it): the members built, the menu
 * turned off, its state cleared, the twelve lines new'ed */
CZPauseTest::CZPauseTest()
{
    Off();
    m_bc4 = 0;
    m_bc8 = 50.0f;
    m_font = 0;
    m_oldmenustate = stateNone;
    m_bd0 = 0;
    m_bd4 = 0;
    m_bd8 = 0;
    m_sel = -1;
    m_items = new CPauseItem[12];
    m_socom2_bdc = 0;
    m_be0 = -1;
    m_be4 = 0;
}

/* the lines cleared and their array deleted, then the members (OVL1 0x205480; the table 0x405320
 * comes with it): the bitmaps' and polys' implicit destructors inline, the strings' out of line */
CZPauseTest::~CZPauseTest()
{
    Clear();
    delete[] m_items;
    m_items = 0;
}

/* the menu closed (CHUD::ReInit_Uninit's first call; SOCOM 1's, 0x1AC there): the voices resumed,
 * the player's pad's four face buttons let go, the menu state put back; offline with the player
 * driving, the night mission's lens colour put back unless a lens effect is up, the camera's zoom
 * given back and the lens effects released; the heartbeat sensor on, the menu off and its state
 * cleared */
void CZPauseTest::ClosePauseMenu()
{
    if (!m_open)
        return;
    CSndInstance::ResumeAllSounds();

    CZSealBody *seal = ftsGetPlayer();
    CPad *pad = GetPad(seal->m_ctrl->m_port);

    if (pad) {
        pad->SetButtonState(PAD_CROSS, KEY_UP);
        pad->SetButtonState(PAD_SQUARE, KEY_UP);
        pad->SetButtonState(PAD_TRIANGLE, KEY_UP);
        pad->SetButtonState(PAD_CIRCLE, KEY_UP);
    }
    seal->m_ctrl->SetMenuState(m_oldmenustate, false);

    ZOOMSTATE zoomstate = seal->m_zoomstate;
    zdb::CWorld *world = zdb::CWorld::m_world;
    CZKit *kit = &seal->m_kit;

    if (!theNetwork.m_bNetwork && seal->IsPlayer()) {
        if (!CameraZoom::LensFxOn() && world->m_night_mission && seal->IsPlayer()) {
            if (zoomstate == ZOOMSTATE_NVG) {
                PNT4D color = world->m_lensfx_nvg;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            } else if (zoomstate >= ZOOMSTATE_BINOCS) {
                PNT4D color = world->m_lensfx_starlightscope;

                kit->Apply_LensEffects(true, (CPnt3D *)&color);
            }
        }
        if (s_pauseCamZoom != 1.0f) {
            appCamera->SetZoom(s_pauseCamZoom);
            s_pauseCamZoom = 1.0f;
        }
        CameraZoom::ReleaseLensFx(2);
    }
    if (kit)
        kit->m_heartbeat_enabled = true;
    Off();
    m_socom2_bc1 = 1;
    data_00409080 = 1;
    m_socom2_bdc = 0;
    m_be4 = 0;
    m_be0 = -1;
}

/* the menu set up for a mission: the first line that can be selected (wrapping after the twelfth),
 * the accept and back prompts, the two questions, the answers (CHUD::ReInit_Init's call; the name
 * ours) */
void CZPauseTest::ReInit()
{
    signed char sel = -1;
    bool found = false;

    /* the step cast back to a signed char: MW re-extends the line before adding, as the target
     * (an increment or `++sel` adds to the register as it stands) */
    do {
        sel = (signed char)(sel + 1);
        if (sel == 12)
            sel = 0;
        if (IsSelectable(sel))
            found = true;
    } while (!found);
    m_sel = sel;
    m_socom2_bc1 = 0;
    SetStrings();
    data_00409088 = 0;
}

/* both flags down, every line and string wiped, the children let go (the line index narrowed to a
 * signed char, as Draw's) */
void CZPauseTest::Clear()
{
    data_00409080 = 0;
    data_00409070 = 0;
    for (unsigned int i = 0; i < 12; i++)
        m_items[(signed char)i].m_text.ClearAndWipe();
    m_texta90.ClearAndWipe();
    m_textb28.ClearAndWipe();
    m_text420.ClearAndWipe();
    m_textcf0.ClearAndWipe();
    m_textd88.ClearAndWipe();
    m_texte20.ClearAndWipe();
    m_texteb8.ClearAndWipe();
    m_textf50.ClearAndWipe();
    m_textfe8.ClearAndWipe();
    m_text1080.ClearAndWipe();
    m_text1220.ClearAndWipe();
    clear();
}

void CZPauseTest::Draw(zdb::CCamera *cam)
{
    if (!m_open && !data_00409080)
        return;
    m_background.Draw(cam);
    if (m_socom2_bdc) {
        m_panel2.Draw(cam);
        return;
    }
    m_panel.Draw(cam);
    /* an int counter, the index narrowed to a signed char each pass (sign-extended, never
     * strength-reduced, as the target; a signed char counter is reduced to a walking pointer) --
     * likely an accessor taking a char */
    for (int i = 0; i < 12; i++)
        m_items[(signed char)i].m_text.Draw(cam);
}

void CZPauseTest::Off()
{
    m_open = 0;
    C2D::Off();
}

void CZPauseTest::On()
{
    m_open = 1;
    C2D::On();
}

/* the radio menu and the voice recogniser (hud_tcm.h's, reco.cpp's) */
class CTCM {
public:
    void Close(bool now);
};

extern CTCM theTCM;

class CReco {
public:
    void Suspend(bool on);
};

extern CReco theReco;

/* the options the lines show (their owners' globals: the look's inversion, the auto-aim, the text
 * messages, the fire-mode swap, the run-and-talk, the voice modulation, the spectator HUD; the last
 * three have no rows yet) */
extern float hackPitchFactor;
extern unsigned char _AutoAim;
extern unsigned char g_bExternalWhispers;
extern unsigned char FireModeSwapIsOn;
extern unsigned char data_0044D4F8;
extern unsigned char data_003DF1A8;
extern unsigned char data_003DC3E0;
extern float data_00409078;
bool IsRumbleOn();
bool hudHelpWindowsEnabled();
const char *GetString(int id);
const char *TranslateString(const char *s);

/* the mission (fts_mission's) and its name (no row name yet) */
class CMission;
extern CMission theMission;
extern "C" const char *func_002A42A0(CMission *mission);

/* the menu opened (SOCOM 1's OpenPauseMenu, 0x3BF210 there, its body much grown): the radio and the
 * recogniser shut, the rumble stopped, the player's pad's start held, offline the sounds paused; the
 * map back to the compass, the menu state kept and set, the HUD off and the menu's parts on; offline
 * with the player driving, the lens effects off and the camera's zoom put aside; every line's text
 * by its kind and the options' states, centred; the mission's name */
void CZPauseTest::OpenPauseMenu()
{
    theTCM.Close(false);
    theReco.Suspend(true);

    CZSealBody *seal = ftsGetPlayer();
    CSealCtrl *ctrl = seal->m_ctrl;

    ctrl->StopRumbler();

    CPad *pad = GetPad(ctrl->m_port);

    if (pad)
        pad->SetButtonState(PAD_START, KEY_DOWN);
    if (!theNetwork.m_bNetwork) {
        CSndInstance::PauseAllSounds();
        CSnd::PostTick(0.0f, 0);
    }
    On();
    CHUD *hud = theHUD;
    CZNewHudMap *map = hud->m_map;

    if ((map ? map->m_isFullScreen : false) && map)
        hud->m_map->SwitchToCompassMap();
    m_oldmenustate = (MENU_STATE)*(signed char *)&ctrl->m_menustate;
    ctrl->SetMenuState(statePauseTestMenu, false);
    theHUD->Off();
    m_background.On();
    m_panel.On();
    m_bitmap2a0.On();
    m_bitmap320.On();
    m_bitmap3a0.On();
    m_poly6b0.On();
    m_texta90.On();
    m_textb28.On();
    m_sel = 0;
    m_poly6b0.SetPos(171.0f, (float)(int)(m_items->m_text.m_posy + m_items->m_text.m_socom2_80) - 21.0f - -2.0f, 297.0f,
                     21.0f);
    if (!theNetwork.m_bNetwork && seal->IsPlayer()) {
        if (!CameraZoom::LensFxOn())
            seal->m_kit.Apply_LensEffects(false, 0);
        if (s_pauseCamZoom == 1.0f) {
            s_pauseCamZoom = appCamera->m_camera->m_zoom;
            appCamera->SetZoom(1.0f);
        }
        CameraZoom::HoldLensFx(2);
    }
    if (seal->IsPlayer()) {
        CZKit *kit = &seal->m_kit;

        if (kit)
            kit->m_heartbeat_enabled = false;
    }
    m_background.SetTrans(0, 100.0f);
    m_background.SetTrans(3, 100.0f);
    m_panel.SetTrans(80.0f);
    m_bitmap2a0.SetTrans(100.0f);
    m_poly6b0.SetTrans(100.0f);
    m_bitmap320.SetTrans(80.0f);
    m_bitmap3a0.SetTrans(100.0f);
    m_text420.SetTrans(110.0f);
    m_texta90.SetTrans(110.0f);
    m_textb28.SetTrans(110.0f);
    data_00409078 = 0.0f;
    for (unsigned int i = 0; i < 12; i++) {
        CPauseItem *item;
        CPauseItem *items = m_items;

        item = &items[(signed char)i];

        C2DString *text = &item->m_text;

        if (!item->m_flag2) {
            text->SetString(GetString(item->m_id), false);
        } else {
            switch ((signed char)item->m_kind) {
            case 3:
                ((C2DString *)&items[3].m_text)->SetString(GetString(hackPitchFactor < 0.0f ? 0xEC67 : 0xEC68), false);
                break;
            case 6: {
                int id = hudHelpWindowsEnabled() ? 0xEC65 : 0xEC66;
                C2DString *help = &m_items[6].m_text;

                help->SetString(GetString(id), false);
                break;
            }
            case 7:
                ((C2DString *)&items[7].m_text)->SetString(GetString(_AutoAim ? 0xEC63 : 0xEC64), false);
                break;
            case 11:
                ((C2DString *)&items[11].m_text)->SetString(g_bExternalWhispers ? "IN-GAME TEXT MESSAGES : ON" : "IN-GAME TEXT MESSAGES : OFF", true);
                break;
            case 8: {
                const char *t = IsRumbleOn() ? "VIBRATION: ON" : "VIBRATION OFF";

                ((C2DString *)&m_items[8].m_text)->SetString(t, true);
                break;
            }
            case 5:
                ((C2DString *)&items[5].m_text)->SetString(data_0044D4F8 ? "RUN AND TALK: ON" : "RUN AND TALK: OFF", true);
                break;
            case 9:
                ((C2DString *)&items[9].m_text)->SetString(theNetwork.m_bNetwork ? (FireModeSwapIsOn ? "FIREMODE/SCOREBOARD: ALTERNATE" : "FIREMODE/SCOREBOARD: DEFAULT")
                                                                               : (FireModeSwapIsOn ? "FIREMODE/TACMAP: ALTERNATE" : "FIREMODE/TACMAP: DEFAULT"),
                                                           true);
                break;
            case 4:
                ((C2DString *)&items[4].m_text)->SetString(data_003DF1A8 ? "VOICE MODULATION: ON" : "VOICE MODULATION: OFF", true);
                break;
            case 10:
                ((C2DString *)&items[10].m_text)->SetString(data_003DC3E0 ? "SPECTATOR HUD: ON" : "SPECTATOR HUD: OFF", true);
                break;
            default:
                text->SetString("DYNAMIC TEXT: MISSING", true);
                break;
            }
        }
        item->m_text.m_isCentered = 1;
        item->m_text.m_color[0] = 100.0f;
        item->m_text.m_color[1] = 100.0f;
        item->m_text.m_color[2] = 100.0f;
        text->SetTrans(128.0f);
        item->m_text.UpdatePos();
    }
    m_text420.SetString(TranslateString(func_002A42A0(&theMission)), true);
    m_bc4 = 1.0f;
    m_socom2_bdc = 0;
    m_be4 = 0;
    m_be0 = -1;
    if (m_bd0)
        m_bd0->Notify(0, 255);
}

/* the valve the quit confirmation sets (no row name yet); the word the delete-from-map line clears
 * (no row name yet); the voice object and its modulation (hud_mpinfobox.cpp's data_0045B718) */
#define data_0043668C (*(CValve **)((char *)&::theMission + 0x1AC))   /* theMission+0x1AC */
extern float data_003DC418;
extern void *data_0045B718;
extern "C" void func_0030E5A0(void *voice, bool off);

void hudToggleHelpWindows();
void EnableExternalWhispers(bool on);
void SetRumbleOn(bool on);
void ToggleFireModeSwap();

/* the pad, with the menu open (SOCOM 1's HandleSelectionInput, much grown): start closes it; with the
 * third panel up, the triangle or the cross answers it -- a line's kind kept in m_be0 sets the valve,
 * and a confirmation closes the menu; otherwise the cross runs the selected line by its kind: the
 * panels swapped, or an option toggled and its line's text put right */
void CZPauseTest::HandleSelectionInput(const CPad &pad)
{
    char sel = 3;
    /* a device: an int, so that the triangle's `cross = 3` is a plain copy of the compare's constant
     * (a char local narrows it with dsll32/dsra32); the byte itself is sign-extended, lb, and its tests
     * mask it again */
    int cross = (signed char)pad.m_state[PAD_CROSS];

    if ((unsigned char)cross == KEY_RISING)
        sel = 0;
    if (pad.m_state[PAD_START] == KEY_FALLING)
        ClosePauseMenu();
    if (m_socom2_bdc) {
        if (pad.m_state[PAD_TRIANGLE] == KEY_RISING) {
            cross = 3;
            m_be4 = 0;
            m_be0 = -1;
        }
        if ((unsigned char)cross == KEY_RISING) {
            if (m_be4) {
                if (m_be0 == 2) {
                    data_0043668C->SetValue(1);
                    CHUD::GetHUDOwner()->m_ctrl->StopRumbler();
                } else if (m_be0 == 1) {
                    data_0043668C->SetValue(2);
                }
                data_00409070 = 1;
                m_socom2_bdc = 0;
                m_be4 = 0;
                m_be0 = -1;
                ClosePauseMenu();
            } else {
                m_socom2_bdc = 0;
                m_be4 = 0;
                m_be0 = -1;
            }
            if (m_bd0)
                m_bd0->Notify(0, 255);
        }
        return;
    }
    if (pad.m_state[PAD_START] != KEY_DOWN && pad.m_state[PAD_TRIANGLE] == KEY_RISING)
        ClosePauseMenu();
    if ((unsigned char)sel != 3 && m_bd0)
        m_bd0->Notify(0, 255);
    if ((unsigned char)sel != 3) {
        CPauseItem *item = &m_items[m_sel];

        switch ((signed char)item->m_kind) {
        case 2:
            m_textcf0.On();
            m_textd88.On();
            m_texte20.On();
            m_texteb8.Off();
            m_textf50.Off();
            m_textfe8.Off();
            m_socom2_bdc = 1;
            m_be0 = 2;
            break;
        case 3: {
            hackPitchFactor *= -1.0f;
            CValve *valve = CValve::Create("INVERT_PITCH_CONTROLLER", VALVE_TEMP);

            valve->SetValue(!(hackPitchFactor > 0.0f));
            int id = hackPitchFactor < 0.0f ? 0xEC67 : 0xEC68;

            C2DString *line = &m_items[3].m_text;

            line->SetString(GetString(id), false);
            break;
        }
        case 6: {
            hudToggleHelpWindows();
            int id = hudHelpWindowsEnabled() ? 0xEC65 : 0xEC66;

            C2DString *line = &m_items[6].m_text;

            line->SetString(GetString(id), false);
            break;
        }
        case 7: {
            _AutoAim = !_AutoAim;
            CValve *valve = CValve::GetByName("AutoAim");

            if (valve)
                valve->SetValue(_AutoAim);
            int id = _AutoAim ? 0xEC63 : 0xEC64;
            C2DString *line = &m_items[7].m_text;

            line->SetString(GetString(id), false);
            break;
        }
        case 11:
            EnableExternalWhispers(!g_bExternalWhispers);
            m_items[11].m_text.SetString(g_bExternalWhispers ? "IN-GAME TEXT MESSAGES : ON" : "IN-GAME TEXT MESSAGES : OFF", true);
            break;
        case 8: {
            SetRumbleOn(!IsRumbleOn());
            CValve *valve = CValve::GetByName("VIBRATION_CONTROLLER");

            if (valve)
                valve->SetValue(IsRumbleOn());
            const char *t = IsRumbleOn() ? "VIBRATION: ON" : "VIBRATION OFF";

            m_items[8].m_text.SetString(t, true);
            break;
        }
        case 4: {
            bool on = !data_003DF1A8;

            data_003DF1A8 = on;
            m_items[4].m_text.SetString(on ? "VOICE MODULATION: ON" : "VOICE MODULATION: OFF", true);
            CValve *valve = CValve::GetByName("VoiceMask");

            if (valve)
                valve->SetValue(data_003DF1A8);
            func_0030E5A0(data_0045B718, !data_003DF1A8);
            break;
        }
        case 10: {
            bool on = !data_003DC3E0;

            data_003DC3E0 = on;
            m_items[10].m_text.SetString(on ? "SPECTATOR HUD: ON" : "SPECTATOR HUD: OFF", true);
            break;
        }
        case 5: {
            bool on = !data_0044D4F8;

            data_0044D4F8 = on;
            m_items[5].m_text.SetString(on ? "RUN AND TALK: ON" : "RUN AND TALK: OFF", true);
            break;
        }
        case 9:
            FireModeSwapIsOn = !FireModeSwapIsOn;
            ToggleFireModeSwap();
            m_items[9].m_text.SetString(theNetwork.m_bNetwork ? (FireModeSwapIsOn ? "FIREMODE/SCOREBOARD: ALTERNATE" : "FIREMODE/SCOREBOARD: DEFAULT")
                                                              : (FireModeSwapIsOn ? "FIREMODE/TACMAP: ALTERNATE" : "FIREMODE/TACMAP: DEFAULT"),
                                        true);
            break;
        case 1:
            m_textcf0.Off();
            m_textd88.Off();
            m_texte20.Off();
            m_texteb8.On();
            m_textf50.On();
            m_textfe8.On();
            m_socom2_bdc = 1;
            m_be0 = 1;
            break;
        }
        if (item->m_flag3)
            data_003DC418 = 0;
        if (item->m_flag4)
            ClosePauseMenu();
    }
}

/* the statics of the menu's tick (OVL1 .data 0x3DC428-0x3DC43B, the top of bss 0x409090-0x4090A0;
 * no rows named yet): the selection bar's fast speed, the delay and the elapsed time between pad moves
 * and whether one may come, the selected line's pulse and its direction */
static float s_pauseBarFast = 3.0f;
static float s_pauseMoveDelay = 0.2f;
static float s_pausePulseDir = 1.0f;
extern float data_00409090;
extern unsigned char data_00409098;
extern float data_004090A0;

/* the screen's size (zVid's, OVL1 bss 0x4A44C4/0x4A44C8; no rows) */
extern unsigned int data_004A44C4;
extern unsigned int data_004A44C8;

/* the menu built (CHUD::Init's call; SOCOM II's own -- SOCOM 1's is 0x28): the twelve lines' string
 * ids, kinds and flags, the three sounds, the lines placed down the panel, the background, the panel
 * and its bars, the mission's name and the prompts, then the third panel -- its questions and its
 * yes/no boxes -- each part a child of the one before; ReInit's set-up last (inlined: the target's
 * tail is ReInit's body, so ReInit's strings went into the inline SetStrings to bring it under -inline
 * auto's fifteen statements). Each line's flags through two inline setters taking bools: a `false`
 * argument stays `andi rX,zero,1` before its shift, a `true` folds to the constant. The yes/no boxes'
 * right edges are written out in each SetPos, `4.0f + (40.0f + yes)` then `40.0f + yes` (a `right`
 * local assigned before the call materialises 40.0 ahead of the outer box's 253.0, 15 off); the box
 * edges are float locals, their (int) conversions kept in s2-s5 across both boxes */
void CZPauseTest::Init()
{
    m_items[0].Set(0xec60, 0, true, true);
    m_items[0].SetFlags(false, true, true);
    m_items[1].Set(0xec62, 1, true, false);
    m_items[1].SetFlags(false, true, false);
    m_items[2].Set(0xec61, 2, true, true);
    m_items[2].SetFlags(false, true, false);
    m_items[3].Set(0xec67, 3, true, true);
    m_items[3].SetFlags(true, true, false);
    m_items[8].Set(-1, 8, true, true);
    m_items[8].SetFlags(true, true, false);
    m_items[5].Set(-1, 5, false, true);
    m_items[5].SetFlags(true, true, false);
    m_items[9].Set(-1, 9, true, true);
    m_items[9].SetFlags(true, true, false);
    m_items[4].Set(-1, 4, false, true);
    m_items[4].SetFlags(true, true, false);
    m_items[6].Set(0xec65, 6, true, false);
    m_items[6].SetFlags(true, true, false);
    m_items[7].Set(0xec63, 7, true, false);
    m_items[7].SetFlags(true, true, false);
    m_items[11].Set(-1, 11, false, true);
    m_items[11].SetFlags(true, true, false);
    m_items[10].Set(-1, 10, false, true);
    m_items[10].SetFlags(true, true, false);
    m_bd0 = CSnd::GetSoundByName(".TCM_SELECT");
    m_bd4 = CSnd::GetSoundByName(".TCM_SLIDE");
    m_bd8 = CSnd::GetSoundByName(".TCM_SLIDE");

    unsigned int n = 0;

    for (unsigned int i = 0; i < 12; i++) {
        signed char line = i;
        CPauseItem *item = &m_items[line];

        if (IsSelectable(line)) {
            item->m_text.Load(1.1f, "UNUSED", m_font, (float)(data_004A44C4 / 2), 155.0f + 21.0f * (float)n++, true);
            item->m_text.m_isCentered = 1;
            item->m_text.SetColor(100.0f, 100.0f, 100.0f);
            item->m_text.SetTrans(128.0f);
            item->m_text.On();
        } else {
            item->m_text.Off();
        }
    }

    m_background.SetHasTexture(false);
    m_background.SetPos(0.0f, 0.0f, (float)data_004A44C4, (float)data_004A44C8);
    m_background.SetColor(0.0f, 0.0f, 0.0f);
    m_background.SetIsTrans(true);
    m_background.SetTrans(1, 0.0f);
    m_background.SetTrans(2, 0.0f);
    m_background.Off();

    m_panel.SetHasTexture(false);
    m_panel.Load(164.0f, 119.0f, 476.0f, 361.0f, 0);
    m_panel.SetColor(s_pauseColor1.x, s_pauseColor1.y, s_pauseColor1.z);
    m_panel.SetTrans(80.0f);
    m_panel.SetIsTrans(true);
    m_panel.Off();

    m_bitmap2a0.SetHasTexture(false);
    m_bitmap2a0.Load(170.0f, 125.0f, 470.0f, 355.0f, 0);
    m_bitmap2a0.SetColor(s_pauseColor0.x, s_pauseColor0.y, s_pauseColor0.z);
    m_bitmap2a0.SetTrans(100.0f);
    m_bitmap2a0.SetIsTrans(true);
    m_bitmap2a0.Off();
    m_panel.push_back(&m_bitmap2a0);

    m_bitmap320.SetHasTexture(false);
    m_bitmap320.Load(164.0f, 56.0f, 476.0f, 106.0f, 0);
    m_bitmap320.SetColor(s_pauseColor1.x, s_pauseColor1.y, s_pauseColor1.z);
    m_bitmap320.SetTrans(80.0f);
    m_bitmap320.SetIsTrans(true);
    m_bitmap320.Off();
    m_bitmap2a0.push_back(&m_bitmap320);

    m_bitmap3a0.SetHasTexture(false);
    m_bitmap3a0.Load(170.0f, 62.0f, 470.0f, 100.0f, 0);
    m_bitmap3a0.SetColor(s_pauseColor0.x, s_pauseColor0.y, s_pauseColor0.z);
    m_bitmap3a0.SetTrans(100.0f);
    m_bitmap3a0.SetIsTrans(true);
    m_bitmap3a0.Off();
    m_bitmap320.push_back(&m_bitmap3a0);

    m_text420.Load(1.3f, " ", m_font, 0.0f, 0.0f, true);
    m_text420.SetPos(data_004A44C4 / 2, 92);
    m_text420.SetString("MISSION NAME", true);
    m_text420.SetColor(128.0f, 128.0f, 64.0f);
    m_text420.SetTrans(110.0f);
    m_text420.m_isCentered = 1;
    m_text420.UpdatePos();
    m_bitmap3a0.push_back(&m_text420);

    m_poly6b0.SetHasTexture(false);
    m_poly6b0.SetColor(s_pauseColor2.x, s_pauseColor2.y, s_pauseColor2.z);
    m_poly6b0.SetTrans(100.0f);
    m_poly6b0.SetIsTrans(true);
    m_poly6b0.Off();
    m_bitmap2a0.push_back(&m_poly6b0);

    m_texta90.Load(0.8f, "\xa6 ACCEPT", m_font, 0.0f, 0.0f, true);
    m_texta90.SetPos(245, 345);
    m_texta90.SetString(" ", true);
    m_texta90.SetColor(128.0f, 128.0f, 64.0f);
    m_texta90.SetTrans(110.0f);
    m_bitmap2a0.push_back(&m_texta90);

    m_textb28.Load(0.8f, "\xac BACK", m_font, 0.0f, 0.0f, true);
    m_textb28.SetPos(337, 345);
    m_textb28.SetString(" ", true);
    m_textb28.SetColor(128.0f, 128.0f, 64.0f);
    m_textb28.SetTrans(110.0f);
    m_bitmap2a0.push_back(&m_textb28);

    m_panel2.SetPos((int)((float)(data_004A44C4 / 2) - 128.0f - 10.0f), 125,
                    (int)(10.0f + (128.0f + (float)(data_004A44C4 / 2))), 275);
    m_panel2.SetHasTexture(false);
    m_panel2.SetColor(s_pauseColor1);
    m_panel2.SetTrans(80.0f);

    m_bitmapbf0.SetPos((int)((float)(data_004A44C4 / 2) - 128.0f), 135,
                       (int)(128.0f + (float)(data_004A44C4 / 2)), 265);
    m_bitmapbf0.SetHasTexture(false);
    m_bitmapbf0.SetColor(s_pauseColor0);
    m_bitmapbf0.SetTrans(100.0f);
    m_bitmapbf0.On();
    m_panel2.push_back(&m_bitmapbf0);

    m_textcf0.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_textcf0.SetPos(data_004A44C4 / 2, 165);
    m_textcf0.m_isCentered = 1;
    m_textcf0.SetColor(100.0f, 100.0f, 100.0f);
    m_textcf0.UpdatePos();
    m_textcf0.On();
    m_bitmapbf0.push_back(&m_textcf0);

    m_textd88.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_textd88.SetPos(data_004A44C4 / 2, 185);
    m_textd88.m_isCentered = 1;
    m_textd88.SetColor(100.0f, 100.0f, 100.0f);
    m_textd88.UpdatePos();
    m_textd88.On();
    m_bitmapbf0.push_back(&m_textd88);

    m_texte20.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_texte20.SetPos(data_004A44C4 / 2, 205);
    m_texte20.m_isCentered = 1;
    m_texte20.SetColor(100.0f, 100.0f, 100.0f);
    m_texte20.UpdatePos();
    m_texte20.On();
    m_bitmapbf0.push_back(&m_texte20);

    m_texteb8.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_texteb8.SetPos(data_004A44C4 / 2, 165);
    m_texteb8.m_isCentered = 1;
    m_texteb8.SetColor(100.0f, 100.0f, 100.0f);
    m_texteb8.UpdatePos();
    m_texteb8.On();
    m_bitmapbf0.push_back(&m_texteb8);

    m_textf50.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_textf50.SetPos(data_004A44C4 / 2, 185);
    m_textf50.m_isCentered = 1;
    m_textf50.SetColor(100.0f, 100.0f, 100.0f);
    m_textf50.UpdatePos();
    m_textf50.On();
    m_bitmapbf0.push_back(&m_textf50);

    m_textfe8.Load(1.1f, "UNUSED", m_font, 0.0f, 0.0f, true);
    m_textfe8.SetPos(data_004A44C4 / 2, 205);
    m_textfe8.m_isCentered = 1;
    m_textfe8.SetColor(100.0f, 100.0f, 100.0f);
    m_textfe8.UpdatePos();
    m_textfe8.On();
    m_bitmapbf0.push_back(&m_textfe8);

    float yes = (float)(data_004A44C4 / 2) - 60.0f;
    float no = 60.0f + (float)(data_004A44C4 / 2);
    float top = 223.0f;
    float bottom = 249.0f;

    m_bitmap11a0.On();
    m_bitmap11a0.SetHasTexture(false);
    m_bitmap11a0.SetColor(s_pauseColor3);
    m_bitmap11a0.SetPos((int)(yes - 40.0f - 4.0f), (int)(top - 4.0f), (int)(4.0f + (40.0f + yes)), (int)(bottom + 4.0f));
    m_bitmapbf0.push_back(&m_bitmap11a0);

    m_bitmap1120.On();
    m_bitmap1120.SetHasTexture(false);
    m_bitmap1120.SetColor(s_pauseColor3);
    m_bitmap1120.SetPos((int)(yes - 40.0f), (int)top, (int)(40.0f + yes), (int)bottom);
    m_bitmapbf0.push_back(&m_bitmap1120);

    m_bitmap1340.On();
    m_bitmap1340.SetHasTexture(false);
    m_bitmap1340.SetColor(s_pauseColor3);
    m_bitmap1340.SetPos((int)(no - 40.0f - 4.0f), (int)(top - 4.0f), (int)(4.0f + (40.0f + no)), (int)(bottom + 4.0f));
    m_bitmapbf0.push_back(&m_bitmap1340);

    m_bitmap12c0.On();
    m_bitmap12c0.SetHasTexture(false);
    m_bitmap12c0.SetColor(s_pauseColor3);
    m_bitmap12c0.SetPos((int)(no - 40.0f), (int)top, (int)(40.0f + no), (int)bottom);
    m_bitmapbf0.push_back(&m_bitmap12c0);

    float y = 245.0f;

    m_text1080.Load(1.1f, " ", m_font, 0.0f, 0.0f, true);
    m_text1080.SetPos((int)yes, (int)y);
    m_text1080.m_isCentered = 1;
    m_text1080.SetColor(100.0f, 100.0f, 100.0f);
    m_text1080.UpdatePos();
    m_text1080.On();
    m_bitmapbf0.push_back(&m_text1080);

    m_text1220.Load(1.1f, " ", m_font, 0.0f, 0.0f, true);
    m_text1220.SetPos((int)no, (int)y);
    m_text1220.m_isCentered = 1;
    m_text1220.SetColor(100.0f, 100.0f, 100.0f);
    m_text1220.UpdatePos();
    m_text1220.On();
    m_bitmapbf0.push_back(&m_text1220);

    m_panel2.Off();
    ReInit();
}

/* the menu's tick (SOCOM 1's HudTick, much grown): the third panel's bitmap kept up, the selection bar
 * slid to the selected line, then closed the closed tick, or open: the sound ticked, the yes/no of the
 * third panel or the up/down of the lines at a delay between moves, the pad's selection handled, each
 * line's colour reset and the selected one's pulsed; a word's blink of the selected line */
void CZPauseTest::HudTick(CPad *pad, float dT)
{
    if (m_socom2_bdc)
        m_panel2.On();
    else
        m_panel2.Off();
    if (!m_socom2_bdc) {
        CPauseItem *item = &m_items[m_sel];
        float target = (float)(int)(item->m_text.m_posy + item->m_text.m_socom2_80) - 21.0f - -2.0f;
        float cur = m_poly6b0.m_tris[0].m_vertex[0][1];

        if (target != cur) {
            float dir = 1.0f;

            if (target < cur)
                dir = -1.0f;

            float dist = target - cur;
            float speed = 1.0f;

            if (dist < 0.0f)
                dist *= -1.0f;
            if (!(dist <= 31.0f))
                speed = s_pauseBarFast;

            float y;

            if (speed == s_pauseBarFast)
                y = cur + 640.0f * dir;
            else
                y = cur + speed * (400.0f * (dir * dT));
            if (dir == -1.0f) {
                if (y < target)
                    y = target;
            } else if (!(y <= target)) {
                y = target;
            }
            m_poly6b0.SetPos(171.0f, y, 297.0f, 21.0f);
        }
    }
    if (!m_open) {
        TickOffState(*pad, dT);
    } else {
        CSnd::PostTick(0.0f, 0);

        CZSealBody *owner = CHUD::GetHUDOwner();
        CSealCtrl *ctrl = owner ? owner->m_ctrl : 0;
        bool input = ctrl ? (bool)ctrl->m_input_enabled != false : false;

        if (!data_00409098) {
            data_00409090 += dT;
            if (!(data_00409090 <= s_pauseMoveDelay)) {
                data_00409090 = 0.0f;
                data_00409098 = 1;
            }
        }
        if (m_socom2_bdc) {
            if (data_00409098) {
                char left = pad->m_state[PAD_LEFT];
                char right = pad->m_state[PAD_RIGHT];

                if (m_be4 && ((unsigned char)right == KEY_FALLING || (unsigned char)right == KEY_DOWN)) {
                    m_be4 = !m_be4;
                    if (m_bd4)
                        m_bd4->Notify(0, 255);
                } else if (!m_be4 && ((unsigned char)left == KEY_FALLING || (unsigned char)left == KEY_DOWN)) {
                    m_be4 = !m_be4;
                    if (m_bd8)
                        m_bd8->Notify(0, 255);
                }
            }
        } else {
            char up = pad->m_state[PAD_UP];
            char down = pad->m_state[PAD_DOWN];

            if (data_00409098) {
                unsigned char kup = up;

                if (kup == KEY_FALLING || kup == KEY_DOWN || (unsigned char)down == KEY_FALLING || (unsigned char)down == KEY_DOWN) {
                    if ((unsigned char)down == KEY_FALLING || (unsigned char)down == KEY_DOWN) {
                        if (m_bd8)
                            m_bd8->Notify(0, 255);

                        signed char sel = m_sel;
                        bool done = false;

                        do {
                            /* a device: the plain char read is what keeps the target's narrowing of the
                             * read before the add; sel itself goes to IsSelectable as it is */
                            sel = (signed char)((char)sel + 1);
                            if (sel == 12)
                                sel = 0;
                            if (IsSelectable(sel))
                                done = true;
                        } while (!done);
                        m_sel = sel;
                        data_00409098 = 0;
                    }
                    if (kup == KEY_FALLING || kup == KEY_DOWN) {
                        if (m_bd4)
                            m_bd4->Notify(0, 255);

                        signed char sel = m_sel;
                        bool done = false;

                        do {
                            sel = (signed char)((char)sel - 1);
                            if (sel == -1)
                                sel = 11;
                            if (IsSelectable(sel))
                                done = true;
                        } while (!done);
                        m_sel = sel;
                        data_00409098 = 0;
                    }
                    for (unsigned int i = 0; i < 12; i++)
                        m_items[(signed char)i].m_text.SetTrans(128.0f);
                }
            }
        }
        data_00409078 += dT;
        if (pad && input)
            HandleSelectionInput(*pad);
        for (unsigned int i = 0; i < 12; i++) {
            CPauseItem *item = &m_items[(signed char)i];

            item->m_text.m_color[0] = 100.0f;
            item->m_text.m_color[1] = 100.0f;
            item->m_text.m_color[2] = 100.0f;
        }
        data_004090A0 += s_pausePulseDir * (7.0f * dT);
        if (!(data_004090A0 <= 1.0f)) {
            data_004090A0 = 1.0f;
            s_pausePulseDir *= -1.0f;
        }
        if (data_004090A0 < 0.0f) {
            data_004090A0 = 0.0f;
            s_pausePulseDir *= -1.0f;
        }

        static CPnt3D from = s_pauseColor3;
        static CPnt3D to(52.0f, 76.5f, 84.0f);
        CPnt3D diff = to - from;

        if (m_socom2_bdc) {
            if (m_be4) {
                m_text1080.SetColor(100.0f + 28.0f * data_004090A0, 100.0f + 28.0f * data_004090A0, 100.0f + -36.0f * data_004090A0);
                m_bitmap1120.SetColor(from.x + diff.x * data_004090A0, from.y + diff.y * data_004090A0, from.z + diff.z * data_004090A0);
                m_text1220.SetColor(100.0f, 100.0f, 100.0f);
                m_bitmap12c0.SetColor(from.x, from.y, from.z);
            } else {
                m_text1220.SetColor(100.0f + 28.0f * data_004090A0, 100.0f + 28.0f * data_004090A0, 100.0f + -36.0f * data_004090A0);
                m_bitmap12c0.SetColor(from.x + diff.x * data_004090A0, from.y + diff.y * data_004090A0, from.z + diff.z * data_004090A0);
                m_text1080.SetColor(100.0f, 100.0f, 100.0f);
                m_bitmap1120.SetColor(from.x, from.y, from.z);
            }
        } else {
            m_items[m_sel].m_text.SetColor(100.0f + 28.0f * data_004090A0, 100.0f + 28.0f * data_004090A0, 100.0f + -36.0f * data_004090A0);
            m_poly6b0.SetColor(from.x + diff.x * data_004090A0, from.y + diff.y * data_004090A0, from.z + diff.z * data_004090A0);
        }
        m_bc4 = 0.2f;
        m_bc8 = -0.7f;
    }
    if (!(data_003DC418 < 0.0f)) {
        data_003DC418 += dT;

        bool hi = false;

        if (!(data_003DC418 < 0.1f) && data_003DC418 < 0.2f)
            hi = true;

        int trans = hi || data_003DC418 >= 0.3f ? 128 : 0;

        m_items[m_sel].m_text.SetTrans((float)trans);
        if (!(data_003DC418 < 0.3f))
            data_003DC418 = -1.0f;
    }
}

/* the menu closed (SOCOM 1's TickOffState, its unit's first function as here): nothing until two
 * seconds after the mission's set-up; then the start button -- or, off line, the pad's second state
 * (m_dc) going down -- opens the menu when the player's input is on, the 0x409070 byte is clear and
 * the controller is not in menu states 5 or 3; otherwise, while 0x409080 holds, the background,
 * the panels, the bitmaps and the strings fade out at 300 a second (the byte cleared once all four
 * groups reach zero). The HUD comes back on after the menu's own flag (m_socom2_bc1). Each fade
 * read, then reduced in its own statement (`t = x.GetTrans() - 300.0f * dT` computes the product
 * ahead of the first call: 0x394); the menu test's two arms written out, its first a bool local */
void CZPauseTest::TickOffState(const CPad &pad, float dT)
{
    if (data_00409088 < s_pauseDelay) {
        data_00409088 += dT;
        return;
    }

    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player ? player->m_ctrl : 0;
    bool input = ctrl ? (bool)ctrl->m_input_enabled != false : false;
    bool start = pad.m_state[0] == 1;
    bool pressed = true;

    if (!theNetwork.m_bNetwork) {
        pressed = pad.m_dc == 1;
        start = start || !pressed;
    }

    bool open = start && input && !data_00409070;
    MENU_STATE state = ctrl->GetMenuState();

    if ((open && state != 5 && state != 3) || (!pressed && state != 5 && state != 3)) {
        OpenPauseMenu();
    } else if (data_00409080) {
        bool bgDone = false;
        bool bitmapDone = false;
        bool textDone = false;
        bool panelDone = false;
        float t;

        t = m_background.GetTrans();
        t -= 300.0f * dT;
        if (t < 0.0f) {
            t = 0.0f;
            bgDone = true;
        }
        m_background.SetTrans(0, t);
        m_background.SetTrans(3, t);

        t = m_bitmap2a0.GetTrans();
        t -= 300.0f * dT;
        if (t < 0.0f) {
            t = 0.0f;
            bitmapDone = true;
        }
        m_bitmap2a0.SetTrans(t);
        m_poly6b0.SetTrans(t);
        m_bitmap3a0.SetTrans(t);

        t = m_panel.GetTrans();
        t -= 300.0f * dT;
        if (t < 0.0f) {
            t = 0.0f;
            panelDone = true;
        }
        m_panel.SetTrans(t);
        m_bitmap320.SetTrans(t);

        t = m_text420.GetTrans();
        t -= 300.0f * dT;
        if (t < 0.0f) {
            t = 0.0f;
            textDone = true;
        }
        m_text420.SetTrans(t);
        m_texta90.SetTrans(t);
        m_textb28.SetTrans(t);
        for (unsigned int i = 0; i < 12; i++)
            m_items[(signed char)i].m_text.SetTrans(t);
        if (bgDone && bitmapDone && textDone && panelDone)
            data_00409080 = 0;
    }
    if (m_socom2_bc1) {
        m_socom2_bc1 = 0;
        theHUD->On();
    }
}
