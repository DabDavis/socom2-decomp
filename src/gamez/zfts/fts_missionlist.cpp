/* fts_missionlist.cpp -- the multiplayer missions: a list of the missions the disc and the
 * downloads hold, by name, id and kind, each name's newest version the one played, the count of
 * multiplayer ones in the "NumMultiMissions" valve; and SOCOM II's message editor (CMsgEditor,
 * C2D's: the chat line's text, its cursor bitmap). A SOCOM II unit with no SOCOM 1 counterpart
 * (OVL1 0x2C8760-0x2CA32F, after fts_net_platform.cpp and before the DNAS client's patch
 * downloader); the file name and every name in it are ours. Only some of it so far. Defined in
 * reverse of their image order (-inline deferred). */

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

/* the message editor: a C2D with a cursor bitmap and the text typed (the chat line: CHUD's Init and
 * teardown and the mission's destructor reach it at theMission+0x1060, hud_main.cpp's m_messages4).
 * Its code follows the list's in the image, so it is defined first; the names are ours */
class CMsgEditor : public C2D {
public:
    void SetMaxLength(int n);
    void DeleteCharacter();
    void AddCharacter(char c);
    void Clear();
    void Init(C2DFont *font);
    void Submit();

    C2DBitmap m_cursor;   // 0x20
    float m_cursorTrans;   // 0xa0: the cursor's alpha, faded in while there is text
    C2DString m_text;   // 0xa4
    float m_textTrans;   // 0x13c: the text's alpha
    int m_maxlen;   // 0x140: the text's limit, 5 to 100 (5 at construction)
};

/* the line's bounds (OVL1 .data floats: its left, top, right and bottom edges and a character's
 * width) and the text's position */
extern float data_003DE7A8;   /* 25 */
extern float data_003DE7B0;   /* 338 */
extern float data_003DE7B8;   /* 615 */
extern float data_003DE7C0;   /* 360 */
extern float data_003DE7E0;   /* 3 */
extern float data_00441678;
extern float data_00441680;

char *_strdup(const char *s, const char *file, int line);
void _free(void *p, const char *file, int line);
bool ParseWhisper(char *args);
void SendWhisper(char *name, char *msg);

/* the text loaded in the font given and the cursor's black box over the line, both faded out and
 * made children (OVL1 0x2CA100: CHUD::Init's) */
void CMsgEditor::Init(C2DFont *font)
{
    m_cursorTrans = 0.0f;
    m_textTrans = 0.0f;
    m_text.Load(0.8f, 0, font, data_00441678, data_00441680, true);
    m_text.SetTrans(m_textTrans);
    m_text.SetColor(100.0f, 100.0f, 20.0f);
    m_cursor.Load(data_003DE7A8, data_003DE7B0, data_003DE7B8, data_003DE7C0, 0);
    m_cursor.SetHasTexture(false);
    m_cursor.SetTrans(m_cursorTrans);
    m_cursor.SetColor(0.0f, 0.0f, 0.0f);
    push_back(&m_cursor);
    push_back(&m_text);
}

/* the text wiped, both alphas zero, the cursor told (OVL1 0x2CA0B0: CHUD::Init's) */
void CMsgEditor::Clear()
{
    m_text.ClearAndWipe();
    m_cursorTrans = 0.0f;
    m_textTrans = 0.0f;
    m_cursor.SetTrans(m_cursorTrans);
}

/* a character typed at the end while the line has room and the limit allows (OVL1 0x2C9D90) */
void CMsgEditor::AddCharacter(char c)
{
    int len;
    char *s = m_text.m_string + m_text.m_firstCharPos;
    char buf[100];

    len = s ? strlen(s) : 0;

    if (m_text.m_charheight < (data_003DE7B8 - data_003DE7A8) - 4.0f * data_003DE7E0 &&
        len < m_maxlen - 2) {
        if (len > 0)
            strcpy(buf, s);
        buf[len] = c;
        buf[++len] = 0;
        m_text.SetString(buf, true);
        m_text.UpdatePos();
    }
}

/* the last character taken off (OVL1 0x2C9D00) */
void CMsgEditor::DeleteCharacter()
{
    int len;
    char *s = m_text.m_string + m_text.m_firstCharPos;
    char buf[100];

    len = s ? strlen(s) : 0;

    if (len > 0) {
        strcpy(buf, s);
        buf[len - 1] = 0;
        m_text.SetString(buf, true);
        m_text.UpdatePos();
    }
}

/* the line entered: a "/w" command's arguments handed to the whisper's parser, then the text
 * emptied (OVL1 0x2C9B80) */
void CMsgEditor::Submit()
{
    char *s = m_text.m_string + m_text.m_firstCharPos;
    int len = s ? strlen(s) : 0;

    if (len > 0) {
        char *text = m_text.m_string + m_text.m_firstCharPos;
        char cmd[8] = "xxxx";
        int i = 0;

        if (*text == '/') {
            i++;
            bool done = false;
            char *p = cmd;
            int n = 0;

            while (i < len && !done) {
                char c = text[i];

                if (c == ' ' || c == 0 || n >= 7) {
                    *p = 0;
                    done = true;
                } else {
                    *p = c;
                    i++;
                    p++;
                    n++;
                }
            }
            if (strlen(cmd) && !strcasecmp(cmd, "w"))
                ParseWhisper(text + i);
        }
    }
    m_text.SetString(0, true);
    m_text.UpdatePos();
}

