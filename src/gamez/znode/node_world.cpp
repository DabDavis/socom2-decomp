/* node_world.cpp -- zdb::CWorld, the scene's root: its wind, its textured characters, its shadow,
 * light-map and landmark lists, its intersection queries and children (OVL1 0x31AF70-0x3209F3).
 * The unit name is SOCOM 1's (the press beta's CmdParseWind and ~CWorld assert node_world.cpp);
 * its order is SOCOM 1's, the wind first and the template instances last. SOCOM II opens it with
 * a REMOVE_DECALS command and the foliage sound's setter (CWorld::RemoveDecals, defined here, is
 * inlined into the command's tick, so they are this unit's), and adds a shadow caster list.
 * Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "node_world.h"
#include "../zanim/zanim.h"
#include "../zcamera/zcamera.h"
#include "../zsys/zsys.h"
#include "../zvid/zvid.h"

/* the light matrices from three directions, and from three colours and the ambient (libvu0's;
 * include/libvu0.h does not declare them) */
extern "C" void sceVu0NormalLightMatrix(sceVu0FMATRIX m, sceVu0FVECTOR l0, sceVu0FVECTOR l1,
                                        sceVu0FVECTOR l2);
extern "C" void sceVu0LightColorMatrix(sceVu0FMATRIX m, sceVu0FVECTOR c0, sceVu0FVECTOR c1,
                                       sceVu0FVECTOR c2, sceVu0FVECTOR a);

/* the grid's queries and the intersect set's setter not named yet (SOCOM 1's names, each
 * called where SOCOM 1's CWorld calls that one): CGrid::GetIntersectBatch, GetIntersect,
 * GetAltitude, InitDescendantsVisuals and InitDescendantsDI; DiIntersect's new mask setter */
extern "C" int func_002D3CF0(zdb::CGrid *grid, zdb::DiIntersect **di, int n);
extern "C" void func_002D4780(zdb::CGrid *grid, zdb::DiIntersect *di);
extern "C" void func_002D49C0(zdb::CGrid *grid, zdb::DiIntersect *di);
extern "C" void func_002D2DF0(zdb::CGrid *grid);
extern "C" void func_002D2F10(zdb::CGrid *grid);
extern "C" void func_002DC660(zdb::DiIntersect *di, unsigned int mask);
/* DiIntersect::GetNearestIntersect and GetHighestAltitude (SOCOM 1's, called where SOCOM 1's
 * GetIntersect and GetAltitude call them; not named yet) */
extern "C" zdb::IntersectStruct *func_002D4CC0(zdb::DiIntersect *di);
extern "C" zdb::IntersectStruct *func_002D4C20(zdb::DiIntersect *di, const CPnt3D *p, float alt);
/* SOCOM II's call on both decal pools after the recycle (not named yet) */
extern "C" void func_003BF050(zdb::CDecalPool *pool);
/* CTextureAssetCharacters' adder by library (this unit's, not yet written) and the asset list's
 * library finders by name and by phase (CAssetList's, not named yet) */
extern "C" void func_0031D260(zdb::CWorld *world, zdb::CAssetLib *lib, zdb::CNode *node);
extern "C" zdb::CAssetLib *func_00321130(zdb::CAssetList *list, const char *name);
extern "C" zdb::CAssetLib *func_003210E0(zdb::CAssetList *list, zdb::_RenderPhase phase);
/* the texture list's handle lookup (not named yet) */
extern "C" int func_00354790(zdb::CTexList *list, zdb::CTexture *tex);

