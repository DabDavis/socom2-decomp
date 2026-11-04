/* ai_util.cpp (OVL2 0x50DC50-0x50E727, the name ours): the AI's geometry helpers -- namespace ai's
 * clock direction, quadratic, elevate, noise and motion test, and CAi's aiming helpers. SOCOM 1's
 * functions in the demo's order (0x17DE90-0x17F06F, between aidemo.cpp's plot_tick and
 * ai_events.cpp; no DWARF names their file); SOCOM II adds an XZ noise beside the 3D one and an
 * AddNoise pair over each. Defined here in reverse of the image's order (-inline deferred).
 * THE UNIT IS COMPILED WITHOUT AUTO-INLINING (the AI core's round 23; README "The inlining mode,
 * measured" had it as a candidate): no function of the unit is inlined into another anywhere in the
 * target -- both AddNoise pairs tail-call their three-argument halves, AddNoise(3) and AddNoiseXZ(3)
 * call MakeNoise/MakeNoiseXZ -- and one file-wide `#pragma auto_inline off` replaces round 22's three
 * pins, every function scoring as with them (the functions declared `inline` still inline). */

#include <stdlib.h>

#include "ai.h"

#pragma auto_inline off

/* a point into a VU0 vector, w 1 -- read through the reference, an address-taken local's fields
 * come off the frame (AlignToPoint; the names are ours) */
inline void to_vec(sceVu0FVECTOR v, const CPnt3D &p)
{
    v[0] = p.x;
    v[1] = p.y;
    v[2] = p.z;
    v[3] = 1.0f;
}

/* a draw in [0, 1), an inline of its own: the end of rand_range's second arm is then its exit,
 * the join MW pads to 8 bytes (the target's `b join; nop` before the sign multiply) where the draw
 * written in the arm let MW steal the multiply into the `b`'s delay slot (the name is ours) */
inline float _frand()
{
    return 4.656613e-10f * rand();
}

/* a random value in [lo, lo + span): MakeNoise's draws inline it (the name is ours) */
inline float rand_range(float lo, float span)
{
    float r;

    if (span == 0.0f)
        r = lo;
    else
        r = lo + span * _frand();
    return r;
}

/* where to aim to meet a moving point: the two motions over a time as velocities, the meeting
 * time a root of the quadratic they make, false if there is none (SOCOM 1's; the names are
 * ours). The t == 0 return sits at the end (the body nested under t != 0), v is declared first
 * (the highest slot).
 * OPEN, 32 instructions (right size): the dot products' float registers. The target loads w.x
 * twice (once per dot product, w.y and w.z once) and multiplies x first in v.w, so its registers
 * run f4/f6/f7 where this build's run f6/f7/f8 (w.y loaded first); Dot(w, v), the products
 * written out, the sums in locals in either order all compile alike
 * (build/scratch/ai/spec_lead.py) */
bool CAi::LeadTargetG(const CPnt3D *a, const CPnt3D *b, float t, const CPnt3D *c, const CPnt3D *d,
                      CPnt3D *out)
{
    CPnt3D v;

    if (t != 0.0f) {
        float inv = 1.0f / t;
        CPnt3D w = *c - *a;

        w.Scale(inv, &w);
        CPnt3D u = *d - *b;

        u.Scale(inv, &v);
        float k = 1.0f - Dot(v, v);

        if (k <= 0.0f)
            return false;
        float vw = Dot(v, w);

        u.Scale((vw + sqrtf(k * Dot(w, w) + vw * vw)) / k, &v);
        c->Add(&v, out);
        return true;
    }
    return false;
}

/* a point a time ahead along a velocity (SOCOM 1's) */
void CAi::Extrapolate(const CPnt3D *pos, const CPnt3D *vel, float t, CPnt3D *out)
{
    vel->Scale(t, out);
    out->Add(pos, out);
}

/* a matrix turned to face a point in XZ and moved to so far short of it, its up row kept
 * (SOCOM 1's) */
void CAi::AlignToPoint(const CMatrix *m, const CPnt3D *p, CMatrix *out, float dist)
{
    CPnt3D dir(0.0f, 0.0f, 0.0f);

    p->SubXZ((const CPnt3D *)m->m_matrix[3], &dir);
    float len = dir.Normalize();
    CPnt3D at = *(const CPnt3D *)m->m_matrix[3] + dir * (len - dist);
    CPnt3D right;
    sceVu0FVECTOR side, fwd, top;

    to_vec(fwd, dir);
    to_vec(top, *(const CPnt3D *)m->m_matrix[1]);
    sceVu0OuterProduct(side, fwd, top);
    sceVu0Normalize(side, side);
    right.x = side[0];
    right.y = side[1];
    right.z = side[2];
    out->SetRows(&right, (const CPnt3D *)m->m_matrix[1], &dir, &at);
}

/* whether a velocity is above a speed squared (SOCOM 1's) */
bool ai::is_moving(const CPnt3D *v, float speedSq)
{
    return v->x * v->x + v->y * v->y + v->z * v->z > speedSq;
}

