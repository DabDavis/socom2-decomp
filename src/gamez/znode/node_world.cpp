/* node_world.cpp -- zdb::CWorld, the scene's root: its wind, its textured characters, its shadow,
 * light-map and landmark lists, its intersection queries and children (OVL1 0x31AF70-0x3209F3).
 * The unit name is SOCOM 1's (the press beta's CmdParseWind and ~CWorld assert node_world.cpp);
 * its order is SOCOM 1's, the wind first and the template instances last. SOCOM II opens it with
 * a REMOVE_DECALS command and the foliage sound's setter (CWorld::RemoveDecals, defined here, is
 * inlined into the command's tick, so they are this unit's), and adds a shadow caster list.
 * Defined in reverse of their image order (-inline deferred). */

#include <iostream>
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

/* SOCOM II's call on both decal pools after the recycle (not named yet) */
extern "C" void func_003BF050(zdb::CDecalPool *pool);

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

/* a light's direction or colour set and flagged to be sent (ours: the constructor sets each light
 * through one -- a vector copied into the parameter whole, then into the light word by word; the
 * constants stored straight; the flag after each) */
static inline void SetLightDir(CWorld *w, int i, CPnt4D d)
{
    w->m_gLight.dir[i] = d;
    w->m_gLightDirUpdate = 1;
}

static inline void SetLightDir(CWorld *w, int i, float x, float y, float z, float v)
{
    w->m_gLight.dir[i].x = x;
    w->m_gLight.dir[i].y = y;
    w->m_gLight.dir[i].z = z;
    w->m_gLight.dir[i].w = v;
    w->m_gLightDirUpdate = 1;
}

static inline void SetLightCol(CWorld *w, int i, float r, float g, float b, float a)
{
    w->m_gLight.col[i].x = r;
    w->m_gLight.col[i].y = g;
    w->m_gLight.col[i].z = b;
    w->m_gLight.col[i].w = a;
    w->m_gLightColUpdate = 1;
}

static inline void SetAmbient(CWorld *w, float r, float g, float b, float a)
{
    w->m_gLight.ambient.x = r;
    w->m_gLight.ambient.y = g;
    w->m_gLight.ambient.z = b;
    w->m_gLight.ambient.w = a;
    w->m_gLightColUpdate = 1;
}

/* SOCOM 1's (0x230030): the world made current, its scale set, its intersection made, a light from
 * above and the ambient set, the lens effects' colours, one shadow texture loaded at the textures'
 * base and shared by the four (SOCOM 1 loaded four), the shadow's direction and the defaults */
