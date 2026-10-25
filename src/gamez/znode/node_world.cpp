/* node_world.cpp -- zdb::CWorld, the scene's root: its wind, its textured characters, its shadow,
 * light-map and landmark lists, its intersection queries and children (OVL1 0x31AF70-0x3209F3).
 * The unit name is SOCOM 1's (the press beta's CmdParseWind and ~CWorld assert node_world.cpp);
 * its order is SOCOM 1's, the wind first and the template instances last. SOCOM II opens it with
 * a REMOVE_DECALS command and the foliage sound's setter (CWorld::RemoveDecals, defined here, is
 * inlined into the command's tick, so they are this unit's), and adds a shadow caster list.
 * Defined in reverse of their image order (-inline deferred). */

#include <algorithm>
#include <stdio.h>
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

/* a light map taken off the list (SOCOM 1's: std::remove inlined, its remove_copy out of line).
 * OPEN, 1 instruction (0x13C of 0x138): the target computes end() once, at 76(sp), and copies it
 * for remove's last and again for erase's; written this way end() is made twice, and a local end
 * iterator passed to both makes the walk shorter still (0x124) */
void CWorld::DeleteLightMap(CLightMap *light)
{
    m_lightMaps.erase(std::remove(m_lightMaps.begin(), m_lightMaps.end(), light), m_lightMaps.end());
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

/* a vector's storage cut to its size: a copy of it swapped in. Inlined one level, as zsys.h's
 * FreeVector is -- its copy constructor, swap and destructor called out of line (the name ours) */
template <class V>
inline void TrimVector(V &v)
{
    V tmp(v);

    v.swap(tmp);
}

/* the children's storage cut to their count, and each child but the cells its atom pointers'
 * (SOCOM 1's; the vector's copy constructor's out-of-line copy follows it) */
void CWorld::GarbageCollect()
{
    TrimVector<std::vector<CNode *> >(GetChildren());
    for (CNodeVector::iterator it = GetChildren().begin(); it != GetChildren().end(); ++it)
        if (!dynamic_cast<CCell *>(*it))
            (*it)->ShrinkAtomBasePtrs();
}

/* a shadow caster made for the node and listed (pinned: AddChild calls it, where AddShadow, its
 * neighbour, is inlined) */
#pragma auto_inline off
void CWorld::AddShadowCaster(CNode *node)
{
    m_shadow_casters.push_back(new CShadowCaster(node));
}
#pragma auto_inline reset

/* each shadow caster's node switched off, its texture taken from the node's first visual's first
 * dynamic texture, and the caster brought up to date (new in SOCOM II, between ReserveChildren and
 * AddShadowCaster; the name is ours) */
void CWorld::UpdateShadowCasters()
{
    for (std::vector<CShadowCaster *>::iterator it = m_shadow_casters.begin();
         it != m_shadow_casters.end(); ++it) {
        CShadowCaster *c = *it;

        c->m_node->SetActive(false);
        c->m_texture = c->m_node->m_visual[0]->m_dynTexList.m_tex_list[0]->m_texture;
        func_0031F570(*it);
    }
}

/* room for n more children (SOCOM 1's) */
void CWorld::ReserveChildren(int n)
{
    m_child.reserve(m_child.size() + n);
}

/* a child added: taken from its old parent, put in the grid and listed (room made a hundred at a
 * time); a model that casts shadows given one, a shadow caster made for a node that wants one, and
 * a common character set member made dynamic and given its textures (SOCOM 1's; SOCOM II's shadow
 * flag is 0xA1's bit 5, and the caster and the character set are its own) */
void CWorld::AddChild(CNode *child)
{
    if (child->m_parent && child->m_parent != this)
        child->m_parent->DeleteChild(child);
    m_grid.Insert(child);
    child->m_parent = this;
    if (m_child.size() == m_child.capacity())
        m_child.reserve(m_child.size() + 100);
    m_child.push_back(child);
    CNode *model = child->m_type == 2 ? child : 0;

    if (model && model->m_socom2_a1_bits.bit5 == 1)
        AddShadow(model);
    if (child->m_shadow == 1)
        AddShadowCaster(child);
    if (child->m_char_common) {
        child->SetDynamic(true, true);
        CNode *n = child;

        n->m_socom2_a1_bits.bit2 = 1;
        n->m_bumpmap = 1;
        CNode *node = child;
        CAssetLib *lib = CAssetMgr::m_assets.FindLib(CHARSETCOMMON);

        AddTextureAssetCharacter(lib, node);
    }
}

}

