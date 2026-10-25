/* zui_2d.h -- the UI's display objects as the spec units build them (zui_2d.cpp's classes: SOCOM 1
 * kept them beside zui.h; their own code is the 2D family's, src/gamez/ztwod/zui_2d.cpp). Each
 * spec's CreateDisplayObj news one and inlines its constructor: what is declared here is what
 * those constructors store, at SOCOM II's offsets (read off the CreateDisplayObjs and the
 * constructors' out-of-line copies), with SOCOM 1's DWARF names where it has the member.
 *
 * Their vtables are SOCOM II's tables in data.txt: a class with a UIVarMixin base has a second
 * table right after its first (C2DString's 34 slots, or C2D's 26), which the constructors store at
 * the mixin's offset. The functions the 2D family has not named are called by address. */
#ifndef ZUI_2D_H
#define ZUI_2D_H

#include "../ztwod/ztwod.h"
#include "zui_spec.h"

/* a texture by name, prepared for drawing (zui.cpp's PrepTexture, 0x36A230) */
zdb::CTexHandle *PrepTexture(const char *name, zdb::CTextureRelocMgr *mgr);

/* a string a spec configures (SOCOM 1's DWARF: : C2DString, UIVarMixin; SOCOM II's 0xC0 bytes, its
 * constructor 0x37A770 out of line: grey-white 255, scale 2, the offset a static's copy) */
class CRenderableString : public C2DString, public UIVarMixin {
public:
    CRenderableString();
    virtual ~CRenderableString();
    virtual void UpdateUIVar(CUIVariable *var);
    void LoadSpec(const CTextSpec &spec);

    C2DFont *m_alternateFont;   // 0xa0
    CPnt3D m_offset;            // 0xa4
    CPnt3D m_Color;             // 0xb0
    int m_socom2_bc;            // 0xbc: cleared by the constructor
};   // 0xc0 bytes

/* a clock (SOCOM 1's C2DClock: a renderable string formatting its variable's seconds) */
class C2DClock : public CRenderableString {
public:
    C2DClock() {}
    virtual ~C2DClock();
    virtual void UpdateUIVar(CUIVariable *var);
};   // 0xc0 bytes

/* a password's stars (SOCOM II's CSecureText, a renderable string) */
class CSecureText : public CRenderableString {
public:
    CSecureText() {}
    virtual ~CSecureText();
};   // 0xc0 bytes

/* a counter running from its start to its end, or for a duration, from a value (SOCOM 1's
 * C2DCounter; the members' names ours) */
class C2DCounter : public CRenderableString {
public:
    C2DCounter(float start, bool counting, float end, int value)
        : m_start(start), m_counting(counting), m_end(end), m_value(value), m_elapsed(0), m_last(-1)
    {
    }
    virtual ~C2DCounter();

    float m_start;       // 0xc0: the spec's StartTime
    bool m_counting;     // 0xc4: an end given
    float m_end;         // 0xc8: EndTime, or Duration
    int m_value;         // 0xcc: the value variable's when made
    int m_elapsed;       // 0xd0
    int m_last;          // 0xd4: -1
};   // 0xd8 bytes

/* a plain bitmap of the UI's (SOCOM 1's DWARF; zui_2d.cpp's): it may pulse, and a string variable
 * may name its texture. Its constructor is inline: grey, opaque, alpha-blended, taking no input */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    CPlainBmp()
    {
        SetColor(128.0f, 128.0f, 128.0f);
        SetTrans(128.0f);
        SetIsTrans(true);
        SetCanHandleInput(false);
        m_pulsing = false;
        m_period = 1.0f;
    }
    virtual ~CPlainBmp();
    virtual void UpdateUIVar(CUIVariable *var);

    bool m_pulsing;     // 0x88
    float m_period;     // 0x8c
    float m_curtrans;   // 0x90
} __attribute__((aligned(16)));   // 0xa0 bytes

