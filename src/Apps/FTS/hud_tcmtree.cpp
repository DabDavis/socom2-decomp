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
 * tick, push-to-talk, close, open, root, next and the item searches. OPEN: RemoveChild,
 * ShowRoot, Select (registers). Not yet: the voice items' adder 0x2329F0 (its loop passes the
 * third block's caption register as the item's argument -- an uninitialised local?), the cache
 * builders 0x232BC0/0x2344E0, the open menu's and closed menu's pad handlers 0x235D40/0x2359A0.
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

/* the controller as the menu reads it (zseal.h's CSealCtrl) */
class CSealCtrl {
public:
    void SetMenuState(MENU_STATE state, bool b);
    void StopRumbler();

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
};

CZSealBody *ftsGetPlayer();

/* OVL2's: the player's menu flag (by address) and OVL1's pad call (by address) */
extern "C" void func_005AA620(CZSealBody *seal, int on);
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

/* hud_tcmcmds.cpp's (0x231690, not written yet): the menu laid out */
extern "C" void func_00231690(CTCM *tcm, float t);

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
static float s_tcm_talkmax = 10.0f;   // 0x3DCB00: the longest push-to-talk, and the close test's wait
static float s_tcm_releasehold = 1.0f;   // 0x3DCB08: the hold after a release

/* the push-to-talk state (bss 0x4130A0-0x4130B8; the names ours) */
static bool s_tcm_talking;   // 0x4130A0
static float s_tcm_talktime;   // 0x4130A8
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

/* the menu's input by its mode (hud_tcmtree.cpp's 0x235D40 and 0x2359A0, not written yet) */
extern "C" void func_00235D40(CTCM *tcm, CPad *pad);
extern "C" void func_002359A0(CTCM *tcm, CPad *pad);

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
        func_00235D40(this, pad);
        break;
    case 4:
        func_002359A0(this, pad);
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
 * first).
 * OPEN, 4 instructions, one register pair: the target keeps the walked child in v1 (the address
 * temporary's register) and its state byte in a1, MW the child in a1 and the byte in v1 -- a named
 * child at the top, after the counters, through GetChild or CTCMNodeList::Get, or none (the
 * element read twice) all tried */
void CTCM::ShowRoot()
{
    unsigned int n = m_children.size();
    int first = -1;

    for (unsigned int i = 0; i < n && first == -1; i++) {
        CTCMNode *c = m_children[i];

        if (c->m_state == TCM_SHOWN)
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
    func_00231690(&theTCM, 0.0f);
}

/* the menu opened: the player's controls in the menu state, the states updated, the root shown,
 * the compass off, the recogniser listening */
void CTCM::Open()
{
    CZSealBody *player = ftsGetPlayer();
    CSealCtrl *ctrl = player->m_ctrl;
    CPad *pad = GetPad(ctrl->m_padIndex);

    func_005AA620(player, 1);
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
    func_00231690(this, 0.0f);
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
    func_005AA620(player, 0);
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
 * declared first; the column assigned in its test).
 * OPEN, 21 instructions, the size right: the target reads the count through v0 (the column's copy
 * in the test's delay slot) where MW reads it through the copy, and sets the result flag with
 * `move s5,zero ... movn s5,a0,v1` (the 1 the state's compare loaded) where MW gives `sltu` -- an
 * unsigned char or bool flag, `r = 0; if (size) r = 1;`, `!empty()`, the count assigned in the
 * column's test all tried */
int CTCM::Select(const char *name)
{
    int done = 0;
    CTCMNode *c;

    if (name && strlen(name)) {
        CTCMNode *column;

        if ((column = GetColumn(m_path.size() - 1)) != 0) {
            unsigned int n = column->m_children.size();

            if (n)
                for (int i = 0; i < n && !done; i++) {
                    c = column->m_children[i];
                    const char *s = c ? c->m_name : 0;

                    if (s && strlen(s) && s && !strcasecmp(s, name)) {
                        m_path.pop_back();
                        m_path.push_back(i);
                        int r = c->m_children.size() ? 1 : 0;
                        CTCMEntry *e = c->m_entry;

                        if (c->m_state != TCM_HIDDEN) {
                            if (e->m_action) {
                                tcmSetCurNode(c);
                                r = e->m_action(ftsGetPlayer(), e->m_actionArg);
                                tcmClearCurNode();
                            }
                            switch ((unsigned char)r) {
                            case 1:
                                tcmOpenNode(0, c);
                                break;
                            case 0:
                                m_sounds.Play(TCM_SND_BACK);
                                Close(true);
                                theReco.Suspend(true);
                                break;
                            }
                            done = 1;
                        } else {
                            tcmSetRecoMode(true);
                            if (e->m_action)
                                e->m_action(ftsGetPlayer(), e->m_actionArg);
                            tcmSetRecoMode(false);
                            m_sounds.Play(TCM_SND_BACK);
                            Close(true);
                            theReco.Suspend(true);
                            done = 1;
                        }
                    }
                }
        }
    }
    return done;
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

CTCMSounds::CTCMSounds()
{
    m_loaded = false;
    m_back = 0;
    m_slide = 0;
    m_select = 0;
}

void CTCMSounds::Load()
{
    m_loaded = true;
    m_slide = CSnd::GetSoundByName(".TCM_SLIDE");
    m_select = CSnd::GetSoundByName(".TCM_SELECT");
    m_back = CSnd::GetSoundByName(".TCM_SELECT");
}

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
 * OPEN, 5 instructions, registers: the target computes `n - 1` into t1 and the data pointer into
 * t0 and adds the store's address data first (`addu a2,t0,t2`), MW swaps the two and adds the
 * outer offset first -- `n - 1` as a local, `n - 1 != i`, begin()/data() indexing, a local for the
 * copied child, signed outer counters all tried */
CTCMNode *CTCMNode::RemoveChild(CTCMNode *child)
{
    CTCMNode *found = 0;
    unsigned int n = m_children.size();

    for (unsigned int i = 0; i < n && !found; i++) {
        if (m_children[i] == child) {
            if (i != n - 1) {
                for (int j = i + 1; j < n; j++)
                    m_children[i] = m_children[j];
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
