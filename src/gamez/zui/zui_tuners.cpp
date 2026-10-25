/* zui_tuners.cpp -- the system tuners (OVL1 0x389C60-0x38B178): one per option UI variable
 * (SOUND_VOLUME ... HEADSET_CONTROLLER), each attached to its variable and applying it to the
 * sound groups, the brightness or a pad's stick whenever it changes. SOCOM 1 kept them in a unit
 * of their own between zui_cleanup.cpp and zui_skb.cpp (UninitSystemTuners, the destructors,
 * InitSystemTuners, ~SystemTuner, then the inline UpdateUIVars); no assert names it, so the file
 * name is ours. SOCOM II renamed the acceleration tuner the inertia tuner, added the dialog and
 * headset volumes, tunes both sticks, moved the UpdateUIVars out of line and re-applies every
 * tuner after an IOP reboot. */

#include "zui.h"
#include "../zinput/zinput.h"

/* (the sound lane's zsnd.h has the whole class) */
class CSnd {
public:
    static void SetGroupVol(int group, float vol);
};

/* the screen's brightness (SOCOM 1's CVideo::Brightness; 0x3AFBE0, not named yet) */
extern "C" void func_003AFBE0(float brightness);

/* CIOPManager::UnRegisterRebootCallback (SOCOM 1's; zinput.h declares only the registration) */
extern "C" void func_0039C590(CIOPManager *iop, int id);

bool UIStoreOptions(class C2DAnimCmdHdr *hdr, float *dT);

/* a tuner: an option's UI variable made, attached and applied (SOCOM 1's, no DWARF) */
class SystemTuner : public UIVarMixin {
public:
    SystemTuner(const char *name)
    {
        CUIVariable *var = theUIVarManager.Add(name, 0, UIVAR_SESSION);

        var->m_longevity = UIVAR_PERMANENT;
        if (var) {
            AttachUIVar(var);
            UpdateUIVar(var);
        }
    }
    virtual ~SystemTuner() {}
    virtual void UpdateUIVar(CUIVariable *var) {}
    /* the variable given its default (ours) */
    void SetDefault(float f)
    {
        if (m_uivar)
            *m_uivar = f;
    }
};

class CSoundTuner : public SystemTuner {
public:
    CSoundTuner() : SystemTuner("SOUND_VOLUME") { SetDefault(1.0f); }
    void UpdateUIVar(CUIVariable *var);
};

class CMusicTuner : public SystemTuner {
public:
    CMusicTuner() : SystemTuner("MUSIC_VOLUME") { SetDefault(0.74f); }
    void UpdateUIVar(CUIVariable *var);
};

class CBrightnessTuner : public SystemTuner {
public:
    CBrightnessTuner() : SystemTuner("BRIGHTNESS") { SetDefault(0.5f); }
    void UpdateUIVar(CUIVariable *var);
};

/* a stick's tuner: which stick at 8 */
class CStickSensitivityTuner : public SystemTuner {
public:
    CStickSensitivityTuner(int stick) : SystemTuner("STICK_SENSITIVITY") { m_stick = stick; }
    void UpdateUIVar(CUIVariable *var);

    int m_stick;   // 0x8
};

class CStickInertiaTuner : public SystemTuner {
public:
    CStickInertiaTuner(int stick) : SystemTuner("STICK_INERTIA") { m_stick = stick; }
    void UpdateUIVar(CUIVariable *var);

    int m_stick;   // 0x8
};

class CStickDeadzoneTuner : public SystemTuner {
public:
    CStickDeadzoneTuner(int stick) : SystemTuner("STICK_DEADZONE") { m_stick = stick; }
    void UpdateUIVar(CUIVariable *var);

    int m_stick;   // 0x8
};

class CDialogTuner : public SystemTuner {
public:
    CDialogTuner() : SystemTuner("DIALOG_VOLUME") { SetDefault(1.0f); }
    void UpdateUIVar(CUIVariable *var);
};

class CHeadsetTuner : public SystemTuner {
public:
    CHeadsetTuner() : SystemTuner("HEADSET_CONTROLLER") { SetDefault(0.75f); }
    void UpdateUIVar(CUIVariable *var)
    {
        if (var->IsReal())
            CSnd::SetGroupVol(7, var->Real());
    }
};

static CSoundTuner *s_soundTuner;
static CMusicTuner *s_musicTuner;
static CBrightnessTuner *s_brightnessTuner;
static CStickSensitivityTuner *s_stickSensitivityTuner0;
static CStickInertiaTuner *s_stickInertiaTuner0;
static CStickDeadzoneTuner *s_stickDeadzoneTuner0;
static CStickSensitivityTuner *s_stickSensitivityTuner1;
static CStickInertiaTuner *s_stickInertiaTuner1;
static CStickDeadzoneTuner *s_stickDeadzoneTuner1;
static CDialogTuner *s_dialogTuner;
static CHeadsetTuner *s_headsetTuner;
static int s_rebootCallbackId;

/* after an IOP reboot the options are stored again (ours) */
static bool TunersRebootHandler(CIOPManager::RebootPhase phase, CIOPManager::RebootMode mode, void *data)
{
    UIStoreOptions(0, 0);
    return true;
}

