/* The 2D display objects (ztwod) as the HUD units use them: C2D, C2DFade, C2DBitmap. Their own
 * code is ztwod's (OVL1 0x3630xx-0x365xxx), not written here; this is only what the HUD needs
 * declared, in SOCOM II's layout -- which is not SOCOM 1's DWARF:
 *   - C2D is 0x18 bytes, not 0x1C: SOCOM 1's m_active_and_handling_input byte became the top
 *     bit of the flag byte at 0x14 (CanHandleInput reads bit 7 there), and C2DFade follows at
 *     0x18 (C2DBitmap's constructor zeroes its byte at 0x18 and sets its limit, 128, at 0x20);
 *   - its vtable has 26 slots, not 21: five new after On (slots 6-10). C2DBitmap's 6-9 set a
 *     flag bit and the matching GS PRIM bit in the word at 0x2C: bit 3 with TME (0x10), bit 1
 *     with AA1 (0x80), bit 2 with ABE (0x40), bit 5 with IIP (0x08) -- SOCOM 1's m_hasTexture,
 *     m_isAA and m_isTrans bits, and a new one. Slot 10 is empty in C2D. Their names are ours.
 * C2DBitmap is 16-aligned (its colour rows are VU0 vectors): SOCOM 1's CFader is 0xA0 bytes with
 * its last member ending at 0x98, CFlashFX 0xC0 with its at 0xB4.
 * This belongs in src/gamez/ztwod/ztwod.h, which does not exist yet. */
#ifndef HUD_2D_H
#define HUD_2D_H

#include "../../gamez/zmath/zmath.h"

namespace zdb {
class CAssetLib;
class CCamera;
class CTexHandle;
class CTextureRelocMgr;
}
class CPad;
class CUIVariable;
class C2DAnimCmdHdr;
struct _zrdr;
struct _zanim_cmd_hdr;

/* SOCOM 1's DWARF: `class C2Dlist : public list` (0xC bytes); its contents are C2D's business */
class C2Dlist {
public:
    unsigned char _list[0xc];
};

class C2D : public C2Dlist {
public:
    class CLibList;

    virtual ~C2D();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Enable(bool on);
    virtual void Off();
    virtual void On();
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void Slot10();
    virtual void SetTrans(float trans);
    virtual void Tick(float dT);
    virtual void Parse(_zrdr *rdr, zdb::CAssetLib *lib, zdb::CTextureRelocMgr *mgr);
    virtual void Reset();
    virtual bool HandlePad(const CPad *pad, int port);
    virtual bool CanHandleInput();
    virtual void SetCanHandleInput(bool val);
    virtual void SetActive();
    virtual void SetNormal();
    virtual void SetUseFrameBufferAlpha(bool isFrameAlpha);
    virtual void GetAssetLibs(CLibList &liblist);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr);
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *cmd, float *t, bool *done);
    virtual int GetZAnimCmdType(const char *name);
    virtual void SetMapOffset(int x, int y);

    bool IsOn() const;

    CUIVariable *m_uivar;   // 0x10
    unsigned int first : 1;   // 0x14
    unsigned int m_isAA : 1;
    unsigned int m_isTrans : 1;
    unsigned int m_hasTexture : 1;
    unsigned int m_isFrameAlpha : 1;
    unsigned int m_socom2_bit5 : 1;
    unsigned int m_on : 1;
    unsigned int m_active_and_handling_input : 1;
    unsigned int m_unused : 24;
};   // 0x18 bytes

class C2DFade {
public:
    unsigned char m_fade_enable;   // 0x0
    float m_fade_dx;   // 0x4
    float m_fade_limit;   // 0x8
};   // 0xc bytes

/* the overrides as SOCOM II's vtable (0x406E80) has them */
class C2DBitmap : public C2D, public C2DFade {
public:
    C2DBitmap();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void SetTrans(float trans);
    virtual void Reset();
    virtual void GetAssetLibs(CLibList &liblist);
    void Load(float x, float y, float w, float h, zdb::CTexHandle *tex);
    void SetColor(float r, float g, float b);

    int m_x;   // 0x24
    int m_y;   // 0x28
    unsigned int m_prim;   // 0x2c: the GS PRIM bits the flag setters keep (SOCOM II's)
    float m_RGB[2][4];   // 0x30
    float m_NewUV[2][4];   // 0x50
    zdb::CTexHandle *m_pTexHandle;   // 0x70
    int m_iWidth;   // 0x74
    int m_iHeight;   // 0x78
} __attribute__((aligned(16)));   // 0x80 bytes

#endif
