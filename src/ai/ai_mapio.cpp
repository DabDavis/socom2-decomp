/* ai_mapio.cpp (OVL2 0x51A490-~0x51CB3F, the name SOCOM 1's -- ai.h): CAiMap's file readers. SOCOM 1
 * read a map in one Read (0x189390) and its tables through read_readers, read_routes, read_lines
 * and the ReadCellTable templates; SOCOM II splits Read (0x51AFE0) by file version (0x51A490,
 * 0x51A6D0, 0x51A910, 0x51AB50, 0x51AD90 for 23 down to 19), each calling the same table readers.
 * Each read goes through CFileIO's fread slot (0x24). Still unwritten: the layer reader (0x51B150)
 * and the two converting readers. */

#include "ai.h"
#include "../gamez/zrdr/zrdr.h"
#include "../gamez/zutil/zutil.h"

/* the tables' resizes: MSL's vector::resize, which include/vector does not model yet -- each
 * instance called by its placeholder (SOCOM 1: resize__Q23std...__vector_imp<...,0>FUi, one per
 * element type, emitted in this unit's template tail) */
extern "C" void func_0051C4A0(CAiMapLocVec *v, unsigned int n);
extern "C" void func_0051C3F0(CAiMap::readers *v, unsigned int n);
extern "C" void func_0051C230(CAiMap::routes *v, unsigned int n);
extern "C" void func_0051C070(CAiMap::lines *v, unsigned int n);

extern "C" void func_0051C6D0(CAiMap::actions *v, unsigned int n);
extern "C" void func_0051C8F0(CAiMap::regions *v, unsigned int n);
extern "C" void func_0051CA40(CAiMap::spores *v, unsigned int n);

inline void table_resize(CAiMapLocVec &t, unsigned int n) { func_0051C4A0(&t, n); }
inline void table_resize(CAiMap::readers &t, unsigned int n) { func_0051C3F0(&t, n); }
inline void table_resize(CAiMap::actions &t, unsigned int n) { func_0051C6D0(&t, n); }
inline void table_resize(CAiMap::regions &t, unsigned int n) { func_0051C8F0(&t, n); }
inline void table_resize(CAiMap::spores &t, unsigned int n) { func_0051CA40(&t, n); }

extern "C" void func_0051C630(CAiMap::names *v, unsigned int n);
extern "C" void func_0051C840(CAiMap::links *v, unsigned int n);

inline void table_resize(CAiMap::names &t, unsigned int n) { func_0051C630(&t, n); }
inline void table_resize(CAiMap::links &t, unsigned int n) { func_0051C840(&t, n); }

/* what the version readers call that is not written yet: the cell layer's allocation (0x53A8F0,
 * the layer object at 0xA8), the scanlines' reader and the two converting table readers */
extern "C" CAiCell *func_0053A8F0(void *layer, unsigned int count);
/* the scanlines' resize (MSL's vector::resize, ai_map.cpp's template tail: SOCOM 1's
 * resize__Q23std72__vector_imp<Q26CAiMap8scanline,...,0>FUi) */
extern "C" void func_00519700(CAiMap::scanlines *v, unsigned int n);
extern "C" bool func_0051BBE0(CFileIO &f, CAiMap::names &t);
extern "C" bool func_0051BD40(CFileIO &f, CAiMap::links &t);
/* a further layer of cells read (0x51B150, called from the version-20 readers on) */
extern "C" bool func_0051B150(CAiMap *map, CFileIO &f, int n, CAiMapLayer **layer);

/* the graph's pieces as the fixups see them (graph_tree.cpp's CGEdge and CGNode: what each stands
 * for, a route at an edge's 0x14 and a region at a node's 8) */
class CGEdge {
public:
    unsigned char _0[0x14];
    CAiMap::route *m_route;   // 0x14
};

class CGNode {
public:
    unsigned char _0[0x8];
    CAiMap::region *m_region;   // 0x8
};

/* a table read whole: its count, then its elements as they lie in the file (SOCOM 1's template:
 * true for an empty table, else whether every byte came) */
template <class C, class T>
bool ReadCellTable(CFileIO &f, C &table)
{
    bool ok = false;
    int count = 0;

    if (f.fread(&count, 4) == 4) {
        if (count > 0) {
            table_resize(table, count);
            int bytes = count * sizeof(T);

            ok = bytes == f.fread(table.data_, bytes);
        } else {
            ok = true;
        }
    }
    return ok;
}

