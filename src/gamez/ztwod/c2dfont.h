/* c2dfont.h -- SOCOM II's C2DFont as its own constructor and destructor build it (ours: the header's
 * name, the glyph types' names and every member name SOCOM 1's DWARF does not give). SOCOM II
 * rewrote SOCOM 1's font (a vtable and a vector<C2DFontEntry>, 0x30 bytes): it is 0x64 bytes, with
 * no vtable --
 *   - 0x0 a glyph map keyed by the character's code: std::map<unsigned short, C2DGlyph>. The map's
 *     default constructor builds a comparator and an allocator temporary and calls the tree's
 *     out-of-class constructor (OVL1 0x1FEAC0); its out-of-line map() is 0x1FB3F0. The value is
 *     16-aligned: the tree's destroy (0x1FE760) destroys the pair at node + 0x10, the font's find
 *     (0x35C630) compares an unsigned short at node + 0x10, and insert_node_at (0x35D240) news
 *     0x50-byte nodes and copies a 0x40-byte pair into node + 0x10 -- so the glyph is 0x30 bytes at
 *     pair + 0x10. The glyph: its code (a halfword, 0x3599D0 stores it with sh), a class vector of
 *     0x14-byte pieces at 4 (cleared through the deleter's clear 0x1FEC50, filled by 0x359820),
 *     halfwords from 0x10 (0x10 set to -1), a byte at 0x1A, and something 16-aligned at 0x20;
 *   - 0x10 the textures, a vector<zdb::CTexHandle *> (0x1FB3D0, 0x1FB300; the unload FreeVectors it);
 *   - 0x1C-0x57 scalars: SOCOM 1's m_displaytop, m_displaybottom, XPadding, m_avgWidth, m_opacity and
 *     m_scale_factor in SOCOM 1's order from 0x20 (hud_teammatenames.cpp reads the first two,
 *     c2dstring.cpp the last three), a pointer at 0x38 to the loaded faces (0x684 bytes: 51 0x20-byte
 *     records and three members, deleted by the unload), a byte at 0x3C, two words, a CPnt3D at 0x48
 *     (CHUD::Init's inlined new C2DFont calls its constructor 0x1FB3C0 on font + 0x48), a float at
 *     0x54;
 *   - 0x58 the name, a CSaferStr (its implicit destructor calls the string's with 0, a base's).
 * The constructor builds the members and then resets the scalars through an inline (0x1FB370 is its
 * out-of-line copy, which CHUD::Init and CRenderableString::LoadSpec call: their new C2DFont inlines
 * the constructor one level); the unload (0x35A650) ends with the same reset. The destructor calls
 * the unload, then destroys the name, the textures and the map. CSkbFonts() (0x38E550) is
 * `__construct_array(this, C2DFont(), ~C2DFont(), 0x64, 3)`: three fonts. Byte-exact as a probe
 * (build/scratch/hud/font/probe.cpp): the constructor (0x38E590, the soft keyboard's copy) and the
 * destructor (0x369EB0, zui.cpp's copy), once their two calls' rows carry the tree's names.
 *
 * Include it after a CSaferStr with an inline default constructor (fts_state.h's or zui_spec.h's):
 * the units that use the font define their own. */
#ifndef C2DFONT_H
#define C2DFONT_H

#include <libvu0.h>
#include <map>
#include <vector>

#include "../zmath/zmath.h"

namespace zdb {
class CTexHandle;
}
struct _zrdr;

/* where a piece sits in its character cell and how it is scaled (ours: c2dfont.cpp's ReadPlacement
 * reads it from a DispPos point and the HorScale/VertScale reals, y negated; default unscaled) */
class C2DPlacement {
public:
    C2DPlacement() : x(0), y(0), sx(1.0f), sy(1.0f) {}

    short x;   // 0x0
    short y;   // 0x2
    float sx;   // 0x4
    float sy;   // 0x8
};   // 0xc bytes

/* a piece of a glyph (ours): its rectangle in the texture -- left, width, and the top and bottom
 * rows counted up from the texture's last (c2dfont.cpp fills one from a CFontRect) -- and its
 * placement (0x359820/0x35C2C0 fill one, 0x35A7B0 measures them) */
class C2DGlyphPiece {
public:
    C2DGlyphPiece() : m_0(0), m_2(0), m_4(0), m_6(0) {}

    short m_0;   // 0x0
    short m_2;   // 0x2
    short m_4;   // 0x4
    short m_6;   // 0x6
    C2DPlacement m_place;   // 0x8
};   // 0x14 bytes

/* a glyph's colour, four ints (ours: LoadChar stores a PNT3D's components converted and 128;
 * copied whole, word by word through the float registers). The type is 4-aligned and the member
 * 16-aligned (the MSL lane's round 17: the image's 4D copies take three shapes -- this one, a
 * 4-aligned block at a 16-aligned member, makes the pair's copy 0x35D370 byte-exact) */
struct C2DGlyphColor {
    int r, g, b, a;
};

/* a character's glyph (ours): its code, its pieces, a fixed advance (8 for the space, else -1),
 * the baseline and the left and right kerning, the font texture's index, whether it has a colour
 * of its own, and that colour */