CWorld::CWorld(const char *name) : m_camera(0), m_diInt(0), m_cellData(0), m_maxOverlap(2)
{
    SetName(name);
    m_world = this;
    m_scale = 10.0f;
    m_invscale = 0.1f;
    m_type = 6;
    m_diInt = new DiIntersect(256);
    m_diIntDelayCnt = 0;
    CPnt4D d(0.5f, -0.8f, 0.5f, 1.0f);

    sceVu0Normalize((float *)&d, (float *)&d);
    SetLightDir(this, 0, d);
    SetLightDir(this, 1, 0.0f, 0.0f, 0.0f, 0.0f);
    SetLightDir(this, 2, 0.0f, 0.0f, 0.0f, 0.0f);
    SetLightCol(this, 0, 0.8f, 0.8f, 0.8f, 1.0f);
    SetLightCol(this, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    SetLightCol(this, 2, 0.0f, 0.0f, 0.0f, 1.0f);
    SetAmbient(this, 0.2f, 0.2f, 0.2f, 1.0f);
    m_night_mission = 0;
    m_lensfx_nvg.x = 0.2f;
    m_lensfx_nvg.y = 0.9f;
    m_lensfx_nvg.z = 0.2f;
    m_lensfx_nvg.w = 0.24f;
    m_lensfx_starlightscope.x = 0.3f;
    m_lensfx_starlightscope.y = 0.5f;
    m_lensfx_starlightscope.z = 0.95f;
    m_lensfx_starlightscope.w = 0.24f;
    unsigned int addr = zVid.textureBaseAddr;
    char buf[32];

    sprintf(buf, "ShadowX_%d", 0);
    m_shadowTex[0] = new CTexture(buf);
    m_shadowTex[0]->LoadGS(addr | 0x80000000, buf, 0, 256, 256, 2);
    m_shadowTex[1] = m_shadowTex[0];
    m_shadowTex[2] = m_shadowTex[0];
    m_shadowTex[3] = m_shadowTex[0];
    m_shadowTexH = 0;
    m_shadow_vector.x = 1.0f;
    m_shadow_vector.y = -1.0f;
    m_shadow_vector.z = 1.0f;
    m_shadow_weight = 0.25f;
    m_shadow_vector.Normalize();
    m_default_soiltype = 0;
    m_default_soiltype_name = 0;
    m_world_text_specs[0] = 86.0f;
    m_world_text_specs[1] = 63.0f;
    m_world_text_specs[2] = 27.0f;
    m_world_text_specs[3] = 128.0f;
    m_foliage_sound = 0;
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
    CGSTexBuffer *buf = CTexManager::m_texmanager.m_buffer[CTexManager::TexPhase[SHADOW][0]];

    buf->m_dyntex_list.Add(m_shadowTexH, true);
    buf->m_chainp = 0;
    return true;
}

/* a light map taken off the list (SOCOM 1's: std::remove inlined, its remove_copy out of line).
 * The end built once into a local (76(sp)) and handed to remove and to erase, the new end a local
 * (72): include/algorithm's remove joins its two results through one copy (the MSL headers
 * round 8) */
void CWorld::DeleteLightMap(CLightMap *light)
{
    CLightMapVector::iterator e = m_lightMaps.end();
    CLightMapVector::iterator i = std::remove(m_lightMaps.begin(), e, light);

    m_lightMaps.erase(i, e);
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

/* a node's box through one inline level more than CNode::GetBBox (ours, a stand-in): the set's
 * TestFOV and the shadow caster's constructor call GetBBox straight, with no one-level copy of its
 * body -- what a wrapper inlined one level gives, the recursive GetBBox under it called out of
 * line, where a direct call inlines GetBBox's body once (every other caller) */
static inline const CBBox *NodeBBox(CNode *node)
{
    return node->GetBBox();
}

/* a caster for the node: its box's size, and the box of its corners carried into the world by the
 * node's matrix (new in SOCOM II, the name ours).
 * OPEN, 0x1F0 of 0x228: the target calls CPnt3D()'s out-of-line copy (0x1FB3C0) for each corner
 * and GetBBox straight, which is what an inline helper inlined ONE level gives (its callees called,
 * as hud_linemap.cpp's GetCellCenter calls CPnt3D()) -- the corners, their transform and the box
 * likely an inline of the original's too tall to inline whole; the box is then built through a
 * temporary (the first corner copied whole into 0xD0, from there whole into the temporary's
 * minimum at 0x50 and field by field into its maximum at 0x5C, the temporary assigned to m_bbox
 * field by field: a CBBox made from one point, by value?), the minimum kept with `<=` and the
 * maximum with `<`. The world lane's round 21 (build/scratch/wld/v_sc.py): the construction LOOP is
 * a member array's -- `struct { const CBBox *m_box; CPnt3D m_pts[8]; }` built by a constructor
 * taking the node, `m_box(node->GetBBox())` in its mem-initializers (the recursive GetBBox cuts it
 * one level: GetBBox straight, then the loop calling CPnt3D(), then GetBBoxCorners) gives the
 * target's order at 0x230 -- the box pointer then stored in the struct (sp+112) where the target
 * keeps it in s3; a local array in a helper cut one level unrolls into eight calls instead (h1) */
CShadowCaster::CShadowCaster(CNode *node)
{
    m_node = node;
    node->GetBBox()->GetSize(&m_size);
    const CBBox *box = NodeBBox(m_node);
    CPnt3D pts[8];

    GetBBoxCorners(pts, box);
    m_node->m_matrix.Transform(pts, 8);
    CPnt3D p = pts[0];
    CBBox b;

    b.m_min = p;
    b.m_max = p;
    m_bbox = b;
    for (int i = 1; i < 8; i++) {
        m_bbox.m_min.x = m_bbox.m_min.x >= pts[i].x ? pts[i].x : m_bbox.m_min.x;
        m_bbox.m_min.y = m_bbox.m_min.y >= pts[i].y ? pts[i].y : m_bbox.m_min.y;
        m_bbox.m_min.z = m_bbox.m_min.z >= pts[i].z ? pts[i].z : m_bbox.m_min.z;
        m_bbox.m_max.x = m_bbox.m_max.x > pts[i].x ? m_bbox.m_max.x : pts[i].x;
        m_bbox.m_max.y = m_bbox.m_max.y > pts[i].y ? m_bbox.m_max.y : pts[i].y;
        m_bbox.m_max.z = m_bbox.m_max.z > pts[i].z ? m_bbox.m_max.z : pts[i].z;
    }
}

/* the caster's world-to-texture matrix: a camera at the node looking down its own -z with its y
 * up, then a scale from the box's size to the texture's half (a texel in from the edge) and a
 * half's offset -- CLightMap::SetTexture's projection (new in SOCOM II, the name ours). The two
 * scales are locals (the box's size is read ahead of the projection's stores, which would otherwise
 * reload it), and the camera matrix reaches the product through a pointer taken just ahead of it:
 * passed straight, its address is set up after the projection's (a1 before a2; the original's
 * spelling unknown, the pointer a stand-in that orders the set-up) */
void CShadowCaster::Update()
{
    CMatrix view;
    CMatrix proj;
    sceVu0FVECTOR zd = {0.0f, 0.0f, -1.0f, 0.0f};
    sceVu0FVECTOR yd = {0.0f, 1.0f, 0.0f, 0.0f};

    TransformVectors(&m_node->m_matrix, (CPnt3D *)yd, (CPnt3D *)yd, 1);
    TransformVectors(&m_node->m_matrix, (CPnt3D *)zd, (CPnt3D *)zd, 1);
    sceVu0CameraMatrix(view.m_matrix, m_node->m_matrix.m_matrix[3], zd, yd);
    float s = (1.0f - 1.0f / (int)m_texture->m_width) / 1.414f;
    float sx = s / m_size.x;
    float sy = s / m_size.y;

    proj(0, 0) = sx;
    proj(0, 1) = 0.0f;
    proj(0, 2) = 0.0f;
    proj(0, 3) = 0.0f;
    proj(1, 0) = 0.0f;
    proj(1, 1) = sy;
    proj(1, 2) = 0.0f;
    proj(1, 3) = 0.0f;
    proj(2, 0) = 0.0f;
    proj(2, 1) = 0.0f;
    proj(2, 2) = 1.0f;
    proj(2, 3) = 0.0f;
    proj(3, 0) = 0.5f;
    proj(3, 1) = 0.5f;
    proj(3, 2) = 0.0f;
    proj(3, 3) = 1.0f;
    CMatrix *cam = &view;

    sceVu0MulMatrix(m_matrix.m_matrix, proj.m_matrix, cam->m_matrix);
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
        (*it)->Update();
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
 * of another node gives back its count. The shadows' end built once into a local and handed to
 * remove_if and to erase, as DeleteLightMap's; the remove_if instance (0x320800) reads its
 * predicate in place (the MSL headers round 8: both byte-exact) */
int CWorld::DeleteChild(CNode *child)
{
    if (!child)
        return 0;
    if (child->m_socom2_a1_bits.bit5 == 1) {
        CRenderMapVector::iterator e = m_shadows.end();
        CRenderMapVector::iterator i = std::remove_if(m_shadows.begin(), e, delshadow(child));

        m_shadows.erase(i, e);
    }
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

/* every child taken off: the others first, then the cells, the list emptied (SOCOM 1's). Each end()
 * is computed from the load begin() made: the pod vector's end() through data() (include/vector
 * since the MSL headers' round 4) */
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
 * The target holds the local vector's &size_ in s0 and &data_ in s1 (SOCOM 1's the other way round):
 * the pod vector's end() through data() (include/vector since the MSL headers' round 4) */
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
                lights.insert(lights.begin(), light);
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
 * in x/z, their count given back (SOCOM 1's). The clear and the walk's first node both read the
 * sentinel through a reference (include/list's begin(), SOCOM II's): the target computes
 * &m_lights.end_.next_ once, before the clear, and reads the walk's first node through it after
 * the search; the iterator is assigned (a bitwise copy, then kept in memory) */
int CWorld::GenerateLightList()
{
    m_lights.clear();
    LightList_Search(this, m_lights);
    CLightList::iterator it, e = m_lights.end();

    for (it = m_lights.begin(); it != e; ++it) {
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
    m_grid.GetIntersect(m_diInt);
    hit = m_diInt->m_Cnt ? m_diInt->GetNearestIntersect() : 0;
    m_diInt->SetIntersectCharacters(was);
    return hit;
}

/* the nearest hit along a segment (SOCOM 1's) */
IntersectStruct *CWorld::GetIntersect(const CPnt3D *p0, const CPnt3D *p1)
{
    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    m_grid.GetIntersect(m_diInt);
    if (m_diInt->m_Cnt)
        return m_diInt->GetNearestIntersect();
    return 0;
}

/* every hit along a segment (SOCOM 1's) */
DiIntersect *CWorld::GetIntersects(const CPnt3D *p0, const CPnt3D *p1)
{
    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    m_grid.GetIntersect(m_diInt);
    return m_diInt;
}

/* every hit along a segment, characters hit as asked */
DiIntersect *CWorld::GetIntersects(const CPnt3D *p0, const CPnt3D *p1, bool characters)
{
    bool was = m_diInt->SetIntersectCharacters(!characters);

    m_diInt->m_Tail = *p0;
    m_diInt->m_Tip = *p1;
    m_grid.GetIntersect(m_diInt);
    m_diInt->SetIntersectCharacters(was);
    return m_diInt;
}

/* the highest ground under a point, characters hit as asked */
IntersectStruct *CWorld::GetAltitude(const CPnt3D *p, float alt, bool characters)
{
    bool was = m_diInt->SetIntersectCharacters(!characters);
    IntersectStruct *hit;

    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    m_grid.GetAltitude(m_diInt);
    hit = m_diInt->m_Cnt ? m_diInt->GetHighestAltitude(p, alt) : 0;
    m_diInt->SetIntersectCharacters(was);
    return hit;
}

/* the highest ground under a point (SOCOM 1's) */
IntersectStruct *CWorld::GetAltitude(const CPnt3D *p, float alt)
{
    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    m_grid.GetAltitude(m_diInt);
    if (m_diInt->m_Cnt)
        return m_diInt->GetHighestAltitude(p, alt);
    return 0;
}

/* every ground hit under a point, characters not hit, no node ignored (SOCOM 1's; SOCOM II takes
 * no height -- none of its five callers sets $f12, the SEAL lane's reading) */
DiIntersect *CWorld::GetAltitudes(const CPnt3D *p)
{
    bool was = m_diInt->m_IntersectCharacters;

    m_diInt->m_Type = 2;
    m_diInt->m_IntersectCharacters = false;
    m_diInt->m_Node = 0;
    m_diInt->m_Tail = CPnt3D(p->x, 0.0f, p->z);
    m_grid.GetAltitude(m_diInt);
    m_diInt->SetIntersectCharacters(was);
    return m_diInt;
}

/* a batch of intersect sets run through the world's grid (SOCOM 1's) */
int CWorld::GetIntersectBatch(DiIntersect **di, int n)
{
    return m_world->m_grid.GetIntersectBatch(di, n);
}


void CWorld::InitDescendantsDI()
{
    m_grid.InitDescendantsDI();
}

void CWorld::InitDescendantsVisuals()
{
    m_grid.InitDescendantsVisuals();
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
    di->SetMask(mask);
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
    di->SetMask(mask);
    di->m_Tip.x = radius;
    di->m_Tip.y = 0.0f;
    di->m_Tip.z = 0.0f;
    return i;
}

/* the light reaching a point, as a luminance: the ambient (unless flag 2), the three global colours
 * (unless 1) and the point's cell's active lights the radius reaches (unless 4) -- full within a
 * light's minimum range, falling off to its maximum -- summed and weighted 0.4901 red, 0.4657 green,
 * 0.0441 blue (new in SOCOM II between Segregate_CCells and RegProximityDelayed, which SOCOM 1 has
 * side by side; nothing in the image calls it or takes its address -- the name and the parameters'
 * order ours, after RegProximityDelayed's).
 * OPEN, 0x354 against 0x34C (written in round 25 of the world lane; build/scratch/wld/v_gli*.py):
 * the colour starts as a copy of CPnt3D::zero, the difference is `operator-`'s temporary copied out,
 * and the light walk's iterator is ASSIGNED from begin() (so it lives in its stack slot, copied in
 * through $f0, the frame then the target's: the colour at sp+184) -- all as the target. Two misses:
 * (1) MW rotates the light walk -- its test repeated as a guard ahead of the hoisted squared radius
 * and field addresses, where the target's preheader holds them and branches to the bottom test.
 * Tried: the loop without its init, as a while, `i++`, `continue`s for the two skips, the iterator
 * declared at the top, before the cell or after the squaring, the radius squared into a named local
 * before the walk (all 0x354), squared before the cell test (0x34C, 38 off: still guarded, the
 * multiply in a delay slot), squared in the body (rotation stops, but the preheader is laid out
 * after the return: 0x364), a do-while (0x344), the cell's lights an inline with an early return
 * (0x364). (2) The global colours' nine products are `mul.s d,k,col` here, `d,col,k` in the target,
 * whichever side the constant is written on -- also through a named factor, a reference to the colour,
 * the nine written out, `c = c + ...`, or an inline helper taking the factor */
float CWorld::GetLightIntensity(const CPnt3D *p, float radius, unsigned int flags)
{
    CPnt3D c = CPnt3D::zero;

    if (!(flags & 2)) {
        c.x += m_gLight.ambient.x;
        c.y += m_gLight.ambient.y;
        c.z += m_gLight.ambient.z;
    }
    if (!(flags & 1)) {
        for (int i = 0; i < 3; i++) {
            c.x += m_gLight.col[i].x * 0.999999f;
            c.y += m_gLight.col[i].y * 0.999999f;
            c.z += m_gLight.col[i].z * 0.999999f;
        }
    }
    if (!(flags & 4)) {
        CCell *cell = m_grid.GetCell(p->x, p->z);
        CLightList *lights = cell ? &cell->m_lightList : 0;

        if (lights && lights->size()) {
            CLightList::iterator i;

            radius *= radius;
            for (i = lights->begin(); i != lights->end(); ++i) {
                CLight *l = *i;

                if (l->m_active) {
                    CPnt3D d = l->m_WorldPosition - *p;
                    float dist = d.x * d.x + d.y * d.y + d.z * d.z;

                    if (dist < radius + l->m_maxRangeSq && dist < l->m_maxRangeSq) {
                        float minSq = l->m_minRange * l->m_minRange;
                        float k;

                        if (dist <= minSq)
                            k = 1.0f;
                        else
                            k = 1.0f / (l->m_maxRangeSq - minSq) * (l->m_maxRangeSq - dist);
                        c.x += l->m_diffuse.x * k;
                        c.y += l->m_diffuse.y * k;
                        c.z += l->m_diffuse.z * k;
                    }
                }
            }
        }
    }
    return 0.4901f * c.x + 0.4657f * c.y + 0.0441f * c.z;
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
    if (!m_TextureDecals.GetHandle(tex->m_texture))
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
 * ~CNodeVector a call (-1) here; it turns on the vector remodel. MEASURED (the world lane's round
 * 16, build/scratch/wld/podsub.py): with the pointer pod's destructor deleting its block directly
 * (`operator delete(data_)`, the pointer vector's destructor three high, not four) this source is
 * byte-exact -- the set read into a second local, `p`, which the target keeps in s0 across the
 * remove while `c`, whose address the remove's binder takes, stays in its stack home (one local,
 * reloaded after the call, is 0x14C). Not applied: the same header change loses six destructors
 * that need the deallocate level (README "Level 6, or 8?") */
void CWorld::RemoveEmptyTextureAssetCharacters()
{
    CTextureAssetCharacterList::iterator it = m_TextureAssetCharacters.begin(),
                                         e = m_TextureAssetCharacters.end();

    while (it != e) {
        CTextureAssetCharacters *c = *it;
        CTextureAssetCharacters *p = c;

        if (p->m_nodes.size() == 0) {
            m_TextureAssetCharacters.remove(c);
            delete p;
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
            (*it)->m_buffer->Load(false);
}

/* every character set's nodes let go of and the set deleted, the list emptied (SOCOM 1's).
 * OPEN, 0xF0 of 0x124: as RemoveEmptyTextureAssetCharacters, `delete c` inlines ~CNodeVector's four
 * this-tested levels in the target and calls it here (the vector remodel); FreeVector's
 * constructor, clear, swap and destructor calls and the list's clear already line up. MEASURED
 * (round 16): byte-exact as it stands once the pointer pod's destructor deletes its block directly
 * (the pointer vector's destructor three high) -- which also closes ClearGroups 0x3C2A10 and
 * vector<SKBCOMPLETE>'s resize and loses ~CNodeVector (database.cpp's copy), ~CCell, ~CHUD,
 * ~CVisual, CPipe::~CAlpha and ~CGSTexBuffer: level 6's split, so not applied */
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

/* every set's visible nodes counted: the first set in view returned, the common characters' set
 * (CHARSETCOMMON) given back apart, the first in view (SOCOM 1's, its render phase read ahead of
 * the count). The cursor is assigned from begin() (a temporary copied in, below the found
 * iterator's slot) and the end taken once, the found test re-deriving it */
CTextureAssetCharacterList::iterator CTextureAssetCharacterList::TestFOV(CCamera *cam,
                                                                        CTextureAssetCharacters **common)
{
    iterator it;
    iterator e = end();
    iterator found = e;

    *common = 0;
    for (it = begin(); it != e; ++it) {
        _RenderPhase phase = (*it)->m_lib->m_renderphase;

        if ((*it)->TestFOV(cam)) {
            if (phase == CHARSETCOMMON) {
                if (*common == 0)
                    *common = *it;
            } else if (found == end())
                found = it;
        }
    }
    return found;
}

/* the set's nodes in view counted: each active node's box, its eight corners carried into the
 * world by the node's matrix, tested against the camera's view, the node's in-view bit set from
 * the answer (SOCOM 1's; SOCOM II tests the box's corners in 4D after the node's transform). The
 * corners are a local of the function's own block, declared ahead of the loop, and set field by
 * field from the box's copy in GetBBoxCorners' order: the box declared after them takes the lower
 * frame slot (0xA0, the corners 0xC0); initialised in the loop after the box they sat below it (44
 * off), `c[i] = CPnt4D(...)` evaluates each constructor's arguments right to left (57 off) -- the
 * world lane's rounds 16 and 21 */
int CTextureAssetCharacters::TestFOV(CCamera *cam)
{
    CPnt4D c[8];

    m_visible = 0;
    for (CNodeVector::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        CNode *node = *it;

        if (node->m_active) {
            CBBox b = *NodeBBox(node);

            c[0].x = b.m_min.x;
            c[0].y = b.m_min.y;
            c[0].z = b.m_max.z;
            c[0].w = 1.0f;
            c[1].x = b.m_min.x;
            c[1].y = b.m_min.y;
            c[1].z = b.m_min.z;
            c[1].w = 1.0f;
            c[2].x = b.m_max.x;
            c[2].y = b.m_min.y;
            c[2].z = b.m_min.z;
            c[2].w = 1.0f;
            c[3].x = b.m_max.x;
            c[3].y = b.m_min.y;
            c[3].z = b.m_max.z;
            c[3].w = 1.0f;
            c[4].x = b.m_min.x;
            c[4].y = b.m_max.y;
            c[4].z = b.m_max.z;
            c[4].w = 1.0f;
            c[5].x = b.m_min.x;
            c[5].y = b.m_max.y;
            c[5].z = b.m_min.z;
            c[5].w = 1.0f;
            c[6].x = b.m_max.x;
            c[6].y = b.m_max.y;
            c[6].z = b.m_min.z;
            c[6].w = 1.0f;
            c[7].x = b.m_max.x;
            c[7].y = b.m_max.y;
            c[7].z = b.m_max.z;
            c[7].w = 1.0f;

            node->m_matrix.Transform(c, c, 8);
            if (cam->TestFOV((PNT4D *)c, 0, true) == ZVIS_FOV_ALL_OUT)
                node->m_socom2_a1_bits.bit4 = false;
            else
                node->m_socom2_a1_bits.bit4 = true;
            m_visible += node->m_socom2_a1_bits.bit4 && node->m_active;
        }
    }
    return m_visible;
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
        m_buffer = CTexManager::m_texmanager.m_buffer[CTexManager::m_texmanager.doAddBuffer(
            name, CTexManager::TexBufParams[0], CTexManager::TexBufParams[1])];
        if (dyntex)
            m_buffer->SetDynTexList(dyntex);
        else
            m_buffer->Add(lib, true);
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

/* the WIND command read (SOCOM 1's): each of AZIMUTH, ZENITH and SPEED a goal (the angles in
 * degrees, made radians) and its time, each gust's range and its rate from the _OFFSET arrays; a
 * flag for each setting given (the array tested by its type byte: zanim.h's _zrdr). Byte-exact (the
 * world lane round 23) */
_zanim_cmd_hdr *CWind::CmdParseWind(_zrdr *rdr)
{
    if (!rdr)
        return 0;
    _zanim_cmd_wind *cmd = (_zanim_cmd_wind *)_calloc(1, sizeof(_zanim_cmd_wind), 0, 0);

    cmd->data_size = sizeof(_zanim_cmd_wind);
    cmd->timeless = 1;
    _zrdr *t = zrdr_findtag(rdr, "AZIMUTH");

    if (t && t->type == 4) {
        cmd->flags |= 1;
        cmd->azimuth = t->Item(0)->Real();
        cmd->azimuth *= 0.017453292f;
        if (t->Count() >= 2)
            cmd->azimuth_time = t->Item(1)->Real();
        else
            cmd->azimuth_time = 0.0f;
    }
    t = zrdr_findtag(rdr, "ZENITH");
    if (t && t->type == 4) {
        cmd->flags |= 2;
        cmd->zenith = t->Item(0)->Real();
        cmd->zenith *= 0.017453292f;
        if (t->Count() >= 2)
            cmd->zenith_time = t->Item(1)->Real();
        else
            cmd->zenith_time = 0.0f;
    }
    t = zrdr_findtag(rdr, "SPEED");
    if (t && t->type == 4) {
        cmd->flags |= 4;
        cmd->speed = t->Item(0)->Real();
        if (t->Count() >= 2)
            cmd->speed_time = t->Item(1)->Real();
        else
            cmd->speed_time = 0.0f;
    }
    t = zrdr_findtag(rdr, "AZIMUTH_OFFSET");
    if (t && t->type == 4) {
        if (t->Count() >= 2) {
            cmd->flags |= 8;
            cmd->min_azimuth_offset = t->Item(0)->Real();
            cmd->max_azimuth_offset = t->Item(1)->Real();
        }
        if (t->Count() >= 3) {
            cmd->flags |= 0x10;
            cmd->azimuth_offset_velocity = t->Item(2)->Real();
        }
    }
    t = zrdr_findtag(rdr, "ZENITH_OFFSET");
    if (t && t->type == 4) {
        if (t->Count() >= 2) {
            cmd->flags |= 0x20;
            cmd->min_zenith_offset = t->Item(0)->Real();
            cmd->max_zenith_offset = t->Item(1)->Real();
        }
        if (t->Count() >= 3) {
            cmd->flags |= 0x40;
            cmd->zenith_offset_velocity = t->Item(2)->Real();
        }
    }
    t = zrdr_findtag(rdr, "SPEED_OFFSET");
    if (t && t->type == 4) {
        if (t->Count() >= 2) {
            cmd->flags |= 0x80;
            cmd->min_speed_offset = t->Item(0)->Real();
            cmd->max_speed_offset = t->Item(1)->Real();
        }
        if (t->Count() >= 3) {
            cmd->flags |= 0x100;
            cmd->speed_offset_velocity = t->Item(2)->Real();
        }
    }
    return cmd;
}

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

/* an offset wandered at random by its rate, kept below its upper bound and above its lower (ours:
 * CWind::Tick's three offsets, each through references -- MW reloads the offset after each store;
 * written in place it forwards the stored value, 0x404). `off > lo ? off : lo` is MW's `!(off <=
 * lo)`: `c.le.s off,lo; bc1t` to the bound's arm, the offset's arm first, as the target (and SOCOM
 * 1's) -- `off <= lo ? lo : off` puts the bound's arm first */
static inline void WindOffset(float &off, const float &vel, const float &lo, const float &hi, float dt)
{
    off += dt * (vel * (2.0f * (4.656613e-10f * rand()) - 1.0f));
    off = off < hi ? off : hi;
    off = off > lo ? off : lo;
}

/* the gusts: each offset wandered at random by its rate and kept in its range; the wind vector from
 * the azimuth, the zenith (a speed below zero turning it about) and the speed (SOCOM 1's).
 * Byte-exact (the world lane round 23; open since round 14): the zenith clamped by value (the
 * bounds lui immediates -- through `const T &` selects they were read from memory), the sum
 * declared azimuth, zenith, speed (their loads' registers), the wind's z through a reference local
 * (its address s0 + 8 in a saved register from the cosf on: s1, the frame's extra quad), y written
 * straight into the vector and the scale's test `y >= 0` read back from it (MW forwards the stored
 * value: a local y moves the compare up and takes other registers) */
void CWind::Tick(float dt)
{
    if (m_TickGoal)
        TickGoal(dt);
    WindOffset(m_AzimuthOffset, m_AzimuthOffsetVelocity, m_MinAzimuthOffset, m_MaxAzimuthOffset, dt);
    WindOffset(m_ZenithOffset, m_ZenithOffsetVelocity, m_MinZenithOffset, m_MaxZenithOffset, dt);
    WindOffset(m_SpeedOffset, m_SpeedOffsetVelocity, m_MinSpeedOffset, m_MaxSpeedOffset, dt);
    float azimuth = m_Azimuth + m_AzimuthOffset;
    float zenith = m_Zenith + m_ZenithOffset;
    float speed = m_Speed + m_SpeedOffset;

    if (speed < 0.0f) {
        speed = -speed;
        azimuth += 3.1415927f;
    }
    zenith = zenith < 90.0f ? zenith : 90.0f;
    zenith = zenith > -90.0f ? zenith : -90.0f;
    if (azimuth < 0.0f) {
        while (azimuth < 0.0f)
            azimuth += 6.2831855f;
    } else if (azimuth >= 6.2831855f)
        while (azimuth >= 6.2831855f)
            azimuth -= 6.2831855f;
    m_Wind.x = sinf(azimuth);
    float &z = m_Wind.z;
    z = cosf(azimuth);

    m_Wind.x = -m_Wind.x;
    m_Wind.y = 0.0f;
    z = -z;
    m_Wind.y = 0.011111111f * zenith;
    float scale = m_Wind.y >= 0.0f ? 1.0f - m_Wind.y : 1.0f + m_Wind.y;

    m_Wind.x *= scale;
    z *= scale;
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
