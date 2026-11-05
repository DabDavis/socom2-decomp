/* seal_orders.cpp -- the orders' names and tables, and the cells an order's target names: the
 * radio menu's addressees, the HUD's order and item names, the orders' and the multiplayer
 * messages' names and their lookups, an order's target cell -- the crosshairs' hit, a point ahead or
 * a nav point -- and the cell searches behind it (new in SOCOM II, the file name ours; SOCOM 1's
 * GetCommandName and GetCommandId sat in seal_command.cpp between AllowCommand and
 * AllowCommandHOLDPOS, where SOCOM II has three new functions). OVL2 0x58DBB0-0x58FBDF: a unit of
 * its own -- seal_cfg.cpp's template tail (0x58CE70-0x58DBAF) closes the unit before it, and
 * seal_command.cpp starts at 0x58FBE0 with OnCommand, as SOCOM 1's does; the strings follow
 * seal_cfg.cpp's tail's (0x65D280-0x65E525, seal_command.cpp's from 0x65E528). Defined in reverse of
 * their image order (-inline deferred). */

#include "zseal.h"


/* a cell search's answer: the line walks' callbacks below take the walk (ai.h's MAP_LINEWALK_STRUCT)
 * and this as their data (0x58F230 and 0x58F6E0 pass them; the names ours) */
struct CCellAnswer {
    /* none yet (0x58F6E0 builds one so: the invalid cell copied field by field) */
    CCellAnswer() : m_found(false), m_loc(CAiMapLoc::invalid) {}

    bool m_found;   // 0x0
    CAiMapLoc m_loc;   // 0x4
};

/* whether a cell can be taken: on the maps, traversable, and under no overlay when occupied -- the
 * cell looked up when none is given (OVL2 0x58FA70, new in SOCOM II: the cell searches' steps
 * 0x58F8A0 and 0x58F960 ask it; the name unknown) */
extern "C" bool func_0058FA70(const CAiMapLoc &loc, const CAiCell *cell)
{
    bool r = false;

    if (!cell)
        cell = AiMaps.GetCell(loc);
    if (cell && cell->IsTraversable())
        r = !ai::overlays.Contains(cell, loc);
    return r;
}

extern "C" {
/* how far above the search's height a marked cell may lie (OVL2 data 0x6505A0, read as a variable) */
extern float data_006505A0;
}

/* the cell's bit 21 as a bool (an inline: the target materialises it before its branch; the name
 * ours) */
static inline bool IsMarked(const CAiCell *cell)
{
    return cell->m_mes ? true : false;
}

/* a cell search's step that keeps to a height: a marked cell lying too far above the search's height
 * is passed over; a cell taken ends it, found (OVL2 0x58F960, new in SOCOM II: a callback, as
 * 0x58F8A0; the altitude CAiMap::GetAltitude's body, 0x516940, written out) */
extern "C" bool func_0058F960(const MAP_LINEWALK_STRUCT &q, CCellAnswer *a)
{
    bool skip = false;
    const CAiCell *cell = q.m_cell;

    if (IsMarked(cell)) {
        const CAiMap *map = q.m_map;
        float h;
        bool ok;

        if (cell) {
            h = map->m_bbox.m_min.y + map->m_alt_quant * (float)cell->m_altitude;
            ok = true;
        } else
            ok = false;
        if (ok && !(h - q.m_pos.y <= data_006505A0))
            skip = true;
    }
    if (!skip && func_0058FA70(q.m_loc, q.m_cell)) {
        a->m_found = true;
        a->m_loc = q.m_loc;
        return false;
    }
    return true;
}

/* whether a cell is on a map: either coordinate off the invalid cell's (an inline bool: the target
 * materialises the `||`) */
static inline bool IsMapped(const CAiMapLoc &loc)
{
    return loc.x != CAiMapLoc::invalid.x || loc.y != CAiMapLoc::invalid.y;
}

/* a cell search's step: a cell taken ends it, found; otherwise the answer is whether the cell it
 * holds is on a map, and the search goes on (OVL2 0x58F8A0, new in SOCOM II: a callback 0x58F230
 * passes by address) */
