/* fts_missionlist.cpp -- the multiplayer missions: a list of the missions the disc and the
 * downloads hold, by name, id and kind, each name's newest version the one played, the count of
 * multiplayer ones in the "NumMultiMissions" valve. A SOCOM II unit with no SOCOM 1 counterpart
 * (OVL1 0x2C8760-0x2C980F, after fts_net_platform.cpp and before the message editor's unit,
 * fts_msgeditor.cpp: each has an initialiser of its own, this one's 0x4007D0 building the list
 * 0x441650 and registering its destructor 0x2C9720, this unit's last function); the file name and
 * every name in it are ours. Only some of it so far. Defined in reverse of their image order
 * (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

#include "../ztwod/ztwod.h"
#include "../zrdr/zrdr.h"

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

/* a mission of the list (0x20 bytes, newed by the list's Add) */
class CMultiMission {
public:
    CMultiMission(const _zrdr *rec);
    /* a mission from a downloaded one's description (ours: inline, the version cleared first; the
     * parameters' order numbers the inlined temporaries -- name, id, type, kind is one of the four
     * orders that give the target's registers) */
    CMultiMission(const char *name, int id, unsigned char type, bool multi) : m_version(0)
    {
        m_name = name;
        m_id = id;
        m_multi = multi;
        m_14 = type;
    }

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
    /* the missions deleted, then the vectors freed (OVL1 0x2C9720, the unit's last function: the
     * initialiser 0x4007D0 registers it for the list) */
    /* the vectors built, the base's count cleared again (the initialiser stores the base's size
     * word after the members' words: the constructor's clear() makes the first store dead -- the
     * body ours, the shape the target's) */
    CMultiMissionList() { clear(); }
    ~CMultiMissionList() { Clear(); }

    CMultiMission *Find(const char *name);
    CMultiMission *Find(int id, bool multi);
    CMultiMission *GetNext(int id, bool multi);
    signed char GetMultiValue(int id);
    signed char GetMultiValue(const char *code);
    const char *GetName(int id, bool multi);
    void Purge();
    void Clear();
    void Read(const _zrdr *rdr);
    CMultiMission *GetRandom(bool multi);
    CMultiMission *Add(const struct MissionDesc *desc);

    /* the multiplayer missions that are their names' newest, and the rest, by index into the list
     * (GetRandom's pools, rebuilt when the counts no longer add up; the names ours) */
    std::vector<unsigned int> m_c;   // 0xc
    std::vector<unsigned int> m_18;   // 0x18
};

/* the list itself (zanim_menu.cpp's UIGetAvailableMPMissions, CMission::Init and the lobby's
 * filters read it): built in place, its three vectors' words cleared, by the unit's initialiser
 * __sinit_fts_missionlist.cpp 0x4007D0, which registers ~CMultiMissionList */
extern "C" {
CMultiMissionList data_00441650;
}

/* the game types a mission's TYPE names (the names ours, as the rdr spells them) */
enum MISSION_TYPE {
    MISSION_NONE,
    MISSION_BREACH,
    MISSION_DEMOLITION,
    MISSION_ESCORT,
    MISSION_EXTRACTION,
    MISSION_SUPPRESSION
};

/* a mission from its rdr record: its name, number and version, whether it is a multiplayer one, its
 * game type (OVL1 0x2C9540; the list's Read news each) */
CMultiMission::CMultiMission(const _zrdr *rec)
{
    m_name = zrdr_findstring(rec, "NAME");
    m_id = 0;
    zrdr_findint(rec, "NUMBER", &m_id, 1);
    m_version = 0;
    zrdr_findint(rec, "VERSION", &m_version, 1);
    if (zrdr_findtag(rec, "MULTIPLAYER"))
        m_multi = true;
    else
        m_multi = false;
    m_1c = 0;
    const char *type = zrdr_findstring(rec, "TYPE");

    if (!strcasecmp(type, "DEMOLITION"))
        m_14 = MISSION_DEMOLITION;
    else if (!strcasecmp(type, "EXTRACTION"))
        m_14 = MISSION_EXTRACTION;
    else if (!strcasecmp(type, "SUPPRESSION"))
        m_14 = MISSION_SUPPRESSION;
    else if (!strcasecmp(type, "BREACH"))
        m_14 = MISSION_BREACH;
    else if (!strcasecmp(type, "ESCORT"))
        m_14 = MISSION_ESCORT;
    else
        m_14 = MISSION_NONE;
}

/* whether a mission is its name's newest: the list's find by name gives it back (OVL1 0x2C94F0, the name ours;
 * pinned: the list's own Read, GetRandom and Purge call it out of line, as zanim_menu.cpp does) */
#pragma auto_inline off
bool IsNewestVersion(CMultiMission *m)
{
    return m == data_00441650.Find(m->m_name.c_str());
}
#pragma auto_inline reset

/* a valve (zanim.h's CValve: only its creation, its value and the callbacks told of a change) */
enum VALVE_TYPE { VALVE_ANY = 0, VALVE_PERM = 1, VALVE_TEMP = 2, VALVE_PERSIST = 3 };
enum VALVE_STATE {
    VALVE_SAME,
    VALVE_ON,
    VALVE_OFF
};

