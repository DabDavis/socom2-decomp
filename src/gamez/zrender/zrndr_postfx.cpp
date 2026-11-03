/* zrndr_postfx.cpp -- SOCOM II's effect the pipe keeps at thePipe+0x688 (OVL1 0x33CD50-0x33D22F): a
 * unit of its own after zrndr_pipe.cpp, whose template tail closes at 0x33CD4F -- CPipe() and ~CPipe
 * call this unit's constructor and destructor out of line, and its initialiser 0x402F80, right after
 * the pipe's, builds only <iostream>'s two objects. No SOCOM 1 counterpart; the unit's and the
 * class's names are ours. Its tick 0x33CD50 (CCoreState::Tick calls it; it flushes the pipe's FIFO
 * and eases two of the parameters through 0x33D1E0) is not written yet. Defined in reverse of their
 * image order (-inline deferred). */

#include <eetypes.h>
#include <iostream>

/* a byte the setter mirrors its flag into (OVL1 bss 0x4A4548; not named) */
extern unsigned char data_004A4548;

class CPipeEffect {
public:
    CPipeEffect();
    ~CPipeEffect();
    /* the flag at 0x31 set, and mirrored (0x33D100; CCoreState's 0x264610 calls it) */
    void SetOn(bool on);

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
