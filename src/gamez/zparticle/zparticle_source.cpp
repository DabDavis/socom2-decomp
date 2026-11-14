/* zparticle_source.cpp -- Particle::Source, an emitter: its sequences of scale, colour and
 * texture, its texture list, its tick (OVL1 0x3294B0-0x32C03F). The unit name is SOCOM 1's (its
 * __sinit_zparticle_source.cpp; SOCOM II's 0x402CE0). Defined in reverse of their image order
 * (-inline deferred). Written so far: the sequence setters, which branch the shared properties
 * before changing them, as SOCOM 1's do; RenderBBox, CheckExtents, the destructor and
 * Source::List::Uninit. */

#include <iostream>
#include <string.h>
#include "zparticle.h"

extern "C" float sqrtf(float);

/* SOCOM II's pointer-list begin() builds its first node from a reference to the sentinel's link,
 * so `&end_.next_` is a temporary computed once and kept for the clear after a walk: znode.h's
 * stand-ins, the same shapes (include/list's begin() takes the link by value; the names ours) */
typedef std::__list_deleter<const void *, std::allocator<const void *> > PtrListBase;
struct PtrListFirst : public PtrListBase::iterator {
    PtrListFirst(PtrListBase::node_base *const &link) : PtrListBase::iterator(link) {}
};
inline PtrListBase::iterator ListBegin(PtrListBase &l) { return PtrListFirst(l.end_node().next_); }
inline PtrListBase::iterator ListEnd(PtrListBase &l) { return PtrListBase::iterator(&l.end_node()); }
inline void ListClear(PtrListBase &l) { l.erase(ListBegin(l), ListEnd(l)); }

