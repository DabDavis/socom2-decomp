/* compass.cpp -- the compass and its anchors: every point the HUD compass shows (objectives,
 * vehicles, bases), made, freed and listed here (OVL1 0x2A27E0-0x2A2EAF, its initialiser 0x4000C0
 * building the one compass). SOCOM 1's CCompass::CreateAnchor, DeleteAnchor, Close and Open
 * (0x24E1A0-0x24E497, without DWARF); SOCOM II gives each anchor a list of points with a current
 * one, and puts the unit right after the entity's. The file name is ours. Defined in reverse of
 * their image order (-inline deferred); the implicit ~CCompass, which the initialiser registers,
 * closes the unit. */

#include <string.h>

#include "../zsys/zsys.h"
#include "zentity.h"

CCompass theCompass;

/* a point added to the anchor's list, or found there: its index (new in SOCOM II; the name is
 * ours) */
int CCompassAnchor::AddPoint(const CPnt3D *p)
{
    unsigned int i;
    unsigned int n = m_points.size();

    for (i = 0; i < n; i++)
        if (*p == m_points[i])
            return i;
    m_points.push_back(*p);
    return m_points.size() - 1;
}

/* the current point, -1 for none or an index past the list (new in SOCOM II; the name is ours) */
void CCompassAnchor::SetPoint(int index)
{
    if (index < 0)
        index = -1;
    else if ((unsigned int)index >= m_points.size())
        index = -1;
    m_point = index;
}

/* the current point, if there is one, noted as the last one given (new in SOCOM II; the name is
 * ours) */
bool CCompassAnchor::GetPoint(CPnt3D *out)
{
    if (m_point != -1) {
        if (m_point != m_lastPoint) {
            m_lastPoint = m_point;
            *out = m_points[m_point];
        } else
            *out = m_points[m_point];
        return true;
    }
    return false;
}

/* the anchor's name, a copy (SOCOM 1's CreateAnchor inlines the same free and strdup; ent_main.cpp
 * calls this copy) */
void CCompassAnchor::SetName(const char *name)
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_name = 0;
    if (name)
        m_name = _strdup(name, 0, 0);
}

/* room for 48 anchors (SOCOM 1's) */
void CCompass::Open()
{
    reserve(48);
}

/* every anchor dropped and the list's storage given up (SOCOM 1's: the clear, then the free idiom
 * on the vector itself -- its temporary is a vector of anchors, three destructor levels, not a
 * compass) */
void CCompass::Close()
{
    clear();
    FreeVector<std::vector<CCompassAnchor *> >(*this);
}

/* an anchor at a position, named, on the compass (SOCOM 1's).
 * OPEN, 4 bytes short: the target inlines the anchor's constructor whole -- its vector<CPnt3D>
 * member's three stores, the bits, the position -- where include/vector's class vector (four
 * constructor levels) makes the constructor five high, so MW inlines only its body and calls the
 * vector's constructor, CPnt3D's and the copy. A three-high class vector matches it (and CEntity's
 * constructor) but loses CZAnimSet::SaveNodeStates, which needs `new CZAnimSetSave` five high
 * with plain vector members (the probes: build/scratch/ent/zss.cpp, h5.cpp) */
CCompassAnchor *CCompass::CreateAnchor(const CPnt3D &pos, const char *name)
{
    CCompassAnchor *anchor = new CCompassAnchor;

    anchor->m_posW = pos;
    anchor->SetName(name);
    theCompass.push_back(anchor);
    return anchor;
}

/* an anchor off the compass -- swapped with the last and popped -- and deleted: whether it was
 * there (SOCOM 1's; the compass through a local, its address kept in t1 as both builds do).
 * OPEN, 0x20 short: the target inlines the anchor's destructor whole (SetName(0)'s free and two
 * stores, then the point list's ~vector and ~__vector_imp this-tests and a call to
 * ~__vector_deleter<CPnt3D>), where include/vector's class-vector destructor is six high and MW
 * inlines the anchor's destructor one level (SetName and ~vector called); the loop's registers
 * also differ (n in a1, not t0) */
bool CCompass::DeleteAnchor(CCompassAnchor *anchor)
{
    CCompass &compass = theCompass;
    bool found = false;

    if (anchor) {
        unsigned int n = compass.size();

        if (n) {
            for (unsigned int i = 0; i < n; i++) {
                if (compass[i] == anchor) {
                    if (n >= 2 && i != n - 1) {
                        CCompassAnchor *t = compass[i];

                        compass[i] = compass[n - 1];
                        compass[n - 1] = t;
                    }
                    compass.pop_back();
                    found = true;
                    n = compass.size();
                }
            }
        }
    }
    delete anchor;
    return found;
}
