/* node_util.cpp -- small node utilities: a node's DIs given a material, a box DI made from its
 * bounds (OVL1 0x31AE30-0x31AF63), and the closest-node test's default cosine. The unit name is
 * SOCOM 1's: its static initialiser (__sinit_node_util.cpp, 0x402960) pairs exactly with SOCOM
 * 1's, which sets DEFAULT_CLOSESTNODE_DOT. SOCOM 1's GetClosestNode, which used it, is not in
 * SOCOM II's image (unreferenced, stripped). Defined in reverse of their image order (-inline
 * deferred). */

#include "node_world.h"

/* CDIBBox's constructor (SOCOM 1's __ct__Q23zdb7CDIBBoxFv; not named yet) */
extern "C" zdb::CDIBBox *func_002DDB30(zdb::CDIBBox *di);

/* the default dot product a node must face within to be the closest one: 65 degrees */
float DEFAULT_CLOSESTNODE_DOT = cosf(1.134464f);

/* a box DI over the node's bounds (its own, or its parent's when it borrows them) added to it,
 * and its bounds updated from it; the box and flag passed are not read (SOCOM 1's) */
void SetDIBBox(zdb::CNode *node, const CBBox *bbox, bool b)
{
    zdb::CDIBBox *di = (zdb::CDIBBox *)operator new(0x18);

    if (di)
        di = func_002DDB30(di);
    di->m_flags.m_ditype = 3;
    di->m_bbox = node->GetBBox();
    node->AddDI(di);
    node->UpdateBBox(true);
}

/* every DI of the node given the material (SOCOM 1's) */
void SetMaterial(zdb::CNode *node, int material)
{
    zdb::CDIVector &di = node->m_di;

    if (node->m_di.size())
        for (zdb::CDIVector::iterator it = di.begin(); it != di.end(); ++it)
            (*it)->m_flags.m_material = material;
}
