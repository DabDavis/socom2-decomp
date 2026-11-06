/* hud_tcmtree.cpp -- the radio command menu's tree (OVL1 0x231A30-0x23624F; the file name ours, as
 * every name here: SOCOM II's own menu, no SOCOM 1 counterpart). hud_tcm.h has the cut into three
 * units: this one calls hud_tcmcmds.cpp's two eight-byte node setters (0x230550/0x230540) and
 * hud_tcm.cpp's mode setter (0x236250) out of line, and has no initialiser (its statics
 * 0x4130A0-0x4130BF).
 *
 * Written: CTCMNode's constructor and destructor (the children a vector subclass: four this-tests),
 * its state update, search, children, captions, name and delay, the self-inlined display-off and
 * delay tick (HideAll five copies deep, Tick three: each copy calls the accessor chains one level
 * shallower, so their ten out-of-line copies 0x231DD0-0x231E30 and 0x232460-0x232480 name the
 * chains' levels -- CTCMNode's accessor, CTCMNodeList's, the vector's), the sounds, and CTCM's
 * tick, push-to-talk, close, open, root, next and the item searches (ShowRoot since round 42).
 * OPEN: the open menu's pad handler HandleOpenPad 0x235D40 (round 54, temporaries); RemoveChild
 * byte-exact since round 62.
 * Written in round 41: one kind's items, tcmAddKindItems 0x2329F0 (its entries go under the third
 * group item's caption, which an inline returns); round 57: Select byte-exact and the closed menu's
 * pad handler HandleClosedPad 0x2359A0; round 66: the cache builders 0x232BC0/0x2344E0.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "hud_tcm.h"
#include "../../gamez/zsys/zsys_public.h"
#include "../../gamez/zinput/zinput.h"

/* the controller's menu states (zseal.h's MENU_STATE; the names ours) */
enum MENU_STATE {
    MENU_STATE_NONE = 0,
    MENU_STATE_1 = 1,
    MENU_STATE_TCM = 2
};

/* a radio command (zseal.h's, a byte enum under -enum min) */
enum FT_COMMAND { CMD_UNKNOWN = 0, CMD_ACTION = 1, CMD_LAST = 255 };

/* the controller as the menu reads it (zseal.h's CSealCtrl) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool b);
    void StopRumbler();
    /* a command's name (OVL2 0x58EF80, the SEALs' round 46: SOCOM 1's by order) */
    static const char *GetCommandName(FT_COMMAND cmd);

    unsigned char _0[0x114];
    unsigned int m_padIndex;   // 0x114: its pad
    unsigned char _118[0x221 - 0x118];
    unsigned char m_221;   // 0x221: 3 leaves the menu state alone on a close
};

/* the player (zseal.h's CZSealBody: its controller) */
class CZSealBody {
public:
    unsigned char _0[0xc0];
    CSealCtrl *m_ctrl;   // 0xc0
    unsigned char _c4[0xe1 - 0xc4];
    unsigned char _e1_0 : 4;
    unsigned char m_socom2_e1_4 : 1;   // 0xe1 bit 4 (the closed menu's open test, offline)
    unsigned char _e1_5 : 3;

    void CacheReticuleSeal(bool on);   // OVL2 0x5AA620 (the SEALs' round 39: seal_env.cpp)
};

CZSealBody *ftsGetPlayer();

/* OVL1's pad call (by address) */
extern "C" void func_002C5D10(CPad *pad, int a, int b);

/* the HUD's compass (hud_mapcompass.h's MapCompass: only its switches) */
class MapCompass {
public:
    void Off();
    void On();
};

/* the HUD (hud_main.cpp's CHUD: only its compass) */
class CHUD {
public:
    static CZSealBody *GetHUDOwner();
    void Add2D(C2D *p2d, int phase, bool add_to_back);

    unsigned char _0[0x4fe0];
    MapCompass m_compass;   // 0x4fe0
};

extern CHUD *theHUD;

/* the voice recogniser (reco.cpp's theReco) */
class CReco {
public:
    void Suspend(bool now);
    void Resume();
};

extern CReco theReco;

/* the network game (fts_net_game's): whether it is one */
class CZNetwork {
public:
    unsigned char m_bSessionMaster;   // 0x0
    unsigned char m_bNetwork;   // 0x1
};

extern CZNetwork theNetwork;

/* the mission (fts_mission.cpp's CMission): the menu's root item its rdr names, and bit 2 of the
 * flags byte at 0xFC, the menu's open request -- cleared here through its bool setter, which leaves
 * the store's `(0 & 1) << 2` to run time */
class CMission {
public:
    /* the root item's name, if the mission names one (ours) */
    const char *GetTcmRootName() { return m_20[0] ? m_20 : 0; }
    void SetFc2(bool b) { m_fc_2 = b; }

    unsigned char _0[0x20];
    char m_20[0x40];   // 0x20: the Tcm NODEACTION_ROOTNAME
    unsigned char _60[0xfc - 0x60];
    unsigned char m_DelayEndMission : 1;   // 0xfc
    unsigned char m_FadingToEndMission : 1;
    unsigned char m_fc_2 : 1;   // the menu's open request (SOCOM II's)
};

extern CMission theMission;


/* a pad by its index, none past the last (SOCOM 1's inline over CInput's table) */
static inline CPad *GetPad(unsigned int n)
{
    return n < CInput::NUM_PADS ? CInput::m_pads[n] : 0;
}

/* hud_tcmcmds.cpp's: the node an item's test runs for, a node opened, the recogniser's mode */
void tcmSetCurNode(CTCMNode *node);
void tcmClearCurNode();
int tcmOpenNode(CZSealBody *player, CTCMNode *node);
void tcmSetRecoMode(bool on);

/* the menu's sounds (their virtual's slot 1: CSnd::PlaySound, as hud_weaponsel.cpp's) */
class CSnd {
public:
    virtual ~CSnd();
    virtual void *PlaySound(void *inst, unsigned int flags);

    static CSnd *GetSoundByName(const char *name);
};

/* the unit's float pool (0x3DCAF0-0x3DCB20, 8 bytes apart: function-local statics, written as file
 * statics -- scdiff compares them by content) */
static float s_tcm_slidehold = 0.2f;   // 0x3DCAF0: the hold after a slide
static float s_tcm_talkwait = 0.3f;   // 0x3DCAF8: the hold before the closed menu's talk starts
static float s_tcm_talkmax = 10.0f;   // 0x3DCB00: the longest push-to-talk, and the close test's wait
static float s_tcm_releasehold = 1.0f;   // 0x3DCB08: the hold after a release

/* the push-to-talk state (bss 0x4130A0-0x4130B8; the names ours) */
static bool s_tcm_talking;   // 0x4130A0
static float s_tcm_talktime;   // 0x4130A8
static bool s_tcm_armed;   // 0x4130B0: the closed menu's button let go out of hold (HandleClosedPad)
static bool s_tcm_talkheld;   // 0x4130B8

/* the pad's buttons as the controller unit reads them (OVL1 0x2C64E0, by address: a byte state) */
enum TCM_BUTTON {
    TCM_BUTTON_UP = 0,
    TCM_BUTTON_PRESSED = 1,
    TCM_BUTTON_HELD = 2
};

extern "C" TCM_BUTTON func_002C64E0(int button, CPad *pad);

/* the radio's talk switch (OVL1 0x30E6E0 on the global 0x45B718, by address) and the HUD flag
 * 0x44D4F8 that allows it */
extern "C" void func_0030E6E0(void *radio, int on);
extern void *data_0045B718;
extern unsigned char data_0044D4F8;

