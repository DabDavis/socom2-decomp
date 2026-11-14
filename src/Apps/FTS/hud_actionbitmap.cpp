/* hud_actionbitmap.cpp -- the HUD's action icon (OVL1 0x21D9D0-0x220FAF, between the weapon
 * selector and the team names as SOCOM 1's): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM 1 kept the radio bitmaps here too; SOCOM II renamed
 * them (CRadioIcon_Static and CRadioIcon_Dynamic in its RTTI) and their Ticks sit with the
 * network info box's (hud_mpinfobox.cpp, 0x239250 and 0x2393F0), so the static icon's Tick moved
 * there. Written: HudTick (0x21D9D0, the unit's first: the HUD's round 63), UnInit (0x21DE50), which wipes the unit's static string (0x40D564) and turns
 * off its 22 action entries' bitmaps (0x40C200, 0xE0 apart), which the unit's initialiser
 * (0x3FE550) builds. Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/ztwod/ztwod.h"
#include "../../gamez/zvid/zvid.h"

#include <string.h>

/* the action icons' place and size and the bomb timer's bar (OVL1 .data 0x3DC628-0x3DC67C and bss
 * 0x40C160-0x40C188): SOCOM 1's file statics, every name its symbol table's -- SOCOM II's TEXT_SCALAR
 * gone; the half sizes, the bar's inner corners and width and the prompt's x made by the unit's
 * initialiser 0x3FE550, in this order */
static float ACTIONICON_CENTERX = 306.0f;
static float ACTIONICON_YPOS = 365.0f;
static float ACTIONICON_WIDTH = 50.0f;
static float ACTIONICON_HEIGHT = 50.0f;
static float ACTIONICON_HWIDTH = ACTIONICON_WIDTH * 0.5f;
static float ACTIONICON_HHEIGHT = ACTIONICON_HEIGHT * 0.5f;
static float BOMBTIMER_BKGRD_POSX1 = 585.0f;
static float BOMBTIMER_BKGRD_POSY1 = 345.0f;
static float BOMBTIMER_BKGRD_POSX2 = 635.0f;
static float BOMBTIMER_BKGRD_POSY2 = 360.0f;
static float BOMBTIMER_INSET_SIZE = 2.0f;
static float BOMBTIMER_IMAGE_POSX1 = BOMBTIMER_BKGRD_POSX1 + BOMBTIMER_INSET_SIZE;
static float BOMBTIMER_IMAGE_POSY1 = BOMBTIMER_BKGRD_POSY1 + BOMBTIMER_INSET_SIZE;
static float BOMBTIMER_IMAGE_POSY2 = BOMBTIMER_BKGRD_POSY2 - BOMBTIMER_INSET_SIZE;
static float BOMBTIMER_IMAGE_WIDTH = BOMBTIMER_BKGRD_POSX2 - BOMBTIMER_IMAGE_POSX1 - BOMBTIMER_INSET_SIZE;
static float BOMBTIMER_SCALAR = 0.2f;
static PNT2D posWeaponTakenStr = { ACTIONICON_CENTERX, 435.0f };

/* the icons' colours as pulse pairs (OVL1 bss 0x40C190: four pairs of a base and a swing, which the
 * initialiser builds -- __construct_array over eight CPnt3Ds -- and func_0021F120 sets; SOCOM 1's
 * color_base and color_incr, one pair; the name ours) */
static CPnt3D color_table[4][2];

/* an action's name before it has one (OVL1 .data 0x3DC680, a pointer variable: the entries'
 * constructor and 0x2209C0 load it; SOCOM 1's DEFAULT_ACTION_NAME, the name ours) */
static const char *c_szUnknownAction = "UNKNOWN";

/* an action's icon (SOCOM 1's CActionTxtr, its DWARF: 0xE0 bytes -- the texture, the action, the
 * bitmap at 0x10, the name at 0x90; SOCOM II's action a byte and its flag at 0xD1). Its inline
 * constructor's and implicit destructor's copies are this unit's tail (0x220CA0, 0x220C30), the
 * array's __construct_array and __arraydtor 0x220C10 calling them, both byte-exact. The constructor
 * names its entry through SetName(0): the inline's `name ? name : c_szUnknownAction` with the null
 * folded leaves the target's `b` over the dead `move a1,zero` arm (open from round 32 to 39: a
 * plain copy loads the pointer straight, a constant condition folds the arm away) */
class CActionTxtr {
public:
    CActionTxtr()
    {
        m_texhandle = 0;
        m_action_id = 0;
        SetName(0);
        m_active = false;
    }
    /* the name, or "UNKNOWN" for none (ours: 0x2209C0 inlines it twice, a real test each time) */
    void SetName(const char *name) { strcpy(m_name, name ? name : c_szUnknownAction); }

    zdb::CTexHandle *m_texhandle;   // 0x0
    unsigned char m_action_id;   // 0x4
    C2DBitmap m_2DBitmap;   // 0x10
    char m_name[64];   // 0x90
    signed char m_colorset;   // 0xd0: its colour pair in the icons' table (Render_ver_2 reads it with lb)
    bool m_active;   // 0xd1
};

/* the action icons (OVL1 bss 0x40C200, 22 of them: the unit's initialiser 0x3FE550 builds them;
 * SOCOM 1's ActionTxtrPool, 18 there -- Add reads entry 7's texture in both builds; the name ours) */
CActionTxtr g_arrActionTxtrs[22];

/* the world's action points as the machine asks them (SOCOM 1's CNodeAction: SOCOM II's texture at
 * 0x28 and its prompt at 0x2C, each 4 on from SOCOM 1's) and its prompt text when it has none of its
 * own (SOCOM 1's CNodeAction::GetUIString, by its call here: OVL1 0x2B48E0) */
class CNodeAction {
public:
    /* its prompt from its kind when it has no string of its own (SOCOM 1's GetUIString, OVL1
     * 0x2B48E0) */
    const char *GetUIString();

    /* whether its prompt is the mission's own text (bit 2 of 0x3F; the name ours) */
    bool UsesMissionText() const { return m_usemissiontext ? true : false; }

