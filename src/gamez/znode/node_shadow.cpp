/* node_shadow.cpp -- zdb::CRenderMap, a node's shadow: a texture rendered from it each frame and
 * projected back as a light map (OVL1 0x31A180-0x31AE2F). The unit name is SOCOM 1's (the press
 * beta's CRenderMap destructor and constructor assert node_shadow.cpp); SOCOM 1 opened it with
 * CProjectedMap_Object's destructor and constructor, which SOCOM II's has not. Defined in reverse
 * of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>
#include "node_world.h"
#include "../zsys/zsys_public.h"

/* the margin a shadow caster's box is grown by (SOCOM 1's file-static boxAdjust; its
 * __sinit_node_shadow.cpp builds it as SOCOM II's 0x402940 does, 0x45C338) -- read by the shadow's
 * set-up 0x31A180 */
/* a box from two points (ours; SOCOM II's CBBox has a constructor nothing else needs here: zmath.h's
 * is left without one) */
class CornerBox {
public:
    CornerBox() {}
    CornerBox(const CPnt3D &p, const CPnt3D &q) : m_min(p), m_max(q) {}
    CornerBox(const CornerBox &b) : m_min(b.m_min), m_max(b.m_max) {}

    CPnt3D m_min;
    CPnt3D m_max;
};

static CornerBox s_nodeBox;
static CPnt3D boxAdjust(5.0f, 5.0f, 5.0f);

/* the render map's count, its names' number (SOCOM 1's function-local static) and the GS packet's
 * template (OVL1 data) */
extern int data_0045C310;
extern const unsigned long data_003DFD40[26];

/* the tree's box (seal_env.cpp's, OVL2, not named); a switch this set-up reads (OVL1 data, not
 * named) */
extern "C" CBBox *func_005B67F0(zdb::CNode *node, CBBox *box);
extern unsigned char data_0045C348;
u_long128 *zSysSprGetPacket(u_long128 **end);
void zSysFifoKick(u_long128 *packet, unsigned int qwc);

extern "C" void sceVu0OuterProduct(float *v0, float *v1, float *v2);
extern "C" void sceVu0CameraMatrix(float m[4][4], float *p, float *zd, float *yd);
extern "C" void sceVu0Normalize(float *v0, float *v1);

