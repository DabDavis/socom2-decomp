/* node_main.cpp -- zdb::CNode, the scene graph's node: its place in the world, its bounds, its
 * children, its grid atoms (OVL1 to 0x31789F, after node_vector.cpp's; node_model.cpp follows). The unit name is
 * SOCOM 1's (CNode::SetName's allocations name it). Defined in reverse of their image order
 * (-inline deferred), as the map has them: a function this unit calls without inlining, though
 * small, is evidence of a unit boundary (CreateXRef's SetName and SetPosition were). */

#include <string.h>

#include "znode.h"
#include "../zanim/zanim.h"
#include "../zsys/zsys.h"
#include "../zvid/zvid.h"

/* the name a node without one shares (SOCOM 1's file static, 0x3DFD10 here) */
extern char *UNNAMED_NODE;

namespace zdb {

/* a node's visuals let go of (not named yet) */
extern "C" void func_00313620(CNode *node, int a);
/* the world told of a shadow-casting node: a new 0x70-byte object made from it, pushed on a list
 * at 0x580 (new in SOCOM II, not named yet; CWorld::AddChild calls it too) */
extern "C" void func_0031F500(CWorld *world, CNode *node);

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
        func_00313620(child, 0);
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
 * twice, as SetNodeEx's does).
 * OPEN (SetDynamic, 0x280 of 0x270): its walk declares the iterator, then assigns begin() to it
 * (the second copy calls the iterator's default constructor, 0x313490, and its assignment,
 * 0x313480) -- and still inlines begin() whole in its own body. That needs the vector base's
 * begin() to read data_ directly: with it SetDynamic matches byte for byte, but FindChild by
 * name loses the register holding &data_ that data()'s reference gives it. One model fits
 * either, none both yet. */
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


}