/* hud_tcmcmds.cpp's: a node closed (its parent's column shown again) */
void tcmCloseNode(CTCMNode *node);

/* the menu's tick: the hold run down, then by its mode -- 1 the open menu's input, 2 and 3 the
 * controller's menu state off (2 moving on to 3), 4 the closed menu's, 5 a close once the button is
 * let go or the wait is over, 6 a close once the button is let go */
void CTCM::Tick(CPad *pad, float dt)
{
    if (!(m_hold <= 0.0f))
        m_hold -= dt;
    /* (the blocks lie in source order: 2, 1, 4, 3, 5, 6) */
    switch (m_mode.m_mode) {
    case 0:
        break;
    case 2:
        m_mode.Set(3);
        ftsGetPlayer()->m_ctrl->SetMenuState(MENU_STATE_NONE, false);
        break;
    case 1:
        HandleOpenPad(pad, dt);
        break;
    case 4:
        HandleClosedPad(pad, dt);
        break;
    case 3:
        ftsGetPlayer()->m_ctrl->SetMenuState(MENU_STATE_NONE, false);
        break;
    case 5: {
        TCM_BUTTON st = func_002C64E0(11, pad);
        bool close = false;

        if (!(m_dc8 <= s_tcm_talkmax))
            close = true;
        else if (st != TCM_BUTTON_HELD && st != TCM_BUTTON_PRESSED)
            close = true;
        if (close) {
            func_0030E6E0(data_0045B718, 0);
            Close(false);
            Hold(false, s_tcm_releasehold);
            m_dc8 = 0.0f;
            m_dcc = 0;
        }
        break;
    }
    case 6:
        if (func_002C64E0(11, pad) != TCM_BUTTON_HELD) {
            m_dd4 = 0;
            Close(false);
        }
        break;
    }
}

/* the open menu's input (0x235D40, Tick's mode 1; the name ours), once the hold has run out: the menu
 * closed when nothing in the first column can be selected and the selection is gone, hidden or
 * disabled; then triangle backs out (the menu closed at the first depth, else the selected node's
 * column), the game's button 11 or cross runs the selected item's command -- its children opened
 * when it returns 1 (with no command, when there are any), the menu closed when it returns 0 -- and
 * down / up step the open column (with the slide sound and hold). Written in round 54:
 *  - the pad's three bytes are loaded signed (lb) and narrowed unsigned at each test (andi): `char`
 *    locals compared through a KEY_STATE cast (KEY_STATE locals load lbu; plain char compares
 *    sign-extend; a bool inline over the two states materialises its result);
 *  - the command's result is the node's byte state, as Select's (TCM_STATE r from a NoChildren()
 *    ternary: `move r,zero; li; movn` -- an int ternary gives sltu): called through TCMFunc's int MW
 *    narrows it at the assignment, which the target never does, so the call casts the pointer to
 *    return the state -- a MODELLING DEVICE for a command type returning it (TCMFunc stays int:
 *    hud_tcmcmds.cpp's commands match as int functions);
 *  - `> 1`, not `>= 2`: the latter materialises the compare in v1 where the target branches on at.
 * OPEN, 2 instructions since round 65 (12 until then), one load swap: the target's `lb a1,6(s3); lb a2,5(s3)` (down, then up,
 * cross in the delay slot), here up, then down -- every register is the target's. The narrowings in byte-enum locals declared in
 * their blocks (the kit's round 60: an enum holder stays numbered where written) put the constants 1 and 2 above them, as round
 * 54's replay asked (the select test's constant "generated last"); declared cross, up, down numbers the three loads for a3, a2,
 * a1, but they are code-generation temporaries numbered by generation order, and the scheduler keeps that order on a tie (the
 * first generated goes to the delay slot, the others in order), so the target's order needs up numbered below down and generated
 * after it. Tried for that: const or byte-enum locals (copy-propagated at PCode level, no node of their own), dead first
 * definitions (dropped: 5), an inline accessor for cross and up or all three (`char` or by reference: 2-6), a local copy of down
 * or a wider local (2) */
void CTCM::HandleOpenPad(CPad *pad, float dt)
{
    if (m_hold <= 0.0f) {
        if (FindNext(GetColumn(0), false) == -1) {
            CTCMNode *sel = GetSelected();

            if (!sel || (sel && (sel->m_state == TCM_HIDDEN || sel->m_741))) {
                Close(false);
                theReco.Suspend(true);
            }
        }
        TCM_BUTTON st = func_002C64E0(11, pad);
        char cross = pad->m_state[PAD_CROSS];
        char up = pad->m_state[PAD_UP];
        char down = pad->m_state[PAD_DOWN];

        if (pad->m_state[PAD_TRIANGLE] == KEY_RISING) {
            if (m_path.size() < 2) {
                m_sounds.Play(TCM_SND_SLIDE);
                Close(false);
                theReco.Suspend(true);
                pad->Flush(false);
                return;
            }
            m_sounds.Play(TCM_SND_SLIDE);
            tcmCloseNode(GetSelected());
            return;
        }
        if (st == TCM_BUTTON_PRESSED || (KEY_STATE)cross == KEY_FALLING) {
            CTCMNode *sel = GetSelected();
            TCM_STATE r = sel->NoChildren() ? TCM_SHOWN : TCM_HIDDEN;
            CTCMEntry *entry = sel->m_entry;

            if (entry->m_action) {
                tcmSetCurNode(sel);
                r = ((TCM_STATE (*)(CZSealBody *, int))entry->m_action)(ftsGetPlayer(), entry->m_actionArg);
                tcmClearCurNode();
            }
            switch (r) {
            case 1:
                tcmOpenNode(0, sel);
                break;
            case 0:
                m_sounds.Play(TCM_SND_BACK);
                Close(true);
                theReco.Suspend(true);
                pad->Flush(false);
                break;
            }
            theTCM.Layout(dt);
            return;
        }
        KEY_STATE kd = (KEY_STATE)down;

        if (kd == KEY_DOWN || kd == KEY_FALLING) {
            CTCMNode *col = GetColumn(m_path.size() - 1);

            if (col && col->GetNumChildren() > 1) {
                int i = FindNext(col, true);

                m_path[m_path.size() - 1] = i;
                m_sounds.Play(TCM_SND_SLIDE);
                Hold(false, s_tcm_slidehold);
            }
        } else {
        KEY_STATE ku = (KEY_STATE)up;

        if (ku == KEY_DOWN || ku == KEY_FALLING) {
            CTCMNode *col = GetColumn(m_path.size() - 1);

            if (col && col->GetNumChildren() > 1) {
                int i = FindPrev(col);

                m_path[m_path.size() - 1] = i;
                m_sounds.Play(TCM_SND_SLIDE);
                Hold(false, s_tcm_slidehold);
            }
        }
        }
    }
}

/* the closed menu's input (0x2359A0, Tick's mode 4; the name ours): button 11 let go clears the
 * talk clock and arms the menu (once the hold is out), anything but held clears the long-hold clock.
 * Armed and out of hold: a mission's open request opens the menu where the player may (alive offline,
 * the HUD's owner online -- this test's online arm, the release's offline arm: two copies, as the
 * target has them), on the root item the mission names, and is cleared through its bool setter;
 * else held online past the talk wait starts the radio (mode 5), held offline past the long hold
 * opens the menu hidden (mode 6) and disarms it, and pressed opens it where the player may */
