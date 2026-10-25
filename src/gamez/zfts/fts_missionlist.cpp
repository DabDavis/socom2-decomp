/* fts_missionlist.cpp -- the multiplayer missions: a list of the missions the disc and the
 * downloads hold, by name, id and kind, each name's newest version the one played, the count of
 * multiplayer ones in the "NumMultiMissions" valve; and SOCOM II's message editor (CMsgEditor,
 * C2D's: the chat line's text, its cursor bitmap). A SOCOM II unit with no SOCOM 1 counterpart
 * (OVL1 0x2C8760-0x2CA32F, after fts_net_platform.cpp and before the DNAS client's patch
 * downloader); the file name and every name in it are ours. Only some of it so far. Defined in
 * reverse of their image order (-inline deferred). */

#include <string.h>
#include <string>
#include <vector>

/* a mission of the list (0x20 bytes, newed by the list's Add) */
class CMultiMission {
public:
    std::string m_name;   // 0x0
    bool m_multi;   // 0xc: one of the multiplayer missions
    int m_id;   // 0x10
    signed char m_14;   // 0x14
    int m_version;   // 0x18: the newest of a name is the one played
};

/* the list (data 0x441650): a pointer vector of the missions */
class CMultiMissionList : public std::vector<CMultiMission *> {
public:
    CMultiMission *Find(const char *name);
    CMultiMission *Find(int id, bool multi);
    signed char GetMultiValue(int id);
    const char *GetName(int id, bool multi);
};

/* the mission of an id and kind, none (OVL1 0x2C9040: the list's own walk, the kind narrowed
 * once ahead of it) */
CMultiMission *CMultiMissionList::Find(int id, bool multi)
{
    CMultiMission *m = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (id == (*i)->m_id && multi == (*i)->m_multi) {
            m = *i;
            break;
        }
    return m;
}

/* a mission's name by id and kind, "unknown" without one (OVL1 0x2C8AE0, Find inlined) */
const char *CMultiMissionList::GetName(int id, bool multi)
{
    const char *name = "unknown";
    CMultiMission *m = Find(id, multi);

    if (m)
        name = m->m_name.c_str();
    return name;
}

/* a multiplayer mission's byte at 0x14 by id, 0 without one (OVL1 0x2C8B80, Find inlined) */
signed char CMultiMissionList::GetMultiValue(int id)
{
    signed char v = 0;
    CMultiMission *m = Find(id, true);

    if (m)
        v = m->m_14;
    return v;
}

/* the newest version of a name, none (OVL1 0x2C8760) */
CMultiMission *CMultiMissionList::Find(const char *name)
{
    CMultiMission *best = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (!strcmp((*i)->m_name.c_str(), name) && (!best || (*i)->m_version > best->m_version))
            best = *i;
    return best;
}
