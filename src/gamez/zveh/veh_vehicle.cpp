/* veh_vehicle.cpp -- CVehicle (new in SOCOM II: CEntity's type 4), the turret unit's twin: its
 * definitions read from an rdr, nodes whose names they prefix hooked up, a vehicle made for each.
 * OVL1 0x3ACA60-0x3AFB2F, after veh_turret.cpp and before zvid_init.cpp; its static initialiser
 * (0x4043A0) builds the definitions (0x4A4138) and sets five bits at 0x4A4148, as the turret's
 * does. The file name is ours. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zveh.h"

using namespace zdb;

extern "C" {
/* not yet written here (placeholders): the vehicle's half of the tick and the AI maps' cell
 * lookup, map and region fill (OVL2) */
void func_003ACC10(CVehicle *vehicle, float dT);
void func_003AD3B0(CVehicle *vehicle);
void func_003ADAB0(CVehicle *vehicle, CNode *node, const CVehicleDef *def);
int func_0052DC70(CAiMaps *maps, const CPnt3D *pos, unsigned int *loc);
void *func_0052E6B0(CAiMaps *maps, unsigned int mapid);
void func_00515880(void *map, CPnt3D *corners, void *region);
/* the vehicles made, and the most there may be (-1: no limit) -- the unit's statics */
extern int data_004A4120;
extern int data_003E0860;
}

/* the unit's debug switches (0x4A4148: five 1-bit flags; the unit's twin of SOCOM 1's file-static
 * turret_debug_mask, the names ours). Set in the mem-initializer list, each is a word read-mask-write
 * with `true` folded (`daddiu 1<<n`) and `false` left `(0 & 1) << n`; assigned in the body MW narrows
 * the accesses to bytes, and inline bool setters do too */
class CVehicleDebugMask {
public:
    CVehicleDebugMask() : m_0(true), m_1(true), m_2(true), m_3(false), m_4(false) {}

    unsigned int m_0 : 1;
    unsigned int m_1 : 1;
    unsigned int m_2 : 1;
    unsigned int m_3 : 1;
    unsigned int m_4 : 1;
};

/* the initialiser (__sinit_veh_vehicle.cpp, 0x4043A0) builds these in order: the definitions (the
 * vector's words zeroed in place, its destructor registered), then the debug switches */
CVehicleDefVector CVehicle::m_vehicle_def;
static CVehicleDebugMask vehicle_debug_mask;

/* the definition whose name starts the node's (called, never inlined) */
#pragma auto_inline off
const CVehicleDef *CVehicleDefVector::PrefixMatch(const char *name)
{
    if (name) {
        unsigned int i;
        unsigned int n = size();

        if (strlen(name)) {
            for (i = 0; i < n; i++) {
                CVehicleDef *def = &(*this)[i];
                unsigned int len = def ? strlen(def->m_name) : 0;

                if (def && len && !strncmp(name, def->m_name, len))
                    return def;
            }
        }
    }
    return 0;
}
#pragma auto_inline reset

/* the definitions read: the vector sized to the "VEHICLE" list's pairs, each parsed from its name
 * and value (CTurretDefVector::Init's twin). The resize is include/vector's, over CVehicleDef's
 * inline constructor: it and its erase (0x3AE750, 0x3AE800) are byte-exact in this unit's template
 * tail. OPEN, its insert (0x3AF040), the target's size now (the dead-stripped stand-in at the end of
 * the unit makes the tail inline the temporary's destruction, as the target's does) but 121
 * instructions renamed: include/vector's open class insert -- the in-place branch's end pointer
 * shares data_'s register (a0) in the target where MW gives it a fresh one (a3). The tail's deleter
 * destructor and clear at 0x3AFAA0/0x3AFB10 are NOT CVehicleDef's, though their code is the same:
 * ~CVehicle (0x3ACA60, not written) calls 0x3AFAA0 for its region's vector at 0x18C. */
