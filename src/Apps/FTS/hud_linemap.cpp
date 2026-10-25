/* hud_linemap.cpp -- the map's lines (CZLineMap): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_linemap.cpp; the unnamed namespace's mangling, @unnamed@hud_linemap_cpp@,
 * names it in SOCOM II too), OVL1 0x20D390-0x210AA0: Draw, Clip, the tick of the way points,
 * regions and lines, the point iterator, Tick, the polygon set-up, LineTo, the constructor and the
 * line and polygon types' readers, as SOCOM 1's. Written: the polygon and line types' readers
 * (0x210140, 0x210210) and the point iterator's GetPt (0x210A70), as SOCOM 1's (its line 37: an
 * inline of the class's, its copy linked from here). Defined in reverse of their image order
 * (-inline deferred). */

#include "../../gamez/zmath/zmath.h"

/* a point iterator over a line (SOCOM 1's CPtItr: only its vtable) */
class CPtItr {
public:
    virtual PNT2D GetPt() const = 0;
};

namespace {

/* the AI map's line walked point by point (SOCOM 1's DWARF; the references' targets not
 * declared here) */
class CAiLinePtItr : public CPtItr {
public:
    virtual PNT2D GetPt() const;

    void *_line;   // 0x4 (a reference to the line)
    void *_it;   // 0x8
    void *_map;   // 0xc (a reference to the line map)
    CPnt3D _pt;   // 0x10
};

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

/* a line or polygon type's look (SOCOM 1's DWARF) */
class LINE_DATA {
public:
    PNT3D color;   // 0x0
    float opacity;   // 0xc
    float _fWidth;   // 0x10
};

/* the types by id: two std::map<int, LINE_DATA> (OVL1 bss; no include/map yet, so the tree's
 * find_or_insert -- operator[]'s out-of-line half -- is called by its placeholder on the maps'
 * addresses: the pair's value 4 bytes in) */
struct LINE_PAIR {
    int first;
    LINE_DATA second;
};

extern "C" LINE_PAIR *func_00210AA0(void *tree, const int &key);
extern unsigned char data_0040BF40[];   /* the line types */
extern unsigned char data_0040BF60[];   /* the polygon types */

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
    ReadLineData(pRdr, func_00210AA0(data_0040BF40, type)->second);
}

/* a polygon type, the same way */
void hudAddPolyType(_zrdr *pRdr)
{
    int type = 0;

    func_0032E970(pRdr, "Type", &type, 1);
    ReadLineData(pRdr, func_00210AA0(data_0040BF60, type)->second);
}
