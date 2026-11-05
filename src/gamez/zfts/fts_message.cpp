/* fts_message.cpp -- the 2D message queues: C2DMessage_Q, a C2D whose children are the lines of
 * text the mission puts up (the network's notices, the chat), each a C2DMessageString typed out a
 * character at a time and faded in and out. SOCOM 1's code, the unit after fts_action.cpp's
 * CNodeAction as in SOCOM 1's symbol table (which has no DWARF for it, so the file's name is ours):
 * OVL1 0x2B5830-0x2B794F. Written so far: Clear_Messages, C2DMessageString's SetString and Tick, the
 * colour table's setter and accessor (0x2B7530, 0x2B7520), the constructor (0x2B7610), Init
 * (0x2B6FC0, the states' round 41), LastMessage (open); the unit's vtable copies of C2D's inline
 * virtuals and the implicit ~C2DMessageString (0x2B63B0) come out byte-exact with them. The rest --
 * Draw (0x2B5830), Tick (0x2B5DB0), Add_Message (0x2B6530), 0x2B6430 -- still to write. Defined in
 * reverse of their image order (-inline deferred). */

#include "../ztwod/ztwod.h"

#include <string.h>
#include <string>

/* a texture's handle and the asset manager's list (znode.h's: only the handle's texture and the
 * lookup by name -- zdb::CAssetMgr::m_assets, OVL1 bss 0x45C3C0) */
namespace zdb {
class CTexture;
class CTexHandle {
public:
    char *m_name;   // 0x0
    int m_count;   // 0x4
    unsigned int m_gsAddr;   // 0x8
    CTexture *m_texture;   // 0xc
    unsigned char m_libref;   // 0x10
};

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

/* the rdr reader (zrdr.h's: a file is read as its root value) */
class CRdrFile;
int zrdr_findint(const _zrdr *rdr, const char *tag, int *v, int n);
int zrdr_findreal(const _zrdr *rdr, const char *tag, float *v, int n);
const char *zrdr_findstring(const _zrdr *rdr, const char *tag);
_zrdr *zrdr_findtag(const _zrdr *rdr, const char *tag);
extern "C" int strcasecmp(const char *a, const char *b);

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

/* the queue: a C2D whose children are its lines (SOCOM 1's DWARF names the members from the
 * scale on, 0x2E0 further on in SOCOM II, whose two backgrounds are C2DBitmapPolys; SOCOM 1's
 * three bools are bits of the byte at 0x4DC. The names at 0x18 and 0x454-0x45C ours) */
class C2DMessage_Q : public C2D {
public:
    C2DMessage_Q();
    virtual ~C2DMessage_Q();
    void Clear_Messages();
    const char *LastMessage();
    void SetScrolling(bool on) { m_scrolling = on; }
    void Init(C2DFont *font, CRdrFile *file);

