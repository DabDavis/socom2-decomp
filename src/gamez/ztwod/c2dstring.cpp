/* c2dstring.cpp -- C2DString (OVL1 0x361B00-0x3641EF; its initialiser 0x4035C0 builds the string
 * tables, so the unit opens with SOCOM II's string-table wrappers 0x361B00-0x361BCB, defined at the
 * file's end, and MakePacket's layout 0x361BD0-0x362B1F): SOCOM 1's functions in SOCOM 1's order
 * (SetFont, UpdatePos, NewLine, AddCharacter, the Loads, SetString, Format, MakePacket, the two
 * Draws, Tick, On, Off, ClearAndWipe, the destructor and constructor), with SOCOM II's own among
 * them: a character's delete before NewLine, SetPos and the scaled Load after the Loads, the
 * string-table lookup by id after SetString, and two static setters before the constructor. SOCOM
 * 1's symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor and destructor, ClearAndWipe, GetCharWidth (0x363C20), On, Off, Tick, both Draws,
 * Format, SetStringId, SetString, the three Loads, SetPos, AddCharacter, NewLine, DeleteCharacter,
 * UpdatePos, SetFont and FormatString (0x363F60, after the constructor); round 50: MakePacket
 * (0x3635C0) and the text cursor's constructor 0x362900 -- MakePacket builds a packet writer and a
 * layout cursor on its stack and loops `while (cursor.More()) { packet.Write(&cursor);
 * cursor.Advance(); }`. Written, OPEN: the cursor's SeekGlyph 0x362350 and Advance 0x3625E0 (one
 * shared FPR spot); round 66: the writer's Write 0x361BD0. Defined in reverse of their image order
 * (-inline deferred). */

#include <stdarg.h>
#include <string.h>
#include <string>

#include "ztwod.h"

/* SOCOM 1's CSaferStr (zutil.h), as far as the font's name needs it */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
};

/* the font (SOCOM II's C2DFont: c2dfont.h -- the widths, opacity and scale the string reads, the
 * glyphs its layout measures) */
#include "c2dfont.h"

/* the string's scale whole (ztwod.h declares it; here, as c2dfont.cpp sees no C2DString) */
inline float C2DString::GetFullScale()
{
    return m_scale * m_socom2_78 * m_font->GetScaleFactor();
}

/* the string's default colour (black, opaque: a 16-aligned block, copied with lq / sq) and the PRIM
 * word its glyphs draw with (0x11E: textured sprites by UV; a transparent string ORs alpha blending
 * in, for good) -- c2dstring.cpp's .data 0x3E0350 and 0x3E0360 (the names ours) */
struct C2DStringColor {
    int c[4];
} __attribute__((aligned(16)));
static C2DStringColor s_defcolor = {{0, 0, 0, 128}};
static unsigned int s_prim = 0x11E;

/* the cursor: the string, the pen's place, the next character, its glyph (a C2DGlyphRef: MakePacket's
 * cursor deletes the owned glyph at its end, the reference's destructor), the glyph's piece by index
 * and by address, whether the font set the colour, the bright flag ('#B' toggles it; the string's
 * +0x8C seeds it) and the colour it writes as ints (round 50: the glyph reference and the piece read
 * off 0x362350's GetGlyph(font, cursor + 0x10) and its piece walk) */
class C2DStringCursor {
public:
    C2DStringCursor(C2DString *str, int *color);
    /* the next character with a glyph: newlines and colour codes taken on the way (0x362350) */
    void SeekGlyph();
    /* whether characters are left (inline: MakePacket's loop tests through it -- the plain read
     * shares the hoisted address with the loop's first test, the target reads it sp-relative) */
    bool More() const { return *m_p != 0; }
    /* the next piece, else the next character's glyph (0x3625E0) */
    void Advance();
    /* the glyph's current piece taken, and the font's drop-shadow colour written for a glyph without
     * its own (inline: SeekGlyph and Advance end alike). The glyph through a local: m_piece is then
     * loaded after Get()'s ternary, the target's order (indexing Get()'s result evaluates the
     * subscript first). The alpha capped by a ternary whose arms are the scaled opacity, an
     * expression (CSE'd), and the alpha: the result takes the alpha's register, both arms convert
     * and their tails merge, the join an expression's (unpadded; `if (cap < alpha) alpha = cap;` pads
     * it, a ternary over a scaled `cap` local takes cap's register and keeps two arms). `cap = cap;`
     * keeps the opacity a named local -- numbered before the 128's temporary, which then takes $f0
     * (propagated, the load numbers after the 128 and the two swap) -- the round 75 fit */
    void SetPiece()
    {
        C2DGlyph *g = m_glyph.Get();

        m_pieceptr = &g->m_pieces[m_piece];
        if (m_str->m_font->m_3c && !m_glyph.Get()->m_1a) {
            m_colorSet = true;
            m_color[0] = (int)m_str->m_font->m_48.x;
            m_color[1] = (int)m_str->m_font->m_48.y;
            m_color[2] = (int)m_str->m_font->m_48.z;

            float alpha;
            float cap = m_str->m_font->m_54;

            alpha = m_str->m_color[3];
            cap = cap;
            m_color[3] = (int)((float)(int)(128.0f * cap) < alpha ? (float)(int)(128.0f * cap) : alpha);
        }
    }

