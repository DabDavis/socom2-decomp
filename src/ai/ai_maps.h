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
    CAiMap::name *GetCellName(const CAiMapLoc &loc) const;
    const char *GetCellNameString(const CAiMapLoc &loc) const;
    CAiMap::link *GetCellLink(const CAiMapLoc &loc);
    CAiMap::action *GetCellAction(const CAiMapLoc &loc);
    CAiMap::region *GetRegion(const CAiMapLoc &loc);
    CAiMap::region *GetRegion(const char *name, CAiMapLoc &loc);
    bool GetActionById(unsigned int id, CAiMapLoc &loc, CAiMap::action **pa);
    bool GetLocation(const char *name, CAiMapLoc &loc);

    CAiMap::spores m_spores;   // 0x30
    CAiMap::names m_names[6];   // 0x3c
    CAiMapLocVec m_regions[7];   // 0x84
    CGraph *m_graph;   // 0xd8
    unsigned char m_cookie;   // 0xdc
};

extern CAiMaps AiMaps;

#endif