    C2DFont *m_font;   // 0x18 (SOCOM 1's at 0x1C: Init's font)
    C2DBitmapPoly m_background1;   // 0x20
    C2DBitmapPoly m_background2;   // 0x210
    float m_scale;   // 0x400
    unsigned int m_maxsize;   // 0x404
    int m_top;   // 0x408
    int m_left;   // 0x40c
    int m_red;   // 0x410
    int m_blue;   // 0x414
    int m_green;   // 0x418
    float m_maxtrans;   // 0x41c
    float m_transstep;   // 0x420
    int m_time;   // 0x424
    int m_lowestbottom;   // 0x428
    int m_spacing;   // 0x42c
    float m_maxlength;   // 0x430: Init's "maxlength"
    float m_TtySpeed;   // 0x434: Init's "TtySpeed" (100000 until then: typed at once)
    float m_438;   // 0x438
    float m_43c;   // 0x43c: 1 once Init has a background
    float m_440;   // 0x440: the background's margin above, negated
    int m_xmargin;   // 0x444: Init's "XMargin"
    int m_ymargin;   // 0x448: Init's "YMargin"
    unsigned char _44c[0x454 - 0x44c];
    int m_454;   // 0x454
    int m_458;   // 0x458
    unsigned char m_45c;   // 0x45c
    unsigned char _45d[0x4dc - 0x45d];
    unsigned char m_centered : 1;   // 0x4dc
    unsigned char m_descends : 1;
    unsigned char m_bEnabled : 1;
    unsigned char m_scrolling : 1;
    unsigned char m_subtitles : 1;   // Init's: the background named "SUBTITLES"
    unsigned char m_4dc_5 : 3;
};   // 0x4e0 bytes (the mission's queues' stride)

/* the lines' fade: its speed (alpha a second) and the alpha a line fades in to (OVL1 data
 * 0x3DE6D0, 0x3DE6D8: variables, read with lwc1; the unit's first statics, as C2DMessageString::Tick
 * is its last function; the names ours) */
static float s_fadeRate = 280.0f;
static float s_fadeMax = 100.0f;
/* the queue's own fade speed (OVL1 data 0x3DE6E0, 280.0: Tick and Add_Message read it, not written
 * yet) and the backgrounds' transparency, a texture's and the subtitles' (0x3DE6E8 76.8, 0x3DE6F0
 * 128.8: Init's; variables, read with lwc1; the names ours) */
static float s_queueFadeRate = 280.0f;
static float s_backgroundTrans = 76.8f;
static float s_subtitlesTrans = 128.8f;

/* the messages' six colours, red, green, blue and alpha each (OVL1 BSS 0x440C70; the mission's
 * Init and the queue's set them, the name ours): the unit's one dynamically initialised static,
 * so its initialiser (0x4003E0) is __construct_array over the six with CPnt4D's empty constructor
 * (0x2110E0) and no destructor */
static CPnt4D s_msgColors[6];

/* the queue set up: SOCOM 1's constructor, the colours -1 and a fourth flag cleared (OVL1
 * 0x2B7610) */
C2DMessage_Q::C2DMessage_Q()
{
    m_scale = 1.0f;
    m_maxsize = 3;
    m_top = 100;
    m_left = 20;
    m_maxtrans = 100.0f;
    m_transstep = 3.0f;
    m_time = 0;
    m_red = m_blue = m_green = -1;
    m_lowestbottom = m_top + 20;
    m_spacing = 20;
    m_centered = false;
    m_descends = false;
    m_TtySpeed = 100000.0f;
    m_43c = 0;
    m_438 = 0;
    m_440 = 0;
    m_bEnabled = true;
    m_subtitles = false;
}

/* the colours set: greys, yellows, blues, greens, oranges and reds at alpha 100 (OVL1 0x2B7530) */
extern "C" void func_002B7530()
{
    s_msgColors[0].x = 128.0f;
    s_msgColors[0].y = 128.0f;
    s_msgColors[0].z = 128.0f;
    s_msgColors[0].w = 100.0f;
    s_msgColors[1].x = 128.0f;
    s_msgColors[1].y = 128.0f;
    s_msgColors[1].z = 64.0f;
    s_msgColors[1].w = 100.0f;
    s_msgColors[2].x = 64.0f;
    s_msgColors[2].y = 64.0f;
    s_msgColors[2].z = 96.0f;
    s_msgColors[2].w = 100.0f;
    s_msgColors[3].x = 64.0f;
    s_msgColors[3].y = 128.0f;
    s_msgColors[3].z = 64.0f;
    s_msgColors[3].w = 100.0f;
    s_msgColors[4].x = 128.0f;
    s_msgColors[4].y = 64.0f;
    s_msgColors[4].z = 32.0f;
    s_msgColors[4].w = 100.0f;
    s_msgColors[5].x = 128.0f;
    s_msgColors[5].y = 64.0f;
    s_msgColors[5].z = 64.0f;
    s_msgColors[5].w = 100.0f;
}

/* the colours (OVL1 0x2B7520) */
extern "C" CPnt4D *func_002B7520()
{
    return s_msgColors;
}

/* the queue read from its rdr (SOCOM 1's Init(C2DFont *, CRdrFile *), the same reads in the same
 * order, grown; OVL1 0x2B6FC0): the colours set, the font kept, then the scale, the fade time, the
 * left and bottom, the size and spacing, centred and descending, the longest line; a background by
 * texture name with its margins -- the subtitles' its own grey box, any other texture's stretched
 * -- or none; the typing speed; the scroll cleared */
void C2DMessage_Q::Init(C2DFont *font, CRdrFile *file)
{
    func_002B7530();
    m_font = font;
    if (!file)
        return;
    _zrdr *rdr = (_zrdr *)file;

    zrdr_findreal(rdr, "scale", &m_scale, 1);
    zrdr_findreal(rdr, "fadetime", &m_transstep, 1);
    zrdr_findint(rdr, "left", &m_left, 1);
    zrdr_findint(rdr, "bottom", &m_top, 1);
    zrdr_findint(rdr, "maxsize", (int *)&m_maxsize, 1);
    zrdr_findint(rdr, "spacing", &m_spacing, 1);
    if (zrdr_findtag(rdr, "centered"))
        m_centered = true;
    if (zrdr_findtag(rdr, "descends"))
        m_descends = true;
    else
        m_descends = false;
    zrdr_findreal(rdr, "maxlength", &m_maxlength, 1);
    const char *name = zrdr_findstring(rdr, "background");

    if (name) {
        m_xmargin = m_ymargin = 0;
        zrdr_findint(rdr, "XMargin", &m_xmargin, 1);
        zrdr_findint(rdr, "YMargin", &m_ymargin, 1);
        zdb::CTexHandle *tex = zdb::CAssetMgr::m_assets.GetTexHandle(name);

        m_subtitles = !strcasecmp(name, "SUBTITLES");
        if (m_subtitles) {
            tex = zdb::CAssetMgr::m_assets.GetTexHandle("newweapnbkrnd.tif");
            m_background1.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex ? tex->m_texture : 0);
            m_background1.SetColor(128.0f, 128.0f, 128.0f);
            m_background1.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
            m_background1.SetTrans(s_subtitlesTrans);
            m_background1.On();
            m_background2.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex ? tex->m_texture : 0);
            m_background2.SetColor(128.0f, 128.0f, 128.0f);
            m_background2.SetTrans(s_subtitlesTrans);
            m_background2.SetUV(0.3f, 0.3f, 0.4f, 0.4f);
            m_background2.On();
        } else if (tex) {
            m_background1.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex ? tex->m_texture : 0);
            m_background1.SetUV(1.0f, 1.0f, 0.05f, 0.0f);
            m_background1.On();
            m_background1.SetTrans(s_backgroundTrans);
            m_background2.Load(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, tex ? tex->m_texture : 0);
            m_background2.SetUV(0.05f, 0.0f, 1.0f, 1.0f);
            m_background2.On();
            m_background2.SetTrans(s_backgroundTrans);
        } else {
            m_background1.Off();
            m_background2.Off();
        }
        m_440 = (float)-m_ymargin;
        m_43c = 1.0f;
    } else {
        m_background1.Off();
        m_background2.Off();
    }
    zrdr_findreal(rdr, "TtySpeed", &m_TtySpeed, 1);
    m_454 = 0;
    m_458 = 0;
    m_45c = 0;
    m_scrolling = false;
}

