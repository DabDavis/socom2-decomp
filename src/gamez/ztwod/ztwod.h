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

#include <eetypes.h>
#include <libvu0.h>

#include "../zmath/zmath.h"

namespace zdb {
class CAssetLib;
class CCamera;
class CTexHandle;
class CTexture;

/* the texture relocation managers (SOCOM 1's; the slots as CBatchRelocator's vtable 0x406C10 holds
 * them): a batch relocator is only its vtable pointer, all its state the texture manager's */
class CTextureRelocMgr {
public:
    virtual void AddToBatch(CTexHandle *tex) const;
    virtual void EndBatchReloc() const;
    virtual void PreGetSprPacket(CTexHandle *tex);
    virtual void StartBatchReloc() const;
    virtual void TakeDrawPacket(u_long128 *packet, unsigned int qwc);
    virtual void ClearAll();
};

class CBatchRelocator : public CTextureRelocMgr {
public:
    virtual void AddToBatch(CTexHandle *tex) const;
    virtual void EndBatchReloc() const;
    virtual void StartBatchReloc() const;
    virtual void TakeDrawPacket(u_long128 *packet, unsigned int qwc);
    virtual void ClearAll();
};
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
    /* the children's libraries (inline, as SOCOM 1's DWARF files it: hud_main.cpp's copy is linked,
     * 0x1F6FD0) */
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

inline void C2D::GetAssetLibs(CLibList &liblist)
{
    iterator e = end();

    for (iterator i = begin(); i != e; ++i)
        (*i)->GetAssetLibs(liblist);
}

class C2DFont;

/* a string's kind (SOCOM 1's tag_string_type, a byte): 1 pulses its alpha (Tick) */
enum tag_string_type {
    STRING_NORMAL = 0,
    STRING_PULSE = 1,
};

/* a text string: 0x98 bytes in SOCOM II (an array's stride; SOCOM 1's 0x88), laid out from its
 * constructor against SOCOM 1's DWARF -- the members SOCOM 1 has, four bytes earlier with C2D, but
 * SOCOM 1's m_length and m_height gone and words of SOCOM II's own after m_numChars, at 0x6A and
 * from 0x78; the position two floats at 0x90 (SOCOM 1's IPNT2D at 0x80). Its vtable has 34 slots
 * (0x406DF0): C2D's 26, then SOCOM 1's eight with SOCOM II's changes -- slot 27 sets a string by
 * its id in the string table, slot 29 takes a scale and a float position and calls slot 30 with
 * them as ints, and the Loads pass SetString's flag on. The parameter lists of 26, 27 and 29-31
 * and the name of 27 are ours (26 takes a string and a flag that sends it through a lookup) */
class C2DString : public C2D {
public:
    C2DString();
    virtual ~C2DString();
    virtual void Draw(zdb::CCamera *cam);
    virtual void Draw(const CMatrix &m, zdb::CTextureRelocMgr *mgr);
    virtual void Off();
    virtual void On();
    /* inline, as SOCOM 1's DWARF files it (hud_main.cpp's copy is linked, 0x1F9150) */
    virtual void SetTrans(float trans) { m_color[3] = trans; }
    virtual void Tick(float dT);
    virtual void SetString(const char *s, bool translate);
    virtual void SetStringId(int id);
    virtual void Format(const char *fmt, ...);
    virtual void Load(float scale, const char *s, C2DFont *font, float x, float y, bool translate);
    virtual void Load(const char *s, C2DFont *font, int x, int y, bool translate);
    virtual void Load(const char *s, C2DFont *font, int x, int y, int w, int h, bool translate);
    virtual float GetSpacing();
    virtual void ClearAndWipe();

    void SetFont(C2DFont *font);
    void DeleteCharacter();
    void NewLine();
    void AddCharacter(char c);
    void SetPos(int x, int y);
    void Draw(zdb::CTextureRelocMgr *mgr, zdb::CCamera *cam, int depth);
    void MakePacket(zdb::CTextureRelocMgr *mgr, int depth);