extern "C" bool func_0058F8A0(const MAP_LINEWALK_STRUCT &q, CCellAnswer *a)
{
    bool done = true;

    if (!func_0058FA70(q.m_loc, q.m_cell)) {
        a->m_found = IsMapped(a->m_loc);
        done = false;
    } else {
        a->m_loc = q.m_loc;
        a->m_found = true;
    }
    return done;
}

/* a line walk's callback (ai.h's type: the steps above take their answer typed) */
typedef bool (*CELL_WALK)(const MAP_LINEWALK_STRUCT &s, void *data);

/* a cell that can be taken near a point: the point's own, else the first along a line -- from that
 * cell toward the second point on its own map, or between the two points over the maps -- passing
 * marked cells too far above the line; whether one was found, and it (OVL2 0x58F6E0, new in SOCOM II;
 * the name unknown) */
extern "C" bool func_0058F6E0(const CPnt3D *from, const CPnt3D *to, CAiMapLoc *out, bool along)
{
    CCellAnswer ans;
    CAiMapLoc loc;

    if (AiMaps.GetLocation(from, loc)) {
        if (func_0058FA70(loc, 0)) {
            ans.m_found = true;
            ans.m_loc = loc;
        } else if (along)
            AiMaps.GetMapByIndex(loc.m_mapid)->WalkLine(loc, to, (CELL_WALK)func_0058F960, &ans, 0.2f);
        else
            AiMaps.WalkLine(from, to, (CELL_WALK)func_0058F960, &ans, 0.2f);
    }
    if (ans.m_found)
        *out = ans.m_loc;
    return ans.m_found;
}

extern "C" {
/* the hit under the SEAL's reticule (OVL2 0x5AA6D0, seal_env.cpp's span: `this + 0x3C0`, its
 * m_last_reticule -- SOCOM 1's GetReticuleDi by its body, unproven; its draft matches) */
zdb::IntersectStruct *func_005AA6D0(CZSealBody *seal);
}

/* the cell an order's target names (OVL2 0x58F230, new in SOCOM II; CSealCtrlAi's OnCommand and
 * AllowCommand and CSealUnit::IssueCommand ask it; the name unknown): the squad's order data (1);
 * for the crosshairs (0, 2, 21) the reticule's hit when it hit a node -- its own cell when it can be
 * taken, else the first cell taken along the line from the SEAL -- or with no hit the first cell
 * taken along the line to the point the SEAL's visual range ahead; a nav point by name (22-31). The
 * cases listed in reverse of the target's compare chain. An int, as the target's andi after every
 * bool call it returns says (a bool return passes them through); the hit's cell tested `!= false`
 * (the andi before the branch); the cell and the point ahead declared first, ahead of the two
 * answers, give the target's frame (the answers at 0x40/0x48, the point 0x50, the cell 0x5C), and
 * the point assigned from the three-float constructor its stores' order (x, z, y) */
extern "C" int func_0058F230(CSealUnit *unit, CZSealBody *seal, unsigned char target, CAiMapLoc *out)
{
    switch (target) {
    case 1:
        return unit->GetData(out, 4);
    case 0:
    case 2:
    case 21: {
        CAiMapLoc loc;
        CPnt3D ahead;
        zdb::IntersectStruct *hit = seal ? func_005AA6D0(seal) : 0;

        if (hit && hit->m_node) {
            const CPnt3D *at = &func_005AA6D0(seal)->m_pos;

            if (AiMaps.GetLocation(at, *out) != false)
                return func_0058F6E0(at, &seal->m_posW, out, false);
            CCellAnswer ans;

            AiMaps.WalkLine(&seal->m_posW, at, (CELL_WALK)func_0058F8A0, &ans, 0.2f);
            if (ans.m_found)
                *out = ans.m_loc;
            return ans.m_found;
        }
        ahead = CPnt3D(0.0f, 0.0f, -((CSealCtrl *)seal->m_control)->m_aiParams.visual_range);

        seal->m_node->m_matrix.Transform(&ahead, 1);
        CCellAnswer ans;

        if (AiMaps.GetLocation(&seal->m_posW, loc))
            AiMaps.GetMapByIndex(loc.m_mapid)->WalkLine(loc, &ahead, (CELL_WALK)func_0058F8A0, &ans, 0.2f);
        if (ans.m_found)
            *out = ans.m_loc;
        return ans.m_found;
    }
    case 22:
        return AiMaps.GetLocation("charlie", *out);
    case 23:
        return AiMaps.GetLocation("delta", *out);
    case 24:
        return AiMaps.GetLocation("echo", *out);
    case 25:
        return AiMaps.GetLocation("foxtrot", *out);
    case 26:
        return AiMaps.GetLocation("juliet", *out);
    case 27:
        return AiMaps.GetLocation("romeo", *out);
    case 28:
        return AiMaps.GetLocation("sierra", *out);
    case 29:
        return AiMaps.GetLocation("whiskey", *out);
    case 30:
        return AiMaps.GetLocation("xray", *out);
    case 31:
        return AiMaps.GetLocation("zulu", *out);
    }
    return false;
}

