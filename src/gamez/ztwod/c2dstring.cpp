/* c2dstring.cpp -- C2DString (OVL1 0x361B00-0x3641EF; its initialiser 0x4035C0 builds the string
 * tables, so the unit opens with SOCOM II's string-table wrappers 0x361B00-0x361BCB, defined at the
 * file's end, and four functions 0x361BD0-0x362B1F not written yet): SOCOM 1's functions in SOCOM 1's order
 * (SetFont, UpdatePos, NewLine, AddCharacter, the Loads, SetString, Format, MakePacket, the two
 * Draws, Tick, On, Off, ClearAndWipe, the destructor and constructor), with SOCOM II's own among
 * them: a character's delete before NewLine, SetPos and the scaled Load after the Loads, the
 * string-table lookup by id after SetString, and two static setters before the constructor. SOCOM
 * 1's symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor and destructor, ClearAndWipe, GetCharWidth (0x363C20), On, Off, Tick, both Draws,
 * Format, SetStringId, SetString, the three Loads, SetPos, AddCharacter, NewLine, DeleteCharacter,
 * UpdatePos, SetFont and FormatString (0x363F60, after the constructor). Not yet: MakePacket
 * (0x3635C0) and the four layout functions it calls (0x361BD0-0x362B1F: it builds a packet writer
 * and a layout cursor on its stack -- the cursor's constructor 0x362900 takes the string and the
 * colour -- and loops `while (*cursor.m_p) { 0x361BD0(&writer, &cursor); 0x3625E0(&cursor); }`,
 * 0x3625E0 the only caller of GetCharWidth). Defined in reverse of their image order (-inline
 * deferred). */

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

    int x = (int)m_charheight;
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
    m_charheight += x;
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
    m_charheight = 0.0f;
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
            m_charheight += m_shortLength;
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
    m_charheight += m_shortLength;
}

/* the font, and an opaque string takes the font's opacity */
void C2DString::SetFont(C2DFont *font)
{
    m_font = font;
    if (m_color[3] == 128.0f)
        SetTrans(128.0f * m_font->m_opacity);
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