    C2DFont *m_font;   // 0x18
    char *m_string;   // 0x1c
    int m_stringlen_max;   // 0x20
    int m_numChars;   // 0x24
    int m_socom2_28;   // 0x28
    char *m_Ustring;   // 0x2c
    int m_Ustringlen_max;   // 0x30
    int m_numUChars;   // 0x34
    float m_charwidth;   // 0x38
    float m_charheight;   // 0x3c
    float m_scale;   // 0x40
    int m_numLines;   // 0x44
    float m_color[4];   // 0x48 (SOCOM 1's CPnt4D at 0x50; the HUD writes it a float at a time)
    int m_revnum;   // 0x58
    float m_fadeTime;   // 0x5c
    float m_fadeMax;   // 0x60
    float m_fadeMin;   // 0x64
    tag_string_type m_Type;   // 0x68
    unsigned char m_isCentered;   // 0x69: the HUD's spectator lines set it
    unsigned char m_socom2_6a;   // 0x6a: PoseBitmap sets it for the network game's position
    unsigned int m_firstCharPos;   // 0x6c
    int m_shortLength;   // 0x70
    float m_elapsedTime;   // 0x74
    float m_socom2_78;   // 0x78: a second scale (1; slot 27 takes it from the string table)
    float m_socom2_7c;   // 0x7c: added to the position (WordIntersectionTest reads both as floats)
    float m_socom2_80;   // 0x80
    int m_stringid;   // 0x84: the id slot 27 looked up
    int m_stringrev;   // 0x88: the string table's revision then (the draws look it up again on a change)
    unsigned char m_socom2_8c;   // 0x8c
    float m_posx;   // 0x90
    float m_posy;   // 0x94
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

/* a textured triangle: 0xD0 bytes in SOCOM II as in SOCOM 1 (CZPlayerMapItem's arrays' stride), the
 * arrays where SOCOM 1's DWARF has them; m_x and m_y moved up with C2D and SOCOM II's PRIM word in
 * the gap after them (the flag setters, slots 6-9, keep its bits as C2DBitmap's do). Its destructor
 * is implicit (an out-of-line copy is emitted wherever an array of them is destroyed: hud_main.cpp's
 * is linked, 0x1FC620) */
class C2DPoly : public C2D {
public:
    C2DPoly();
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void SetTrans(float trans);
    virtual void GetAssetLibs(CLibList &liblist);
    /* one corner's alpha (SOCOM II's: past the last, the last; Gouraud then, so it shades) */
    void SetTrans(unsigned int vertex, float trans);
    void SetPos(float x0, float y0, float x1, float y1, float x2, float y2);
    void SetUV(float u0, float v0, float u1, float v1, float u2, float v2);
    void Load(float x0, float y0, float x1, float y1, float x2, float y2, zdb::CTexture *tex);
    float GetTrans();
    CPnt3D GetColor();
    void SetColor(float r, float g, float b);
    void MakePacket(zdb::CCamera *cam);

