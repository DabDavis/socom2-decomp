/* node_main.cpp -- zdb::CNode, the scene graph's node: its place in the world, its bounds, its
 * children, its grid atoms (OVL1 0x3134C0-0x31789F, after node_vector.cpp's; node_model.cpp
 * follows). The unit name is SOCOM 1's (CNode::SetName's allocations name it). Defined in reverse
 * of their image order (-inline deferred), as the map has them: a function this unit calls
 * without inlining, though small, is evidence of a unit boundary (CreateXRef's SetName and
 * SetPosition were), and so is a cluster of template instances and inline functions' copies,
 * which close a unit (this one's from 0x3176B0; node_vector.cpp's at 0x313450). */

#include <algorithm>
#include <string.h>

#include "znode.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include "../zvid/zvid.h"

/* the name a node without one shares (SOCOM 1's file static, 0x3DFD10 here) */
extern char *UNNAMED_NODE;
/* the name a node is left with as it goes (SOCOM 1's, the next word, 0x3DFD18 here) */
extern char *DELETED_NODE;

namespace zdb {

/* the unit's statics, CLight::m_tick_list and CNode::m_deferred_visuals, which its __sinit builds
 * in that order (0x4028D0), are not defined here yet: defined, this unit emits their destructors,
 * whose generated drafts match (0x317820, 0x3177C0) where these do not -- CTickLightList's vector
 * is constructed as a vector of pointers is and destroyed clearing its size, which no vector in
 * include/vector does, and the list's destructor calls the deleter's after two levels where
 * list<T *>'s inlines all three. With a vector of pointers in its place the initialiser matches */

/* a mesh's decals all freed (SOCOM 1's mesh took the view as other visuals do; not named yet) */
extern "C" void func_003B8440(CVisual *mesh);
/* the world-to-texture projectors (zmath_main.cpp: SOCOM 1's CalcWorldToUV pair, given the
 * texture's width where SOCOM 1 passed the texture) */
extern "C" void func_003079C0(const CPnt3D *eye, const CPnt3D *at, const CPnt3D *dir, float a, float b,
                              sceVu0FMATRIX out);
extern "C" void func_00307810(const CPnt3D *eye, const CPnt3D *dir, float a, float b, sceVu0FMATRIX out);
/* a decal onto a sub-mesh, through its mesh and its place there (new in SOCOM II; not named yet) */
extern "C" void func_003B8680(CVisual *mesh, CMatrix *mtw, int index, float size, CPnt3D *normal,
                              CTexHandle *tex, CMatrix *uv);
/* the world told of a shadow-casting node: a new 0x70-byte object made from it, pushed on a list
 * at 0x580 (new in SOCOM II, not named yet; CWorld::AddChild calls it too) */
extern "C" void func_0031F500(CWorld *world, CNode *node);
/* the visual effects' ticks (SOCOM 1's VisualEffectsTick calls CDecal's, CRipple's and CTextureFX's
 * in this order; SOCOM II's third runs on an object of its own, not placed) */
class CDecal {
public:
    static void Tick(float dt);
};
class CRipple {
public:
    static void Tick(float dt);
};
extern "C" void func_003C0120(void *fx, float dt);
/* a scrolling texture handed to the texture effects (SOCOM 1 took a CTextureFX from its pool and
 * initialised it in CNode::ScrollTexture; SOCOM II's call takes InitScrollTexture's arguments in
 * its order and an index, on the same object as the tick, and gives an index back -- not named
 * yet; the order is the one the target's argument set-up follows) */
extern "C" int func_003C0460(void *fx, float u, float v, bool b, CVisual *visual, CNode *node,
                             int index);
extern "C" char data_004B50D0[];

/* a copy of the node: _Copy into a new one */
CNode *CNode::Copy() const
{
    return _Copy(0);
}

/* a reference given back; never below none */
int CNode::Release()
{
    m_count--;
    if (m_count < 0)
        m_count = 0;
    return m_count;
}

/* a visual taken on, once: counted, listed, the node marked changed; a transparent one makes the
 * node transparent, a mesh (type 1) gives it a mesh, and it and its parents have visuals now.
 * The transparency test is a bitwise & of the two flag tests, both computed (SOCOM 1's too) */
bool CNode::AddVisual(CVisual *visual)
{
    bool added = false;

    if (visual && !m_visual.Exists(visual)) {
        visual->m_refs++;
        added = true;
        m_visual.Add(visual);
        m_socom2_a1 |= 2;
        if ((visual->m_flags & 1) & ((visual->m_flags & 0x100) == 0))
            m_transparent = 1;
        if (visual->m_type == 1)
            m_hasMesh = 1;
        m_hasVisuals = 1;
        SetParentHasVisuals();
    }
    return added;
}

/* a child taken in: a shadow caster told to the world, a character's common flag copied into a
 * cell, the child taken from its old parent and counted (not by a type-3 node), and put on the
 * list -- which grows by a hundred at a time once past a thousand. A child with visuals marks
 * the node as having them. */
void CNode::AddChild(CNode *child)
{
    if (!child)
        return;
    if (child->m_shadow == 1)
        func_0031F500(CWorld::m_world, child);
    if (child->GetType() == 7 && GetType() == 3)
        m_char_common = child->m_char_common != 0;
    if (child->m_parent == this)
        return;
    if (m_type != 3) {
        if (child->m_parent)
            child->m_parent->DeleteChild(child);
        child->m_parent = this;
        child->m_count++;
    }
    int n = m_child.size();
    if (n == m_child.capacity() && n > 1000)
        m_child.reserve(n + 100);
    m_child.push_back(child);
    if (child->m_hasVisuals)
        m_hasVisuals = 1;
    m_socom2_a1 |= 2;
}

/* a child let go of: its reference given back (a type-3 node's children are not counted) and
 * the child out of the list; its count after, or -1 */
int CNode::DeleteChild(CNode *child)
{
    int count = child->m_count;

    if (m_child.Exists(child)) {
        child->RemoveDecals(0);
        if (m_type == 3) {
            m_child.Remove(child);
            m_socom2_a1 |= 2;
            count = -1;
        } else {
            count = child->Release();
            m_child.Remove(child);
            child->m_parent = 0;
            m_socom2_a1 |= 2;
        }
    }
    return count;
}

/* a child by name (its length first, then its text), among the children and then, when deep,
 * below each. The second walk's iterator is its own, initialised from begin(): the address of
 * the list's storage it reloads begin through is computed once, before the first walk (an
 * iterator assigned from begin() leaves data() a call) */
CNode *CNode::FindChild(const char *name, bool deep)
{
    if (!name)
        return 0;
    CNode *found = 0;
    size_t len = strlen(name);
    CNodeVector::iterator it = m_child.begin();

    for (; !found && it != m_child.end(); ++it) {
        CNode *child = *it;

        if (strlen(child->m_name) == len && !strcmp(child->m_name, name))
            found = child;
    }
    if (!found && deep) {
        for (CNodeVector::iterator i2 = m_child.begin(); !found && i2 != m_child.end(); ++i2)
            found = (*i2)->FindChild(name, deep);
    }
    return found;
}

/* a child by name from the given index on (a negative one counting as 0), and where it stood: the
 * index after it. Then, when deep, the same search below every child, all of them -- the walk
 * does not stop at a hit, and what it returns is the last child's answer. */
CNode *CNode::FindChild(const char *name, int start, int *index, bool deep)
{
    CNode *found = 0;

    if (name) {
        CNode *child;
        unsigned int n = m_child.size();
        unsigned int i = start > 0 ? start : 0;
        size_t len = strlen(name);

        for (; !found && i < n; i++) {
            child = m_child[i];

            if (strlen(child->m_name) == len && !strcmp(child->m_name, name)) {
                found = child;
                *index = i + 1;
            }
        }
        if (!found && deep) {
            for (i = 0; i < n; i++)
                found = m_child[i]->FindChild(name, start, index, deep);
        }
    }
    return found;
}

/* a node among the children and then, when deep, below each. The first walk's `!found` is always
 * true (a hit returns) and folds away, but not before MW has laid the loop out as a compound
 * test's (a branch to the test at the bottom, where a lone compare is tested once on entry too) */
CNode *CNode::FindChild(CNode *node, bool deep)
{
    if (!node)
        return 0;
    CNode *found = 0;
    CNodeVector::iterator it = m_child.begin();

    for (; !found && it != m_child.end(); ++it)
        if (*it == node)
            return *it;
    if (!found && deep) {
        for (CNodeVector::iterator i2 = m_child.begin(); !found && i2 != m_child.end(); ++i2)
            found = (*i2)->FindChild(node, deep);
    }
    return found;
}

/* an extension attached to the node and, when told to, to every node below it. The recursion
 * inlines itself twice, and the copies call the child vector's pieces out of line (begin, end,
 * the iterator's ==, !=, *, ++ and copy) where MW's inline budget runs out -- the walk's ++ is
 * the postfix one, its copy and the prefix ++ called in the second copy (include/vector) */
void CNode::SetNodeEx(CNodeEx *ex, bool recurse)
{
    m_nodeEx = ex;
    if (recurse) {
        for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); it++)
            (*it)->SetNodeEx(ex, recurse);
    }
}