void CTCM::HandleClosedPad(CPad *pad, float dt)
{
    TCM_BUTTON st = func_002C64E0(11, pad);

    if (st == TCM_BUTTON_UP) {
        m_dc8 = 0;
        m_dcc = 1;
        if (m_hold <= 0.0f)
            s_tcm_armed = true;
    }
    if (st != TCM_BUTTON_HELD)
        m_dd4 = 0;
    if (m_hold <= 0.0f && s_tcm_armed) {
        if (theMission.m_fc_2) {
            CZSealBody *player = ftsGetPlayer();
            bool ok = true;

            if (!player)
                ok = false;
            else if (!theNetwork.m_bNetwork && !player->m_socom2_e1_4)
                ok = false;
            else if (theNetwork.m_bNetwork && player != CHUD::GetHUDOwner())
                ok = false;
            if (ok) {
                Open();
                unsigned int n = m_children.size();
                unsigned int i = 0;
                const char *name = theMission.GetTcmRootName();

                if (name && strlen(name))
                    for (; i < n; i++) {
                        CTCMNode *c = m_children[i];
                        const char *s;

                        if (c && (s = c->m_text.m_string + c->m_text.m_firstCharPos) != 0 &&
                            !strcasecmp(s, name))
                            break;
                    }
                if (i && i < n)
                    m_path[0] = i;
                tcmOpenNode(0, GetSelected());
            }
            theMission.SetFc2(false);
            return;
        }
        if (st == TCM_BUTTON_HELD) {
            if (st == TCM_BUTTON_HELD && theNetwork.m_bNetwork) {
                if (m_dcc && !(m_dc8 <= s_tcm_talkwait) && !s_tcm_talking) {
                    m_mode.Set(5);
                    func_0030E6E0(data_0045B718, 1);
                }
            } else if (st == TCM_BUTTON_HELD) {
                m_dd4 += dt;
                if (!(m_dd4 <= m_dd8)) {
                    Open();
                    m_root.Off();
                    m_mode.Set(6);
                    s_tcm_armed = false;
                }
            }
        } else if (st == 3) {
            if (m_dcc) {
                CZSealBody *player = ftsGetPlayer();
                bool ok = true;

                if (!player)
                    ok = false;
                else if (!theNetwork.m_bNetwork && !player->m_socom2_e1_4)
                    ok = false;
                else if (!theNetwork.m_bNetwork && player != CHUD::GetHUDOwner())
                    ok = false;
                if (ok)
                    Open();
            }
        }
    }
}

/* the next selectable item of a column after the open one, wrapping round to the first; with no
 * other, the open one if asked (else -1). Members reading theTCM, not `this` (a0 is never read) */
int CTCM::FindNext(CTCMNode *column, bool wrap)
{
    CTCMNode *c;
    int found = -1;

    if (!column)
        return found;
    unsigned int cur = theTCM.m_path.back();
    unsigned int n = column ? column->m_children.size() : 0;

    for (unsigned int i = cur + 1; i < n && found == -1; i++) {
        c = column->m_children[i];

        if (c && c->m_state != TCM_HIDDEN && !c->m_741)
            found = i;
    }
    if (found == -1)
        for (unsigned int i = 0; i < cur && found == -1; i++) {
            c = column->m_children[i];

            if (c && c->m_state != TCM_HIDDEN && !c->m_741)
                found = i;
        }
    if (wrap && found == -1)
        found = cur;
    return found;
}

/* the previous selectable item of a column, wrapping round to the last; the open one if none */
int CTCM::FindPrev(CTCMNode *column)
{
    CTCMNode *c;
    unsigned int cur = theTCM.m_path.back();
    int found = -1;
    unsigned int n = column ? column->m_children.size() : 0;

    for (int i = cur - 1; i >= 0 && found == -1; i--) {
        c = column->m_children[i];

        if (c && c->m_state != TCM_HIDDEN && !c->m_741)
            found = i;
    }
    if (found == -1)
        for (unsigned int i = n - 1; cur < i && found == -1; i--) {
            c = column->m_children[i];

            if (c && c->m_state != TCM_HIDDEN && !c->m_741)
                found = i;
        }
    if (found == -1)
        found = cur;
    return found;
}

/* the open column's next item selected, with the slide's sound and a short hold */
void CTCM::Next()
{
    CTCMNode *column = GetColumn(m_path.size() - 1);

    if (column && column->m_children.size() > 1) {
        int next = FindNext(column, true);

        m_path[m_path.size() - 1] = next;
        m_sounds.Play(TCM_SND_SLIDE);
        Hold(false, s_tcm_slidehold);
    }
}

/* the root's items shown: the first one shown or with no test opened, the select sound, the
 * layout (the pushed selection a ternary: its temporary stored in each arm, the count declared
 * first). The state a local declared, uninitialised, ahead of the walked child (round 42): numbered
 * below the child, it is coloured after it, so the child keeps v1 (its address temporary's register)
 * and the byte takes a1, as the target has them (the state read straight off the child, the byte is
 * the later temporary and takes v1 first) */
void CTCM::ShowRoot()
{
    unsigned int n = m_children.size();
    int first = -1;

    for (unsigned int i = 0; i < n && first == -1; i++) {
        TCM_STATE st;
        CTCMNode *c = m_children[i];

        st = c->m_state;
        if (st == TCM_SHOWN)
            first = i;
        if (c->m_entry && !c->m_entry->m_update)
            first = i;
    }
    m_path.clear();
    if (m_children.size()) {
        m_path.push_back(first != -1 ? first : 0);
    }
    m_root.On();
    unsigned int count = m_children.size();

    for (unsigned int i = 0; i < count; i++)
        m_children[i]->m_root.On();
    m_sounds.Play(TCM_SND_SELECT);
    theTCM.Layout(0.0f);
}

/* the menu opened: the player's controls in the menu state, the states updated, the root shown,
 * the compass off, the recogniser listening */
void CTCM::Open()
{
    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player->m_ctrl;
    CPad *pad = GetPad(ctrl->m_padIndex);

    player->CacheReticuleSeal(true);
    m_dc8 = 0;
    ctrl->SetMenuState(MENU_STATE_TCM, false);
    ctrl->StopRumbler();
    if (pad)
        func_002C5D10(pad, 11, 1);
    UpdateState(true);
    ShowRoot();
    m_mode.Set(0);
    theHUD->m_compass.Off();
    m_da0 = 0;
    m_da4 = 1.0f;
    g_tcmColumns[0].m_dir = 1.0f;
    g_tcmColumns[0].m_4 = 1.0f;
    m_root.On();
    Layout(0.0f);
    if (!theNetwork.m_bNetwork)
        theReco.Resume();
    m_13b0 = 0;
}

/* the menu closed (selected: mode 2 and the selection's flags; else mode 3), the columns sliding
 * out, the compass back, the recogniser suspended */
void CTCM::Close(bool selected)
{
    CZSealBody *player = ftsGetPlayer();

    if (player->m_ctrl->m_221 != 3)
        player->m_ctrl->SetMenuState(MENU_STATE_NONE, m_mode.m_mode == 5);
    player->CacheReticuleSeal(false);
    if (m_mode.m_mode != 6)
        Hold(false, 0.3f);
    m_mode.Set(selected ? 2 : 3);
    if (selected) {
        m_dce = 1;
        m_dd0 = 0;
    }
    for (int i = 0; i < 8; i++)
        g_tcmColumns[i].m_dir = -1.0f;
    theHUD->m_compass.On();
    m_da4 = -1.0f;
    if (!theNetwork.m_bNetwork)
        theReco.Suspend(false);
    CInput::Flush();
}

