/* ai_maps.h -- a mission's AI maps, CAiMaps (ai_maps.cpp). Apart from ai.h: zseal.h and two
 * units declare stand-ins of their own for the few calls they make, which this class would
 * rival in any unit including both. */
#ifndef AI_MAPS_H
#define AI_MAPS_H

#include "ai.h"

/* the maps file's header (SOCOM 1's DWARF, 0x24 bytes) */
class AIMAPS_PARAMS {
public:
    unsigned int m_map_count;   // 0x0
    unsigned int m_reserved[8];   // 0x4
};

/* a mission's AI maps, by index (SOCOM 1's DWARF: CAiMaps : list, AIMAPS_PARAMS, 0xE0 bytes; the
 * same in SOCOM II -- __sinit_ai_maps.cpp (0x668710) builds AiMaps at 0x66A1B0 so and registers
 * ~CAiMaps, 0x52EA10). A cell's map is GetMapByIndex(loc.m_mapid); each lookup by cell asks that
 * map, each lookup by name every map in turn, giving the cell the index of the one that knew it */
class CAiMaps : public std::list<CAiMap *>, public AIMAPS_PARAMS {
public:
    CAiMap *GetMapByIndex(unsigned int i) const;
    /* the cell's map (new in SOCOM II, the name ours) */
    CAiMap *GetMap(const CAiMapLoc &loc) const;
    CAiCell *GetCell(const CAiMapLoc &loc);
    unsigned int GetBoundary(const CAiMapLoc &loc);
    bool GetAltitude(const CAiMapLoc &loc, float &alt);
    bool CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out);
    bool CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out);
    /* a line between two points walked over the maps, each cell handed to a callback (new in
     * SOCOM II, the name ours: the visibility tests' walk, 0x5352D0, the step 0.2 as the maps'
     * own WalkLines take it) */
    bool WalkLine(const CPnt3D *from, const CPnt3D *to, bool (*f)(const MAP_LINEWALK_STRUCT &s, void *data),
                  void *data, float step);
    /* how much of a line can be seen through, 0 to 1, between two points or from a cell's centre
     * raised by a height (new in SOCOM II, the names after SOCOM 1's ComputeLineVisibility: the
     * walker unit's 0x538E30 and 0x538D10, the AI states' line-of-sight tests) */
    float ComputeLineVisibility(const CPnt3D *from, const CPnt3D *to, unsigned int flags);
    float ComputeLineVisibility(const CAiMapLoc &from, const CPnt3D *to, unsigned int flags, float height);
    /* whether a point is seen from another, by chance of the visibility the walk leaves (new in
     * SOCOM II, the name ours: 0x538980) */
    bool CanSeePoint(const CPnt3D *from, const CPnt3D *to, float rangeSq);
    CAiMap::name *GetCellName(const CAiMapLoc &loc) const;
    const char *GetCellNameString(const CAiMapLoc &loc) const;
    CAiMap::link *GetCellLink(const CAiMapLoc &loc);
    CAiMap::action *GetCellAction(const CAiMapLoc &loc);
    CAiMap::region *GetRegion(const CAiMapLoc &loc);
    CAiMap::region *GetRegion(const char *name, CAiMapLoc &loc);
    bool GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **pa);
    bool GetLocation(const char *name, CAiMapLoc &loc);
    bool GetLocation(const CPnt3D *pos, CAiMapLoc &loc);
    /* a cell a step may end on near a cell, near a point (new in SOCOM II, the names ours: the
     * AI states' pickup, no-ammo and vehicle searches) */
    bool FindTraversable(const CAiMapLoc &loc, CAiMapLoc &out);
    bool GetTraversableLocation(const CPnt3D *pos, CAiMapLoc &out);
    bool GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
    /* the distance between two cells' centres, squared and not (new in SOCOM II, the names ours:
     * 0 for one cell, 1e12 when a centre is missing) */
    float GetDistanceSq(const CAiMapLoc &a, const CAiMapLoc &b);
    float GetDistance(const CAiMapLoc &a, const CAiMapLoc &b);
    /* how many spores an id has (new in SOCOM II, the name ours) */
    int GetSporeCount(unsigned int id);

    CAiMap::spores m_spores;   // 0x30
    CAiMap::names m_names[6];   // 0x3c
    CAiMapLocVec m_regions[7];   // 0x84
    CGraph *m_graph;   // 0xd8
    unsigned char m_cookie;   // 0xdc
};

extern CAiMaps AiMaps;

#endif