/* a position set: the translation, the grid told, the matrix marked changed */
void CNode::SetPosition(float x, float y, float z)
{
    m_matrix.m_matrix[3][0] = x;
    m_matrix.m_matrix[3][1] = y;
    m_matrix.m_matrix[3][2] = z;
    UpdateGrid();
    m_socom2_a1 |= 2;
}

/* the squared radius of the bounds about the node's origin: the further corner's */
float CNode::GetRadiusSq()
{
    float a = m_bbox.m_min.x * m_bbox.m_min.x + m_bbox.m_min.y * m_bbox.m_min.y + m_bbox.m_min.z * m_bbox.m_min.z;
    float b = m_bbox.m_max.x * m_bbox.m_max.x + m_bbox.m_max.y * m_bbox.m_max.y + m_bbox.m_max.z * m_bbox.m_max.z;

    return MAX(a, b);
}

float CNode::GetRadius()
{
    return sqrtf(GetRadiusSq());
}

/* whether the node was drawn this frame, or (asked for two frames, as every caller does) the
 * frame before */
bool CNode::Rendered(int frames)
{
    if (m_frameRendered == zVid.frameNumber || (frames == 2 && m_frameRendered == zVid.frameNumber - 1))
        return true;
    return false;
}

/* a light copied into another node (a new light when none): the node's part, then the light's --
 * the ranges, the colour, the opacity; not yet applied to anything */