class CValve {
public:
    static CValve *Create(const char *name, VALVE_TYPE type);
    void MakeCallbacks(VALVE_STATE state);

    char *m_name;   // 0x0
    unsigned short m_value;   // 0x4
};

/* CValve::SetValue, inlined (fts_mission.cpp's helper) */
inline void SetValveValue(CValve *valve, int value)
{
    VALVE_STATE state = VALVE_SAME;

    if (valve->m_value != value)
        state = value == 0 ? VALVE_OFF : VALVE_ON;
    valve->m_value = value;
    if (state != VALVE_SAME)
        valve->MakeCallbacks(state);
}

/* a downloaded mission's description as 0x278B80 hands it over: its number, whether it is a
 * multiplayer one, its game type and its name (the names ours) */
struct MissionDesc {
    unsigned char _0[2];
    unsigned char m_id;   // 0x2
    unsigned char _3[2];
    unsigned char m_multi;   // 0x5
    unsigned char m_type;   // 0x6
    char m_name[1];   // 0x7
};

/* a downloaded mission entered once: the one of its number and kind, else a new one pushed and,
 * for a multiplayer one, the "NumMultiMissions" count taken again (OVL1 0x2C91A0; 0x278B80 calls it).
 * Find(int, bool)'s walk written out (see Find): the number and kind taken first as Find's arguments
 * would be -- the kind converted to a bool --, the walk's own result in a register and the mission
 * kept a copy of it (push_back homes only that copy) */
CMultiMission *CMultiMissionList::Add(const MissionDesc *desc)
{
    int id = desc->m_id;
    bool multi = desc->m_multi;
    CMultiMission *found = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (id == (*i)->m_id && multi == (*i)->m_multi) {
            found = *i;
            break;
        }
    CMultiMission *m = found;

    if (!m) {
        m = new CMultiMission(desc->m_name, desc->m_id, desc->m_type, desc->m_multi);
        push_back(m);
        if (desc->m_multi) {
            int count = 0;

            for (iterator i = begin(); i != end(); ++i)
                if ((*i)->m_multi && IsNewestVersion(*i))
                    count++;
            SetValveValue(CValve::Create("NumMultiMissions", VALVE_PERM), count);
        }
    }
    return m;
}

/* a mission at random -- a multiplayer one that is its name's newest, or one of the rest -- the
 * pools rebuilt first when the list has changed; the pick swapped to its pool's end (OVL1 0x2C8840;
 * the menu's random mission, zanim_menu.cpp) */
CMultiMission *CMultiMissionList::GetRandom(bool multi)
{
    if (m_c.size() + m_18.size() != size()) {
        m_c.clear();
        m_18.clear();
        for (unsigned int i = 0; i < size(); i++) {
            if (at(i)->m_multi) {
                if (IsNewestVersion(at(i)))
                    m_c.push_back(i);
            } else
                m_18.push_back(i);
        }
    }
    std::vector<unsigned int> &pool = multi ? m_c : m_18;
    int n = pool.size();
    int k = (int)(0.99f * (n - 1) * (rand() * (1.0f / 2147483648.0f))) % n;
    unsigned int t = pool[k];

    pool[k] = pool[n - 1];
    pool[n - 1] = t;
    return at(t);
}

/* the missions an rdr lists added, the multiplayer ones that are their names' newest counted into
 * the "NumMultiMissions" valve (OVL1 0x2C8C10; CMission::Init calls it). The item taken into a
 * local ahead of the new (written as the constructor's argument MW reads it after the allocation);
 * an int counter against the unsigned count keeps the target's per-pass `sll` (unsigned, MW walks
 * the address in a register of its own) */
void CMultiMissionList::Read(const _zrdr *rdr)
{
    int count = 0;

    if (rdr) {
        _zrdr *list = rdr->Item(0);

        reserve(size() + list->Count());
        for (int i = 0; i < list->Count(); i++) {
            _zrdr *item = list->Item(i);
            CMultiMission *m = new CMultiMission(item);

            push_back(m);
            if (m->m_multi && IsNewestVersion(m))
                count++;
        }
    }
    SetValveValue(CValve::Create("NumMultiMissions", VALVE_PERM), count);
}

/* every mission deleted and the three vectors emptied (OVL1 0x2C93E0; the destructor's first call) */
void CMultiMissionList::Clear()
{
    std::transform(begin(), end(), begin(), DeleteMission());
    clear();
    m_c.clear();
    m_18.clear();
}

/* the missions flagged at 0x1c taken out and deleted (OVL1 0x2C90B0). The next element in a
 * register (s0: erase's result, or the step), the walking iterator in memory (read for the element
 * and for erase's by-value copy) and assigned from it at the join; the chained initialisation
 * `i = next = begin()` is what lets the entry test compare the register (`iterator i = next`, a
 * while loop, a separate begin() or a declared-ahead `i` all reload the iterator's slot there) */
void CMultiMissionList::Purge()
{
    iterator next;

    for (iterator i = next = begin(); i != end(); i = next) {
        CMultiMission *m = *i;

        if (m->m_1c) {
            next = erase(i);
            delete m;
        } else
            ++next;
    }
}

