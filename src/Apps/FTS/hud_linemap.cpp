/* hud_linemap.cpp -- the map's lines (CZLineMap): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_linemap.cpp; the unnamed namespace's mangling, @unnamed@hud_linemap_cpp@,
 * names it in SOCOM II too), OVL1 0x20D390-0x210AA0: Draw, Clip, the tick of the way points,
 * regions and lines, the point iterator, Tick, the polygon set-up, LineTo, the constructor and the
 * line and polygon types' readers, as SOCOM 1's. Written: the polygon and line types' readers
 * (0x210140, 0x210210), the point iterator's IsValid (0x20F1C0) and GetPt (0x210A70), as SOCOM 1's (its lines 37-38:
 * inlines of the class's, their copies linked from here); (round 35) SetupWideLine (0x20EF50, SOCOM
 * II's body: the C2D library builds the wide line), the iterator's FindValid (0x20F100) and Advance
 * (0x2107C0), CalcPoint (0x210870), and with them every copy they leave out of line -- ai.h's
 * CAiMap::GetCellCenter (0x20F220), GetCell (0x2106A0), GetAltitude (0x210630, made inline here),
 * IsMapCell, GetIndex, IsLayer and the vector pieces, all on the AI core's real ai.h. Defined in
 * reverse of their image order (-inline deferred). */

#include "../../gamez/zmath/zmath.h"
#include "../../ai/ai.h"
#include "hud_newhudmap.h"
#include <map>

/* a cell's altitude: the box's floor plus the quantum times the cell's altitude bits (ai_map.cpp's
 * 0x516940 -- an inline in SOCOM II: OVL1 has its copy, 0x210630, and GetCellCenter's out-of-line
 * copy 0x20F220 inlines it whole; ai.h declares it out of line, so it is made inline here) */
inline bool CAiMap::GetAltitude(const CAiCell *cell, float &alt) const
{
    if (cell) {
        alt = m_bbox.m_min.y + m_alt_quant * (float)cell->m_altitude;
        return true;
    }
    return false;
}

/* a point iterator over a line (SOCOM 1's CPtItr: only its vtable -- the walk's step, the point,
 * whether it is still on the line; CPtItr's own table 0x405560 holds three zeros) */
class CPtItr {
public:
    CPtItr() {}
    virtual void Advance() = 0;
    virtual PNT2D GetPt() const = 0;
    virtual bool IsValid() const = 0;
};

struct LINE_DATA;

/* a wide line's polygons (SOCOM II's; the name ours): coloured and faded all at once -- the colour
 * read through its reference for each one, the fade's 0-128 scale taken once -- and deleted with
 * the set (~CZLineMap's delete of a set: the null test merged with the set's own this-test, its
 * walk, then the vector's three) */
class CPolySet : public std::vector<C2DBitmapPoly *> {
public:
    ~CPolySet()
    {
        for (int i = 0; i < size(); i++)
            delete (*this)[i];
    }
    void SetColor(const PNT3D &c)
    {
        for (int i = 0; i < size(); i++)
            (*this)[i]->SetColor(c.x, c.y, c.z);
    }
    void SetTrans(float t)
    {
        for (int i = 0; i < size(); i++)
            (*this)[i]->SetTrans(128.0f * t);
    }
};

/* the map's lines, regions and way points (SOCOM 1's CZLineMap: a map item; SOCOM II's grew past
 * 0x14BC8, where its polygon sets are) */
class CZLineMap : public CZMapItem {
public:
    CZLineMap();
    virtual ~CZLineMap();
    virtual bool Tick(CZNewHudMap *map, float dT);
    virtual void Draw(zdb::CCamera *camera);
    virtual void Clip(CZNewHudMap *map);

    /* a wide line's polygons, built along its cells (SOCOM II's, the name ours: TickLine's 0x20EF50) */
    void SetupWideLine(const CAiMap::line &line, const LINE_DATA &data);
    /* the map's lines ticked: each drawn as a line unless a polygon without its outline, each
     * polygon filled with triangles (SOCOM 1's TickLines, TickLine and SetupPolygon, 0x20E370,
     * 0x20E470, 0x20F5C0) */
    void TickLines(const CAiMap &map, float fPulseTrans, bool bDim);
    void TickLine(const CAiMap::line &line, float fPulseTrans, bool bDim);
    void SetupPolygon(const CAiMap::line &line, float fPulseTrans, bool bDim);
    /* the pen put on a cell, and a line drawn to a point or a cell (SOCOM 1's LineTo, the cell's
     * forms SOCOM II's; the line drawn back, none when the cell has no point) */
    void MoveTo(const CAiMapLoc &loc);
    C2DLine *LineTo(const CPnt3D *p);
    C2DLine *LineTo(const CAiMapLoc &loc);
    C2DLine *NewLine();
    /* a world point as the map draws it (SOCOM 1's WorldToDraw; out of line at 0x20F210) */
    CPnt3D WorldToDraw(const CPnt3D *p) const { return m_hudmap->MapPoint(p); }
    /* a cell's drawn point, false for a cell off the map (the name ours: the iterator's walks;
     * out of line at 0x210870, the centre inlined one level there) */
    bool CalcPoint(CPnt3D *pt, const CAiMapLoc &loc)
    {
        if (!m_map->GetCellCenter(loc, pt))
            return false;
        *pt = WorldToDraw(pt);
        return true;
    }

