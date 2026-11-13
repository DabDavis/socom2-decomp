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
/* whether the game is online (OVL2 0x5750A0, seal_ai.cpp's: theNetwork's second byte) */
bool func_005750A0(void);
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
            r = same && seal == other->GetReticuleSeal();
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
 * s2, between the result and the SEAL). The result declared first and set late: numbered ahead of the
 * escort bit it takes a0 and the bit v1 in AllowCommandFOLLOW, as the target has them (declared after
 * them, the two swap; set where it is declared, 0x591520 is lost) */
static inline bool Escorts(CZSealBody *seal, CZSealBody *other, bool &sw)
{
    sw = data_0066B3B0;
    if (!seal || !seal->m_isAlive)
        return false;
    bool r;
    unsigned int team = seal->m_team;
    bool escort = (team & 0x40000) != 0;
    bool same = other ? (other->m_team & team) != 0 : true;

    r = false;

    if (same && escort && other) {
        seal->GetSealCtrlAi();
        r = !sw || seal == other->GetReticuleSeal();
    }
    return r;
}

extern "C" {
/* whether two SEALs' AIs share a group (OVL2 0x5EF340, the kit lane's range: both controllers' group
 * words, none for either -- its draft matches) */
bool func_005EF340(CZSealBody *a, CZSealBody *b);
}

/* a negation as a value (a marked device, the name ours): the target materialises `!x` (sltu, xori)
 * before branching on it where a plain `!` in the condition branches on x -- the offline test of
 * AllowCommandFOLLOW / HOLDPOS's Navy flag and the tests below (README: an inline bool returning a
 * negation, used inside `&&`, is materialised) */
static inline bool NotB(bool b) { return !b; }

/* whether an escort's AI may take the following state: no state of kind 1 on the stack, not in state 9,
 * and free to push it (an inline, the name ours: the target narrows its result into the caller's `r`
 * at the join, `andi s1,s1,0xff`, as an inline's bool returned into a local does) */
static inline bool TakesFollow(CSealCtrlAi *ai)
{
    bool r = false;
    bool free = false;

    if (NotB(ai->m_stateStack.m_10 & 2) && NotB(ai->AiStateIs((AI_STATE)9)))
        free = true;
    if (free && ai->CanPushAiState((AI_STATE)9, 0))
        r = true;
    return r;
}

/* whether an order to follow may go to the SEAL (SOCOM 1's AllowCommandFOLLOW; OVL2 0x591AB0, SOCOM
 * II's by the SEAL's side): none from a player's controller or with no SEAL named; a hostage (team bit
 * 17) as 0x591DC0 says, in squad 17 -- or offline in squad 3; an escort (bit 18) as Escorts says, in
 * squad 18, its AI (when it has one) with no state of kind 1 on the stack, not in state 9 and free to
 * take it; a Navy SEAL's side (bit 1) offline when its AI is not in state 9. The SEAL is DECLARED LAST
 * and assigned first: MW numbers named locals in declaration order, and only numbered after `sw` does
 * it take s0 with the result in s1 and the online flag in s2 (`igreplay --movers`: any place after
 * `sw`); declared last and assigned after the call, its load leaves the call's delay slot (+8) */
bool CSealCtrl::AllowCommandFOLLOW(CZSealBody *other)
{
    if (!other || IsPlayer())
        return false;
    bool online;
    unsigned int team;
    bool hostage;
    bool escort;
    bool navy;
    bool r;
    bool sw;
    CZSealBody *seal;

    seal = (CZSealBody *)m_entity;
    online = func_005750A0();
    team = seal->m_team;
    hostage = (team & 0x20000) != 0;
    escort = (team & 0x40000) != 0;
    navy = false;

    if ((team & 2) && NotB(online))
        navy = true;
    r = false;
    if (hostage) {
        r = func_00591DC0(seal, other);
        if (r)
            r = CSealUnit::IsMemberOfUnit(17, seal, true) || (!online && CSealUnit::IsMemberOfUnit(3, seal, true));
    } else if (escort) {
        r = Escorts(seal, other, sw) && CSealUnit::IsMemberOfUnit(18, seal, true);
        if (r) {
            CSealCtrlAi *ai = GetSealCtrlAi();

            if (ai)
                r = TakesFollow(ai);
        }
    } else if (navy) {
        CSealCtrlAi *ai = GetSealCtrlAi();

        if (ai)
            r = !ai->AiStateIs((AI_STATE)9);
    }
    return r;
}