extern "C" {
/* the other names, their drafts matching (OVL2 0x58E120, 0x58E360, 0x58E4D0, 0x58E750, 0x58EB00,
 * 0x58F190: string switches and an order's table) */
const char *func_0058E120(unsigned char v);
const char *func_0058E360(unsigned char v);
const char *func_0058E4D0(unsigned char v);
const char *func_0058E750(unsigned char v);
const char *func_0058EB00(unsigned char v);
int func_0058F190(unsigned char cmd);
}


extern "C" {
int strcasecmp(const char *a, const char *b);
/* two tables of 72 names each, a byte value's name (OVL2 0x58ED60 and 0x58EF80, string switches) */
const char *func_0058ED60(unsigned char v);
const char *func_0058EF80(unsigned char v);
}

/* an order's name, CMD_UNKNOWN for an order without one (OVL2 0x58EF80, new in SOCOM II; the second
 * table 0x58E0A0 searches -- SOCOM II's own FT_COMMAND values, not SOCOM 1's: ESCORT 8, AMBUSH 30, 24
 * and 29 unnamed). The cases in the names' order, each a block of its own (the image's), case 0's
 * apart from the default's */
extern "C" const char *func_0058EF80(unsigned char cmd)
{
    switch (cmd) {
    case 0: return "CMD_UNKNOWN";
    case 1: return "CMD_ACTION";
    case 30: return "CMD_AMBUSH";
    case 12: return "CMD_ATTACKTO";
    case 27: return "CMD_AWAKE";
    case 16: return "CMD_BREACH";
    case 23: return "CMD_CLEARAREA";
    case 14: return "CMD_COVERTARGET";
    case 2: return "CMD_DEPLOY";
    case 8: return "CMD_ESCORT";
    case 22: return "CMD_EXTRACT";
    case 3: return "CMD_FIREATWILL";
    case 4: return "CMD_FOLLOW";
    case 17: return "CMD_GETDOWN";
    case 6: return "CMD_HOLDFIRE";
    case 20: return "CMD_HOLDFIRE_OVERRIDE";
    case 5: return "CMD_HOLDPOS";
    case 15: return "CMD_LASETARGET";
    case 13: return "CMD_LEADTO";
    case 10: return "CMD_MOVETO";
    case 19: return "CMD_OVERWATCH";
    case 18: return "CMD_PULLOUT";
    case 7: return "CMD_REGROUP";
    case 26: return "CMD_REMOVEKILLS";
    case 25: return "CMD_RESTRAIN";
    case 9: return "CMD_RUNTO";
    case 28: return "CMD_SLEEP";
    case 11: return "CMD_STEALTHTO";
    case 21: return "CMD_SUPPORT";
    }
    return "CMD_UNKNOWN";
}

/* an order's name for the HUD (OVL2 0x58E950, a switch over 24 orders; its draft matches) */
extern "C" const char *func_0058E950(unsigned char cmd);

/* an order's name, regroup's spelled apart (OVL2 0x58E900; new in SOCOM II, its name unknown --
 * OVL1's HUD calls it) */
extern "C" const char *func_0058E900(unsigned char cmd)
{
    switch (cmd) {
    case 7:
        return "RE GROUP";
    default:
        return func_0058E950(cmd);
    }
}

