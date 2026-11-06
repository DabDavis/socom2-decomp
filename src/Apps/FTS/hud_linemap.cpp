/* hud_linemap.cpp -- the map's lines (CZLineMap): SOCOM 1's unit (its DWARF names it,
 * Z:\dev\Apps\FTS\hud_linemap.cpp; the unnamed namespace's mangling, @unnamed@hud_linemap_cpp@,
 * names it in SOCOM II too), OVL1 0x20D390-0x210AA0: Draw, Clip, the tick of the way points,
 * regions and lines, the point iterator, Tick, the polygon set-up, LineTo, the constructor and the
 * line and polygon types' readers, as SOCOM 1's. Written: the polygon and line types' readers
 * (0x210140, 0x210210), the point iterator's IsValid (0x20F1C0) and GetPt (0x210A70), as SOCOM 1's (its lines 37-38:
 * inlines of the class's, their copies linked from here); (round 35) SetupWideLine (0x20EF50, SOCOM
 * II's body: the C2D library builds the wide line), the iterator's FindValid (0x20F100) and Advance
 * (0x2107C0), CalcPoint (0x210870), and with them every copy they leave out of line -- ai.h's
 * CAiMap::GetCellCenter (0x20F220), GetCell (0x2106A0), GetAltitude (0x210630, inline in ai.h),
 * IsMapCell, GetIndex, IsLayer and the vector pieces, all on the AI core's real ai.h; (round 43)
 * Tick (0x20EBD0, SOCOM 1's: its two map ticks one inline each, TickLines pinned out of line) and
 * TickRegions (0x20DF70, SOCOM 1's); (round 60) Clip (0x20D5F0, SOCOM 1's: the clipper held in a local --
 * SOCOM 1's s5 too --, zdb's CCircleClip, SetClipInfo's count passed) and TickWayPoints (0x20DAC0, SOCOM
 * II's labels with SOCOM 1's names) and SetupPolygon (0x20F5C0, SOCOM 1's; the kind-to-look table made the
 * file's, TickLine reading the same one); (round 61) TickAnchors (0x20E750: the position test through a
 * pointer an inline returns); LineTo (0x20FA70) OPEN. Defined in reverse of their image order (-inline deferred). */

#include "../../gamez/zmath/zmath.h"
#include "../../ai/ai.h"
#include "hud_newhudmap.h"
#include "hud_mapcompass.h"
#include "../../gamez/zentity/zentity.h"
#include <map>

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
    /* the map's way points and regions, dimmed on request, and the anchors pulsed (SOCOM 1's
     * TickWayPoints, TickRegions and TickAnchors, called from Tick as SOCOM 1's: 0x20DAC0,
     * 0x20DF70, 0x20E750; unwritten) */
    void TickWayPoints(const CAiMap &map, bool bDim);
    void TickRegions(const CAiMap &map, bool bDim);
    void TickAnchors(float fPulseTrans);
    /* one map's lines, way points and regions ticked, the map kept while they are (inline: SOCOM
     * 1's line table gives each of Tick's two calls one line, and its pulse is read before the map
     * is looked up -- the arguments' right-to-left order; the name ours) */
    void TickMap(CAiMap *map, float fPulseTrans, bool bDim)
    {
        m_map = map;
        if (m_map) {
            TickLines(*m_map, fPulseTrans, bDim);
            TickWayPoints(*m_map, bDim);
            TickRegions(*m_map, bDim);
        }
    }
    /* the pen put on a cell, and a line drawn to a point or a cell (SOCOM 1's LineTo, the cell's
     * forms SOCOM II's; the line drawn back, none when the cell has no point) */
    void MoveTo(const CAiMapLoc &loc);
    C2DLine *LineTo(const CPnt3D *p);
    C2DLine *LineTo(const CAiMapLoc &loc);
    C2DLine *NewLine();
    /* whether a line is one of the current objective's (SOCOM II's, the name ours) */
    bool IsObjectiveLine(const CAiMap::line &line) const;
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
/* the map's line kinds (SOCOM 1's DWARF; a byte) -- the default looks' order */
enum LINE_TYPES {
    LINETYPE_NORMAL_WHITE = 0,
    LINETYPE_EXTRACT = 1,
    LINETYPE_NAVLINE = 2,
    LINETYPE_NAVPOINT = 3,
    LINETYPE_NORMAL_GREY = 4,
    LINETYPE_NORMAL_YELLOW = 5,
    LINETYPE_NORMAL_BLUE = 6,
    LINETYPE_NORMAL_RED = 7,
    LINETYPE_NORMAL_GREEN = 8,
    LINETYPE_MAX = 9
};

