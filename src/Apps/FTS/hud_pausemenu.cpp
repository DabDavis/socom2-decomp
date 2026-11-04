/* hud_pausemenu.cpp -- the HUD's pause menu (CZPauseTest, OVL1 0x201580-0x2057FF): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_pausemenu.cpp), after the lens flares and before the
 * objectives as there; its first functions in the image are the vtable's On, Off and Draw, its
 * last the destructor and the constructor. SOCOM II's CZPauseTest is 0x13C0 bytes (SOCOM 1's
 * 0x380), laid out from its destructor. Written: On, Off, Draw, Clear, ReInit, the constructor, the
 * lines' test and the line's constructor and destructor (round 37). Defined in reverse of their
 * image order (-inline deferred): the unit ends with the lines' test 0x205A70, the line's
 * constructor 0x205970 (generated before the menu's constructor, which takes its address) and the
 * menu's constructor 0x205800. */

#include "../../gamez/ztwod/ztwod.h"

/* a menu line (0xA4 bytes: the menu's array of twelve is new'ed): a string first -- a member, not a
 * base: the line's implicit destructor (0x2057A0, which new[] and delete[] hand to the array
 * helpers) destroys the string with -1 -- then a word (0xDEADBEEF until set: 0x202520 stores each
 * line's string id there), a byte (12 until set) and a bit-field group at 0x9D (a word at a byte
 * offset: a 19-bit field there is written with lwr/lwl) whose first two flags the lines' test reads,
 * the first offline, the second online; the 27-bit field at 0xA0 only the constructor touches. The
 * names ours */
struct CPauseItem {
    CPauseItem();
    void SetFlag0(bool b) { m_flag0 = b; }
    void SetFlag1(bool b) { m_flag1 = b; }
    void SetFlag2(bool b) { m_flag2 = b; }
    void SetFlag3(bool b) { m_flag3 = b; }
    void SetFlag4(bool b) { m_flag4 = b; }
    void SetA0(unsigned int v) { m_a0 = v; }

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
    void Clear();
    void ClosePauseMenu();
    bool IsSelectable(signed char line);
    /* a line by its index (an int: the test re-extends the char line after the valve lookup, where
     * m_items[line] keeps the extension across the call in a saved register) */
    CPauseItem &Item(int i) { return m_items[i]; }

    CPauseItem *m_items;   // 0x18
    signed char m_sel;   // 0x1c: the line selected
    int m_20;   // 0x20
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
    int m_bc4;   // 0xbc4
    float m_bc8;   // 0xbc8
    signed char m_bcc;   // 0xbcc
    void *m_bd0;   // 0xbd0
    void *m_bd4;   // 0xbd4
    void *m_bd8;   // 0xbd8
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

/* a word the menu keeps (OVL1 bss 0x409088; no row named yet) */
extern int data_00409088;

/* the network game (fts_net_game's theNetwork, 0x45A0C0): whether it is one */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* whether the network game is the online one (OVL1 0x2C2FA0, no row named yet) */
extern "C" bool func_002C2FA0();

/* the LAN lobby (zNetLANLobby.cpp's, 0x44FE10): whether the menus came from the LAN menu */
class CZLANLobby {
public:
    bool IsActive();
};

extern CZLANLobby data_0044FE10;

/* a valve (zvalve.h's): its value a 16-bit field */
class CValve {
public:
    static CValve *GetByName(const char *name);

    char *m_name;   // 0x0
    unsigned int m_value : 16;   // 0x4
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

/* a line made empty (0x205970: its address handed to __construct_new_array by the menu's
 * constructor; generated before it, so not an inline's copy).
 * OPEN, 10 instructions (0xD0 vs 0xF8): the target sets the five flags of the bit-field group at
 * 0x9D through its UNALIGNED WORD (lwr/lwl 157..160, swr/swl) -- the only such read-modify-write in
 * the image (build/scratch/hud/swrscan.py) -- where MW narrows each one-bit store to the byte
 * (lbu/sb 157), as the target itself does in 0x202520's line set-up. Everything else is the target's:
 * the group's container is a word at 0x9D (a 19-bit field written there gives lwr/lwl), the 27-bit
 * field the aligned word at 0xA0, the five setters' one unfolded `andi t6,zero,1`. Tried, all
 * narrowed alike: int, unsigned long, unsigned short, a 4-byte enum and volatile fields; group sizes
 * 5-32 bits and byte members after it; __attribute__((packed)) and #pragma pack(1) (MW still places
 * a packed member struct at 0xA0); the flags in a nested struct with its own setters, or in a member
 * class whose inline constructor sets them under an implicit CPauseItem(); non-inline setters
 * auto-inlined (defined before or after); #pragma opt_scalarizebitfields on/off, opt_lifetimes off */
CPauseItem::CPauseItem()
{
    m_id = 0xdeadbeef;
    m_kind = 12;
    SetFlag0(false);
    SetFlag1(false);
    SetFlag2(false);
    SetFlag3(false);
    SetFlag4(false);
    SetA0(0);
}

/* the menu made closed (CHUD's constructor builds CHUD+0x18900 with it): the members built, the menu
 * turned off, its state cleared, the twelve lines new'ed */
CZPauseTest::CZPauseTest()
{
    Off();
    m_bc4 = 0;
    m_bc8 = 50.0f;
    m_20 = 0;
    m_bcc = 0;
    m_bd0 = 0;
    m_bd4 = 0;
    m_bd8 = 0;
    m_sel = -1;
    m_items = new CPauseItem[12];
    m_socom2_bdc = 0;
    m_be0 = -1;
    m_be4 = 0;
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