/* a random offset in a ring: each axis a random size in [range.x, range.y), a random sign
 * (SOCOM 1's; SOCOM II returns the point).
 * Byte-exact with MakeNoiseXZ since the AI core's round 23 (open from round 2 to 22 on the saved
 * float registers alone: span f21 and sign f20 against the target's span f20, sign f21). MW's
 * colouring (build/scratch/ai/ig/, the MSL lane's igdump under gdb) showed why: a sign assigned from
 * a ternary is absorbed into the ternary's temporary, numbered after every named local and so
 * coloured ahead of span; assigned in an if/else it stays the named local declared between lo and
 * span -- and the product's operands then come out in the target's order (value, sign) only with the
 * draw in a named local of its own (an inline's result multiplied by a variable always goes second) */
CPnt3D *ai::MakeNoise(CPnt3D *out, const PNT2D &range)
{
    float lo = range.x;
    float sign;
    float span = range.y - range.x;
    float r;

    if (rand_range(0.0f, 1.0f) <= 0.5f)
        sign = 1.0f;
    else
        sign = -1.0f;
    r = rand_range(lo, span);
    out->x = r * sign;
    if (rand_range(0.0f, 1.0f) <= 0.5f)
        sign = 1.0f;
    else
        sign = -1.0f;
    r = rand_range(lo, span);
    out->y = r * sign;
    if (rand_range(0.0f, 1.0f) <= 0.5f)
        sign = 1.0f;
    else
        sign = -1.0f;
    r = rand_range(lo, span);
    out->z = r * sign;
    return out;
}

/* the same in the ground plane, y zero (new in SOCOM II; the name is ours) */
CPnt3D *ai::MakeNoiseXZ(CPnt3D *out, const PNT2D &range)
{
    float lo = range.x;
    float sign;
    float span = range.y - range.x;
    float r;

    if (rand_range(0.0f, 1.0f) <= 0.5f)
        sign = 1.0f;
    else
        sign = -1.0f;
    r = rand_range(lo, span);
    out->x = r * sign;
    out->y = 0.0f;
    if (rand_range(0.0f, 1.0f) <= 0.5f)
        sign = 1.0f;
    else
        sign = -1.0f;
    r = rand_range(lo, span);
    out->z = r * sign;
    return out;
}

/* a point moved by a random offset (SOCOM 1's two; the two-argument one tail-calls the other) */
CPnt3D *ai::AddNoise(const CPnt3D *p, CPnt3D *out, const PNT2D &range)
{
    CPnt3D noise;

    MakeNoise(&noise, range);
    p->Add(&noise, out);
    return out;
}

CPnt3D *ai::AddNoise(CPnt3D *p, const PNT2D &range)
{
    return AddNoise(p, p, range);
}

/* the same in the ground plane (new in SOCOM II; the names are ours) */
CPnt3D *ai::AddNoiseXZ(const CPnt3D *p, CPnt3D *out, const PNT2D &range)
{
    CPnt3D noise;

    MakeNoiseXZ(&noise, range);
    p->Add(&noise, out);
    return out;
}

CPnt3D *ai::AddNoiseXZ(CPnt3D *p, const PNT2D &range)
{
    return AddNoiseXZ(p, p, range);
}

/* a point scaled in XZ and set to a height (SOCOM 1's) */
void ai::elevate(CPnt3D &p, float y, float scale)
{
    p.x *= scale;
    p.z *= scale;
    p.y = y;
}

/* the real roots of a*x*x + b*x + c, false if none (SOCOM 1's). The linear case stores its root
 * twice into r2 and never into r1 -- both builds do.
 * The root taken back into d (the AI core's round 18, byte-exact): the target takes it in f0 and -b
 * in f2 after it (SOCOM 1's sqrtf call left the same pair); a root in a local of its own took f2
 * and -b f0, and sqrtf written into both expressions took f0 but let MW hoist -b above it into the
 * divide's hazard slot */
bool ai::quadratic(float a, float b, float c, float &r1, float &r2)
{
    bool ok = true;

    if (a != 0.0f) {
        float d = b * b - 4.0f * a * c;
        float inv = 1.0f / (2.0f * a);

        if (d > 0.0f) {
            d = sqrtf(d);
            r1 = inv * (-b + d);
            r2 = inv * (-b - d);
        } else if (d < 0.0f) {
            ok = false;
        } else {
            r1 = r2 = -b * inv;
        }
    } else if (b != 0.0f) {
        r2 = r2 = -c / b;
    } else {
        ok = false;
    }
    return ok;
}

/* a direction as the hour on a clock face, 12 ahead (SOCOM 1's). The hour goes through one
 * variable: returned from each arm, MW if-converts every left/right pair into one exit */
int ai::get_clock_direction(const CPnt3D *dir)
{
    int hour;
    float ax = fabsf(dir->x);

    if (dir->z < 0.0f) {
        if (ax <= 0.2833f) {
            hour = 12;
        } else if (ax <= 0.708f) {
            if (dir->x > 0.0f)
                hour = 1;
            else
                hour = 11;
        } else if (dir->z > -0.2833f) {
            if (dir->x > 0.0f)
                hour = 2;
            else
                hour = 10;
        } else {
            if (dir->x > 0.0f)
                hour = 3;
            else
                hour = 9;
        }
    } else {
        if (ax <= 0.2833f) {
            hour = 6;
        } else if (ax <= 0.708f) {
            if (dir->x > 0.0f)
                hour = 5;
            else
                hour = 7;
        } else if (dir->z > -0.2833f) {
            if (dir->x > 0.0f)
                hour = 4;
            else
                hour = 8;
        } else {
            if (dir->x > 0.0f)
                hour = 3;
            else
                hour = 9;
        }
    }
    return hour;
}
