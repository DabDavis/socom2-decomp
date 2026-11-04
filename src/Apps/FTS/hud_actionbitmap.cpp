/* hud_actionbitmap.cpp -- the HUD's action icon (OVL1 0x21D9D0-0x220FAF, between the weapon
 * selector and the team names as SOCOM 1's): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_actionbitmap.cpp). SOCOM 1 kept the radio bitmaps here too; SOCOM II renamed
 * them (CRadioIcon_Static and CRadioIcon_Dynamic in its RTTI) and their Ticks sit with the
 * network info box's (hud_mpinfobox.cpp, 0x239250 and 0x2393F0), so the static icon's Tick moved
 * there. Written: UnInit (0x21DE50), which wipes the unit's static string (0x40D564) and turns
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
    unsigned char m_d0;   // 0xd0
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
    unsigned char _0[0x28];
    zdb::CTexHandle *m_texhandle;   // 0x28
    const char *m_uistring;   // 0x2c
};

extern "C" const char *func_002B48E0(CNodeAction *action);

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
};

class CZSealBody {
public:
    CNodeAction *GetReticuleAction(bool b);
    CPickup *CurrentPickupSecondary();
    CPickup *CurrentPickup();

    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
};

/* the action the HUD shows marked as the one selected (SOCOM 1's CZSealBody::ActionSelectionMask
 * by its call here, SOCOM II's taking the action alone: OVL2 0x544C80 sets the mask word at 0xED4) */
extern "C" void func_00544C80(CZSealBody *seal, signed char action);

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
        func_00544C80(m_driver, (signed char &)pActionTxtr->m_action_id);
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
                pActionTxtr->SetName(func_002B48E0(pNodeAction));
        }
    }
    pActionTxtr->m_2DBitmap.SetTrans(0.0f);
    push_back(pActionTxtr);
    return 0;
}

/* the string the action's prompt is written in (the texture machine's at 0x40D550 + 0x14, by
 * address until the machine is written) */
extern C2DString data_0040D564;

class CZActionBitmap : public C2DBitmap {
public:
    void UnInit();
};

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
