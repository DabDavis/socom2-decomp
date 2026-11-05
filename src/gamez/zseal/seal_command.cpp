/* seal_command.cpp -- the orders: the AI's and the pad's controllers taking them, the SEAL issuing
 * them, which are allowed (SOCOM 1's unit, __sinit_seal_command.cpp, in SOCOM 1's order). OVL2
 * 0x58FBE0 (CSealCtrlAi::OnCommand) to seal_control.cpp (0x5920A0). Defined in reverse of their
 * image order (-inline deferred). */

#include "zseal.h"

extern "C" {
/* a switch that lets a charge be set anywhere (OVL2 bss 0x66B030; the name ours) */
extern unsigned char data_0066B030;
}

extern "C" {
/* the action whose node is nearest a point and its distance, and whether an action can start (OVL1
 * 0x2B4B40, 0x2B46F0: fts_action.cpp's -- the second's bool taken as an int, as the target's caller
 * reads it with no andi) */
CNodeAction *func_002B4B40(const CPnt3D *pos, float *dist, float height);
int func_002B46F0(CNodeAction *action);
}

/* whether a living SEAL may send another to an action near itself: its team's bit 18 set, the other
 * on its side (an inline, the name ours: the AI's controller asked for and dropped) */
static inline bool CanSendToNearAction(CZSealBody *seal, CZSealBody *other)
{
    if (!seal || !seal->m_isAlive)
        return false;
    unsigned int team = seal->m_team;
    bool bit = (team & 0x40000) != 0;
    bool same = other ? (other->m_team & team) != 0 : true;
    bool r = false;

    if (same && bit && other) {
        seal->GetSealCtrlAi();
        r = true;
    }
    return r;
}

/* the action an order names: the squad's order data (1), or the other SEAL's reticule action (0) (an
 * inline, the name ours: the squad read before the switch, the result left in v0) */
static inline CNodeAction *OrderAction(CSealUnit *unit, CZSealBody *other, unsigned char which)
{
    CNodeAction *a = 0;

    switch (which) {
    case 1:
        a = (CNodeAction *)unit->GetDataPtr();
        break;
    case 0:
        if (other)
            a = other->GetReticuleAction(false);
        break;
    }
    return a;
}

/* the action an order to another SEAL is about: the squad's order data (1), or the other's reticule
 * action (0); failing those, the action nearest the SEAL within 20 when the inline allows -- one of
 * kind 2 only within 250 (OVL2 0x591380, new in SOCOM II: CSealCtrlAi::OnCommand and 0x5911F0 ask
 * it; the name unknown). Pinned: both callers call it, and 0x5911F0 would inline it */
#pragma auto_inline off
extern "C" CNodeAction *func_00591380(CZSealBody *seal, CZSealBody *other, unsigned int which)
{
    CSealCtrlAi *ai = seal->GetSealCtrlAi();
    CNodeAction *action = 0;

    if (ai && other) {
        action = OrderAction(ai->m_unit, other, which);
        if (!action && CanSendToNearAction(seal, other)) {
            float dist = 0.0f;

            action = func_002B4B40(&seal->m_posW, &dist, 20.0f);
            if (action && action->m_type == 2 && !(dist <= 250.0f))
                action = 0;
        }
    }
    return action;
}
#pragma auto_inline reset

extern "C" {
/* whether the game is online (OVL2 0x5750A0, seal_ai.cpp's: theNetwork's second byte) and the SEAL a
 * SEAL is looking at, its cached one or its reticule node's (OVL2 0x5AA690, seal_env.cpp's) */
int func_005750A0(void);
CZSealBody *func_005AA690(CZSealBody *seal);
/* a switch that lets any order through (OVL2 bss; the name ours) */
extern bool data_0066B3B8;
}

/* the steps between two cells, across and down (an inline, the name ours: both differences ints -- the
 * x field's compare is unsigned all the same, the y's, read as a halfword, signed) */
static inline unsigned int CellSteps(const CAiMapLoc &a, const CAiMapLoc &b)
{
    int dx = a.x > b.x ? a.x - b.x : b.x - a.x;
    int dy = a.y > b.y ? a.y - b.y : b.y - a.y;

    return dx + dy;
}

/* whether a SEAL is one an order may concern: alive; the switch let through, or a hostage (team bit 17)
 * restrained and not in AI state 17 or online -- and, with another SEAL named, that one on its side,
 * looking at it and within 20 cells (OVL2 0x591DC0, new in SOCOM II: CSealCtrlAi::AllowCommand's
 * cases 8-13 and AllowCommandHOLDPOS/FOLLOW ask it; the name unknown) */
extern "C" bool func_00591DC0(CZSealBody *seal, CZSealBody *other)
{
    if (!seal || !seal->m_isAlive)
        return false;
    bool state17 = false;
    unsigned int team = seal->m_team;
    bool hostage = (team & 0x20000) != 0;
    bool restrained = seal->m_restrained;
    bool same = other ? (other->m_team & team) != 0 : true;
    CSealCtrlAi *ai = seal->GetSealCtrlAi();

    if (ai)
        state17 = ai->AiStateIs((AI_STATE)17);
    bool r = data_0066B3B8;

    if (!r) {
        r = (hostage && restrained && !state17) || (hostage && func_005750A0());
        if (r && other) {
            r = same && seal == func_005AA690(other);
            if (r)
                r = CellSteps(seal->MapLocValid(), other->MapLocValid()) < 20;
        }
    }
    return r;
}

extern "C" {
/* a switch that lets an escort's order through (OVL2 bss 0x66B3B0, beside 0x66B3B8; the name ours) and
 * whether the AI may take a stance (OVL2 0x562C80, seal_ai.cpp's) */
extern bool data_0066B3B0;
bool func_00562C80(CSealCtrlAi *ai, SEAL_STANCE stance);
}

