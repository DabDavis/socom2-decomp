/* ztwod -- the 2D display objects (SOCOM 1's gamez/ztwod/ztwod.h, its DWARF's file): C2D, C2DFade,
 * C2DBitmap, C2DString, as their users see them. Their own code (OVL1 0x3630xx-0x365xxx) is not
 * written yet; the layout is SOCOM II's, which is not SOCOM 1's DWARF:
 *   - C2D is 0x18 bytes, not 0x1C: SOCOM 1's m_active_and_handling_input byte became the top
 *     bit of the flag byte at 0x14 (CanHandleInput reads bit 7 there), and C2DFade follows at
 *     0x18 (C2DBitmap's constructor zeroes its byte at 0x18 and sets its limit, 128, at 0x20);
 *   - its vtable has 26 slots, not 21: five new after On (slots 6-10). C2DBitmap's 6-9 set a
 *     flag bit and the matching GS PRIM bit in the word at 0x2C: bit 3 with TME (0x10), bit 1
 *     with AA1 (0x80), bit 2 with ABE (0x40), bit 5 with IIP (0x08) -- SOCOM 1's m_hasTexture,
 *     m_isAA and m_isTrans bits, and a new one. Slot 10 is empty in C2D. Their names are ours.
 * C2DBitmap is 16-aligned (its colour rows are VU0 vectors): SOCOM 1's CFader is 0xA0 bytes with
 * its last member ending at 0x98, CFlashFX 0xC0 with its at 0xB4. */
#ifndef ZTWOD_H
#define ZTWOD_H

#include <list>
#include <vector>

#include "../zmath/zmath.h"

namespace zdb {
class CAssetLib;
class CCamera;
class CTexHandle;
class CTexture;
class CTextureRelocMgr;
}
class CPad;
class CUIVariable;
class C2DAnimCmdHdr;
struct _zrdr;
struct _zanim_cmd_hdr;

class C2D;

int GetUICommandType(const char *name);

/* SOCOM 1's DWARF: `class C2Dlist : public list` (0xC bytes) -- a pointer list (CZHudMissionCams::
 * Init pushes its strings through __list_deleter<PCv>'s insert, 0x1FEAE0) */
class C2Dlist : public std::list<C2D *> {
public:
    typedef std::__list_deleter<const void *, std::allocator<const void *> > __deleter;

    /* an item put first: the base's insert at the first node (CHUD::Add2D's front insert reads
     * the end node's next_ as SOCOM 1's begin() did; the name is ours) */
    void push_front(C2D *const &x)
    {
        __deleter::insert(__deleter::iterator(this->end_node().next_), (const void *const &)x);
    }
};

class C2D : public C2Dlist {
public:
    class CLibList;

    C2D();
    virtual ~C2D();
    virtual void Draw(zdb::CCamera *cam);
    /* the small virtuals are inline (SOCOM 1's DWARF files them under ztwod.h): their out-of-line
     * copies come out wherever a 2D class's vtable does -- hud_main.cpp's are linked,
     * 0x1FDE10-0x1FE330 */
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr) {}
    virtual void Enable(bool on);
    virtual void Off();
    virtual void On();
    virtual void SetHasTexture(bool on) { m_hasTexture = on; }
    virtual void SetAA(bool on) { m_isAA = on; }
    virtual void SetIsTrans(bool on) { m_isTrans = on; }
    virtual void SetGouraud(bool on) { m_socom2_bit5 = on; }
    virtual void Slot10() {}
    virtual void SetTrans(float trans) {}
    virtual void Tick(float dT) {}
    virtual void Parse(_zrdr *rdr, zdb::CAssetLib *lib, zdb::CTextureRelocMgr *mgr) {}
    virtual void Reset() {}
    virtual bool HandlePad(const CPad *pad, int port) { return false; }
    virtual bool CanHandleInput() { return m_active_and_handling_input; }
    virtual void SetCanHandleInput(bool val) { m_active_and_handling_input = val; }
    virtual void SetActive() {}
    virtual void SetNormal() {}
    virtual void SetUseFrameBufferAlpha(bool isFrameAlpha) { m_isFrameAlpha = isFrameAlpha; }
    virtual void GetAssetLibs(CLibList &liblist);
    virtual C2DAnimCmdHdr *ParseZAnimCmd(_zrdr *rdr) { return 0; }
    virtual bool TickZAnimCmd(_zanim_cmd_hdr *cmd, float *t, bool *done) { return false; }
    /* the UI's command types (it passes no name: SOCOM II's copy loads a0 with zero) */
    virtual int GetZAnimCmdType(const char *name) { return GetUICommandType(0); }
    virtual void SetMapOffset(int x, int y) {}

    bool IsOn() const;
    /* the GS PRIM word for a primitive type with the flags' bits (ztwod.cpp; the name ours) */
    void MakePrim(unsigned int &prim, unsigned int type);
    /* CGame::StartEngine's: the screen's window, then the 2D camera (SOCOM 1's) */
    static void Init();
    static void Open();

    /* the screen's window in GS coordinates (SOCOM 1's statics: every 2D position is scaled by the
     * pixel sizes and moved by the top corner) and the 2D camera C2D::Open makes */
    static float m_fXPixel;
    static float m_fYPixel;
    static float m_topx;
    static float m_topy;
    static float m_botx;
    static float m_boty;
    static zdb::CCamera *m_p2dcamera;

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

