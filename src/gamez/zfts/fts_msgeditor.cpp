/* fts_msgeditor.cpp -- SOCOM II's message editor (CMsgEditor, C2D's: the chat line's text, its
 * cursor bitmap) and the whisper it sends (OVL1 0x2C9810-0x2CA32F, after fts_missionlist.cpp and
 * before the DNAS client's patch downloader). A unit of its own: it has its own initialiser,
 * __sinit_fts_msgeditor.cpp 0x400830 (the text's position worked out from the line's bounds),
 * after fts_missionlist.cpp's 0x4007D0, and one unit has one initialiser; the floats it reads are
 * this unit's .data (0x3DE798-0x3DE7E0, read only by its functions). A SOCOM II unit with no
 * SOCOM 1 counterpart: the file name and every name in it are ours. Complete: its thirteen rows and
 * its initialiser byte-exact. Defined in reverse of their image order (-inline deferred). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>

#include "../ztwod/ztwod.h"
#include "../zrdr/zrdr.h"

/* the message editor: a C2D with a cursor bitmap and the text typed (the chat line: CHUD's Init and
 * teardown and the mission's destructor reach it at theMission+0x1060, hud_main.cpp's m_messages4;
 * the names are ours) */
class CMsgEditor : public C2D {
public:
    CMsgEditor();
    virtual ~CMsgEditor();
    virtual void Tick(float dT);
    void UnInit();
    void SetMaxLength(int n);
    void DeleteCharacter();
    void Cancel();
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
extern float data_003DE798;   /* the cursor's full alpha */
extern float data_003DE7A0;   /* and its fade's rate a second */
extern float data_003DE7C8;   /* the text's full alpha */
extern float data_003DE7D0;   /* and its rate */
extern float data_003DE7A8;   /* 25 */
extern float data_003DE7B0;   /* 338 */
extern float data_003DE7B8;   /* 615 */
extern float data_003DE7C0;   /* 360 */
extern float data_003DE7D8;   /* 6 */
extern float data_003DE7E0;   /* 3 */

/* the text's position, a character's width in from the left edge and 6 up from the bottom (the
 * unit's initialiser __sinit_fts_msgeditor.cpp 0x400830 works them out from the bounds) */
float data_00441678 = data_003DE7A8 + data_003DE7E0;
float data_00441680 = data_003DE7C0 - data_003DE7D8;

char *_strdup(const char *s, const char *file, int line);
void _free(void *p, const char *file, int line);
bool ParseWhisper(char *args);
void SendWhisper(char *name, char *msg);

/* both alphas clear, the limit 5 (OVL1 0x2CA2D0: CMission()'s, for its +0x1060 editor) */
CMsgEditor::CMsgEditor()
{
    m_cursorTrans = 0.0f;
    m_textTrans = 0.0f;
    m_maxlen = 5;
}

/* (OVL1 0x2CA240: the editor's only virtual of its own -- its vtable 0x4064B0's slot 2 --, so
 * written here, which is where the vtable comes out; the text, the cursor -- C2DBitmap's implicit
 * destructor inlined -- and the C2D base torn down) */
CMsgEditor::~CMsgEditor()
{
}

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

/* the editor taken down: the text wiped and the children let go of (OVL1 0x2CA050: ~CHUD's call,
 * Init's counterpart; the name ours) */
void CMsgEditor::UnInit()
{
    m_text.ClearAndWipe();
    clear();
}

/* the cursor and the text faded in, each to its full alpha, while there is text, and out to
 * nothing while there is none (OVL1 0x2C9E90: C2D's Tick slot) */
void CMsgEditor::Tick(float dT)
{
    char *s = m_text.m_string + m_text.m_firstCharPos;
    bool has = s && strlen(s);

    if (has) {
        if (m_cursorTrans < data_003DE798) {
            m_cursorTrans += dT * data_003DE7A0;
            if (m_cursorTrans > data_003DE798)
                m_cursorTrans = data_003DE798;
            m_cursor.SetTrans(m_cursorTrans);
        }
        if (m_textTrans < data_003DE7C8) {
            m_textTrans += dT * data_003DE7D0;
            if (m_textTrans > data_003DE7C8)
                m_textTrans = data_003DE7C8;
            m_text.SetTrans(m_textTrans);
        }
    } else {
        if (m_cursorTrans > 0.0f) {
            m_cursorTrans -= dT * data_003DE7A0;
            if (m_cursorTrans < 0.0f)
                m_cursorTrans = 0.0f;
            m_cursor.SetTrans(m_cursorTrans);
        }
        if (m_textTrans > 0.0f) {
            m_textTrans -= dT * data_003DE7D0;
            if (m_textTrans < 0.0f)
                m_textTrans = 0.0f;
            m_text.SetTrans(m_textTrans);
        }
    }
}

/* a character typed at the end while the line has room and the limit allows (OVL1 0x2C9D90) */
void CMsgEditor::AddCharacter(char c)
{
    int len;
    char *s = m_text.m_string + m_text.m_firstCharPos;
    char buf[100];

    len = s ? strlen(s) : 0;

    if (m_text.m_length < (data_003DE7B8 - data_003DE7A8) - 4.0f * data_003DE7E0 &&
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

/* the line dropped unsent: the text emptied (OVL1 0x2C9CC0: the chat keyboard's escape key,
 * fts_controller.cpp's 0x2C6C70; the name ours) */
void CMsgEditor::Cancel()
{
    m_text.SetString(0, true);
    m_text.UpdatePos();
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