/* whether the AI's current state is of kind 2 with flag 0x20 / 0x80 (inlines, the names ours:
 * seal_ai.cpp's 0x559000 shape) */
static inline bool IsKind2Flag(CAiState *s, unsigned int flag)
{
    bool r = false;

    AI_STATE st = s->m_state;

    if (st == (AI_STATE)2 && (s->m_flags & flag))
        r = true;
    return r;
}

static inline bool InKind2Flag(const CAiStateStack &st, unsigned int flag)
{
    CAiState *s;

    if (st.size() == 0)
        s = 0;
    else
        s = st.GetState();
    return s ? IsKind2Flag(s, flag) : false;
}

/* whether an order to hold position may go to the SEAL (SOCOM 1's AllowCommandHOLDPOS; OVL2 0x591770,
 * SOCOM II's by the SEAL's side): none from a player's controller or with no SEAL named; a hostage
 * (team bit 17) as 0x591DC0 says, out of squad 17 and online only in the asker's group; an escort (bit
 * 18) as Escorts says, out of squad 18; a Navy SEAL's side (bit 1) offline when its AI is not following
 * (state 2) or follows with flag 0x20, or else with flag 0x80. FOLLOW's numbering: the SEAL declared
 * last (s0, sharing `this`'s register), and the AI a node of its own declared first (s2) -- asked
 * through the control's virtual directly; through CZSealBody's inline GetSealCtrlAi the assigned-once
 * local is the inline's result temporary, numbered last, and takes s0 (`igreplay --move 34=40.5
 * --move 55=34.5`). The Navy block's result is a block local copied into `r`: the hostage's `r` then
 * stays in v0, the block's in s1 (with `r` itself preset to true, one web over every arm, the hostage's
 * takes a saved register) */
bool CSealCtrl::AllowCommandHOLDPOS(CZSealBody *other)
{
    if (!other || IsPlayer())
        return false;
    CSealCtrlAi *ai;
    bool online;
    unsigned int team;
    bool hostage;
    bool escort;
    bool navy;
    bool r;
    bool sw;
    CZSealBody *seal;

    seal = (CZSealBody *)m_entity;
    ai = ((CSealCtrl *)seal->GetControl())->GetSealCtrlAi();
    online = func_005750A0();
    team = seal->m_team;
    hostage = (team & 0x20000) != 0;
    escort = (team & 0x40000) != 0;
    navy = false;

    if ((team & 2) && NotB(online))
        navy = true;
    r = false;
    if (hostage) {
        r = func_00591DC0(seal, other);
        if (r)
            r = !CSealUnit::IsMemberOfUnit(17, seal, true) && (!online || func_005EF340(seal, other));
    } else if (escort)
        r = Escorts(seal, other, sw) && !CSealUnit::IsMemberOfUnit(18, seal, true);
    else if (navy && ai) {
        bool res = true;
        bool ok = true;

        if (!NotB(ai->AiStateIs((AI_STATE)2)) && !InKind2Flag(ai->m_stateStack, 0x20))
            ok = false;
        if (!ok && !InKind2Flag(ai->m_stateStack, 0x80))
            res = false;
        r = res;
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
        if ((unsigned char)item != 152 || data_0066B030 || type == NODEACTION_BREACH)
            ok = true;
    }
    return ok;
}

