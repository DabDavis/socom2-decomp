/* grid_clutter.cpp -- the grid's clutter: the props scattered on the cells (CClutter, CClutterList)
 * and their animation by effectors (CClutterAnimManager, CClutterAnim) (OVL1 0x2D7EA0-0x2D9CBF).
 * The unit is SOCOM 1's (its static initialiser, __sinit_grid_clutter.cpp, registers CStack's
 * destructor for its own matrix stack, after grid_di.cpp's); SOCOM 1's order runs from
 * CClutterAnimManager::UnRegisterEffector to CClutterList::Add, CInput's unit after it. Defined in
 * reverse of their image order (-inline deferred).
 * Written so far: GetTranslate, the manager's list walks and its destructor. The pointer-list
 * walks run on include/list's begin() and clear(): SOCOM II's begin() reads the first node through
 * a reference to the sentinel, so &end_.next_ is computed once and shared by the walk and the clear
 * after it (the target keeps it in the lowest saved register) -- znode.h's stand-ins for it
 * (ListBegin, ListEnd, ListClear) retired, every walk alike through the header. */

#include <algorithm>
#include <iostream>

#include "../zanim/zanim.h"
#include "../zcamera/zcamera.h"
#include "../zvid/zvid.h"
#include "znode.h"

/* the unit's matrix stack (0x44F0F0): __sinit_grid_clutter.cpp 0x401CF0 builds it after <iostream>'s
 * pair -- CStack(false), the owner byte cleared and nothing pushed, as grid_di.cpp's -- and registers
 * ~CStack 0x2D4FE0. CClutter::Render (written in the core lane's round 17), CClutterAnimManager() and
 * ClutterTick read it.
 * SOCOM 1 calls it `stack`, as grid_di.cpp's: MW names a file static by its own name, and scdiff
 * qualifies an MW object's statics as NAME@UNIT (the GCC lane's round 25a), so the map's data row
 * is stack@grid_clutter, grid_di.cpp's the bare `stack` */
static CStack stack(false);

extern CSched_Manager zTaskScheduler;

/* the render pipe (zrndr_pipe.cpp's CPipe, 0x720 bytes): only what a prop's draw reaches -- the
 * camera, the node being drawn and the two draws */
class CPipe {
public:
    void RenderMesh(zdb::CNode *node, zdb::CMesh *mesh, CMatrix *matrix, zdb::tag_ZVIS_FOV fov);
    void RenderNode(zdb::CNode *node, zdb::tag_ZVIS_FOV fov);

    unsigned char _0[0x628];
    zdb::CCamera *m_camera;   // 0x628
    zdb::CCell *m_cell;   // 0x62c
    zdb::CNode *m_node;   // 0x630
};

extern CPipe thePipe;   // 0x4887C0

/* a packed colour as a point (vis_data.cpp's, 0x3BA300) */
CPnt3D ColConvTo3D(unsigned int col);