/* whether a SEAL is one an escort's order may concern: alive, on the asker's side, an escort (team bit
 * 18) asked by another SEAL -- through the switch, or by the SEAL it looks at (an inline, the name ours:
 * 0x591DC0's shape, its AI asked and not used). The switch read into the caller's local through a
 * reference: as the inline's own local MW numbers it after the caller's and it takes s0 (the target's
 * s2, between the result and the SEAL) */
static inline bool Escorts(CZSealBody *seal, CZSealBody *other, bool &sw)
{
    sw = data_0066B3B0;
    if (!seal || !seal->m_isAlive)
        return false;
    unsigned int team = seal->m_team;
    bool escort = (team & 0x40000) != 0;
    bool same = other ? (other->m_team & team) != 0 : true;
    bool r = false;

    if (same && escort && other) {
        seal->GetSealCtrlAi();
        r = !sw || seal == func_005AA690(other);
    }
    return r;
}

/* whether an order to come along may go to the SEAL: as one to hold position, else not prone and the
 * AI in its following state for its side -- a hostage's state 23, an escort's or a SEAL's state 2 --
 * and free to stand (OVL2 0x591520, new in SOCOM II: CSealCtrlAi::AllowCommand calls it; the name
 * unknown) */
extern "C" bool func_00591520(CSealCtrl *ctrl, CZSealBody *other)
{
    CSealCtrlAi *ai = ctrl->GetSealCtrlAi();
    bool ok = false;
    bool sw;

    if (ai) {
        CZSealBody *seal = (CZSealBody *)ai->m_entity;

        ok = ctrl->AllowCommandHOLDPOS(other);
        if (!ok && !ai->InStance((SEAL_STANCE)2, true)) {
            unsigned int team = seal->m_team;
            bool escort = (team & 0x40000) != 0;
            bool seal_side = (team & 2) != 0;

            if (team & 0x20000)
                ok = ai->AiStateIs((AI_STATE)23);
            else if (escort && Escorts(seal, other, sw))
                ok = ai->AiStateIs((AI_STATE)2);
            else if (seal_side)
                ok = ai->AiStateIs((AI_STATE)2);
        }
        if (ok && ai)
            ok = func_00562C80(ai, (SEAL_STANCE)2);
    }
    return ok;
}

/* whether an order to act may go to another SEAL: the action it names (0x591380) for the SEAL's side,
 * able to start and not in use, the AI free to take it on (state 11); a map action of kind 3 always, of
 * kind 1 or 2 only for a Navy SEAL -- the squad's order data then pointed at it (OVL2 0x5911F0, new in
 * SOCOM II: CSealCtrlAi::AllowCommand's case 1, where SOCOM 1 tested the reticule's action in place;
 * the name unknown) */
/* an entity's team bits (an inline, the name ours: expanded ahead of the `&`, the bits load first though
 * they are its right operand -- the HUD's command menu reads its player's the same way) */
static inline unsigned int TeamOf(const CEntity *e) { return e->m_team; }

extern "C" bool func_005911F0(CSealCtrl *ctrl, CZSealBody *other, void *data)
{
    CSealCtrlAi *ai = ctrl->GetSealCtrlAi();
    bool ok = false;

    if (ai && other) {
        CZSealBody *seal = (CZSealBody *)ctrl->m_entity;
        CNodeAction *action = 0;
        CSealUnit *unit = ai->m_unit;

        action = func_00591380(seal, other, (unsigned int)data);
        if (action) {
            bool b = (action->m_teams & TeamOf(seal)) != 0;

            if (b)
                b = func_002B46F0(action) != 0;
            if (b && action->m_owner == CEntity::invalidId && ai->CanPushAiState((AI_STATE)11, 0)) {
                CAiMapLoc loc;
                CAiMap::action *mapaction = 0;

                if (AiMaps.GetActionById(action->m_id, loc, &mapaction)) {
                    switch (mapaction->GetType()) {
                    case 1:
                    case 2:
                        ok = ai->m_navyseal != 0;
                        break;
                    case 3:
                        ok = true;
                        break;
                    }
                }
                if (ok)
                    unit->SetDataPtr(action);
            }
        }
    }
    return ok;
}

/* whether an item can be deployed at an action: held, with ammunition, and a breaching charge only
 * at a breach unless the switch is on (OVL2 0x591130; SOCOM 1's with the switch) */
bool CanDeploy(CZKit *kit, EQUIP_ITEM item, CNodeAction *action)
{
    NODEACTION_TYPE type = action ? action->m_type : NODEACTION_NONE;
    bool ok = false;

    if (kit->HasItem(item) && kit->ItemHasAmmo(kit->GetItemIndex(item))) {
        if (item != 152 || data_0066B030 || type == NODEACTION_BREACH)
            ok = true;
    }
    return ok;
}

/* an order given: order 17 only if the controller allows it; flagged once given (OVL2 0x590AC0;
 * SOCOM 1's, whose test was order 11) */
void CZSealBody::IssueCommand(CZSealBody *seal, FT_COMMAND cmd, void *data, bool b)
{
    CSealCtrl *ctrl = (CSealCtrl *)m_control;

    switch (cmd) {
    case (FT_COMMAND)17:
        if (ctrl->AllowCommand(seal, cmd, data)) {
            ctrl->OnCommand(seal, cmd, data, b);
            m_order_flag = true;
        }
        break;
    default:
        ctrl->OnCommand(seal, cmd, data, b);
        m_order_flag = true;
        break;
    }
}