    unsigned char _0[0x28];
    zdb::CTexHandle *m_texhandle;   // 0x28
    const char *m_uistring;   // 0x2c
    unsigned int m_teammask;   // 0x30: the teams it is open to (the driver's sides tested against it)
    unsigned char _34[0x3c - 0x34];
    unsigned int _3c_0 : 26;
    unsigned int m_usemissiontext : 1;   // 0x3c bit 26 (byte 0x3f's bit 2)
    unsigned int _3c_27 : 5;
};

/* a weapon's name as the pickup prompt shows it (zweapon.h's: OVL1 0x3D2830) */
class CZWeapon {
public:
    const char *GetDescription() const;
};

/* a pickup (zseal.h's CPickup: only what the machine reads) */
class CPickup {
public:
    CZWeapon *GetWeapon();

    /* the multiplayer bomb's node, when one is down (OVL1 bss 0x44D4A0) */
    static zdb::CNode *m_mpbombnode;

    float m_timer;   // 0x0
    zdb::CNode *m_node;   // 0x4
    void *m_data;   // 0x8
    unsigned char m_type;   // 0xc: 8, a weapon
};

/* the driver's controller and the driver (zseal.h's: only what the machine calls) */
class CSealCtrl {
public:
    bool CanPerformAction(CNodeAction *action);
    /* the actions in reach marked in the driver's mask (OVL2 0x593500, SOCOM 1's) */
    void SampleActionAvailable();
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
    /* the bomb's timer offered or withdrawn (OVL2 0x5920A0, SOCOM 1's: bit 12 of the flags) */
    void MPBomb_TimerAvailable(bool b);

    unsigned char _4[0x160 - 0x4];
    float m_mpbomb_timer;   // 0x160 (zseal.h's)
    unsigned char _164[0x170 - 0x164];
    unsigned int m_bits0 : 12;   // 0x170
    unsigned int m_mpbomb_bTimerAvailable : 1;   // bit 12
    unsigned int m_mpbomb_bTimerEngaged : 1;   // bit 13
};

/* the SEAL's kit (zseal.h's CZKit: only its place) */
class CZKit {
public:
    unsigned char _0[0x10];
};

class CZSealBody {
public:
    CNodeAction *GetReticuleAction(bool b);
    /* the action the HUD shows marked as the one selected (SOCOM 1's ActionSelectionMask by its call
     * here, SOCOM II's taking the action alone: OVL2 0x544C80 sets the mask word at 0xED4). An int
     * (the HUD's round 42: with an unsigned char parameter MW folds the conversion into an `lbu` of the
     * reinterpreted byte in every spelling, where the target loads it with `lb`; the callee's `andi
     * 0xff` rules out signed char) -- the row retyped ActionSelectionMask__10CZSealBodyFi by the SEALs'
     * round 34, its body `1 << (unsigned char)id` */
    void ActionSelectionMask(int id);
    CPickup *CurrentPickupSecondary();
    CPickup *CurrentPickup();
    /* whether a player drives it (hud_pausemenu.cpp's inline) */
    bool IsPlayer() { return m_ctrl ? m_ctrl->IsPlayer() : false; }
    /* the kit (SOCOM 1's GetKit, inline) */
    CZKit *GetKit() { return &m_kit; }
    /* SOCOM 1's out-of-line setters, inline in SOCOM II (the HUD tick stores the fields) */
    void ActionDisplayOn(bool on) { m_action_display_on = on; }
    void ActionCommand(unsigned int cmd) { m_action_command = cmd; }

    /* its controller, none for none (ours: the machine's last test) */
    static CSealCtrl *CtrlOf(CZSealBody *seal) { return seal ? seal->m_ctrl : 0; }

    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xc8 - 0xc4];
    unsigned int m_teamflags;   // 0xc8: its sides (hud_tcmcmds.cpp's)
    unsigned char _cc[0xe1 - 0xcc];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4 (the action icon shown only with it)
    unsigned char _e1_5 : 3;
    unsigned char _e2[Z_SEALBODY_OFS(0x5e0) - 0xe2];
    CZKit m_kit;   // 0x5e0
    unsigned char _5f0[Z_SEALBODY_OFS(0xed0) - Z_SEALBODY_OFS(0x5f0)];
    unsigned int m_action_available;   // 0xed0: a bit per action (zseal.h's; SOCOM 1's ActionAvailableMask)
    unsigned int m_action_selection;   // 0xed4: the selected action's bit
    unsigned char m_action_display_on;   // 0xed8
    unsigned int m_action_command;   // 0xedc: the pad's cycle asked for -- 3 the next icon, 2 the last
};


/* the icons' machine (SOCOM 1's CActionTxtrMachine: a vector of the icons shown, the prompt's string
 * at 0x14 -- the texture machine at 0x40D550 -- and the driver, SOCOM II's at 0x1F4) */
class CActionTxtrMachine : public std::vector<CActionTxtr *> {
public:
    /* room for every icon (SOCOM 1's reserve(18), its 18 icons; inline in the unit's initialiser) */
    CActionTxtrMachine() : m_main_iconcolor(0), m_side_iconcolor(0), m_curpickup_node(0), m_driver(0)
    {
        reserve(22);
    }
    void TickTransparencyFade(bool b);
    /* the prompt and the timer's bitmaps by reference (ours: the fade calls their virtuals through
     * them) */
    C2DString &ActionString() { return m_action_string; }
    C2DBitmap &TimerImage() { return m_timer_IMAGE; }
    C2DBitmap &TimerBkgrd() { return m_timer_BKGRD; }
    bool UpdateDriverPickup();
    unsigned int Add(CActionTxtr *pActionTxtr);
    /* the prompt for the first icon written (SOCOM 1's SpecializeActionString, SOCOM II's taking the
     * reticule's action point) */
    void SpecializeActionString(CNodeAction *pNodeAction);
    /* the prompt set, placed, centred and shown (ours: SpecializeActionString's every case) */
    inline void ShowActionString(const char *s, bool translate);
    inline void PlaceActionString();
    /* the icons drawn about the action bitmap (SOCOM 1's Render_ver_2, OVL1 0x21F210) */
    bool Render_ver_2(class CZActionBitmap &bmp);
    /* the icons for an action mask made (SOCOM 1's Open, OVL1 0x21F850) */
    unsigned int Open(unsigned int cursel_mask);
    /* one icon added and shown when its bit is in the mask, marked active or not (ours: Open's
     * every action) */
    inline bool OpenIcon(unsigned int mask, unsigned int bit, CActionTxtr *pActionTxtr);

