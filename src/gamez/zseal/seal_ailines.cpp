/* seal_ailines.cpp -- the AI's debug lines: DrawLine, LineTo, MoveTo, the line colour stack (SOCOM 1's
 * unit, __sinit_seal_ailines.cpp, after seal_squirm.cpp). OVL2 from ~0x5FF2F0 (after CAiTrigger's
 * SetTimes, seal_squirm.cpp's last) to before seal_look.cpp. So far DrawLine, LineTo, MoveTo, the
 * line-type test, the opacity and the colour (0x5FFDB0-0x600287) and, round 29, the octahedron of
 * twelve lines 0x5FF2F0 the unit opens with. Defined in reverse of their image order (-inline deferred).
 *
 * THE UNIT IS COMPILED WITHOUT INLINING (`#pragma dont_inline on`, its state at the unit's end: in
 * deferred mode MW decides every inline by it -- set over DrawLine and LineTo alone and reset, or
 * flushed with defer_codegen, it inlines all as before): DrawLine, LineTo and 0x5FF2F0 call CPnt4D's
 * four-store constructor, the colour accessor and the stack's top out of line (their copies
 * 0x5FFDB0-0x5FFDEF follow 0x5FF2F0, their first caller), where SOCOM 1's inlined them; so every
 * CPnt3D copy here is written field by field (the implicit assignment would be called, 0x20F1F0).
 * DrawLine's kind test is SOCOM 1's, in place, a bool preset true and cleared (`a || b` in the `if`
 * is 0x10 shorter). OPEN, the initialiser (OVL2 0x668FB0, unnamed in the map): the target's inlines
 * CStack()'s push of the unit matrix and builds the colour (1, 0, 0) into 0x66BAEC, copied to
 * 0x66BAE0 -- here CStack() is called (the pragma) and the colours are not built. Round 42: every
 * whole-unit setting inlines alike in the functions and the initialiser (dont_inline and
 * inline_depth(0) keep the functions and call CStack(); inline_max_size(0), auto_inline off or none
 * lose DrawOctahedron, DrawLine and LineTo), but THE UNIT NON-DEFERRED -- `#pragma defer_codegen off`
 * after the includes, dont_inline on over the functions as now and off after the namespace -- keeps
 * every function byte-exact (17 of 18 with <iostream> included) while the initialiser inlines
 * CStack() and the colour's (1, 0, 0) constructor: the target's whole but for the copy into 0x66BAE0,
 * which the target makes member by member through $f0 from a pointer to 0x66BAEC (an implicit
 * operator= of a class holding the colour, by its shape) -- a copy construction is ld/sd + lwc1, the
 * implicit CPnt3D assignment (in an initialiser object's constructor) batches $f2/$f1/$f0 from
 * absolute loads, the three-float constructor from the fields likewise, a pointer cast 0x10C, the
 * colour as a member of a struct 0x104 (build/scratch/kit/v_ail1-8.py). Non-deferred, the image's
 * order wants the functions in source order (today's reverse) */

#include "../zmath/zmath.h"

/* a line drawn in the world, its two ends and their colours by value (OVL1 0x33B5F0, zrndr_pipe.cpp's) */
void RenderLineWorld(CPnt4D p0, CPnt4D p1, CPnt4D c0, CPnt4D c1);

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
/* where the next line starts (SOCOM 1's ai_line_pos; OVL2 bss 0x66BAC8) */
static CPnt3D ai_line_pos;

/* the colour as the lines read it (OVL2 0x5FFDC0, an inline's copy: a0 handed back) */
inline const CPnt3D &LineColor(const CPnt3D &c)
{
    return c;
}

/* the lines' matrix stack (OVL2 bss 0x66BB08: the unit's initialiser pushes the unit matrix and
 * registers ~CStack) and its top (OVL2 0x5FFDB0, an inline's copy: the stack handed in a0 and
 * dropped, CStack::m_top read -- likely a CStack member in the original; a free stand-in here) */
static CStack ai_line_mstack;

inline CMatrix *StackTop(CStack &s)
{
    return CStack::m_top;
}

#pragma dont_inline on

