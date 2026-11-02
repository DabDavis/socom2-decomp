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

#include "../ztwod/ztwod.h"

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

    C2DBitmap m_cursor;   // 0x20
    float m_cursorTrans;   // 0xa0: the cursor's alpha, faded in while there is text
    C2DString m_text;   // 0xa4
    float m_textTrans;   // 0x13c: the text's alpha
    int m_maxlen;   // 0x140: the text's limit, 5 to 100 (5 at construction)
};

/* the line's bounds (OVL1 .data floats: its right edge, left edge and a character's width) */
extern float data_003DE7A8;   /* 25 */
extern float data_003DE7B8;   /* 615 */
extern float data_003DE7E0;   /* 3 */

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