    C2DString *m_str;   // 0x0
    float m_x;   // 0x4
    float m_y;   // 0x8
    const char *m_p;   // 0xc
    C2DGlyphRef m_glyph;   // 0x10
    int m_piece;   // 0x18
    C2DGlyphPiece *m_pieceptr;   // 0x1c
    bool m_colorSet;   // 0x20
    unsigned char m_bright;   // 0x21
    int *m_color;   // 0x24
};

/* the packet MakePacket writes the cursor's pieces into (0x2C bytes on its stack; the class and the
 * names ours): the relocator, the string, the depth (a quadword index), whether the string is
 * transparent (C2D's m_isTrans: its PRIM word gains alpha blending), the colour the cursor writes, the packet -- its start, its DMA tag,
 * a word, its end --, the pieces in it and the font texture it draws from (-1: none yet). Write
 * (0x361BD0) closes the packet when a piece needs another texture; the destructor closes the last */
class C2DStringPacket {
public:
    C2DStringPacket(zdb::CTextureRelocMgr *mgr, C2DString *str, int depth, bool trans, int *color)
        : m_mgr(mgr), m_str(str), m_depth(depth << 4), m_trans(trans), m_color(color),
          m_texture(-1)
    {
        Clear();
    }
    ~C2DStringPacket()
    {
        if (m_start)
            Flush();
    }
    /* the cursor's piece written (0x361BD0) */
    void Write(C2DStringCursor *cursor);
    /* the packet closed and handed to the relocator: the DMA tag (its quadwords, DIRECT), the GIF tag
     * (the pieces as loops of RGBAQ, UV and XYZ2 under the PRIM word), then emptied */
    void Flush()
    {
        int qwc = m_end - (m_tag + 1);

        ((unsigned int *)m_tag)[0] = qwc | 0x10000000;
        ((unsigned int *)m_tag)[2] = 0;
        ((unsigned int *)m_tag)[3] = qwc | 0x50000000;
        if (m_trans)
            s_prim |= 0x40;
        ((unsigned long *)m_tag)[2] = (unsigned long)m_count | 0x400000008000 |
            ((unsigned long)s_prim << 47) | 0x3000000000000000;
        ((unsigned long *)m_tag)[3] = 0x531;
        m_mgr->TakeDrawPacket(m_start, m_end - m_start);
        Clear();
    }
    /* no packet begun */
    void Clear()
    {
        m_start = 0;
        m_tag = 0;
        m_1c = 0;
        m_end = 0;
        m_count = 0;
    }

    zdb::CTextureRelocMgr *m_mgr;   // 0x0
    C2DString *m_str;   // 0x4
    int m_depth;   // 0x8
    bool m_trans;   // 0xc
    int *m_color;   // 0x10
    u_long128 *m_start;   // 0x14
    u_long128 *m_tag;   // 0x18
    int m_1c;   // 0x1c
    u_long128 *m_end;   // 0x20
    int m_count;   // 0x24
    int m_texture;   // 0x28
};