CNode *CLight::_Copy(CNode *to) const
{
    CLight *light;

    if (!to) {
        light = new CLight;
        to = light;
    } else
        light = dynamic_cast<CLight *>(to);
    CNode::_Copy(to);
    light->m_minRange = m_minRange;
    light->m_maxRange = m_maxRange;
    light->m_maxRangeSq = m_maxRangeSq;
    light->m_invMaxRange = m_invMaxRange;
    light->m_invDeltaRange = m_invDeltaRange;
    light->m_diffuse = m_diffuse;
    light->m_opacity = m_opacity;
    light->m_appliedToNode = 0;
    return to;
}

/* a light: type 8, no range, a mid grey, half opaque (64 of 128) */
CLight::CLight()
{
    m_type = 8;
    m_maxRangeSq = 0.0f;
    m_minRange = 0.0f;
    m_diffuse.x = 128.0f;
    m_diffuse.y = 128.0f;
    m_diffuse.z = 128.0f;
    m_opacity = 64.0f;
    m_appliedToNode = 0;
}

CLight::~CLight()
{
}

/* a light's bounds: a cube about it, its half side the larger of its two ranges */
CBBox *CLight::UpdateBBox(bool up)
{
    float r = MAX(m_maxRange, m_minRange);

    m_bbox.m_max.x = r;
    m_bbox.m_max.y = r;
    m_bbox.m_max.z = r;
    m_bbox.m_min.x = -r;
    m_bbox.m_min.y = -r;
    m_bbox.m_min.z = -r;
    return (CBBox *)GetBBox();
}