extern "C" unsigned char func_0058E0A0(const char *name)
{
    if (name)
        for (int i = 0; i < 72; i++) {
            unsigned char v = i;

            if (!strcasecmp(func_0058EF80(v), name))
                return v;
        }
    return 0;
}

/* a name's value in the first and the second table, 0 for none (OVL2 0x58E020, 0x58E0A0, new in
 * SOCOM II; CCharacterType::InitMultiplayerMessages reads the multiplayer messages through them). The
 * byte value a local declared after the counter: it takes s0, the counter s1 */
extern "C" unsigned char func_0058E020(const char *name)
{
    if (name)
        for (int i = 0; i < 72; i++) {
            unsigned char v = i;

            if (!strcasecmp(func_0058ED60(v), name))
                return v;
        }
    return 0;
}

/* a slot for an item kind: 3 for kind 2, 17 for kind 16, 21 for kinds 9-13, none otherwise -- asked
 * only as a bool (OVL2 0x58DFA0, new in SOCOM II; OVL1's 0x22FF30 and 0x230050 and the kit lane's
 * 0x5EAD70 call it). The shared cases are listed in reverse of the target's test order */
enum ITEM_SLOT { ITEM_SLOT_NONE = 0, ITEM_SLOT_3 = 3, ITEM_SLOT_17 = 17, ITEM_SLOT_21 = 21 };

extern "C" bool func_0058DFA0(unsigned char item)
{
    ITEM_SLOT slot = ITEM_SLOT_NONE;

    switch (item) {
    case 2:
        slot = ITEM_SLOT_3;
        break;
    case 16:
        slot = ITEM_SLOT_17;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        slot = ITEM_SLOT_21;
        break;
    }
    return slot != ITEM_SLOT_NONE;
}

/* two lookups over one byte kind (OVL2 0x58DEB0, 0x58DF20, new in SOCOM II: the HUD's radio menu
 * at 0x2329F0/0x232BC0 asks both in turn; the names ours): 2, 16 and the group 9-13 each give their
 * own value, any other kind 0. Each case's compare runs in reverse source order */
extern "C" int func_0058DF20(unsigned char k)
{
    int r = 0;

    switch (k) {
    case 2:
        r = 3;
        break;
    case 16:
        r = 17;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        r = 21;
        break;
    }
    return r;
}

extern "C" int func_0058DEB0(unsigned char k)
{
    int r = 0;

    switch (k) {
    case 2:
        r = 16;
        break;
    case 16:
        r = 20;
        break;
    case 10:
    case 11:
    case 9:
    case 13:
    case 12:
        r = 32;
        break;
    }
    return r;
}

/* a radio order's group named (OVL2 0x58DD70, new in SOCOM II; the name unknown): 0x58DBB0 below
 * inlines it. The cases listed in reverse of the target's compare chain */
extern "C" const char *func_0058DD70(unsigned char who)
{
    switch (who) {
    case 1:
        return "FIRETEAM";
    case 2:
        return "ABLE";
    case 3:
        return "BRAVO";
    case 17:
        return "ESCORTEE";
    case 18:
        return "FRIENDLY";
    case 26:
        return "NEUTRAL";
    case 27:
        return "TURRETS";
    case 29:
        return "SUPPORT";
    case 28:
        return "HOSTAGE";
    case 30:
        return "ENEMIES";
    case 31:
        return "ALLIES";
    default:
        return "FT_UNKNOWN";
    }
}

/* a radio order's addressee for the HUD: the team, its elements, an escortee and the air support
 * described, any other group by its name (OVL2 0x58DBB0, new in SOCOM II: OVL1's radio menu builder
 * 0x232BC0 asks it; the name unknown) -- the names' switch inlined, its compares reusing this one's
 * constants */
extern "C" const char *func_0058DBB0(unsigned char who)
{
    switch (who) {
    case 1:
        return "Issue an order to the entire team . . .";
    case 2:
        return "Issue an order to team Able . . .";
    case 3:
        return "Issue an order to team Bravo . . .";
    case 17:
        return "Issue an order to an Escortee . . .";
    case 29:
        return "Call in for air support";
    }
    return func_0058DD70(who);
}
