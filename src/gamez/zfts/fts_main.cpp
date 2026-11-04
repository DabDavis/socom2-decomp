/* fts_main.cpp -- the game's height and line-of-sight queries over a dummy polygon (SOCOM 1's
 * Apps/FTS/fts_main.cpp: ftsGetAltitudeApprox and CDIDummy, between ent_target.cpp and
 * fts_mission.cpp; OVL1 0x2A2EB0-0x2A3BBF, after compass.cpp). Its initialiser
 * __sinit_fts_main.cpp 0x400100 builds the unit's two statics as SOCOM 1's does, less the header
 * CSTable: `di`, a CDIDummy (zdb::CDI's constructor out of line, then the dummy's vtable at 0xc;
 * ~CDIDummy registered), and `hit`, an intersection whose polygon and node are cleared -- both read
 * only by the query 0x2A2EB0. Only the statics and the dummy so far: 0x2A2EB0 (0x2C0, the AI maps'
 * pieces called out of line), 0x2A3170 (0x7B4) and 0x2A3950 are unwritten, and their inline
 * copies (0x2A3930, 0x2A3AF0) stay with the drafts. Defined in reverse of their image order
 * (-inline deferred). */

#include "../znode/znode.h"

namespace zdb {
/* a ray's hit (SOCOM 1's DWARF: 0x20 bytes; camera.h's has no constructor -- this one clears the
 * polygon and the node, as the initialiser does) */
struct IntersectStruct {
    IntersectStruct() : m_poly(0), m_node(0) {}

    CPnt3D m_pos;   // 0x0
    CDI *m_poly;   // 0xc
    CPnt3D m_norm;   // 0x10
    CNode *m_node;   // 0x1c
};
}

/* a polygon that is never hit (SOCOM 1's CDIDummy: its three slots inline, its table 0x406260 the
 * base's with GetIntersect, GetAltitude and a third slot of its own) */
class CDIDummy : public zdb::CDI {
public:
    virtual int GetIntersect(zdb::DiIntersect *di) { return 0; }
    virtual int GetAltitude(zdb::DiIntersect *di) { return 0; }
    virtual void LoadTipAndTail(zdb::DiIntersect *di) {}
};

static CDIDummy di;
static zdb::IntersectStruct hit;