/* the bitmap set up from an image spec and its texture (SOCOM 1's CPlainBmp::LoadSpec; by address) */
extern "C" void func_00379080(CPlainBmp *bmp, const CImageSpec *spec, zdb::CTexHandle *tex);

/* a horizontal progress bar: a plain bitmap filled to its variable's value (SOCOM 1's
 * CHProgressBar; SOCOM II's 0xC0 bytes. Its UpdateUIVar takes CPlainBmp's slot after the tables,
 * +0x80, and its LoadSpec is a slot of its own, +0x84) */
class CHProgressBar : public CPlainBmp {
public:
    CHProgressBar() : m_fill(0.0f), m_time(0.0f), m_ac(0), m_b0(0), m_b4(0) {}
    virtual ~CHProgressBar();
    virtual void UpdateUIVar(CUIVariable *var);
    virtual void LoadSpec(const CHProgressBarSpec &spec, zdb::CTexHandle *tex);

    float m_fill;               // 0xa0: 0 to 1
    float m_a4;                 // 0xa4
    float m_time;               // 0xa8
    int m_ac;                   // 0xac
    int m_b0;                   // 0xb0
    int m_b4;                   // 0xb4
} __attribute__((aligned(16)));   // 0xc0 bytes

/* text wrapped to a width, its arrows two plain bitmaps (SOCOM 1's CWrappedText; SOCOM II's 0x250
 * bytes, its inline constructor's out-of-line copy linked from an earlier unit, 0x27F250). Too big
 * to inline whole: its callers inline its body and call CPlainBmp's constructor (0x20CD50) */
class CWrappedText : public CRenderableString {
public:
    CWrappedText() : m_line(-1), m_c8(0), m_cc(0), m_d0(false), m_d4(0), m_d8(0), m_240(false) {}
    virtual ~CWrappedText();

    int m_c0;                   // 0xc0
    int m_line;                 // 0xc4: -1
    int m_c8;                   // 0xc8
    int m_cc;                   // 0xcc
    bool m_d0;                  // 0xd0
    int m_d4;                   // 0xd4
    int m_d8;                   // 0xd8
    unsigned char _dc[0x100 - 0xdc];
    CPlainBmp m_upArrow;        // 0x100
    CPlainBmp m_downArrow;      // 0x1a0
    bool m_240;                 // 0x240
};   // 0x250 bytes

/* the wrapped text set up from its spec (SOCOM 1's CWrappedText::LoadSpec; by address) */
extern "C" void func_0037A430(CWrappedText *text, const CWrapSpec *spec, zdb::CTextureRelocMgr *mgr);

/* wrapped text scrolling by itself (SOCOM 1's CScrollText; SOCOM II's 0x270 bytes) */
class CScrollText : public CWrappedText {
public:
    CScrollText() : m_254(false), m_258(0), m_25c(0), m_260(0) {}
    virtual ~CScrollText();

    int m_250;                  // 0x250
    bool m_254;                 // 0x254
    int m_258;                  // 0x258
    int m_25c;                  // 0x25c
    int m_260;                  // 0x260
};   // 0x270 bytes

/* the scrolling text set up from its spec (by address: an eight-byte jump into the wrapped text's) */
extern "C" void func_00370340(CScrollText *text, const CScrollSpec *spec);

/* a teletype: text typed out, its cursor a plain bitmap (SOCOM 1's CTeleType; SOCOM II's 0x1B0) */
class CTeleType : public CRenderableString {
public:
    CTeleType() : m_c0(0), m_c4(0), m_c8(0), m_1a0(0) {}
    virtual ~CTeleType();

    int m_c0;                   // 0xc0
    int m_c4;                   // 0xc4
    int m_c8;                   // 0xc8
    unsigned char _cc[0x100 - 0xcc];
    CPlainBmp m_cursor;         // 0x100
    int m_1a0;                  // 0x1a0
};   // 0x1b0 bytes

/* the teletype set up from its spec (SOCOM 1's CTeleType::LoadSpec; by address) */
extern "C" void func_00378B00(CTeleType *tty, const CTtySpec *spec);

#endif
