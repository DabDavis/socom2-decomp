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

class C2DButton;

/* a button's four states' textures and its caption set (zui.cpp's InitButton, 0x36A3B0) */
void InitButton(C2DButton *button, zdb::CTexHandle *normal, zdb::CTexHandle *active, zdb::CTexHandle *pressed,
                zdb::CTexHandle *disabled, const char *caption);

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
    /* inline: the soft keyboard's ~CBmpWithNode (0x38E490) inlines it; its out-of-line copy is
     * 0x1FDBE0 (zui_2d.cpp's stand-in defines it) */
    virtual ~CPlainBmp() {}
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

/* a button's caption and states (SOCOM 1's DWARF: : C2D; SOCOM II's 0xC0 bytes, its vtable 29 slots:
 * SetActive and SetNormal pure, three new, DrawBackground the last) */
class C3StateButton : public C2D {
public:
    virtual ~C3StateButton();
    virtual void SetActive() = 0;
    virtual void SetNormal() = 0;
    virtual void Slot26() = 0;
    virtual void Slot27() = 0;
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr) = 0;

    char *_szCaption;           // 0x18
    C2DString _strCaption;      // 0x1c
    int _iTextX;                // 0xb4
    int _iTextY;                // 0xb8
    bool _bCaptionCentered;     // 0xbc
};   // 0xc0 bytes

/* a button (SOCOM 1's DWARF: : C3StateButton, 0x410 bytes there; SOCOM II's 0x440 with a UIVarMixin
 * at 0xC0, its constructor 0x377800 out of line) */
class C2DButton : public C3StateButton, public UIVarMixin {
public:
    C2DButton();
    virtual ~C2DButton();
    virtual void SetActive();
    virtual void SetNormal();
    virtual void Slot26();
    virtual void Slot27();
    virtual void DrawBackground(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void UpdateUIVar(CUIVariable *var);

    unsigned char _c8[0x440 - 0xc8];
} __attribute__((aligned(16)));   // 0x440 bytes

/* the button set up from its spec (SOCOM 1's C2DButton::LoadSpec; by address) */
extern "C" void func_00376BD0(C2DButton *button, const CButtonSpec *spec);

/* a strip of choices (SOCOM 1's CRadioStrip; SOCOM II's 0x450 bytes) */
class CRadioStrip : public C2DButton {
public:
    CRadioStrip() : m_choice(0) {}
    virtual ~CRadioStrip();

    int m_choice;               // 0x440
};   // 0x450 bytes

/* the strip set up from its spec (SOCOM 1's CRadioStrip::LoadSpec, with a texture; by address) */
extern "C" void func_003710C0(CRadioStrip *strip, const CRadioStripSpec *spec, zdb::CTexHandle *tex);

/* a button showing a spec of its own (SOCOM 1's CHandlerButton; SOCOM II's 0x440 bytes) */
class CHandlerButton : public C2DButton {
public:
    CHandlerButton() {}
    virtual ~CHandlerButton();
};   // 0x440 bytes

/* the handler button set up from its spec (by address) */
extern "C" void func_00376690(CHandlerButton *button, const CHandlerButtonSpec *spec);

/* two words between flags a check box keeps twice (the names ours) */
struct CCheckState {
    CCheckState() : m_on(false), m_4(0), m_8(0), m_c(true), m_d(16) {}

    bool m_on;                  // 0x0
    int m_4;                    // 0x4
    int m_8;                    // 0x8
    bool m_c;                   // 0xc: true
    unsigned char m_d;          // 0xd: 16
};   // 0x10 bytes

/* a check box: a button with a check mark (SOCOM 1's CCheckBox; SOCOM II's 0x520 bytes) */
class CCheckBox : public C2DButton {
public:
    CCheckBox() : m_checked(false) {}
    virtual ~CCheckBox();

    bool m_checked;             // 0x440
    unsigned char _441[0x454 - 0x441];
    CCheckState m_state0;       // 0x454 (two members: an array is built in a loop)
    CCheckState m_state1;       // 0x464
    unsigned char _474[0x480 - 0x474];
    CPlainBmp m_check;          // 0x480
};   // 0x520 bytes

/* the check box set up from its spec and its check's texture (by address) */
extern "C" void func_00371550(CCheckBox *box, const CCheckBoxSpec *spec, zdb::CTexHandle *tex);

/* a slider: a button with a knob (SOCOM 1's CSliderBar; SOCOM II's 0x510 bytes) */
class CSliderBar : public C2DButton {
public:
    CSliderBar() : m_value(0.5f), m_444(0), m_center(0.5f), m_intervals(10) {}
    virtual ~CSliderBar();

    float m_value;              // 0x440: 0.5
    int m_444;                  // 0x444
    unsigned char _448[0x450 - 0x448];
    float m_center;             // 0x450: 0.5
    unsigned char _454[0x460 - 0x454];
    CPlainBmp m_knob;           // 0x460
    int m_intervals;            // 0x500: 10
};   // 0x510 bytes

/* the slider set up from its spec and its knob's texture (by address) */
extern "C" void func_003724F0(CSliderBar *slider, const CSliderBarSpec *spec, zdb::CTexHandle *tex);

#endif
