/* ai_maplayer.cpp (OVL2 0x53A730-0x53AA4F, the name ours): the AI map's cell layers, new in SOCOM II
 * -- CAiMapLayer's own functions, the layers' vector's destructor and the map's removal of every
 * layer but its first. They sit after vehicle_rdr.cpp and the map walkers, after the look and
 * cover searches (0x539EB0-0x53A72F, SOCOM 1's FindLookLoc stretch) with no template tail between,
 * so they may be that unit's; the pod vector's resize and erase (0x53AA50, 0x53AAC0) follow them.
 * Defined here in reverse of the image's order (-inline deferred). */

#include <string.h>

#include "ai.h"

/* named (none: an empty name), its words cleared, flag 1 set (new in SOCOM II: CAiMap(int) builds
 * its first layer with a name of 0, the layer reader each further one) */
CAiMapLayer::CAiMapLayer(const char *name)
{
    m_cells = 0;
    memset(m_name, 0, sizeof(m_name));
    m_flags = 0;
    m_unk14 = 0;
    memset(m_unk18, 0, sizeof(m_unk18));
    if (!name) {
        m_name[0] = 0;
    } else {
        strncpy(m_name, name, sizeof(m_name));
        m_name[15] = 0;
    }
    m_flags |= 1;
}

/* the cells freed (~CAiMap calls it on the first layer with -1, DeleteLayers inlines it) */
CAiMapLayer::~CAiMapLayer()
{
    FreeCells();
}

void CAiMapLayer::FreeCells()
{
    if (m_cells)
        _free(m_cells, 0, 0);
    m_cells = 0;
}

CAiCell *CAiMapLayer::AllocCells(unsigned int n)
{
    m_cells = CAiCell::Alloc(n);
    return m_cells;
}

CAiMapLayerVec::~CAiMapLayerVec()
{
}

/* a layer deleted, for std::for_each (the name ours) */
struct CAiMapLayerDelete {
    CAiMapLayerDelete() {}
    void operator()(CAiMapLayer *layer) { delete layer; }
};

/* the layers after the first deleted through an MSL for_each (its class iterators in stack slots,
 * the first stepped past m_layer0 as the argument), then dropped from the vector, and the map
 * marked changed. `last` declared first: only so do the slots fall as the target's (end at sp+76).
 * OPEN (0x12C against 0x134), two include/vector questions: the target loads data_ once for both
 * end() and begin() (size first) where end()'s store here makes begin() load it again -- the pod
 * vector's end() reusing begin()'s load, README "Open, and next"; and the target's erase copies
 * both iterators (sp+60, sp+56) where the inlined erase here copies only `first`. Declaring both in
 * one statement, assigning them, or passing explicit copies to erase all miss further */
void CAiMap::DeleteLayers()
{
    if (m_layers.size() > 1) {
        CAiMapLayerVec::iterator last = m_layers.end();
        CAiMapLayerVec::iterator first = m_layers.begin();

        std::for_each(++first, last, CAiMapLayerDelete());
        m_layers.erase(first, last);
        SetModified(true);
    }
}

/* what the look and cover searches' callbacks read and set: the cell found, the point looked at
 * and its distance from the start (SOCOM 1's; the names ours) */
struct LOOK_DATA {
    CAiMapLoc m_loc;   // 0x0
    CPnt3D m_pos;   // 0x4
    float m_dist;   // 0x10
};

/* a cell a search may settle on: walkable, no action on it, no overlay over it (the name ours:
 * inlined one level into both callbacks in the target, so the cell's two tests and the overlays'
 * walk are called out of line -- 0x511F40, 0x516920, 0x53A500).
 * OPEN with both callbacks (0x204 against 0xEC, _find_cover likewise): MW here inlines it, the
 * cell's tests and the overlays' walk, cutting only the walk's deepest list pieces (tail() and
 * the generic_iterator constructors, `warn_notinlined`) -- a depth budget spent from the call
 * site down, where the target stops at the helper's own body. The target's 0x53A500 inlines the
 * whole const walk with its iterators in stack slots, as ai_astar.cpp's fcost does. Not tried:
 * the helper as a member of the floodfill struct or the cell, the test spelled with `== false` */
inline bool _open_cell(const MAP_FLOODFILL_STRUCT &s)
{
    return s.m_cell->IsTraversable() && !s.m_cell->HasAction() && !ai::overlays.Contains(s.m_cell, s.m_start);
}

/* the cover search's callback: an open cell is taken for testing; else an opaque cell squarely
 * between the start and the point ends the search, the cell found the one being tested (SOCOM 1's
 * _find_cover, the overlays new) */
static bool _find_cover(MAP_FLOODFILL_STRUCT &s, void *data)
{
    LOOK_DATA *d = (LOOK_DATA *)data;
    bool r = false;

    switch (s.m_mode) {
    case 0:
        if (_open_cell(s)) {
            r = true;
        } else if (s.m_cell->IsOpaque()) {
            if (CAiMap::GetDirDelta(s.m_map->GetClosestDir(s.m_start, &d->m_pos), (CAiMap::DIR)s.m_dir) == 0) {
                s.m_mode = 2;
                r = true;
                d->m_loc = s.m_loc;
            }
        } else {
            r = false;
        }
        break;
    case 1:
        r = true;
        break;
    }
    return r;
}

/* the look search's callback: an open cell is taken for testing, and a tested one from which the
 * point is seen ends the search (SOCOM 1's _find_look, the overlays new) */
static bool _find_look(MAP_FLOODFILL_STRUCT &s, void *data)
{
    LOOK_DATA *d = (LOOK_DATA *)data;
    bool r = false;

    switch (s.m_mode) {
    case 0:
        if (_open_cell(s))
            r = true;
        break;
    case 1: {
        r = true;
        CAiMapLoc block;

        if (s.m_map->CanSeeLine(s.m_loc, &d->m_pos, &block)) {
            s.m_mode = 2;
            r = false;
            d->m_loc = s.m_loc;
        }
        break;
    }
    }
    return r;
}

/* the cell from which the point is seen, flooded out from the start toward it (SOCOM 1's; the
 * direction handed back is the start's toward the point). The direction declared ahead of the
 * centre: only so do their slots fall as the target's (sp+224, sp+208).
 * OPEN, right size, 17 instructions: the normalisation -- the target reads d.z again inside the
 * test (an inline length, not shared with the sum's load?) and computes both products before
 * either store, keeping d.x from the sum; this build keeps &d.z in v1 from the sum on. Compound
 * assignments, products into locals, z first and an inline normalize_xz(CPnt3D &) were tried */
bool CAiMap::FindLookLoc(const CAiMapLoc &loc, const CPnt3D *pos, CAiMapLoc &out, DIR &dir, int radius)
{
    CPnt3D d;
    CPnt3D center;

    if (!GetCellCenter(loc, &center))
        return false;

    pos->SubXZ(&center, &d);
    d.y = 0.0f;
    float len = sqrtf(d.x * d.x + d.z * d.z);

    if (len != 0.0f) {
        float inv = 1.0f / len;
        float x = d.x * inv;
        float z = d.z * inv;

        d.x = x;
        d.z = z;
    }
    dir = GetClosestDir(&d);
    LOOK_DATA data;

    data.m_loc = loc;
    data.m_pos = *pos;
    data.m_dist = len;
    if (FloodFill(loc, dir, radius, _find_look, &data)) {
        out = data.m_loc;
        return true;
    }
    return false;
}