/* the objective a mission shows (zfts' class; here only the map lines it lights, SOCOM II's: a
 * vector of line ids at 0x44 -- the name ours) */
class CMissionObjectiveCam {
public:
    bool HasLine(int id)
    {
        for (int i = 0; i < m_lines.size(); i++)
            if (id == m_lines[i])
                return true;
        return false;
    }

    /* the objective lit by the anchors of its letter (kinds 14-37 are the letters from C on: the
     * letter's bit in the word at 0x50; the names ours) */
    bool HasLetter(char c) const { return (m_letters & (1 << (c - 'A'))) != 0; }

    unsigned char _0[0x40];
    unsigned int _40_0 : 3;
    unsigned int m_extract : 1;   // 0x40 bit 3: an extraction objective (kind 6's anchors pulse)
    unsigned int _40_4 : 28;
    std::vector<int> m_lines;   // 0x44
    unsigned int m_letters;   // 0x50
};

/* a line's or polygon's kind (0-6) to its default look (OVL1 .data 0x3E4388, seven bytes: TickLine
 * and SetupPolygon read the one table, so it is the file's, not a function's) */
static const LINE_TYPES c_arrTypes[7] = {
    LINETYPE_NORMAL_WHITE, LINETYPE_NAVLINE, LINETYPE_NORMAL_GREY, LINETYPE_NORMAL_YELLOW,
    LINETYPE_NORMAL_BLUE, LINETYPE_NORMAL_RED, LINETYPE_NORMAL_GREEN,
};

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
 * return, an else-arm zero and the ternary of the visibility all miss (round 35). Round 58: NewLine
 * as `return m_lastline < 500 ? &m_linelist[++m_lastline - 1] : 0;` with the line initialised
 * `IsLineVisible(...) ? NewLine() : 0` gives the target's size and its this / line registers, 18
 * off: the zero then leaves the visibility test's delay slot and the count is not reloaded after
 * its store (the target's m_lastline - 1 reads it again: the if form's); the ternary alone 0x210,
 * a result variable in NewLine 0x220 (0x260 with the ternary), the comma form 0x230 / 0x240.
 * Round 62: SOCOM 1's LineTo (0x3CA5E0) is the target's shape without the `b` -- the line zeroed in
 * the visibility test's delay slot, the 400-line test's false arm empty -- so the `b join` is an
 * inline's early-return goto; NewLine with no return on its false path keeps the b but its
 * uninitialised result takes a third saved register (0x22C); NewLine filling a C2DLine *& / ** (with
 * or without `return;`, or returning a bool) drops the b (0x220) */
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

/* a polygon filled: a fan of triangles from its first cell's point over each later pair, in its type's
 * look -- the type's own, or a default one -- pulsing when the current objective lights it, dimmed on
 * request; the triangles are the map's pool, grown one at a time (SOCOM 1's SetupPolygon, its DWARF:
 * poly, pt3, pLineData, pt2, ptAnchor, itSegment; the look found as TickLine finds it). The first two
 * cells taken with a postfix step in the calls (the target folds both steps into the second call's
 * delay slot), the new triangle through a pointer local declared first (its frame slot above the
 * type's and the find's: at the push_back the slot order was 8 off), the dimming a ternary */