CLightMap::~CLightMap()
{
}

/* the texture a light map projects, and its world-to-texture matrix: a camera at the light looking
 * down -y, then a scale from its range to the texture's half (a texel in from the edge) and a
 * half's offset. The width converts as an int; the range comes through an inline accessor (it is
 * loaded ahead of the 2.0, whose register it would otherwise take) */
void CLightMap::SetTexture(CTexture *tex)
{
    m_pTex = tex;
    CMatrix view;
    sceVu0FVECTOR yd = {0.0f, 0.0f, -1.0f, 1.0f};
    sceVu0FVECTOR zd = {0.0f, -1.0f, 0.0f, 1.0f};
    sceVu0FVECTOR p = {0};

    sceVu0CameraMatrix(view.m_matrix, p, zd, yd);
    CMatrix proj;
    float s = (1.0f - 1.0f / (int)m_pTex->m_width) / 1.414f;

    s /= 2.0f * GetMaxRange();

    proj(0, 0) = s;
    proj(0, 1) = 0.0f;
    proj(0, 2) = 0.0f;
    proj(0, 3) = 0.0f;
    proj(1, 0) = 0.0f;
    proj(1, 1) = s;
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
    sceVu0MulMatrix(m_worldToUV.m_matrix, proj.m_matrix, view.m_matrix);
}

/* the visuals let go of while drawn, deleted now: the deferred list emptied from its end (SOCOM
 * II's; the name is ours) */
void CNode::DeleteDeferredVisuals()
{
    while (m_deferred_visuals.size()) {
        CVisual *visual = m_deferred_visuals.back();

        m_deferred_visuals.pop_back();
        delete visual;
    }
}

/* a node destroyed: its visuals and DIs let go of, its children deleted, its atoms, model and
 * names given up, and it left out of the universe and the count (SOCOM 1's, but the visuals and
 * DIs are calls now, and the world's texture characters are not consulted) */
CNode::~CNode()
{
    DeleteVisuals();
    if (m_hasDI)
        DeleteDI();
    DeleteChildren();
    FreeAtom();
    if (m_model)
        m_model->Release(this);
    if (m_modelname)
        _free(m_modelname, 0, 0);
    m_modelname = 0;
    SetName(0);
    m_name = DELETED_NODE;
    if (NodeUniverse)
        NodeUniverse->RemoveNode(this);
    numNodes--;
}

/* the node's name, its own copy (or the shared default when given none); a name of its own freed */
void CNode::SetName(const char *name)
{
    if (m_name && m_name != UNNAMED_NODE)
        _free(m_name, 0, 0);
    m_name = name ? _strdup(name, 0, 0) : UNNAMED_NODE;
}

/* every child let go of, each deleted that nothing else holds */
void CNode::DeleteChildren()
{
    while (m_child.size()) {
        CNode *child = m_child[0];

        if (!DeleteChild(child))
            delete child;
    }
}

/* the atom pointers sized for a grid's cells, x by z (one more each way for a dynamic, lit
 * node) */
void CNode::AllocateAtomBasePtrs(int x, int z)
{
    int n = m_dynamic_motion && m_dynamic_light ? (x + 1) * (z + 1) : x * z;
    if (!m_Atom) {
        m_AtomAlloc = n;
        m_Atom = (CGridAtom **)_malloc(m_AtomAlloc * 4, 0, 0);
    } else if (m_AtomAlloc < n) {
        m_AtomAlloc = n;
        m_Atom = (CGridAtom **)_realloc(m_Atom, n * 4, 0, 0);
    }
}

