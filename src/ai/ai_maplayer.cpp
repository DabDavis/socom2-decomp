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