namespace zdb {

/* a box's eight corners with w 1, in GetBBoxCorners' order (zrndr_pipe.cpp's; ours) */
static inline void BoxCorners4(PNT4D *c, const CornerBox &b)
{
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
}

/* the smaller and the larger of two floats, both by reference (node_model.cpp's) */
static inline float MinOf(const float &a, const float &b) { return a <= b ? a : b; }
static inline float MaxOf(const float &a, const float &b) { return a >= b ? a : b; }

/* a point taken into a box (ours, inline) */
static inline void AddPoint(CornerBox &b, const PNT4D &p)
{
    b.m_min.x = p.x <= b.m_min.x ? p.x : b.m_min.x;
    b.m_min.y = MinOf(p.y, b.m_min.y);
    b.m_min.z = MinOf(p.z, b.m_min.z);
    b.m_max.x = MaxOf(p.x, b.m_max.x);
    b.m_max.y = MaxOf(p.y, b.m_max.y);
    b.m_max.z = MaxOf(p.z, b.m_max.z);
}

/* SOCOM 1's Update grown a bool: for a new texture, the tree's box about the node's position, the
 * light's range and place from it and the world's shadow direction, the view from the light; the
 * box's corners through that view on the scratchpad, the box of them scaling the texture's two
 * matrices; the packet's PRIM word through its uncached address, 6 or 0; then the DMA reference,
 * the VIF codes, the matrix and the GIF tag kicked. Byte-exact (the world lane's round 33, unwritten
 * before; build/scratch/wld/v_up*.py). What each piece of the source is there for:
 * - a box with its own constructors (CornerBox: SOCOM II's CBBox has them, zmath.h's does not): a
 *   member initialised from a point copies by ld / sd at an aligned offset and by three FPRs at
 *   +12, the file static's assignment likewise only when the static's type is that class;
 * - the corners taken from a temporary copy bound to a reference (a by-value parameter's `&b + 12`
 *   is shared with the copy's and read through a register; here the reads stay frame-relative);
 * - the cursor a `char *` indexed `q + i * 16`: `(q + i * 16) + 4` is subable where `p + 4` off a
 *   register local is not (IroSubable.c), so y's and z's addresses are common between the min and
 *   the max (v1, v0), hoisted from the strength-reduced pointer -- a PNT4D cursor folds them;
 * - x's minimum a ternary in place, the rest by-reference MinOf / MaxOf (an inline taking the
 *   stack object at offset 0 by reference re-reads it);
 * - center.y and .z held through a `(float)` copy so they number ahead of size's (f23 / f22 against
 *   f21 / f20; plain copies fold into temporaries numbered after), center.x a cast argument
 *   so SetPos reads it once; `size` initialised, `center` and `ext` assigned (the frame's order);
 *   the larger of three a ternary (an `if` puts a pad at its join); the CStack ahead of the matrix */
void CRenderMap::Update(bool newTexture)
{
    unsigned long *gs = (unsigned long *)((char *)m_packet + 112);

    if (newTexture) {
        CPnt3D center;
        CPnt3D ext;
        CPnt3D zero = CPnt3D::zero;
        CPnt4D dir;
        CornerBox box;
        CPnt4D pos;

        s_nodeBox = CornerBox(zero, zero);
        box = *(CornerBox *)func_005B67F0(m_node->m_child[0], (CBBox *)&s_nodeBox);
        center = box.m_min + (box.m_max - box.m_min) * 0.5f + *(CPnt3D *)m_node->m_matrix.m_matrix[3];
        float cy = (float)center.y;
        float cz = (float)center.z;

        CPnt3D size = box.m_max - box.m_min;
        float sy = size.y;
        float big = size.x > sy ? size.x : sy;
        float sz = size.z;

        big = sz > big ? sz : big;
        m_lightmap->SetMaxRange(1.5f * big);
        CWorld *world = CWorld::m_world;

        dir = CPnt4D(world->m_shadow_vector.x, world->m_shadow_vector.y, world->m_shadow_vector.z, 0.0f);
        m_lightmap->SetPos((float)center.x, cy, cz);
        *(PNT4D *)&m_lightmap->m_dir = dir;
        pos = CPnt4D(center.x, cy, cz, 0.0f);
        sceVu0CameraMatrix(m_mtx0.m_matrix, (float *)&pos, (float *)&dir, m_axis);
        CStack stack(false);
        CMatrix m;

        m_node->m_matrix.Multiply(&m_mtx0, &m);
        BoxCorners4((PNT4D *)0x70000000, CornerBox(box));
        m.Transform((CPnt4D *)0x70000000, (CPnt4D *)0x70000000, 8);
        CPnt3D zero2 = CPnt3D::zero;
        CornerBox b(zero2, zero2);
        char *q = (char *)0x70000000;

        for (int i = 0; i < 8; i++)
            AddPoint(b, *(PNT4D *)(q + i * 16));
        ext = b.m_max - b.m_min;
        float sx;
        float sy2;

        if (data_0045C348) {
            sx = 1.0f / sy;
            sy2 = 1.0f / sz;
        } else {
            float e = ext.x > ext.y ? ext.x : ext.y;

            e = ext.z > e ? ext.z : e;
            sx = 1.0f / e;
            sy2 = sx;
        }
        m_mtx1.m_matrix[0][0] = m_size * sx;
        m_mtx1.m_matrix[1][1] = m_size * sy2;
        m_mtx2.m_matrix[0][0] = m_bias * sx;
        m_mtx2.m_matrix[1][1] = m_bias * sy2;
        sceVu0MulMatrix(m_lightmap->m_worldToUV.m_matrix, m_mtx2.m_matrix, m_mtx0.m_matrix);
        *(unsigned long *)((unsigned int)gs | 0x20000000) = 6;
    } else
        *(unsigned long *)((unsigned int)gs | 0x20000000) = 0;
    _word128 *pk = (_word128 *)zSysSprGetPacket(0);

    pk[0].u128 = *(u_long128 *)m_dmaRef;
    pk[1].u128 = *(u_long128 *)m_vif0;
    sceVu0MulMatrix((float (*)[4])&pk[2], m_mtx1.m_matrix, m_mtx0.m_matrix);
    pk[6].u128 = *(u_long128 *)m_vif1;
    pk[7].u128 = *(u_long128 *)m_giftag;
    zSysFifoKick((u_long128 *)pk, 8);
}

/* SOCOM 1's: the map's own light, named and listed in the world, its packet from a template -- the
 * frame, scissors and offset for the texture's size -- and an axis perpendicular to the shadow's
 * direction (the component nearest zero made 1 in a vector the original leaves uninitialised) */
CRenderMap::CRenderMap(CNode *node, _tag_render_map_type type, const CTexture *tex)
{
    char name[32];
    sceVu0FVECTOR up;
    sceVu0FVECTOR dir;
    sceVu0FVECTOR tmp;
    CWorld *world = CWorld::m_world;

    m_type = type;
    m_node = node->m_type == 2 ? node : 0;
    m_packet = _malloc(208, 0, 0);
    m_texture = tex;
    switch (type) {
    case RENDER_MAP_SHADOW:
        m_isShadow = 1;
        break;
    case RENDER_MAP_2:
        break;
    }
    sprintf(name, "render_map_%d", data_0045C310++);
    m_lightmap = this;
    m_lightmap->SetRange(1.0f, 30.0f);
    m_lightmap->SetTexture((CTexture *)m_texture);
    m_lightmap->SetName(name);
    m_lightmap->m_isShadow = 1;
    m_lightmap->m_instance = node;
    m_lightmap->m_opacity = 255.0f * world->m_shadow_weight;
    world->AddLightMap(m_lightmap);
    float width = (float)(int)m_texture->m_width;
    unsigned int size = (unsigned int)width;

    m_size = width - 2.0f;
    m_bias = 1.0f - 1.0f / m_size;
    dir[0] = world->m_shadow_vector.x;
    dir[1] = world->m_shadow_vector.y;
    dir[2] = world->m_shadow_vector.z;
    dir[3] = 0.0f;
    float best = fabsf(dir[0]);
    int axis = 0;

    if (fabsf(dir[1]) < best) {
        axis = 1;
        best = fabsf(dir[1]);
    }
    if (fabsf(dir[2]) < best)
        axis = 2;
    up[axis] = 1.0f;
    sceVu0OuterProduct(m_axis, dir, up);
    sceVu0Normalize(m_axis, m_axis);
    sceVu0OuterProduct(tmp, m_axis, dir);
    unsigned long *p = (unsigned long *)m_packet;

    m_dmaRef[0] = 0x3000000D;
    m_dmaRef[1] = (unsigned int)p;
    m_dmaRef[2] = 0x11000000;
    m_dmaRef[3] = 0x5000000D;
    memcpy(m_packet, data_003DFD40, 208);
    p[2] = (unsigned long)((m_texture->m_gsaddr * 64) / 2048) | ((unsigned long)(size >> 6) << 16) |
           ((unsigned long)m_texture->m_format << 24);
    p[12] = ((unsigned long)(size - 1) << 16) | ((unsigned long)(size - 1) << 48);
    p[18] = 0;
    p[20] = (unsigned long)(size << 4) | ((unsigned long)(size << 4) << 16);
    p[22] = (unsigned long)((2048 - (size >> 1)) << 4) | ((unsigned long)((2048 - (size >> 1)) << 4) << 32);
    p[24] = 1UL | ((unsigned long)(size - 2) << 16) | (1UL << 32) | ((unsigned long)(size - 2) << 48);
    m_mtx1.m_matrix[3][0] = 2048.0f;
    m_mtx1.m_matrix[3][1] = 2048.0f;
    m_mtx1.m_matrix[3][2] = 0.0f;
    m_mtx1.m_matrix[3][3] = 1.0f;
    m_mtx2.m_matrix[3][0] = 0.5f;
    m_mtx2.m_matrix[3][1] = 0.5f;
    m_mtx2.m_matrix[3][2] = 0.0f;
    m_mtx2.m_matrix[3][3] = 1.0f;
    m_vif0[0] = 0x10000004;
    m_vif0[1] = 0;
    m_vif0[2] = 0x01000101;
    m_vif0[3] = 0x6C040004;
    m_vif1[0] = 0x10000001;
    m_vif1[1] = 0;
    m_vif1[2] = 0x01000101;
    m_vif1[3] = 0x6C01001A;
    if (m_isShadow)
        m_giftag[0] = 0x3025C00000008000;
    m_giftag[1] = 0x412;
}

/* the light taken off the world's list and the packet freed (SOCOM 1's) */
CRenderMap::~CRenderMap()
{
    CWorld::m_world->DeleteLightMap(m_lightmap);
    if (m_packet)
        _free(m_packet, 0, 0);
    m_packet = 0;
}

}
