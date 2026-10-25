/* hud_linemap.cpp -- the map's lines (CZLineMap): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_linemap.cpp; the unnamed namespace's mangling, @unnamed@hud_linemap_cpp@,
 * names it in SOCOM II too), OVL1 0x20D390-0x210AA0: Draw, Clip, the tick of the way points,
 * regions and lines, the point iterator, Tick, the polygon set-up, LineTo, the constructor and the
 * line and polygon types' readers, as SOCOM 1's. Written: the polygon and line types' readers
 * (0x210140, 0x210210), the point iterator's IsValid (0x20F1C0) and GetPt (0x210A70), as SOCOM 1's (its lines 37-38:
 * inlines of the class's, their copies linked from here). Defined in reverse of their image order
 * (-inline deferred). */

#include "../../gamez/zmath/zmath.h"
#include <map>

/* a point iterator over a line (SOCOM 1's CPtItr: only its vtable) */
class CPtItr {
public:
    virtual PNT2D GetPt() const = 0;
    virtual bool IsValid() const = 0;
};

class CAiMapLoc;

/* an AI map line as the iterator reads it (SOCOM 1's `line : CAiMapLocVec, line_data`: the
 * vector of its locations first, its size at 4 and its elements at 8) */
class line {
public:
    CAiMapLoc **end() const { return m_data + m_size; }

    unsigned int _0;
    unsigned int m_size;   // 0x4
    CAiMapLoc **m_data;   // 0x8
};

namespace {

/* the AI map's line walked point by point (SOCOM 1's DWARF; the references' targets not
 * declared here) */
class CAiLinePtItr : public CPtItr {
public:
    virtual PNT2D GetPt() const;
    virtual bool IsValid() const;

    line *_line;   // 0x4 (a reference to the line)
    CAiMapLoc **_it;   // 0x8
    void *_map;   // 0xc (a reference to the line map)
    CPnt3D _pt;   // 0x10
};

/* whether the walk is still on the line (SOCOM 1's line 38, an inline there; SOCOM II's vtable
 * slot 2, the iterators' `!=` written as `!(==)`) */
bool CAiLinePtItr::IsValid() const
{
    return !(_it == _line->end());
}

/* the point's first two coordinates as a 2D point (SOCOM 1's: through a local, assigned -- a brace
 * initialiser zeroes it first) */
PNT2D CAiLinePtItr::GetPt() const
{
    PNT2D pt;

    pt.x = _pt.x;
    pt.y = _pt.y;
    return pt;
}

}

struct _zrdr;

/* the reader's finders (zrdr; the int and PNT3D ones' rows not named yet: called as C) */
extern "C" int func_0032E970(const _zrdr *rdr, const char *name, int *value, int n);   /* zrdr_findint */
extern "C" int func_0032DE80(const _zrdr *rdr, const char *name, PNT3D *value);        /* zrdr_findPNT3D */
int zrdr_findreal(const _zrdr *rdr, const char *name, float *value, int n);

/* a line or polygon type's look (SOCOM 1's DWARF); built white, half opaque, one wide -- the
 * colour's three stores in reverse, as one chained assignment gives them (operator[]'s new
 * element: the tree's find_or_insert, 0x210AA0) */
class LINE_DATA {
public:
    LINE_DATA()
    {
        color.x = color.y = color.z = 255.0f;
        opacity = 128.0f;
        _fWidth = 1.0f;
    }

    PNT3D color;   // 0x0
    float opacity;   // 0xc
    float _fWidth;   // 0x10
};

namespace {

/* the types by id (SOCOM 1's statics; OVL1 bss 0x40BF40, 0x40BF60, built and registered for
 * destruction by the unit's initialiser, 0x3FE190). The tree's members are this unit's tail,
 * 0x210AA0-0x2113C0 (include/tree) */
std::map<int, LINE_DATA> g_mapLineProps;
std::map<int, LINE_DATA> g_mapPolyProps;

}

/* a type's colour, opacity (0-1, kept as 0-127) and width out of the reader (SOCOM 1's one line:
 * an inline) */
static inline void ReadLineData(_zrdr *pRdr, LINE_DATA &data)
{
    func_0032DE80(pRdr, "Color", &data.color);

    float opacity = 1.0f;

    zrdr_findreal(pRdr, "Opacity", &opacity, 1);
    data.opacity = 127.0f * opacity;

    float width = 1.0f;

    zrdr_findreal(pRdr, "Width", &width, 1);
    data._fWidth = width;
}

/* a line type from its reader, by its Type */
void hudAddLineType(_zrdr *pRdr)
{
    int type = 0;

    func_0032E970(pRdr, "Type", &type, 1);
    ReadLineData(pRdr, g_mapLineProps[type]);
}

/* a polygon type, the same way */
void hudAddPolyType(_zrdr *pRdr)
{
    int type = 0;

    func_0032E970(pRdr, "Type", &type, 1);
    ReadLineData(pRdr, g_mapPolyProps[type]);
}
