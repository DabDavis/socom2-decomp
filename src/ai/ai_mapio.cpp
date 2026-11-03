/* ai_mapio.cpp (OVL2 0x51A490-~0x524170, the name SOCOM 1's -- ai.h): CAiMap's file readers. SOCOM 1
 * read a map in one Read (0x189390) and its tables through read_readers, read_routes, read_lines
 * and the ReadCellTable templates; SOCOM II splits Read (0x51AFE0) by file version (0x51A490,
 * 0x51A6D0, 0x51A910, 0x51AB50, 0x51AD90 for 23 down to 19), each calling the same table readers.
 * Each read goes through CFileIO's fread slot (0x24). The template tail (0x51B6C0-0x52416F) is the
 * table readers and every table's resize, insert and erase. Still unwritten: the layer reader
 * (0x51B150). */

#include "ai.h"
#include "../gamez/zrdr/zrdr.h"
#include "../gamez/zutil/zutil.h"

/* the tables' resizes: MSL's vector::resize (include/vector), one instance per element type in
 * this unit's template tail, each building its default element: byte-exact with the element's
 * constructor in ai.h (names, links, regions, spores, the scanlines -- ai_map.cpp's copy, 0x519700 --
 * and CAiMapLoc's).
 * OPEN, resize<reader> (0x51C3F0), right size, 6 instructions: registers only -- the target keeps the
 * size in s1 and the end in s2 around the reader's out-of-line constructor (0x52FD70), this build the
 * other way round; the shape is include/vector's, which serves every other instance.
 * OPEN, the routes' and the lines' (0x51C230, 0x51C070; called as members since round 18, which
 * emits them, their insert and erase here -- every one of those copies misses, so the count is
 * unchanged): the target inlines the element's two- and one-CAiMapLocVec constructor whole, six
 * levels with include/vector's chain -- one more level-6 candidate (this build calls CAiMapLocVec()
 * and CAiMapLoc() out of line, 0xAC and 0x13C against 0x144 and 0x11C); the line's temporary's bits
 * are line_data()'s, ai.h. The lines' erase 0x51CB40 (0x128 vs 0x188) and insert 0x51CCF0 (0x440 vs
 * 0x4B8) call the element's copy and __copy out of line where the target inlines them: the class
 * inserts' question (the MSL lane's) */

inline void table_resize(CAiMapLocVec &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::readers &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::actions &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::regions &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::spores &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::names &t, unsigned int n) { t.resize(n); }
inline void table_resize(CAiMap::links &t, unsigned int n) { t.resize(n); }

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

/* a name as maps before version 21 hold it (the names ours, after SOCOM 1's action_v15 and
 * region_v4): the cell, a float, fifteen characters and a byte -- 0x18 bytes. Built empty for
 * resize<name_v20>'s temporary (0x51E430: the name's first byte, the word, the last byte) */
class name_v20 {
public:
    name_v20()
    {
        m_name[0] = 0;
        m_4 = 0.0f;
        m_23 = 0;
    }

    CAiMapLoc m_loc;   // 0x0
    float m_4;   // 0x4: copied through the FPU (the old names' erase, 0x522A50)
    char m_name[15];   // 0x8
    unsigned char m_23;   // 0x17
};
class names_v20 : public std::vector<name_v20> {
};

/* a link as maps before version 23 hold it: SOCOM 1's two cells and no bits (8 bytes) */
class link_v22 {
public:
    CAiMapLoc m_loc;   // 0x0
    CAiMapLoc m_link;   // 0x4
};
class links_v22 : public std::vector<link_v22> {
};

/* each old element made into the table's: a name keeps its cell, its name (cut to fifteen
 * characters) and its last byte; a link its two cells, of kind 8 with its bits cleared -- the
 * fields assigned in the body, so the four-bit kind is a byte read-mask-write */