namespace zdb {

/* a pointer's object deleted and the pointer cleared: the operation SOCOM 1's lists are freed
 * with, through std::transform (the name is ours) */
template <class T>
struct DeleteAndClear {
    DeleteAndClear() {}
    T *operator()(T *p) const
    {
        delete p;
        return 0;
    }
};

/* the version the world's archives carry (SOCOM 1's) */
int CWorld::GetVersion()
{
    return 0x10001;
}

/* nothing to set up or tear down (SOCOM 1's) */
void CWorld::Init()
{
}

void CWorld::Uninit()
{
}

/* the camera made and placed at the origin, the global lights to be sent, the shadow texture's
 * handle made and added to the shadow phase's buffer (SOCOM 1's) */
bool CWorld::Initalize()
{
    CCamera *cam = new CCamera;

    if (m_camera != cam) {
        m_camera = cam;
        CVisual::m_camera = cam;
    }
    m_camera->SetName("camera");
    m_camera->SetPosition(CPnt3D::zero);
    m_world->m_gLightColUpdate = 1;
    m_world->m_gLightDirUpdate = 1;
    m_shadowTexH = CTexHandle::Create(m_shadowTex[0]);
    CGSTexBuffer *buf = CTexManager::m_texmanager[CTexManager::TexPhase[45]];

    buf->m_dyntex.Add(m_shadowTexH, true);
    buf->m_18 = 0;
    return true;
}

/* a model of the loaded libraries' (SOCOM 1's) */
CNode *CWorld::GetModel(const char *name)
{
    return CAssetMgr::m_assets.GetModel(name);
}

/* a light map listed, given the default light map texture if it has none and is neither a shadow
 * nor a reflection (SOCOM 1's) */
void CWorld::AddLightMap(CLightMap *light)
{
    if (!light->m_pTex && !light->m_isShadow && !light->m_isReflection)
        light->SetTexture(CVisual::m_lightMapTexture->m_texture);
    m_lightMaps.push_back(light);
}

/* a shadow made for the node, on the next shadow texture, and listed (SOCOM 1's) */
void CWorld::AddShadow(CNode *node)
{
    CTexture *tex = m_shadowTex[m_shadows.size()];

    m_shadows.push_back(new CRenderMap(node, RENDER_MAP_SHADOW, tex));
}

/* a shadow caster made for the node and listed */
void CWorld::AddShadowCaster(CNode *node)
{
    m_shadow_casters.push_back(new CShadowCaster(node));
}

/* room for n more children (SOCOM 1's) */
void CWorld::ReserveChildren(int n)
{
    m_child.reserve(m_child.size() + n);
}

/* the wind ticked; the global lights' matrices rebuilt into the camera's VU block when they
 * changed (SOCOM 1's) */
void CWorld::Update()
{
    m_Wind.Tick(zVid.frameTime);
    tag_FrameSetup *vu = m_camera->m_mtxSet.m_vu;

    if (m_gLightDirUpdate) {
        sceVu0NormalLightMatrix(vu->mtxLightNormal, (float *)&m_gLight.dir[0],
                                (float *)&m_gLight.dir[1], (float *)&m_gLight.dir[2]);
        vu->mtxLightNormal[3][3] = 1.0f;
        m_gLightDirUpdate = 0;
    }
    if (m_gLightColUpdate) {
        sceVu0LightColorMatrix(vu->mtxLightColor, (float *)&m_gLight.col[0],
                               (float *)&m_gLight.col[1], (float *)&m_gLight.col[2],
                               (float *)&m_gLight.ambient);
        m_gLightColUpdate = 0;
    }
}

/* every landmark deleted, the list emptied (SOCOM 1's) */
void CWorld::ReleaseLandmarkList()
{
    for (CNodeVector::iterator it = m_landmarks.begin(); it != m_landmarks.end(); ++it)
        CNode::Delete(*it);
    m_landmarks.clear();
}

/* the lights' list emptied (SOCOM 1's) */
void CWorld::ReleaseLightList()
{
    m_lights.erase(m_lights.begin(), m_lights.end());
}

/* a landmark node taken off its parent and listed, unless it is a world child (SOCOM 1's) */
void CWorld::AddLandmark(CNode *node)
{
    if (node->m_landmark && !node->m_bumpmap) {
        node->RemoveFromParent();
        m_landmarks.push_back(node);
    }
}

/* a landmark no more (SOCOM 1's) */
void CWorld::DeleteLandmark(CNode *node)
{
    if (node->IsLandmark()) {
        node->m_landmark = false;
        m_landmarks.Remove(node);
    }
}

/* a landmark by name (SOCOM 1's) */
CNode *CWorld::FindLandmark(const char *name)
{
    for (CNodeVector::iterator it = m_landmarks.begin(); it != m_landmarks.end(); ++it) {
        CNode *node = *it;

        if (!strcmp(node->m_name, name))
            return node;
    }
    return 0;
}

/* the nearest hit along a segment, characters hit as asked */
IntersectStruct *CWorld::GetIntersect(const CPnt3D *p0, const CPnt3D *p1, bool characters)
{
    bool was = m_diInt->SetIntersectCharacters(!characters);
    IntersectStruct *hit;

    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    func_002D4780(&m_grid, m_diInt);
    hit = m_diInt->m_Cnt ? func_002D4CC0(m_diInt) : 0;
    m_diInt->SetIntersectCharacters(was);
    return hit;
}

/* the nearest hit along a segment (SOCOM 1's) */
IntersectStruct *CWorld::GetIntersect(const CPnt3D *p0, const CPnt3D *p1)
{
    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    func_002D4780(&m_grid, m_diInt);
    if (m_diInt->m_Cnt)
        return func_002D4CC0(m_diInt);
    return 0;
}

/* every hit along a segment (SOCOM 1's) */
DiIntersect *CWorld::GetIntersects(const CPnt3D *p0, const CPnt3D *p1)
{
    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    func_002D4780(&m_grid, m_diInt);
    return m_diInt;
}

/* every hit along a segment, characters hit as asked */
DiIntersect *CWorld::GetIntersects(const CPnt3D *p0, const CPnt3D *p1, bool characters)
{
    bool was = m_diInt->SetIntersectCharacters(!characters);

    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    func_002D4780(&m_grid, m_diInt);
    m_diInt->SetIntersectCharacters(was);
    return m_diInt;
}

/* the highest ground under a point, characters hit as asked */
IntersectStruct *CWorld::GetAltitude(const CPnt3D *p, float alt, bool characters)
{
    bool was = m_diInt->SetIntersectCharacters(!characters);
    IntersectStruct *hit;

    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    func_002D49C0(&m_grid, m_diInt);
    hit = m_diInt->m_Cnt ? func_002D4C20(m_diInt, p, alt) : 0;
    m_diInt->SetIntersectCharacters(was);
    return hit;
}

/* the highest ground under a point (SOCOM 1's) */
IntersectStruct *CWorld::GetAltitude(const CPnt3D *p, float alt)
{
    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    func_002D49C0(&m_grid, m_diInt);
    if (m_diInt->m_Cnt)
        return func_002D4C20(m_diInt, p, alt);
    return 0;
}

/* every ground hit under a point, characters not hit, no node ignored (SOCOM 1's; the height is
 * not read) */
DiIntersect *CWorld::GetAltitudes(const CPnt3D *p, float alt)
{
    bool was = m_diInt->m_IntersectCharacters;

    m_diInt->m_Type = 2;
    m_diInt->m_IntersectCharacters = false;
    m_diInt->m_Node = 0;
    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    func_002D49C0(&m_grid, m_diInt);
    m_diInt->SetIntersectCharacters(was);
    return m_diInt;
}

/* a batch of intersect sets run through the world's grid (SOCOM 1's) */
int CWorld::GetIntersectBatch(DiIntersect **di, int n)
{
    return func_002D3CF0(&m_world->m_grid, di, n);
}


void CWorld::InitDescendantsDI()
{
    func_002D2F10(&m_grid);
}

void CWorld::InitDescendantsVisuals()
{
    func_002D2DF0(&m_grid);
}

/* the delayed intersect sets run, then forgotten (SOCOM 1's scheduler task) */
int CWorld::diTick(float dt, void *data)
{
    m_world->GetIntersectBatch(m_world->m_diIntDelay, m_world->m_diIntDelayCnt);
    m_world->m_diIntDelayCnt = 0;
    return 0;
}

/* a segment set to be intersected at the next tick, its index given back (SOCOM 1's; SOCOM II's
 * takes the set's mask) */
int CWorld::RegIntersectDelayed(DiIntersect *di, const CPnt3D *tail, const CPnt3D *tip, unsigned int mask)
{
    if (!di)
        return -1;
    int i = m_diIntDelayCnt;

    if (i >= 256)
        return -1;
    m_diIntDelayCnt = i + 1;
    m_diIntDelay[i] = di;
    di->m_Type = 2;
    di->m_Tail = *tail;
    di->m_Tip = *tip;
    func_002DC660(di, mask);
    return i;
}

/* the node a delayed set ignores (SOCOM 1's) */
int CWorld::RegIgnoreNodeDelayed(DiIntersect *di, CNode *node)
{
    if (!di)
        return -1;
    if (!node)
        return -1;
    di->m_Node = node;
    return 1;
}

int CWorld::ClearIgnoreNodeDelayed(DiIntersect *di)
{
    if (!di)
        return -1;
    di->m_Node = 0;
    return 1;
}

/* a sphere set to be tested at the next tick, its index given back (SOCOM 1's; SOCOM II's takes
 * the set's mask) */
int CWorld::RegProximityDelayed(DiIntersect *di, const CPnt3D *p, float radius, unsigned int mask)
{
    if (!di)
        return -1;
    int i = m_diIntDelayCnt;

    if (i >= 256)
        return -1;
    m_diIntDelayCnt = i + 1;
    m_diIntDelay[i] = di;
    di->m_Type = 4;
    di->m_Tail = *p;
    func_002DC660(di, mask);
    di->m_Tip.x = radius;
    di->m_Tip.y = 0.0f;
    di->m_Tip.z = 0.0f;
    return i;
}

/* a textured character's texture added for every character (SOCOM 1's) */
void CWorld::AddTextureAssetCharacterAll(CTexHandle *tex)
{
    m_TextureCharacterCommon.push_back(tex);
}

/* a decal texture listed, unless it is already (SOCOM 1's) */
void CWorld::AddTextureAssetDecals(CTexHandle *tex)
{
    if (!func_00354790(&m_TextureDecals, tex->m_texture))
        m_TextureDecals.push_back(tex);
}

/* the light maps deleted, the list emptied and its storage given up (SOCOM 1's). FreeVector runs
 * on the base vector: its temporary's out-of-line destructor (0x31CC70) has three this tests, a
 * pointer vector's, where CLightMapVector's own has four */
void CWorld::ClearLightMapList()
{
    CLightMapVector &v = m_lightMaps;

    if (v.size()) {
        std::transform(v.begin(), v.end(), v.begin(), DeleteAndClear<CLightMap>());
        v.clear();
    }
    FreeVector<std::vector<CLightMap *> >(m_lightMaps);
}

/* the shadows deleted, the list emptied and its storage given up (SOCOM 1's) */
void CWorld::ClearShadowList()
{
    CRenderMapVector &v = m_shadows;

    if (v.size()) {
        std::transform(v.begin(), v.end(), v.begin(), DeleteAndClear<CRenderMap>());
        v.clear();
    }
    FreeVector<std::vector<CRenderMap *> >(m_shadows);
}

/* every child's decals removed, the decal pools recycled and emptied (SOCOM 1's; inlined into the
 * REMOVE_DECALS tick) */
void CWorld::RemoveDecals()
{
    CMeshDecal::m_tempPool.RecycleTick();
    CDecal::m_tempPool.RecycleTick();
    unsigned int n = m_child.size();

    for (unsigned int i = 0; i < n; i++)
        m_child[i]->RemoveDecals(0);
    func_003BF050(&CDecal::m_tempPool);
    func_003BF050(&CDecal::m_permPool);
}

}