void CZLineMap::SetupPolygon(const CAiMap::line &line, float fPulseTrans, bool bDim)
{
    C2DPoly *pNew;

    if (line.size() < 3)
        return;

    CAiMapLocVec::const_iterator itSegment = line.begin();
    CPnt3D ptAnchor;

    if (!GetLocPoint(m_map, *itSegment++, &ptAnchor))
        return;
    ptAnchor = WorldToDraw(&ptAnchor);

    CPnt3D pt2;

    if (!GetLocPoint(m_map, *itSegment++, &pt2))
        return;
    pt2 = WorldToDraw(&pt2);

    LINE_DATA *pLineData;
    int type = line.m_type;
    std::map<int, LINE_DATA>::iterator it = g_mapPolyProps.find(type);

    if (it != g_mapPolyProps.end())
        pLineData = &(*it).second;
    else if (type >= 0 && type < 7)
        pLineData = &_linedata[c_arrTypes[type]];
    else
        pLineData = 0;

    bool bObjective = line.m_id && IsObjectiveLine(line);
    float trans = pLineData ? pLineData->opacity : 0.0f;

    fPulseTrans = bObjective ? fPulseTrans : trans;
    fPulseTrans = bDim ? 0.3f * fPulseTrans : fPulseTrans;

    CPnt3D pt3;

    while (true) {
        if (!GetLocPoint(m_map, *itSegment, &pt3))
            return;
        pt3 = WorldToDraw(&pt3);
        if (m_iNumActiveTriangles == m_vecTriangles.size())
        {
            pNew = new C2DPoly;
            m_vecTriangles.push_back(pNew);
        }

        C2DPoly &poly = *m_vecTriangles[m_iNumActiveTriangles++];

        poly.Load(ptAnchor.x, ptAnchor.y, pt2.x, pt2.y, pt3.x, pt3.y, m_texture);
        poly.SetUV(0.5f, 0.9375f, 0.5f, 0.9375f, 0.5f, 0.9375f);
        if (pLineData) {
            poly.SetColor(pLineData->color.x, pLineData->color.y, pLineData->color.z);
            poly.SetTrans(fPulseTrans);
        }
        if (++itSegment == line.end())
            return;
        pt2 = pt3;
    }
}

namespace {

/* whether the walk is still on the line (SOCOM 1's line 38, an inline there; SOCOM II's vtable
 * slot 2, the iterators' `!=` written as `!(==)`) */
bool CAiLinePtItr::IsValid() const
{
    return !(_it == _line.end());
}

}

/* whether a label overlaps a box (hud_teammatenames.cpp's, 0x227130) */
unsigned char WordIntersectionTest(int tx1, int ty1, int tx2, int ty2, C2DString *target);

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

/* the viewer: only its last valid cell (CEntity's, zentity.h), whose map id picks the maps */
class CZSealBody : public CEntity {
};

/* the maps a map's list shows, and the AI's maps (fts_mission.cpp's, ai_maps.cpp's) */
int ftsGetNumVisibleMaps(unsigned int list);
/* (an int here: Tick compares and passes the returned map with no narrowing, as SOCOM 1's) */
int ftsGetVisibleMap(unsigned int list, int i);

class CAiMaps {
public:
    CAiMap *GetMapByIndex(unsigned int i) const;
};

extern CAiMaps AiMaps;

/* the pulse stepped (up and down between 60 and 127); last tick's lines, strings, triangles, wide
 * lines and bitmaps dropped; then the maps the viewer's map shows, dimmed, its own map bright and
 * the anchors (SOCOM 1's Tick, the wide lines and bitmaps SOCOM II's) */
bool CZLineMap::Tick(CZNewHudMap *hudmap, float dT)
{
    static float transp_delta = 167.5f;

    g_fPulseTrans += transp_delta * dT;
    if (g_fPulseTrans > 127.0f) {
        g_fPulseTrans = 127.0f;
        transp_delta = -transp_delta;
    } else if (g_fPulseTrans < 60.0f) {
        g_fPulseTrans = 60.0f;
        transp_delta = -transp_delta;
    }
    m_hudmap = hudmap;
    m_lastline = 0;
    m_laststring = 0;
    m_iNumActiveTriangles = 0;
    for (int iLine = 0; iLine < m_vecWideLines.size(); iLine++)
        delete m_vecWideLines[iLine];
    m_vecWideLines.clear();
    for (int iBmp = 0; iBmp < m_vecBitmaps.size(); iBmp++)
        delete m_vecBitmaps[iBmp];
    m_vecBitmaps.clear();

    CZSealBody *viewer = hudmap->m_viewer;

    if (!viewer)
        return false;

    unsigned int iMapId = viewer->m_mapLocValid.m_mapid;

    for (int iMap = 0; iMap < ftsGetNumVisibleMaps(iMapId); iMap++) {
        unsigned int map = ftsGetVisibleMap(iMapId, iMap);

        if (iMapId != map)
            TickMap(AiMaps.GetMapByIndex(map), g_fPulseTrans, true);
    }
    TickMap(AiMaps.GetMapByIndex(iMapId), g_fPulseTrans, false);
    TickAnchors(g_fPulseTrans);
    m_map = 0;
    m_hudmap = 0;
    return false;
}