/* a "/w" command's arguments -- ` name message` or ` "a name" message` -- split in a copy and the
 * whisper sent; whether it was (OVL1 0x2C9990; the name ours) */
bool ParseWhisper(char *args)
{
    unsigned int len = args ? strlen(args) : 0;
    bool sent = false;

    if (len > 1 && *args == ' ') {
        char *buf = _strdup(args + 1, 0, 0);
        unsigned int n = strlen(buf);
        unsigned int i = 0;
        char *name = 0;
        char *msg = 0;

        if (*buf == '"') {
            i++;
            while (i < n && !name) {
                if (buf[i] == '"') {
                    buf[i] = 0;
                    i++;
                    if (strlen(buf))
                        name = buf + 1;
                } else
                    i++;
            }
        } else {
            while (i < n && !name) {
                if (buf[i] == ' ') {
                    buf[i] = 0;
                    i++;
                    if (strlen(buf))
                        name = buf;
                } else
                    i++;
            }
        }
        if (name) {
            while (i < n && !msg) {
                if (buf[i] != ' ')
                    msg = &buf[i];
                else
                    i++;
            }
        }
        if (name && msg) {
            sent = true;
            SendWhisper(name, msg);
        }
        if (buf)
            _free(buf, 0, 0);
    }
    return sent;
}

/* the text's limit set, kept within 5 to 100 (OVL1 0x2C9960; the name ours) */
void CMsgEditor::SetMaxLength(int n)
{
    if (n < 5)
        n = 5;
    m_maxlen = n;
    if (m_maxlen > 100)
        m_maxlen = 100;
}

/* the world's entities as the whisper reads them (zentity.h's CEntity: its team mask and its alive
 * bit, the thirteenth of the flags at 0xe0; zentity.h's CSaferStr clashes with this unit's) */
class CEntity;

class CEntityList : public std::list<CEntity *> {
public:
    CEntity *GetEntity(const char *name) const;
};

class CEntity {
public:
    bool IsAlive() const { return m_isAlive; }

    static CEntityList m_list;

    unsigned char _0[0xc8];
    unsigned int m_team;   // 0xc8
    unsigned char _cc[0xe0 - 0xcc];
    unsigned int m_flags : 12;   // 0xe0
    unsigned int m_isAlive : 1;
};

CEntity *ftsGetPlayer();
extern "C" bool func_002C2FD0(CEntity *e);
extern "C" void func_002B6530(void *queue, const char *text, int a, int b, int c, float time);   /* C2DMessage_Q::Add_Message */

/* the mission (only its first message queue) */
class CMissionMessages {
public:
    unsigned char _0[0x1c0];
    unsigned char m_messages[0x4e0];   // 0x1c0: a C2DMessage_Q
};

extern CMissionMessages theMission;

class CZOnlineLobby {
public:
    void SendChat(const char *msg, const char *name);
};

extern CZOnlineLobby theLobby;

/* a whisper sent to a player by name, when there is one on the player's team, alive or dead alike
 * and of the same persona state -- else the queue says the player is unavailable; either way what
 * was sent shown (OVL1 0x2C9810; the name ours) */
void SendWhisper(char *name, char *msg)
{
    if (name && msg) {
        void *queue;
        bool ok;
        CEntity *target = CEntity::m_list.GetEntity(name);

        queue = theMission.m_messages;
        ok = true;

        if (target) {
            CEntity *player = ftsGetPlayer();

            if (player == target || !(target->m_team & player->m_team) ||
                player->IsAlive() != target->IsAlive() ||
                func_002C2FD0(player) != func_002C2FD0(target)) {
                char buf[128];

                ok = false;
                sprintf(buf, ":: Player \"%s\" is unavailable", name);
                func_002B6530(queue, buf, 2, 0, 0, 0.0f);
            }
        }
        if (ok) {
            theLobby.SendChat(msg, name);
            func_002B6530(queue, msg, 0, 0, 0, 0.0f);
        }
    }
}

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
    CMultiMission *Find(const char *name);
    CMultiMission *Find(int id, bool multi);
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

/* whether a mission is its name's newest (OVL1 0x2C94F0, a draft's match) */
extern "C" bool func_002C94F0(CMultiMission *m);

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
 * for a multiplayer one, the "NumMultiMissions" count taken again (OVL1 0x2C91A0; 0x278B80 calls it) */
CMultiMission *CMultiMissionList::Add(const MissionDesc *desc)
{
    CMultiMission *m = Find(desc->m_id, desc->m_multi);

    if (!m) {
        m = new CMultiMission(desc->m_name, desc->m_id, desc->m_type, desc->m_multi);
        push_back(m);
        if (desc->m_multi) {
            int count = 0;

            for (iterator i = begin(); i != end(); ++i)
                if ((*i)->m_multi && func_002C94F0(*i))
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
                if (func_002C94F0(at(i)))
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
            if (m->m_multi && func_002C94F0(m))
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
