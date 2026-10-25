/* veh_vehicle.cpp -- CVehicle (new in SOCOM II: CEntity's type 4), the turret unit's twin: its
 * definitions read from an rdr, nodes whose names they prefix hooked up, a vehicle made for each.
 * OVL1 0x3ACA60-0x3AFB2F, after veh_turret.cpp and before zvid_init.cpp; its static initialiser
 * (0x4043A0) builds the definitions (0x4A4138) and sets five bits at 0x4A4148, as the turret's
 * does. The file name is ours. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zveh.h"

using namespace zdb;

extern "C" {
/* not yet written here (placeholders): the vehicle's half of the tick, its definitions' parse
 * and resize, and the AI maps' cell lookup, map and region fill (OVL2) */
void func_003ACC10(CVehicle *vehicle, float dT);
void func_003AD3B0(CVehicle *vehicle);
void func_003AE530(CVehicleDefVector *v, unsigned int n);
int func_0052DC70(CAiMaps *maps, const CPnt3D *pos, unsigned int *loc);
void *func_0052E6B0(CAiMaps *maps, unsigned int mapid);
void func_00515880(void *map, CPnt3D *corners, void *region);
/* the vehicles made, and the most there may be (-1: no limit) -- the unit's statics */
extern int data_004A4120;
extern int data_003E0860;
}

CVehicleDefVector CVehicle::m_vehicle_def;

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

int CVehicle::Init(const char *name, const char *path)
{
    return m_vehicle_def.Init(name, path);
}

void CVehicle::LoadVehicles()
{
    CWorld *world = CWorld::m_world;
    CNodeVector &landmarks = world->m_landmarks;

    for (CNodeVector::iterator it = landmarks.begin(); it != landmarks.end(); ++it)
        ScanForVehicleDefinitionHookups(*it, true);
    ScanForVehicleDefinitionHookups(world, true);
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
    func_00515880(map, m_corners, m_region);
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