/* a shadow deleted when it is the node's (SOCOM 1's functor, by the name its remove_if instance
 * carries) */
struct delshadow {
    delshadow(zdb::CNode *node) : m_node(node) {}
    bool operator()(zdb::CRenderMap *shadow) const
    {
        if (shadow->m_node == m_node) {
            delete shadow;
            return true;
        }
        return false;
    }

    zdb::CNode *m_node;
};

namespace zdb {

/* a child taken off: its shadows deleted, out of the grid unless a cell, off the list and its
 * parent forgotten, its holders counted down (SOCOM 1's; the light maps' half is gone). A child
 * of another node gives back its count.
 * OPEN, 2 instructions (0x11C of 0x114): DeleteLightMap's -- the target builds end() once (76(sp)),
 * passes it to remove_if and copies it for erase's last after the call, where this stores it twice
 * and copies before; the remove_if instance (0x320800) misses on its predicate, which the inlined
 * find_if copies where the target reads it in place (include/algorithm's) */
int CWorld::DeleteChild(CNode *child)
{
    if (!child)
        return 0;
    if (child->m_socom2_a1_bits.bit5 == 1)
        m_shadows.erase(std::remove_if(m_shadows.begin(), m_shadows.end(), delshadow(child)),
                        m_shadows.end());
    if (child->m_parent != this)
        return child->m_count;
    if (!dynamic_cast<CCell *>(child))
        m_grid.Remove(child);
    m_child.Remove(child);
    child->m_parent = 0;
    return child->Release();
}

/* a child that is not a cell let go of: out of the grid, its parent forgotten, deleted when no one
 * holds it; its slot cleared (DeleteChildren's first pass; the name is ours) */
struct ReleaseNonCell {
    ReleaseNonCell(CWorld *world) : m_world(world) {}
    CNode *operator()(CNode *node) const
    {
        if (dynamic_cast<CCell *>(node))
            return node;
        m_world->m_grid.Remove(node);
        node->m_parent = 0;
        if (!node->Release())
            delete node;
        return 0;
    }

