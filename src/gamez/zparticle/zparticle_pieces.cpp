/* zparticle_pieces.cpp -- the destruction pieces' two big functions (OVL1 0x322800-0x322DEF Create3Sided, 0x322DF0-0x3245AB
 * CreateVisuals, SOCOM 1's names). IN THE IMAGE THESE SIT INSIDE zparticle.cpp's RANGE (SOCOM 1's demo holds them there,
 * between DestructionManager and the rest): a unit of ours, A MODELLING DEVICE, only because that unit is written on
 * zparticle.h's stand-in classes (its own CPnt3D, CPnt4D and CMatrix, CVisual and CVisData opaque) where these two need
 * znode.h's and zmath.h's real ones: CMatrix's copy constructor, by value on VU0; CVisData; CVisual's buffers. */

#include <eetypes.h>
#include <vector>
#include <libvu0.h>
#include "../znode/node_world.h"

extern "C" int rand();

namespace Particle {

class Props;
/* zparticle.h's (zparticle_instance.cpp's) */
class Destruction_Instance {
public:
    static Destruction_Instance *Create(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin, float f, float life,
                                        float opacity, CPnt3D axis, Props *props);
    void SetVisual(zdb::CVisual *vis);
};

/* the destruction pieces' counts (not named: 0x45C478 the visuals made, 0x45C480 the pieces) */
extern int data_0045C478;
extern int data_0045C480;

/* a value between two (ours, inline: the lower bound read ahead of the random's call, the upper after) */
static inline float RandRange(float lo, const float &hi)
{
    return lo + (hi - lo) * (4.656613e-10f * (float)rand());
}

/* a spin's component (ours, inline) */
static inline float RandSpin()
{
    return -5.0f + 10.0f * (4.656613e-10f * (float)rand());
}

/* a face's normal direction: VU0's outer product of two points as vectors (ours, inline) */
static inline CPnt3D OuterProduct(CPnt3D *p, CPnt3D *q)
{
    PNT4D r;
    PNT4D a;
    PNT4D b;

    a.x = p->x;
    a.y = p->y;
    a.z = p->z;
    a.w = 1.0f;
    b.x = q->x;
    b.y = q->y;
    b.z = q->z;
    b.w = 1.0f;
    sceVu0OuterProduct((float *)&r, (float *)&a, (float *)&b);
    return CPnt3D(r.x, r.y, r.z);
}

/* a vertex of a piece's strip written: its place about the block's origin, its colour, the face's normal and its
 * texture coordinate (ours, inline) */
static inline void SetPieceVertex(zdb::CVisData *v, const CPnt3D &d, CPnt4D *color, const CPnt3D &n, const CPnt3D &uv)
{
    ((short *)v->m_vertex)[0] = (short)(16.0f * (d.x - ((float *)v->m_14)[0]));
    ((short *)v->m_vertex)[1] = (short)(16.0f * (d.y - ((float *)v->m_14)[1]));
    ((short *)v->m_vertex)[2] = (short)(16.0f * (d.z - ((float *)v->m_14)[2]));
    ((unsigned char *)v->m_color)[0] = (unsigned int)(color->x + 0.5f);
    ((unsigned char *)v->m_color)[1] = (unsigned int)(color->y + 0.5f);
    ((unsigned char *)v->m_color)[2] = (unsigned int)(color->z + 0.5f);
    ((unsigned char *)v->m_color)[3] = (unsigned int)(color->w + 0.5f);
    ((short *)v->m_vertex)[3] = (short)(32767.5f * n.x);
    ((short *)v->m_vertex)[6] = (short)(32767.5f * n.y);
    ((short *)v->m_vertex)[7] = (short)(32767.5f * n.z);
    ((short *)v->m_vertex)[4] = (short)(4096.0f * uv.x);
    ((short *)v->m_vertex)[5] = (short)(4096.0f * uv.y);
}

/* a piece made and given its visual (ours, inline) */
static inline void AddPiece(const CPnt4D &pos, const CPnt4D &vel, const CPnt3D &spin, float life, float opacity,
                            const CPnt3D &axis, Props *props, zdb::CVisual *vis)
{
    Destruction_Instance::Create(pos, vel, spin, 0.0f, life, opacity, axis, props)->SetVisual(vis);
    data_0045C480++;
}

/* SOCOM 1's (0x232EA0, 0xF10 bytes there): a triangle of a broken node made into pieces -- split about a random point of
 * each edge into four while it is small against the size (the first three corners' pieces made recursively, the middle
 * one with their visuals), each piece a copy of the "fragment" model's visual moved to the triangle's middle, with its
 * own three vertices, given a random place, spin and life. SOCOM II passes the second float, the parent's middle and the
 * three sub-visuals on. Unwritten before round 34 */
/* OPEN, 12 instructions long and the registers off (0x17EC of 0x17BC, frame 1616 of 1584; the world lane round 34, written
 * from the target, build/scratch/wld/cvd.py, greedy.py): every statement and call here is the target's in its order, the
 * unit of the split (the six interpolations, the mid, the four recursive calls), the piece test, the buffers' fill and the
 * pieces. It has been at the target's 1531 instructions with vel and spin in the arms; two slots of 16 are extra with them at
 * the top. The target's frame, for whoever closes it: uv0 1560, uv1 1544, uv2 1528, spin 1512, n0 1496, n1 1480, n2 1464, m01
 * 1448, t01 1432, m12 1416, t12 1400, m20 1384, t20 1368, mid 1352, axis 1336, tmp 1320, at 1304, d0 1288, d1 1272, d2 1256,
 * axis2 1240, sum 1224, temporaries 1208..1096, the operators' results 1080..872, data 784, vel 768, pos 752, pos2 736, the
 * matrix copies 672/608/544/480, the r/a/b triples 464..336, m's home 332, piece 304, &at.y 288, &at.z 272, base 1580, none
 * 1576, none2 1572. Its colours: p0 s2, p1 s1, p2 s0, tex s3, a s4, color s5, b s6, props s7, c s8, the new visual's temporary
 * s2, k s3, &n0.y s1, &n0.z s0; g f26, size f20; the colour order p2,p1,p0,tex,a,color,b,props,c -- ours colours v58,a,color,
 * p2,p1,p0,v59,v57,k,b,props,tex,c with tex and c spilled, and keeps &uv0.y where the target rematerialises all three uv y
 * addresses. The order of the interpolation statements was searched greedily over the shape distance (477 lines here). A
 * slot below every temporary is a split lifetime (CAlpha::Insert's rule, IRO_SplitLifetimes): not yet tried on this frame. */
zdb::CVisual *CreateVisuals(CPnt3D *p0, CPnt3D *p1, CPnt3D *p2, CPnt3D uv0, CPnt3D uv1, CPnt3D uv2, CMatrix m,
                            zdb::CTexHandle *tex, CPnt4D *color, float size, zdb::CModel *model, CPnt3D *a, CPnt3D *b,
                            CPnt3D *c, Props *props, float g, CPnt3D *center, zdb::CVisual *v0, zdb::CVisual *v1,
                            zdb::CVisual *v2)
{
    zdb::CVisData data;
    CPnt3D spin;
    CPnt4D vel;
    CPnt3D n0 = OuterProduct(p0, p1);
    CPnt3D n1 = OuterProduct(p1, p2);
    CPnt3D n2 = OuterProduct(p2, p0);
    CPnt3D sum = n0 + n1 + n2;
    float ratio = 0.6f * (size / sqrtf(sum.x * sum.x + sum.y * sum.y + sum.z * sum.z));
    bool piece;
    bool split;

    if (v0 || v1 || v2) {
        piece = false;
        split = false;
    } else if (ratio > 0.3f && 4.656613e-10f * (float)rand() < ratio) {
        piece = true;
        split = false;
    } else {
        split = true;
        piece = false;
    }
    if (split) {
        float t = 0.4f + 0.2f * (4.656613e-10f * (float)rand());
        CPnt3D m01;
        CPnt3D t01;
        CPnt3D m12;
        CPnt3D t12;
        CPnt3D m20;
        CPnt3D t20;

        m01.z = p1->z + t * (p0->z - p1->z);
        m01.y = p1->y + t * (p0->y - p1->y);
        t01.y = uv1.y + t * (uv0.y - uv1.y);
        t01.z = 0.0f;
        m01.x = p1->x + t * (p0->x - p1->x);
        t01.x = uv1.x + t * (uv0.x - uv1.x);
        t12.x = uv2.x + t * (uv1.x - uv2.x);
        t12.y = uv2.y + t * (uv1.y - uv2.y);
        t12.z = 0.0f;
        m12.x = p2->x + t * (p1->x - p2->x);
        m12.y = p2->y + t * (p1->y - p2->y);
        m12.z = p2->z + t * (p1->z - p2->z);
        t20.x = uv2.x + t * (uv0.x - uv2.x);
        t20.y = uv2.y + t * (uv0.y - uv2.y);
        t20.z = 0.0f;
        m20.x = p2->x + t * (p0->x - p2->x);
        m20.y = p2->y + t * (p0->y - p2->y);
        m20.z = p2->z + t * (p0->z - p2->z);
        CPnt3D mid = (m01 + m12 + m20) * (1.0f / 3.0f);

        v0 = CreateVisuals(p0, &m01, &m20, uv0, t01, t20, m, tex, color, size, model, a, b, c, props, g, &mid, 0, 0, 0);
        v1 = CreateVisuals(p1, &m01, &m12, uv1, t01, t12, m, tex, color, size, model, a, b, c, props, g, &mid, 0, 0, 0);
        v2 = CreateVisuals(p2, &m20, &m12, uv2, t20, t12, m, tex, color, size, model, a, b, c, props, g, &mid, 0, 0, 0);
        zdb::CVisual *vis = CreateVisuals(&m01, &m12, &m20, t01, t12, t20, m, tex, color, size, model, a, b, c, props, g, &mid, v0, v1, v2);

        if (vis) {
            vel.x = RandRange(a->x, b->x);
            vel.y = RandRange(a->y, b->y);
            vel.z = RandRange(a->z, b->z);
            spin.x = RandSpin();
            spin.y = RandSpin();
            spin.z = RandSpin();
            float life = RandRange(c->x, c->y);
            CPnt3D axis;

            axis.x = 1.0f;
            axis.y = 1.0f;
            axis.z = 1.0f;
            CPnt3D at = (m01 + m12 + m20) * (1.0f / 3.0f);
            CPnt4D pos;

            pos.x = at.x;
            pos.y = at.y;
            pos.z = at.z;
            AddPiece(pos, vel, spin, life, g, axis, props, vis);
        }
        return 0;
    }
    CPnt3D at;

    if (piece && center) {
        at.x = center->x;
        at.y = center->y;
        at.z = center->z;
    } else {
        CPnt3D q = (*p0 + *p1 + *p2) * (1.0f / 3.0f);

        at.x = q.x;
        at.y = q.y;
        at.z = q.z;
    }
    if (!piece && !v0 && !v1 && !v2 && !(4.656613e-10f * (float)rand() <= 0.3f))
        return 0;
    CPnt4D pos;

    pos.x = at.x;
    pos.y = at.y;
    pos.z = at.z;
    CPnt3D d0 = *p0 - at;
    CPnt3D d1 = *p1 - at;
    CPnt3D d2 = *p2 - at;

    if (!model->m_visual.size())
        return 0;
    zdb::CVisual *src = model->m_visual[0];

    if (!src)
        return 0;
    CVisBase *base = 0;
    _word128 *buf = src->CopyBuffer(&base, 0);
    zdb::CVisual *vis = new zdb::CVisual;

    data_0045C478++;
    zdb::CMesh *none = 0;

    vis->m_lodMesh.push_back(none);
    void *none2 = 0;

    vis->m_24.push_back(none2);
    vis->SetBuffer(buf, 0, base);
    zdb::CVisData *v = data.SelectVertex(vis, 0, 0);

    v->ReplaceTextureData((_word128 *)((char *)tex->m_texture + 0x40));
    vis->m_bits._b8 = tex->m_texture->m_bumpmap != 0;
    vis->m_bits._b0 = tex->m_texture->IsAlphaBlended();
    v = v->SelectVertex(vis, 0, 0);
    for (int k = 0; k < 1; k++) {
        SetPieceVertex(v, d0, color, n0, uv0);
        v = v->NextVertex(1);
        if (!v)
            break;
        SetPieceVertex(v, d1, color, n0, uv1);
        v = v->NextVertex(1);
        if (!v)
            break;
        SetPieceVertex(v, d2, color, n0, uv2);
        v = v->NextVertex(1);
    }
    v->m_mesh->m_numTris = 1;
    v->m_mesh->m_numVerts = 3;
    vel.x = RandRange(a->x, b->x);
    vel.y = RandRange(a->y, b->y);
    vel.z = RandRange(a->z, b->z);
    spin.x = RandSpin();
    spin.y = RandSpin();
    spin.z = RandSpin();
    float life = RandRange(c->x, c->y);
    CPnt3D axis;

    axis.x = 1.0f;
    axis.y = 1.0f;
    axis.z = 1.0f;
    if (piece)
        return vis;
    AddPiece(pos, vel, spin, life, g, axis, props, vis);
    if (v0)
        AddPiece(pos, vel, spin, life, g, axis, props, v0);
    if (v1)
        AddPiece(pos, vel, spin, life, g, axis, props, v1);
    if (v2)
        AddPiece(pos, vel, spin, life, g, axis, props, v2);
    return 0;
}

/* SOCOM 1's: a three-sided piece made for each collision piece of a node: each of its polygon's fans of three points
 * in the world, with their texture coordinates off the points' fourth words, broken by CreateVisuals (SOCOM II's call
 * passes SOCOM 1's arguments, the second float last); a visual given back is one more piece. Unwritten before round 34.
 * OPEN, 2 instructions (0x5EC of 0x5E4): one block off, the fan's first point (`uv1.x`): the target has the constant in f1 and
 * the high half in f2 where its other two blocks, and all three of ours, have f2 and f1 -- so ours has mtc1's hazard nop there
 * and the pad after it. igreplay: the high half numbered before the constant gives the target (`--move 60=44.5`, or the
 * constant to 60.5). Tried without effect: the half named, const, cast; inlines for the half, the low half and the pass;
 * the constant on the left; the second coordinate first. */
void Create3Sided(zdb::CDIVector *dis, zdb::CNode *node, CPnt4D *color, zdb::CTexHandle *tex, float f,
                  zdb::CModel *model, CPnt3D *a, CPnt3D *b, CPnt3D *c, CPnt3D *d, Props *props, float g)
{
    CMatrix mtw;

    node->BuildMTW(mtw);
    for (zdb::CDIVector::iterator i = dis->begin(); i != dis->end(); ++i) {
        zdb::CDI *poly = *i;
        CPnt4D *pnt0 = &poly->m_pnts[0];
        int word0 = *(int *)&pnt0->w;
        CPnt3D uv0;

        uv0.x = (word0 >> 16) * 2.4414e-4f;
        uv0.y = (word0 & 0xffff) * 2.4414e-4f;
        uv0.z = 0.0f;
        CPnt3D p0 = *(CPnt3D *)pnt0;

        mtw.Transform(&p0, &p0, 1);
        for (int j = 1; j < poly->m_flags.m_ptcount - 1; j++) {
            CPnt4D *pnt1 = &poly->m_pnts[j];
            int word1 = *(int *)&pnt1->w;
            CPnt3D uv1;

            uv1.x = (word1 >> 16) * 2.4414e-4f;
            uv1.y = (word1 & 0xffff) * 2.4414e-4f;
            uv1.z = 0.0f;
            CPnt3D p1 = *(CPnt3D *)pnt1;

            mtw.Transform(&p1, &p1, 1);
            CPnt4D *pnt2 = &poly->m_pnts[j + 1];
            int word2 = *(int *)&pnt2->w;
            CPnt3D uv2;

            uv2.x = (word2 >> 16) * 2.4414e-4f;
            uv2.y = (word2 & 0xffff) * 2.4414e-4f;
            uv2.z = 0.0f;
            CPnt3D p2 = *(CPnt3D *)pnt2;

            mtw.Transform(&p2, &p2, 1);
            color->w = 100.0f;
            data_0045C480 = 0;
            data_0045C478 = 0;
            zdb::CVisual *vis = CreateVisuals(&p0, &p1, &p2, uv0, uv1, uv2, mtw, tex, color, f, model, a, b, c, props, g, 0, 0, 0, 0);

            if (vis) {
                CPnt4D vel;
                CPnt3D spin;

                vel.x = RandRange(a->x, b->x);
                vel.y = RandRange(a->y, b->y);
                vel.z = RandRange(a->z, b->z);
                spin.x = RandSpin();
                spin.y = RandSpin();
                spin.z = RandSpin();
                float life = RandRange(c->x, c->y);
                CPnt3D axis;

                axis.x = 1.0f;
                axis.y = 1.0f;
                axis.z = 1.0f;
                CPnt3D at = (p0 + p1 + p2) * (1.0f / 3.0f);
                CPnt4D pos;

                pos.x = at.x;
                pos.y = at.y;
                pos.z = at.z;
                AddPiece(pos, vel, spin, life, g, axis, props, vis);
            }
        }
    }
}


}
