/* zrndr_postfx.cpp -- SOCOM II's effect the pipe keeps at thePipe+0x688 (OVL1 0x33CD50-0x33D22F): a
 * unit of its own after zrndr_pipe.cpp, whose template tail closes at 0x33CD4F -- CPipe() and ~CPipe
 * call this unit's constructor and destructor out of line, and its initialiser 0x402F80, right after
 * the pipe's, builds only <iostream>'s two objects. No SOCOM 1 counterpart; the unit's and the
 * class's names are ours. Its tick 0x33CD50 (CCoreState::Tick calls it; it flushes the pipe's FIFO
 * and eases two of the parameters through 0x33D1E0) is not written yet. Defined in reverse of their
 * image order (-inline deferred). */

#include <eetypes.h>
#include <iostream>

/* libm (zmath.h declares them so) */
extern "C" float sinf(float);
extern "C" float cosf(float);

/* a byte the setter mirrors its flag into (OVL1 bss 0x4A4548; not named) */
extern unsigned char data_004A4548;
/* the word the tick stores the effect's flag byte 0x32 into before the flush (OVL1 bss 0x4A4550) */
extern int data_004A4550;
/* the frame's time (fts_patch.cpp's) */
extern float data_004A449C;
/* the level the effect follows, by a selector (OVL1 0x3B1BF0, not named) and a value eased toward a
 * target by exp(rate * dt), the value's address given back (0x33D1E0, this unit's tail: a draft) */
extern "C" float func_003B1BF0(int which);
extern "C" float *func_0033D1E0(float *v, float *target, float rate, float dt);

/* the pipe, as much as the tick uses (zrndr_pipe.cpp's thePipe, 0x4887C0) */
class CPipe {
public:
    void FIFO_Flush();
};
extern CPipe thePipe;

/* a colour and a GS register word (zrndr_pipe.cpp's tag_PipeColor) */
struct tag_PipeColor {
    unsigned int m_0;   // 0x0
    unsigned int m_rgba;   // 0x4
    u_long m_reg;   // 0x8
};
/* the colour the tick sets each frame: thePipe + 0x6D0, just past the effect (thezoom.cpp clears
 * its first word) */
extern tag_PipeColor data_00488E90;

/* a GS register word as its two halves (ours) */
struct tag_RegWords {
    unsigned int m_lo;
    unsigned int m_hi;
};

class CPipeEffect {
public:
    CPipeEffect();
    ~CPipeEffect();
    /* the flag at 0x31 set, and mirrored (0x33D100; CCoreState's 0x264610 calls it) */
    void SetOn(bool on);
    /* the frame's colour from the followed level (0x33CD50; CCoreState::Tick calls it) */
    void Tick();

    int m_mode;   // 0x0
    float m_4;   // 0x4
    float m_8;   // 0x8
    float m_c;   // 0xc
    float m_10;   // 0x10
    unsigned int m_color;   // 0x14
    float m_18;   // 0x18: eased by the tick
    float m_1c;   // 0x1c: eased by the tick
    int m_20;   // 0x20
    unsigned char m_24;   // 0x24
    u_long m_reg;   // 0x28: a GS register word
    unsigned char m_30;   // 0x30
    unsigned char m_on;   // 0x31
    unsigned char m_32;   // 0x32
    float m_34;   // 0x34
    unsigned char _38[0x44 - 0x38];   // 0x38
    int m_44;   // 0x44
    unsigned char _48[0x58 - 0x48];   // 0x48
};   // 0x58 bytes (the pipe's next member at 0x6e0)

/* the parameters' defaults (0x33D160) */
CPipeEffect::CPipeEffect()
{
    m_mode = 2;
    m_4 = 0.35f;
    m_8 = 0.0f;
    m_c = 100.0f;
    m_10 = 0.0f;
    m_20 = 16;
    m_18 = -5.0f;
    m_1c = -0.6666667f;
    m_color = 0x80808080;
    m_34 = 0.0f;
    m_44 = 0;
    m_on = 0;
    m_24 = 0;
    m_32 = 0;
    m_reg = 0x8000000069;
    m_30 = 0;
}

/* nothing to give back (0x33D110) */
CPipeEffect::~CPipeEffect()
{
}

void CPipeEffect::SetOn(bool on)
{
    m_on = on;
    data_004A4548 = on;
}

/* while on: the FIFO flushed when the mirror byte is set; the followed level's distance past the
 * threshold (above it when the flag 0x24 allows, else below it) as a fraction of the span to 0x8,
 * the value at 0x34 eased toward it (rising at 0x1c's rate, falling at 0x18's) and averaged over
 * the last three frames; the alpha the average times 0xc -- shaped by a cosine when 0x10 is
 * positive -- capped at 255 and put in the register word's high half: the pipe's colour gets the
 * mode, the colour and the word. Off, the colour's mode is cleared (written in round 25 of the
 * world lane; the names ours).
 * OPEN, 76 instructions at the target's size: the float registers -- the mode converted from the
 * unsigned word takes f20 here, f21 in the target, and the average f21 against f20 -- the average
 * summed as h1 + h2 then h0 (the target adds 0.0, h0, h1, h2 in order into f20: a summing loop does,
 * but costs 8 bytes elsewhere), and the alpha's last product `m_c * x` where the target has `x * m_c`
 * with its 1.0 and 0.5 in each other's registers. The statics are function-local (data rows
 * @LOCAL@/@GUARD@ for hist, idx and the guarded k) */
void CPipeEffect::Tick()
{
    static float hist[3];
    static unsigned int idx;

    if (m_on) {
        if (data_004A4548) {
            data_004A4550 = m_32;
            thePipe.FIFO_Flush();
        }
        float level = func_003B1BF0(m_20);
        float target;

        if (m_24 && !(level <= m_4))
            target = (level - m_4) / (m_8 - level);
        else if (level < m_4)
            target = (m_4 - level) / (m_4 - m_8);
        float mode = (unsigned int)m_mode;

        if (m_34 < target)
            func_0033D1E0(&m_34, &target, m_1c, data_004A449C);
        else if (!(m_34 <= target))
            func_0033D1E0(&m_34, &target, m_18, data_004A449C);
        hist[idx] = m_34;
        idx++;
        if (idx >= 3)
            idx = 0;
        float avg = (0.0f + hist[0] + hist[1] + hist[2]) / 3.0f;
        float a;

        if (!(m_10 <= 0.0f)) {
            static float k = cosf(0.7853982f) - 0.5f;
            float t = 3.1415927f * (1.0f - avg);
            float w = k * ((m_10 - 1.0f) * sinf(2.0f * t));

            a = (int)(0.5f + 0.5f * (1.0f + (cosf(t) - w)) * m_c);
        } else
            a = (int)(0.5f + avg * m_c);
        if (255.0f < a)
            a = 255.0f;
        tag_RegWords r;

        r.m_lo = *(unsigned int *)&m_reg;
        r.m_hi = (unsigned int)a;
        data_00488E90.m_0 = (unsigned int)mode;
        data_00488E90.m_reg = *(u_long *)&r;
        data_00488E90.m_rgba = m_color;
    } else
        data_00488E90.m_0 = 0;
}