/* the atom pointers cut to those in use */
void CNode::ShrinkAtomBasePtrs()
{
    if (m_AtomCnt < m_AtomAlloc) {
        CGridAtom **atoms = (CGridAtom **)_malloc(m_AtomCnt * 4, 0, 0);

        memcpy(atoms, m_Atom, m_AtomCnt * 4);
        _free(m_Atom, 0, 0);
        m_Atom = atoms;
        m_AtomAlloc = m_AtomCnt;
    }
}

/* the grid atoms the node is in: all freed (the pointer zeroed twice over), one added, the n-th */
void CNode::FreeAtom()
{
    if (m_Atom)
        _free(m_Atom, 0, 0);
    m_Atom = 0;
    m_Atom = 0;
    m_AtomAlloc = 0;
}

void CNode::InsertAtom(CGridAtom *atom)
{
    m_Atom[m_AtomCnt++] = atom;
}

CGridAtom *CNode::GetAtom(int i)
{
    if (i >= m_AtomAlloc)
        return 0;
    return m_Atom[i];
}

/* moving and lit as a dynamic node, or not; below too when told to (each recursion inlines itself
 * twice, as SetNodeEx's does). SetDynamic's walk declares the iterator, then assigns begin() to
 * it: the second copy calls the iterator's default constructor (0x313490) and its assignment
 * (0x313480), the outer copy inlines begin() whole -- three high only so (include/vector) */
void CNode::SetDynamic(bool on, bool recurse)
{
    m_dynamic_motion = on;
    m_dynamic_light = on;
    if (recurse) {
        CNodeVector::iterator it;

        for (it = m_child.begin(); it != m_child.end(); it++)
            (*it)->SetDynamic(on, recurse);
    }
}

void CNode::SetDynamicMotion(bool on, bool recurse)
{
    m_dynamic_motion = on;
    if (recurse) {
        for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); it++)
            (*it)->SetDynamicMotion(on, recurse);
    }
}

void CNode::SetDynamicLight(bool on, bool recurse)
{
    m_dynamic_light = on;
    if (recurse) {
        for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); it++)
            (*it)->SetDynamicLight(on, recurse);
    }
}

/* bounds borrowed from the parent, or not, through the whole tree (SetDynamic's walk) */
void CNode::SetUseParentBBoxTree(bool on)
{
    m_use_parent_bbox = on;
    CNodeVector::iterator it;

    for (it = m_child.begin(); it != m_child.end(); it++)
        (*it)->SetUseParentBBoxTree(on);
}

/* decals let onto the node's visuals, or not, through the whole tree (the walk over the visuals
 * re-reads the vector through a reference: &m_visual stays in a register of its own). Pinned,
 * as is SetApplyMultipassEffects: twelve statements and every piece inlinable, so MW would
 * inline the recursion twice, where the target calls itself from the outer copy -- SOCOM 1's
 * (the beta's too, no asserts in it) inlined it once */
#pragma auto_inline off
void CNode::SetApplyDecals(bool on)
{
    if (m_visual.size()) {
        CVisualVector &visuals = m_visual;

        for (CVisualVector::iterator v = visuals.begin(); v != visuals.end(); ++v)
            (*v)->m_bits.m_decals = on;
    }
    for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); ++it)
        (*it)->SetApplyDecals(on);
}

/* multipass effects let onto the node's visuals, or not, through the whole tree */
void CNode::SetApplyMultipassEffects(bool on)
{
    if (m_visual.size()) {
        CVisualVector &visuals = m_visual;

        for (CVisualVector::iterator v = visuals.begin(); v != visuals.end(); ++v)
            (*v)->m_bits.m_multipass = on;
    }
    for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); ++it)
        (*it)->SetApplyMultipassEffects(on);
}

#pragma auto_inline reset