/* the debugging allocator (zsys): file and line passed as nothing in this build */
void *_malloc(unsigned int size, const char *file, int line);
void *_realloc(void *p, unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

/* the UTF-8 readers (util_stable.cpp) */
int UTF8CharLen(const char *&s);
unsigned short UTF8GetChar(const char *&s);

/* the video's depth for 2D (inside zVid: SOCOM II's) */
extern int data_004A44E8;

/* the language's change count (OVL1 bss 0x49E9B0: a string built in an older language looks its id
 * up again when drawn; IncLanguageVersion counts it -- the name ours) */
int g_languageVersion;

struct LOCENTRY;
class CLocTable;

/* the string tables (util_loc.cpp's class: a vector of the loaded tables and the entry found last;
 * its members util_loc.cpp's) -- the unit's global, built with room for twelve */
class CStringTables : public std::vector<CLocTable *> {
public:
    CStringTables() : m_last(0) { reserve(12); }
    /* an int here (util_loc.cpp's is unsigned short): TranslateString tests the id unextended */
    int GetId(const char *name, const char *table);
    const char *GetString(int id, const char *table);
    LOCENTRY *Find(int id, const char *table);
    /* a string's own scale and offsets (SOCOM II's: the entry looked up and nothing read from it --
     * 1 and 0 whatever the table holds; SetString and SetStringId inline all three, each through
     * its own lookup; the names ours) */
    float GetScale(const char *name)
    {
        int id = GetId(name, 0);

        if (id > 0) {
            Find(id, 0);
            return 1.0f;
        }
        return 1.0f;
    }
    int GetOffsetX(const char *name)
    {
        int id = GetId(name, 0);

        if (id > 0) {
            Find(id, 0);
            return 0;
        }
        return 0;
    }
    int GetOffsetY(const char *name)
    {
        int id = GetId(name, 0);

        if (id > 0) {
            Find(id, 0);
            return 0;
        }
        return 0;
    }

    LOCENTRY *m_last;   // 0xc: the entry found last
};

extern CStringTables g_stringTables;
const char *TranslateString(const char *name);
const char *GetString(int id);
int GetStringId(const char *name);

/* a string table entry by id into a buffer, each `$<n>s` (n 1-9, below the argument count) replaced
 * by the n'th argument (none: empty), stopping where the next one would not fit; an unknown id
 * gives the empty string (SOCOM II's, after C2DString's constructor in the image: the HUD's number
 * formatter 0x1FE120, the score popup, DNASErrorString and 0x30C090 call it; the name ours) */
enum FORMAT_STATE {
    FORMAT_TEXT,
    FORMAT_DOLLAR,
    FORMAT_ARG
};

void FormatString(char *out, int size, int id, const char **args, int nargs)
{
    const char *s = GetString(id);

    if (!s) {
        *out = 0;
        return;
    }

    int n = 0;
    FORMAT_STATE state = FORMAT_TEXT;
    int arg = 0;

    for (int i = 0; s[i]; i++) {
        if (n + 1 >= size)
            break;

        char c = s[i];

        out[n++] = c;

        bool done = false;

        switch (state) {
        case FORMAT_TEXT:
            if (c == '$')
                state = FORMAT_DOLLAR;
            break;
        case FORMAT_DOLLAR:
            if (c >= '1' && c <= '9' && c - '1' < nargs) {
                arg = c - '1';
                state = FORMAT_ARG;
            } else
                state = FORMAT_TEXT;
            break;
        case FORMAT_ARG:
            if (c == 's') {
                const char *a = args[arg];

                if (!a)
                    a = "";

                int len = n - 3 + strlen(a);

                if (len < size) {
                    strcpy(out + n - 3, a);
                    n = len;
                } else
                    done = true;
            }
            state = FORMAT_TEXT;
            break;
        }
        if (done)
            break;
    }
    out[n] = 0;
}

/* empty strings of 15, white and opaque at the window's corner, no font; blended */
C2DString::C2DString()
{
    m_string = (char *)_malloc(16, 0, 0);
    m_string[0] = 0;
    m_stringlen_max = 15;
    m_numChars = 0;
    m_socom2_28 = 0;
    m_Ustring = (char *)_malloc(16, 0, 0);
    m_Ustring[0] = 0;
    m_Ustringlen_max = 15;
    m_numUChars = 0;
    m_posx = 0.0f;
    m_posy = 0.0f;
    m_font = 0;
    m_scale = 1.0f;
    m_socom2_78 = 1.0f;
    m_socom2_7c = 0.0f;
    m_socom2_80 = 0.0f;
    m_Type = STRING_NORMAL;
    m_revnum = 0;
    m_color[0] = m_color[1] = m_color[2] = 255.0f;
    m_color[3] = 128.0f;
    m_isCentered = 0;
    m_socom2_6a = 0;
    m_firstCharPos = 0;
    m_elapsedTime = 0.0f;
    m_socom2_8c = 0;
    SetIsTrans(true);
}

/* both strings let go of */
C2DString::~C2DString()
{
    if (m_string)
        _free(m_string, 0, 0);
    m_string = 0;
    if (m_Ustring)
        _free(m_Ustring, 0, 0);
    m_Ustring = 0;
}

/* both strings let go of and their sizes zeroed */
void C2DString::ClearAndWipe()
{
    if (m_string)
        _free(m_string, 0, 0);
    m_string = 0;
    m_stringlen_max = 0;
    if (m_Ustring)
        _free(m_Ustring, 0, 0);
    m_Ustring = 0;
    m_Ustringlen_max = 0;
}

/* a character's advance in pixels (SOCOM II's, between Off and ClearAndWipe; the name ours): the
 * glyph's width less one at the string's two scales and the font's, rounded, and the font's padding
 * -- none without a glyph (the reference's Get() const: c2dfont.h) */
int C2DString::GetCharWidth(unsigned short c)
{
    C2DGlyphRef ref;

    m_font->GetGlyph(&ref, c);

    C2DGlyph *g = ref.Get();

    if (!g)
        return 0;

    float w = g->GetWidth() - 1;
    int px = (int)(w * (m_scale * m_socom2_78 * m_font->GetScaleFactor()) + 0.5f);

    return px + m_font->XPadding;
}

void C2DString::Off()
{
    C2D::Off();
}

void C2DString::On()
{
    C2D::On();
}

/* a pulsing string's alpha: half the time's step, wrapped at the fade time, along half a sine
 * between the minimum and the maximum */
void C2DString::Tick(float dT)
{
    if (m_Type == STRING_PULSE) {
        m_elapsedTime += dT / 2.0f;
        if (!(m_elapsedTime <= m_fadeTime))
            m_elapsedTime -= m_fadeTime;
        float s = sinf(3.14f * (m_elapsedTime / m_fadeTime));

        SetTrans(128.0f * (m_fadeMin + s * (m_fadeMax - m_fadeMin)));
    }
}

/* slot 1: as below, through a batch relocator of its own at the video's depth */
void C2DString::Draw(zdb::CCamera *cam)
{
    zdb::CBatchRelocator reloc;
    int depth = data_004A44E8;

    if (IsOn() && m_numChars && m_font) {
        if (m_stringrev != g_languageVersion)
            SetStringId(m_stringid);
        MakePacket(&reloc, depth);
        C2D::Draw(cam);
    }
}

/* on, with text and a font: looked up again if the string table changed, packeted, the children */
void C2DString::Draw(zdb::CTextureRelocMgr *mgr, zdb::CCamera *cam, int depth)
{
    if (!(IsOn() && m_numChars && m_font))
        return;
    if (m_stringrev != g_languageVersion)
        SetStringId(m_stringid);
    MakePacket(mgr, depth);
    C2D::Draw(cam);
}

/* slot 28 (SOCOM 1's Format): the text kept as given, then its escapes resolved into a local copy
 * that is set through the tables -- `$~` ends it, a `$` ending the text is kept, any other `$`
 * stays with the character after it; the arguments are not read (SOCOM 1's either) */
void C2DString::Format(const char *fmt, ...)
{
    char buf[10512];
    char *p = buf;
    bool done = false;
    int i = 0;
    va_list args;

    va_start(args, fmt);
    if (fmt) {
        m_numUChars = strlen(fmt);
        if (m_numUChars > m_Ustringlen_max) {
            m_Ustring = (char *)_realloc(m_Ustring, m_numUChars + 1, 0, 0);
            m_Ustringlen_max = m_numUChars;
        }
        strcpy(m_Ustring, fmt);
    }
    while (!done) {
        char c = fmt[i++];

        if (c == '$') {
            c = fmt[i++];
            switch (c) {
            case 0:
                *p++ = '$';
                *p++ = 0;
                done = true;
                break;
            case '~':
                *p++ = 0;
                done = true;
                break;
            default:
                *p++ = '$';
                i--;
                break;
            }
        } else {
            *p++ = c;
            done = c == 0;
        }
    }
    SetString(buf, true);
}


/* the string's packets made (SOCOM 1's MakePacket(CTextureRelocMgr *, int)): nothing for an empty
 * string; else the colour starts at the default, a packet writer and a layout cursor walk the
 * string -- each piece written, the cursor moved on -- the pen's last place kept as the short length,
 * and the last packet drawn as the writer goes (the cursor's glyph freed first) */
void C2DString::MakePacket(zdb::CTextureRelocMgr *mgr, int depth)
{
    if (!m_string || !*m_string)
        return;

    C2DStringColor color = s_defcolor;
    C2DStringPacket packet(mgr, this, depth, m_isTrans, color.c);
    C2DStringCursor cursor(this, color.c);

    while (cursor.More()) {
        packet.Write(&cursor);
        cursor.Advance();
    }
    m_shortLength = (int)cursor.m_x;
}

/* slot 27: the string of an id in the tables, its scale and offsets with it, kept with the
 * tables' revision; an id of none only takes the revision */
void C2DString::SetStringId(int id)
{
    if (id <= 0) {
        m_stringrev = g_languageVersion;
        return;
    }

    const char *s = GetString(id);

    m_stringid = id;
    SetString(s, false);
    m_socom2_78 = g_stringTables.GetScale(s);
    m_socom2_7c = g_stringTables.GetOffsetX(s);
    m_socom2_80 = g_stringTables.GetOffsetY(s);
}

/* slot 26 (SOCOM 1's SetString with SOCOM II's flag): the text, through the tables when asked --
 * the id kept, the scale and offsets taken -- copied in unless it is already the string; none
 * frees the string */
void C2DString::SetString(const char *s, bool translate)
{
    const char *text;

    if (translate)
        text = TranslateString(s);
    else
        text = s;
    if (translate)
        m_stringid = GetStringId(s);
    m_stringrev = g_languageVersion;
    if (text && m_string && !strcmp(text, m_string))
        return;
    if (!m_string && text) {
        m_string = (char *)_malloc(16, 0, 0);
        m_stringlen_max = 15;
    }
    if (text) {
        m_numChars = strlen(text);
        if (m_numChars > m_stringlen_max) {
            m_string = (char *)_realloc(m_string, m_numChars + 1, 0, 0);
            m_stringlen_max = m_numChars;
        }
        if (translate) {
            m_socom2_78 = g_stringTables.GetScale(s);
            m_socom2_7c = g_stringTables.GetOffsetX(s);
            m_socom2_80 = g_stringTables.GetOffsetY(s);
        }
        strcpy(m_string, text);
    } else {
        if (m_string)
            _free(m_string, 0, 0);
        m_string = 0;
        m_stringlen_max = 0;
        m_numChars = 0;
        m_socom2_28 = 0;
    }
}

/* the scaled Load: the scale, then the plain one at the position as ints */
void C2DString::Load(float scale, const char *s, C2DFont *font, float x, float y, bool translate)
{
    m_scale = scale;
    Load(s, font, (int)x, (int)y, translate);
}

void C2DString::SetPos(int x, int y)
{
    m_posx = x;
    m_posy = y;
}

/* placed, set, in the font; the characters as wide as the font's average at both scales and the
 * font's (the average cast to float, a no-op that makes MW load it first -- 1 instruction to a
 * match after 120 groupings and accessors: build/scratch/hud/v_ld*.py) */
void C2DString::Load(const char *s, C2DFont *font, int x, int y, bool translate)
{
    SetPos(x, y);
    SetString(s, translate);
    SetFont(font);
    m_charwidth = m_scale * (float)font->m_avgWidth * m_socom2_78 * font->GetScaleFactor();
}

/* placed, set, in the font, the characters w wide (h unused in SOCOM II) */
void C2DString::Load(const char *s, C2DFont *font, int x, int y, int w, int h, bool translate)
{
    SetPos(x, y);
    SetString(s, translate);
    SetFont(font);
    m_charwidth = w;
}

/* a character typed on (SOCOM 1's AddCharacter with SOCOM II's growth: 50 more once fewer than five
 * are spare, the room kept at 0x28): stored and terminated, then -- but for a new line -- its advance
 * added to the length: the glyph's width at the string's two scales and the font's, or its fixed
 * advance, then its kerning and the font's padding (added to the length as it was, as SOCOM 1's) */
void C2DString::AddCharacter(char c)
{
    if (m_socom2_28 < m_numChars + 5) {
        m_socom2_28 += 50;
        m_string = (char *)_realloc(m_string, m_socom2_28, 0, 0);
    }
    m_numChars++;
    if (m_numChars > 1) {
        m_string[m_numChars - 1] = c;
        m_string[m_numChars] = 0;
    } else {
        m_string[0] = c;
        m_string[1] = 0;
    }
    if (c == '\n')
        return;

    int x = (int)m_length;
    C2DGlyphRef ref;

    m_font->GetGlyph(&ref, c);
    if (!ref.Get())
        return;

    C2DGlyph *g = ref.Get();

    if (g->m_10 == -1)
        x = (int)(x + m_socom2_78 * (m_scale * g->GetWidth()) * m_font->GetScaleFactor());
    else
        x += g->m_10;
    x += (int)m_font->XPadding + g->m_16;
    m_length += x;
}

void C2DString::NewLine()
{
    AddCharacter('\n');
}

/* the last character dropped, while more than one is left */
void C2DString::DeleteCharacter()
{
    if (m_numChars > 1) {
        m_numChars--;
        m_string[m_numChars] = 0;
    }
}

/* the string measured again (SOCOM 1's UpdatePos, on SOCOM II's glyphs and UTF-8): the lines
 * counted, each line's width -- every character's glyph width less one at the string's two scales
 * and the font's, rounded, and the font's padding; a space opening a later line skipped -- added
 * to the length */
void C2DString::UpdatePos()
{
    m_shortLength = 0;
    m_length = 0.0f;
    m_numLines = 0;
    if (!m_string || !m_font)
        return;

    int i = 0;
    float scale = m_scale * m_socom2_78 * m_font->GetScaleFactor();

    while (i < m_numChars) {
        char c = m_string[i];

        if (c == '\n') {
            m_numLines++;
            i++;
            m_length += m_shortLength;
            m_shortLength = 0;
            continue;
        }
        if (m_shortLength == 0 && c == ' ' && m_numLines != 0) {
            i++;
            continue;
        }

        const char *p = m_string + i;
        unsigned short code = UTF8GetChar(p);

        i += UTF8CharLen(p);

        C2DGlyphRef ref;

        m_font->GetGlyph(&ref, code);
        if (!ref.Get())
            continue;
        m_shortLength += 0.5f + scale * (ref.Get()->GetWidth() - 1);
        m_shortLength += m_font->XPadding;
    }
    m_length += m_shortLength;
}

/* the font, and an opaque string takes the font's opacity */
void C2DString::SetFont(C2DFont *font)
{
    m_font = font;
    if (m_color[3] == 128.0f)
        SetTrans(128.0f * m_font->m_opacity);
}

/* ---- MakePacket's layout (SOCOM II's, 0x361BD0-0x362B1F: a cursor walking the string and the
 * packet writer it feeds; the names ours) ---- */


/* the colour's alpha scale for a bright string (.data 0x3E0338: a variable, read with lwc1; the
 * name ours) */
static float s_brightalpha = 1.5f;

/* the cursor at the string's place (its position and offset, whole pixels), its first shown
 * character, centred or right-aligned by the measured length; its line set up and, unless the
 * string is empty, the colour written (the alpha 1.5 times for a bright string) (0x362900; the
 * name ours -- MakePacket's; round 49). Both products read the string's member through an inline
 * accessor (round 50: open since round 49 on `0.5f * m_length`'s FPR pair -- the inline call is
 * expanded ahead of the expression, so the length is numbered before the 0.5 and the alpha before
 * the scale, the target's registers with the constant still the left operand; read plainly, either
 * operand order swapped a pair) */
C2DStringCursor::C2DStringCursor(C2DString *str, int *color)
    : m_str(str), m_x((float)(int)(str->m_posx + str->m_socom2_7c)),
      m_y((float)(int)(str->m_posy + str->m_socom2_80)), m_p(str->m_string + str->m_firstCharPos),
      m_piece(0), m_pieceptr(0), m_colorSet(false), m_bright(0), m_color(color)
{
    m_bright = str->m_socom2_8c;
    if (str->m_isCentered || str->m_socom2_6a) {
        str->UpdatePos();
        if (str->m_isCentered)
            m_x -= 0.5f * str->GetLength();
        if (str->m_socom2_6a)
            m_x -= str->m_length;
    }
    SeekGlyph();
    if (*m_p && !m_colorSet) {
        if (m_bright) {
            m_color[0] = (int)m_str->m_color[0];
            m_color[1] = (int)m_str->m_color[1];
            m_color[2] = (int)m_str->m_color[2];
            m_color[3] = (int)(s_brightalpha * m_str->GetTrans());
        } else {
            m_color[0] = (int)m_str->m_color[0];
            m_color[1] = (int)m_str->m_color[1];
            m_color[2] = (int)m_str->m_color[2];
            m_color[3] = (int)m_str->m_color[3];
        }
    }
}

/* the cursor moved on (0x3625E0; the name ours): after a drop-shadow piece the string's own colour
 * back (the bright alpha as the constructor's), else the glyph's next piece, else the next
 * character -- a '#B' after it toggles the bright flag and moves on again (one byte: the next call's
 * UTF8CharLen skips the 'B'), any other advances the pen by the passed character's width and seeks
 * the next glyph. An if / else chain: early returns each branch to a shared `b` (2 words).
 * Byte-exact since round 75: SetPiece's alpha as a ternary (its comment), and the '#B' arm's own
 * `return;` after the recursion -- the call falls into its join's `b` (without it the body ends in a
 * `b` of its own ahead of the join's) */
void C2DStringCursor::Advance()
{
    if (m_colorSet) {
        m_colorSet = false;
        if (m_bright) {
            m_color[0] = (int)m_str->m_color[0];
            m_color[1] = (int)m_str->m_color[1];
            m_color[2] = (int)m_str->m_color[2];
            m_color[3] = (int)(s_brightalpha * m_str->GetTrans());
        } else {
            m_color[0] = (int)m_str->m_color[0];
            m_color[1] = (int)m_str->m_color[1];
            m_color[2] = (int)m_str->m_color[2];
            m_color[3] = (int)m_str->m_color[3];
        }
    } else if (m_piece + 1 < m_glyph.Get()->m_pieces.size()) {
        m_piece++;
        SetPiece();
    } else {
        const char *p = m_p;

        m_p += UTF8CharLen(p);
        if (*m_p == '#' && m_p[1] == 'B') {
            m_p++;
            m_bright = !m_bright;
            if (*m_p)
                Advance();
            return;
        } else {
            m_x += m_str->GetCharWidth(UTF8GetChar(p));
            SeekGlyph();
        }
    }
}

/* the next character with a glyph (0x362350; the name ours): a newline puts the pen back at the
 * string's left and a line down (the spacing whole pixels) and skips one space after it, each '#B'
 * toggles the bright flag, a character the font has no glyph (or no pieces) for is skipped; at a
 * glyph its first piece is taken, and when the font colours its glyphs and this one has no colour of
 * its own the font's drop-shadow colour is written (C2DFont's m_3c, m_48 and m_54: the faces rdr's
 * "dropshadow" block), its alpha the string's capped at the shadow's opacity (whole).
 * Byte-exact since round 75 (0x298 against 0x290 since round 61): SetPiece's clamp read as code --
 * the target's join after `mov.s` is unpadded and its cvt.w.s precedes the store's address load, the
 * shape of a ternary whose two converting arms merged their tails, not of an if-statement (a statement
 * label pads; the earlier "scheduling" was the label). Its arms an expression and the alpha give the
 * alpha's register; `cap = cap;` keeps the opacity's numbering (SetPiece's comment). Before round 61:
 * the target colours the 128 first; MW numbers
 * the constant before the opacity's load whichever side it is written on (it canonicalises the
 * constant to the first operand), and the replay (tools/igreplay.py --want 53=f0,54=f1,46=f1 --movers
 * 54) wants the load anywhere below it. Tried: an accessor (C2DFont::GetShadowOpacity, folded away --
 * unlike the constructor's GetLength), static helpers taking the font or the opacity (folded), a named
 * opacity local (propagated, or computed at the top when declared first), the cap or the alpha first,
 * both declared uninitialised, a font local, MIN(), a ternary. Matched on the way: the newline's
 * spacing through an int local (the target's FPR pair), `*m_p == 0` in the colour-code loop (`!*m_p`
 * makes MW sign-extend the reloaded char again before the '#' test: 2 words), break (not return) out of
 * the walk at a glyph (2 words), a glyph local for the piece-count test (6 words), the character code
 * an unsigned short local (the andi), SetPiece's glyph local (the piece pointer's load order) */
void C2DStringCursor::SeekGlyph()
{
    while (*m_p) {
        if (*m_p == '\n') {
            m_x = (float)(int)(m_str->m_posx + m_str->m_socom2_7c);
            int spacing = (int)m_str->GetSpacing();

            m_y += spacing;
            m_p++;
            if (*m_p == ' ')
                m_p++;
            continue;
        }
        while (*m_p == '#' && m_p[1] == 'B') {
            m_p += 2;
            m_bright = !m_bright;
            if (*m_p == 0)
                return;
        }

        const char *p = m_p;
        unsigned short code = UTF8GetChar(p);

        m_str->m_font->GetGlyph(&m_glyph, code);
        C2DGlyph *g = m_glyph.Get();

        if (g && g->m_pieces.size()) {
            m_piece = 0;
            SetPiece();
            break;
        }
        m_p += UTF8CharLen(p);
    }
}

/* the texture as the packet needs it: a handle's texture, and its GS registers (c2dline.cpp's views) */
namespace zdb {
class CTexture {
public:
    unsigned char _0[0x30];
    u_long128 m_gsregs;   // 0x30
};
class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};
}

