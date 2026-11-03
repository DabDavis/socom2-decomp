/* seal_ailines.cpp -- the AI's debug lines: DrawLine, LineTo, MoveTo, the line colour stack (SOCOM 1's
 * unit, __sinit_seal_ailines.cpp, after seal_squirm.cpp). OVL2 from ~0x5FF2F0 (after CAiTrigger's
 * SetTimes, seal_squirm.cpp's last) to before seal_look.cpp; ai::MoveTo (0x600070) is SOCOM 1's,
 * byte-exact as a draft. So far the line-type test, the opacity and the colour (0x6000A0-0x600287).
 * Defined in reverse of their image order (-inline deferred). */

#include "../zmath/zmath.h"

namespace ai {

/* a line's kind, a bit each (SOCOM 1's ai::LINE_TYPE; the values SOCOM II's callers pass -- 512 for
 * the follow state's -- are masks, used unextended: a word) */
enum LINE_TYPE { LINE_ANY = 0, LINE_TYPE_LAST = 0x7fffffff };

/* the kinds of line drawn (SOCOM 1's ai_line_mask; OVL2 bss 0x66BB10) */
extern unsigned int ai_line_mask;

/* the lines' opacity and the range a fraction blends it over (SOCOM 1's statics of the unit; OVL2
 * data 0x650D48-0x650D58, 8 bytes apart), and the colour with the one it replaced (SOCOM 1's
 * ai_line_color and its ai_line_stack of colours; SOCOM II keeps one colour back: OVL2 bss
 * 0x66BAE0, 0x66BAEC -- the second name ours) */
static float ai_line_opacity = 1.0f;
static float ai_line_opacity_start = 1.0f;
static float ai_line_opacity_end = 1.0f;
static CPnt3D ai_line_color;
static CPnt3D ai_line_color_prev;

/* a colour put on as 0xRRGGBB, the current one kept back (OVL2 0x6001F0, new in SOCOM II: the
 * overload ours) -- each byte converted as an int (plain cvt.s.w: unsigned, MW tests the sign) */
bool PushLineColor(unsigned int rgb)
{
    ai_line_color_prev = ai_line_color;
    ai_line_color.x = (1.0f / 255.0f) * (int)((rgb >> 16) & 0xff);
    ai_line_color.y = (1.0f / 255.0f) * (int)((rgb >> 8) & 0xff);
    ai_line_color.z = (1.0f / 255.0f) * (int)(rgb & 0xff);
    return true;
}

/* a colour put on, the current one kept back (OVL2 0x600190; SOCOM 1's pushed it on a list) */
bool PushLineColor(const CPnt3D &color)
{
    ai_line_color_prev = ai_line_color;
    ai_line_color = color;
    return true;
}

/* the colour kept back put on again (OVL2 0x600150; SOCOM 1's erased the list's front) */
void PopLineColor()
{
    ai_line_color = ai_line_color_prev;
}

/* the opacity set, held within [0, 1] (OVL2 0x600120; SOCOM 1's) */
void SetLineOpacity(float opacity)
{
    ai_line_opacity = opacity;
    _saturate(&ai_line_opacity, 0.0f, 1.0f);
}

/* (OVL2 0x600110; SOCOM 1's) */
float GetLineOpacity()
{
    return ai_line_opacity;
}

/* the range a fraction blends the opacity over (OVL2 0x6000F0; SOCOM 1's) */
void SetLineFuncOpacity(float start, float end)
{
    ai_line_opacity_start = start;
    ai_line_opacity_end = end;
}

/* the opacity at a fraction of that range, held within [0, 1] (OVL2 0x6000A0; SOCOM 1's) */
void SetLineFuncOpacity(float t)
{
    ai_line_opacity = ai_line_opacity_start * (1.0f - t) + ai_line_opacity_end * t;
    _saturate(&ai_line_opacity, 0.0f, 1.0f);
}

/* whether a kind of line is drawn: any line when given none (OVL2 0x5FFF10, new in SOCOM II --
 * SOCOM 1's DrawLine tested it inline, as SOCOM II's still does; CAiSFollow::Tick calls this copy;
 * the name ours) */
bool LineOn(LINE_TYPE type)
{
    return type == LINE_ANY || (ai_line_mask & type) != 0;
}

}   // namespace ai