    CPnt3D *m_main_iconcolor;   // 0xc
    CPnt3D *m_side_iconcolor;   // 0x10
    C2DString m_action_string;   // 0x14
    C2DBitmap m_timer_IMAGE;   // 0xb0
    C2DBitmap m_timer_BKGRD;   // 0x130
    zdb::CNode *m_curpickup_node;   // 0x1b0
    char m_pickup_name[64];   // 0x1b4
    CZSealBody *m_driver;   // 0x1f4
};   // 0x200 bytes (SOCOM 1's two speech meters and the speech icon gone)

/* the HUD's machine (SOCOM 1's ActionTxtrMachine_HUD; OVL1 bss 0x40D550): the unit's initialiser
 * builds it and registers its implicit destructor (0x220B50) */
CActionTxtrMachine ActionTxtrMachine_HUD;

/* the timer image's colour (OVL1 .data 0x3E4BA0, zero in the image: the unit's initialiser sets 100,
 * 100, 100 last; the name ours) */
static CPnt3D timer_color(100.0f, 100.0f, 100.0f);

/* the icons faded out by 400 a second (SOCOM 1's CActionTxtrMachine::TickTransparencyFade, 0x3D7930
 * there): with b, the prompt set to 80 and every icon but the first and the active ones faded; else
 * the prompt, the timer's two bitmaps and every icon but the first, each turned off at nothing --
 * the prompt with any icon. The HUD's machine is read by name, its members through accessors
 * returning references (on the members themselves MW calls the bitmaps' and the string's virtuals
 * directly, the target through their vtables; a reference local is kept in a saved register, the
 * target rebuilds the address at each use) */
void CActionTxtrMachine::TickTransparencyFade(bool b)
{
    float fade = 400.0f * zVid.frameTime;

    if (b) {
        ActionTxtrMachine_HUD.ActionString().SetTrans(80.0f);
        for (unsigned int i = 1; i < 22; i++) {
            if (g_arrActionTxtrs[i].m_2DBitmap.IsOn() && !g_arrActionTxtrs[i].m_active) {
                float trans = g_arrActionTxtrs[i].m_2DBitmap.GetTrans();

                if (trans != 0.0f) {
                    trans -= fade;
                    if (trans < 0.0f) {
                        trans = 0.0f;
                        ActionTxtrMachine_HUD.ActionString().Off();
                    }
                    g_arrActionTxtrs[i].m_2DBitmap.SetTrans(trans);
                }
            }
        }
        return;
    }

    if (ActionTxtrMachine_HUD.ActionString().IsOn()) {
        float trans = ActionTxtrMachine_HUD.ActionString().m_color[3];

        trans -= fade;
        if (trans < 0.0f) {
            trans = 0.0f;
            ActionTxtrMachine_HUD.ActionString().Off();
        }
        ActionTxtrMachine_HUD.ActionString().SetTrans(trans);
    }

    if (ActionTxtrMachine_HUD.TimerImage().IsOn()) {
        float trans = ActionTxtrMachine_HUD.TimerImage().GetTrans() - fade;

        if (trans < 0.0f) {
            trans = 0.0f;
            ActionTxtrMachine_HUD.TimerImage().Off();
            ActionTxtrMachine_HUD.TimerBkgrd().Off();
        }
        ActionTxtrMachine_HUD.TimerImage().SetTrans(trans);
        ActionTxtrMachine_HUD.TimerBkgrd().SetTrans(trans);
    }
    for (unsigned int i = 1; i < 22; i++) {
        if (g_arrActionTxtrs[i].m_2DBitmap.IsOn()) {
            float trans = g_arrActionTxtrs[i].m_2DBitmap.GetTrans();

            if (trans != 0.0f) {
                trans -= fade;
                if (trans < 0.0f) {
                    trans = 0.0f;
                    ActionTxtrMachine_HUD.ActionString().Off();
                }
                g_arrActionTxtrs[i].m_2DBitmap.SetTrans(trans);
            }
        }
    }
}

/* the mission's own prompt text (theMission + 0x20; empty for none) */
class CMission;
extern CMission theMission;
/* Z_PORT: through the host object; the PS2 build keeps the placeholders apart, as MW shares one %hi between accesses to one symbol */
#if Z_PORT
#define data_00436500 (*(char (*)[])((char *)&::theMission + 0x20))   /* theMission+0x20 */
#else
extern char data_00436500[];
#endif

/* a string id's text (the string table's) */
const char *GetString(int id);

inline void CActionTxtrMachine::PlaceActionString()
{
    ActionString().SetPos((int)posWeaponTakenStr.x, (int)posWeaponTakenStr.y);
    ActionString().m_isCentered = 1;
    ActionString().On();
}

inline void CActionTxtrMachine::ShowActionString(const char *s, bool translate)
{
    ActionString().SetString(s, translate);
    PlaceActionString();
}

/* the prompt for the first icon written and shown: a pickup's (4, 5) as its name and the weapon's
 * when the driver stands on one; the bomb's (7) with its timer's bar while the timer runs, else the
 * bar faded out and the bomb's plant or defuse text; a door's (13) the mission's text, a reminder
 * when the mission has none, or the icon's name -- "UNAVAILABLE" to another side; any other the
 * icon's name; the bar off and the driver's bomb timer withdrawn for all but the bomb (SOCOM 1's
 * CActionTxtrMachine::SpecializeActionString, 0x3D7B90 there: its tempstr a static). Byte-exact
 * (the HUD's round 63) with: the timer's zero clamp a ternary (an if-clamp's join takes an alignment
 * nop the target does not have, the bomb test after it an if); the door's three strings set in the
 * arms and placed once after them; the mission-text bit read `m_bit ? true : false` (the target's
 * sltu: a plain bool return, `!= 0`, an int return or a cast all test the bit straight) */