namespace zdb {

/* two props traded (MSL's swap through references, written here: MW inlines no template
 * function; the name is ours) */
inline void ClutterSwap(CClutter *&a, CClutter *&b)
{
    CClutter *t = a;

    a = b;
    b = t;
}

/* one prop inserted before position, through the pointer vector's pod insert (MSL's
 * insert(iterator, const T &), which include/vector lacks: the position a class iterator passed by
 * value, so copied, then its pointer handed on; the name is ours) */
inline void ClutterInsert(CClutterList *list, CClutterList::iterator pos, CClutter *const &x)
{
    list->std::__vector_pod<unsigned int, std::allocator<unsigned int> >::insert(
        (unsigned int *)pos.m_p, 1, (const unsigned int &)x);
}

/* the visuals' added colour set, by value (ours: the two Renders' -- the call's result in a local, copied
 * as a block into the parameter, then member by member into the static, its two temporaries laid
 * out as the target's) */
inline void SetAddColor(CPnt3D c)
{
    CVisual::m_addColor = c;
}

/* a prop listed: the list's repel flag and farthest fade kept up, room made ten at a time, the prop
 * held once more (refused at fifteen holders), put at the end when its node has the flag, else
 * after the props put in front so far; then moved down past every fading prop that fades nearer
 * (SOCOM 1's; the holders and the front insertion SOCOM II's).
 * OPEN, 13 instructions (the right size), registers only: in the front insertion the target
 * loads the index into a1 and its increment into t0, and in the sort keeps &data[i] in a1 and the
 * prop in a2 -- here each pair the other way round (both the first-made temporary in the lower
 * register there). The swap traded as words (a typed view, or a volatile read) is what makes MW
 * reload the element as the target does; a swap of the CClutter pointers reuses the tested one.
 * Tried: the insert as a member, through begin(), the index taken first; the counter's types and
 * declaration places; a local or a reference for the tested prop */
bool CClutterList::Add(CClutter *clutter)
{
    if (size_ == 0) {
        m_ContainsRepelClutter = 0;
        m_max_farFadeSq = 0.0f;
    }
    if (clutter->m_quat)
        m_ContainsRepelClutter = 1;
    if (clutter->m_bit0 && clutter->m_farFadeSq > m_max_farFadeSq)
        m_max_farFadeSq = clutter->m_farFadeSq;
    if (size_ == capacity_)
        reserve(size_ + 10);
    if (!clutter->AddHolder())
        return false;
    if (clutter->m_node->m_reflective)
        push_back(clutter);
    else
        ClutterInsert(this, iterator(&(*this)[m_socom2_14++]), clutter);
    if (size() > 1) {
        for (int i = size() - 2; i >= 0; i--) {
            if ((*this)[i]->m_bit0) {
                if (!clutter->m_bit0 || clutter->m_farFadeSq > (*this)[i]->m_farFadeSq)
                    std::swap((unsigned int &)(*this)[i], (unsigned int &)(*this)[i + 1]);
                else
                    return true;
            }
        }
    }
    return true;
}

/* a point set from three values (the arguments evaluated right to left: z, y and x loaded before
 * the stores; the name is ours) */
inline void ClutterPntSet(float *p, float x, float y, float z)
{
    p[0] = x;
    p[1] = y;
    p[2] = z;
}

/* a prop let go of by a list: its holder count dropped, the prop deleted at none (the function
 * object ours) */
struct CClutterRelease {
    void operator()(CClutter *c) const
    {
        c->m_count--;
        if (c->m_count == 0)
            delete c;
    }
};

/* the cell's props drawn from the camera's place, the visuals' local lights off meanwhile: the
 * effectors near the cell asked first when the list holds a repelling prop; a list split at its
 * word at 0x14 (the flag at 0x19) draws one part a frame, alternating; the fading props sit first,
 * so the walk stops at the first one past the list's farthest fade, or past its own -- the cell's
 * distance from the camera (its middle at the camera's height, each of x and z short by the
 * cell's half-diagonal, scaled by the camera's zoomed range) measured once (SOCOM 1's, 0x2190C0:
 * SOCOM II reads the camera's world place, splits the list and scales by the zoom; written in the
 * core lane's round 17). The camera declared after the walk's locals, the prop among them (the
 * target numbers it last of the named locals: s1); the walk's start a nested ternary (each test's
 * zero in its branch's delay slot); midDone set ahead of the effectors' test (its delay slot) */
void CClutterList::Render(CCell *cell)
{
    if (!size())
        return;
    bool midDone = false;
    bool testDist;
    int i;
    int n;
    CClutter *c;
    CCamera *cam = thePipe.m_camera;
    bool lights = CVisual::m_applyLocalLights;

    CVisual::m_applyLocalLights = false;
    CPnt3D mid;
    CPnt3D camPos;

    camPos = cam->m_worldPos;
    testDist = true;

    if (m_ContainsRepelClutter) {
        cell->GetMidPoint(&mid);
        midDone = true;
        CWorld::m_world->m_grid.m_ClutterAnimManager->TestCellMid(&mid, &camPos);
    }
    i = m_socom2_19 ? (m_socom2_18 ? m_socom2_14 : 0) : 0;
    n = m_socom2_19 ? (m_socom2_18 ? size() : m_socom2_14) : size();
    float distSq;

    m_socom2_18 = !m_socom2_18;
    for (; i < n; i++) {
        c = (*this)[i];

        if (c->m_bit0) {
            if (testDist) {
                if (!midDone)
                    cell->GetMidPoint(&mid);
                mid.y = camPos.y;
                mid.Sub(&camPos, &mid);
                float half = 0.707f * CWorld::m_world->m_grid.m_CellDim;

                mid.x = fabsf(mid.x);
                mid.z = fabsf(mid.z);
                if (mid.x > half)
                    mid.x -= half;
                else
                    mid.x = 0.0f;
                if (mid.z > half)
                    mid.z -= half;
                else
                    mid.z = 0.0f;
                float len = mid.x * mid.x + mid.y * mid.y + mid.z * mid.z;

                distSq = len * cam->m_ZoomedRangeScale;
                testDist = false;
                if (distSq > m_max_farFadeSq)
                    break;
            }
            if (distSq > c->m_farFadeSq)
                break;
        }
        c->Render(&camPos);
    }
    SetAddColor(CPnt3D(0.0f, 0.0f, 0.0f));
    CVisual::m_applyLocalLights = lights;
}

/* every prop let go of (an MSL for_each, its class iterators in stack slots), the list emptied
 * (SOCOM 1's; SOCOM II clears its word at 0x14 too) */
void CClutterList::Free()
{
    std::for_each(begin(), end(), CClutterRelease());
    size_ = 0;
    m_socom2_14 = 0;
}

/* the unit matrix, the flags cleared each through its setter, no animation, the node kept (SOCOM
 * 1's; SOCOM II's flag word has three fields more) */
CClutter::CClutter(CNode *node)
{
    SetBit2(false);
    SetBit0(false);
    SetBit12(false);
    SetQuat(false);
    SetBit3(false);
    SetBits4(0);
    SetBit13(false);
    SetCount(0);
    SetBits18(0);
    m_node = 0;
    m_anim = 0;
    m_5c = -1;
    m_node = node;
}

bool CClutter::Read(zar::CZAR *zar)
{
    if (m_node) {
        m_node->SetDynamic(true, true);
        m_node->SetApplyMultipassEffects(false);
        m_node->SetApplyDecals(false);
    }
    zar->Fetch("params", this, 0x60);
    SetCount(0);
    bool ok = zar->Fetch("scale_inverse", &m_scale_inverse, 4);

    return ok;
}

/* the prop's translation: its matrix's last row, or the translation row of its rotation form
 * (SOCOM 1's, the same eight words) */
CPnt3D *CClutter::GetTranslate()
{
    if (m_quat)
        return (CPnt3D *)m_matrix.m_matrix[2];
    return (CPnt3D *)m_matrix.m_matrix[3];
}

/* the prop's matrix: its own, or -- in the rotation form -- built in the clutter manager's matrix
 * from the product of its two rotations (normalised), its scale (row 2's w) and its translation
 * (SOCOM 1's; SOCOM II normalises the product and skips a unit scale).
 * Byte-exact (open rounds 9-18, 19 off on the registers alone): the target's `this` is two
 * variables -- a0 on the plain path, a copy in s1 on the rotation path (round 16's replay) -- and
 * every plain copy (`CClutter *c = this`, a free inline's parameter) was coalesced back into it; a
 * copy of another type, `const CClutter *c = this` (or any cast copy), is a variable of its own
 * (round 19, ZAnimWildcardApply's rule) */
CMatrix *CClutter::GetMatrix()
{
    if (m_quat) {
        const CClutter *c = this;
        CMatrix *m = &CWorld::m_world->m_grid.m_ClutterAnimManager->m_matrix;
        CQuat q;

        m->SetIdentity();
        ((CQuat *)c->m_matrix.m_matrix[1])->Mul((CQuat *)c->m_matrix.m_matrix[0], &q);
        q.Normalize();
        q.ToMatrix(m);
        if (c->m_matrix.m_matrix[2][3] != 1.0f) {
            CMatrix s;
            float k = c->m_matrix.m_matrix[2][3];

            s.m_matrix[0][0] = k;
            s.m_matrix[1][1] = k;
            s.m_matrix[2][2] = k;
            sceVu0MulMatrix(m->m_matrix, s.m_matrix, m->m_matrix);
        }
        ClutterPntSet(m->m_matrix[3], c->m_matrix.m_matrix[2][0], c->m_matrix.m_matrix[2][1],
                      c->m_matrix.m_matrix[2][2]);
        return m;
    }
    return &m_matrix;
}

/* a point's squared length, through a reference (ours: CClutter::Render's distance -- the point a
 * local whose address the Sub takes, read once through the constructor's `this` and once through
 * this reference, so MW reads its fields off the frame, as the target does; read directly, it keeps
 * the fields' addresses in saved registers, as SOCOM 1's build did) */
inline float LenSq(const CPnt3D &p)
{
    return p.x * p.x + p.y * p.y + p.z * p.z;
}

/* the prop drawn, once a frame (the frame's low fourteen bits kept in its flag word): its animation
 * ticked first in the rotation form (the node it hands back drawn), its region asked of the camera
 * when it has one; faded out past m_fadeOutStartSq and gone at m_farFadeSq, and faded in from
 * m_fadeInSq, by its squared distance from the point scaled by the camera's zoomed range -- not
 * drawn under 0.008; the node's opacity lowered for the draw, its matrix pushed, its colour and
 * light scale set, the mesh or the node drawn and everything put back (SOCOM 1's, 0x218A10: SOCOM
 * II draws once a frame and scales the distance by the zoom; written in the core lane's round 17).
 * The node declared first and set after the frame's test (numbered ahead of the frame, loaded
 * after the test); the frame read as a word (a 64-bit read through an unsigned long loads with ld);
 * the length a named float, so the scale's product takes it first; the flip a byte (andi), the
 * bound negated in place (mov.s in the delay slot) */
void CClutter::Render(CPnt3D *pos)
{
    CNode *node;
    unsigned int frame = (unsigned int)zVid.frameNumber & 0x3fff;

    if (frame == m_bits18)
        return;
    node = m_node;
    if (!node)
        return;
    float scale = 1.0f;
    CCamera *cam = thePipe.m_camera;

    if (m_quat)
        node = CWorld::m_world->m_grid.m_ClutterAnimManager->ClutterTick(this);
    if (m_regionTest && !cam->CanSeeRegion(1 << m_region))
        return;
    if (m_bit0 || m_bit2) {
        CPnt3D d(pos->x, pos->y, pos->z);

        d.Sub(GetTranslate(), &d);
        float len = LenSq(d);
        float distSq = len * cam->m_ZoomedRangeScale;

        if (m_bit0) {
            if (distSq >= m_farFadeSq)
                return;
            if (distSq > m_fadeOutStartSq) {
                scale = m_fadeOutScale * (m_farFadeSq - distSq);
                if (scale < 0.008f)
                    return;
            }
        }
        if (m_bit2) {
            unsigned char flip = m_bit12;
            float in = m_fadeInSq;
            float lim = in;

            if (flip)
                lim = -in;
            if (distSq <= lim)
                return;
            if (distSq < m_fadeInEndSq) {
                if (flip)
                    scale *= m_fadeInScale * (in + distSq);
                else
                    scale *= 1.0f - m_fadeInScale * (m_fadeInEndSq - distSq);
                if (scale < 0.008f)
                    return;
            }
        }
    }
    float saved;

    if (scale < 1.0f) {
        saved = node->m_Opacity;
        node->m_Opacity = scale;
    }
    thePipe.m_node = node;
    stack.Multiply(GetMatrix(), true);
    CPnt3D col = ColConvTo3D(m_5c);

    SetAddColor(col);
    CVisual::m_GlobalLightScale = m_scale_inverse;
    if (node->m_hasMesh)
        thePipe.RenderMesh(node, 0, 0, ZVIS_FOV_CLIP);
    else
        thePipe.RenderNode(node, ZVIS_FOV_CLIP);
    m_bits18 = frame;
    CVisual::m_GlobalLightScale = 1.0f;
    stack.Pop();
    if (scale < 1.0f)
        node->m_Opacity = saved;
}

/* a point's length in x and z (ours) */
inline float LengthXZ(const CPnt3D *p)
{
    return sqrtf(p->x * p->x + p->z * p->z);
}

/* a point's x and z made unit length, unless both are zero (ours: TickForce's two). The length
 * through an inline of its own: its parameter is another variable, so the scaling reloads x where
 * the length's load would be reused (the target's reload, and the nop padding the join) */
inline void NormalizeXZ(CPnt3D *p)
{
    float len = LengthXZ(p);

    if (len != 0.0f) {
        float inv = 1.0f / len;

        p->x *= inv;
        p->z *= inv;
    }
}

/* the prop pushed: a force over a hundredth in x and z whose share of the push (1 - a / b, times
 * the prop's stiffness) reaches what the prop already holds turns the force toward the point's
 * distance, doubled, onto the point -- the point, unit in x and z and scaled by what the stiffness
 * leaves of the held push, onto the lean, which stays unit length -- and the push is held; a weaker
 * one lets the held push relax by half the frame's time, to zero. The prop's rotation slerped (at
 * most all the way, five times the frame's time) toward the lean tipped by the held push (or by the
 * rest angle alone when none is held); its node handed back (SOCOM 1's, 0x218600: SOCOM II takes
 * the square roots inline) */
CNode *CClutterAnim::TickForce(CPnt3D *force, CPnt3D *pos, float a, float b)
{
    float len = sqrtf(force->x * force->x + force->z * force->z);

    if (len > 0.01f) {
        float push = (1.0f - a / b) * m_clutter->Stiffness();

        if (push >= m_14) {
            *force = *force * (sqrtf(pos->x * pos->x + pos->z * pos->z) / len) * 2.0f;
            *pos += *force;
            float left = 1.0f - m_14 / m_clutter->Stiffness();

            NormalizeXZ(pos);
            pos->Scale(left);
            m_lean += *pos;
            NormalizeXZ(&m_lean);
            m_14 = push;
        } else {
            if (m_14 > 0.0f)
                m_14 -= 0.5f * zVid.frameTime;
            if (m_14 < 0.0f)
                m_14 = 0.0f;
        }
    }
    float ang = m_14 == 0.0f ? m_20 : m_14 + m_14 * m_20;
    CClutter *c = m_clutter;
    CQuat q;

    q.MakeYXZRot(0.0f, m_lean.z * ang, -m_lean.x * ang);
    float t = 5.0f * zVid.frameTime;

    if (t > 1.0f)
        t = 1.0f;
    CQuat r;

    ((CQuat *)c->m_matrix.m_matrix[1])->SlerpFaster(&q, t, &r);
    *(CQuat *)c->m_matrix.m_matrix[1] = r;
    return m_node;
}

/* an animation ticked: its wobble's clock run down, its swing timer stepped (the rest angle
 * reversed each half second); with no push held the rest angle damped, and once the clock is out
 * and the prop's lean all but gone (each of the rotation's x, z and y under 0.005) the animation
 * is done -- false (ours: l_Tick's test, a bool through an inline as the target materialises it;
 * the prop read ahead of the clock's test, into its branch's delay slot) */
inline bool AnimTick(CClutterAnim *a, float dT)
{
    a->m_18 -= dT;
    a->m_1c += dT;
    if (a->m_1c > 0.5f) {
        a->m_1c = 0.0f;
        a->m_20 = -a->m_20;
    }
    if (a->m_14 < 0.0001f) {
        a->m_20 *= 0.05f;
        CClutter *c = a->m_clutter;

        if (a->m_18 <= 0.0f) {
            if (fabsf(c->m_matrix.m_matrix[1][0]) < 0.005f && fabsf(c->m_matrix.m_matrix[1][2]) < 0.005f &&
                fabsf(c->m_matrix.m_matrix[1][1]) < 0.005f)
                return false;
        }
    }
    return true;
}

/* the animations ticked, each one done let go of by its prop, deleted and taken off the list; the
 * effectors that bring no velocity of their own given one from their move since the last tick
 * (SOCOM 1's, 0x217840; written in the core lane's round 17) */
int CClutterAnimManager::l_Tick(float dT, void *data)
{
    std::list<CClutterAnim *>::iterator it = m_ClutterAnims.begin();

    while (it != m_ClutterAnims.end()) {
        CClutterAnim *anim = *it;

        if (!AnimTick(anim, dT)) {
            anim->m_clutter->m_anim = 0;
            delete anim;
            it = m_ClutterAnims.erase(it);
        } else
            ++it;
    }

    std::list<CClutterEffector *>::iterator i = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

    for (; i != e; ++i) {
        CClutterEffector *f = *i;

        if (!f->m_vel) {
            f->m_18 = *f->m_pos - f->m_c;
            f->m_18.Scale(1.0f / dT);
            f->m_c = *f->m_pos;
        }
    }
    return 0;
}

/* the clutter task: the world's manager ticked (SOCOM 1's) */
int CClutterAnimManager::Tick(float dT, void *data)
{
    CWorld::m_world->m_grid.m_ClutterAnimManager->l_Tick(dT, data);
    return 0;
}

/* the effectors let go of (SOCOM 1's) */
void CClutterAnimManager::ClearEffectors()
{
    std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

    for (; it != e; ++it)
        delete *it;
    m_ClutterEffectors.clear();
}

/* whether the cell at mid wants animating, for a viewer at pos: within four half-cells of the
 * viewer, and an effector within a half-cell and its radius of the cell's middle, in x and z
 * (SOCOM 1's TestCellMid, 0x217F50, its differences through CPnt3D::Sub; SOCOM II's take the two
 * components straight) */
void CClutterAnimManager::TestCellMid(CPnt3D *mid, CPnt3D *pos)
{
    m_CellAnimate = false;
    if (m_ClutterEffectors.size()) {
        float half = 0.5f * CWorld::m_world->m_grid.m_CellDim;
        float range = 4.0f * half;

        if (!(fabsf(pos->x - mid->x) <= range && fabsf(pos->z - mid->z) <= range))
            return;

        std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

        for (; it != e; ++it) {
            CClutterEffector *f = *it;

            if (fabsf(mid->x - f->m_pos->x) <= half + f->GetRadius() &&
                fabsf(mid->z - f->m_pos->z) <= half + f->GetRadius()) {
                m_CellAnimate = true;
                return;
            }
        }
    }
}

/* the clutter task taken off the scheduler, the animations and the effectors let go of (SOCOM
 * 1's). The effector list's implicit destructor reads its first node through the inlined
 * ClearEffectors' &end_.next_ (lw v0,0(s2)): byte-exact since include/list's ~__list_deleter binds
 * the sentinel by reference, as SOCOM II's begin() does */
CClutterAnimManager::~CClutterAnimManager()
{
    zTaskScheduler.RemoveTask("CClutterAnimManager", false);
    ClearAnimations(false);
    ClearEffectors();
}

/* every animation deleted -- its prop's rotation reset and its node deleted -- and the list
 * emptied; with detach, each prop forgets its animation first (SOCOM 1's) */
void CClutterAnimManager::ClearAnimations(bool detach)
{
    std::list<CClutterAnim *>::iterator it = m_ClutterAnims.begin(), e = m_ClutterAnims.end();

    for (; it != e; ++it) {
        if (detach && (*it)->m_clutter)
            (*it)->m_clutter->m_anim = 0;
        delete *it;
    }
    m_ClutterAnims.clear();
}

/* an owner's effector deleted and taken off the list (SOCOM 1's) */
void CClutterAnimManager::UnRegisterEffector(const void *owner)
{
    std::list<CClutterEffector *>::iterator it = m_ClutterEffectors.begin(), e = m_ClutterEffectors.end();

    for (; it != e; ++it) {
        if ((*it)->m_owner == owner) {
            delete *it;
            m_ClutterEffectors.erase(it);
            return;
        }
    }
}

}