/* the asset libraries a 2D object draws from (SOCOM 1's DWARF: `class CLibList : public vector`,
 * 0xC bytes, of CAssetLib pointers -- CHUD::Add2D reads each one's render phase) */
class C2D::CLibList : public std::vector<zdb::CAssetLib *> {
};

class C2DFont;

/* a text string: 0x98 bytes in SOCOM II (an array's stride; SOCOM 1's 0x88), its vtable 34 slots
 * (0x406DF0): C2D's 26, then SOCOM 1's eight with one new after SetString and the scaled Load
 * gone -- slot 29 now takes a scale and a float position and calls slot 30 with them as ints.
 * The parameter lists of 26, 27, 29 and 30 and the names of 26 and 27 are ours (slot 26 takes a
 * string and a flag that sends it through a lookup; 27 an int). SetPos stores two floats at
 * 0x90. Its members past C2D are not placed yet */
class C2DString : public C2D {
public:
    C2DString();
    virtual ~C2DString();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Off();
    virtual void On();
    virtual void SetTrans(float trans);
    virtual void Tick(float dT);
    virtual void SetString(const char *s, bool translate);
    virtual void SetStringLength(int len);
    virtual void Format(const char *fmt, ...);
    virtual void Load(float scale, const char *s, C2DFont *font, float x, float y, int flags);
    virtual void Load(const char *s, C2DFont *font, int x, int y, int flags);
    virtual void Load(const char *s, C2DFont *font, int x, int y, int w, int h);
    virtual float GetSpacing();
    virtual void ClearAndWipe();

    void SetPos(int x, int y);

    unsigned char _18[0x48 - 0x18];
    float m_color[4];   // 0x48 (SOCOM 1's CPnt4D at 0x50; the HUD writes it a float at a time)
    unsigned char _58[0x69 - 0x58];
    unsigned char m_socom2_69;   // 0x69: the HUD's spectator lines set it to 1
    unsigned char m_socom2_6a;   // 0x6a: PoseBitmap sets it for the network game's position
    unsigned char _6b[0x98 - 0x6b];
};   // 0x98 bytes

/* a fade's state (SOCOM 1's DWARF); its constructor inline: off, no step, the limit opaque */
class C2DFade {
public:
    C2DFade()
    {
        m_fade_enable = 0;
        m_fade_dx = 0.0f;
        m_fade_limit = 128.0f;
    }

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
    void Load(float x1, float y1, float x2, float y2, zdb::CTexHandle *tex);
    void Load(float x, float y, zdb::CTexHandle *tex);
    void MakePacket(zdb::CCamera *cam, zdb::CTextureRelocMgr *mgr, const CMatrix *m);
    void SetColor(float r, float g, float b);
    void SetColor(const CPnt3D &color);
    float *GetColor();
    float GetTrans();
    void SetPos(int x, int y);
    void SetPos(int x1, int y1, int x2, int y2);
    void TickFade();

    int m_x;   // 0x24
    int m_y;   // 0x28
    unsigned int m_prim;   // 0x2c: the GS PRIM bits the flag setters keep (SOCOM II's)
    float m_RGB[2][4];   // 0x30
    float m_NewUV[2][4];   // 0x50
    zdb::CTexHandle *m_pTexHandle;   // 0x70
    int m_iWidth;   // 0x74
    int m_iHeight;   // 0x78
} __attribute__((aligned(16)));   // 0x80 bytes

/* a textured triangle: 0xD0 bytes in SOCOM II as in SOCOM 1 (CZPlayerMapItem's arrays' stride),
 * its members past C2D not placed; its destructor is implicit (an out-of-line copy is emitted
 * wherever an array of them is destroyed: hud_main.cpp's is linked, 0x1FC620) */
class C2DPoly : public C2D {
public:
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void SetTrans(float trans);
    virtual void GetAssetLibs(CLibList &liblist);

    unsigned char _18[0xd0 - 0x18];
} __attribute__((aligned(16)));   // 0xd0 bytes

/* a bitmap of two triangles (SOCOM 1's DWARF; the offsets unchanged, the 16-alignment absorbing
 * C2D's lost 4 bytes); its destructor is implicit too */
class C2DBitmapPoly : public C2D, public C2DFade {
public:
    C2DBitmapPoly();
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void SetTrans(float trans);
    virtual void SetUseFrameBufferAlpha(bool isFrameAlpha);
    virtual void GetAssetLibs(CLibList &liblist);
    /* the corners, clockwise from the top left as MapCompass::SetPos passes them */
    void Load(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
              zdb::CTexture *tex);
    void SetColor(float r, float g, float b);
    void SetUV(float u0, float v0, float u1, float v1);

    int x1;   // 0x24
    int x2;   // 0x28
    int y1;   // 0x2c
    int y2;   // 0x30
    C2DPoly m_tris[2];   // 0x40
    zdb::CTexture *m_texture;   // 0x1e0
} __attribute__((aligned(16)));   // 0x1f0 bytes

/* a fan of triangles clipped out of a bitmap: 0x28 bytes in SOCOM II (SOCOM 1's 0x2C less C2D's
 * 4); its constructor and destructor are ztwod's own */
class C2DTriFan : public C2D {
public:
    C2DTriFan();
    virtual ~C2DTriFan();
    virtual void Draw(zdb::CCamera *cam);
    virtual void GetAssetLibs(CLibList &liblist);

    unsigned char m_polylist[0xc];   // 0x18: SOCOM 1's `vector m_polylist`
    zdb::CTexture *m_texture;   // 0x24
};   // 0x28 bytes

#endif
