/* fts_missionlist.cpp -- the multiplayer missions: a list of the missions the disc and the
 * downloads hold, by name, id and kind, each name's newest version the one played, the count of
 * multiplayer ones in the "NumMultiMissions" valve; and SOCOM II's message editor (CMsgEditor,
 * C2D's: the chat line's text, its cursor bitmap). A SOCOM II unit with no SOCOM 1 counterpart
 * (OVL1 0x2C8760-0x2CA32F, after fts_net_platform.cpp and before the DNAS client's patch
 * downloader); the file name and every name in it are ours. Only some of it so far. Defined in
 * reverse of their image order (-inline deferred). */

#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

/* a string that is never null (zutil.h's, SOCOM 1's: MSL's basic_string and its assignment) */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    CSaferStr &operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
        return *this;
    }
};

/* the message editor (C2D's: its cursor bitmap at 0x20, its text at 0xA4; only what is written
 * here so far). Its code follows the list's in the image, so it is defined first */
class CMsgEditor {
public:
    void SetMaxLength(int n);

    unsigned char _0[0x140];
    int m_maxlen;   // 0x140: the text's limit, 5 to 100 (5 at construction)
};

/* the text's limit set, kept within 5 to 100 (OVL1 0x2C9960; the name ours) */
void CMsgEditor::SetMaxLength(int n)
{
    if (n < 5)
        n = 5;
    m_maxlen = n;
    if (m_maxlen > 100)
        m_maxlen = 100;
}

/* a mission of the list (0x20 bytes, newed by the list's Add) */
class CMultiMission {
public:
    CSaferStr m_name;   // 0x0
    bool m_multi;   // 0xc: one of the multiplayer missions
    int m_id;   // 0x10
    signed char m_14;   // 0x14
    int m_version;   // 0x18: the newest of a name is the one played
    unsigned char m_1c;   // 0x1c: dropped by the list's purge
};

typedef std::vector<CMultiMission *> CMultiMissionVector;

/* a mission deleted, its slot left null (std::transform's operation) */
struct DeleteMission {
    DeleteMission() {}
    CMultiMission *operator()(CMultiMission *m) const
    {
        delete m;
        return 0;
    }
};

/* the list (data 0x441650): a pointer vector of the missions */
class CMultiMissionList : public CMultiMissionVector {
public:
    CMultiMission *Find(const char *name);
    CMultiMission *Find(int id, bool multi);
    signed char GetMultiValue(int id);
    signed char GetMultiValue(const char *code);
    const char *GetName(int id, bool multi);
    void Purge();
    void Clear();

    CMultiMissionVector m_c;   // 0xc
    CMultiMissionVector m_18;   // 0x18
};

/* every mission deleted and the three vectors emptied (OVL1 0x2C93E0; the destructor's first call) */
void CMultiMissionList::Clear()
{
    std::transform(begin(), end(), begin(), DeleteMission());
    clear();
    m_c.clear();
    m_18.clear();
}

/* the missions flagged at 0x1c taken out and deleted (OVL1 0x2C90B0).
 * OPEN, 46 instructions (the frame and the three saved registers right): the target keeps the
 * iterator both in its stack slot (the element read through it, erase's copy made from it) and in
 * s0 (begin loaded there first; the step, `addiu s0,s0,4`, and erase's result written back from
 * it), where here the slot is reloaded each time. Tried: erase's result straight into i (0xDC, two
 * saved registers), an assigned iterator, a for loop with the declaration */
void CMultiMissionList::Purge()
{
    iterator i = begin();

    while (i != end()) {
        CMultiMission *m = *i;

        if (m->m_1c) {
            iterator next = erase(i);

            delete m;
            i = next;
        } else
            ++i;
    }
}

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

/* the same by a code whose third character on is the id, 0 for a shorter code (OVL1 0x2C8A00) */
signed char CMultiMissionList::GetMultiValue(const char *code)
{
    signed char v = 0;

    if (code && strlen(code) > 2) {
        CMultiMission *m = Find(atoi(code + 2), true);

        if (m)
            v = m->m_14;
    }
    return v;
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