/* a version-19 map (Read's oldest case, 0x51AFE0): the header, the cells (a map before version
 * 15 has no count of its own), the scanlines, the tables -- the names and links through their
 * converting readers -- and the extra data (SOCOM 1's Read, one per version in SOCOM II; the name
 * ours). The target reads m_cellcount twice -- for the byte count, then for the count -- and keeps
 * the bytes == count * 8 test: only the count read through a const view of `this` stops MW
 * sharing the load (a plain, int-cast or volatile read does not), and declared ahead of the bytes
 * it takes the higher saved register. The extra data's size multiplies after AllocEx, the count
 * cast to int so it loads first */
bool CAiMap::read_v19(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int count;
    unsigned int bytes = m_cellcount * sizeof(CAiCell);

    count = ((const CAiMap *)this)->m_cellcount;

    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    func_0051BBE0(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    func_0051BD40(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes) {
        char *ex = AllocEx(m_mapex_bytes);

        f.fread(ex, m_mapex_bytes * (int)m_cellcount);
    }
    return true;
}

/* a map's tail from version 20 on: its graph, if it says it has one (the flag cleared when none
 * reads), and its further layers, the count then made the layers' own */
#define READ_GRAPH_AND_LAYERS()                                              \
    if (m_flags & 1)                                                         \
        m_graph = CGraph::Create(f, _fixupNode, _fixupEdge, this);           \
    if (!m_graph)                                                            \
        m_flags &= ~1;                                                       \
    if (m_layercount > 1) {                                                  \
        m_layers.reserve(m_layercount);                                      \
        for (unsigned int i = 1; i < m_layercount; i++) {                    \
            CAiMapLayer *layer = 0;                                          \
                                                                             \
            if (func_0051B150(this, f, 1, &layer))                           \
                m_layers.push_back(layer);                                   \
        }                                                                    \
        m_layercount = m_layers.size();                                      \
    }

/* the version readers from 20 on: version 19's body, the graph and the layers after it (new in
 * SOCOM II, the names ours). The four compile to the same code but for the table readers they
 * call: a version-20 map converts its names and links, versions 21 and 22 their links, and a
 * version-23 map reads every table as it lies */
bool CAiMap::read_v20(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int count;
    unsigned int bytes = m_cellcount * sizeof(CAiCell);

    count = ((const CAiMap *)this)->m_cellcount;
    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    func_0051BBE0(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    func_0051BD40(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes) {
        char *ex = AllocEx(m_mapex_bytes);

        f.fread(ex, m_mapex_bytes * (int)m_cellcount);
    }
    READ_GRAPH_AND_LAYERS();
    return true;
}

bool CAiMap::read_v21(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int count;
    unsigned int bytes = m_cellcount * sizeof(CAiCell);

    count = ((const CAiMap *)this)->m_cellcount;
    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTable<names, name>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    func_0051BD40(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes) {
        char *ex = AllocEx(m_mapex_bytes);

        f.fread(ex, m_mapex_bytes * (int)m_cellcount);
    }
    READ_GRAPH_AND_LAYERS();
    return true;
}

bool CAiMap::read_v22(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int count;
    unsigned int bytes = m_cellcount * sizeof(CAiCell);

    count = ((const CAiMap *)this)->m_cellcount;
    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTable<names, name>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    func_0051BD40(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes) {
        char *ex = AllocEx(m_mapex_bytes);

        f.fread(ex, m_mapex_bytes * (int)m_cellcount);
    }
    READ_GRAPH_AND_LAYERS();
    return true;
}

bool CAiMap::read_v23(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int count;
    unsigned int bytes = m_cellcount * sizeof(CAiCell);

    count = ((const CAiMap *)this)->m_cellcount;
    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTable<names, name>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    ReadCellTable<links, link>(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes) {
        char *ex = AllocEx(m_mapex_bytes);

        f.fread(ex, m_mapex_bytes * (int)m_cellcount);
    }
    READ_GRAPH_AND_LAYERS();
    return true;
}

/* a map read: its version, the reader for it (19 to 23; older or newer maps are refused), then
 * the altitude scale, the cells, the routes and the graph made good (SOCOM 1's Read, its body
 * split by version in SOCOM II) */
bool CAiMap::Read(CFileIO &f)
{
    unsigned int version;
    bool ok;

    f.fread(&version, 4);
    m_version = version;
    if (version < 19) {
        ok = false;
    } else {
        switch (version) {
        case 19:
            ok = read_v19(f);
            break;
        case 20:
            ok = read_v20(f);
            break;
        case 21:
            ok = read_v21(f);
            break;
        case 22:
            ok = read_v22(f);
            break;
        case 23:
            ok = read_v23(f);
            break;
        default:
            ok = false;
            break;
        }
        if (ok) {
            m_alt_quant = 0.000976563f * (m_bbox.m_max.y - m_bbox.m_min.y);
            ValidateCells();
            ValidateRoutes(false);
            CreateGraph(false);
        }
    }
    return ok;
}

/* the graph's edges saved and read back: each edge's route as its index in the table (SOCOM 1's
 * cleared the route and the index) */
void _fixupEdge(CGraph *, CGEdge *edge, bool fix, unsigned int &n, void *data)
{
    if (fix) {
        edge->m_route = ((CAiMap *)data)->GetRoute(n);
        return;
    }
    n = ((CAiMap *)data)->GetRouteIndex(edge->m_route);
}

/* the graph's nodes saved and read back: each node's region as the packed cell of its corner
 * (SOCOM 1's). The cell is a CAiMapLoc left unbuilt (its constructor empty), its fields assigned
 * one by one */
void _fixupNode(CGraph *, CGNode *node, bool fix, unsigned int &n, void *data)
{
    if (fix) {
        CAiMapLoc loc;
        unsigned int packed = n;

        loc.m_mapid = packed >> 26;
        loc.x = (packed >> 13) & 0x1fff;
        loc.y = packed & 0x1fff;
        node->m_region = ((CAiMap *)data)->GetRegion(loc);
    } else {
        const CAiMapLoc &loc = node->m_region->m_loc;

        n = loc.m_mapid << 26 | loc.x << 13 | loc.y;
    }
}

/* the scanlines' reader and the three table readers are called by every version reader, never
 * inlined: pinned */
#pragma auto_inline off

/* the rows' runs read whole, then each run's first cell numbered on from the last (SOCOM 1's
 * pointed each run into the cells and zeroed its extra data instead). The target keeps &data_ in
 * s1 from before the read (SOCOM 1's code too) and loads begin through it, its first end() sharing
 * that load while the loop's later end()s read size and data off `this`: include/vector's begin()
 * and end() read through the deleter's data() reference, as the read does */
bool CAiMap::read_scanlines(CFileIO &f)
{
    func_00519700(&m_scanlines, m_ny);
    f.fread(m_scanlines.data(), m_ny * sizeof(scanline));
    int index = 0;

    for (scanlines::iterator i = m_scanlines.begin(); i != m_scanlines.end(); ++i) {
        (*i).m_index = index;
        index += (*i).m_end - (*i).m_start;
    }
    return true;
}


/* the lines: their count, then each line's cells and, from version 10 on, its data (SOCOM 1's) */
bool CAiMap::read_lines(CFileIO &f)
{
    unsigned int count = 0;

    if (f.fread(&count, 4) == 4 && count) {
        func_0051C070(&m_lineTable, count);
        for (lines::iterator i = m_lineTable.begin(); i != m_lineTable.end(); ++i) {
            ReadCellTable<CAiMapLocVec, CAiMapLoc>(f, *i);
            if (m_version >= 10)
                f.fread((line_data *)&*i, 4);
        }
    }
    return true;
}

/* the routes: their count, then each route's two regions, its flags, its waypoints and its path
 * (SOCOM 1's) */
bool CAiMap::read_routes(CFileIO &f)
{
    unsigned int count = 0;

    if (f.fread(&count, 4) == 4 && count) {
        func_0051C230(&m_routeTable, count);
        for (routes::iterator i = m_routeTable.begin(); i != m_routeTable.end(); ++i) {
            f.fread((*i).m_region, 8);
            f.fread(&(*i).m_flags, 4);
            ReadCellTable<CAiMapLocVec, CAiMapLoc>(f, (*i).m_waypts);
            ReadCellTable<CAiMapLocVec, CAiMapLoc>(f, (*i).m_path);
        }
    }
    return true;
}

/* the readers: the table, then each reader's packed script, freed again if it will not read
 * (SOCOM 1's) */
bool CAiMap::read_readers(CFileIO &f)
{
    ReadCellTable<readers, reader>(f, m_readerTable);
    if (m_readerTable.size()) {
        for (readers::iterator i = m_readerTable.begin(); i != m_readerTable.end(); ++i) {
            (*i).m_reader = 0;
            CRdrFile *r = new CRdrFile;

            if (r->ReadPacked(f, (*i).m_bytes))
                (*i).SetReader(r);
            else
                zrdr_free(r);
        }
    }
    return true;
}

#pragma auto_inline reset