/* a colour put on as 0xRRGGBB, the current one kept back (OVL2 0x6001F0, new in SOCOM II: the
 * overload ours) -- each byte converted as an int (plain cvt.s.w: unsigned, MW tests the sign) */
bool PushLineColor(unsigned int rgb)
{
    ai_line_color_prev.x = ai_line_color.x;
    ai_line_color_prev.y = ai_line_color.y;
    ai_line_color_prev.z = ai_line_color.z;
    ai_line_color.x = (1.0f / 255.0f) * (int)((rgb >> 16) & 0xff);
    ai_line_color.y = (1.0f / 255.0f) * (int)((rgb >> 8) & 0xff);
    ai_line_color.z = (1.0f / 255.0f) * (int)(rgb & 0xff);
    return true;
}

/* a colour put on, the current one kept back (OVL2 0x600190; SOCOM 1's pushed it on a list) */
bool PushLineColor(const CPnt3D &color)
{
    ai_line_color_prev.x = ai_line_color.x;
    ai_line_color_prev.y = ai_line_color.y;
    ai_line_color_prev.z = ai_line_color.z;
    ai_line_color.x = color.x;
    ai_line_color.y = color.y;
    ai_line_color.z = color.z;
    return true;
}

/* the colour kept back put on again (OVL2 0x600150; SOCOM 1's erased the list's front) */
void PopLineColor()
{
    ai_line_color.x = ai_line_color_prev.x;
    ai_line_color.y = ai_line_color_prev.y;
    ai_line_color.z = ai_line_color_prev.z;
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

/* where the next line starts (OVL2 0x600070; SOCOM 1's) */
void MoveTo(const CPnt3D &p)
{
    ai_line_pos.x = p.x;
    ai_line_pos.y = p.y;
    ai_line_pos.z = p.z;
}


/* a line from where the last one ended to a point, which the next one starts from (OVL2 0x5FFF40;
 * SOCOM 1's LineTo, with no kind to test) */
void LineTo(const CPnt3D &p)
{
    CPnt4D pa(ai_line_pos.x, ai_line_pos.y, ai_line_pos.z, 1.0f);
    CPnt4D pb(p.x, p.y, p.z, 1.0f);
    const CPnt3D &c = LineColor(ai_line_color);
    CPnt4D ca(c.x, c.y, c.z, ai_line_opacity);
    CPnt4D cb(c.x, c.y, c.z, ai_line_opacity);
    CMatrix *m = StackTop(ai_line_mstack);

    m->Transform(&pa, &pa, 1);
    m->Transform(&pb, &pb, 1);
    RenderLineWorld(pa, pb, ca, cb);
    ai_line_pos.x = p.x;
    ai_line_pos.y = p.y;
    ai_line_pos.z = p.z;
}

/* whether a kind of line is drawn: any line when given none (OVL2 0x5FFF10, new in SOCOM II --
 * SOCOM 1's DrawLine tested it inline, as SOCOM II's still does; CAiSFollow::Tick calls this copy;
 * the name ours) */
bool LineOn(LINE_TYPE type)
{
    return type == LINE_ANY || (ai_line_mask & type) != 0;
}

/* a line from a point to another in the current colour and opacity, both ends through the line
 * stack's top matrix (OVL2 0x5FFDF0; SOCOM 1's DrawLine: the kind tested in place, as there) */
void DrawLine(LINE_TYPE type, const CPnt3D *a, const CPnt3D *b)
{
    bool on = true;

    if (type != LINE_ANY && (ai_line_mask & type) == 0)
        on = false;
    if (on) {
        CPnt4D pa(a->x, a->y, a->z, 1.0f);
        CPnt4D pb(b->x, b->y, b->z, 1.0f);
        const CPnt3D &c = LineColor(ai_line_color);
        CPnt4D ca(c.x, c.y, c.z, ai_line_opacity);
        CPnt4D cb(c.x, c.y, c.z, ai_line_opacity);
        CMatrix *m = StackTop(ai_line_mstack);

        m->Transform(&pa, &pa, 1);
        m->Transform(&pb, &pb, 1);
        RenderLineWorld(pa, pb, ca, cb);
    }
}




/* one line of DrawOctahedron: DrawLine's body written out, its kind already tested (the unit's pragma
 * keeps DrawLine a call, so the body is spelled in place -- a macro here) */
#define AI_OCTAHEDRON_LINE(ax, ay, az, bx, by, bz) \
    { \
        CPnt4D pa(ax, ay, az, 1.0f); \
        CPnt4D pb(bx, by, bz, 1.0f); \
        const CPnt3D &c = LineColor(ai_line_color); \
        CPnt4D ca(c.x, c.y, c.z, ai_line_opacity); \
        CPnt4D cb(c.x, c.y, c.z, ai_line_opacity); \
        CMatrix *m = StackTop(ai_line_mstack); \
        m->Transform(&pa, &pa, 1); \
        m->Transform(&pb, &pb, 1); \
        RenderLineWorld(pa, pb, ca, cb); \
    }

/* an octahedron of twelve lines about a point (OVL2 0x5FF2F0, new in SOCOM II; the name ours):
 * four corners half the width out in x and z (the width the height's when -1), each joined to a
 * tip half the height above and one below, then the corners joined in their own order (-x-z,
 * +x-z, -x+z, +x+z: two of the four a diagonal) -- CAiSFollow::Tick marks its goal with one,
 * (512, goal, 4, 3). DrawLine's kind test once, in place; then each line DrawLine's body.
 * The tips' parts are numbered in the order w, h, x, the top's y, z, the bottom's y while the
 * point's z is read before its x (the target's first loads: z, then the width's product, then
 * x) -- a dead first definition of each in that order, MW numbering a variable at its first
 * definition (README "Tells from the world, rdr and sound, round 12"); defined in their own
 * order, either the registers or that schedule moves. Each corner-to-corner line reads the
 * point into locals of its own (z first: x and y take $f20/$f21 across the first constructor),
 * a coordinate the two ends share folded into its local. */
void DrawOctahedron(LINE_TYPE type, const CPnt3D *pos, float height, float width)
{
    bool on = true;

    if (type != LINE_ANY && (ai_line_mask & type) == 0)
        on = false;
    if (on) {
        if (width == -1.0f)
            width = height;
        float w = 0.0f, h = 0.0f, tx = 0.0f, ty = 0.0f, tz = 0.0f, by = 0.0f;

        w = 0.5f * width;
        tz = pos->z;
        tx = pos->x;
        h = 0.5f * height;
        ty = pos->y + h;
        by = pos->y - h;
        AI_OCTAHEDRON_LINE(pos->x - w, pos->y, pos->z - w, tx, ty, tz)
        AI_OCTAHEDRON_LINE(pos->x + w, pos->y, pos->z - w, tx, ty, tz)
        AI_OCTAHEDRON_LINE(pos->x - w, pos->y, pos->z + w, tx, ty, tz)
        AI_OCTAHEDRON_LINE(pos->x + w, pos->y, pos->z + w, tx, ty, tz)
        AI_OCTAHEDRON_LINE(pos->x - w, pos->y, pos->z - w, tx, by, tz)
        AI_OCTAHEDRON_LINE(pos->x + w, pos->y, pos->z - w, tx, by, tz)
        AI_OCTAHEDRON_LINE(pos->x - w, pos->y, pos->z + w, tx, by, tz)
        AI_OCTAHEDRON_LINE(pos->x + w, pos->y, pos->z + w, tx, by, tz)
        {
            float z = pos->z - w, y = pos->y, x = pos->x;

            AI_OCTAHEDRON_LINE(x - w, y, z, x + w, y, z)
        }
        {
            float z = pos->z, y = pos->y, x = pos->x;

            AI_OCTAHEDRON_LINE(x + w, y, z - w, x - w, y, z + w)
        }
        {
            float z = pos->z + w, y = pos->y, x = pos->x;

            AI_OCTAHEDRON_LINE(x - w, y, z, x + w, y, z)
        }
        {
            float z = pos->z, y = pos->y, x = pos->x;

            AI_OCTAHEDRON_LINE(x + w, y, z + w, x - w, y, z - w)
        }
    }
}

}   // namespace ai
