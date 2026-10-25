/* ai_mapwalk.cpp (OVL2 ~0x5350C0-0x539BBF, the name ours): the AI map's walkers -- SOCOM 1's unit
 * after vehicle_rdr.cpp (the demo's 0x1A5400-0x1A9180: CountMapTravelDir, CountMapVisDir, Tile and
 * its callbacks, FloodFill, the two WalkLines, ComputeLineVisibility, CanSeeLine x2, CanTravelLine,
 * the line callbacks, GetLocationWidth, then the template tail with _saturate<f> and the CAiMapLoc
 * list's pieces -- OVL2 0x539B70 on). Only the line tests so far (0x539070-0x5390CF). Defined
 * here in reverse of the image's order (-inline deferred). */

#include "ai.h"

/* the line walks' callbacks (SOCOM 1's _SeeLineCallback and _TravelLineCallback, static here:
 * placeholders until written) */
extern "C" bool func_00539400(const MAP_LINEWALK_STRUCT &s, void *data);
extern "C" bool func_00539580(const MAP_LINEWALK_STRUCT &s, void *data);

bool CAiMap::CanTravelLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, func_00539580, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CAiMapLoc &to, CAiMapLoc *out)
{
    return WalkLine(from, to, func_00539400, out, 0.2f);
}

bool CAiMap::CanSeeLine(const CAiMapLoc &from, const CPnt3D *dir, CAiMapLoc *out)
{
    return WalkLine(from, dir, func_00539400, out, 0.2f);
}