/* the recogniser's word: the open column's item of that name selected and run -- a submenu opened,
 * else the menu closed with the back sound (a hidden item's command run in the recogniser's mode);
 * whether one was found (the counter an int, kept in memory for push_back's reference; the child
 * declared first; the column assigned in its test). Round 54: a BOOL -- the target returns the flag
 * unconverted and sets it with daddiu (README: a bool assigned true; reco.cpp's view already
 * declared it bool) -- and the command's result the node's byte state, as HandleOpenPad's (the
 * NoChildren() ternary gives the target's `move s5,zero ... movn s5,a0,v1`; the call casts the
 * command's pointer, the same modelling device): 21 instructions to 1.
 * Round 57, byte-exact: the target's count reads v0 and the loop a copy in s6 made in the test's
 * delay slot -- TWO variables, both defined ahead of the null test: the column (short-lived, so
 * coalesced with the call's v0: the test and the count read it) and a reference to its children
 * (m_children sits at offset 0, so the reference is a copy of the column: s6, the loop's). Taken
 * after the test (round 56's try) the copy lands in the count's block, after the load */
bool CTCM::Select(const char *name)
{
    bool done = false;
    CTCMNode *c;

    if (name && strlen(name)) {
        CTCMNode *column = GetColumn(m_path.size() - 1);
        CTCMNodeList &kids = column->m_children;

        if (column) {
            unsigned int n = column->m_children.size();

            if (n)
                for (int i = 0; i < n && !done; i++) {
                    c = kids[i];
                    const char *s = c ? c->m_name : 0;

                    if (s && strlen(s) && s && !strcasecmp(s, name)) {
                        m_path.pop_back();
                        m_path.push_back(i);
                        TCM_STATE r = c->NoChildren() ? TCM_SHOWN : TCM_HIDDEN;
                        CTCMEntry *e = c->m_entry;

                        if (c->m_state != TCM_HIDDEN) {
                            if (e->m_action) {
                                tcmSetCurNode(c);
                                r = ((TCM_STATE (*)(CZSealBody *, int))e->m_action)(ftsGetPlayer(), e->m_actionArg);
                                tcmClearCurNode();
                            }
                            switch (r) {
                            case 1:
                                tcmOpenNode(0, c);
                                break;
                            case 0:
                                m_sounds.Play(TCM_SND_BACK);
                                Close(true);
                                theReco.Suspend(true);
                                break;
                            }
                            done = true;
                        } else {
                            tcmSetRecoMode(true);
                            if (e->m_action)
                                e->m_action(ftsGetPlayer(), e->m_actionArg);
                            tcmSetRecoMode(false);
                            m_sounds.Play(TCM_SND_BACK);
                            Close(true);
                            theReco.Suspend(true);
                            done = true;
                        }
                    }
                }
        }
    }
    return done;
}

/* the commands and tests the caches hold (hud_tcmcmds.cpp's) */
int tcmCharacterTaunt(CZSealBody *player, int arg);
int tcmCueScaryMusic(CZSealBody *player, int arg);
int tcmIntimidate(CZSealBody *player, int arg);
int tcmMute(CZSealBody *player, int arg);
int tcmNiceOne(CZSealBody *player, int arg);
int tcmNodeAction(CZSealBody *player, int arg);
int tcmRadioOnOff(CZSealBody *player, int arg);
int tcmReportStatus(CZSealBody *player, int arg);
int tcmSayAgain(CZSealBody *player, int arg);
int tcmSelectPlayer(CZSealBody *player, int arg);
int tcmSelectTeam(CZSealBody *player, int arg);
int tcmSendMessage(CZSealBody *player, int arg);
int tcmSetChannel(CZSealBody *player, int arg);
int tcmTeamCommand(CZSealBody *player, int arg);
int tcmTeamMember(CZSealBody *player, int arg);
int tcmUpdateIntimidate(CZSealBody *player, int arg);
int tcmUpdateMessage(CZSealBody *player, int arg);
int tcmUpdateMute(CZSealBody *player, int arg);
int tcmUpdateNodeAction(CZSealBody *player, int arg);
int tcmUpdatePlayer(CZSealBody *player, int arg);
int tcmUpdateRadio(CZSealBody *player, int arg);
int tcmUpdateRadioOnOff(CZSealBody *player, int arg);
int tcmUpdateTeam(CZSealBody *player, int arg);
int tcmUpdateTeamCommand(CZSealBody *player, int arg);
int tcmUpdateTeamMember(CZSealBody *player, int arg);
int tcmUpdateVote(CZSealBody *player, int arg);
int tcmUpdateVoteAll(CZSealBody *player, int arg);
int tcmVote(CZSealBody *player, int arg);
int tcmVoteAll(CZSealBody *player, int arg);
int tcmWhassup(CZSealBody *player, int arg);
/* an entry of the kinds' table (ours: a byte enum -- the kind's first and end entries come back
 * unnarrowed and are zero-extended where they are compared as ints) */
enum TCM_ENTRY { TCM_ENTRY_NONE = 0, TCM_ENTRY_LAST = 0xff };

/* OVL2's item tables: a team's name (and its text, func_0058DBB0), and the kinds' (below) */
extern "C" {
const char *func_0058DD70(signed char group);
const char *func_0058DBB0(signed char group);
const char *func_0058E950(FT_COMMAND kind);
const char *func_0058E900(FT_COMMAND kind);
const char *func_0058E750(FT_COMMAND kind);
TCM_ENTRY func_0058DF20(FT_COMMAND kind);
TCM_ENTRY func_0058DEB0(FT_COMMAND kind);
const char *func_0058EB00(TCM_ENTRY entry);
const char *func_0058E4D0(TCM_ENTRY entry);
const char *func_0058E360(TCM_ENTRY entry);
const char *func_0058E120(TCM_ENTRY entry);
}
extern CTCMCache spCache;
extern CTCMCache mpCache;

/* the six teams' numbers (.data 0x3DCAE8, bytes: the two builders read them) */
static signed char s_tcmTeams[6] = { 1, 2, 3, 17, 18, 29 };

/* the caches filled: the single player's teams, the teammates' FT_COMMAND names, the team commands and a
 * few words; then the network game's radio menu (0x2344E0: the menu's Init calls it; the name ours) */
