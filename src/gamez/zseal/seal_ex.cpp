/* seal_ex.cpp -- SOCOM 1's unit of the name (__sinit_seal_ex.cpp, linked between seal_env.cpp and
 * seal_ik.cpp): the team-mask reader and CSealEx. OVL2 from 0x5B8190 (seal_env.cpp, the SEAL lane's,
 * ends at 0x5B818B) to before seal_ik.cpp's CZSealBody code. So far ftmask_create. Defined in
 * reverse of their image order (-inline deferred). */

#include "../zrdr/zrdr.h"
#include "zseal.h"

/* a weapon's hit on a SEAL's node: the SEAL takes it (SOCOM 1's CSealEx::OnWeaponHit, the same tail
 * call; OVL2 0x5B8260, the class's key function -- its table 0x669570 comes out with it) */
int CSealEx::OnWeaponHit(zdb::CNode *node, zdb::IntersectStruct *hit, CZProjectile *projectile)
{
    return m_seal->OnWeaponHit(hit, projectile);
}

/* a tag's values as a mask, a bit per value: the array's numbers each shifted into place, 0 for
 * anything but an array (SOCOM 1's ftmask_create__FP5_zrdr, the same walk over SOCOM II's
 * arrays -- the count in `length`, the items from 0; OVL2 0x5B8190: ai::TEAM's parser calls it).
 * Read through a const pointer: zrdr.h's const accessors through the non-const parameter reload
 * the type the first test loaded (12 instructions), where the target keeps it for every Count() */
unsigned int ftmask_create(_zrdr *tag)
{
    const _zrdr *rdr = tag;
    unsigned int mask = 0;

    if (!rdr || rdr->type != ZRDR_ARRAY)
        return 0;
    int n = rdr->Count();
    for (int i = 0; i < n; i++)
        mask |= 1 << rdr->Item(i)->Int();
    return mask;
}
