/* ai_mapio.cpp (OVL2, the name SOCOM 1's -- ai.h): CAiMap's file readers. SOCOM 1 read a map in one
 * Read (0x189390) and its tables through read_readers, read_routes, read_lines and the
 * ReadCellTable templates; SOCOM II splits Read by file version (0x51A490, 0x51A6D0, 0x51A910,
 * 0x51AB50, 0x51AD90), each calling the same table readers. Only the table readers so far (OVL2
 * 0x51B390-0x51B6B3 and their template instances); defined here in reverse of the image's order
 * (-inline deferred). Each read goes through CFileIO's fread slot (0x24). */

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

/* what the version readers call that is not written yet: the cell layer's allocation (0x53A8F0,
 * the layer object at 0xA8), the scanlines' reader and the two converting table readers */
extern "C" CAiCell *func_0053A8F0(void *layer, unsigned int count);
extern "C" bool func_0051AEF0(CAiMap *map, CFileIO &f);
extern "C" bool func_0051BBE0(CFileIO &f, CAiMap::names &t);
extern "C" bool func_0051BD40(CFileIO &f, CAiMap::links &t);

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
 * ours). OPEN, 7 instructions short (0x144 against 0x160): the target reads m_cellcount twice --
 * once for the byte count, once for the count -- and keeps the bytes == count * 8 test, which MW
 * folds here; the count read again in the test, an inline helper taking both (count read for
 * each argument) and a volatile read (14 off) were tried */
bool CAiMap::read_v19(CFileIO &f)
{
    f.fread(this, 0xa4);
    if (m_version < 15) {
        m_cellcount = m_nx * m_ny;
        m_compressed = 0;
    }
    unsigned int bytes = m_cellcount * sizeof(CAiCell);
    unsigned int count = m_cellcount;

    if (count) {
        CAiCell *cells = func_0053A8F0(m_layer0, count);

        if (bytes == count * sizeof(CAiCell))
            f.fread(cells, bytes);
    }
    func_0051AEF0(this, f);
    func_0051BBE0(f, m_namesTable);
    ReadCellTable<actions, action>(f, m_actionTable);
    func_0051BD40(f, m_linksTable);
    ReadCellTable<regions, region>(f, m_regionTable);
    ReadCellTable<spores, spore>(f, m_sporeTable);
    read_lines(f);
    read_readers(f);
    read_routes(f);
    if (m_mapex_bytes)
        f.fread(AllocEx(m_mapex_bytes), m_mapex_bytes * m_cellcount);
    return true;
}

/* the three table readers are called by every version reader, never inlined: pinned */
#pragma auto_inline off

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