void tcmBuildCaches()
{
    int i;

    spCache.m_entries.reserve(96);
    mpCache.m_entries.reserve(64);
    for (i = 0; i < 6; i++) {
        signed char t = s_tcmTeams[i];
        unsigned char id = t;

        spCache.Add(func_0058DD70(t), tcmSelectTeam, id, tcmUpdateTeam, id);
    }
    spCache.Add("INTIMIDATE", tcmIntimidate, 0, tcmUpdateIntimidate, 0);
    for (i = 0; i < 24; i++)
        spCache.Add(CSealCtrl::GetCommandName((FT_COMMAND)i), tcmTeamMember, i, tcmUpdateTeamMember, i);
    for (i = 1; i < 42; i++)
        spCache.Add(func_0058EB00((TCM_ENTRY)i), tcmTeamCommand, i, tcmUpdateTeamCommand, i);
    spCache.Add("NICE ONE", tcmNiceOne, 0, 0, 0);
    spCache.Add("WHASSUP", tcmWhassup, 0, 0, 0);
    spCache.Add("REPORT STATUS", tcmReportStatus, 0, 0, 0);
    spCache.Add("CUE SCARY MUSIC", tcmCueScaryMusic, 0, 0, 0);
    spCache.Add("SAY AGAIN", tcmSayAgain, 0, 0, 0);
    mpCache.Add("NODEACTION ROOT", 0, 0, tcmUpdateRadio, 0);
    mpCache.Add("NODEACTION 01", tcmNodeAction, 1, tcmUpdateNodeAction, 1);
    mpCache.Add("NODEACTION 02", tcmNodeAction, 2, tcmUpdateNodeAction, 2);
    mpCache.Add("NODEACTION 03", tcmNodeAction, 3, tcmUpdateNodeAction, 3);
    mpCache.Add("NODEACTION 04", tcmNodeAction, 4, tcmUpdateNodeAction, 4);
    mpCache.Add("NODEACTION 05", tcmNodeAction, 5, tcmUpdateNodeAction, 5);
    mpCache.Add("NODEACTION 06", tcmNodeAction, 6, tcmUpdateNodeAction, 6);
    mpCache.Add("NODEACTION 07", tcmNodeAction, 7, tcmUpdateNodeAction, 7);
    mpCache.Add("NODEACTION 08", tcmNodeAction, 8, tcmUpdateNodeAction, 8);
    mpCache.Add("TACTICAL ORDERS", tcmSelectTeam, 31, tcmUpdateTeam, 31);
    mpCache.Add("MP HOLD FIRE", tcmSendMessage, 6, tcmUpdateMessage, 6);
    mpCache.Add("MP FIRE AT WILL", tcmSendMessage, 3, tcmUpdateMessage, 3);
    mpCache.Add("MP CLEAR AREA", tcmSendMessage, 23, tcmUpdateMessage, 23);
    mpCache.Add("MP COVER ME", tcmSendMessage, 45, tcmUpdateMessage, 45);
    mpCache.Add("MP DEFUSE BOMB", tcmSendMessage, 46, tcmUpdateMessage, 46);
    mpCache.Add("MP PROTECT", tcmSendMessage, 47, tcmUpdateMessage, 47);
    mpCache.Add("MP POINT", tcmSendMessage, 48, tcmUpdateMessage, 48);
    mpCache.Add("TAUNTS", 0, 0, tcmUpdateRadio, 0);
    mpCache.Add("CHARACTER TAUNT", tcmCharacterTaunt, 0, 0, 0);
    mpCache.Add("CUSTOM TAUNT 01", tcmSendMessage, 54, tcmUpdateMessage, 54);
    mpCache.Add("CUSTOM TAUNT 02", tcmSendMessage, 55, tcmUpdateMessage, 55);
    mpCache.Add("CUSTOM TAUNT 03", tcmSendMessage, 56, tcmUpdateMessage, 56);
    mpCache.Add("CUSTOM TAUNT 04", tcmSendMessage, 57, tcmUpdateMessage, 57);
    mpCache.Add("CUSTOM TAUNT 05", tcmSendMessage, 58, tcmUpdateMessage, 58);
    mpCache.Add("MESSAGES", tcmSelectTeam, 31, tcmUpdateTeam, 31);
    mpCache.Add("CUSTOM MSG 1", tcmSendMessage, 49, tcmUpdateMessage, 49);
    mpCache.Add("CUSTOM MSG 2", tcmSendMessage, 50, tcmUpdateMessage, 50);
    mpCache.Add("CUSTOM MSG 3", tcmSendMessage, 51, tcmUpdateMessage, 51);
    mpCache.Add("CUSTOM MSG 4", tcmSendMessage, 52, tcmUpdateMessage, 52);
    mpCache.Add("CUSTOM MSG 5", tcmSendMessage, 53, tcmUpdateMessage, 53);
    mpCache.Add("RADIO", 0, 0, tcmUpdateRadio, 0);
    mpCache.Add("RADIO ONOFF", tcmRadioOnOff, 0, tcmUpdateRadioOnOff, 0);
    mpCache.Add("ACTIVE CHANNEL", 0, 0, 0, 0);
    mpCache.Add("TEAM", tcmSetChannel, 0, 0, 0);
    mpCache.Add("OFFENSE", tcmSetChannel, 2, 0, 0);
    mpCache.Add("DEFENSE", tcmSetChannel, 1, 0, 0);
    mpCache.Add("TEAMMATES", 0, 0, 0, 0);
    mpCache.Add("SPECTATORS", 0, 0, 0, 0);
    mpCache.Add("PLAYER 1", tcmSelectPlayer, 0, tcmUpdatePlayer, 0);
    mpCache.Add("PLAYER 2", tcmSelectPlayer, 1, tcmUpdatePlayer, 1);
    mpCache.Add("PLAYER 3", tcmSelectPlayer, 2, tcmUpdatePlayer, 2);
    mpCache.Add("PLAYER 4", tcmSelectPlayer, 3, tcmUpdatePlayer, 3);
    mpCache.Add("PLAYER 5", tcmSelectPlayer, 4, tcmUpdatePlayer, 4);
    mpCache.Add("PLAYER 6", tcmSelectPlayer, 5, tcmUpdatePlayer, 5);
    mpCache.Add("PLAYER 7", tcmSelectPlayer, 6, tcmUpdatePlayer, 6);
    mpCache.Add("PLAYER 8", tcmSelectPlayer, 7, tcmUpdatePlayer, 7);
    mpCache.Add("VOTE RETAIN:REMOVE", tcmVote, 0, tcmUpdateVote, 0);
    mpCache.Add("MUTE ON:OFF", tcmMute, 0, tcmUpdateMute, 0);
    mpCache.Add("VOTE ALL ON:OFF", tcmVoteAll, 0, tcmUpdateVoteAll, 0);
}

void tcmAddKindItems(FT_COMMAND kind);

namespace zdb {
class CTexture;
class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};
class CWorld {
public:
    static CTexHandle *GetTexHandle(const char *name);
};
}

/* a poly textured from a handle (hud_tcm.cpp's inline, through `this`: here the bool comes first, the poly in the
 * call's delay slot) */
inline void C2DBitmapPoly::SetTexture(zdb::CTexHandle *h)
{
    zdb::CTexture *tex = h ? h->m_texture : 0;

    m_texture = tex;
    m_tris[0].m_texture = tex;
    m_tris[1].m_texture = tex;
    SetHasTexture(tex != 0);
}

/* an item of a kind with its entries, under a group's parent (ours: the builder's ten copies of it): the
 * kind's item, then each entry under the kind's caption */
static inline void tcmAddKindEntries(signed char group, FT_COMMAND kind)
{
    const char *parent = func_0058DD70(group);
    const char *name = CSealCtrl::GetCommandName(kind);
    const char *caption = func_0058E950(kind);
    const char *nodename = func_0058E900(kind);

    tcmAddItem(name, parent, caption, nodename, func_0058E750(kind), false);

    TCM_ENTRY first = func_0058DF20(kind);
    TCM_ENTRY end = func_0058DEB0(kind);

    for (int i = first; i < end; i++) {
        TCM_ENTRY e = (TCM_ENTRY)i;
        const char *ename = func_0058EB00(e);
        const char *ecaption = func_0058E4D0(e);
        const char *enodename = func_0058E360(e);

        tcmAddItem(ename, caption, ecaption, enodename, func_0058E120(e), false);
    }
}

/* the menu built for the game's kind (0x232BC0: CHUD's call, 0x1FAF68; the name ours): the bars listed under the
 * root; the current cache's items -- the single player's teams and orders (0), the network game's whole
 * menu (1) or only its radio (2); the node actions and the mission's prompt on them; the bars' textures, the
 * root and the item bars given to the HUD, the action icon hidden and the sounds loaded */