/* clutter, or not, through the whole tree: lit as a dynamic node (only the node itself told) */
void CNode::SetApplyClutterFlag(bool on)
{
    m_clutter = on;
    SetDynamicLight(on, false);
    for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); ++it)
        (*it)->SetApplyClutterFlag(on);
}

/* room for n more DIs */
void CNode::ReserveDI(int n)
{
    m_di.reserve(m_di.size() + n);
}

/* a DI taken on, once: counted, listed, the node marked changed and holding collision data */
bool CNode::AddDI(CDI *di)
{
    bool added = false;

    if (di && !m_di.Exists(di)) {
        di->m_refcount++;
        added = true;
        m_di.Add(di);
        m_socom2_a1 |= 2;
        m_hasDI = 1;
    }
    return added;
}

/* every visual let go of, each deleted that nothing else holds -- unless the node was drawn this
 * frame, when it waits on the deferred list (once) for DeleteDeferredVisuals; then the list
 * emptied, the node marked changed and holding no mesh.
 * OPEN (0x1A4 of 0x1A8): the final erase's end is the target's begin load (through &data_,
 * kept since entry) plus the size the loop's last test scaled, where here it is the loop's whole
 * end; and the insert's temporaries take the stack slots below the erase's there, above them
 * here. Not moved by begin() + size() for the end */
void CNode::DeleteVisuals()
{
    bool drawn = m_frameRendered == zVid.frameNumber;

    for (CVisualVector::iterator it = m_visual.begin(); it != m_visual.end(); ++it) {
        CVisual *visual = *it;

        if (!visual->Release()) {
            if (!drawn)
                delete visual;
            else if (std::find(m_deferred_visuals.begin(), m_deferred_visuals.end(), visual) ==
                     m_deferred_visuals.end())
                m_deferred_visuals.push_back(visual);
        }
    }
    m_visual.erase(m_visual.begin(), m_visual.end());
    m_socom2_a1 |= 2;
    m_hasMesh = 0;
}

/* every DI let go of, each deleted that nothing else holds, the list emptied and the node marked
 * changed (SOCOM 1's destructor inlined it, so no name survives: ours). Pinned: SOCOM II's
 * destructor calls it, where MW would inline it */
#pragma auto_inline off
void CNode::DeleteDI()
{
    for (CDIVector::iterator it = m_di.begin(); it != m_di.end(); ++it) {
        CDI *di = *it;

        if (!di->Release())
            delete di;
    }
    m_di.clear();
    m_socom2_a1 |= 2;
}

#pragma auto_inline reset

/* a decal projected onto the node's visuals that take decals (0x10000), and below unless one of
 * them is a sub-mesh: the projection from the texture's width, the world matrix built only once
 * a visual needs it (SOCOM 1 built it first), a sub-mesh handed to its mesh.
 * OPEN (2 of 0x278): the sub-mesh call loads its mesh (0x6c) before its index (0x68), the target
 * the index first -- not moved by locals for either, the call as the mesh's member, an inline
 * wrapper, accessors, or reading either through the iterator */
void CNode::ApplyDecal(CNode *node, float size, CPnt3D *pos, CPnt3D *dir, CPnt3D *normal,
                       CTexHandle *tex, float depth, CPnt3D *up)
{
    CSubMesh *submesh = 0;

    if (m_hasVisuals) {
        CMatrix uv;
        CMatrix mtw;
        bool build = true;

        if (up)
            func_003079C0(pos, dir, up, (int)tex->m_texture->m_width, depth, uv);
        else
            func_00307810(pos, dir, (int)tex->m_texture->m_width, depth, uv);
        for (CVisualVector::iterator it = m_visual.begin(); it != m_visual.end(); ++it) {
            if ((*it)->m_flags & 0x10000) {
                if ((*it)->GetType() == 2) {
                    submesh = (CSubMesh *)*it;
                    func_003B8680(submesh->m_mesh, &mtw, submesh->m_index, size, normal, tex, &uv);
                } else {
                    if (build) {
                        BuildMTW(mtw);
                        build = false;
                    }
                    (*it)->ApplyDecalRender(&mtw, node, size, pos, normal, tex, &uv);
                }
            }
        }
    }
    if (!submesh && m_child.size()) {
        for (CNodeVector::iterator i = m_child.begin(), end = m_child.end(); i != end; ++i)
            (*i)->ApplyDecal(node, size, pos, dir, normal, tex, depth, up);
    }
}