    CWorld *m_world;
};

/* a cell deleted, its slot cleared (the second pass; the name is ours) */
struct DeleteCell {
    DeleteCell() {}
    CNode *operator()(CNode *node) const
    {
        CCell *cell = dynamic_cast<CCell *>(node);

        if (cell) {
            cell->m_parent = 0;
            delete cell;
            return 0;
        }
        return node;
    }
};

/* every child taken off: the others first, then the cells, the list emptied (SOCOM 1's).
 * OPEN, 2 instructions (0x27C of 0x274): each end() reloads data_ where the target computes it from
 * the load begin() made (the pod vector's end() lever, as ~CAssetLib's); the functors and the
 * transforms' copies are the target's */
void CWorld::DeleteChildren()
{
    std::transform(m_child.begin(), m_child.end(), m_child.begin(), ReleaseNonCell(this));
    std::transform(m_child.begin(), m_child.end(), m_child.begin(), DeleteCell());
    m_child.clear();
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

/* a tree's landmarks gathered, each given its world matrix, the drawn nodes without far clipping
 * counted, the matrix stack pushed and popped around each node (SOCOM 1's) */
void CWorld::LandmarkList_Search(CNode *node, CStack &stack, CNodeVector &list)
{
    if (node) {
        stack.Multiply(&node->m_matrix, true);
        if (node->m_hasVisuals) {
            if (node->m_landmark) {
                node->SetMatrix(CStack::m_top);
                list.push_back(node);
            } else if (!node->m_apply_clip)
                m_numNoFarClipNodes++;
        }
        for (CNodeVector::iterator it = node->m_child.begin(), e = node->m_child.end(); it != e;
             ++it)
            LandmarkList_Search(*it, stack, list);
        stack.Pop();
    }
}

/* the landmarks gathered from the whole tree and listed afresh, their count given back (SOCOM 1's).
 * OPEN, 10 instructions (the right size): the local vector's field addresses trade saved
 * registers -- the target holds &size_ in s0 and &data_ in s1 (SOCOM 1's the other way round, as
 * here); not the declaration order of the vector, the stack or the count, a plain std::vector, the
 * clear's place, a postfix step or a pointer walk */
int CWorld::GenerateLandmarkList()
{
    CNodeVector landmarks;
    CStack stack;

    m_numNoFarClipNodes = 0;
    LandmarkList_Search(this, stack, landmarks);
    int n = landmarks.size();

    m_landmarks.clear();
    for (CNodeVector::iterator it = landmarks.begin(); it != landmarks.end(); ++it)
        AddLandmark(*it);
    landmarks.clear();
    return n;
}

/* every landmark deleted, the list emptied (SOCOM 1's) */
void CWorld::ReleaseLandmarkList()
{
    for (CNodeVector::iterator it = m_landmarks.begin(); it != m_landmarks.end(); ++it)
        CNode::Delete(*it);
    m_landmarks.clear();
}

/* a tree's lights listed, each at the front, the cells left out; SOCOM II also gives each
 * transparent visual the mip levels of phase 19, and marks a node whose first visual's material is
 * of type 2 (SOCOM 1's) */
void LightList_Search(CNode *node, CLightList &lights)
{
    if (node && node->GetType() != 10) {
        CLight *light = 0;

        if (node->GetType() == 8) {
            if ((light = dynamic_cast<CLight *>(node)) != 0)
                ((PtrListBase &)lights).insert(ListBegin(lights), (const void *const &)light);
        }
        if (node->m_visual.size()) {
            for (CVisualVector::iterator it = node->m_visual.begin(), e = node->m_visual.end(); it != e;
                 ++it) {
                CVisual *v = *it;

                if ((v->m_flags & 1) & ((v->m_flags & 0x100) == 0))
                    CAssetMgr::AddMipLevels(&v->m_dynTexList, (_RenderPhase)19);
            }
            unsigned int m = node->GetFirstVisual()->m_7;

            if (m) {
                MATERIAL_ENTRY *mat = CWorld::m_world->m_Material_Object.GetMaterial(m);

                if (mat && mat->m_type == 2)
                    node->m_socom2_a1_bits.bit7 = true;
            }
        }
        for (CNodeVector::iterator it = node->m_child.begin(), e = node->m_child.end(); it != e; ++it)
            LightList_Search(*it, lights);
    }
}

/* the lights gathered afresh, each placed in the world and listed in every cell its range covers
 * in x/z, their count given back (SOCOM 1's). The clear and the walk's first node both bind the
 * sentinel's link by reference (znode.h's ListClear and PtrListFirst, SOCOM II's begin()): the
 * target computes &m_lights.end_.next_ once, before the clear, and reads the walk's first node
 * through it after the search; the iterator is assigned (a bitwise copy, then kept in memory) */
int CWorld::GenerateLightList()
{
    ListClear(m_lights);
    LightList_Search(this, m_lights);
    CLightList::iterator it, e = m_lights.end();

    for (it = CLightList::iterator(PtrListFirst(m_lights.end_node().next_)); it != e; ++it) {
        CLight *light = *it;
        CPnt3D pos(0.0f, 0.0f, 0.0f);
        CPnt3D bounds[2];

        light->TransformToWorld(pos);
        light->m_WorldPosition = pos;
        bounds[0].x = pos.x - light->GetMaxRange();
        bounds[0].y = 0.0f;
        bounds[0].z = pos.z - light->GetMaxRange();
        bounds[1].x = pos.x + light->GetMaxRange();
        bounds[1].y = 0.0f;
        bounds[1].z = pos.z + light->GetMaxRange();
        m_grid.SetTraversalBoundary(bounds, 2, false);
        for (CGridAtom *atom = m_grid.StartTraversal(); atom; atom = m_grid.GetNextAtom()) {
            CCell *cell = atom->GetCell();

            if (cell)
                cell->m_lightList.Add(light);
        }
    }
    return m_lights.size();
}

/* the custom character nodes found so far (new in SOCOM II, node_world.cpp's static; the name is
 * ours) */
static unsigned int s_customCharacterCnt;

/* a tree's custom characters: the flagged (bit 11), non-instanced nodes outside the cells -- a
 * landmark one stacked, any other given the custom characters' textures at once (new in SOCOM II;
 * the name ours) */
void FindCustomCharacters(CNode *node, CNode **list)
{
    if (node && node->GetType() != 10) {
        if (node->m_bumpmap && !(node->m_socom2_a1 & 4)) {
            if (node->m_landmark) {
                list[s_customCharacterCnt] = node;
                s_customCharacterCnt++;
            } else {
                CWorld *world = CWorld::m_world;

                world->AddTextureAssetCharacter(CAssetMgr::m_assets.FindLib(CHARSETCUSTOMCHARS),
                                                node);
            }
            return;
        }
        for (CNodeVector::iterator it = node->m_child.begin(), e = node->m_child.end(); it != e;
             ++it)
            FindCustomCharacters(*it, list);
    }
}

/* the world's custom characters given the custom characters' textures (new in SOCOM II: CMission's
 * Open and ReOpen_Load; the name ours) */
void CWorld::AddCustomCharacters()
{
    CNode *list[32];

    s_customCharacterCnt = 0;
    FindCustomCharacters(this, list);
    for (unsigned int i = 0; i < s_customCharacterCnt; i++) {
        CNode *node = list[i];
        CAssetLib *lib = CAssetMgr::m_assets.FindLib(CHARSETCUSTOMCHARS);

        AddTextureAssetCharacter(lib, node);
    }
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

/* a node's children sorted into the cells and the rest; whether the node is the world is asked
 * and the answer dropped (SOCOM 1's, the same; its this is not read) */
void CWorld::Segregate_CCells(CNode *node, CNodeVector &cells, CNodeVector &others)
{
    if (node) {
        CNodeVector::iterator it = node->m_child.begin(), e = node->m_child.end();

        dynamic_cast<CWorld *>(node);
        for (; it != e; ++it) {
            CNode *cell = dynamic_cast<CCell *>(*it);

            if (cell)
                cells.push_back(cell);
            else
                others.push_back(*it);
        }
    }
}

/* the world model's children made the world's own, each put in the grid, and the emptied model
 * deleted (SOCOM 1's; read through the inline accessor, which keeps &m_child in a register for the
 * loop's end, the member's clear stored directly) */
bool CWorld::DismemberWorldModel()
{
    CModel *model = CAssetMgr::m_assets.GetModel("worldmodel");

    if (!model)
        return false;
    ReserveChildren(model->GetChildren().size());
    for (CNodeVector::iterator it = model->GetChildren().begin(); it != model->GetChildren().end();
         ++it) {
        CNode *child = *it;

        m_child.push_back(child);
        child->m_parent = this;
        m_grid.Update(child);
    }
    model->m_child.clear();
    delete model;
    return true;
}

/* whether a texture list holds every texture of another, and is no shorter (new in SOCOM II:
 * AddTextureAssetCharacter asks it of each set's list; the name is ours). Pinned: its one caller
 * calls it out of line */
#pragma auto_inline off
bool TexListHolds(CTexList *have, CTexList *want)
{
    bool ok = have->size() != 0 && want->size() <= have->size();

    for (CTexList::iterator it = want->begin(); it != want->end() && ok; ++it)
        if (!have->GetHandle((*it)->m_texture))
            ok = false;
    return ok;
}
#pragma auto_inline reset

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

/* a node no longer drawn with its character set's textures: taken off the first set that has it,
 * of the libraries of a name or a render phase (SOCOM 1's took it off every set and deleted the
 * sets left empty; SOCOM II leaves that to RemoveEmptyTextureAssetCharacters) */
inline void CWorld::DeleteTextureAssetCharacter(CAssetLib *lib, CNode *node)
{
    if (lib) {
        bool removed = false;

        for (CTextureAssetCharacterList::iterator it = m_TextureAssetCharacters.begin(),
                                                  e = m_TextureAssetCharacters.end();
             it != e && removed == false; ++it)
            removed = (*it)->m_nodes.Remove(node);
    }
}

void CWorld::DeleteTextureAssetCharacter(char *name, CNode *node)
{
    DeleteTextureAssetCharacter(CAssetMgr::m_assets.FindLib(name), node);
}

void CWorld::DeleteTextureAssetCharacter(_RenderPhase phase, CNode *node)
{
    DeleteTextureAssetCharacter(CAssetMgr::m_assets.FindLib(phase), node);
}

/* the character sets no node is drawn with any more deleted, the walk begun again after each (new
 * in SOCOM II: SOCOM 1's DeleteTextureAssetCharacter did this as it went).
 * OPEN, 0x11C of 0x144: `delete c` inlines the destructor and the nodes' vector teardown whole in
 * the target (the set's null test, doDelBuffer, then four this-tests of &m_nodes and the block
 * freed) -- five destructor levels under the delete, where include/vector's pointer vector makes
 * ~CNodeVector a call (-1) here; it turns on the vector remodel. Apart from that the target keeps
 * the set in s0 across the remove (loaded from its stack home before the size test), where this
 * reloads it after the call */
void CWorld::RemoveEmptyTextureAssetCharacters()
{
    CTextureAssetCharacterList::iterator it = m_TextureAssetCharacters.begin(),
                                         e = m_TextureAssetCharacters.end();

    while (it != e) {
        CTextureAssetCharacters *c = *it;

        if (c->m_nodes.size() == 0) {
            m_TextureAssetCharacters.remove(c);
            delete c;
            it = m_TextureAssetCharacters.begin();
            e = m_TextureAssetCharacters.end();
        } else
            ++it;
    }
}

/* every character set's buffer released (new in SOCOM II: CCoreState::Init) */
void CWorld::ReleaseTextureAssetCharacters()
{
    CTextureAssetCharacterList::iterator it, e = m_TextureAssetCharacters.end();

    for (it = m_TextureAssetCharacters.begin(); it != e; ++it)
        if ((*it)->m_buffer)
            func_00356BE0((*it)->m_buffer, false);
}

/* every character set's nodes let go of and the set deleted, the list emptied (SOCOM 1's).
 * OPEN, 0xF0 of 0x124: as RemoveEmptyTextureAssetCharacters, `delete c` inlines ~CNodeVector's four
 * this-tested levels in the target and calls it here (the vector remodel); FreeVector's
 * constructor, clear, swap and destructor calls and the list's clear already line up */
void CWorld::WipeoutTextureAssetCharacters()
{
    CTextureAssetCharacterList::iterator it, e = m_TextureAssetCharacters.end();

    for (it = m_TextureAssetCharacters.begin(); it != e; ++it) {
        CTextureAssetCharacters *c = *it;

        FreeVector<std::vector<CNode *> >(c->m_nodes);
        delete c;
    }
    m_TextureAssetCharacters.clear();
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

/* a character set for a library's textures, in a buffer of its own named after the node (with a
 * dynamic list) or the library, its first node listed (SOCOM 1's; the node's presence is tested
 * and the answer dropped, as there) */
CTextureAssetCharacters::CTextureAssetCharacters(CAssetLib *lib, CNode *node, CDynTexList *dyntex)
{
    m_visible = 0;
    m_lib = lib;
    m_buffer = 0;
    if (node) {
        char name[256];

        m_nodes.Exists(node);
        m_nodes.push_back(node);
        sprintf(name, "%s", dyntex ? node->m_name : lib->RootName());
        m_buffer = CTexManager::m_texmanager[func_003566C0(&CTexManager::m_texmanager, name,
                                                           CTexManager::TexBufParams[0],
                                                           CTexManager::TexBufParams[1])];
        if (dyntex)
            func_00357820(m_buffer, dyntex);
        else
            func_003569F0(m_buffer, lib, true);
    }
}

}

/* the WIND command registered (SOCOM 1's) */
void CWind::RegisterAnimCommands()
{
    ZAnim.AddCmd("WIND", CmdParseWind, 0, CmdTickWind, 0);
}

/* the WIND command: which settings it carries, then each -- a goal and the time to reach it for the
 * azimuth, zenith and speed, the gusts' ranges and rates (SOCOM 1's command, by CmdTickWind's
 * reads; the names ours) */
struct _zanim_cmd_wind : public _zanim_cmd_hdr {
    unsigned short flags;   // 0x4
    float azimuth;   // 0x8
    float azimuth_time;   // 0xc
    float zenith;   // 0x10
    float zenith_time;   // 0x14
    float speed;   // 0x18
    float speed_time;   // 0x1c
    float min_azimuth_offset;   // 0x20
    float max_azimuth_offset;   // 0x24
    float azimuth_offset_velocity;   // 0x28
    float min_zenith_offset;   // 0x2c
    float max_zenith_offset;   // 0x30
    float zenith_offset_velocity;   // 0x34
    float min_speed_offset;   // 0x38
    float max_speed_offset;   // 0x3c
    float speed_offset_velocity;   // 0x40
};   // 0x44 bytes

/* the world's wind given the command's settings: each goal reached at once or over its time, the
 * gusts' ranges and rates set (SOCOM 1's) */
bool CWind::CmdTickWind(_zanim_cmd_hdr *hdr, float *dt)
{
    _zanim_cmd_wind *cmd = (_zanim_cmd_wind *)hdr;

    if (cmd->flags & 1) {
        float t = cmd->azimuth_time;
        CWind &wind = zdb::CWorld::m_world->m_Wind;

        wind.m_AzimuthGoal = cmd->azimuth;
        if (t > 0.01f) {
            wind.m_AzimuthGoalDelta = (wind.m_AzimuthGoal - wind.m_Azimuth) / t;
            wind.m_TickGoal = true;
            wind.m_TickAzimuth = true;
        } else
            wind.m_Azimuth = wind.m_AzimuthGoal;
    }
    if (cmd->flags & 2) {
        float t = cmd->zenith_time;
        CWind &wind = zdb::CWorld::m_world->m_Wind;

        wind.m_ZenithGoal = cmd->zenith;
        if (t > 0.01f) {
            wind.m_ZenithGoalDelta = (wind.m_ZenithGoal - wind.m_Zenith) / t;
            wind.m_TickGoal = true;
            wind.m_TickZenith = true;
        } else
            wind.m_Zenith = wind.m_ZenithGoal;
    }
    if (cmd->flags & 4) {
        float t = cmd->speed_time;
        CWind &wind = zdb::CWorld::m_world->m_Wind;

        wind.m_SpeedGoal = cmd->speed;
        if (t > 0.01f) {
            wind.m_SpeedGoalDelta = (wind.m_SpeedGoal - wind.m_Speed) / t;
            wind.m_TickGoal = true;
            wind.m_TickSpeed = true;
        } else
            wind.m_Speed = wind.m_SpeedGoal;
    }
    if (cmd->flags & 8)
        zdb::CWorld::m_world->m_Wind.SetAzimuthOffsetRange(cmd->min_azimuth_offset,
                                                           cmd->max_azimuth_offset);
    if (cmd->flags & 0x10)
        zdb::CWorld::m_world->m_Wind.m_AzimuthOffsetVelocity = cmd->azimuth_offset_velocity;
    if (cmd->flags & 0x20)
        zdb::CWorld::m_world->m_Wind.SetZenithOffsetRange(cmd->min_zenith_offset,
                                                           cmd->max_zenith_offset);
    if (cmd->flags & 0x40)
        zdb::CWorld::m_world->m_Wind.m_ZenithOffsetVelocity = cmd->zenith_offset_velocity;
    if (cmd->flags & 0x80)
        zdb::CWorld::m_world->m_Wind.SetSpeedOffsetRange(cmd->min_speed_offset,
                                                           cmd->max_speed_offset);
    if (cmd->flags & 0x100)
        zdb::CWorld::m_world->m_Wind.m_SpeedOffsetVelocity = cmd->speed_offset_velocity;
    return true;
}

/* a value kept below a bound, and above one: the selects the target makes (by value; the names
 * ours -- MSL's min and max return references, which select addresses) */
template <class T>
inline T WindMin(const T &a, const T &b)
{
    return b < a ? b : a;
}

template <class T>
inline T WindMax(const T &a, const T &b)
{
    return a < b ? b : a;
}

/* the gusts: each offset wandered at random by its rate and kept in its range; the wind vector from
 * the azimuth, the zenith (a speed below zero turning it about) and the speed (SOCOM 1's).
 * OPEN, 0x404 of 0x418: the target reads each offset afresh after every store to it (the +=, the
 * upper bound, the lower) where MW forwards the stored value here, and tests the lower bound as
 * `c.le.s off,min; bc1t` (this: c.lt.s min,off; bc1f); the frame has a saved s1 more (the wind's z
 * address, s0 + 8, held from the cosf on). Tried: the selects returning references (address
 * selects), `<=`/`>=`/negated forms of the lower bound */
void CWind::Tick(float dt)
{
    if (m_TickGoal)
        TickGoal(dt);
    m_AzimuthOffset += dt * (m_AzimuthOffsetVelocity * (2.0f * (4.656613e-10f * rand()) - 1.0f));
    m_AzimuthOffset = WindMin(m_MaxAzimuthOffset, m_AzimuthOffset);
    m_AzimuthOffset = WindMax(m_MinAzimuthOffset, m_AzimuthOffset);
    m_ZenithOffset += dt * (m_ZenithOffsetVelocity * (2.0f * (4.656613e-10f * rand()) - 1.0f));
    m_ZenithOffset = WindMin(m_MaxZenithOffset, m_ZenithOffset);
    m_ZenithOffset = WindMax(m_MinZenithOffset, m_ZenithOffset);
    m_SpeedOffset += dt * (m_SpeedOffsetVelocity * (2.0f * (4.656613e-10f * rand()) - 1.0f));
    m_SpeedOffset = WindMin(m_MaxSpeedOffset, m_SpeedOffset);
    m_SpeedOffset = WindMax(m_MinSpeedOffset, m_SpeedOffset);
    float speed = m_Speed + m_SpeedOffset;
    float azimuth = m_Azimuth + m_AzimuthOffset;
    float zenith = m_Zenith + m_ZenithOffset;

    if (speed < 0.0f) {
        speed = -speed;
        azimuth += 3.1415927f;
    }
    zenith = WindMin(90.0f, zenith);
    zenith = WindMax(-90.0f, zenith);
    if (azimuth < 0.0f) {
        while (azimuth < 0.0f)
            azimuth += 6.2831855f;
    } else if (azimuth >= 6.2831855f)
        while (azimuth >= 6.2831855f)
            azimuth -= 6.2831855f;
    m_Wind.x = sinf(azimuth);
    m_Wind.z = cosf(azimuth);
    float y = 0.011111111f * zenith;

    m_Wind.x = -m_Wind.x;
    m_Wind.y = 0.0f;
    m_Wind.z = -m_Wind.z;
    m_Wind.y = y;
    float scale = y < 0.0f ? 1.0f + y : 1.0f - y;

    m_Wind.x *= scale;
    m_Wind.z *= scale;
    m_Wind = (CPnt4D &)m_Wind * speed;
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