void CTCM::Build(unsigned char mode)
{
    GetRoot()->push_back(&m_7d0);
    GetRoot()->push_back(&m_9c0);
    GetRoot()->push_back(&m_bb0);
    switch (mode) {
    case 2:
        SetCache(&mpCache);
        tcmAddItem("RADIO", 0, 0, 0, "Use the radio", false);
        tcmAddItem("RADIO ONOFF", "RADIO", 0, 0, "Radio (On, Off)", false);
        break;
    case 1:
        SetCache(&mpCache);
        tcmAddItem("NODEACTION ROOT", 0, 0, 0, "mission.rdr, 'tcm::nodeaction_root'", false);
        tcmAddItem("NODEACTION 01", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 02", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 03", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 04", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 05", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 06", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 07", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("NODEACTION 08", "NODEACTION ROOT", 0, 0, "actions.rdr, 'descriptor'", false);
        tcmAddItem("TACTICAL ORDERS", 0, 0, 0, "Issue tactical orders to your allies", false);
        tcmAddItem("MP HOLD FIRE", "TACTICAL ORDERS", "MP HOLD FIRE", "MP HOLD FIRE", "Tactical: Hold your fire", false);
        tcmAddItem("MP FIRE AT WILL", "TACTICAL ORDERS", "MP FIRE AT WILL", "MP FIRE AT WILL", "Tactical: Fire at will", false);
        tcmAddItem("MP CLEAR AREA", "TACTICAL ORDERS", "MP CLEAR AREA", "MP CLEAR AREA", "Tactical: Clear the area of hostiles", false);
        tcmAddItem("MP COVER ME", "TACTICAL ORDERS", "MP COVER ME", "MP COVER ME", "Tactical: Cover me", false);
        tcmAddItem("MP DEFUSE BOMB", "TACTICAL ORDERS", "MP DEFUSE BOMB", "MP DEFUSE BOMB", "Tactical: Defuse the bomb", false);
        tcmAddItem("MP PROTECT", "TACTICAL ORDERS", "MP PROTECT", "MP PROTECT", "Tactical: Protect target", false);
        tcmAddItem("MP POINT", "TACTICAL ORDERS", "MP POINT", "MP POINT", "Tactical: I'll take point", false);
        tcmAddItem("TAUNTS", 0, 0, 0, "Taunt your enemies", false);
        tcmAddItem("CHARACTER TAUNT", "TAUNTS", 0, 0, "Your character will taunt the enemies", false);
        tcmAddItem("CUSTOM TAUNT 01", "TAUNTS", 0, 0, "Send a custom taunt to your enemies", false);
        tcmAddItem("CUSTOM TAUNT 02", "TAUNTS", 0, 0, "Send a custom taunt to your enemies", false);
        tcmAddItem("CUSTOM TAUNT 03", "TAUNTS", 0, 0, "Send a custom taunt to your enemies", false);
        tcmAddItem("CUSTOM TAUNT 04", "TAUNTS", 0, 0, "Send a custom taunt to your enemies", false);
        tcmAddItem("CUSTOM TAUNT 05", "TAUNTS", 0, 0, "Send a custom taunt to your enemies", false);
        tcmAddItem("RADIO", 0, 0, 0, "Use the radio", false);
        tcmAddItem("ACTIVE CHANNEL", "RADIO", 0, 0, "Set the active radio channel", false);
        tcmAddItem("RADIO ONOFF", "RADIO", 0, 0, "Radio (On, Off)", false);
        tcmAddItem("TEAM", "ACTIVE CHANNEL", "TEAM", "TEAM", "Set radio channel to TEAM", false);
        tcmAddItem("OFFENSE", "ACTIVE CHANNEL", 0, 0, "Set radio channel to OFFENSE ", false);
        tcmAddItem("DEFENSE", "ACTIVE CHANNEL", 0, 0, "Set radio channel to DEFENSE", false);
        tcmAddItem("TEAMMATES", 0, 0, 0, "Teammate controls", false);
        tcmAddItem("PLAYER 1", "TEAMMATES", 0, 0, "Player 1", false);
        tcmAddItem("PLAYER 2", "TEAMMATES", 0, 0, "Player 2", false);
        tcmAddItem("PLAYER 3", "TEAMMATES", 0, 0, "Player 3", false);
        tcmAddItem("PLAYER 4", "TEAMMATES", 0, 0, "Player 4", false);
        tcmAddItem("PLAYER 5", "TEAMMATES", 0, 0, "Player 5", false);
        tcmAddItem("PLAYER 6", "TEAMMATES", 0, 0, "Player 6", false);
        tcmAddItem("PLAYER 7", "TEAMMATES", 0, 0, "Player 7", false);
        tcmAddItem("PLAYER 8", "TEAMMATES", 0, 0, "Player 8", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 1", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 1", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 2", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 2", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 3", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 3", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 4", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 4", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 5", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 5", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 6", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 6", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 7", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 7", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("VOTE RETAIN:REMOVE", "PLAYER 8", 0, 0, "Vote to Retain or Remove the selected player", false);
        tcmAddItem("MUTE ON:OFF", "PLAYER 8", 0, 0, "Mute (On, Off)", false);
        tcmAddItem("SPECTATORS", 0, 0, 0, "Spectator controls", false);
        tcmAddItem("VOTE ALL ON:OFF", "SPECTATORS", 0, 0, "Vote to Retain or Remove all players", false);
        tcmAddItem("MESSAGES", 0, 0, 0, "Send custom text messages", false);
        tcmAddItem("CUSTOM MSG 1", "MESSAGES", 0, 0, "Custom Team Message 1", false);
        tcmAddItem("CUSTOM MSG 2", "MESSAGES", 0, 0, "Custom Team Message 2", false);
        tcmAddItem("CUSTOM MSG 3", "MESSAGES", 0, 0, "Custom Team Message 3", false);
        tcmAddItem("CUSTOM MSG 4", "MESSAGES", 0, 0, "Custom Team Message 4", false);
        tcmAddItem("CUSTOM MSG 5", "MESSAGES", 0, 0, "Custom Team Message 5", false);
        break;
    case 0: {
        SetCache(&spCache);
        for (int i = 0; i < 6; i++) {
            const signed char t = s_tcmTeams[i];
            const char *name = func_0058DD70(t);

            tcmAddItem(name, 0, 0, 0, func_0058DBB0(t), false);
        }
        tcmAddItem("INTIMIDATE", 0, 0, 0, "Intimidate nearby enemies . . .", true);
        tcmAddItem("INTIMIDATE", 0, "DROP YOUR WEAPONS", "DROP YOUR WEAPONS", "Intimidate nearby enemies . . .", true);
        const char *name = CSealCtrl::GetCommandName((FT_COMMAND)20);
        const char *caption = func_0058E950((FT_COMMAND)20);
        const char *nodename = func_0058E900((FT_COMMAND)20);
        const char *text2 = func_0058E750((FT_COMMAND)20);

        tcmAddItem(name, func_0058DD70(1), caption, nodename, text2, true);
        tcmAddItem(name, func_0058DD70(2), caption, nodename, text2, true);
        tcmAddItem(name, func_0058DD70(3), caption, nodename, text2, true);
        tcmAddKindItems((FT_COMMAND)6);
        tcmAddKindItems((FT_COMMAND)3);
        tcmAddKindItems((FT_COMMAND)2);
        tcmAddKindItems((FT_COMMAND)15);
        tcmAddKindItems((FT_COMMAND)1);
        tcmAddKindItems((FT_COMMAND)8);
        tcmAddKindItems((FT_COMMAND)14);
        tcmAddKindItems((FT_COMMAND)16);
        tcmAddKindItems((FT_COMMAND)23);
        tcmAddKindItems((FT_COMMAND)10);
        tcmAddKindItems((FT_COMMAND)9);
        tcmAddKindItems((FT_COMMAND)11);
        tcmAddKindItems((FT_COMMAND)12);
        tcmAddKindItems((FT_COMMAND)13);
        tcmAddKindItems((FT_COMMAND)7);
        tcmAddKindItems((FT_COMMAND)4);
        tcmAddKindItems((FT_COMMAND)5);
        tcmAddKindItems((FT_COMMAND)17);
        tcmAddKindItems((FT_COMMAND)19);
        tcmAddKindEntries(17, (FT_COMMAND)4);
        tcmAddKindEntries(17, (FT_COMMAND)17);
        tcmAddKindEntries(17, (FT_COMMAND)5);
        tcmAddKindEntries(18, (FT_COMMAND)4);
        tcmAddKindEntries(18, (FT_COMMAND)17);
        tcmAddKindEntries(18, (FT_COMMAND)5);
        tcmAddKindEntries(18, (FT_COMMAND)18);
        tcmAddKindEntries(18, (FT_COMMAND)1);
        tcmAddKindEntries(29, (FT_COMMAND)21);
        tcmAddKindEntries(29, (FT_COMMAND)22);
        tcmAddItem("NICE ONE", 0, 0, 0, 0, true);
        tcmAddItem("WHASSUP", 0, 0, 0, 0, true);
        tcmAddItem("REPORT STATUS", 0, 0, 0, 0, true);
        tcmAddItem("CUE SCARY MUSIC", 0, 0, 0, 0, true);
        tcmAddItem("SAY AGAIN", 0, 0, 0, 0, true);
        break;
    }
    }
    theTCM.SetNodeActions();

    const char *prompt = theMission.GetTcmRootName();

    if (prompt && strlen(prompt)) {
        std::vector<CTCMNode *> found;

        theTCM.Find("NODEACTION ROOT", found, true);

        unsigned int n = found.size();

        if (n) {
            for (unsigned int i = 0; i < n; i++) {
                CTCMNode *node = found[i];

                if (node) {
                    node->m_text.SetString(prompt, true);
                    node->m_text2.SetString(prompt, true);
                }
            }
        }
    }
    zdb::CTexHandle *tex = zdb::CWorld::GetTexHandle("TCM.tif");

    m_9c0.SetTrans(0.0f);
    m_9c0.SetTexture(tex);
    m_9c0.SetUV(0.7f, 0.0f, 0.3f, 1.0f);
    m_bb0.SetTrans(0.0f);
    m_bb0.SetTexture(tex);
    m_bb0.SetUV(0.7f, 0.0f, 0.3f, 1.0f);
    theHUD->Add2D(&m_root, -1, true);
    SetupItemBars();
    m_items.SetTexture(zdb::CWorld::GetTexHandle("action_TCM.tif"));
    m_items.SetColor(0.0f, 0.0f, 0.0f);
    m_items.SetTrans(0.0f);
    m_items.SetPos(100.0f, 100.0f, 132.0f, 132.0f);
    m_items.Off();
    theHUD->Add2D(&m_items, -1, true);
    m_sounds.Load();
}

/* push-to-talk while the menu is closed in a network game: the radio on while the button is held
 * (for at most the longest talk), off with a short hold when it is let go */
void CTCM::TalkTick(float dt)
{
    if (data_0044D4F8 && theNetwork.m_bNetwork && m_mode.m_mode == 4) {
        CPad *pad = GetPad(ftsGetPlayer()->m_ctrl->m_padIndex);

        if (pad) {
            TCM_BUTTON st = func_002C64E0(10, pad);

            if (st == TCM_BUTTON_HELD && m_hold <= 0.0f && s_tcm_talktime < s_tcm_talkmax) {
                s_tcm_talking = true;
                s_tcm_talkheld = true;
                s_tcm_talktime += dt;
                func_0030E6E0(data_0045B718, 1);
            } else if (st == TCM_BUTTON_UP) {
                s_tcm_talktime = 0.0f;
                s_tcm_talking = false;
                if (s_tcm_talkheld) {
                    Hold(false, s_tcm_releasehold);
                    s_tcm_talkheld = false;
                }
                func_0030E6E0(data_0045B718, 0);
            }
        }
    }
}

/* the item tables of OVL2 the kind builder reads (no rows named yet but the kind's name, CSealCtrl's
 * GetCommandName): a group's parent name, a kind's name, caption, node name and second line, the
 * kind's first and end entry, and an entry's name, caption, node name and second line -- each a
 * byte argument (the callees narrow it). The kind is GetCommandName's FT_COMMAND for all five of
 * its readers (round 54): converted from an unsigned char at a call MW narrows it with an andi the
 * target never has, so the kind builder takes the enum too (its name ours, its row renamed) */

/* the kind's item under a group's parent, its caption returned (ours) */
static inline const char *tcmAddGroupItem(signed char group, FT_COMMAND kind)
{
    const char *parent = func_0058DD70(group);
    const char *name = CSealCtrl::GetCommandName(kind);
    const char *caption = func_0058E950(kind);
    const char *nodename = func_0058E900(kind);

    tcmAddItem(name, parent, caption, nodename, func_0058E750(kind), false);
    return caption;
}

/* one kind's items: under each of the first three groups' parents the kind's own item, then the
 * kind's entries under the third one's caption (0x2329F0: the cache builder 0x232BC0 calls it for
 * twenty kinds; the name ours). Each item's strings are named locals, fetched in argument order --
 * written as tcmAddItem's arguments MW calls them last to first; the group item an inline returning
 * its caption; the entries a byte enum, which gives the target's unnarrowed first entry and its
 * registers (as unsigned char locals MW narrows the first at the call and swaps two pairs) */
#pragma auto_inline off
void tcmAddKindItems(FT_COMMAND kind)
{
    tcmAddGroupItem(1, kind);
    tcmAddGroupItem(2, kind);

    const char *gcaption = tcmAddGroupItem(3, kind);
    TCM_ENTRY first = func_0058DF20(kind);
    TCM_ENTRY end = func_0058DEB0(kind);

    for (int i = first; i < end; i++) {
        TCM_ENTRY e = (TCM_ENTRY)i;
        const char *ename = func_0058EB00(e);
        const char *ecaption = func_0058E4D0(e);
        const char *enodename = func_0058E360(e);

        tcmAddItem(ename, gcaption, ecaption, enodename, func_0058E120(e), false);
    }
}
#pragma auto_inline reset

CTCMSounds::CTCMSounds()
{
    m_loaded = false;
    m_back = 0;
    m_slide = 0;
    m_select = 0;
}

#pragma auto_inline off
void CTCMSounds::Load()
{
    m_loaded = true;
    m_slide = CSnd::GetSoundByName(".TCM_SLIDE");
    m_select = CSnd::GetSoundByName(".TCM_SELECT");
    m_back = CSnd::GetSoundByName(".TCM_SELECT");
}
#pragma auto_inline reset

void CTCMSounds::Clear()
{
    m_loaded = false;
    m_slide = 0;
    m_select = 0;
    m_back = 0;
}

/* called out of line by the unit's own callers (Next): pinned */
#pragma auto_inline off
void CTCMSounds::Play(TCM_SOUND which)
{
    if (m_loaded) {
        switch (which) {
        case TCM_SND_SLIDE:
            if (m_slide)
                m_slide->PlaySound(0, 255);
            break;
        case TCM_SND_SELECT:
            if (m_select)
                m_select->PlaySound(0, 255);
            break;
        case TCM_SND_BACK:
            if (m_back)
                m_back->PlaySound(0, 255);
            break;
        }
    }
}
#pragma auto_inline reset

/* a string freed and cleared (ours: the name's, inlined in its setter and the destructor) */
static inline void FreeString(char *&s)
{
    if (s)
        _free(s, 0, 0);
    s = 0;
}

/* the node: hidden, no parent, entry or name; its backing, bar, both captions and icon listed
 * under its root; the backing untextured, translucent and black, the bar's right corners clear;
 * the captions at 0.9 scale; ready */
CTCMNode::CTCMNode()
{
    m_parent = 0;
    m_entry = 0;
    m_name = 0;
    m_734 = -1;
    m_state = TCM_HIDDEN;
    m_root.push_back(&m_back);
    m_root.push_back(&m_bar);
    m_root.push_back(&m_text);
    m_root.push_back(&m_text2);
    m_root.push_back(&m_icon);
    m_back.SetHasTexture(false);
    m_back.SetIsTrans(true);
    m_back.SetTrans(100.0f);
    m_back.SetColor(0.0f, 0.0f, 0.0f);
    m_bar.SetHasTexture(false);
    m_bar.SetIsTrans(true);
    m_bar.SetTrans(0, 100.0f);
    m_bar.SetTrans(1, 100.0f);
    m_bar.SetTrans(2, 0.0f);
    m_bar.SetTrans(3, 0.0f);
    m_bar.SetColor(0.0f, 0.0f, 0.0f);
    m_text.m_scale = 0.9f;
    m_text2.m_scale = 0.9f;
    m_delay = 0.0f;
    m_ready = true;
    m_741 = false;
    m_action = 0;
}

/* its children deleted, its name freed (a written destructor: its first test reads the parent, but
 * the target then takes the node out of its OWN children -- a0 is still `this` at the call; the -1
 * last stored is the flag the C2DString destructors take) */
CTCMNode::~CTCMNode()
{
    if (m_parent)
        RemoveChild(this);
    DeleteChildren();
    FreeString(m_name);
    m_entry = 0;
    m_734 = -1;
}

/* the delays counted down, the whole subtree's (self-inlined: each copy calls the accessors one
 * level shallower) */
void CTCMNode::Tick(float dt)
{
    if (IsWaiting()) {
        m_delay -= dt;
        if (m_delay < 0.0f) {
            m_delay = 0.0f;
            m_ready = true;
        }
    }
    if (!NoChildren()) {
        unsigned int n = GetNumChildren();

        for (unsigned int i = 0; i < n; i++)
            GetChild(i)->Tick(dt);
    }
}

/* the message's delay: only ever lengthened (the node waits until it runs out) */
void CTCMNode::SetDelay(float delay)
{
    if (!(delay <= m_delay)) {
        m_delay = delay;
        m_ready = false;
    }
}

void CTCMNode::SetEntry(CTCMEntry *entry)
{
    m_entry = entry;
}

/* the caption, cut to a length with a dash when it is longer (a length of 0 takes it whole) */
void CTCMNode::SetCaption(const char *caption, int len)
{
    char buf[128];

    if (len == 0) {
        m_text.SetString(caption, true);
        return;
    }
    if (caption && strlen(caption)) {
        strncpy(buf, caption, len);
        if (strlen(buf) > len - 1) {
            buf[len - 2] = '-';
            buf[len - 1] = 0;
        }
        m_text.SetString(buf, true);
    }
}

void CTCMNode::SetName(const char *name)
{
    if (m_name)
        FreeString(m_name);
    if (name)
        m_name = _strdup(name, 0, 0);
}

void CTCMNode::SetText2(const char *s)
{
    m_text2.SetString(s, true);
}

/* a child taken out of the list and returned (as the target does it: every later child copied
 * into its slot in turn, and the parent cleared through the found pointer before it is set --
 * never the child's; a signed counter, or MW unrolls the copy 8x).
 * Byte-exact since round 62, by two shapes (open since round 27 on t0/t1 and an add's operands):
 * - `(int)(n - 1)`: IRO_IsSubableExpression refuses `register local +/- constant` (mweppc
 *   IroSubable.c), so `n - 1` stayed in the outer loop for the back end to hoist as a code-generation
 *   temporary, numbered after every IRO temporary; a widening-or-equal int ETYPCON IS subable, so the
 *   outer loop's IRO_MoveInvariantExpressions hoists it into a temporary made right after the strength
 *   reduction's i * 4 and before the inner loop's data common subexpression -- the target's t1 / t0;
 * - the slot through a reference local: IRO_MoveInvarianceInAddressExpr (IroLoop.c) rewrites an
 *   EINDIRECT(EADD) in a loop with its loop-invariant addends first, so `m_children[i]` in the inner
 *   loop became (i * 4) + data; through a local the store's address is a variable while the loops are
 *   transformed, and copy propagation puts data + i * 4 back afterwards in its written order */
CTCMNode *CTCMNode::RemoveChild(CTCMNode *child)
{
    CTCMNode *found = 0;
    unsigned int n = m_children.size();

    for (unsigned int i = 0; i < n && !found; i++) {
        if (m_children[i] == child) {
            if (i != (int)(n - 1)) {
                for (int j = i + 1; j < n; j++) {
                    CTCMNode *&slot = m_children[i];
                    slot = m_children[j];
                }
                found->m_parent = 0;
                found = child;
            }
            m_children.pop_back();
        }
    }
    return found;
}

/* (a top-tested loop: inlined into the destructor after a join, a `while` is entered at its
 * bottom test) */
void CTCMNode::DeleteChildren()
{
    if (m_children.size())
        do {
            delete m_children.back();
            m_children.pop_back();
        } while (m_children.size());
}

CTCMNode *CTCMNode::AddChild(CTCMNode *child)
{
    m_children.push_back(child);
    child->m_parent = this;
    return child;
}

/* the children captioned with a name (and, if asked, theirs): pushed onto a list, counted */
int CTCMNode::Find(const char *name, std::vector<CTCMNode *> &found, bool recurse)
{
    int count = 0;

    if (name) {
        unsigned int n = m_children.size();
        unsigned int i;

        for (i = 0; i < n; i++) {
            const char *s = m_children[i]->m_text.m_string + m_children[i]->m_text.m_firstCharPos;

            if (s && !strcasecmp(s, name)) {
                found.push_back(m_children[i]);
                count++;
            }
        }
        /* a counter of its own (the shared one re-colours this loop) */
        if (recurse)
            for (unsigned int i = 0; i < n; i++)
                count += m_children[i]->Find(name, found, recurse);
    }
    return count;
}

/* the whole subtree's display off (self-inlined four times: each copy calls the accessors one level
 * shallower, the fifth calls them all and itself) */
void CTCMNode::HideAll()
{
    GetRoot()->Off();
    unsigned int n = GetNumChildren();

    for (unsigned int i = 0; i < n; i++)
        GetChild(i)->HideAll();
}

/* the node's state from its item's test (shown when it has none), its children's if asked; a
 * node with children is hidden when none of them is shown */
TCM_STATE CTCMNode::UpdateState(bool recurse)
{
    if (m_entry && m_entry->m_update) {
        tcmSetCurNode(this);
        m_state = (TCM_STATE)m_entry->m_update(ftsGetPlayer(), m_entry->m_updateArg);
        tcmClearCurNode();
    } else {
        m_state = TCM_SHOWN;
    }
    if (m_state != TCM_HIDDEN && recurse) {
        unsigned int n = m_children.size();

        if (n)
            for (unsigned int i = 0; i < n; i++)
                m_children[i]->UpdateState(recurse);
    }
    if (m_state != TCM_HIDDEN) {
        unsigned int n = m_children.size();

        if (n) {
            bool shown = false;

            for (unsigned int i = 0; i < n && !shown; i++)
                if (m_children[i]->m_state == TCM_SHOWN)
                    shown = true;
            if (!shown)
                m_state = TCM_HIDDEN;
        }
    }
    return m_state;
}
