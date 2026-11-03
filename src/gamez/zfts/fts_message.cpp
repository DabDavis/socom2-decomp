/* fts_message.cpp -- the 2D message queues: C2DMessage_Q, a C2D whose children are the lines of
 * text the mission puts up (the network's notices, the chat), each a C2DMessageString typed out a
 * character at a time and faded in and out. SOCOM 1's code, the unit after fts_action.cpp's
 * CNodeAction as in SOCOM 1's symbol table (which has no DWARF for it, so the file's name is ours):
 * OVL1 0x2B5830-0x2B794F. Written so far: Clear_Messages, C2DMessageString's SetString and Tick, the
 * colour table's setter and accessor (0x2B7530, 0x2B7520), LastMessage (open); the unit's vtable
 * copies of C2D's inline virtuals and the implicit ~C2DMessageString (0x2B63B0) come out byte-exact
 * with them. The rest -- Draw (0x2B5830), Tick (0x2B5DB0), Add_Message (0x2B6530), Init (0x2B6FC0),
 * 0x2B6430, 0x2B7610 -- still to write. Defined in reverse of their image order (-inline deferred). */

#include "../ztwod/ztwod.h"

#include <string.h>
#include <string>

/* a string that is never null (SOCOM 1's zutil.h inline over MSL's basic_string; zui_spec.h's
 * shape: the text assigned, or the string emptied) -- include/string's, whose c_str() tests a bool
 * is_long(): LastMessage's materialised test is it */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

/* a message's line: SOCOM 1's C2DMessageString (a C2DString), SOCOM II's members after the
 * string's 0x98 bytes -- how long it stays up, how long it has been up, how many characters a
 * second it types, and the whole text it is typing (the names ours) */
class C2DMessageString : public C2DString {
public:
    virtual void Tick(float dT);
    virtual void SetString(const char *s, bool translate);

    float m_displayTime;   // 0x98
    float m_time;   // 0x9c
    float m_ttySpeed;   // 0xa0
    CSaferStr m_text;   // 0xa4
};   // 0xb0 bytes

/* the queue: a C2D whose children are its lines (only what Clear_Messages touches: SOCOM II's
 * fields, the names ours) */
class C2DMessage_Q : public C2D {
public:
    void Clear_Messages();
    const char *LastMessage();
    void SetScrolling(bool on) { m_scrolling = on; }

    int m_18;   // 0x18
    unsigned char _1c[0x454 - 0x1c];
    int m_454;   // 0x454
    int m_458;   // 0x458
    unsigned char m_45c;   // 0x45c
    unsigned char _45d[0x4dc - 0x45d];
    unsigned char m_4dc_0 : 1;   // 0x4dc
    unsigned char m_4dc_1 : 1;
    unsigned char m_4dc_2 : 1;
    unsigned char m_scrolling : 1;
    unsigned char m_4dc_4 : 4;
};   // 0x4e0 bytes (the mission's queues' stride)

/* the lines' fade: its speed (alpha a second) and the alpha a line fades in to (OVL1 data) */
extern "C" {
extern float data_003DE6D0;
extern float data_003DE6D8;
}

/* the messages' six colours, red, green, blue and alpha each (OVL1 BSS 0x440C70; the mission's
 * Init and the queue's set them, the name ours) */
extern "C" {
extern float data_00440C70[6][4];
}

