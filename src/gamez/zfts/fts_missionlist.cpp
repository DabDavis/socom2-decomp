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
