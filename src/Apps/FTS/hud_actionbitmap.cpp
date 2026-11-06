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

/* an action's name before it has one (OVL1 .data 0x3DC680, a pointer variable: the entries'
 * constructor and 0x2209C0 load it; the name ours) */
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
    unsigned char _e2[0x5e0 - 0xe2];
    CZKit m_kit;   // 0x5e0
    unsigned char _5f0[0xed0 - 0x5f0];
    unsigned int m_action_available;   // 0xed0: a bit per action (zseal.h's; SOCOM 1's ActionAvailableMask)
    unsigned int m_action_selection;   // 0xed4: the selected action's bit
    unsigned char m_action_display_on;   // 0xed8
    unsigned int m_action_command;   // 0xedc: the pad's cycle asked for -- 3 the next icon, 2 the last
};


/* the icons' machine (SOCOM 1's CActionTxtrMachine: a vector of the icons shown, the prompt's string
 * at 0x14 -- the texture machine at 0x40D550 -- and the driver, SOCOM II's at 0x1F4) */
class CActionTxtrMachine : public std::vector<CActionTxtr *> {
public:
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

/* the prompt's place and the bomb timer's bar (OVL1 .data 0x3DC670-0x3DC67C and bss 0x40C170-0x40C188,
 * the unit's file statics -- the initialiser 0x3FE550 sets the prompt's x and the bar's corners and
 * width; by address until it is written) */
extern float data_003DC670;   /* 0.2: the bar's width per second of the timer */
extern float data_003DC678;   /* the prompt's x */
extern float data_003DC67C;   /* 435: its y */
extern float data_0040C170;   /* the bar's left */
extern float data_0040C178;   /* its top */
extern float data_0040C180;   /* its bottom */
extern float data_0040C188;   /* its width */
/* the mission's own prompt text (theMission + 0x20; empty for none) */
extern char data_00436500[];

/* a string id's text (the string table's) */
const char *GetString(int id);

inline void CActionTxtrMachine::PlaceActionString()
{
    ActionString().SetPos((int)data_003DC678, (int)data_003DC67C);
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
            ActionTxtrMachine_HUD.TimerImage().SetPos((int)data_0040C170, (int)data_0040C178,
                                                      (int)(data_0040C170 + data_0040C188 * (t * data_003DC670)),
                                                      (int)data_0040C180);
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

/* the icons' colours as pulse pairs (OVL1 bss 0x40C190: four pairs of a base and a swing, which the
 * unit's initialiser builds -- __construct_array over eight CPnt3Ds -- and sets), the pulse (0x40D750,
 * 0 to 1 and back), its direction and rate (.data 0x3DC710, 1; 0x3DC718, 5) and the colour last made
 * (bss 0x40D758); the main icon's place and size (.data 0x3DC628-0x3DC640) and its half width (bss
 * 0x40C160) -- the unit's statics, by address until its initialiser is written */
extern CPnt3D data_0040C190[4][2];
extern float data_0040D750;
extern float data_003DC710;
extern float data_003DC718;
extern CPnt3D data_0040D758;
extern float data_003DC628;   /* the main icon's centre x */
extern float data_003DC630;   /* its top */
extern float data_003DC638;   /* an icon's width (the side icons 12 further apart) */
extern float data_003DC640;   /* its height */
extern float data_0040C160;   /* its half width */

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

    data_0040D750 += data_003DC718 * (zVid.frameTime * data_003DC710);
    if (data_003DC710 == 1.0f) {
        if (data_0040D750 > 1.0f) {
            data_0040D750 = 1.0f;
            data_003DC710 *= -1.0f;
        }
    } else if (data_0040D750 < 0.0f) {
        data_0040D750 = 0.0f;
        data_003DC710 *= -1.0f;
    }

    CActionTxtr *pActionTxtr = (*this)[0];

    pActionTxtr->m_2DBitmap.Load(data_003DC628 - data_0040C160, data_003DC630, data_003DC628 + data_0040C160,
                                 data_003DC630 + data_003DC640, pActionTxtr->m_texhandle);
    pActionTxtr->m_2DBitmap.m_fade_dx = 28.0f;
    pActionTxtr->m_2DBitmap.m_fade_limit = 90.0f;
    pActionTxtr->m_2DBitmap.TickFade();

    CNodeAction *pNodeAction = m_driver->GetReticuleAction(true);
    signed char set = pActionTxtr->m_colorset;

    if (pNodeAction && pActionTxtr->m_action_id == 13 && !(m_driver->m_teamflags & pNodeAction->m_teammask))
        set = 3;
    CPnt3D *pair = data_0040C190[(unsigned char)set];

    data_0040D758 = pair[0] + pair[1] * data_0040D750;
    pActionTxtr->m_2DBitmap.SetColor(data_0040D758.x, data_0040D758.y, data_0040D758.z);
    pActionTxtr->m_2DBitmap.On();
    SpecializeActionString(pNodeAction);

    unsigned int count = size();
    unsigned int ll = 0;
    unsigned int rr = 0;

    for (unsigned int ii = 1; ii < count; ii++) {
        pActionTxtr = (*this)[ii];
        if (ii < 3) {
            if (ii & 1) {
                float x1 = data_003DC628 + (float)(rr + 1) * (12.0f + data_003DC638);

                pActionTxtr->m_2DBitmap.Load(x1 - data_0040C160, data_003DC630, x1 + data_0040C160,
                                             data_003DC630 + data_003DC640, pActionTxtr->m_texhandle);
                rr++;
            } else {
                float x1 = data_003DC628 - (float)(ll + 1) * (12.0f + data_003DC638);

                pActionTxtr->m_2DBitmap.Load(x1 - data_0040C160, data_003DC630, x1 + data_0040C160,
                                             data_003DC630 + data_003DC640, pActionTxtr->m_texhandle);
                ll++;
            }
            pActionTxtr->m_2DBitmap.GetTrans();
            pActionTxtr->m_2DBitmap.m_fade_dx = 19.0f;
            pActionTxtr->m_2DBitmap.m_fade_limit = 28.0f;
            pActionTxtr->m_2DBitmap.TickFade();

            signed char set = pActionTxtr->m_colorset;

            if (pNodeAction && pActionTxtr->m_action_id == 13 && !(m_driver->m_teamflags & pNodeAction->m_teammask))
                set = 3;
            CPnt3D *pair = data_0040C190[(unsigned char)set];

            data_0040D758 = pair[0] + pair[1] * data_0040D750;
            pActionTxtr->m_2DBitmap.SetColor(data_0040D758.x, data_0040D758.y, data_0040D758.z);
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

/* the string the action's prompt is written in (the texture machine's at 0x40D550 + 0x14, by
 * address until the machine is written) */
extern C2DString data_0040D564;

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

class CZActionBitmap : public C2DBitmap {
public:
    void UnInit();
    void HudTick(MENU_STATE menustate);
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

    unsigned char _0[0x11c90];
    CZNewHudMap *m_newmap;   // 0x11c90
};

extern CHUD *theHUD;

/* the timer image's colour (OVL1 .data 0x3E4BA0, zero in the image: the unit's initialiser
 * 0x3FE550 sets 100, 100, 100 last -- by address until the initialiser is written) */
extern CPnt3D data_003E4BA0;

/* the prompt let go of, every entry's bitmap clear and off. The string through a reference: on
 * the static itself MW calls C2DString's functions directly, where the target goes through the
 * vtable; the array's elements it dispatches as they stand (a pointer local per element costs a
 * saved register the target does not have) */
void CZActionBitmap::UnInit()
{
    C2DString &text = data_0040D564;

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
        ActionTxtrMachine_HUD.m_timer_IMAGE.SetColor(data_003E4BA0.x, data_003E4BA0.y, data_003E4BA0.z);
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
