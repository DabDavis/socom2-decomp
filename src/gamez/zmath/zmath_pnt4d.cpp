/* zmath_pnt4d.cpp -- CPnt4D (OVL1 0x309280-0x30936C) and, after it in the image, the
 * exp/trig tables (0x309370-0x30954C): whether those are this unit's or a unit of their own
 * without static initialisers is not known. The unit name is SOCOM 1's (__sinit_zmath_pnt4d.cpp).
 * Defined in reverse of their image order (-inline deferred). */

#include "zmath.h"

/* the exp table spans [0, 10] in 256 steps; the trig tables one turn */
static bool tableInit;
static float exptbl[257];
static float sintbl[257];
static float costbl[257];
static float m_exp_scl = 25.6f;

void init_trig_table()
{
    if (!tableInit) {
        float step = 0.024543693f;      /* one turn in 256 steps; a constant MW would fold */

        for (int i = 0; i < 257; i++) {
            float a = i * step;

            sintbl[i] = sinf((double)a);
            costbl[i] = cosf((double)a);
            exptbl[i] = exp(10.0 * i / 256.0);
        }
        tableInit = true;
    }
}

void uninit_trig_table()
{
    if (tableInit)
        tableInit = false;
}

/* e^x for x in [0, 10], linear between table entries */
float exp_tl(float x)
{
    if (x > 10.0f)
        x = 10.0f;
    else if (x < 0.0f)
        x = 0.0f;
    float f = x * m_exp_scl;
    int i = (int)f;
    int j = i + 1;
    float lo = exptbl[i];
    return lo + (f - i) * (exptbl[j] - lo);
}

const CPnt4D CPnt4D::zero(0.0f, 0.0f, 0.0f, 0.0f);
const CPnt4D CPnt4D::unit_x(1.0f, 0.0f, 0.0f, 0.0f);
const CPnt4D CPnt4D::unit_y(0.0f, 1.0f, 0.0f, 0.0f);
const CPnt4D CPnt4D::unit_z(0.0f, 0.0f, 1.0f, 0.0f);
const CPnt4D CPnt4D::unit_w(0.0f, 0.0f, 0.0f, 1.0f);

void CPnt4D::Rotate_AxisX(float a)
{
    float c = cosf(a);
    float s = sinf(a);
    float ns = -s;
    float zz = z;
    float yy = y;

    y = yy * c + zz * ns;
    z = yy * s + zz * c;
}

void CPnt4D::Rotate_AxisY(float a)
{
    float c = cosf(a);
    float s = sinf(a);
    float zz = z;
    float xx = x;

    x = xx * c + zz * s;
    z = zz * c - xx * s;
}

extern "C" {

/* VU0: out.xyz = a * s + b, out.w kept (new in SOCOM II: no SOCOM 1 name) */
void func_00309280(float *out, const float *a, const float *b, float s)
{
    float w = out[3];
#if Z_PORT
    /* vmulx.xyz vf6 = a * s; vadd.xyz vf6 += b; the sqc2 stores a stale w, put back below. Both
     * inputs read before the first store (out may be a or b) */
    float t0 = a[0] * s, t1 = a[1] * s, t2 = a[2] * s;
    float b0 = b[0], b1 = b[1], b2 = b[2];
    out[0] = t0 + b0;
    out[1] = t1 + b1;
    out[2] = t2 + b2;
#else
    asm __volatile__ (
        "lqc2 $vf4, 0(%0)\n"
        "lqc2 $vf7, 0(%1)\n"
        "mfc1 $8, %2\n"
        "nop\n"
        "qmtc2 $8, $vf5\n"
        "vmulx.xyz $vf6, $vf4, $vf5\n"
        "vadd.xyz $vf6, $vf6, $vf7\n"
        "sqc2 $vf6, 0(%3)\n"
        : : "r"(a), "r"(b), "f"(s), "r"(out) : "$8", "memory");
#endif
    out[3] = w;
}

}
