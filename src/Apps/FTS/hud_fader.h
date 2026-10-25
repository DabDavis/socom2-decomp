/* The HUD's faders: a full-screen bitmap faded in or out (CFader), a flash that fades out, holds
 * and fades back in (CFlashFX), and a list of flashes. SOCOM 1's DWARF layouts, the offsets
 * unchanged in SOCOM II. The header's name is ours. */
#ifndef HUD_FADER_H
#define HUD_FADER_H

#include <vector>

#include "../../gamez/zsys/zsys.h"
#include "../../gamez/ztwod/ztwod.h"

enum FADE_DIRECTION {
    IN = 0,
    OUT = 1,
    FADE = 2,
};

class CFader : public C2DBitmap {
public:
    CFader();
    virtual void Tick(float dT);
    void SetBrightness(float f);
    void SetMinBrightness(float brightness);
    void FadeIn(float time);
    /* not out of line in SOCOM 1's DWARF: CFlashFX::Flash has it inlined */
    void FadeOut(float time)
    {
        m_fadedir = OUT;
        m_fadetime = time;
        m_fadetotal = time;
        m_brightness = 1.0f;
        m_faderate = (1.0f - m_min_brightness) / m_fadetotal;
    }

    FADE_DIRECTION m_fadedir;   // 0x80
    float m_fadetime;   // 0x84
    float m_fadetotal;   // 0x88
    float m_faderate;   // 0x8c
    float m_brightness;   // 0x90
    float m_min_brightness;   // 0x94
};   // 0xa0 bytes

class CFlashFX : public CFader {
public:
    virtual void Tick(float dT);
    virtual void Reset();
    void Flash(float fadeout_time, float fade_duration, float fadein_time);

    FADE_DIRECTION m_fadedir;   // 0xa0
    float m_fadeout_time;   // 0xa4
    float m_fade_duration_time;   // 0xa8
    float m_fadein_time;   // 0xac
    float m_timer;   // 0xb0
};   // 0xc0 bytes

class CFlashFXVector {
public:
    /* a flash kept: the pointer a parameter of its own, whose address the push_back takes --
     * CHUD::Flash keeps its fader in a saved register through its calls and stores this copy
     * only for the push_back (the name is ours) */
    void Add(CFlashFX *fx) { m_FlashFX.push_back(fx); }
    void Reset();
    void Draw(zdb::CCamera *cam);
    void Tick(float dT);

    std::vector<CFlashFX *> m_FlashFX;   // 0x0
};   // 0xc bytes

#endif