    static CPnt4D m_pi[2];
    static CPnt4D m_rgbi[2];
    static CPnt4D m_uvi[2];
    static CPnt4D m_po[2];
    static CPnt4D m_rgbo[2];
    static CPnt4D m_uvo[2];

    CPnt3D m_pointA;   // 0x4
    CPnt3D m_pointB;   // 0x10
    CAiMap *m_map;   // 0x1c
    CZNewHudMap *m_hudmap;   // 0x20
    C2DLine m_linelist[500];   // 0x30 (SOCOM 1's 400)
    unsigned int m_lastline;   // 0x138b0
    C2DString m_stringlist[32];   // 0x138b4
    unsigned int m_laststring;   // 0x14bb4
    std::vector<C2DPoly *> m_vecTriangles;   // 0x14bb8
    int m_iNumActiveTriangles;   // 0x14bc4
    std::vector<CPolySet *> m_vecWideLines;   // 0x14bc8: SOCOM II's wide lines' polygon sets
    std::vector<C2DBitmap *> m_vecBitmaps;   // 0x14bd4
    zdb::CTexture *m_texture;   // 0x14be0 (SOCOM 1's at 0x10b60)
};

namespace {

/* the AI map's line walked point by point (SOCOM 1's DWARF: references to the line and the line
 * map, the cell iterator, the cell's drawn point) */
class CAiLinePtItr : public CPtItr {
public:
    /* the walk from the line's first cell (SOCOM 1's; inlined one level where it is made: the base
     * constructor, begin(), the iterator's copy, CPnt3D() and FindValid called) */
    CAiLinePtItr(const CAiMap::line &line, CZLineMap &map) : _line(line), _it(line.begin()), _map(map)
    {
        FindValid();
    }
    virtual void Advance();
    virtual PNT2D GetPt() const;
    virtual bool IsValid() const;

    /* the first cell from here that has a point (SOCOM 1's line 42; out of line at 0x20F100 where
     * the constructor is inlined one level, and inlined one level in Advance -- the point, the
     * iterator's * and postfix ++ called there, 0x210870, 0x210A60, 0x210850) */
    void FindValid()
    {
        while (IsValid()) {
            if (_map.CalcPoint(&_pt, *_it))
                break;
            _it++;
        }
    }

    const CAiMap::line &_line;   // 0x4
    CAiMapLocVec::const_iterator _it;   // 0x8
    CZLineMap &_map;   // 0xc
    CPnt3D _pt;   // 0x10
};

}

struct _zrdr;

/* the reader's finders (zrdr) */
int zrdr_findint(const _zrdr *rdr, const char *name, int *value, int n);
int zrdr_findPNT3D(const _zrdr *rdr, const char *name, PNT3D *value);
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
    /* a type's look given (the default table's: SOCOM 1's initialiser builds each entry as a
     * temporary, opacity and width first, then the colour, and copies it in) */
    LINE_DATA(float r, float g, float b, float o, float w)
    {
        opacity = o;
        _fWidth = w;
        color.x = r;
        color.y = g;
        color.z = b;
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

/* the line map there is (SOCOM II's, the name ours: OVL1 bss 0x40BF70, set by the constructor and
 * cleared by the destructor) */
CZLineMap *g_pLineMap;

}

/* the default looks, nine (SOCOM 1's file static _linedata, 180 bytes; OVL1 .data 0x3E42D0-0x3E4383,
 * stored by the unit's initialiser) */
static LINE_DATA _linedata[9] = {
    LINE_DATA(200.0f, 200.0f, 200.0f, 100.0f, 1.0f),
    LINE_DATA(200.0f, 0.0f, 200.0f, 100.0f, 1.0f),
    LINE_DATA(0.0f, 255.0f, 0.0f, 100.0f, 1.0f),
    LINE_DATA(30.0f, 255.0f, 30.0f, 255.0f, 1.0f),
    LINE_DATA(60.0f, 60.0f, 60.0f, 100.0f, 1.0f),
    LINE_DATA(200.0f, 200.0f, 0.0f, 100.0f, 1.0f),
    LINE_DATA(0.0f, 0.0f, 255.0f, 100.0f, 1.0f),
    LINE_DATA(255.0f, 0.0f, 0.0f, 100.0f, 1.0f),
    LINE_DATA(0.0f, 255.0f, 0.0f, 100.0f, 1.0f),
};

