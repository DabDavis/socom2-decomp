/* c2dstring.cpp -- C2DString (OVL1 0x362B20-0x3641EF): SOCOM 1's functions in SOCOM 1's order
 * (SetFont, UpdatePos, NewLine, AddCharacter, the Loads, SetString, Format, MakePacket, the two
 * Draws, Tick, On, Off, ClearAndWipe, the destructor and constructor), with SOCOM II's own among
 * them: a character's delete before NewLine, SetPos and the scaled Load after the Loads, the
 * string-table lookup by id after SetString, and two static setters before the constructor. SOCOM
 * 1's symbol table gives the class its own unit; the name is ours. Written, all byte-exact: the
 * constructor and destructor, ClearAndWipe, On, Off, Tick, both Draws, the scaled Load and the
 * sized one, SetPos, NewLine, DeleteCharacter and SetFont; Load(text, font, x, y) OPEN. Not yet:
 * UpdatePos (0x362B70), AddCharacter, SetString, SetStringId, MakePacket, Format, 0x363C20 and the
 * functions after the constructor. Defined in reverse of their image order (-inline deferred). */

#include "ztwod.h"

/* the font as the string reads it (SOCOM II's C2DFont: SOCOM 1's widths and opacity moved on) */
class C2DFont {
public:
    float GetScaleFactor() { return m_scale_factor; }

    unsigned char _0[0x2c];
    float m_avgWidth;   // 0x2c
    float m_opacity;   // 0x30
    float m_scale_factor;   // 0x34
};

/* the debugging allocator (zsys): file and line passed as nothing in this build */
void *_malloc(unsigned int size, const char *file, int line);
void _free(void *p, const char *file, int line);

/* the string table's revision (OVL1 bss, ztwod's; no row yet) and the video's depth for 2D
 * (inside zVid: SOCOM II's) */
extern int data_0049E9B0;
extern int data_004A44E8;

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
    m_socom2_7c = 0;
    m_socom2_80 = 0;
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
        if (m_stringrev != data_0049E9B0)
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
    if (m_stringrev != data_0049E9B0)
        SetStringId(m_stringid);
    MakePacket(mgr, depth);
    C2D::Draw(cam);
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

/* placed, set, in the font; the characters as wide as the font's average at both scales.
 * OPEN, 1 instruction: the first product's operand order. The target loads the font's average
 * width first (f3), then m_scale (f2), and multiplies m_scale by it (mul f2,f2,f3); MW keeps loads
 * and operands in one order (avg * scale here: mul f2,f3,f2). Measured (build/scratch/hud/v_ld*.py,
 * all 120 groupings x an inline accessor on each operand, helper functions, locals): the font's
 * factor through an inline accessor puts it last as the right operand, as the target has it (4
 * instructions to 1); nothing yet loads the width first and multiplies it second */
void C2DString::Load(const char *s, C2DFont *font, int x, int y, bool translate)
{
    SetPos(x, y);
    SetString(s, translate);
    SetFont(font);
    m_charwidth = font->m_avgWidth * m_scale * m_socom2_78 * font->GetScaleFactor();
}

/* placed, set, in the font, the characters w wide (h unused in SOCOM II) */
void C2DString::Load(const char *s, C2DFont *font, int x, int y, int w, int h, bool translate)
{
    SetPos(x, y);
    SetString(s, translate);
    SetFont(font);
    m_charwidth = w;
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

/* the font, and an opaque string takes the font's opacity */
void C2DString::SetFont(C2DFont *font)
{
    m_font = font;
    if (m_color[3] == 128.0f)
        SetTrans(128.0f * m_font->m_opacity);
}
