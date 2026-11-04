/* node_shadow.cpp -- zdb::CRenderMap, a node's shadow: a texture rendered from it each frame and
 * projected back as a light map (OVL1 0x31A180-0x31AE2F). The unit name is SOCOM 1's (the press
 * beta's CRenderMap destructor and constructor assert node_shadow.cpp); SOCOM 1 opened it with
 * CProjectedMap_Object's destructor and constructor, which SOCOM II's has not. Defined in reverse
 * of their image order (-inline deferred). */

#include "node_world.h"

/* the margin a shadow caster's box is grown by (SOCOM 1's file-static boxAdjust; its
 * __sinit_node_shadow.cpp builds it as SOCOM II's 0x402940 does, 0x45C338) -- read by the shadow's
 * set-up 0x31A180 */
static CPnt3D boxAdjust(5.0f, 5.0f, 5.0f);

namespace zdb {

/* the light taken off the world's list and the packet freed (SOCOM 1's) */
CRenderMap::~CRenderMap()
{
    CWorld::m_world->DeleteLightMap(m_lightmap);
    if (m_packet)
        _free(m_packet, 0, 0);
    m_packet = 0;
}

}