/* the line map's clip buffers (SOCOM 1's statics: m_pi, m_rgbi, m_uvi, m_po, m_rgbo, m_uvo, two
 * CPnt4Ds each -- the points, colours and UVs in and out of the clipper; OVL1 bss 0x40BF80-0x40C03F,
 * built by the initialiser through __construct_array with CPnt4D's empty constructor, 0x2110E0) */
CPnt4D CZLineMap::m_pi[2];
CPnt4D CZLineMap::m_rgbi[2];
CPnt4D CZLineMap::m_uvi[2];
CPnt4D CZLineMap::m_po[2];
CPnt4D CZLineMap::m_rgbo[2];
CPnt4D CZLineMap::m_uvo[2];

namespace {

/* the point's first two coordinates as a 2D point (SOCOM 1's: through a local, assigned -- a brace
 * initialiser zeroes it first) */
PNT2D CAiLinePtItr::GetPt() const
{
    PNT2D pt;

    pt.x = _pt.x;
    pt.y = _pt.y;
    return pt;
}

/* the next cell that has a point (SOCOM 1's line 36: the step, then FindValid -- inlined one level
 * here, its point, * and postfix ++ called) */
void CAiLinePtItr::Advance()
{
    ++_it;
    FindValid();
}

}

/* the map's box, by value (the name ours: the cell's fallback reads it from a copy on the stack) */
static inline CBBox GetMapBox(const CAiMap *map)
{
    return map->m_bbox;
}

/* a cell's point in the world: its centre, or for a cell with none its corner plus half a cell on
 * the ground; none for no map or the invalid cell (SOCOM II's, the name ours: GetCellCenter inlined
 * one level -- GetCell, GetAltitude, HasAction, GetDir, CPnt3D(), Scale and Add called, their copies
 * after this, 0x2105E0-0x2106A0, in reverse of first use) */
CPnt3D *GetLocPoint(const CAiMap *map, const CAiMapLoc &loc, CPnt3D *pt)
{
    CPnt3D *r = 0;

    if (map && loc != CAiMapLoc::invalid) {
        if (!((CAiMap *)map)->GetCellCenter(loc, pt)) {
            CBBox box = GetMapBox(map);
            float dx = map->m_dx;
            float dy = map->m_dy;

            pt->x = box.m_min.x + (loc.x * dx + 0.5f * dx);
            pt->y = 0.0f;
            pt->z = box.m_min.z + (loc.y * dy + 0.5f * dy);
        }
        r = pt;
    }
    return r;
}

/* a type's colour, opacity (0-1, kept as 0-127) and width out of the reader (SOCOM 1's one line:
 * an inline) */
static inline void ReadLineData(_zrdr *pRdr, LINE_DATA &data)
{
    zrdr_findPNT3D(pRdr, "Color", &data.color);

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

    zrdr_findint(pRdr, "Type", &type, 1);
    ReadLineData(pRdr, g_mapLineProps[type]);
}

/* a polygon type, the same way */
void hudAddPolyType(_zrdr *pRdr)
{
    int type = 0;

    zrdr_findint(pRdr, "Type", &type, 1);
    ReadLineData(pRdr, g_mapPolyProps[type]);
}

namespace {

/* the tactical map's pulse, as an alpha (SOCOM 1's file static: CZLineMap::Tick drives it; OVL1
 * .data 0x3DC4C0, 60 at first) */
float g_fPulseTrans = 60.0f;

}

/* the pulse for the other map items (SOCOM 1's: CZPlayerMapItem::SetViewCone reads it) */
float hudGetTacMapPulseTrans()
{
    return g_fPulseTrans;
}

/* no map, no lines yet; the line map registered (SOCOM 1's, SOCOM II's 500 lines, 32 strings of its
 * own size and the global) */
CZLineMap::CZLineMap() : m_map(0), m_hudmap(0), m_iNumActiveTriangles(0)
{
    g_pLineMap = this;
}

/* every triangle, wide line and bitmap deleted, the line map unregistered (SOCOM 1's; the wide
 * lines' sets SOCOM II's) */
CZLineMap::~CZLineMap()
{
    for (int i = 0; i < m_vecTriangles.size(); i++)
        delete m_vecTriangles[i];
    for (int i = 0; i < m_vecWideLines.size(); i++)
        delete m_vecWideLines[i];
    for (int i = 0; i < m_vecBitmaps.size(); i++)
        delete m_vecBitmaps[i];
    g_pLineMap = 0;
}

