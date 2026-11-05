/* veh_vehicle.cpp -- CVehicle (new in SOCOM II: CEntity's type 4), the turret unit's twin: its
 * definitions read from an rdr, nodes whose names they prefix hooked up, a vehicle made for each.
 * OVL1 0x3ACA60-0x3AFB2F, after veh_turret.cpp and before zvid_init.cpp; its static initialiser
 * (0x4043A0) builds the definitions (0x4A4138) and sets five bits at 0x4A4148, as the turret's
 * does. The file name is ours. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zveh.h"

using namespace zdb;

extern "C" {
/* not yet written here (placeholders): the vehicle's half of the tick */
void func_003ACC10(CVehicle *vehicle, float dT);
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

unsigned int ParseTeams(_zrdr *rdr);

/* a string field set, cut to its 31 characters, or emptied (inline: the length is taken and not
 * used -- an assert compiled out?; the name ours) */
static inline void SetDefString(char *dst, const char *src)
{
    if (!src) {
        dst[0] = 0;
    } else {
        (void)strlen(src);
        strncpy(dst, src, 31);
        dst[31] = 0;
    }
}

/* a definition read: its name, the ACTIVATE and HEALTH_ROOT_NAME nodes, the COLLISION_BBOX node
 * with its RECURSIVE tag, and the teams it serves (the name SOCOM 1's turret definition's) */
void CVehicleDef::Parse(const char *name, _zrdr *rdr)
{
    SetDefString(m_name, name);

    _zrdr *t = zrdr_findtag(rdr, "ACTIVATE");

    if (t) {
        _zrdr *item = t->Item(0);

        if (item)
            SetDefString(m_20, item->String());
    }
    t = zrdr_findtag(rdr, "HEALTH_ROOT_NAME");
    if (t) {
        _zrdr *item = t->Item(0);

        if (item)
            SetDefString(m_40, item->String());
    }

    _zrdr *bbox;

    if ((bbox = zrdr_findtag(rdr, "COLLISION_BBOX")) != 0) {
        _zrdr *item = bbox->Item(0);

        if (item)
            SetDefString(m_60, item->String());
        if (zrdr_findtag(bbox, "RECURSIVE"))
            m_80 = 1;
        else
            m_80 = 0;
    }
    if ((t = zrdr_findtag(rdr, "TEAM")) != 0 && t->type == ZRDR_ARRAY)
        m_84 = ParseTeams(t);
}

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
 * tail, and so is its insert (0x3AF040: include/vector's class insert since the MSL lane's work on
 * it -- the dead-stripped stand-in at the end of the unit makes the tail inline the temporary's
 * destruction, as the target's does). The tail's deleter
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
    m_healthy = 0;
    m_1d0 = node;
    m_root = 0;
    m_1d4 = 0;
    m_1e0.SetIdentity();
    m_220.SetIdentity();
    Init(node, def);

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

/* the AI's overlays (ai.h's: the vehicle's region added to the list) */
class CAiOverlay;
namespace ai {
void AddOverlay(CAiOverlay *overlay);
}

/* the eight corners of a box, round the bottom from the minimum's x and y at the far z, then the
 * top (fts_mission.cpp's BoxCorners, the states' round 39): too big for inline_max_size, inlined
 * one level where it is used, the corners' CPnt3D(fff) and assignments called (0x2A8F30, 0x20F1F0) */
inline CPnt3D *BoxCorners(CPnt3D *p, const CBBox &b)
{
    p[0] = CPnt3D(b.m_min.x, b.m_min.y, b.m_max.z);
    p[1] = b.m_min;
    p[2] = CPnt3D(b.m_max.x, b.m_min.y, b.m_min.z);
    p[3] = CPnt3D(b.m_max.x, b.m_min.y, b.m_max.z);
    p[4] = CPnt3D(b.m_min.x, b.m_max.y, b.m_max.z);
    p[5] = CPnt3D(b.m_min.x, b.m_max.y, b.m_min.z);
    p[6] = CPnt3D(b.m_max.x, b.m_max.y, b.m_min.z);
    p[7] = b.m_max;
    return p;
}

/* the node's parts hooked up (0x3ADAB0; the turret's Init's twin, the name ours): the "healthy"
 * node (the definition's ACTIVATE name), the health root, and the collision box's node -- the node
 * itself when it is named so -- its box taken from its DI (recursively when asked), half its
 * diagonal kept, its bottom corners and height kept, the region added to the AI's overlays; the
 * team joined */
void CVehicle::Init(CNode *node, const CVehicleDef &def)
{
    if (!node)
        return;
    if (def.m_20)
        m_healthy = node->FindChild(def.m_20, true);
    else
        m_healthy = node->FindChild("healthy", true);
    if (def.m_40)
        m_1d4 = node->FindChild(def.m_40, true);

    const char *name = def.m_60;

    m_radius = 0.0f;
    if (name) {
        CNode *bb = 0;

        if (!strcmp(node->m_name, name))
            bb = node;
        if (!bb)
            bb = node->FindChild(name, true);
        if (bb) {
            m_flags |= 4;
            bb->UpdateBBoxFromDI(def.m_80);

            const CBBox *box = bb->GetBBox();

            if (box) {
                CPnt3D size;

                box->GetSize(&size);
                m_radius = 0.5f * size.Length();

                CPnt3D c[8];
                CBBox b(*box);

                BoxCorners(c, b);
                m_bounds[0] = c[3];
                m_bounds[1] = c[2];
                m_bounds[2] = c[1];
                m_bounds[3] = c[0];

                float lo = c[0].y;
                float hi = lo;

                for (int i = 1; i < 8; i++) {
                    if (c[i].y < lo)
                        lo = c[i].y;
                    if (c[i].y > hi)
                        hi = c[i].y;
                }
                m_height = hi - lo;
            }
            ai::AddOverlay((CAiOverlay *)&m_region);
            bb->UpdateBBoxFromDI(true);
        }
    }
    m_radiusSq = m_radius * m_radius;
    SetTeamMask(def.m_84);
    JoinTeam(26);
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
        SampleDynamics(dT);
    }
}