/* the WIND command registered (SOCOM 1's) */
void CWind::RegisterAnimCommands()
{
    ZAnim.AddCmd("WIND", CmdParseWind, 0, CmdTickWind, 0);
}

/* the azimuth, zenith and speed each moved toward its goal at its rate, stopping there; the goal
 * ticks while any of them moves (SOCOM 1's) */
void CWind::TickGoal(float dt)
{
    if (m_TickAzimuth) {
        m_Azimuth += m_AzimuthGoalDelta * dt;
        if (m_AzimuthGoalDelta > 0.0f) {
            if (!(m_Azimuth < m_AzimuthGoal)) {
                m_Azimuth = m_AzimuthGoal;
                m_TickAzimuth = false;
            }
        } else if (m_Azimuth <= m_AzimuthGoal) {
            m_Azimuth = m_AzimuthGoal;
            m_TickAzimuth = false;
        }
    }
    if (m_TickZenith) {
        m_Zenith += m_ZenithGoalDelta * dt;
        if (m_ZenithGoalDelta > 0.0f) {
            if (!(m_Zenith < m_ZenithGoal)) {
                m_Zenith = m_ZenithGoal;
                m_TickZenith = false;
            }
        } else if (m_Zenith <= m_ZenithGoal) {
            m_Zenith = m_ZenithGoal;
            m_TickZenith = false;
        }
    }
    if (m_TickSpeed) {
        m_Speed += m_SpeedGoalDelta * dt;
        if (m_SpeedGoalDelta > 0.0f) {
            if (!(m_Speed < m_SpeedGoal)) {
                m_Speed = m_SpeedGoal;
                m_TickSpeed = false;
            }
        } else if (m_Speed <= m_SpeedGoal) {
            m_Speed = m_SpeedGoal;
            m_TickSpeed = false;
        }
    }
    m_TickGoal = m_TickAzimuth || m_TickZenith || m_TickSpeed;
}