/* a line to a cell's point, none for a cell without one (SOCOM II's) */
C2DLine *CZLineMap::LineTo(const CAiMapLoc &loc)
{
    CPnt3D pt;

    if (GetLocPoint(m_map, loc, &pt))
        return LineTo(&pt);
    return 0;
}

/* whether a line between two drawn points can show: not both ends past the same side of the map's
 * screen box (SOCOM 1's four tests, each pair materialised before the next) */
static inline bool IsLineVisible(const CZNewHudMap *map, const CPnt3D &a, const CPnt3D &b)
{
    return !((a.x < map->m_screenx1 && b.x < map->m_screenx1) || (a.x > map->m_screenx2 && b.x > map->m_screenx2)
             || (a.y < map->m_screeny1 && b.y < map->m_screeny1) || (a.y > map->m_screeny2 && b.y > map->m_screeny2));
}

/* the next of the line list's lines, none once all 500 are out (SOCOM 1's, its 400) */
inline C2DLine *CZLineMap::NewLine()
{
    if (m_lastline < 500) {
        m_lastline++;
        return &m_linelist[m_lastline - 1];
    }
    return 0;
}

/* a line from the pen to a point, the pen moved there; kept only when it can show (SOCOM 1's, the
 * point through MapPoint in SOCOM II).
 * OPEN, 8 bytes long (0x230 vs 0x228) and this/line swapped (the target keeps this in s0, the line
 * in s1): the target zeroes the line in the visibility test's delay slot, and its 500-line test's
 * true arm ends `b join` over a lone nop while the false arm adds nothing -- NewLine with an early
 * return gives the b but a `move s0,zero; nop` false arm, a result variable (or the test written
 * here) drops both (0x220); a ternary, an if/else with `line = line`, a switch, the inverted early
 * return, an else-arm zero and the ternary of the visibility all miss (round 35) */
C2DLine *CZLineMap::LineTo(const CPnt3D *p)
{
    m_pointB = WorldToDraw(p);

    C2DLine *line = 0;

    if (IsLineVisible(m_hudmap, m_pointA, m_pointB)) {
        line = NewLine();
        if (line) {
            line->Load(m_pointA.x, m_pointA.y, m_pointB.x, m_pointB.y, m_texture);
            line->SetUV(0.5f, 0.9375f, 0.5f, 0.9375f);
            line->On();
        }
    }
    m_pointA = m_pointB;
    return line;
}

/* the pen on a cell's drawn point (SOCOM II's) */
void CZLineMap::MoveTo(const CAiMapLoc &loc)
{
    CPnt3D pt;

    if (GetLocPoint(m_map, loc, &pt))
        m_pointA = WorldToDraw(&pt);
}

namespace {

/* whether the walk is still on the line (SOCOM 1's line 38, an inline there; SOCOM II's vtable
 * slot 2, the iterators' `!=` written as `!(==)`) */
bool CAiLinePtItr::IsValid() const
{
    return !(_it == _line.end());
}

}

/* the wide line's polygons along a point walk (the C2D library's, OVL1 0x357F30; unwritten) */
extern "C" void func_00357F30(CPolySet *polys, CPtItr *itr, float width);

/* a wide line's polygons: a new set kept, built along the line's cells, coloured and faded as the type
 * says (SOCOM II's; the name ours -- SOCOM 1's SetupPolygon is the polygon fill, 0x20F5C0) */
void CZLineMap::SetupWideLine(const CAiMap::line &line, const LINE_DATA &data)
{
    CPolySet *polys = new CPolySet;

    m_vecWideLines.push_back(polys);

    CAiLinePtItr itr(line, *this);

    func_00357F30(polys, &itr, data._fWidth);
    polys->SetColor(data.color);
    polys->SetTrans((1.0f / 128.0f) * data.opacity);
}

/* each of the map's lines: drawn as a line unless it is a polygon without its outline, and a
 * polygon filled (SOCOM 1's TickLines; the polygon's own tick, 0x20F5C0, SOCOM II's) */
void CZLineMap::TickLines(const CAiMap &map, float fPulseTrans, bool bDim)
{
    const CAiMap::lines &lines = map.m_lineTable;

    for (CAiMap::lines::const_iterator i = lines.begin(); i != lines.end(); ++i) {
        bool drawn = !(bool)(*i).m_polygon || (bool)(*i).m_outline;

        if (drawn)
            TickLine(*i, fPulseTrans, bDim);
        if ((bool)(*i).m_polygon != false)
            SetupPolygon(*i, fPulseTrans, bDim);
    }
}