    int m_x;   // 0x18
    int m_y;   // 0x1c
    unsigned int m_prim;   // 0x20
    sceVu0FVECTOR m_RGB[3];   // 0x30 (SOCOM 1's DWARF: float[3][4], 16-aligned)
    sceVu0FVECTOR m_NewUV[3];   // 0x60
    sceVu0FVECTOR m_vertex[3];   // 0x90
    zdb::CTexture *m_texture;   // 0xc0
} __attribute__((aligned(16)));   // 0xd0 bytes

/* a bitmap of two triangles (SOCOM 1's DWARF; the offsets unchanged, the 16-alignment absorbing
 * C2D's lost 4 bytes), the first the top left, bottom left and bottom right corners, the second
 * the top left, bottom right and top right. The flag setters are inline: they pass the flag on to
 * both triangles (their out-of-line copies come with the vtable: c2dbitmap_poly.cpp's SetAA and
 * SetGouraud are linked, 0x357E90 and 0x357EE0, the CZPauseTest unit's SetHasTexture and
 * SetIsTrans, 0x204190 and 0x204140). Its destructor is implicit too */
class C2DBitmapPoly : public C2D, public C2DFade {
public:
    C2DBitmapPoly();
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetHasTexture(bool on)
    {
        m_tris[0].SetHasTexture(on);
        m_tris[1].SetHasTexture(on);
    }
    virtual void SetAA(bool on)
    {
        m_tris[0].SetAA(on);
        m_tris[1].SetAA(on);
    }
    virtual void SetIsTrans(bool on)
    {
        m_tris[0].SetIsTrans(on);
        m_tris[1].SetIsTrans(on);
    }
    virtual void SetGouraud(bool on)
    {
        m_tris[0].SetGouraud(on);
        m_tris[1].SetGouraud(on);
    }
    virtual void SetTrans(float trans);
    virtual void SetUseFrameBufferAlpha(bool isFrameAlpha);
    virtual void GetAssetLibs(CLibList &liblist);
    /* the corners, counter-clockwise from the top left */
    void Load(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4,
              zdb::CTexture *tex);
    void SetPos(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
    /* a rectangle at a point (SOCOM II's; the name ours) */
    void SetPos(float x, float y, float w, float h);
    void SetCenterPos(PNT2D center, float w, float h);
    void SetUV(float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4);
    void SetUV(float u0, float v0, float u1, float v1);
    void SetColor(float r, float g, float b);
    /* one corner's alpha (SOCOM II's; the name ours) */
    void SetTrans(int corner, float trans);
    float GetTrans();
    void FlipImage_Xaxis();
    void FlipImage_Yaxis();
    void FlipImage_XYaxis();
    void Rotate_Zaxis(float s);
    void Rotate_Zaxis(float s, float c);

    int x1;   // 0x24
    int x2;   // 0x28
    int y1;   // 0x2c
    int y2;   // 0x30
    C2DPoly m_tris[2];   // 0x40
    zdb::CTexture *m_texture;   // 0x1e0
} __attribute__((aligned(16)));   // 0x1f0 bytes

/* a textured line (SOCOM 1's DWARF; m_x and m_y moved up with C2D, SOCOM II's PRIM word at 0x20) */
class C2DLine : public C2D {
public:
    C2DLine();
    virtual void Draw(zdb::CCamera *cam);
    virtual void SetHasTexture(bool on);
    virtual void SetAA(bool on);
    virtual void SetIsTrans(bool on);
    virtual void SetGouraud(bool on);
    virtual void SetTrans(float trans);
    virtual void GetAssetLibs(CLibList &liblist);
    void Load(float x1, float y1, float x2, float y2, zdb::CTexture *tex);
    void SetPos(int x1, int y1, int x2, int y2);
    void SetUV(float u1, float v1, float u2, float v2);
    void SetColor(float r, float g, float b);
    void MakePacket(zdb::CCamera *cam);
    /* the vertices, colours and UVs set from or filled into the clipper's rows (SOCOM 1's
     * SetClipInfo took a count as well) */
    void SetClipInfo(float (*vertex)[4], float (*rgb)[4], float (*uv)[4]);
    void FillClipInfo(float (*vertex)[4], float (*rgb)[4], float (*uv)[4]);

    int m_x;   // 0x18
    int m_y;   // 0x1c
    unsigned int m_prim;   // 0x20
    sceVu0FVECTOR m_vertex[2];   // 0x30
    sceVu0FVECTOR m_RGB[2];   // 0x50
    sceVu0FVECTOR m_NewUV[2];   // 0x70
    zdb::CTexture *m_texture;   // 0x90
    bool do3D;   // 0x94
} __attribute__((aligned(16)));   // 0xa0 bytes

/* a fan of triangles clipped out of a bitmap: 0x28 bytes in SOCOM II (SOCOM 1's 0x2C less C2D's
 * 4); its constructor and destructor are ztwod's own */
class C2DTriFan : public C2D {
public:
    C2DTriFan();
    virtual ~C2DTriFan();
    virtual void Draw(zdb::CCamera *cam);
    virtual void GetAssetLibs(CLibList &liblist);

    std::vector<C2DPoly> m_polylist;   // 0x18
    zdb::CTexture *m_texture;   // 0x24
};   // 0x28 bytes

#endif
