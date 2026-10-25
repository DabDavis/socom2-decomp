/* node_shadow.cpp -- zdb::CRenderMap, a node's shadow: a texture rendered from it each frame and
 * projected back as a light map (OVL1 0x31A180-0x31AE2F). The unit name is SOCOM 1's (the press
 * beta's CRenderMap destructor and constructor assert node_shadow.cpp); SOCOM 1 opened it with
 * CProjectedMap_Object's destructor and constructor, which SOCOM II's has not. Defined in reverse
 * of their image order (-inline deferred). */

#include "node_world.h"

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