void CActionTxtrMachine::SpecializeActionString(CNodeAction *pNodeAction)
{
    static char tempstr[128] = "UNUSED";
    CActionTxtr *pActionTxtr = (*this)[0];

    switch (pActionTxtr->m_action_id) {
    case 4:
    case 5:
        if (ActionTxtrMachine_HUD.m_curpickup_node) {
            sprintf(tempstr, "%s %s", pActionTxtr->m_name, m_pickup_name);
            ShowActionString(tempstr, true);
        }
        break;
    case 7: {
        CSealCtrl *ctrl = m_driver->m_ctrl;

        if (ctrl->m_mpbomb_bTimerEngaged && ctrl->m_mpbomb_bTimerAvailable) {
            float t = ctrl->m_mpbomb_timer;

            t = t < 0.0f ? 0.0f : t;
            if (!CPickup::m_mpbombnode)
                t = 5.0f - t;
            ActionTxtrMachine_HUD.TimerBkgrd().On();
            ActionTxtrMachine_HUD.TimerBkgrd().SetTrans(38.4f);
            ActionTxtrMachine_HUD.TimerImage().On();
            ActionTxtrMachine_HUD.TimerImage().SetTrans(100.0f);
            ActionTxtrMachine_HUD.TimerImage().SetPos((int)BOMBTIMER_IMAGE_POSX1, (int)BOMBTIMER_IMAGE_POSY1,
                                                      (int)(BOMBTIMER_IMAGE_POSX1 + BOMBTIMER_IMAGE_WIDTH * (t * BOMBTIMER_SCALAR)),
                                                      (int)BOMBTIMER_IMAGE_POSY2);
        } else {
            if (ActionTxtrMachine_HUD.TimerImage().IsOn()) {
                float dT_by_faderate = 400.0f * zVid.frameTime;
                float trans_val = ActionTxtrMachine_HUD.TimerImage().GetTrans() - dT_by_faderate;

                if (trans_val < 0.0f) {
                    trans_val = 0.0f;
                    ActionTxtrMachine_HUD.TimerImage().Off();
                    ActionTxtrMachine_HUD.TimerBkgrd().Off();
                }
                ActionTxtrMachine_HUD.TimerImage().SetTrans(trans_val);
                ActionTxtrMachine_HUD.TimerBkgrd().SetTrans(trans_val);
            }
            ShowActionString(GetString(CPickup::m_mpbombnode ? 0xEC40 : 0xEC41), false);
        }
        break;
    }
    case 13:
        if (m_driver->m_teamflags & pNodeAction->m_teammask) {
            if (pNodeAction->UsesMissionText()) {
                const char *text = data_00436500[0] ? data_00436500 : 0;

                ActionString().SetString(text ? text : "PLEASE ADD TO MISSION.RDR", true);
            } else
                ActionString().SetString(pActionTxtr->m_name, true);
        } else
            ActionString().SetString("UNAVAILABLE", true);
        PlaceActionString();
        break;
    default:
        ShowActionString(pActionTxtr->m_name, true);
        break;
    }
    if (pActionTxtr->m_action_id != 7) {
        ActionTxtrMachine_HUD.TimerImage().Off();
        ActionTxtrMachine_HUD.TimerBkgrd().Off();
        CSealCtrl *ctrl = CZSealBody::CtrlOf(m_driver);

        if (ctrl)
            ctrl->MPBomb_TimerAvailable(false);
    }
}

/* the pulse (SOCOM 1's color_scalar, bss 0x40D750: 0 to 1 and back), its direction (.data 0x3DC710, 1;
 * SOCOM 1's bFadeColor, a bool there; the name ours) and rate (SOCOM 1's color_scalar_rate, .data
 * 0x3DC718, 5), and the colour last made (bss 0x40D758; the name ours) */
static float color_scalar;
static float color_scalar_dir = 1.0f;
static float color_scalar_rate = 5.0f;
static CPnt3D color_cur;

/* the pop-up a texture's name triggers (hud_scorepopup.cpp's, OVL1 0x22D270) */
struct CNamedObject;
void hudTriggerPopup(CNamedObject *source);

inline bool CActionTxtrMachine::OpenIcon(unsigned int mask, unsigned int bit, CActionTxtr *pActionTxtr)
{
    if (mask & bit) {
        Add(pActionTxtr);
        pActionTxtr->m_2DBitmap.On();
        pActionTxtr->m_active = true;
        hudTriggerPopup((CNamedObject *)pActionTxtr->m_texhandle);
    } else
        pActionTxtr->m_active = false;
    return pActionTxtr->m_active;
}

/* the icons remade for the driver's action mask, in the HUD's order of importance -- each bit
 * the action of that number's icon -- and counted, the driver's pickup noted and its bomb timer
 * offered when the first icon is the bomb's (SOCOM 1's CActionTxtrMachine::Open, 0x3D7380 there).
 * Byte-exact (the HUD's round 63) with each icon held in the named pointer local (the target's s3:
 * passed as &g_arrActionTxtrs[k] the constant address propagates into every field access, 0x9D8)
 * and the bomb flag declared before the controller local and set in nested ifs: declared after it,
 * or tested with `&&`, MW makes the flag a set-on-condition (sltu / sltiu, 0x864-0x868); a switch
 * keeps branches but adds a case jump (0x880) */