/* still air from due north, speed 8, and the gusts' ranges and rates (SOCOM 1's) */
void CWind::Reset()
{
    m_AzimuthGoal = m_ZenithGoal = 0.0f;
    m_SpeedGoal = 8.0f;
    m_TickGoal = false;
    m_Azimuth = m_Zenith = 0.0f;
    m_Speed = 8.0f;
    m_AzimuthOffset = 0.0f;
    m_MinAzimuthOffset = -0.7853982f;
    m_MaxAzimuthOffset = 0.7853982f;
    m_ZenithOffset = 0.0f;
    m_MinZenithOffset = -0.7853982f;
    m_MaxZenithOffset = 0.7853982f;
    m_AzimuthOffsetVelocity = 0.2617994f;
    m_ZenithOffsetVelocity = 0.08726646f;
    m_SpeedOffset = 3.0f;
    m_MinSpeedOffset = -7.5f;
    m_MaxSpeedOffset = 9.0f;
    m_SpeedOffsetVelocity = 3.0f;
}

namespace zdb {

/* the foliage sound's name replaced by a copy (or none) */
void CWorld::SetFoliageSound(const char *name)
{
    if (m_foliage_sound)
        _free(m_foliage_sound, 0, 0);
    m_foliage_sound = 0;
    if (name)
        m_foliage_sound = _strdup(name, 0, 0);
}

/* the REMOVE_DECALS command registered */
void CWorld::RegisterAnimCommands()
{
    ZAnim.AddCmd("REMOVE_DECALS", CmdParseRemoveDecals, 0, CmdTickRemoveDecals, 0);
}

/* the command: a bare header, timeless */
_zanim_cmd_hdr *CWorld::CmdParseRemoveDecals(_zrdr *rdr)
{
    if (!rdr)
        return 0;
    _zanim_cmd_hdr *cmd = (_zanim_cmd_hdr *)_calloc(1, sizeof(_zanim_cmd_hdr), 0, 0);

    cmd->data_size = sizeof(_zanim_cmd_hdr);
    cmd->timeless = 1;
    return cmd;
}

/* the world's decals removed */
bool CWorld::CmdTickRemoveDecals(_zanim_cmd_hdr *cmd, float *dt)
{
    m_world->RemoveDecals();
    return true;
}

}