extern "C" {
/* the order helpers (OVL2 0x58F190, 0x58F230, 0x58F6E0, seal_orders.cpp's -- the order's data handed on
 * as it stands; 0x5664F0, seal_ai.cpp's; 0x5EC230, seal_unit.cpp's; 0x5F4360, the states' -- whether a
 * state's animation has a sequence) */
EQUIP_ITEM func_0058F190(void *target);
bool func_0058F230(CSealUnit *unit, CZSealBody *seal, void *target, CAiMapLoc *out);
bool func_0058F6E0(const CPnt3D *from, const CPnt3D *to, CAiMapLoc *out, bool along);
bool func_005664F0(EQUIP_ITEM item);
bool func_005EC230(CZSealBody *from, CZSealBody *seal, CNodeAction *na, void *data);
int func_005F4360(CAiState *state, const char *name);
}

/* whether a living SEAL, an escort (its team's bit 18), on the asker's side, is the one the asker looks at
 * (an inline, the name ours: its AI asked and not used, Escorts' shape without the switch) */
static inline bool EscortLooks(CZSealBody *seal, CZSealBody *other)
{
    if (!seal || !seal->m_isAlive)
        return false;
    unsigned int team = seal->m_team;
    bool escort = (team & 0x40000) != 0;
    bool same = other ? (((const CEntity *)other)->m_team & team) != 0 : true;
    bool r = false;

    if (same && escort && other) {
        seal->GetSealCtrlAi();
        r = seal == other->GetReticuleSeal();
    }
    return r;
}

/* whether an order may be given (SOCOM 1's CSealCtrlAi::AllowCommand; slot 0x28 of its table, OVL2
 * 0x590BC0): the SEAL alive and on the asker's side, then per order */