unsigned int CActionTxtrMachine::Open(unsigned int cursel_mask)
{
    unsigned int n = 0;
    CActionTxtr *pActionTxtr;

    clear();
    if (cursel_mask) {
        pActionTxtr = &g_arrActionTxtrs[19];
        if (OpenIcon(cursel_mask, 1 << 19, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[20];
        if (OpenIcon(cursel_mask, 1 << 20, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[11];
        if (OpenIcon(cursel_mask, 1 << 11, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[10];
        if (OpenIcon(cursel_mask, 1 << 10, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[12];
        if (OpenIcon(cursel_mask, 1 << 12, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[7];
        if (OpenIcon(cursel_mask, 1 << 7, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[16];
        if (OpenIcon(cursel_mask, 1 << 16, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[1];
        if (OpenIcon(cursel_mask, 1 << 1, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[2];
        if (OpenIcon(cursel_mask, 1 << 2, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[13];
        if (OpenIcon(cursel_mask, 1 << 13, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[18];
        if (OpenIcon(cursel_mask, 1 << 18, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[17];
        if (OpenIcon(cursel_mask, 1 << 17, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[3];
        if (OpenIcon(cursel_mask, 1 << 3, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[21];
        if (OpenIcon(cursel_mask, 1 << 21, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[4];
        if (OpenIcon(cursel_mask, 1 << 4, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[5];
        if (OpenIcon(cursel_mask, 1 << 5, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[8];
        if (OpenIcon(cursel_mask, 1 << 8, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[9];
        if (OpenIcon(cursel_mask, 1 << 9, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[14];
        if (OpenIcon(cursel_mask, 1 << 14, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[15];
        if (OpenIcon(cursel_mask, 1 << 15, pActionTxtr))
            n++;
        pActionTxtr = &g_arrActionTxtrs[6];
        if (OpenIcon(cursel_mask, 1 << 6, pActionTxtr))
            n++;
    }
    UpdateDriverPickup();
    bool bomb = false;
    CSealCtrl *ctrl = m_driver->m_ctrl;

    if (ActionTxtrMachine_HUD.size())
        if (ActionTxtrMachine_HUD[0]->m_action_id == 7)
            bomb = true;
    ctrl->MPBomb_TimerAvailable(bomb);
    return n;
}

/* the icons laid out, faded and coloured: the first in the middle, the next two to its right and
 * left -- the rest off -- each pulsing in its colour pair (3, a door the driver's sides cannot
 * open), and the prompt written; false with nothing to show (SOCOM 1's
 * CActionTxtrMachine::Render_ver_2, 0x3D6EA0 there: its four colour statics are made and no longer
 * read, the colours come from the pair table). Byte-exact as first written (the HUD's round 63) but
 * the colour set: a SIGNED local (signed char, char or int) indexed through `(unsigned char)` -- the
 * field read with lb and narrowed by andi as the target; an unsigned char local makes it lbu and its
 * 3 a daddiu */
bool CActionTxtrMachine::Render_ver_2(CZActionBitmap &bmp)
{
    if (!size())
        return false;
    if (!m_driver)
        return false;
    if (!m_driver->GetKit())
        return false;

    static CPnt3D mainicon_normal(128.0f, 128.0f, 128.0f);
    static CPnt3D sideicon_normal(100.0f, 100.0f, 100.0f);
    static CPnt3D mainicon_nvg(0.0f, 128.0f, 0.0f);
    static CPnt3D sideicon_nvg(0.0f, 128.0f, 0.0f);

    color_scalar += color_scalar_rate * (zVid.frameTime * color_scalar_dir);
    if (color_scalar_dir == 1.0f) {
        if (color_scalar > 1.0f) {
            color_scalar = 1.0f;
            color_scalar_dir *= -1.0f;
        }
    } else if (color_scalar < 0.0f) {
        color_scalar = 0.0f;
        color_scalar_dir *= -1.0f;
    }

    CActionTxtr *pActionTxtr = (*this)[0];

    pActionTxtr->m_2DBitmap.Load(ACTIONICON_CENTERX - ACTIONICON_HWIDTH, ACTIONICON_YPOS, ACTIONICON_CENTERX + ACTIONICON_HWIDTH,
                                 ACTIONICON_YPOS + ACTIONICON_HEIGHT, pActionTxtr->m_texhandle);
    pActionTxtr->m_2DBitmap.m_fade_dx = 28.0f;
    pActionTxtr->m_2DBitmap.m_fade_limit = 90.0f;
    pActionTxtr->m_2DBitmap.TickFade();

    CNodeAction *pNodeAction = m_driver->GetReticuleAction(true);
    signed char set = pActionTxtr->m_colorset;

    if (pNodeAction && pActionTxtr->m_action_id == 13 && !(m_driver->m_teamflags & pNodeAction->m_teammask))
        set = 3;
    CPnt3D *pair = color_table[(unsigned char)set];

    color_cur = pair[0] + pair[1] * color_scalar;
    pActionTxtr->m_2DBitmap.SetColor(color_cur.x, color_cur.y, color_cur.z);
    pActionTxtr->m_2DBitmap.On();
    SpecializeActionString(pNodeAction);

    unsigned int count = size();
    unsigned int ll = 0;
    unsigned int rr = 0;

    for (unsigned int ii = 1; ii < count; ii++) {
        pActionTxtr = (*this)[ii];
        if (ii < 3) {
            if (ii & 1) {
                float x1 = ACTIONICON_CENTERX + (float)(rr + 1) * (12.0f + ACTIONICON_WIDTH);

                pActionTxtr->m_2DBitmap.Load(x1 - ACTIONICON_HWIDTH, ACTIONICON_YPOS, x1 + ACTIONICON_HWIDTH,
                                             ACTIONICON_YPOS + ACTIONICON_HEIGHT, pActionTxtr->m_texhandle);
                rr++;
            } else {
                float x1 = ACTIONICON_CENTERX - (float)(ll + 1) * (12.0f + ACTIONICON_WIDTH);

                pActionTxtr->m_2DBitmap.Load(x1 - ACTIONICON_HWIDTH, ACTIONICON_YPOS, x1 + ACTIONICON_HWIDTH,
                                             ACTIONICON_YPOS + ACTIONICON_HEIGHT, pActionTxtr->m_texhandle);
                ll++;
            }
            pActionTxtr->m_2DBitmap.GetTrans();
            pActionTxtr->m_2DBitmap.m_fade_dx = 19.0f;
            pActionTxtr->m_2DBitmap.m_fade_limit = 28.0f;
            pActionTxtr->m_2DBitmap.TickFade();

            signed char set = pActionTxtr->m_colorset;

            if (pNodeAction && pActionTxtr->m_action_id == 13 && !(m_driver->m_teamflags & pNodeAction->m_teammask))
                set = 3;
            CPnt3D *pair = color_table[(unsigned char)set];

            color_cur = pair[0] + pair[1] * color_scalar;
            pActionTxtr->m_2DBitmap.SetColor(color_cur.x, color_cur.y, color_cur.z);
            pActionTxtr->m_2DBitmap.On();
        } else
            pActionTxtr->m_2DBitmap.Off();
    }
    return true;
}

/* the driver's pickup when the HUD's first icon is a pickup's (4, the primary; 5, the secondary):
 * its node, and its weapon's name or "UNKNOWN" -- whether there is one (SOCOM 1's
 * CActionTxtrMachine::UpdateDriverPickup, 0x3D7F40 there: the HUD's machine read by name) */
bool CActionTxtrMachine::UpdateDriverPickup()
{
    m_curpickup_node = 0;
    if (ActionTxtrMachine_HUD.size() && m_driver) {
        unsigned char id = ActionTxtrMachine_HUD[0]->m_action_id;

        if (id == 4) {
            CPickup *pickup = m_driver->CurrentPickup();

            if (pickup && pickup->m_type == 8) {
                CZWeapon *weapon = pickup->GetWeapon();

                m_curpickup_node = pickup->m_node;
                if (weapon)
                    strcpy(m_pickup_name, weapon->GetDescription());
                else
                    strcpy(m_pickup_name, "UNKNOWN");
            }
        } else if (id == 5) {
            CPickup *pickup = m_driver->CurrentPickupSecondary();

            if (pickup && pickup->m_type == 8) {
                CZWeapon *weapon = pickup->GetWeapon();

                m_curpickup_node = pickup->m_node;
                if (weapon)
                    strcpy(m_pickup_name, weapon->GetDescription());
                else
                    strcpy(m_pickup_name, "UNKNOWN");
            }
        }
    }
    return m_curpickup_node != 0;
}

/* an icon made current: the driver's selection cleared for no action; for a pickup, a door or a
 * bomb (13, 7, 17) the multiplayer bomb's texture when one is down and the action is the bomb's,
 * else the reticule's action point's texture and prompt when it can be taken; the bitmap faded out
 * and the icon added (SOCOM 1's CActionTxtrMachine::Add, 0x3D8070 there). The action is passed as
 * a signed byte read again through the field's address (`lb` off `addiu v1,v0,4` after the `lbu`
 * test): a reinterpreting reference -- the plain argument sign-extends the tested load */
unsigned int CActionTxtrMachine::Add(CActionTxtr *pActionTxtr)
{
    if (!pActionTxtr->m_action_id)
        m_driver->ActionSelectionMask((signed char &)pActionTxtr->m_action_id);
    if (pActionTxtr->m_action_id == 13 || pActionTxtr->m_action_id == 7 ||
        pActionTxtr->m_action_id == 17) {
        zdb::CNode *bomb = CPickup::m_mpbombnode;
        CNodeAction *pNodeAction = m_driver->GetReticuleAction(true);

        if (bomb && pActionTxtr->m_action_id == 7)
            pActionTxtr->m_texhandle = g_arrActionTxtrs[7].m_texhandle;
        else if (pNodeAction && m_driver->m_ctrl->CanPerformAction(pNodeAction)) {
            pActionTxtr->m_texhandle = pNodeAction->m_texhandle;
            if (pNodeAction->m_uistring)
                pActionTxtr->SetName(pNodeAction->m_uistring);
            else
                pActionTxtr->SetName(pNodeAction->GetUIString());
        }
    }
    pActionTxtr->m_2DBitmap.SetTrans(0.0f);
    push_back(pActionTxtr);
    return 0;
}

/* the HUD's menu states (hud_missioncams.h's) */
enum MENU_STATE {
    stateNone = 0,
    stateWeaponSel = 1,
    stateOrders = 2,
    statePauseTestMenu = 3,
    stateFullScreenMapMenu = 4,
    stateFullScreenMapAnim = 5,
    stateLast = 6
};

class CHUD;

class CZActionBitmap : public C2DBitmap {
public:
    void Init(CHUD *parent, C2DFont *pFont);
    void UnInit();
    void HudTick(MENU_STATE menustate);
    /* a child listed (inline, the name ours: the icons' lists a level deeper than the prompt's and
     * the timer's) */
    void AddIcon(C2D *icon) { push_back(icon); }

    unsigned char _80[0xa4 - 0x80];
    CHUD *m_parent;   // 0xa4
};

/* the HUD's map (hud_newhudmap.h's CZNewHudMap: only its full-screen flag) */
class CZNewHudMap {
public:
    unsigned char _0[0xdc];
    bool m_isFullScreen;   // 0xdc
};

/* the HUD (hud_main.cpp's CHUD: only what the tick reads) */
class CHUD {
public:
    bool IsOn();
    static CZSealBody *GetHUDOwner();

    static PNT4D m_hud_textcolor;   /* the HUD's text colour */

    unsigned char _0[Z_CHUD_OFS(0x11c90)];
    CZNewHudMap *m_newmap;   // 0x11c90
};

extern CHUD *theHUD;

extern "C" void func_0021F120();

/* one action's icon set up and listed (a macro: the index folds into each address, which an inline's
 * parameter leaves to a common subexpression held in a saved register) */
#define DEFINE_ICON(id, tex, colorset, name) \
    g_arrActionTxtrs[id].m_texhandle = tex; \
    g_arrActionTxtrs[id].m_action_id = id; \
    g_arrActionTxtrs[id].m_colorset = colorset; \
    g_arrActionTxtrs[id].m_2DBitmap.Reset(); \
    g_arrActionTxtrs[id].m_2DBitmap.Off(); \
    g_arrActionTxtrs[id].m_2DBitmap.m_fade_enable = 1; \
    AddIcon(&g_arrActionTxtrs[id].m_2DBitmap); \
    strcpy(g_arrActionTxtrs[id].m_name, name)

/* the asset manager's list of what the libraries hold (znode.h's) */
namespace zdb {
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};
class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the icons' colours set, the bitmap off and nearly opaque, its list emptied, the actions' textures
 * looked up (the five that may be missing the first's instead), the 22 icons set up and listed, the
 * prompt loaded when there is a font, the timer's bar and background set up, the HUD kept (SOCOM 1's
 * CZActionBitmap::Init, 0x3D5E40 there, 18 icons). Byte-exact (the HUD's round 67) with three shapes,
 * each measured:
 *   - the textures declared C-style with the first one last: it is defined first and takes s0, which
 *     needs a number above every other (igreplay: --move x=51.5, the only single move);
 *   - the last look-up through a temporary, its test on the temporary (`bnez v0` in the target, the
 *     named local moved to fp ahead of the five tests): tested through `hold`, s6/s7/fp rotate;
 *   - THE ICONS AS A MACRO whose list goes through an inline wrapper: a literal index folds into each
 *     address at parse time, so each virtual call rebuilds its address (lui a0; addiu a0), where an
 *     inline's index parameter leaves `&g[id] + 16` to a common subexpression in a saved register; and
 *     push_back one level down gives the frame its order -- the prompt's and the timer's two
 *     arguments at the top (level 0), clear's three, then the icons' 22 (level 1) */
void CZActionBitmap::Init(CHUD *parent, C2DFont *pFont)
{
    zdb::CTexHandle *climb, *slide, *talk, *dropbomb, *mpbomb, *knife, *restrain, *pickup1, *pickup2, *closecombat, *conceal, *mount, *dismount, *c4, *dance, *follow, *hold, *x, *t;

    func_0021F120();
    Off();
    SetTrans(127.0f);
    clear();

    x = zdb::CAssetMgr::m_assets.GetTexHandle("action_x.tif");
    climb = zdb::CAssetMgr::m_assets.GetTexHandle("action_climb.tif");
    slide = zdb::CAssetMgr::m_assets.GetTexHandle("action_slide.tif");
    talk = zdb::CAssetMgr::m_assets.GetTexHandle("action_talk.tif");
    dropbomb = zdb::CAssetMgr::m_assets.GetTexHandle("action_drop_MP_bomb.tif");
    mpbomb = zdb::CAssetMgr::m_assets.GetTexHandle("action_MP_bomb.tif");
    knife = zdb::CAssetMgr::m_assets.GetTexHandle("action_knife.tif");
    restrain = zdb::CAssetMgr::m_assets.GetTexHandle("action_restrain.tif");
    pickup1 = zdb::CAssetMgr::m_assets.GetTexHandle("action_pickup_item1.tif");
    pickup2 = zdb::CAssetMgr::m_assets.GetTexHandle("action_pickup_item2.tif");
    closecombat = zdb::CAssetMgr::m_assets.GetTexHandle("action_close_combat.tif");
    conceal = zdb::CAssetMgr::m_assets.GetTexHandle("action_conceal_body.tif");
    mount = zdb::CAssetMgr::m_assets.GetTexHandle("action_mount_turret.tif");
    dismount = zdb::CAssetMgr::m_assets.GetTexHandle("action_dismount_turret.tif");
    c4 = zdb::CAssetMgr::m_assets.GetTexHandle("action_place_c4.tif");
    dance = zdb::CAssetMgr::m_assets.GetTexHandle("action_victory_dance.tif");
    follow = zdb::CAssetMgr::m_assets.GetTexHandle("action_escortee.tif");
    t = zdb::CAssetMgr::m_assets.GetTexHandle("action_escortee.tif");
    hold = t;

    if (!c4)
        c4 = x;
    if (!dance)
        dance = x;
    if (!dropbomb)
        dropbomb = x;
    if (!follow)
        follow = x;
    if (!t)
        hold = x;
    DEFINE_ICON(0, x, 1, "UNKNOWN");
    DEFINE_ICON(16, slide, 1, "LADDER SLIDE");
    DEFINE_ICON(1, x, 1, "ACTION");
    DEFINE_ICON(2, climb, 1, "CLIMB");
    DEFINE_ICON(3, talk, 1, "TALK");
    DEFINE_ICON(4, pickup1, 1, "TAKE");
    DEFINE_ICON(5, pickup2, 1, "TAKE");
    DEFINE_ICON(6, dropbomb, 1, "DROP BOMB");
    DEFINE_ICON(7, mpbomb, 1, "PRESS AND HOLD   ");
    DEFINE_ICON(8, conceal, 1, "PICKUP BODY");
    DEFINE_ICON(9, conceal, 1, "DROP BODY");
    DEFINE_ICON(10, knife, 0, "KNIFE ATTACK");
    DEFINE_ICON(11, restrain, 0, "RESTRAIN");
    DEFINE_ICON(12, closecombat, 0, "RIFLE BUTT");
    DEFINE_ICON(13, x, 1, "UNDEFINED");
    DEFINE_ICON(18, c4, 0, "PLACE C4 CHARGE");
    DEFINE_ICON(17, x, 0, "SATCHEL");
    DEFINE_ICON(19, mount, 1, "USE TURRET");
    DEFINE_ICON(20, dismount, 1, "DISMOUNT TURRET");
    DEFINE_ICON(21, dance, 1, "VICTORY DANCE");
    DEFINE_ICON(14, follow, 1, "FOLLOW!");
    DEFINE_ICON(15, hold, 1, "HOLD POSITION");
    if (pFont) {
        ActionTxtrMachine_HUD.ActionString().Load(0.9f, "ACTION ICON", pFont, 306.0f, 358.0f, true);
        ActionTxtrMachine_HUD.ActionString().Off();
        ActionTxtrMachine_HUD.ActionString().SetTrans(0.0f);
        *(PNT4D *)ActionTxtrMachine_HUD.m_action_string.m_color = CHUD::m_hud_textcolor;
        push_back(&ActionTxtrMachine_HUD.ActionString());
    }
    ActionTxtrMachine_HUD.TimerBkgrd().SetHasTexture(false);
    ActionTxtrMachine_HUD.TimerBkgrd().SetColor(0.0f, 0.0f, 0.0f);
    ActionTxtrMachine_HUD.TimerBkgrd().SetPos(BOMBTIMER_BKGRD_POSX1, BOMBTIMER_BKGRD_POSY1, BOMBTIMER_BKGRD_POSX2,
                                              BOMBTIMER_BKGRD_POSY2);
    ActionTxtrMachine_HUD.TimerBkgrd().SetTrans(0.0f);
    push_back(&ActionTxtrMachine_HUD.TimerBkgrd());
    ActionTxtrMachine_HUD.TimerImage().SetHasTexture(false);
    ActionTxtrMachine_HUD.TimerImage().SetColor(timer_color.x, timer_color.y, timer_color.z);
    ActionTxtrMachine_HUD.TimerImage().SetPos(BOMBTIMER_IMAGE_POSX1, BOMBTIMER_IMAGE_POSY1,
                                              BOMBTIMER_IMAGE_POSX1 + BOMBTIMER_IMAGE_WIDTH, BOMBTIMER_IMAGE_POSY2);
    ActionTxtrMachine_HUD.TimerImage().SetTrans(0.0f);
    push_back(&ActionTxtrMachine_HUD.TimerImage());
    m_parent = parent;
}

/* the prompt let go of, every entry's bitmap clear and off. The string through a reference: on
 * the static itself MW calls C2DString's functions directly, where the target goes through the
 * vtable; the array's elements it dispatches as they stand (a pointer local per element costs a
 * saved register the target does not have) */
void CZActionBitmap::UnInit()
{
    C2DString &text = ActionTxtrMachine_HUD.m_action_string;

    text.ClearAndWipe();
    for (unsigned int i = 0; i < 22; i++) {
        g_arrActionTxtrs[i].m_2DBitmap.SetTrans(0.0f);
        g_arrActionTxtrs[i].m_2DBitmap.Off();
    }
}

/* the action icon's tick: nothing while the HUD is off or the map is full screen; off, with the
 * driver's display flag down, unless the HUD's owner shows actions and a player drives it; else
 * the prompt and the timer coloured, the icon on, the driver noted, the pad's cycle applied to the
 * icons (3 the next first, 2 the last first), the driver's actions sampled again and the icons
 * remade when they change, drawn and faded, and the first one's action marked selected (SOCOM 1's
 * CZActionBitmap::HudTick, 0x3D5870 there: its reco icon and night-vision colours gone, its
 * setters inline). Byte-exact (the HUD's round 63) with three shapes, each measured:
 *   - `count > 1`: `count >= 2` materialises the compare (sltiu v0, 8 bytes short), `> 1` branches
 *     on `sltiu at` as the target;
 *   - the destination taken as a reference before the source is read (`CActionTxtr *&dst = [i-1];
 *     dst = [i];`): MW's unrolled copies then make each destination's address before the source's,
 *     the target's t-register order (written `[i - 1] = [i]`, every copy's pair swapped, 33 off);
 *   - A MODELLING DEVICE: `count`'s dead first definition before the controller's (`count = 0`,
 *     never used) numbers it between pSeal and pCtrl (README: MW numbers a variable at its first
 *     definition). pCtrl, this and pSeal are all deferred; numbered after count pCtrl is pushed a
 *     sweep earlier and coloured last, s2 (igreplay: --move count=33.5 or pCtrl=36.5 gives the
 *     target, count before pSeal does not). SOCOM 1's DWARF names no controller local either */
void CZActionBitmap::HudTick(MENU_STATE menustate)
{
    if (!theHUD->IsOn() || (theHUD->m_newmap ? theHUD->m_newmap->m_isFullScreen : false))
        return;

    CZSealBody *pSeal = CHUD::GetHUDOwner();

    if (!pSeal->m_socom2_e1_4 || !pSeal->IsPlayer()) {
        pSeal->ActionDisplayOn(false);
        Off();
        return;
    }

    CZKit *pKit = pSeal->GetKit();

    if (pKit) {
        ActionTxtrMachine_HUD.m_action_string.m_color[0] = CHUD::m_hud_textcolor.x;
        ActionTxtrMachine_HUD.m_action_string.m_color[1] = CHUD::m_hud_textcolor.y;
        ActionTxtrMachine_HUD.m_action_string.m_color[2] = CHUD::m_hud_textcolor.z;
        ActionTxtrMachine_HUD.m_timer_IMAGE.SetColor(timer_color.x, timer_color.y, timer_color.z);
    }

    unsigned int count = 0;   /* dead: numbers count here (see above) */
    CSealCtrl *pCtrl = pSeal->m_ctrl;

    pSeal->ActionDisplayOn(true);
    On();
    ActionTxtrMachine_HUD.m_driver = pSeal;
    unsigned int action_command = pSeal->m_action_command;

    if (action_command) {
        if (action_command == 3) {
            count = ActionTxtrMachine_HUD.size();

            if (count > 1) {
                CActionTxtr *first = ActionTxtrMachine_HUD[0];

                for (unsigned int i = 1; i < count; i++) {
                    CActionTxtr *&dst = ActionTxtrMachine_HUD[i - 1];

                    dst = ActionTxtrMachine_HUD[i];
                }
                ActionTxtrMachine_HUD[count - 1] = first;
                ActionTxtrMachine_HUD.UpdateDriverPickup();
            }
        } else if (action_command == 2) {
            unsigned int count = ActionTxtrMachine_HUD.size();

            if (count > 1) {
                CActionTxtr *last = ActionTxtrMachine_HUD.back();

                for (unsigned int i = count - 1; i; i--)
                    ActionTxtrMachine_HUD[i] = ActionTxtrMachine_HUD[i - 1];
                ActionTxtrMachine_HUD[0] = last;
                ActionTxtrMachine_HUD.UpdateDriverPickup();
            }
        }
        pSeal->ActionCommand(0);
    }

    unsigned int prev_actionmask = pSeal->m_action_available;

    pSeal->m_action_available = 0;
    pCtrl->SampleActionAvailable();
    unsigned int curr_actionmask = pSeal->m_action_available;

    if (!prev_actionmask)
        ActionTxtrMachine_HUD.Open(curr_actionmask);
    else if (curr_actionmask != prev_actionmask)
        ActionTxtrMachine_HUD.Open(curr_actionmask);

    bool bValidActionsPresent = !ActionTxtrMachine_HUD.empty();

    ActionTxtrMachine_HUD.Render_ver_2(*this);
    ActionTxtrMachine_HUD.TickTransparencyFade(bValidActionsPresent);
    if (bValidActionsPresent)
        pSeal->ActionSelectionMask((signed char &)ActionTxtrMachine_HUD[0]->m_action_id);
    else
        pSeal->m_action_selection = 0;
}