namespace Particle {

/* the live sources (SOCOM 1's Source::list: the unit's initialiser 0x402CE0 builds it after
 * <iostream>'s pair and registers ~List 0x32BFD0) */
Source::List Source::list;

/* SOCOM 1's: switched on or off when it changes -- its collision node too, when it has one and the
 * flag at 0x120 says so (SOCOM II tests the node) -- then, switched on, the shut-off flag cleared,
 * else the good position. The flag narrowed into a byte local and compared member first (SOCOM 1's
 * compares the parameter: the narrowed copy in v1, the raw one stored); the node's SetActive takes
 * the byte (zparticle.h's marked device) */
void Source::SetActive(bool on)
{
    unsigned char b = on;

    if (m_active != b) {
        m_active = b;
        if (m_hasbbox && m_diNode)
            m_diNode->SetActive(b);
        if (on)
            m_shuttingoff = 0;
        else
            m_goodPos = 0;
    }
}

/* a source's live particles let go of (SOCOM II's, OVL1 0x329AA0: the live list's RemoveSource on
 * it; the name ours) */
extern "C" void func_00329AA0(Source *source)
{
    Instance::list.RemoveSource(source);
}

/* the source's collision box made (below) */
extern "C" void func_0032AD20(Source *s);

/* the source's collision box switched (0x329A40, new in SOCOM II: zAnimParticleSourceTick calls it,
 * the world's round 29; the name ours): made first when the source has no node, then the flag at
 * 0x120 set and the node switched by it (the stored byte read back) */
void Source::SetHasBBox(bool on)
{
    if (!m_diNode)
        func_0032AD20(this);
    m_hasbbox = on;
    m_diNode->SetActive(m_hasbbox);
}

/* SOCOM 1's: the box grown to hold a particle, the radius's square kept */
void Source::CheckExtents(CPnt4D p, float r)
{
    if (m_hasbbox) {
        m_bboxMin.x = m_bboxMin.x < p.x ? m_bboxMin.x : p.x;
        m_bboxMin.y = m_bboxMin.y < p.y ? m_bboxMin.y : p.y;
        m_bboxMin.z = m_bboxMin.z < p.z ? m_bboxMin.z : p.z;
        m_bboxMax.x = m_bboxMax.x > p.x ? m_bboxMax.x : p.x;
        m_bboxMax.y = m_bboxMax.y > p.y ? m_bboxMax.y : p.y;
        m_bboxMax.z = m_bboxMax.z > p.z ? m_bboxMax.z : p.z;
        m_filled_volume = r * r;
    }
}

/* the box's corners laid onto the six faces, each face's normal worked out (ours -- A MARKED
 * MODELLING DEVICE: the target builds the corners through CPnt3D()'s out-of-line copy 0x1FB3C0 in a
 * loop, which only a MEMBER array built by a constructor too big to expand whole gives -- MW inlines
 * the constructor one level, its member array's construction a loop calling CPnt3D(); a local array
 * in such an inline unrolls into eight calls, one in the function none at all. CShadowCaster's
 * constructor in node_world.cpp shows the same loop: the world lane's round 21) */
struct FaceCorners {
    FaceCorners(Source *s, const CBBox *box)
    {
        zdb::GetBBoxCorners(m_pts, box);
        int w2, w3, w0, w6, w7, w5, w4, w1;

        memcpy(&s->m_diPolies[0]->m_pnts[0], (CPnt3D *)(w7 = (int)&m_pts[7]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[0]->m_pnts[1], (CPnt3D *)(w6 = (int)&m_pts[6]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[0]->m_pnts[2], (CPnt3D *)(w5 = (int)&m_pts[5]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[0]->m_pnts[3], (CPnt3D *)(w4 = (int)&m_pts[4]), sizeof(CPnt3D));
        s->m_diPolies[0]->SetEdgeVecsAndNorm();
        memcpy(&s->m_diPolies[1]->m_pnts[0], (CPnt3D *)(w0 = (int)&m_pts[0]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[1]->m_pnts[1], (CPnt3D *)(w3 = (int)&m_pts[3]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[1]->m_pnts[2], (CPnt3D *)w7, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[1]->m_pnts[3], (CPnt3D *)w4, sizeof(CPnt3D));
        s->m_diPolies[1]->SetEdgeVecsAndNorm();
        memcpy(&s->m_diPolies[2]->m_pnts[0], (CPnt3D *)(w2 = (int)&m_pts[2]), sizeof(CPnt3D));
        memcpy(&s->m_diPolies[2]->m_pnts[1], (CPnt3D *)w6, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[2]->m_pnts[2], (CPnt3D *)w7, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[2]->m_pnts[3], (CPnt3D *)w3, sizeof(CPnt3D));
        s->m_diPolies[2]->SetEdgeVecsAndNorm();
        memcpy(&s->m_diPolies[3]->m_pnts[0], (CPnt3D *)w5, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[3]->m_pnts[1], (CPnt3D *)w6, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[3]->m_pnts[2], (CPnt3D *)w2, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[3]->m_pnts[3], (CPnt3D *)(w1 = (int)&m_pts[1]), sizeof(CPnt3D));
        s->m_diPolies[3]->SetEdgeVecsAndNorm();
        memcpy(&s->m_diPolies[4]->m_pnts[0], (CPnt3D *)w4, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[4]->m_pnts[1], (CPnt3D *)w5, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[4]->m_pnts[2], (CPnt3D *)w1, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[4]->m_pnts[3], (CPnt3D *)w0, sizeof(CPnt3D));
        s->m_diPolies[4]->SetEdgeVecsAndNorm();
        memcpy(&s->m_diPolies[5]->m_pnts[0], (CPnt3D *)w0, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[5]->m_pnts[1], (CPnt3D *)w3, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[5]->m_pnts[2], (CPnt3D *)w2, sizeof(CPnt3D));
        memcpy(&s->m_diPolies[5]->m_pnts[3], (CPnt3D *)w1, sizeof(CPnt3D));
        s->m_diPolies[5]->SetEdgeVecsAndNorm();
    }

    CPnt3D m_pts[8];
};

/* SOCOM 1's: the box of the particles about the source's position, given to its collision node
 * (its box and position) and laid onto the six faces' corners -- SOCOM II's, for a source with a
 * box and a node (SOCOM 1's took the box apart further, 0x7D4 bytes). Written in the world lane's
 * round 28: the position built from m_oldPosW's three floats (the constructor's arguments read z, y,
 * x), the box's corners through FaceCorners above (the construction loop), the node's position
 * through CNode's point overload (in place, MW keeps &pos.y and &pos.z in saved registers).
 * Byte-exact since round 34 (32 off since round 28; the engine core's round 27 on zdb::CDIBBox::SetCDIBBox, the
 * same shape): the eight corner addresses' saved registers are a pure numbering. All eight were
 * IRO_CommonSubs temporaries, numbered by each corner's first reuse (p7, p4, p6, p3, p5, p2, p1, p0), and
 * `igreplay --permute` (8 of the 40,320 orders) wants p2, p3, p0, p6, p7, p5, p4 with p1 anywhere: each address
 * held in an int local (FaceCorners': a MARKED DEVICE, a conversion stops propagation and the common
 * subexpression) declared in that order and assigned at its first use */
void Source::GetBBox(CBBox *box)
{
    if (!m_hasbbox || !m_diNode)
        return;
    {
        CPnt3D pos(m_oldPosW.x, m_oldPosW.y, m_oldPosW.z);

        box->m_max = m_bboxMax - pos;
        box->m_min = m_bboxMin - pos;
        m_diNode->m_bbox = *box;
        m_diNode->SetPosition(pos);

        FaceCorners faces(this, box);
    }
}

/* SOCOM 1's: every source's box worked out (the drawing compiled out) */
void Source::RenderBBox()
{
    CBBox box;
    List::iterator e = list.end();

    for (List::iterator i = list.begin(); i != e; ++i)
        (*i)->GetBBox(&box);
}

/* the source's collision box made once (new in SOCOM II as a function of its own: SOCOM 1's
 * constructor built it) -- six four-point polygons of material 1 and camera type 2, the first and
 * last of type 3, the others 2; a node holding them, named, in the world, its extension pointing
 * back at the source, its DI flag and back-face search set (0x32AD20, not named: the box's switch
 * 0x329A40 calls it when the source has no node yet) */
extern "C" void func_0032AD20(Source *s)
{
    if (!s->m_diPolies[0]) {
        s->m_diPolies[0] = new zdb::CDIPoly;
        s->m_diPolies[1] = new zdb::CDIPoly;
        s->m_diPolies[2] = new zdb::CDIPoly;
        s->m_diPolies[3] = new zdb::CDIPoly;
        s->m_diPolies[4] = new zdb::CDIPoly;
        s->m_diPolies[5] = new zdb::CDIPoly;
        s->m_diPolies[0]->Allocate(4);
        s->m_diPolies[1]->Allocate(4);
        s->m_diPolies[2]->Allocate(4);
        s->m_diPolies[3]->Allocate(4);
        s->m_diPolies[4]->Allocate(4);
        s->m_diPolies[5]->Allocate(4);
        s->m_diPolies[0]->SetMaterial(1);
        s->m_diPolies[1]->SetMaterial(1);
        s->m_diPolies[2]->SetMaterial(1);
        s->m_diPolies[3]->SetMaterial(1);
        s->m_diPolies[4]->SetMaterial(1);
        s->m_diPolies[5]->SetMaterial(1);
        s->m_diPolies[0]->SetCameraType(2);
        s->m_diPolies[1]->SetCameraType(2);
        s->m_diPolies[2]->SetCameraType(2);
        s->m_diPolies[3]->SetCameraType(2);
        s->m_diPolies[4]->SetCameraType(2);
        s->m_diPolies[5]->SetCameraType(2);
        s->m_diPolies[0]->SetDiType(3);
        s->m_diPolies[1]->SetDiType(2);
        s->m_diPolies[2]->SetDiType(2);
        s->m_diPolies[3]->SetDiType(2);
        s->m_diPolies[4]->SetDiType(2);
        s->m_diPolies[5]->SetDiType(3);
        s->m_diNode = new zdb::CNode;
        s->m_diNode->AddDI(s->m_diPolies[0]);
        s->m_diNode->AddDI(s->m_diPolies[1]);
        s->m_diNode->AddDI(s->m_diPolies[2]);
        s->m_diNode->AddDI(s->m_diPolies[3]);
        s->m_diNode->AddDI(s->m_diPolies[4]);
        s->m_diNode->AddDI(s->m_diPolies[5]);
        s->m_diNode->SetName("PARTICLE DI NODE");
        zdb::CWorld::m_world->AddChild(s->m_diNode);
        s->m_particleEx = new CParticleEx(s);
        s->m_diNode->SetNodeEx(s->m_particleEx, false);
        s->m_diNode->m_hasDI = 1;
        s->m_diNode->m_di.m_bReturnBackFaceIntersections = 1;
    }
}

/* SOCOM 1's: every source deleted, the list emptied. The end declared first: byte-exact under
 * include/list's iterator as it stands and under SOCOM 1's (node *node_), where `it, e` swaps the
 * two saved registers (the world lane r6) */
void Source::List::Uninit()
{
    PtrListBase::iterator e = ListEnd(*this), it = ListBegin(*this);

    for (; it != e; ++it)
        delete (Source *)*it;
    ListClear(*this);
}

/* the shutdown's last step (OVL1, not named) */
extern "C" void func_0032AB20(Source *s);

/* vectors through the matrix's rotation, each copied into an aligned vector and back (zmath.h's
 * TransformVectors, the particles' header standing in for it) */
static inline void TransformVectors(const CMatrix *m, const CPnt3D *in, CPnt3D *out, int count)
{
    CPnt4D input;
    CPnt4D output;

#if Z_PORT
    /* vf4-vf7: the rows copied once, for the loop below */
    float z_rows[16];
    for (int k = 0; k < 16; k++)
        z_rows[k] = ((const float *)m)[k];
#else
    asm __volatile__(
        "lqc2 $vf4, 0x0(%0)\n"
        "lqc2 $vf5, 0x10(%0)\n"
        "lqc2 $vf6, 0x20(%0)\n"
        "lqc2 $vf7, 0x30(%0)\n"
        : : "r"(m) : "memory");
#endif
    do {
        input.x = in->x;
        input.y = in->y;
        input.z = in->z;
#if Z_PORT
        /* ACC = r0*x; ACC += r1*y; out = ACC + r2*z, xyz (zmath.h's _TransformVectorXYZ) */
        for (int k = 0; k < 3; k++) {
            float acc = z_rows[k] * input.x;
            acc = acc + z_rows[4 + k] * input.y;
            (&output.x)[k] = acc + z_rows[8 + k] * input.z;
        }
#else
        asm __volatile__(
            "lqc2 $vf8, 0(%0)\n"
            "vmulax.xyz $ACC, $vf4, $vf8\n"
            "vmadday.xyz $ACC, $vf5, $vf8\n"
            "vmaddz.xyz $vf9, $vf6, $vf8\n"
            "sqc2 $vf9, 0(%1)\n"
            : : "r"(&input), "r"(&output) : "memory");
#endif
        out->x = output.x;
        out->y = output.y;
        out->z = output.z;
        in++;
        out++;
    } while (--count);
}

/* SOCOM 1's: one tick of a source -- switched off, its instances gone: its textures and properties
 * given up; else its position in the world, its phase carried on by time or by distance and the
 * particles due counted; nothing farther from the camera than its range; each particle due
 * placed along the tick by its share, perturbed, and made, from the pool when it has one.
 * Byte-exact (the world lane's round 33, unwritten before; build/scratch/wld/v_tk*.py): SOCOM 1's
 * body but its range tests. Three things the source is shaped by: the old phase read through a
 * volatile (A MARKED STAND-IN -- the target keeps it apart in f1 and reloads the member for `+=`,
 * as SOCOM 1's does, where a plain read is common with those; read ahead of delta's stores it is
 * apart too but scheduled above them); the constant a named local (a literal is canonicalised
 * first, `c.eq.s K,x`; propagated from a local it stays second); the square root in a local of
 * its own (the operand order: a sum over a call's result is emitted the other way round). Instance
 * is 0x54 bytes here (`new` passes 84): m_opacity is Destruction_Instance's */
void Source::Tick(float dT)
{
    CPnt3D newPos;
    CPnt3D delta;
    CPnt3D d;
    CPnt3D pos;
    CPnt3D pos2;
    CPnt3D rot;

    if (!m_active)
        return;
    if (m_shuttingoff) {
        if (m_refcount <= 0) {
            m_textures.clear();
            if (m_props)
                m_props->DecRefCount(1);
            func_0032AB20(this);
        }
        return;
    }
    CMatrix mtw;

    m_filled_volume = 0.0f;
    if (m_node) {
        mtw.SetIdentity();
        m_node->BuildMTW(mtw);
        mtw.Transform(&m_offset, &newPos, 1);
    } else
        newPos = m_offset;
    m_bboxMin.x = m_bboxMax.x = m_oldPosW.x;
    m_bboxMin.y = m_bboxMax.y = m_oldPosW.y;
    m_bboxMin.z = m_bboxMax.z = m_oldPosW.z;
    if (m_goodPos) {
        delta.x = newPos.x - m_oldPosW.x;
        delta.y = newPos.y - m_oldPosW.y;
        delta.z = newPos.z - m_oldPosW.z;
        float oldPhase = *(volatile float *)&m_phase;

        if (m_numPeriods) {
            if (m_distInterval) {
                float len = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

                if (len < 40000.0f) {
                    float s = sqrtf(len);

                    m_phase += s * frequencyFactor;
                }
            } else
                m_phase += dT * frequencyFactor;
        } else
            m_phase = 0.0f;
        float periods = m_phase * m_invPeriod;
        int n = (int)periods;
        int count = n;

        m_phase -= m_period * (float)n;
        if (n) {
            int left = m_numPeriods;

            if (left != -1) {
                if (n <= left)
                    m_numPeriods = left - n;
                else {
                    m_numPeriods = 0;
                    count = left;
                    m_phase = 0.0f;
                }
            }
        }
        m_frozen = 0;
        m_outOfRange = 0;
        float range = m_distance;

        float inf = 3.4028235e38f;

        if (range != inf) {
            range = range < m_props->m_farFade2 ? range : m_props->m_farFade2;
            float limit = distanceFactor * range;

            d.x = m_oldPosW.x - referencePositionW.x;
            d.y = m_oldPosW.y - referencePositionW.y;
            d.z = m_oldPosW.z - referencePositionW.z;
            if (!(d.x * d.x + d.y * d.y + d.z * d.z < limit)) {
                if (m_freezable)
                    m_frozen = 1;
                m_outOfRange = 1;
                goto done;
            }
        }
        if (n) {
            zdb::CTexHandle *tex = 0;

            if (!m_perParticleVelocity) {
                if (m_node)
                    TransformVectors(&mtw, &m_localVelocity, &pos, 1);
                else
                    pos = m_localVelocity;
                pos.x += m_worldVelocity.x;
                pos.y += m_worldVelocity.y;
                pos.z += m_worldVelocity.z;
            }
            float step = dT / periods;
            float start = dT - step * (1.0f - oldPhase * m_invPeriod);
            float inv = 1.0f / dT;

            delta.x *= inv;
            delta.y *= inv;
            delta.z *= inv;
            for (int i = 0; i < count; i++) {
                unsigned int ntex = m_textures.size();

                if (ntex)
                    tex = m_textures[rand() % ntex];
                float t = start - (float)i * step;

                if (!m_perParticleVelocity) {
                    pos2.x = pos.x + t * m_props->m_accelW.x;
                    pos2.y = pos.y + t * m_props->m_accelW.y;
                    pos2.z = pos.z + t * m_props->m_accelW.z;
                }
                for (int j = 0; j < m_number; j++) {
                    float life = m_lifespanRange.Get();
                    float age = Instance::ageFactor * (t + m_initialAgeRange.Get());

                    if (age < life) {
                        CPnt4D vel;
                        CPnt4D p;
                        CPnt4D v;

                        if (m_perParticleVelocity) {
                            pos.x = m_localVelocity.x + m_perturbVelocityX.Get();
                            pos.y = m_localVelocity.y + m_perturbVelocityY.Get();
                            pos.z = m_localVelocity.z + m_perturbVelocityZ.Get();
                            if (m_node)
                                TransformVectors(&mtw, &pos, &pos, 1);
                            vel.x = pos.x + m_worldVelocity.x + t * m_props->m_accelW.x;
                            vel.y = pos.y + m_worldVelocity.y + t * m_props->m_accelW.y;
                            vel.z = pos.z + m_worldVelocity.z + t * m_props->m_accelW.z;
                        } else {
                            vel.x = pos2.x + m_perturbVelocityX.Get();
                            vel.y = pos2.y + m_perturbVelocityY.Get();
                            vel.z = pos2.z + m_perturbVelocityZ.Get();
                        }
                        p.x = m_oldPosW.x + m_perturbPositionX.Get();
                        p.y = m_oldPosW.y + m_perturbPositionY.Get();
                        p.z = m_oldPosW.z + m_perturbPositionZ.Get();
                        v.x = delta.x + vel.x;
                        v.y = delta.y + vel.y;
                        v.z = delta.z + vel.z;
                        p.x += t * v.x;
                        p.y += t * v.y;
                        p.z += t * v.z;
                        if (m_inheritVelocity)
                            *(PNT4D *)&vel = v;
                        rot.x = m_rotationX.Get();
                        rot.y = m_rotationY.Get();
                        rot.z = m_rotationZ.Get();
                        float radius = m_radiusRange.Get();
                        Props *props = m_props;
                        Instance *inst = Instance::m_instance_pool.m_head;

                        if (inst) {
                            Instance::m_instance_pool.m_head = inst->m_next;
                            inst->m_next = 0;
                        } else
                            inst = 0;
                        if (inst)
                            new (inst) Instance(p, vel, rot, radius, life, age, tex, props, this);
                        else
                            inst = new Instance(p, vel, rot, radius, life, age, tex, props, this);
                        inst->m_listNext = Instance::list.m_head;
                        Instance::list.m_head = inst;
                        m_refcount++;
                    }
                }
            }
        }
    }
done:
    m_oldPosW = newPos;
    m_goodPos = 1;
}

/* SOCOM 1's: the source off the list, its instances let go of, its properties and collision
 * node given up */
Source::~Source()
{
    List::iterator e = list.end();

    for (List::iterator i = list.begin(); i != e; ++i) {
        if (*i == this) {
            list.erase(i);
            break;
        }
    }
    Instance::list.RemoveSource(this);
    if (m_props)
        m_props->DecRefCount(1);
    if (m_diNode) {
        zdb::CWorld::m_world->DeleteChild(m_diNode);
        delete m_diNode;
    }
    if (m_particleEx)
        delete m_particleEx;
}

/* SOCOM 1's: the texture sequence emptied */
void Source::TextureSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_textureSeq.clear();
}

/* SOCOM 1's: a texture keyed at a fraction of the life, registered with the draw buffer */
void Source::TextureSequenceAdd(float fraction, zdb::CTexHandle *tex)
{
    m_props = m_props->Branch();
    m_props->m_textureSeq.push_back(SequenceEntry<zdb::CTexHandle *>(fraction, tex));
    AddDynTexture(tex);
}

/* SOCOM 1's: the colour sequence emptied */
void Source::ColorSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_colorSeq.clear();
}

/* SOCOM 1's: a colour keyed at a fraction of the life. The colour a named local, after the branch:
 * the target's sits above the entry's temporary (SOCOM 1's below it, the colour built in the
 * argument list) */
void Source::ColorSequenceAdd(float fraction, float r, float g, float b, float a)
{
    m_props = m_props->Branch();
    CPnt4D color(r, g, b, a);
    m_props->m_colorSeq.push_back(SequenceEntry<CPnt4D>(fraction, color));
}

/* SOCOM 1's: the scale sequence emptied */
void Source::ScaleSequenceClear()
{
    m_props = m_props->Branch();
    m_props->m_scaleSeq.clear();
}

/* SOCOM 1's: a scale keyed at a fraction of the life */
void Source::ScaleSequenceAdd(float fraction, float scale)
{
    m_props = m_props->Branch();
    m_props->m_scaleSeq.push_back(SequenceEntry<float>(fraction, scale));
}

}