/* the mission of an id and kind, none (OVL1 0x2C9040: the list's own walk, the kind narrowed
 * once ahead of it; the online lobby's GetGameInfo and HasMissions call it). The four callers here
 * (GetName, both GetMultiValues, Add) hold the same walk WRITTEN OUT, not this one inlined: the
 * pointer vector's end() is four inline levels high (include/vector), so `i != end()` -- the end an
 * argument, a level below the != -- is five high in this copy and six inside an inlined Find, and
 * at -inline level=5 the callers' copies would call the raw end and __iter. The end taken into a
 * local before the loop inlines whole but colours differently: MW generates a for loop's test after
 * its body, so the end the test's inlined end() makes is numbered after the narrowed kind and the
 * element (the target colours it first: a0, the element a2, the kind a3, the iterator t0), where a
 * local end is numbered ahead of them (9-11 off), and so is the online lobby's `data_ + size_` end
 * (its round 20's Last(): 11 off here). Written out at the caller's own level the test's
 * end() inlines whole and numbers last, as here. (The states' round 36; until then the unit carried
 * `#pragma inline_depth(6)` -- the MSL headers' round-24 stand-in.) */
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

/* the mission after the one of an id and kind: the next of that kind that is its name's newest,
 * round from the end to the start, or the first such when no mission has that id and kind (OVL1
 * 0x2C8DB0: zanim_menu.cpp's UIAdvanceMPMission asks it for the mission after the current one; the
 * name ours). Find(int, bool)'s walk written out (see Find), then std::find's two iterators copied
 * through the FPRs into frame temporaries, the found one kept in its slot for the second search's
 * end. The first search's own iterator initialised from the one after the found (a loop with an
 * init: its entry test at the top, the narrowed kind in that loop's preheader, so the second
 * search narrows it again -- `for (; i != end(); ++i)` jumps to a bottom test and shares one
 * narrowing), the second search's iterator a loop-scoped one (the found mission's cursor reused,
 * its two registers swap) -- the MSL headers round 26, on include/vector's iterator as it stands */
CMultiMission *CMultiMissionList::GetNext(int id, bool multi)
{
    CMultiMission *cur = 0;
    CMultiMission *next = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (id == (*i)->m_id && multi == (*i)->m_multi) {
            cur = *i;
            break;
        }
    if (cur) {
        iterator at = std::find(begin(), end(), cur);
        iterator from = at;

        if (++from == end())
            from = begin();
        for (iterator i = from; i != end(); ++i)
            if ((*i)->m_multi == multi && IsNewestVersion(*i)) {
                next = *i;
                break;
            }
        if (!next)
            for (iterator i = begin(); i != at; ++i)
                if ((*i)->m_multi == multi && IsNewestVersion(*i)) {
                    next = *i;
                    break;
                }
    } else {
        for (iterator i = begin(); i != end(); ++i)
            if ((*i)->m_multi == multi && IsNewestVersion(*i)) {
                next = *i;
                break;
            }
    }
    return next;
}

/* a mission's name by id and kind, "unknown" without one (OVL1 0x2C8AE0: Find(int, bool)'s walk
 * written out -- see Find) */
const char *CMultiMissionList::GetName(int id, bool multi)
{
    const char *name = "unknown";
    CMultiMission *m = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (id == (*i)->m_id && multi == (*i)->m_multi) {
            m = *i;
            break;
        }

    if (m)
        name = m->m_name.c_str();
    return name;
}

/* the same by a code whose third character on is the id, 0 for a shorter code (OVL1 0x2C8A00:
 * Find(id, true)'s walk written out -- see Find) */
signed char CMultiMissionList::GetMultiValue(const char *code)
{
    signed char v = 0;

    if (code && strlen(code) > 2) {
        int id = atoi(code + 2);
        CMultiMission *m = 0;

        for (iterator i = begin(); i != end(); ++i)
            if (id == (*i)->m_id && true == (*i)->m_multi) {
                m = *i;
                break;
            }

        if (m)
            v = m->m_14;
    }
    return v;
}

/* a multiplayer mission's byte at 0x14 by id, 0 without one (OVL1 0x2C8B80: Find(id, true)'s walk
 * written out -- see Find) */
signed char CMultiMissionList::GetMultiValue(int id)
{
    signed char v = 0;
    CMultiMission *m = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (id == (*i)->m_id && true == (*i)->m_multi) {
            m = *i;
            break;
        }

    if (m)
        v = m->m_14;
    return v;
}

/* the newest version of a name, none (OVL1 0x2C8760; pinned: IsNewestVersion 0x2C94F0, its one caller here, calls
 * it out of line) */
#pragma auto_inline off
CMultiMission *CMultiMissionList::Find(const char *name)
{
    CMultiMission *best = 0;

    for (iterator i = begin(); i != end(); ++i)
        if (!strcmp((*i)->m_name.c_str(), name) && (!best || (*i)->m_version > best->m_version))
            best = *i;
    return best;
}
#pragma auto_inline reset
