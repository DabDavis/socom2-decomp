/* hud_fader.cpp -- the HUD's faders and flashes (OVL1 0x228660-0x228ECF): SOCOM 1's unit (its
 * DWARF names it, Z:\dev\Apps\FTS\hud_fader.cpp), the same functions in the same order, from
 * CFlashFX's implicit destructor to CFader's constructor; hud_missioncams.cpp follows. SOCOM II
 * no longer keeps CFader::SetBrightness out of line, and its CFader::Tick is CFlashFX's call,
 * not inlined there. Defined in reverse of their image order (-inline deferred). */

#include "hud_fader.h"

/* the brightness clamped to [min, 1], and the bitmap made that transparent (inline in SOCOM II:
 * no out-of-line copy is left between Flash and SetMinBrightness) */
inline void CFader::SetBrightness(float f)
{
    m_brightness = f;
    if (f > 1.0f)
        m_brightness = 1.0f;
    else if (f < m_min_brightness)
        m_brightness = m_min_brightness;
    SetTrans(128.0f * (1.0f - m_brightness));
}

/* a black full-screen (640 x 448) untextured bitmap, transparent, off */
CFader::CFader()
{
    m_fadedir = IN;
    m_fadetime = 0.0f;
    m_fadetotal = 0.0f;
    m_faderate = 0.0f;
    m_min_brightness = 0.0f;
    SetHasTexture(false);
    Load(0.0f, 0.0f, 640.0f, 448.0f, 0);
    SetColor(0.0f, 0.0f, 0.0f);
    SetTrans(128.0f);
    SetIsTrans(true);
    Off();
}

/* the fade stepped (by a tenth of a second at most): on and brightening or darkening while it
 * lasts; done, a fade in turns the bitmap off and a fade out keeps it on, darkening */
void CFader::Tick(float dT)
{
    float dt = dT < 0.1f ? dT : 0.1f;

    if (m_fadetime > 0.0f) {
        On();
        if (m_fadedir == IN) {
            m_brightness += m_faderate * dt;
            SetBrightness(m_brightness);
        } else {
            m_brightness -= m_faderate * dt;
            SetBrightness(m_brightness);
        }
    } else if (m_fadedir == IN) {
        Off();
    } else {
        On();
        m_brightness -= m_faderate * dt;
        SetBrightness(m_brightness);
    }
    m_fadetime -= dt;
}

void CFader::FadeIn(float time)
{
    m_fadedir = IN;
    m_fadetime = time;
    m_fadetotal = time;
    m_brightness = m_min_brightness;
    m_faderate = (1.0f - m_min_brightness) / m_fadetotal;
}

void CFader::SetMinBrightness(float brightness)
{
    m_min_brightness = brightness;
    m_faderate = (1.0f - brightness) / m_fadetotal;
}

/* a flash: fade out over the first time, hold for the second, fade back in over the third */
void CFlashFX::Flash(float fadeout_time, float fade_duration, float fadein_time)
{
    m_fadeout_time = fadeout_time;
    m_fade_duration_time = fade_duration;
    m_fadein_time = fadein_time;
    m_fadedir = OUT;
    m_timer = m_fadeout_time;
    FadeOut(m_timer);
}

void CFlashFX::Reset()
{
    m_fadeout_time = 0.0f;
    m_fade_duration_time = 0.0f;
    m_fadein_time = 0.0f;
    m_fadedir = IN;
    m_timer = 0.0f;
    FadeIn(0.0f);
}

/* the fader stepped, then the flash's phase: out, then holding (if it holds), then back in */
void CFlashFX::Tick(float dT)
{
    float dt = dT < 0.1f ? dT : 0.1f;

    CFader::Tick(dt);
    m_timer -= dt;
    switch (m_fadedir) {
    case OUT:
        if (m_timer < 0.0f) {
            if (m_fade_duration_time > 0.0f) {
                m_timer = m_fade_duration_time;
                m_fadedir = FADE;
            } else if (m_fadein_time > 0.0f) {
                m_timer = m_fadein_time;
                m_fadedir = IN;
                FadeIn(m_timer);
            }
        }
        break;
    case FADE:
        if (m_timer < 0.0f && m_fadein_time > 0.0f) {
            m_timer = m_fadein_time;
            m_fadedir = IN;
            FadeIn(m_timer);
        }
        break;
    case IN:
        break;
    }
}

void CFlashFXVector::Tick(float dT)
{
    int n = m_FlashFX.size();

    for (int i = 0; i < n; i++)
        m_FlashFX[i]->Tick(dT);
}

/* the flashes that are on drawn */
void CFlashFXVector::Draw(zdb::CCamera *cam)
{
    int n = m_FlashFX.size();

    for (int i = 0; i < n; i++) {
        if (m_FlashFX[i]->IsOn())
            m_FlashFX[i]->Draw(cam);
    }
}
