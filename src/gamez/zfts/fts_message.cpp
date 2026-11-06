/* fts_message.cpp -- the 2D message queues: C2DMessage_Q, a C2D whose children are the lines of
 * text the mission puts up (the network's notices, the chat), each a C2DMessageString typed out a
 * character at a time and faded in and out. SOCOM 1's code, the unit after fts_action.cpp's
 * CNodeAction as in SOCOM 1's symbol table (which has no DWARF for it, so the file's name is ours):
 * OVL1 0x2B5830-0x2B794F. Written so far: Clear_Messages, C2DMessageString's SetString and Tick, the
 * colour table's setter and accessor (0x2B7530, 0x2B7520), the constructor (0x2B7610), Init
 * (0x2B6FC0, the states' round 41), LastMessage (open); the unit's vtable copies of C2D's inline
 * virtuals and the implicit ~C2DMessageString (0x2B63B0) come out byte-exact with them; Tick
 * (0x2B5DB0, written in the states' round 48, byte-exact since round 49) and Draw (0x2B5830, round
 * 49). The rest -- Add_Message (0x2B6530, its statics 0x3DE6F8-0x3DE708 before Draw's), 0x2B6430
 * -- still to write. Defined in reverse of their image order (-inline deferred). */

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

/* the font (Draw reads its scale): c2dfont.h wants the CSaferStr above ahead of it */
#include "../ztwod/c2dfont.h"

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
/* the lines' placing and the subtitles' background (OVL1 data 0x3DE710 4.0, 0x3DE718 -20.0, 0x3DE720
 * 27.0: Draw's, read with lwc1 -- variables; the names ours) -- after Add_Message's three
 * (0x3DE6F8-0x3DE708, not written yet) */
static float s_lineBase = 4.0f;
static float s_subtitlesTop = -20.0f;
static float s_subtitlesPad = 27.0f;

/* a message's line: SOCOM 1's C2DMessageString (a C2DString), SOCOM II's members after the
 * string's 0x98 bytes -- how long it stays up, how long it has been up, how many characters a
 * second it types, and the whole text it is typing (the names ours) */
class C2DMessageString : public C2DString {
public:
    virtual void Tick(float dT);
    virtual void SetString(const char *s, bool translate);
    /* its time over, and faded out (the queue's Tick's tests; the names ours) */
    bool TimeUp() const { return m_time > m_displayTime; }
    bool Faded() const { return m_color[3] == 0.0f; }
    bool Expired() const { return TimeUp() && Faded(); }

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
    virtual void Draw(zdb::CCamera *cam);
    void Clear_Messages();
    const char *LastMessage();
    void SetScrolling(bool on) { m_scrolling = on; }
    virtual void Tick(float dT);