int CVehicleDefVector::Init(const char *name, const char *path)
{
    _zrdr *rdr = zrdr_read(name, path, 0);

    if (rdr) {
        _zrdr *list = zrdr_findtag(rdr, "VEHICLE");
        unsigned int i;
        unsigned int n = (list ? list->Count() : 0) / 2;

        i = 0;
        resize(n);
        for (iterator it = begin(); it != end(); ++it, i += 2)
            (*it).Parse(list->Item(i)->String(), list->Item(i + 1));
    }
    zrdr_free((CRdrFile *)rdr);
    return size();
}

int CVehicle::Init(const char *name, const char *path)
{
    return m_vehicle_def.Init(name, path);
}

CEntity *CVehicle::Spawn(CVehicleRdrEntry *entry)
{
    const char *type = entry->m_character;
    const CVehicleDef *def = m_vehicle_def.Find(type);

    if (def) {
        CWorld *world = CWorld::m_world;
        CNodeVector &landmarks = world->m_landmarks;

        for (CNodeVector::iterator it = landmarks.begin(); it != landmarks.end(); ++it)
            ScanForVehicleByName(entry->m_name, def, *it, true);
        ScanForVehicleByName(entry->m_name, def, world, true);
    }
    return 0;
}

void CVehicle::LoadVehicles()
{
    CWorld *world = CWorld::m_world;
    CNodeVector &landmarks = world->m_landmarks;

    for (CNodeVector::iterator it = landmarks.begin(); it != landmarks.end(); ++it)
        ScanForVehicleDefinitionHookups(*it, true);
    ScanForVehicleDefinitionHookups(world, true);
}

/* a vehicle for a node: CEntity's type 4, the region and matrices built, the definition kept, the
 * node's parts hooked up (0x3ADAB0) and its ancestor under the world found */
CVehicle::CVehicle(CNode *node, const CVehicleDef &def) : CEntity((CEntity::TYPE)4, node), m_def(&def)
{
    m_flags = 0;
    m_1cc = 0;
    m_1d0 = node;
    m_root = 0;
    m_1d4 = 0;
    m_1e0.SetIdentity();
    m_220.SetIdentity();
    func_003ADAB0(this, node, &def);

    CNode *p = node ? node->GetParent() : 0;

    if (p) {
        do {
            if (p == CWorld::m_world)
                break;
            node = p;
            p = p->GetParent();
        } while (p);
    }
    m_root = node;
    m_flags |= 3;
}

int CVehicle::ScanForVehicleDefinitionHookups(CNode *node, bool recurse)
{
    int n = 0;

    if (!node || m_vehicle_def.size() == 0)
        return 0;
    if (node->GetType() == 10)
        return 0;
    if (!node->m_name)
        return 0;
    const CVehicleDef *def = m_vehicle_def.PrefixMatch(node->m_name);

    if (def) {
        if (node) {
            if (!(data_003E0860 > -1 && data_004A4120 >= data_003E0860)) {
                data_004A4120++;
                CEntity::CreateEntity(new CVehicle(node, *def), true);
            }
        }
        n++;
    } else if (recurse) {
        CNodeVector &children = node->GetChildren();

        for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
            n += ScanForVehicleDefinitionHookups(*it, recurse);
    }
    return n;
}

int CVehicle::ScanForVehicleByName(const char *name, const CVehicleDef *def, CNode *node,
                                   bool recurse)
{
    int n = 0;

    if (!def || !node)
        return 0;
    if (node->GetType() == 10)
        return 0;
    if (!node->m_name)
        return 0;
    if (!strcmp(node->m_name, name)) {
        if (node) {
            if (!(data_003E0860 > -1 && data_004A4120 >= data_003E0860)) {
                data_004A4120++;
                CEntity::CreateEntity(new CVehicle(node, *def), true);
            }
        }
        n++;
    } else if (recurse) {
        CNodeVector &children = node->GetChildren();

        for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it)
            n += ScanForVehicleByName(name, def, *it, recurse);
    }
    return n;
}

void CVehicle::PreTick(float dT)
{
    CNode *root = m_root;

    if (root)
        SetFlag(1, root->m_active);
    else
        SetFlag(1, false);
    if (GetFlag(1)) {
        if (m_1d4 && !m_1d4->m_active) {
            SetFlag(1, false);
            m_isAlive = false;
        }
        func_003AD3B0(this);
    }
}

