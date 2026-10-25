/* ai_maptable.cpp (OVL2 0x52EEA0-, the name ours): CAiMap's tables -- the graph, the routes, the
 * readers, the regions, the links, the actions, the names and the per-cell extra data. SOCOM 1's
 * functions in the demo's order (0x19DCA0-0x1A045F, after ai_maps.cpp's template tail; no DWARF
 * names their file). Only the extra data's allocator and its free so far; defined here in reverse of the
 * image's order (-inline deferred). */

#include <string.h>

#include "ai.h"

/* the extra data given up (SOCOM 1's) */
void CAiMap::FreeEx()
{
    delete[] m_mapex;
    m_mapex = 0;
    m_mapex_bytes = 0;
}

/* the extra data: bytes per cell, zeroed, the old block given up first (SOCOM 1's; SOCOM II no
 * longer points each scanline into it). The count converted to int: only so does MW load it
 * before the bytes and multiply bytes by it, as the target does */
char *CAiMap::AllocEx(unsigned int bytes)
{
    delete[] m_mapex;
    m_mapex = 0;
    m_mapex_bytes = 0;
    if (bytes) {
        m_mapex_bytes = bytes;
        int n = m_mapex_bytes * (int)m_cellcount;

        m_mapex = new char[n];
        memset(m_mapex, 0, n);
    }
    return m_mapex;
}