/* the scratchpad packet (zsys) */
u_long128 *zSysSprGetPacket(u_long128 **end);

/* the screen rows' sub-pixel offsets (OVL1 bss 0x49E9A0 and 0x49E9A8), the far U's (.data 0x3E0340, 15:
 * under a whole texel) -- the names ours */
static float s_yoffset0;
static float s_yoffset1;
static int s_u1offset = 15;

/* the cursor's piece written: the packet closed if it draws from another font texture; a packet begun
 * if none is open -- the new texture's GS registers first; the colour (a glyph's own, with the
 * string's alpha), the piece's two corners in UV and screen coordinates, the drop shadow's offset when
 * the cursor wrote its colour; the packet closed when it is full (0x361BD0) */
void C2DStringPacket::Write(C2DStringCursor *cursor)
{
    if (m_start && m_texture != cursor->m_glyph.Get()->m_18)
        Flush();
    if (!m_start) {
        zdb::CTexHandle *tex = 0;

        if (m_texture != cursor->m_glyph.Get()->m_18) {
            tex = m_str->m_font->GetTexture(cursor->m_glyph.Get()->m_18);
            if (!tex)
                return;
            m_mgr->PreGetSprPacket(tex);
        }
        m_start = zSysSprGetPacket((u_long128 **)&m_1c);
        m_end = m_start;
        if (m_texture != cursor->m_glyph.Get()->m_18)
            *m_end++ = tex->m_texture->m_gsregs;
        m_tag = m_end;
        m_end = m_tag + 2;
    }
    m_texture = cursor->m_glyph.Get()->m_18;

    float *top = &C2D::m_topx;
    float *pix = &C2D::m_fXPixel;
    int *from = cursor->m_glyph.Get()->m_1a ? (int *)&cursor->m_glyph.Get()->m_20 : m_color;
    int c[4] __attribute__((aligned(16)));

    c[0] = from[0];
    c[1] = from[1];
    c[2] = from[2];
    c[3] = m_color[3];
    m_end[0] = *(u_long128 *)c;
    ((int *)m_end)[4] = cursor->m_pieceptr->m_0 << 4;
    ((int *)m_end)[5] = cursor->m_pieceptr->m_6 << 4;

    float dx = cursor->m_pieceptr->m_place.x;

    if (cursor->m_colorSet)
        dx += (float)cursor->m_str->m_font->m_40;
    ((short *)m_end)[16] = (int)(top[0] + pix[0] * (cursor->m_x + dx * cursor->m_str->GetFullScale())) << 4;

    float yoff = s_yoffset0;
    C2DGlyph *g = cursor->m_glyph.Get();
    C2DGlyphPiece *piece = cursor->m_pieceptr;
    short h = piece->m_4 - piece->m_6;
    float dy = (float)(g->m_12 - piece->m_place.y) + ((float)-h + (float)h * piece->GetScaleY());

    if (cursor->m_colorSet)
        dy += (float)cursor->m_str->m_font->m_44;
    float sumy = 0.5f + (cursor->m_y + dy * cursor->m_str->GetFullScale());

    ((short *)m_end)[18] = (int)(top[1] + pix[1] * (sumy + yoff)) << 4;
    m_end[3] = *(u_long128 *)c;
    ((int *)m_end)[16] = s_u1offset + ((cursor->m_pieceptr->m_0 + cursor->m_pieceptr->m_2 - 1) << 4);
    ((int *)m_end)[17] = cursor->m_pieceptr->m_4 << 4;
    piece = cursor->m_pieceptr;

    float prod = (float)piece->m_2 * piece->m_place.sx;
    float dx2 = (prod + (float)piece->m_place.x) - 1.0f;

    if (cursor->m_colorSet)
        dx2 += (float)cursor->m_str->m_font->m_40;
    ((short *)m_end)[40] = (int)(top[0] + pix[0] * (cursor->m_x + (0.5f + dx2 * cursor->m_str->GetFullScale()))) << 4;

    yoff = s_yoffset1;
    g = cursor->m_glyph.Get();
    piece = cursor->m_pieceptr;
    short h2 = piece->m_4 - piece->m_6;
    int dy2 = (g->m_12 - piece->m_place.y) - h2;

    if (cursor->m_colorSet)
        dy2 += cursor->m_str->m_font->m_44;
    float sumy2 = cursor->m_y + (float)dy2 * cursor->m_str->GetFullScale();

    ((short *)m_end)[42] = (int)(top[1] + pix[1] * (sumy2 + yoff)) << 4;
    ((int *)m_end)[22] = m_depth;
    ((int *)m_end)[23] = 0;
    m_end += 6;
    m_count += 2;
    if (!(m_end < (u_long128 *)(m_1c - 96)))
        Flush();
}

/* ---- the string tables' global and its wrappers (SOCOM II's: the image's head of the unit,
 * 0x361B00-0x361BCB, so defined last; the initialiser 0x4035C0 builds the tables, registering their
 * implicit destructor 0x364170, the unit's last function) ---- */

/* OVL1 bss 0x49E9D0 (the name ours) */
CStringTables g_stringTables;

CStringTables *GetStringTables()
{
    return &g_stringTables;
}

/* a name's text when the tables know it, else the name itself */
const char *TranslateString(const char *name)
{
    int id = g_stringTables.GetId(name, 0);

    if (id > 0)
        return g_stringTables.GetString(id, 0);
    return name;
}

/* an id's text, in any table */
const char *GetString(int id)
{
    return g_stringTables.GetString(id, 0);
}

/* a name's id, in any table */
int GetStringId(const char *name)
{
    return g_stringTables.GetId(name, 0);
}

/* the language changed: strings built before it look themselves up again */
void IncLanguageVersion()
{
    g_languageVersion++;
}