    /* a scroll begun: a descending queue's from its margin above, the others' backgrounds faded in
     * (Tick's, three times; the name ours) */
    void Scroll(float dT)
    {
        if (!m_descends) {
            float trans = m_background2.GetTrans();
            float max = m_subtitles ? s_subtitlesTrans : s_backgroundTrans;

            if (trans != max) {
                float t = trans + dT * s_queueFadeRate;

                if (t > max)
                    t = max;
                m_background1.SetTrans(t);
                m_background2.SetTrans(t);
            }
        } else {
            m_440 = -m_ymargin;
            m_450 = m_440 - m_438;
            m_43c = 0.0f;
        }
    }
    /* the queue empty: the backgrounds faded out, a descending queue's scroll put back at its top
     * (Tick's; the name ours) */
    void Unscroll(float dT)
    {
        if (!m_descends) {
            float trans = m_background2.GetTrans();

            if (trans != 0.0f) {
                float t = trans - dT * s_queueFadeRate;

                if (t < 0.0f)
                    t = 0.0f;
                m_background1.SetTrans(t);
                m_background2.SetTrans(t);
            }
        } else {
            m_440 = m_top;
            m_43c = 0.0f;
            m_44c = 0.0f;
            m_450 = m_440 - m_438;
        }
    }
    /* a line erased, the iterator after it given back (Tick's two erases; the name ours): an inline
     * level between Tick and the list's erase puts the erase's result and argument temporaries in a
     * deeper frame group than Tick's own begin() temporaries and the result's conversion, both
     * erases at one depth -- in source order, the drop's above the walk's -- as the target has them */
    iterator EraseLine(iterator i) { return erase(i); }
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
    int m_438;   // 0x438: the lines' bottom, scrolling to m_top
    float m_43c;   // 0x43c: the scroll's time (1 once Init has a background)
    float m_440;   // 0x440: the scroll's start (the background's margin above, negated)
    int m_xmargin;   // 0x444: Init's "XMargin"
    int m_ymargin;   // 0x448: Init's "YMargin"
    float m_44c;   // 0x44c: the time since the last scroll
    float m_450;   // 0x450: the scroll's distance
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


/* the messages' six colours, red, green, blue and alpha each (OVL1 BSS 0x440C70; the mission's
 * Init and the queue's set them, the name ours): the unit's one dynamically initialised static,
 * so its initialiser (0x4003E0) is __construct_array over the six with CPnt4D's empty constructor
 * (0x2110E0) and no destructor */
static CPnt4D s_msgColors[6];
/* the subtitles background's inner corners' transparency (OVL1 bss 0x440CD0, after the colours: only
 * Draw reads it, nothing writes it -- zero; the name ours) */
static float s_subtitlesCornerTrans;

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

/* a frame of the queue: the lines ticked; the scroll -- an ascending queue held at its top, its
 * backgrounds faded in while it has lines and out without; a descending one starting a scroll
 * once a line's fade time has passed at the top, else moving toward the scroll's end --; the
 * oldest line dropped past the queue's size, then every line whose time is up and faded out, a
 * scroll begun when the last goes (OVL1 0x2B5DB0, slot 0x38; SOCOM 1's, with SOCOM II's
 * backgrounds). Byte-exact since the game states' round 49 (written in round 48, 38 off at the
 * size): THE SECOND WALK'S ITERATOR IS THE FIRST WALK'S `i` AGAIN -- its second web is a new object
 * made after inlining, so it takes the frame slot below every temporary (88), where a named `cur`
 * of its own sat just under `next` wherever it was declared (SOCOM 1's Tick, built by the older
 * compiler that lays the frame out upwards, has it at the top, 124: allocated last there too); both
 * erases through one inline level (EraseLine), which moves their temporaries below Tick's own
 * begin() temporaries and the result's conversion (the engine core's round-22 frame groups); and
 * the second walk's end a named local initialised before it, which hoists it into s0 and leaves the
 * line s1 (recomputed each pass without one; `e` reused is 4 bytes short, re-assigned or declared
 * at the top uninitialised it stays in memory). Round 48's: the first walk's iterator assigned from
 * begin() (kept in memory) against a hoisted end, the fades through two variables (the trans a
 * temporary, the new value sharing dT's $f20), Scroll / Unscroll inlined three times and once, the
 * expiry one inline `&&` (materialised, as the target) */
void C2DMessage_Q::Tick(float dT)
{
    if (!IsOn())
        return;
    iterator i, next, e = end();

    for (i = begin(); i != e; ++i)
        (*i)->Tick(dT);
    m_43c += dT;
    m_44c += dT;
    if (!m_descends) {
        m_438 = m_top;
        if (size())
            Scroll(dT);
        else
            Unscroll(dT);
    } else if (m_44c > m_transstep && m_438 == m_top) {
        Scroll(0.0f);
        m_44c = 0.0f;
    } else if (m_43c > 1.0f) {
        m_438 = (int)m_440;
    } else {
        m_438 = (int)(m_440 - m_450 * (1.0f - m_43c));
    }
    if (size() > m_maxsize) {
        C2D *m = *begin();

        EraseLine(begin());
        delete m;
    }
    iterator e2 = end();

    next = begin();
    for (i = next; i != e2; i = next) {
        C2DMessageString *s = dynamic_cast<C2DMessageString *>(*i);

        ++next;
        if (s && s->Expired()) {
            next = EraseLine(i);
            delete s;
            if (!size())
                Scroll(0.0f);
        }
    }
}

/* the queue drawn: the lines placed from the newest up -- each under the one before by the queue's
 * spacing, its height grown by how much smaller its font draws than the queue's, centred, or at the
 * left --, faded to their own transparency; the backgrounds put behind them -- an ascending queue's
 * two halves at the margins over the queue's whole height, the subtitles' around the lines there are
 * (their inner corners clear), a descending queue's to the left of centre --, drawn, and the lines
 * after them (OVL1 0x2B5830, slot 0xC; SOCOM 1's Draw, its backgrounds C2DBitmaps drawn first, its
 * lines walked from the oldest). Byte-exact as first written but for five spellings (the game states'
 * round 49): the reverse walk's ends built from end() and begin() -- include/list's rbegin() and rend()
 * reach the sentinel through tail(), whose end pair's second() a level down is a call (0x5AC); the
 * height declared before the first line's place (their saved float registers swap); the line's
 * length through its GetLength() accessor (written as the field, its load and the 2.0 trade $f1 and
 * $f2); the ascending queue's top `m_spacing * (int)m_maxsize` -- signed, so its product is not the
 * height's unsigned one and the target computes it again for each background (cast outside, MW
 * shares the product: 0x588); the subtitles' empty case first (`count <= 0`, the target's block
 * order) */
void C2DMessage_Q::Draw(zdb::CCamera *cam)
{
    int count = 0;

    if (!IsOn())
        return;
    float height = 0.0f;
    float first = -99.0f;
    float y = first;
    reverse_iterator r;

    for (r = reverse_iterator(end()); r != reverse_iterator(begin()); ++r) {
        C2DString *line = (C2DString *)*r;
        float scale = line->m_font ? line->m_font->m_scale_factor : 1.0f;
        float d = m_font->m_scale_factor - line->m_scale * scale;

        height += m_spacing * d + count * d;
        y = m_438 + s_lineBase - count * m_spacing - height;
        if (first == -99.0f)
            first = y;
        if (!m_centered && !line->m_isCentered)
            line->SetPos(m_left, (int)y);
        else if (!line->m_isCentered)
            line->SetPos((int)(m_left - line->GetLength() / 2.0f), (int)y);
        else
            line->SetPos((int)(m_left + m_maxlength / 2.0f), (int)y);
        line->SetTrans(line->GetTrans());
        count++;
    }
    if (!m_centered) {
        float w = m_left + m_maxlength / 2.0f - (m_left - m_xmargin);
        float h = m_438 + m_ymargin - (m_438 - m_spacing * m_maxsize - m_ymargin);

        m_background1.SetPos(m_left - m_xmargin, m_438 - m_spacing * (int)m_maxsize - m_ymargin, w, h);
        m_background2.SetPos(m_left + m_maxlength / 2.0f, m_438 - m_spacing * (int)m_maxsize - m_ymargin, w, h);
    } else if (m_subtitles) {
        m_background1.SetTrans(0, s_subtitlesCornerTrans);
        m_background1.SetTrans(1, s_subtitlesCornerTrans);
        m_background2.SetTrans(3, s_subtitlesCornerTrans);
        m_background2.SetTrans(2, s_subtitlesCornerTrans);
        float x = m_left - m_maxlength / 2.0f - m_xmargin;
        float w = m_left - x;
        float top;
        float h;

        if (count <= 0) {
            h = 0.0f;
            top = h;
        } else {
            h = s_subtitlesPad + (first - y);
            top = y + s_subtitlesTop;
        }
        m_background1.SetPos(x, top, w, h);
        m_background2.SetPos(m_left, top, w, h);
    } else {
        float x = m_left - m_maxlength / 2.0f - m_xmargin;
        float w = m_left - x;
        float h = m_438 + m_ymargin - (m_438 - m_spacing * m_maxsize - m_ymargin);

        m_background1.SetPos(x, m_438 - m_spacing * m_maxsize - m_ymargin, w, h);
        m_background2.SetPos(m_left, m_438 - m_spacing * m_maxsize - m_ymargin, w, h);
    }
    m_background1.Draw(cam);
    m_background2.Draw(cam);
    C2D::Draw(cam);
}