/* the line typed out: as many of the text's characters as the time allows, then faded in while
 * its time lasts and out after (OVL1 0x2B77A0, slot 0x38) */
void C2DMessageString::Tick(float dT)
{
    m_time += dT;
    float typed = m_time * m_ttySpeed;

    while (m_length < typed && strlen(m_string + m_firstCharPos) < strlen(m_text.c_str())) {
        const char *typed_so_far = m_string + m_firstCharPos;

        AddCharacter(m_text.c_str()[strlen(typed_so_far)]);
        UpdatePos();
    }
    float a = m_color[3];

    if (m_time > m_displayTime) {
        if (a != 0.0f) {
            a -= dT * s_fadeRate;
            if (a < 0.0f)
                a = 0.0f;
            SetTrans(a);
        }
    } else {
        if (a != s_fadeMax) {
            a += dT * s_fadeRate;
            if (a > s_fadeMax)
                a = s_fadeMax;
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
    m_font = 0;
    m_454 = 0;
    m_458 = 0;
    m_45c = 0;
    SetScrolling(false);
}

/* the newest line's text, "" with none (OVL1 0x2B6D60; SOCOM 1's LastMessage, between SetBkgColor
 * and Clear_Messages as here; OVL2's 0x5BB000 calls it). The target materialises the short-form
 * test as a bool (`andi 1; sltu; xori; andi 0xff`, then beqz) and joins the two forms with a `b`
 * over a nop before the return's own `b`, where one c_str() inlines to `andi; beqz` with the short
 * form in the delay slot. A NUMBERING DEVICE gives the target's (the MSL headers' round 31, by the
 * integrator): a dead first c_str() ahead of the return's -- the repeated inline predicate is
 * materialised once, stored, negated and narrowed, as the target has it (byte-exact under the old
 * include/string and under round 31's, where `if (last && last->m_text.size() >= 0)` matches too).
 * Tried before it: an inline GetText() on the line, the ternary, a result local, the null test
 * inverted, all over both strings */
const char *C2DMessage_Q::LastMessage()
{
    C2DMessageString *last = 0;
    iterator e = end();

    for (iterator i = begin(); i != e; ++i)
        last = (C2DMessageString *)*i;
    if (last) {
        const char *s = last->m_text.c_str();   /* a numbering device: dead */

        return last->m_text.c_str();
    }
    return "";
}
