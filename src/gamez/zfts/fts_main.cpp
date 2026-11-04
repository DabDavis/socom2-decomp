/* fts_main.cpp -- the game's height and line-of-sight queries over a dummy polygon (SOCOM 1's
 * Apps/FTS/fts_main.cpp: ftsGetAltitudeApprox and CDIDummy, between ent_target.cpp and
 * fts_mission.cpp; OVL1 0x2A2EB0-0x2A3BBF, after compass.cpp). Its initialiser
 * __sinit_fts_main.cpp 0x400100 builds the unit's two statics as SOCOM 1's does, less the header
 * CSTable: `di`, a CDIDummy (zdb::CDI's constructor out of line, then the dummy's vtable at 0xc;
 * ~CDIDummy registered), and `hit`, an intersection whose polygon and node are cleared -- both read
 * only by the query 0x2A2EB0. The query (the engine core's round 14) reads the AI maps through ai.h's
 * inlines, and the copies it calls follow it in reverse of first use: 0x2A3170 (SOCOM 1's
 * CAiMap::GetBilinearAltitude, 0x7B4: unwritten, called by its placeholder), 0x2A3930 (the cell's
 * bilinear bit: its draft), 0x2A3950 (CAiMap::GetLocation's OVL1 copy, from the inline below) and
 * 0x2A3AF0 (CAiMapLoc()'s); the AI map pieces hud_linemap.cpp emitted first (GetCell's and its
 * callees', the cell's altitude) are that unit's. Defined in reverse of their image order
 * (-inline deferred). */

#include "../znode/znode.h"
#include "../../ai/ai.h"
#include "../../ai/ai_maps.h"

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

/* whether a value lies in a span, both ends in (ai_map.cpp's: CAiMap::GetLocation's box tests) */
inline bool _in_span(const float &v, const float &lo, const float &hi)
{
    return v >= lo && v <= hi;
}

/* whether a point lies over the box in x and z (ai_map.cpp's) */
inline bool _over_box(const CBBox &b, const CPnt3D *pos)
{
    bool r = false;

    if (_in_span(pos->x, b.m_min.x, b.m_max.x) && _in_span(pos->z, b.m_min.z, b.m_max.z))
        r = true;
    return r;
}

/* the cell under a point (ai_map.cpp's inline, the same text: SOCOM 1's GetLocation(CPnt3D *, loc),
 * the cell given the map's id -- OVL2's copy 0x515F80; this unit emits OVL1's, 0x2A3950) */
inline bool CAiMap::GetLocation(const CPnt3D *pos, CAiMapLoc &loc) const
{
    bool inside = _over_box(m_bbox, pos);

    if (inside) {
        sceVu0FVECTOR v;
        float *p = v;

        v[0] = m_idx * (pos->x - m_bbox.m_min.x);
        v[1] = m_idy * (pos->z - m_bbox.m_min.z);
        asm volatile {
            lqc2 vf3, 0(p)
            vftoi0.xyzw vf3, vf3
            sqc2 vf3, 0(p)
        }
        loc.x = ((int *)p)[0];
        loc.y = (unsigned short)((int *)p)[1];
        loc.m_mapid = (unsigned char)GetMapId();
        bool on = false;
        bool ret = false;

        if (loc.y >= 0 && loc.y < m_ny)
            on = true;
        if (on && loc.x < m_nx)
            ret = true;
        return ret;
    }
    return false;
}

/* the cell's bilinear bit, and its corners' blended height at a point (OVL1 0x2A3930, its draft;
 * SOCOM 1's CAiMap::GetBilinearAltitude 0x19A820, 0x2A3170 here: unwritten -- both by placeholder) */
extern "C" {
bool func_002A3930(const CAiCell *cell);
bool func_002A3170(const CAiMap *map, const CAiMapLoc &loc, const CPnt3D *pos, float *alt);
}

/* the cell's map (ours: GetMapByIndex wrapped in an inline gives its result the lowest saved
 * register, as the query's map has it -- named in the caller, the map and the cell trade s0/s1;
 * CAiMaps::GetMap is out of line in OVL2) */
inline CAiMap *LocMap(const CAiMapLoc &loc)
{
    return AiMaps.GetMapByIndex(loc.m_mapid);
}

/* a map's height at a point: the cell under it, its corners blended when the cell asks for it, its
 * own height else; false off the map (SOCOM 1's, inlined in its ftsGetAltitudeApprox; the name
 * ours). Past inline_max_size with GetCell inlined in it, so the query inlines it one level: the
 * cell's constructor, GetLocation, GetCell and the cell's altitude called out of line, as the
 * target's */
inline bool GetMapAltitude(const CAiMap *map, const CPnt3D *pos, float &alt)
{
    CAiMapLoc loc;

    if (map->GetLocation(pos, loc)) {
        CAiCell *cell = map->GetCell(loc, 0);

        if (cell)
            return func_002A3930(cell) ? func_002A3170(map, loc, pos, &alt) : map->GetAltitude(cell, alt);
    }
    return false;
}

/* the ground under a point raised by a height: from the AI maps where they know it -- the cell under
 * the raised point neither of bad altitude nor of soil 10, the height at the point itself, the map's
 * extra byte 3 the dummy polygon's material -- as `hit`, the dummy its polygon, the world's up its
 * normal; otherwise, or for an exact query, the world's own (SOCOM 1's, the same statics: the AI maps
 * on, the world forced off -- MW 3.0 lays both out statically, no guards) */
zdb::IntersectStruct *ftsGetAltitudeApprox(const CPnt3D *pos, float h, bool exact)
{
    static bool useAiMaps = true;
    static bool forceWorld = false;
    bool found = false;

    if (useAiMaps && !exact) {
        CPnt3D p(pos->x, pos->y + h, pos->z);
        CAiMapLoc loc;

        if (AiMaps.GetLocation(&p, loc)) {
            CAiMap *map = LocMap(loc);
            CAiCell *cell = map->GetCell(loc, 0);
            float alt = 0.0f;

            if (!cell->m_badalt && cell->m_soil != 10) {
                if (GetMapAltitude(map, pos, alt)) {
                    char *ex = map->GetMapEx(loc);

                    if (ex) {
                        hit.m_node = 0;
                        found = true;
                        hit.m_norm = CPnt3D::unit_y;
                        hit.m_pos = CPnt3D(pos->x, alt, pos->z);
                        hit.m_poly = &di;
                        di.SetMaterial(ex[3]);
                    }
                }
            }
        }
    }
    if (!found || forceWorld)
        return zdb::CWorld::m_world->GetAltitude(pos, h, true);
    return &hit;
}