void InitSystemTuners()
{
    s_soundTuner = new CSoundTuner;
    s_musicTuner = new CMusicTuner;
    s_brightnessTuner = new CBrightnessTuner;
    s_stickSensitivityTuner0 = new CStickSensitivityTuner(0);
    s_stickInertiaTuner0 = new CStickInertiaTuner(0);
    s_stickDeadzoneTuner0 = new CStickDeadzoneTuner(0);
    s_stickSensitivityTuner1 = new CStickSensitivityTuner(1);
    s_stickInertiaTuner1 = new CStickInertiaTuner(1);
    s_stickDeadzoneTuner1 = new CStickDeadzoneTuner(1);
    s_dialogTuner = new CDialogTuner;
    s_headsetTuner = new CHeadsetTuner;
    s_rebootCallbackId = theIOPManager.RegisterRebootCallback(TunersRebootHandler, 0);
}

void UninitSystemTuners()
{
    if (s_soundTuner)
        delete s_soundTuner;
    s_soundTuner = 0;
    if (s_musicTuner)
        delete s_musicTuner;
    s_musicTuner = 0;
    if (s_brightnessTuner)
        delete s_brightnessTuner;
    s_brightnessTuner = 0;
    if (s_stickSensitivityTuner0)
        delete s_stickSensitivityTuner0;
    s_stickSensitivityTuner0 = 0;
    if (s_stickInertiaTuner0)
        delete s_stickInertiaTuner0;
    s_stickInertiaTuner0 = 0;
    if (s_stickDeadzoneTuner0)
        delete s_stickDeadzoneTuner0;
    s_stickDeadzoneTuner0 = 0;
    if (s_stickSensitivityTuner1)
        delete s_stickSensitivityTuner1;
    s_stickSensitivityTuner1 = 0;
    if (s_stickInertiaTuner1)
        delete s_stickInertiaTuner1;
    s_stickInertiaTuner1 = 0;
    if (s_stickDeadzoneTuner1)
        delete s_stickDeadzoneTuner1;
    s_stickDeadzoneTuner1 = 0;
    if (s_dialogTuner)
        delete s_dialogTuner;
    s_dialogTuner = 0;
    func_0039C590(&theIOPManager, s_rebootCallbackId);
}

void CSoundTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal()) {
        CSnd::SetGroupVol(0, var->Real());
        CSnd::SetGroupVol(3, var->Real());
        CSnd::SetGroupVol(4, var->Real());
        CSnd::SetGroupVol(5, var->Real());
        CSnd::SetGroupVol(14, var->Real());
    }
}

void CMusicTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal())
        CSnd::SetGroupVol(1, var->Real());
}

void CBrightnessTuner::UpdateUIVar(CUIVariable *var)
{
    func_003AFBE0(var->Real());
}

void CStickSensitivityTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal()) {
        CPad *pad = CInput::m_pads[0];
        float f = var->Real();

        if (pad)
            pad->SetAcceleration(m_stick, f);
    }
}

void CStickInertiaTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal()) {
        CPad *pad = CInput::m_pads[0];
        float f = var->Real();

        if (pad)
            pad->m_stickType[m_stick].m_socom2_20 = 1.0f - f;
    }
}

void CStickDeadzoneTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal()) {
        CPad *pad = CInput::m_pads[0];
        float f = 0.02f + var->Real();

        if (f > 1.0f)
            f = 1.0f;
        if (pad)
            pad->SetDeadZone(m_stick, f);
    }
}

void CDialogTuner::UpdateUIVar(CUIVariable *var)
{
    if (var->IsReal()) {
        CSnd::SetGroupVol(2, var->Real());
        CSnd::SetGroupVol(6, var->Real());
    }
}

/* every tuner applied again (ours; the headset's is not) */
void RefreshSystemTuners()
{
    if (s_soundTuner)
        s_soundTuner->UpdateUIVar(s_soundTuner->m_uivar);
    if (s_musicTuner)
        s_musicTuner->UpdateUIVar(s_musicTuner->m_uivar);
    if (s_brightnessTuner)
        s_brightnessTuner->UpdateUIVar(s_brightnessTuner->m_uivar);
    if (s_stickSensitivityTuner0)
        s_stickSensitivityTuner0->UpdateUIVar(s_stickSensitivityTuner0->m_uivar);
    if (s_stickInertiaTuner0)
        s_stickInertiaTuner0->UpdateUIVar(s_stickInertiaTuner0->m_uivar);
    if (s_stickDeadzoneTuner0)
        s_stickDeadzoneTuner0->UpdateUIVar(s_stickDeadzoneTuner0->m_uivar);
    if (s_stickSensitivityTuner1)
        s_stickSensitivityTuner1->UpdateUIVar(s_stickSensitivityTuner1->m_uivar);
    if (s_stickInertiaTuner1)
        s_stickInertiaTuner1->UpdateUIVar(s_stickInertiaTuner1->m_uivar);
    if (s_stickDeadzoneTuner1)
        s_stickDeadzoneTuner1->UpdateUIVar(s_stickDeadzoneTuner1->m_uivar);
    if (s_dialogTuner)
        s_dialogTuner->UpdateUIVar(s_dialogTuner->m_uivar);
}