inline void table_convert(CAiMap::name &d, const name_v20 &s)
{
    d.m_loc = s.m_loc;
    d.SetName(s.m_name);
    d.m_15 = s.m_23;
}
inline void table_convert(CAiMap::link &d, const link_v22 &s)
{
    d.m_loc = s.m_loc;
    d.m_link = s.m_link;
    d.m_kind = 8;
    d.m_unk8 = 0;
}

/* a table read in an older format: its count, the old elements read whole into a vector of
 * their own, then the table sized and each element converted (SOCOM 1's ReadCellTableCvt, by its
 * shape: SOCOM 1's actions_v15 and regions_v4 readers; SOCOM II's converts the names for versions
 * 19 and 20, 0x51BBE0, and the links for 19 to 22, 0x51BD40) */
template <class C, class T, class Cv, class Tv>
bool ReadCellTableCvt(CFileIO &f, C &table)
{
    bool ok = false;
    int count = 0;

    if (f.fread(&count, 4) == 4) {
        if (count > 0) {
            Cv old;

            old.resize(count);
            int bytes = count * sizeof(Tv);

            ok = bytes == f.fread(old.data_, bytes);
            if (ok) {
                table_resize(table, count);
                for (int i = 0; i < count; i++) {
                    T &d = table[i];
                    Tv &o = old[i];

                    table_convert(d, o);
                }
            }
        } else {
            ok = true;
        }
    }
    return ok;
}

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
        CAiCell *cells = m_layer0.AllocCells(count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTableCvt<names, name, names_v20, name_v20>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    ReadCellTableCvt<links, link, links_v22, link_v22>(f, m_linksTable);
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
            if (read_layer(f, true, &layer))                                 \
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
        CAiCell *cells = m_layer0.AllocCells(count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTableCvt<names, name, names_v20, name_v20>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    ReadCellTableCvt<links, link, links_v22, link_v22>(f, m_linksTable);
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
        CAiCell *cells = m_layer0.AllocCells(count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTable<names, name>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    ReadCellTableCvt<links, link, links_v22, link_v22>(f, m_linksTable);
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
        CAiCell *cells = m_layer0.AllocCells(count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    read_scanlines(f);
    ReadCellTable<names, name>(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    ReadCellTableCvt<links, link, links_v22, link_v22>(f, m_linksTable);
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
        CAiCell *cells = m_layer0.AllocCells(count);

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

/* a further layer read (new in SOCOM II, the name ours): its header -- into a new layer when none
 * is given -- and, unless a map from version 22 on marks the layer empty (flag 2), its cells. The
 * result is never set: every caller's push_back is dead, and m_layercount ends at the layers'
 * size. Pinned with the table readers (the version readers call it); the bytes declared ahead of
 * the count, which takes the higher saved register */
bool CAiMap::read_layer(CFileIO &f, bool header, CAiMapLayer **layer)
{
    bool ok = false;
    unsigned int bytes;
    unsigned int count = m_cellcount;
    bool skip = false;

    bytes = count * sizeof(CAiCell);

    if (header) {
        if (!*layer)
            *layer = new CAiMapLayer(0);
        f.fread(*layer, 40);
        skip = m_version >= 22 && ((*layer)->m_flags & 2);
    }
    if (!skip) {
        (*layer)->AllocCells(count);
        f.fread((*layer)->m_cells, bytes);
    }
    return ok;
}


/* the rows' runs read whole, then each run's first cell numbered on from the last (SOCOM 1's
 * pointed each run into the cells and zeroed its extra data instead). The target keeps &data_ in
 * s1 from before the read (SOCOM 1's code too) and loads begin through it, its first end() sharing
 * that load while the loop's later end()s read size and data off `this`: include/vector's begin()
 * and end() read through the deleter's data() reference, as the read does */
bool CAiMap::read_scanlines(CFileIO &f)
{
    m_scanlines.resize(m_ny);
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
        m_lineTable.resize(count);
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
        m_routeTable.resize(count);
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