/* the scene's visual effects ticked: decals, ripples, texture effects */
int CNode::VisualEffectsTick(float dt, void *data)
{
    CDecal::Tick(dt);
    CRipple::Tick(dt);
    func_003C0120(data_004B50D0, dt);
    return 0;
}

/* the matrices of a mesh's sub-meshes, through the tree: each active node's own pushed on the
 * stack over its parent's, a sub-mesh's copied into its mesh's palette (by the uncached
 * address), a mesh starting a palette of its own from the unit matrix; only children holding a
 * mesh or a sub-mesh are walked (SOCOM II's, for decals on skinned meshes; the name is ours).
 * The node's matrix is popped only if there is one -- the test of the node's own address -- and
 * the stack is a function static (the guard, the object and its destructor's record are this
 * unit's own .bss, which scdiff cannot check by content) */
void CNode::BuildMatrixPalette(CMesh *mesh)
{
    static CStack stack;

    if (!m_active)
        return;
    CMesh *own = GetMesh();
    CMatrix *m = stack.Multiply(GetMatrix(), true);

    if (own) {
        mesh = own;
        stack.Push(&CMatrix::identity);
    }
    CSubMesh *sub = GetSubMesh();

    if (sub)
        sceVu0CopyMatrix((float (*)[4])((unsigned int)mesh->m_palette[sub->m_index] | 0x20000000),
                         m->m_matrix);
    CNodeVector &children = m_child;

    for (CNodeVector::iterator it = children.begin(); it != children.end(); ++it) {
        CNode *child = *it;

        if (child->GetSubMesh() || child->GetMesh())
            child->BuildMatrixPalette(mesh);
    }
    if (GetMatrix())
        stack.Pop();
    if (own)
        stack.Pop();
}

/* the decals on the node's visuals and below let go of: a mesh frees all of its own, any other
 * visual those of one view -- an instance's own view unless told -1 (SOCOM 1's took no view and
 * passed the node's every time) */
void CNode::RemoveDecals(unsigned int vid)
{
    if (m_visual.size()) {
        if (vid != -1 && GetType() == 2)
            vid = m_vid;
        CVisualVector &visuals = m_visual;

        for (CVisualVector::iterator it = visuals.begin(); it != visuals.end(); ++it) {
            CVisual *visual = *it;

            if (visual->GetType() == 1)
                func_003B8440(visual);
            else
                visual->DereferenceDecals(vid);
        }
    }
    for (CNodeVector::iterator it = m_child.begin(); it != m_child.end(); ++it)
        (*it)->RemoveDecals(vid);
}

/* a scrolling texture started on every visual below -- handed to the texture effects with an
 * index new since SOCOM 1 (every caller passes -1), the index each hand-over gives back passed on
 * to the next and returned (so MW keeps it in v0 throughout); a mesh among a node's own visuals
 * stops the whole walk there, as in SOCOM 1. The children's end is read once */
int CNode::ScrollTexture(CNode *node, float u, float v, bool b, int index)
{
    for (CVisualVector::iterator it = m_visual.begin(); it != m_visual.end(); ++it) {
        CVisual *visual = *it;

        if (visual->GetType() == 1)
            return index;
        index = func_003C0460(data_004B50D0, u, v, b, visual, node, index);
    }
    if (m_child.size()) {
        for (CNodeVector::iterator i = m_child.begin(), end = m_child.end(); i != end; ++i)
            index = (*i)->ScrollTexture(node, u, v, b, index);
    }
    return index;
}

}