/* the colours set: greys, yellows, blues, greens, oranges and reds at alpha 100 (OVL1 0x2B7530) */
extern "C" void func_002B7530()
{
    data_00440C70[0][0] = 128.0f;
    data_00440C70[0][1] = 128.0f;
    data_00440C70[0][2] = 128.0f;
    data_00440C70[0][3] = 100.0f;
    data_00440C70[1][0] = 128.0f;
    data_00440C70[1][1] = 128.0f;
    data_00440C70[1][2] = 64.0f;
    data_00440C70[1][3] = 100.0f;
    data_00440C70[2][0] = 64.0f;
    data_00440C70[2][1] = 64.0f;
    data_00440C70[2][2] = 96.0f;
    data_00440C70[2][3] = 100.0f;
    data_00440C70[3][0] = 64.0f;
    data_00440C70[3][1] = 128.0f;
    data_00440C70[3][2] = 64.0f;
    data_00440C70[3][3] = 100.0f;
    data_00440C70[4][0] = 128.0f;
    data_00440C70[4][1] = 64.0f;
    data_00440C70[4][2] = 32.0f;
    data_00440C70[4][3] = 100.0f;
    data_00440C70[5][0] = 128.0f;
    data_00440C70[5][1] = 64.0f;
    data_00440C70[5][2] = 64.0f;
    data_00440C70[5][3] = 100.0f;
}

/* the colours (OVL1 0x2B7520) */
extern "C" float (*func_002B7520())[4]
{
    return data_00440C70;
}

/* the line typed out: as many of the text's characters as the time allows, then faded in while
 * its time lasts and out after (OVL1 0x2B77A0, slot 0x38) */
void C2DMessageString::Tick(float dT)
{
    m_time += dT;
    float typed = m_time * m_ttySpeed;

    while (m_charheight < typed && strlen(m_string + m_firstCharPos) < strlen(m_text.c_str())) {
        const char *typed_so_far = m_string + m_firstCharPos;

        AddCharacter(m_text.c_str()[strlen(typed_so_far)]);
        UpdatePos();
    }
    float a = m_color[3];

    if (m_time > m_displayTime) {
        if (a != 0.0f) {
            a -= dT * data_003DE6D0;
            if (a < 0.0f)
                a = 0.0f;
            SetTrans(a);
        }
    } else {
        if (a != data_003DE6D8) {
            a += dT * data_003DE6D0;
            if (a > data_003DE6D8)
                a = data_003DE6D8;
            SetTrans(a);
        }
    }
}

/* the line's text set: the string emptied (the flag passed on) and the whole text kept for Tick
 * to type (OVL1 0x2B7720, slot 0x6C) */
void C2DMessageString::SetString(const char *s, bool translate)
{
    C2DString::SetString("", translate);
    m_text = s;
}

/* the queue emptied: each line taken off and deleted, the queue's state cleared (OVL1 0x2B6DE0;
 * SOCOM 1's walk, SOCOM II's clears after it) */
void C2DMessage_Q::Clear_Messages()
{
    iterator i, next, e = end();

    for (i = begin(); i != e; i = next) {
        C2D *m = *i;

        next = erase(i);
        delete m;
    }
    m_18 = 0;
    m_454 = 0;
    m_458 = 0;
    m_45c = 0;
    SetScrolling(false);
}

/* the newest line's text, "" with none (OVL1 0x2B6D60; SOCOM 1's LastMessage, between SetBkgColor
 * and Clear_Messages as here; OVL2's 0x5BB000 calls it).
 * OPEN, 0x68 against 0x78: the walk is the target's to the nops; the text's c_str() is not -- the
 * target materialises the short-form test as a bool (`andi 1; sltu; xori; andi 0xff`, then beqz)
 * and joins the two forms with a `b` over a nop before the return's own `b`, where include/string's
 * c_str (and fts_state.h's) inline to `andi; beqz` with the short form in the delay slot. Tried: an
 * inline GetText() on the line, the ternary, a result local, the null test inverted, all over both
 * strings. The negation materialised and narrowed reads like a bool accessor one inline level
 * deeper than c_str's own (an is_short() over is_long()?) -- a request for include/string */
const char *C2DMessage_Q::LastMessage()
{
    C2DMessageString *last = 0;
    iterator e = end();

    for (iterator i = begin(); i != e; ++i)
        last = (C2DMessageString *)*i;
    if (last)
        return last->m_text.c_str();
    return "";
}