/* the region's corners pushed out 5 units and carried into the world through the root's matrix --
 * CMatrix's default constructor is the unit-matrix call (a SetIdentity after it was a second call,
 * 8 bytes long: the constructor's &m is computed early among the corners' loads, the last corner's
 * store fills the call's delay slot) */
void CVehicle::Tick(float dT)
{
    unsigned int loc;
    CAiMap *map;

    if (!GetFlag(1))
        return;
    ComputeNextPosition(dT);
    if (!GetFlag(4))
        return;
    func_003ACC10(this, dT);
    if (!GetNode())
        return;
    if (!AiMaps.GetLocation(&m_posW, *(CAiMapLoc *)&loc))
        return;
    map = AiMaps.GetMapByIndex(((CAiMapLoc *)&loc)->m_mapid);
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

    m_root->BuildMTW(m);
    m.Transform(m_corners, 4);
    map->TileFootprint(m_corners, (CAiMap::region *)&m_region);
}

/* the node's frame sampled into 0x1E0: the cosine between its old and new Z (the turn), the step
 * from the position to the frame's origin and the velocity as the step over the frame (the
 * turret's SampleDynamics, its twin). The old Z assigned, so scalarised into $f20-$f22 across the
 * calls (copy-constructed it stays on the stack) */
void CVehicle::SampleDynamics(float dT)
{
    if (m_1d0) {
        CPnt3D z;

        z = *(CPnt3D *)m_1e0.m_matrix[2];

        sceVu0UnitMatrix(m_1e0.m_matrix);
        m_1d0->BuildMTW(m_1e0);

        CPnt3D *nz = (CPnt3D *)m_1e0.m_matrix[2];

        m_turn = z.x * nz->x + z.y * nz->y + z.z * nz->z;

        CPnt3D d = *(CPnt3D *)m_1e0.m_matrix[3] - m_posW;

        m_step = d;

        CPnt3D v = m_step * (1.0f / dT);

        m_velW = v;
    }
}

/* whether a point lies inside a quad of four corners in XZ (no edge's cross with it below zero),
 * the nearest point of the edges given back while it does (0x3ACC10's test, made twice -- new in
 * SOCOM II, not named). The edge's start is a pointer of its own walked with the index, stepped
 * first (`&corners[i]` strength-reduced, or declared in the loop, swaps it with the end's register) */
extern "C" bool func_003AD1F0(const CPnt3D *corners, const CPnt3D *p, CPnt3D *nearest)
{
    bool inside = true;
    float best;
    int i;
    const CPnt3D *a;

    for (i = 0, a = corners; i < 4 && inside; a++, i++) {
        const CPnt3D *b = &corners[(i + 1) % 4];
        CPnt3D e = *b - *a;
        CPnt3D d = *p - *a;

        if (e.z * d.x - e.x * d.z < 0.0f)
            inside = false;
        if (inside && nearest) {
            CPnt3D q;
            float t;

            p->DistToVector(a, b, &q, &t);
            if (i == 0 || t < best) {
                best = t;
                *nearest = q;
            }
        }
    }
    return inside;
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

    if (AiMaps.GetLocation(&m_posW, *(CAiMapLoc *)&loc)) {
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
 * destructor on the region and the cells as a second base change nothing. Round 25
 * (build/scratch/wep/v_vdt*.py): the cells a vector subclass under `#pragma inline_depth(6)` make
 * both byte-exact (the unit 29 of 30, GetBBox's copy off as at level 5) -- a depth-6 stand-in, not
 * adopted; a plain vector as the region's second base matches the constructor and leaves this 8
 * short, a subclass of __vector_imp with a user destructor too (each class level adds one to the
 * constructor's chain and one to the destructor's: the asymmetry the two want is not in a class).
 * Round 26 (build/scratch/wep/v_vdt3.py, v_vdt4.py): the target's extra word is a `b` to the call
 * with a1 = 0 in its slot, after the same two tests -- what a THIRD destructor class level over
 * the deleter gives (its redundant test becomes the `b`). Matching this with the constructor kept:
 * a wrapper struct round the vector (with or without a user destructor), a vector subclass with a
 * user constructor, or the subclass with a region constructor `: m_cells()` -- each 0x98 here but
 * CVehicle() 0xF8 (the region six high); an empty base with a user destructor (either base order),
 * an empty inline called from the region's destructor, `~CVehicleRegion() {}`, a base destructor
 * on CVehicleAiRegion and a __vector_imp subclass all leave this 0x90 with the constructor
 * byte-exact; `~CVehicleRegion() { m_cells.clear(); }` is 0x9C */
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
