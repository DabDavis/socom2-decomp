/* node_shadow.cpp -- zdb::CRenderMap, a node's shadow: a texture rendered from it each frame and
 * projected back as a light map (OVL1 0x31A180-0x31AE2F). The unit name is SOCOM 1's (the press
 * beta's CRenderMap destructor and constructor assert node_shadow.cpp); SOCOM 1 opened it with
 * CProjectedMap_Object's destructor and constructor, which SOCOM II's has not. Defined in reverse
 * of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>
#include "node_world.h"

/* the margin a shadow caster's box is grown by (SOCOM 1's file-static boxAdjust; its
 * __sinit_node_shadow.cpp builds it as SOCOM II's 0x402940 does, 0x45C338) -- read by the shadow's
 * set-up 0x31A180 */
static CPnt3D boxAdjust(5.0f, 5.0f, 5.0f);

/* the render map's count, its names' number (SOCOM 1's function-local static) and the GS packet's
 * template (OVL1 data) */
extern int data_0045C310;
extern const unsigned long data_003DFD40[26];

extern "C" void sceVu0OuterProduct(float *v0, float *v1, float *v2);
extern "C" void sceVu0Normalize(float *v0, float *v1);

namespace zdb {

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
