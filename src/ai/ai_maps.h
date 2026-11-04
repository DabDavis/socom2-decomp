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
    /* SOCOM 1's m_reserved[0]: bit 1 set while ReadMapSet reads the maps in (SOCOM II's; the name
     * ours) */
    unsigned int m_flags;   // 0x4
    unsigned int m_reserved[7];   // 0x8
};

/* a mission's AI maps, by index (SOCOM 1's DWARF: CAiMaps : list, AIMAPS_PARAMS, 0xE0 bytes; the
 * same in SOCOM II -- __sinit_ai_maps.cpp (0x668710) builds AiMaps at 0x66A1B0 so and registers
 * ~CAiMaps, 0x52EA10). A cell's map is GetMapByIndex(loc.m_mapid); each lookup by cell asks that
 * map, each lookup by name every map in turn, giving the cell the index of the one that knew it */
class CAiMaps : public std::list<CAiMap *>, public AIMAPS_PARAMS {
public:
    /* every map's cells reset and the overlays dropped (SOCOM 1's Reset: CMission::ReOpen_Load
     * calls it; the overlays new) */
    void Reset();
    /* the planner flushed, the maps and their tables freed, the overlays dropped, the cookie bumped
     * (SOCOM 1's Close, its middle moved out to FreeMaps) */
    void Close();
    /* every map deleted, then the tables freed (new in SOCOM II, the name ours: the body of SOCOM
     * 1's Close between the flush and the cookie, out of line -- Close is its only caller) */
    void FreeMaps();
    /* the spores, the name tables and the regions emptied and their storage given back, the graph
     * destroyed (SOCOM 1's FreeTables, 0x52E7C0; not written yet) */
    void FreeTables();
    CAiMap *GetMapByIndex(unsigned int i) const;
    /* the cell's map (new in SOCOM II, the name ours) */
    CAiMap *GetMap(const CAiMapLoc &loc) const;
    CAiCell *GetCell(const CAiMapLoc &loc);
    /* the cell's map's repel parameters, false off the maps (SOCOM 1's; ai_map.cpp's 0x516F10, which
     * asks AiMaps, not `this`, as SOCOM 1's does) */
    bool GetCellRepelParams(const CAiMapLoc &loc, const CPnt3D *pos, float range, CPnt3D &dir, float &len,
                            CAiMapLoc &cell);
    unsigned int GetBoundary(const CAiMapLoc &loc);
    bool GetAltitude(const CAiMapLoc &loc, float &alt);
    /* the shadow at a point over a cell, blended over the cell and its three neighbours toward
     * the point, 0 to 1; 1 off the maps (SOCOM 1's; 0x52CE10 -- the explosions' and the
     * projectiles' light) */
    float GetInterpolatedShadow(const CAiMapLoc &loc, const CPnt3D *pos, bool inside);
    bool CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out);
    bool CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out);
    /* how many cells apart two cells are, across maps on the first map's grid (SOCOM 1's
     * GetLocationDelta, by its place: 0x52C320) */
    unsigned int GetLocationDelta(const CAiMapLoc &a, const CAiMapLoc &b);
    /* a path planned from a cell through the given cells (SOCOM 1's RegisterPathPlan, 0x52C5C0:
     * CSealCtrlAi::SetRoute calls it; not written yet), and the one-goal form over it (SOCOM 1's
     * three-argument RegisterPathPlan, 0x52C5B0: the goal passed as a run of one) */
    bool RegisterPathPlan(const CAiMapLoc &start, const CAiMapLoc *goals, int count, CAiPath *path);
    bool RegisterPathPlan(const CAiMapLoc &start, const CAiMapLoc &goal, CAiPath *path);
    /* a path stretched from a cell to another: the cell appended when the feet reach it, or a
     * template path's points between them (SOCOM 1's ExtendPath took the goal alone: 0x52BDF0) */
    bool ExtendPath(CAiPath &path, const CAiMapLoc &from, const CAiMapLoc &to);
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
    /* the cell's centre, the point handed back, none off the maps (SOCOM 1's) */
    CPnt3D *GetCellCenter(const CAiMapLoc &loc, CPnt3D *center);
    /* the distance between two cells' centres, squared and not (new in SOCOM II, the names ours:
     * 0 for one cell, 1e12 when a centre is missing) */
    float GetDistanceSq(const CAiMapLoc &a, const CAiMapLoc &b);
    float GetDistance(const CAiMapLoc &a, const CAiMapLoc &b);
    /* how many spores an id has (new in SOCOM II, the name ours) */
    int GetSporeCount(unsigned int id);
    /* the n-th spore of an id: its cell's centre and the way it faces, false when the id has none
     * or the cell no centre (SOCOM 1's; 0x52B5C0) */
    bool GetSporeLocation(unsigned int id, unsigned int n, CPnt3D *pos, CPnt3D *dir);
    /* the region of a type whose centre is nearest the cell, one holding the cell passed over
     * unless asked for (SOCOM 1's; CAiSFlee::Tick calls it) */
    CAiMap::region *GetClosestRegion(const CAiMapLoc &loc, CAiMap::region::TYPE type, bool inside);
    /* the region of a type holding the cell: each listed region of the type asked in turn
     * (SOCOM 1's; the SEALs' code calls it) */
    CAiMap::region *GetRegionTypeContainer(CAiMap::region::TYPE type, const CAiMapLoc &loc);
    /* a region moved to another type's list, whether it was of that type already (SOCOM 1's; the
     * task commands' CTPaint and CTMap call it) */
    bool SetRegionType(const CAiMap::region *r, CAiMap::region::TYPE type);
    /* a random cell of the region, drawn up to eight times until one is traversable (SOCOM 1's) */
    bool GetRandomLoc(const CAiMap::region *r, CAiMapLoc &loc);
    /* a region of its map's first layer tiled, the planner's queue reset after (SOCOM 1's) */
    bool Tile(const CAiMap::region *r, unsigned int value, unsigned int flags);
    /* an entity moved from one cell to another: the old cell no longer occupied unless an overlay
     * covers it, the new one occupied (SOCOM 1's, the overlay test new: the SEALs' UpdateAiMapLoc
     * and OnDeath, which pass the entity's id, unread) */
    void SetOccupied(const CAiMapLoc &from, const CAiMapLoc &to, unsigned int id);

    /* the map set read from a file: the header, the maps, the set's graph and spores, the tables
     * built after (SOCOM 1's; 0x52B0D0) */
    bool ReadMapSet(const char *name, const char *path);
    /* the tables built from the maps (SOCOM 1's, called by ReadMapSet in the demo's order:
     * 0x52BB70, 0x52B820, 0x52AE20, then the set's graph 0x52A8C0) */
    void UpdateActionTables();
    void UpdateNameTables();
    void ValidateLinks();
    void CreateGraph(bool force);
    /* the set graph's edge whose link joins two cells' maps nearest the walk between them (new in
     * SOCOM II, the name ours: 0x52ACC0) */
    CGEdge *FindLinkEdge(const CAiMapLoc &from, const CAiMapLoc &to);

    CAiMap::spores m_spores;   // 0x30
    CAiMap::names m_names[6];   // 0x3c
    CAiMapLocVec m_regions[7];   // 0x84
    CGraph *m_graph;   // 0xd8
    unsigned char m_cookie;   // 0xdc
    /* the maps file's path as ReadMapSet opened it (new in SOCOM II: strncpy'd 512 bytes; the name
     * ours) -- AiMaps is 0x2E0 bytes, SOCOM 1's 0xE0 */
    char m_path[512];   // 0xdd
};

extern CAiMaps AiMaps;

#endif