bool CSealCtrlAi::AllowCommand(CZSealBody *other, FT_COMMAND cmd, void *data)
{
    signed char mode;
    bool ok;
    EQUIP_ITEM it;
    CZSealBody *seal;
    CAiMapLoc loc_deploy;
    CAiMapLoc loc_order;
    CAiMapLoc loc_cell;

    seal = (CZSealBody *)m_entity;
    mode = (signed char)m_26c;
    ok = false;

    if (seal && seal->m_isAlive && (((const CEntity *)other)->m_team & seal->m_team)) {
        CSealUnit *unit = m_unit;

        switch (cmd) {
        case 1:
            ok = func_005911F0(this, other, data);
            break;
        case 14:
        case 23: {
            CPnt3D center;

            bool r = func_0058F230(unit, other, data, &loc_cell);

            if (r)
                r = AiMaps.GetCellCenter(loc_cell, &center) != 0;
            ok = r;
            break;
        }
        case 2: {
            EQUIP_ITEM item = (EQUIP_ITEM)255;

            switch ((unsigned char)data) {
            case 1:
                unit->GetData(&item, 4);
                break;
            default:
                item = func_0058F190(data);
                break;
            }
            it = item;
            if ((unsigned char)it == 255)
                break;
            CZKit *const kit = &seal->m_kit;
            CNodeAction *action = other->GetReticuleAction(false);

            if (CanDeploy(kit, it, action)) {
                ok = CanPushAiState((AI_STATE)4, 0);
                if (ok && func_005664F0(it))
                    ok = func_0058F6E0(&other->m_1464, &other->m_posW, &loc_deploy, true);
            }
            break;
        }
        case 3:
            ok = m_aibit16;
            break;
        case 15:
            ok = false;
            break;
        case 17:
            ok = func_00591520(this, other);
            break;
        case 4:
            ok = AllowCommandFOLLOW(other);
            break;
        case 7:
            ok = true;
            break;
        case 5:
            ok = AllowCommandHOLDPOS(other);
            break;
        case 6:
            ok = !m_aibit16;
            break;
        case 20:
            ok = m_aibit16 && !m_aibit17;
            break;
        case 19:
            ok = (unsigned char)mode == 1;
            break;
        case 18: {
            CSealCtrlAi *ai = GetSealCtrlAi();
            bool r = false;

            if (ai) {
                CZSealBody *s = (CZSealBody *)m_entity;

                if (EscortLooks(s, other) || func_00591DC0(s, other)) {
                    CAiState *st = ai->FindAiState((AI_STATE)1);

                    st = st && st->m_state == (AI_STATE)1 ? st : 0;
                    r = st != 0 && func_005F4360(st, "on_pullout") != 0;
                }
            }
            ok = r;
            break;
        }
        case 26:
            ok = false;
            break;
        case 25: {
            bool r = false;
            bool has = other != 0;

            if (has)
                has = other->GetReticuleSeal() != 0;
            if (has && other->GetReticuleSeal()->m_restrainable)
                r = true;
            ok = r;
            break;
        }
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
            ok = func_0058F230(unit, other, data, &loc_order);
            break;
        case 16: {
            CNodeAction *action = 0;

            if (other)
                action = other->GetReticuleAction(false);
            ok = func_005EC230(other, seal, action, data);
            break;
        }
        case 8: {
            CZSealBody *t = other->GetReticuleSeal();

            if (t && func_00591DC0(t, 0) && !CSealUnit::IsMemberOfUnit(3, t, true)) {
                static float k_range2 = 22500.0f;
                CPnt3D d = t->m_posW - seal->m_posW;

                if (d.x * d.x + d.y * d.y + d.z * d.z < k_range2)
                    ok = true;
            }
            break;
        }
        }
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

extern "C" {
/* the helpers the AI's order handler calls (OVL2 0x555900, 0x5628F0, 0x564D20: seal_ai.cpp's; 0x5EBB50,
 * seal_unit.cpp's; 0x5F0180, 0x5F0830, 0x5FD010: the states') */
CAiState *func_00555900(CSealCtrlAi *ai, unsigned char kind, unsigned int flags);
void func_005628F0(CSealCtrlAi *ai);
bool func_00564D20(CSealCtrlAi *ai, EQUIP_ITEM item, const CPnt3D *target, const CPnt3D *from,
    unsigned int flags);
void func_005EBB50(void *data, CSealCtrlAi *ai);
void func_005F0180(CAiSAction *s, CSealCtrlAi *ai, CNodeAction *na, int mode);
bool func_005F0830(CSealCtrlAi *ai, void *loc, unsigned int mode, int kind, int step);
void func_005FD010(CAiSFollow *s, CSealCtrlAi *ai, CZSealBody *master);
}
/* a radio message sent to the other consoles (OVL2 0x5E7F20; hud_tcmcmds.cpp declares it alike) */
void PlayRemoteMsg(CEntity *from, int msg, int flags);

/* whether a cell's map lets it be walked (an inline, the name ours) */
static inline bool Traversable(const CAiMapLoc &loc)
{
    CAiMap *map = AiMaps.GetMap(loc);

    return map ? map->IsTraversable(loc) : false;
}

/* an escort or a hostage: team bit 17 or 18 (an inline, the name ours: the team read at each test) */
static inline bool Escort(CZSealBody *s)
{
    return (s->m_team & 0x20000) || (s->m_team & 0x40000);
}

static inline bool OrderCenter(CSealUnit *unit, CZSealBody *other, CPnt3D *center)
{
    CAiMapLoc loc;
    bool r = func_0058F230(unit, other, (void *)2, &loc);

    if (r)
        r = AiMaps.GetCellCenter(loc, center) != 0;
    return r;
}

/* an order taken (SOCOM 1's CSealCtrlAi::OnCommand; slot 0x84 of its table, OVL2 0x58FBE0) */
void CSealCtrlAi::OnCommand(CZSealBody *other, FT_COMMAND cmd, void *data, bool b)
{
    CZSealBody *seal = (CZSealBody *)m_entity;

    if (other)
        other->GetSealCtrlAi();
    CSealUnit *unit = m_unit;

    switch (cmd) {
    case 2:
        if (other) {
            EQUIP_ITEM item = (EQUIP_ITEM)255;

            switch ((unsigned char)data) {
            case 1:
                unit->GetData(&item, 4);
                break;
            default:
                item = func_0058F190(data);
                break;
            }
            bool ok = func_00564D20(this, item, &other->m_1458, &other->m_posW, 8);
            CAiState *s = GetAiState();

            s = s && s->m_state == AI_STATE_DEPLOY ? s : 0;
            if (!ok || !s || !s->m_active)
                PlayAiSound((CHARACTER_SOUND)56, 1.0f, CAiEvent::AIE_NONE);
        }
        break;
    case 18:
        if (m_stateStack.m_10 & 2) {
            CAiState *s = FindAiState(AI_STATE_SCRIPT);

            s = s && s->m_state == AI_STATE_SCRIPT ? s : 0;
            if (s && func_005F4360(s, "on_pullout") && PopToAiState(AI_STATE_SCRIPT, false))
                ((CAiSScript *)s)->ActivateSequence("on_pullout", 11);
        }
        break;
    case 3:
    case 6:
        m_aibit16 = cmd == 6;
        m_aibit17 = false;
        break;
    case 20:
        m_aibit17 = true;
        break;
    case 14:
        m_aibit16 = false;
        m_aibit17 = false;
    case 5:
    case 17:
        func_005628F0(this);
        StopThrottles(true);
        ClearAiState(false);
        if (func_00555900(this, cmd == 17 ? 1 : 2, 0)) {
            CAiState *s = GetAiState();

            s = s && s->m_state == AI_STATE_HOSTAGE ? s : 0;
            if (s)
                ((CAiSHostage *)s)->SetCaptor(other, false);
        } else if (!PopToAiState(AI_STATE_SURRENDER, false)) {
            if (SetAiState(AI_STATE_HOLD, 0)) {
                CAiState *s = GetAiState();
                CAiSHold *h = (CAiSHold *)(s && s->m_state == AI_STATE_HOLD ? s : 0);

                if (cmd == 14) {
                    CPnt3D center;

                    h->SetFlags(0x20, 0);
                    if (OrderCenter(unit, other, &center)) {
                        h->m_pos = center;
                        h->m_cell = other->m_mapLocValid;
                        h->SetFlags(0x30, 0);
                    }
                }
            }
            if (cmd == 17)
                SetStanceDelayed((SEAL_STANCE)2, -1.0f);
        }
        if (seal->m_team & 0x20000) {
            PlayAiSound((CHARACTER_SOUND)55, 1.0f, CAiEvent::AIE_NONE);
            CSealUnit::GetUnitByTeam(17)->SealJoinUnit(seal);
        } else if (seal->m_team & 0x40000) {
            PlayAiSound((CHARACTER_SOUND)55, 1.0f, CAiEvent::AIE_NONE);
            CSealUnit::GetUnitByTeam(18)->SealJoinUnit(seal);
        } else if (cmd != 17)
            SetStanceDelayed((SEAL_STANCE)1, -1.0f);
        break;
    case 19:
        if (IsNavySeal()) {
            if (!AiStateIs(AI_STATE_HOLD))
                OnCommand(other, (FT_COMMAND)5, 0, false);
            CAiState *s = GetAiState();
            CAiSHold *h = (CAiSHold *)(s && s->m_state == AI_STATE_HOLD ? s : 0);

            if (h) {
                SetStanceDelayed((SEAL_STANCE)1, -1.0f);
                h->m_cell = other->m_mapLocValid;
                h->SetFlags(0x80, 0);
                h->m_pos = other->m_1458;
                h->SetFlags(0x10, 0);
                static CPnt3D side(10.0f, 0.0f, 7.0f);
                bool found = false;
                CMatrix *m = (CMatrix *)other->m_node;
                CPnt3D pos;
                CAiMapLoc loc;

                m->Transform(&side, &pos, 1);
                if (AiMaps.GetLocation(&pos, loc) && Traversable(loc))
                    found = true;
                else {
                    side.x = -side.x;
                    m->Transform(&side, &pos, 1);
                    if (AiMaps.GetLocation(&pos, loc) && Traversable(loc))
                        found = true;
                }
                if (found)
                    func_005F0830(this, &loc, 5, -1, -1);
                const CEntityList &list = CEntity::m_list;

                for (CEntityList::const_iterator i = list.begin(); i != list.end(); ++i)
                    (*i)->m_UnitsSeenBy &= ~(other->m_team | seal->m_team);
            }
        }
        break;
    case 4:
        if (other) {
            bool following = AiStateIs(AI_STATE_FOLLOW);
            CSealUnit *ounit = ((CSealCtrl *)other->m_control)->m_unit;
            bool esc = Escort(seal);
            bool join = esc;

            if (esc) {
                bool j;

                if (func_005750A0())
                    j = unit != ounit;
                else
                    j = unit->m_unit_num == 17 || unit->m_unit_num == 18;
                join = j;
            }
            if (join) {
                following = false;
                if (ounit) {
                    ounit->EscorteeJoinUnit(seal);
                    unit = m_unit;
                    PlayAiSound((CHARACTER_SOUND)55, 1.0f, CAiEvent::AIE_NONE);
                    if (func_005750A0())
                        PlayRemoteMsg(other, 43, 0);
                }
            }
            ClearAiState(false);
            if (AiStateIs(AI_STATE_FOLLOW) || PushAiState(AI_STATE_FOLLOW, 0)) {
                CAiState *s = GetAiState();
                CAiSFollow *f = (CAiSFollow *)(s && s->m_state == AI_STATE_FOLLOW ? s : 0);

                func_005FD010(f, this, other);
                f->SetFormation(unit->m_8, 0);
                bool leader = unit->m_seals.front() == seal;

                f->SetBit2(following && cmd != 7 || b || !leader);
                CAiState::Signal(&m_stateStack, (CAiState::SIGNAL)7, this);
            }
        }
        break;
    case 23: {
        CAiMapLoc loc;

        if (func_0058F230(unit, other, (void *)2, &loc)) {
            ClearAiState(false);
            if (PushAiState(AI_STATE_HOLD, 0)) {
                CAiState *h = GetAiState();

                h->SetBit2(true);
                h->SetActive(true);
                if (PushAiState(AI_STATE_GOTO, 0)) {
                    CAiState *s = GetAiState();
                    CAiSGoto *g = (CAiSGoto *)(s && s->m_state == AI_STATE_GOTO ? s : 0);

                    g->m_cell = loc;
                    g->SetFlags(0x100, 0);
                    g->SetActive(true);
                }
            }
        }
        break;
    }
    case 9:
    case 10:
    case 11:
    case 12:
    case 13: {
        CAiMapLoc loc;

        if (func_0058F230(unit, other, data, &loc)) {
            ClearAiState(false);
            if (PushAiState(AI_STATE_HOLD, 0)) {
                CAiState *h = GetAiState();

                h->SetBit2(true);
                h->SetActive(true);
                if (PushAiState(AI_STATE_GOTO, 0)) {
                    CAiState *s = GetAiState();
                    CAiSGoto *g = (CAiSGoto *)(s && s->m_state == AI_STATE_GOTO ? s : 0);

                    g->m_cell = loc;
                    g->SetActive(true);
                    switch (cmd) {
                    case 10:
                        g->SetFlags(0x402, 0);
                        break;
                    case 11:
                        g->SetFlags(0x401, 0);
                        h->SetFlags(0x200, 0);
                        break;
                    case 9:
                        g->SetFlags(0x404, 0);
                        break;
                    case 13:
                        g->SetFlags(8, 0);
                        break;
                    case 12:
                        g->SetFlags(0x410, 0);
                        break;
                    }
                }
            }
        }
        break;
    }
    case 1: {
        CNodeAction *na = func_00591380(seal, other, (unsigned int)data);

        if (na && other && PushAiState(AI_STATE_ACTION, 0)) {
            CAiState *s = GetAiState();

            func_005F0180((CAiSAction *)(s && s->m_state == AI_STATE_ACTION ? s : 0), this, na, -1);
        }
        break;
    }
    case 16: {
        void *p = unit->GetDataPtr();

        if (p)
            func_005EBB50(p, this);
        break;
    }
    case 27:
    case 28:
        SetSleeping(cmd == 28);
        break;
    }
}