class C2DGlyph {
public:
    C2DGlyph() : m_code(0), m_10(-1), m_12(0), m_14(0), m_16(0), m_18(0), m_1a(0) {}

    /* the widest piece's right edge (c2dfont.cpp, 0x35A7B0: C2DString's layout calls it) */
    int GetWidth();
    /* a piece from two rdr points, then its placement (c2dfont.cpp, 0x35C2C0 and 0x35BD80) */
    C2DGlyphPiece *AddPiece(const _zrdr *ul, const _zrdr *lr, int height);
    void LoadElems(const _zrdr *rdr, int height);

    unsigned short m_code;   // 0x0
    std::vector<C2DGlyphPiece> m_pieces;   // 0x4
    short m_10;   // 0x10
    short m_12;   // 0x12
    short m_14;   // 0x14
    short m_16;   // 0x16
    short m_18;   // 0x18
    unsigned char m_1a;   // 0x1a
    C2DGlyphColor m_20 __attribute__((aligned(16)));   // 0x20
};   // 0x30 bytes

/* what GetGlyph hands back: a glyph of the font's, or one composed for the call and owned here
 * (ours: C2DString's layout builds one on the stack, both words zeroed, and deletes the owned) */
class C2DGlyphRef {
public:
    C2DGlyphRef() : m_glyph(0), m_owned(0) {}
    /* the owned glyph freed (no store: c2dstring.cpp's GetCharWidth deletes it and nothing more) */
    ~C2DGlyphRef() { delete m_owned; }

    /* the owned glyph freed (c2dfont.cpp's GetGlyph does it before every answer) */
    void Release()
    {
        delete m_owned;
        m_owned = 0;
    }
    /* the glyph handed back, the font's or the owned (const: its reads fold into the frame's offsets,
     * where a non-const accessor shares the address register the constructor and destructor keep for
     * the owned word -- C2DString's GetCharWidth and AddCharacter) */
    C2DGlyph *Get() const { return m_glyph ? m_glyph : m_owned; }
    /* a glyph of the font's handed out */
    void Set(C2DGlyph *g)
    {
        Release();
        m_glyph = g;
    }
    /* a glyph made for the call handed out, owned */
    void Own(C2DGlyph *g)
    {
        Release();
        m_owned = g;
    }

    C2DGlyph *m_glyph;   // 0x0
    C2DGlyph *m_owned;   // 0x4
};

class CHangulFont;


class C2DFont {
public:
    C2DFont() { Reset(); }
    ~C2DFont() { Unload(); }

    /* c2dfont.cpp's (OVL1 0x359740-0x35F8EF; the names ours): the faces rdr's entry by name -- the
     * textures, the scales, the glow texture, the characters, the Korean composer and the drop
     * shadow -- the glyph for a character, and the unload */
    void Load(const _zrdr *rdr, const char *name);
    void GetGlyph(C2DGlyphRef *ref, unsigned short code);
    void Unload();
    int LoadChar(const _zrdr *rdr);

    /* a font texture by index, none out of range (inline: the loaders read a texture's height; one
     * ternary over both tests -- the HUD's round 55: two early returns, nested ternaries and an
     * if/else per test all give an `index < 0` arm of its own, 8 bytes longer in Compose and
     * LoadChar) */
    zdb::CTexHandle *GetTexture(int i)
    {
        return i >= 0 && (unsigned int)i < m_textures.size() ? m_textures[i] : 0;
    }

    /* the font's scale (inline: c2dstring.cpp's Load multiplies by it last) */
    float GetScaleFactor() { return m_scale_factor; }

    /* the scalars zeroed, the opacity and the two scales 1 (stored in this order: the target's) */
    void Reset()
    {
        m_displaytop = 0;
        m_displaybottom = 0;
        m_scale_factor = 1.0f;
        m_opacity = 1.0f;
        m_1c = 0;
        m_avgWidth = 0.0f;
        m_faces = 0;
        XPadding = 0;
        m_3c = 0;
        m_40 = 0;
        m_44 = 0;
        m_48.x = 0.0f;
        m_48.y = 0.0f;
        m_48.z = 0.0f;
        m_54 = 1.0f;
    }

    std::map<unsigned short, C2DGlyph> m_glyphs;   // 0x0
    std::vector<zdb::CTexHandle *> m_textures;   // 0x10
    zdb::CTexHandle *m_1c;   // 0x1c: the glow texture
    int m_displaytop;   // 0x20
    int m_displaybottom;   // 0x24
    /* unsigned in SOCOM II (SOCOM 1's DWARF: int): C2DString's GetCharWidth and UpdatePos add it
     * after the sum they add it to, AddCharacter converts it ahead of the kerning it adds */
    unsigned int XPadding;   // 0x28
    float m_avgWidth;   // 0x2c
    float m_opacity;   // 0x30
    float m_scale_factor;   // 0x34
    CHangulFont *m_faces;   // 0x38
    bool m_3c;   // 0x3c
    int m_40;   // 0x40
    int m_44;   // 0x44
    CPnt3D m_48;   // 0x48
    float m_54;   // 0x54
    CSaferStr m_name;   // 0x58
};   // 0x64 bytes

#endif