/* OPEN, 8 bytes long, 25 differing: all but the matrix's set-up -- the target computes &m (sp+64)
 * early among the corners' loads and fills sceVu0UnitMatrix's delay slot with the last corner's
 * store, then reloads m_root after the call; here the &m move takes the delay slot. Tried: the
 * matrix declared first (0x180), a const factor. */
void CVehicle::Tick(float dT)
{
    unsigned int loc;
    void *map;

    if (!GetFlag(1))
        return;
    ComputeNextPosition(dT);
    if (!GetFlag(4))
        return;
    func_003ACC10(this, dT);
    if (!GetNode())
        return;
    if (!func_0052DC70(&AiMaps, &m_posW, &loc))
        return;
    map = func_0052E6B0(&AiMaps, ((CAiMapLoc *)&loc)->m_mapid);
    if (!map)
        return;
    float d = 5.0f;

    m_corners[0].x = m_bounds[0].x + d;
    m_corners[0].y = m_bounds[0].y;
    m_corners[0].z = m_bounds[0].z + d;
    m_corners[1].x = m_bounds[1].x + d;
    m_corners[1].y = m_bounds[1].y;
    m_corners[1].z = m_bounds[1].z - d;
    m_corners[2].x = m_bounds[2].x - d;
    m_corners[2].y = m_bounds[2].y;
    m_corners[2].z = m_bounds[2].z - d;
    m_corners[3].x = m_bounds[3].x - d;
    m_corners[3].y = m_bounds[3].y;
    m_corners[3].z = m_bounds[3].z + d;
    CMatrix m;

    m.SetIdentity();
    m_root->BuildMTW(m);
    m.Transform(m_corners, 4);
    func_00515880(map, m_corners, &m_region);
}

void CVehicle::PrePostTick(float dT)
{
    CEntity::UpdateCompassAnchor();
}

void CVehicle::UpdatePosW()
{
    m_posW = *(CPnt3D *)m_1e0.m_matrix[3];
}

void CVehicle::ComputeNextPosition(float dT)
{
}

void CVehicle::OnMissionStart()
{
    CreateCompassAnchor(11, "Vehicle");
}

/* the neck: the position raised by the node's height (30 when its box is upside down) */
CPnt3D *CVehicle::GetNeckPosW(CPnt3D *out)
{
    *out = m_posW;
    if (m_node) {
        const CBBox *box = m_node->GetBBox();
        float h = box->m_max.y - box->m_min.y;

        if (h < 0.0f)
            h = 30.0f;
        CPnt3D up(0.0f, h, 0.0f);

        out->Add(&up, out);
    }
    return out;
}

void CVehicle::UpdateAiMapLoc()
{
    unsigned int loc;

    if (func_0052DC70(&AiMaps, &m_posW, &loc)) {
        *(unsigned int *)&m_mapLoc = loc;
        *(unsigned int *)&m_mapLocValid = loc;
    }
}

/* the table stored, the region's cells destroyed (the deleter's destructor called -- 0x3AFAA0, this
 * unit's copy of OVL2's -- ~vector and ~__vector_imp inlined), then CEntity's.
 * OPEN, 8 bytes short: after the two tests of the cells' address the target has a third, a `b` to
 * the call (the delay slot's a1 = 0 under it) -- what one more inlined destructor level gives: with
 * the cells a vector subclass (CAiMapLocVec's shape), as a member or as a second base of the region,
 * this matches byte for byte, but the region's constructor is then six high and CVehicle's
 * constructor (0x3ADE90, byte-exact as it stands) inlines it one level only. A user-declared
 * destructor on the region and the cells as a second base change nothing */
CVehicle::~CVehicle()
{
}

/* STAND-IN for a dead-stripped original (no row in the map: it scores as no-target). The insert
 * (0x3AF040) inlines its temporary's destruction -- the deleter's destructor, with clear inside --
 * which the template tail does only for members generated before it, i.e. requested by the unit's
 * own code; nothing left in the image clears or destroys a vector of definitions, which fits an own
 * function the linker stripped (the integrator's measurement) */
void DeadVehicleDefinitionsKill()
{
    CVehicle::m_vehicle_def.clear();
    std::vector<CVehicleDef> tmp;
}
