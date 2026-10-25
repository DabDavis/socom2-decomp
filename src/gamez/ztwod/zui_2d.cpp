/* zui_2d.cpp -- the UI's 2D controls (the press beta's asserts name the unit, zui_2d.cpp; SOCOM 1
 * kept it beside zui.h, in gamez/zui -- here with the 2D family until the UI has a directory of its
 * own). Written so far: CHProgressBar::Tick (0x3719E0), CPlainBmp's Tick, Draw and
 * SetMapOffset (OVL1 0x378FC0-0x37925B, 0x37C180) and the two speed setters (0x37B980, 0x37B990),
 * in SOCOM 1's order. Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "ztwod.h"

/* a UI variable as the plain bitmap reads it: its type (3 a string) and the string */
class CUIVariable {
public:
    bool IsString() const { return m_type == 3; }
    const char *GetString() const { return m_type == 3 ? m_str : 0; }

    unsigned char m_type;   // 0x0
    const char *m_str;   // 0x4
};

/* the texture handle's name, first (zdb's) */
namespace zdb {
class CTexHandle {
public:
    const char *m_name;   // 0x0
};
}

zdb::CTexHandle *PrepTexture(const char *name, zdb::CTextureRelocMgr *mgr);

extern "C" void func_00385D90(class UIVarMixin *mixin, CUIVariable *var);

/* a UI variable's hook (SOCOM 1's DWARF; the vtable pointer first) */
class UIVarMixin {
public:
    virtual ~UIVarMixin() { func_00385D90(this, 0); }

    CUIVariable *m_uivar;   // 0x4
};   // 8 bytes

/* a plain bitmap of the UI's (SOCOM 1's DWARF): it may pulse, and a string variable names its
 * texture */
class CPlainBmp : public C2DBitmap, public UIVarMixin {
public:
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Tick(float dT);
    virtual void SetMapOffset(int x, int y);

    bool m_pulsing;   // 0x88
    float m_period;   // 0x8c
    float m_curtrans;   // 0x90
};   // 0xa0 bytes

/* a horizontal progress bar (SOCOM 1's CHProgressBar, a C2D: its layout not in SOCOM 1's DWARF;
 * only what Tick reads placed, the names ours) */
class CHProgressBar : public C2D {
public:
    virtual void Tick(float dT);

    unsigned char _18[0xa0 - 0x18];
    float m_fill;   // 0xa0: 0 to 1
    float _a4;
    float m_time;   // 0xa8: the seconds a full bar takes
};

/* three controls as their command-type lookups see them (slot 24 of each vtable; only that slot
 * declared): the pop-up dialog (0x36D970), the renderable string (0x36FC10, CSecureText's and
 * C2DClock's too) and the button (0x3767C0, its subclasses' too). Each answers its own commands
 * as the UI's count plus an index, and an empty name with 0 */
class CPopUpDialog : public C2D {
public:
    /* declared, not defined: stand-ins whose implicit destructors would supersede the drafts */
    virtual ~CPopUpDialog();
    virtual int GetZAnimCmdType(const char *name);
};

class CRenderableString : public C2D {
public:
    virtual ~CRenderableString();
    virtual int GetZAnimCmdType(const char *name);
};

class C2DButton : public C2D {
public:
    virtual ~C2DButton();
    virtual int GetZAnimCmdType(const char *name);
};

/* slot 12: filled at its rate, to full */
void CHProgressBar::Tick(float dT)
{
    m_fill += dT / m_time;
    if (!(m_fill <= 1.0f))
        m_fill = 1.0f;
}

/* the speeds the counters count and the teletypes type at (SOCOM 1's statics, its symbol table's
 * names: C2DCounter::Tick and CTeleType::Tick read them; the menus' ZAnim commands and
 * CGameMenu::LoadFromDesign set them through the two setters, 0x37B990 and 0x37B980) */
namespace {
float g_fCounterSpeedFactor = 1.0f;
float g_fTtySpeedFactor = 1.0f;
}

void UiSetCounterSpeedFactor(float f)
{
    g_fCounterSpeedFactor = f;
}

void UiSetTtySpeedFactor(float f)
{
    g_fTtySpeedFactor = f;
}

/* slot 2: a string variable naming another texture loads it in the same rectangle first */
void CPlainBmp::Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr)
{
    if (UIVarMixin::m_uivar && UIVarMixin::m_uivar->IsString()) {
        const char *name = UIVarMixin::m_uivar->GetString();

        if (strcasecmp(name, m_pTexHandle ? m_pTexHandle->m_name : "")) {
            zdb::CTexHandle *tex = PrepTexture(name, mgr);

            if (tex)
                Load(m_x, m_y, m_x + m_iWidth, m_y + m_iHeight, tex);
        }
    }
    C2DBitmap::Draw(m, mgr);
}

/* slot 12: a pulsing bitmap's alpha rides a sine, once round per period, between 0 and 128 */
void CPlainBmp::Tick(float dT)
{
    if (m_pulsing) {
        m_curtrans += 6.2831855f * (dT / m_period);
        if (!(m_curtrans <= 6.2831855f))
            m_curtrans -= 6.2831855f;
        if (m_curtrans < 0.0f)
            m_curtrans += 6.2831855f;
        SetTrans(64.0f + 64.0f * sinf(m_curtrans));
    }
}

/* slot 25: moved, keeping its size */
void CPlainBmp::SetMapOffset(int x, int y)
{
    SetPos(x, y, x + m_iWidth, y + m_iHeight);
}

/* the button's commands: Caption and Valve (SOCOM II added Valve: SOCOM 1's answered Caption only) */
int C2DButton::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Valve", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}

/* the renderable string's commands, the button's two */
int CRenderableString::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 3;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Valve", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}

/* the dialog's commands: Caption and Background (its count, as the image has it, one) */
int CPopUpDialog::GetZAnimCmdType(const char *name)
{
    if (!name)
        return C2D::GetZAnimCmdType(name) + 1;
    if (!*name)
        return 0;
    if (!strcasecmp("Caption", name))
        return C2D::GetZAnimCmdType(name) + 1;
    if (!strcasecmp("Background", name))
        return C2D::GetZAnimCmdType(name) + 2;
    return C2D::GetZAnimCmdType(name);
}