inline bool CZLineMap::IsObjectiveLine(const CAiMap::line &line) const
{
    CMissionObjectiveCam *obj = m_hudmap->m_objectivelist.GetCurrentObjective();

    return obj && obj->HasLine(line.m_id);
}

/* the compass anchors the map shows (OVL1 .data 0x3E4390, fifteen kinds; the name ours) and their
 * bitmaps' UVs, a little past the texture's edges (0x3DC4D0-0x3DC4E8, after the pulse's statics; the
 * names ours) */
/* the HUD (hud_main.cpp's, theHUD 0x408E58), as far as the labels' font and the compass go */
class CHUD {
public:
    unsigned char _0[0x498];
    C2DFont *m_myfont;   // 0x498
    unsigned char _49c[0x4fe0 - 0x49c];
    MapCompass m_compass;   // 0x4fe0
};

extern CHUD *theHUD;

/* a UI variable's hook (zui_uivar.h's, SOCOM 1's DWARF; the vtable pointer first) and the UI's plain
 * bitmap (zui_2d.h's, as hud_newhudmap.cpp stands it in: TickAnchors news one for each anchor, its
 * constructor inline) */
class UIVarMixin {
public:
    UIVarMixin() { m_uivar = 0; }
    virtual ~UIVarMixin() { DetachUIVar(0); }
    void DetachUIVar(CUIVariable *var);
    virtual void UpdateUIVar(CUIVariable *var);

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    CPlainBmp()
    {
        SetColor(128.0f, 128.0f, 128.0f);
        SetTrans(128.0f);
        SetIsTrans(true);
        SetCanHandleInput(false);
        m_pulsing = false;
        m_period = 1.0f;
    }
    virtual void Draw(zdb::CCamera *cam) { C2DBitmap::Draw(cam); }
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* (hud_main.cpp's layout floats, 20 and 10) */
extern float g_fHudLayout20;
extern float g_fHudLayout10;

static const unsigned int c_arrAnchorTypes[15] = { 6, 7, 8, 12, 13, 5, 14, 15, 16, 17, 21, 29, 34, 35, 37 };

namespace {
float s_fAnchorU0 = -0.01f;
float s_fAnchorV0 = -0.01f;
float s_fAnchorU1 = 1.02f;
float s_fAnchorV1 = 1.02f;
}

/* the compass anchors put on the map as bitmaps (SOCOM 1's TickAnchors, its DWARF: pObjv, bPulse,
 * ptColor, fBotY, fRightX, ptDraw, bmp, b2nd, bPrim, bExtract, iType, pAnchor, iAnchor): of the kinds
 * the map shows, each a new plain bitmap at its drawn point in its compass image and colour, an
 * insertion point (kinds 7 and 8) shown once -- the others at its place left out -- and pulsing when
 * the current objective lights it. Both walks count with SOCOM 1's int iAnchor (an unsigned counter is
 * strength-reduced, the target indexes each pass). The extract's place is compared through a pointer
 * an inline returns (round 61): the target takes the two positions' addresses into a0 / a1 (`addiu
 * a0,s3,4`, `addiu a1,s1,4`) and reads y and z at offsets from them, only the x's folded into the
 * anchors' own offsets in the first block -- zmath.h's operator== on the members (or on a reference an
 * inline returns, or on reference or pointer locals) folds every offset (0x46C), a cast pointer
 * hoists the invariant extract's field addresses (s3+8, s3+12) out of the loop. The type is an int:
 * the objective tests are slti (sltiu as an unsigned int, 2 off). */
static inline CPnt3D *AnchorPos(CCompassAnchor *a)
{
    return &a->m_posW;
}

void CZLineMap::TickAnchors(float fPulseTrans)
{
    CCompassAnchor *pExtract = 0;

    for (int iAnchor = 0; iAnchor < theCompass.size(); iAnchor++) {
        if (theCompass[iAnchor] && (theCompass[iAnchor]->m_type == 7 || theCompass[iAnchor]->m_type == 8)) {
            pExtract = theCompass[iAnchor];
            break;
        }
    }
    for (int iAnchor = 0; iAnchor < theCompass.size(); iAnchor++) {
        CCompassAnchor *pAnchor = theCompass[iAnchor];

        if (!pAnchor)
            continue;
        if (pExtract && pExtract != pAnchor && *AnchorPos(pExtract) == *AnchorPos(pAnchor))
            continue;

        int iType = pAnchor->m_type;
        bool bShow = false;

        for (int i = 0; i < 15; i++)
            if (iType == c_arrAnchorTypes[i]) {
                bShow = true;
                break;
            }
        if (!bShow)
            continue;

        CPlainBmp *bmp = new CPlainBmp;

        m_vecBitmaps.push_back(bmp);

        CPnt3D ptDraw = WorldToDraw(&pAnchor->m_posW);
        MapCompass *compass = &theHUD->m_compass;
        zdb::CTexHandle *img = compass->GetAnchorImg(iType);

        ptDraw.x -= g_fHudLayout10;
        ptDraw.y -= g_fHudLayout10;
        bmp->Load(ptDraw.x, ptDraw.y, ptDraw.x + g_fHudLayout20, ptDraw.y + g_fHudLayout20, img);
        bmp->SetUV(s_fAnchorU0, s_fAnchorV0, s_fAnchorU1, s_fAnchorV1);

        CPnt3D ptColor = compass->GetAnchorColor(pAnchor);

        bmp->SetColor(ptColor.x, ptColor.y, ptColor.z);

        bool bPulse = iType == 7 || iType == 8;
        CMissionObjectiveCam *pObjv = m_hudmap->m_objectivelist.GetCurrentObjective();

        if (pObjv) {
            bool bPrim = iType >= 14;

            if (iType == 6)
                bPulse = pObjv->m_extract;
            else if (bPrim && iType < 38)
                bPulse = pObjv->HasLetter(iType + 53);
        }
        if (iType == 6)
            bmp->SetTrans(fPulseTrans);
        else
            bmp->SetTrans(bPulse ? fPulseTrans : 128.0f);
    }
}

/* a line drawn cell to cell in its type's look -- the type's own, or a default one -- pulsing when
 * the current objective lights it, dimmed on request; a wide line as polygons (SOCOM 1's, the
 * objective's highlight and the wide line SOCOM II's) */
void CZLineMap::TickLine(const CAiMap::line &line, float fPulseTrans, bool bDim)
{
    if (line.empty())
        return;

    LINE_DATA *pLineData;
    int type = line.m_type;
    std::map<int, LINE_DATA>::iterator it = g_mapLineProps.find(type);

    if (it != g_mapLineProps.end())
        pLineData = &(*it).second;
    else if (type >= 0 && type < 7)
        pLineData = &_linedata[c_arrTypes[type]];
    else
        pLineData = 0;

    bool bObjective = line.m_id && IsObjectiveLine(line);
    float trans = pLineData ? pLineData->opacity : 0.0f;

    fPulseTrans = bObjective ? fPulseTrans : trans;
    fPulseTrans = bDim ? 0.3f * fPulseTrans : fPulseTrans;
    if (pLineData && pLineData->_fWidth > 1.01f) {
        LINE_DATA props = *pLineData;

        props.opacity = fPulseTrans;
        SetupWideLine(line, props);
        return;
    }

    CAiMapLocVec::const_iterator li = line.begin();

    MoveTo(*li);
    for (++li; li != line.end(); ++li) {
        C2DLine *l = LineTo(*li);

        if (l && pLineData) {
            l->SetColor(pLineData->color.x, pLineData->color.y, pLineData->color.z);
            l->SetTrans(fPulseTrans);
        }
    }
}

/* each of the map's lines: drawn as a line unless it is a polygon without its outline, and a
 * polygon filled (SOCOM 1's TickLines; the polygon's own tick, 0x20F5C0, SOCOM II's) */
/* (pinned out of line: Tick calls it, where -inline auto would take its few statements) */
#pragma auto_inline off
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
#pragma auto_inline reset

/* the map's regions of kinds 1 and 5 outlined, corner to corner, in the extract look (dimmed on
 * request) -- SOCOM 1's TickRegions (its DWARF: myLine, type, loc, rit and the static reference
 * ld), the corners' cells drawn through SOCOM II's cell LineTo */
void CZLineMap::TickRegions(const CAiMap &map, bool bDim)
{
    static LINE_DATA &ld = _linedata[LINETYPE_EXTRACT];
    float fTrans = ld.opacity;

    /* (a ternary: the if-statement pads its join and multiplies the other way round, fTrans first) */
    fTrans = bDim ? 0.3f * fTrans : fTrans;

    const CAiMap::regions &regions = map.m_regionTable;

    for (CAiMap::regions::const_iterator rit = regions.begin(); rit != regions.end(); ++rit) {
        int type = (*rit).m_type;

        if (type == 1 || type == 5) {
            CAiMapLoc loc;
            C2DLine *myLine;

            loc = (*rit).m_loc;
            MoveTo(loc);
            loc = CAiMapLoc((*rit).m_loc.x + (*rit).m_dx - 1, (*rit).m_loc.y, 0);
            if ((myLine = LineTo(loc)) != 0) {
                myLine->SetColor(ld.color.x, ld.color.y, ld.color.z);
                myLine->SetTrans(fTrans);
            }
            loc = CAiMapLoc((*rit).m_loc.x + (*rit).m_dx - 1, (*rit).m_loc.y + (*rit).m_dy - 1, 0);
            if ((myLine = LineTo(loc)) != 0) {
                myLine->SetColor(ld.color.x, ld.color.y, ld.color.z);
                myLine->SetTrans(fTrans);
            }
            loc = CAiMapLoc((*rit).m_loc.x, (*rit).m_loc.y + (*rit).m_dy - 1, 0);
            if ((myLine = LineTo(loc)) != 0) {
                myLine->SetColor(ld.color.x, ld.color.y, ld.color.z);
                myLine->SetTrans(fTrans);
            }
            loc = (*rit).m_loc;
            if ((myLine = LineTo(loc)) != 0) {
                myLine->SetColor(ld.color.x, ld.color.y, ld.color.z);
                myLine->SetTrans(fTrans);
            }
        }
    }
}

/* the map's way points labelled (SOCOM II's TickWayPoints, SOCOM 1's DWARF naming its pieces --
 * StringPos, testString, i, j, singlechar, string, myString, pos, origin, nit, names): each name of
 * kind 5 with a point gets a label just off it, green, its whole name on the full-screen map and its
 * first letter on the small one; then each label is moved off the earlier ones it overlaps -- down
 * when it is the lower (the pass restarted), the other moved and the pass ended when not. bDim is
 * not read. The registers want myString declared first and the next string taken in place (as
 * NewLine does it, but through an inline the result is folded into the inline's temporary, numbered
 * after names: s2 / s3 swapped -- igreplay --want put it at 33.5); testString inside the inner loop
 * (hoisted below its guard, as the target computes it), the other label read by index, the restart
 * after the if/else (the target's layout) */
void CZLineMap::TickWayPoints(const CAiMap &map, bool bDim)
{
    C2DString *myString;
    const CAiMap::names &names = map.m_namesTable;

    for (CAiMap::names::const_iterator nit = names.begin(); nit != names.end(); ++nit) {
        if ((*nit).m_15 == 5) {
            CPnt3D pos;

            if (GetLocPoint(&map, (*nit).m_loc, &pos)) {
                CPnt3D origin;

                m_hudmap->m_matrix.Transform(&pos, &origin, 1);
                origin.Add(&CPnt3D(-20.0f, 5.0f, 0.0f));

                if (m_laststring < 32) {
                    m_laststring++;
                    myString = &m_stringlist[m_laststring - 1];
                } else
                    myString = 0;

                if (myString) {
                    const char *string = (*nit).m_name;

                    myString->SetColor(30.0f, 100.0f, 30.0f);
                    if (m_hudmap->m_isFullScreen) {
                        myString->Load(0.9f, string, theHUD->m_myfont, origin.x, origin.y, true);
                        myString->UpdatePos();
                    } else {
                        char singlechar[2] = {0};

                        singlechar[0] = string[0];
                        myString->Load(0.9f, singlechar, theHUD->m_myfont, origin.x, origin.y, true);
                        myString->UpdatePos();
                    }
                }
            }
        }
    }
    for (int i = 0; i < m_laststring; i++) {
        for (int j = 0; j < i; j++) {
            C2DString *testString = &m_stringlist[i];
            CPnt3D StringPos((int)(testString->m_posx + testString->m_socom2_7c) >> 4,
                             (int)(testString->m_posy + testString->m_socom2_80) >> 4, 0.0f);

            if (WordIntersectionTest(StringPos.x - 10.0f, StringPos.y - 10.0f,
                                     StringPos.x + testString->m_length + 10.0f, StringPos.y + 15.0f,
                                     &m_stringlist[j])) {
                if ((int)(testString->m_posy + testString->m_socom2_80) >
                    (int)(m_stringlist[j].m_posy + m_stringlist[j].m_socom2_80)) {
                    StringPos.y += 3.0f;
                    testString->SetPos(StringPos.x, StringPos.y);
                } else {
                    m_stringlist[j].SetPos((int)(m_stringlist[j].m_posx + m_stringlist[j].m_socom2_7c) >> 4,
                                           ((int)(m_stringlist[j].m_posy + m_stringlist[j].m_socom2_80) >> 4) + 3);
                    break;
                }
                j = -1;
            }
        }
    }
}

/* the lines clipped to the map's circle (SOCOM 1's Clip, its DWARF: isvalid, outCnt, ii): each line
 * up to the last (at most 500) filled into the clipper's rows, clipped, the ends the clipper kept
 * copied back -- 1 the second, 2 the first, 3 both, 4 none -- and set and shown, or hidden when
 * nothing is left; no clipper, no clipping */
void CZLineMap::Clip(CZNewHudMap *map)
{
    for (int ii = 0; ii < m_lastline && ii < 500; ii++) {
        zdb::CCircleClip *clip = map->m_circleclipper;

        if (!clip)
            return;
        m_linelist[ii].FillClipInfo((float (*)[4])m_pi, (float (*)[4])m_rgbi, (float (*)[4])m_uvi);

        int outCnt = clip->CircleSegmentClip(&m_po[0], &m_po[1], &m_pi[0], &m_pi[1], &m_rgbo[0],
                                             &m_rgbo[1], &m_rgbi[0], &m_rgbi[1], &m_uvo[0], &m_uvo[1],
                                             &m_uvi[0], &m_uvi[1]);
        bool isvalid = true;

        switch (outCnt) {
        case 1:
            m_po[1] = m_pi[1];
            m_rgbo[1] = m_rgbi[1];
            m_uvo[1] = m_uvi[1];
            break;
        case 2:
            m_po[0] = m_pi[0];
            m_rgbo[0] = m_rgbi[0];
            m_uvo[0] = m_uvi[0];
            break;
        case 3:
            m_po[0] = m_pi[0];
            m_rgbo[0] = m_rgbi[0];
            m_uvo[0] = m_uvi[0];
            m_po[1] = m_pi[1];
            m_rgbo[1] = m_rgbi[1];
            m_uvo[1] = m_uvi[1];
            break;
        case 4:
            break;
        case 0:
            isvalid = false;
            break;
        }
        if (isvalid) {
            m_linelist[ii].SetClipInfo((float (*)[4])m_po, (float (*)[4])m_rgbo, (float (*)[4])m_uvo, 2);
            m_linelist[ii].On();
        } else
            m_linelist[ii].Off();
    }
}

/* the map drawn: the active triangles, the lines (at most 500), the wide lines' polygon sets, the
 * bitmaps and the strings (at most 32) -- SOCOM 1's CZLineMap::Draw with SOCOM II's sets and
 * bitmaps (the unit's first function, 0x20D390). One function-level counter for the triangles'
 * and the bitmaps' walks, the other three walks' their own (round 42: as the target's registers
 * have them -- every counter loop-scoped colours the parameters first, 0x10 longer; one counter
 * for all five takes the target's five registers but shares the webs) */
void CZLineMap::Draw(zdb::CCamera *camera)
{
    int i;

    for (i = 0; i < m_iNumActiveTriangles; i++)
        m_vecTriangles[i]->Draw(camera);
    for (int i = 0; i < m_lastline && i < 500; i++)
        m_linelist[i].Draw(camera);
    for (int i = 0; i < m_vecWideLines.size(); i++) {
        CPolySet *set = m_vecWideLines[i];

        for (int j = 0; j < set->size(); j++)
            (*set)[j]->Draw(camera);
    }
    for (i = 0; i < m_vecBitmaps.size(); i++)
        m_vecBitmaps[i]->Draw(camera);
    for (int i = 0; i < m_laststring && i < 32; i++)
        m_stringlist[i].Draw(camera);
}
