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

/* a piece of a glyph (ours: six halfwords and two floats, as 0x359820 fills one) */
class C2DGlyphPiece {
public:
    short m_0;   // 0x0
    short m_2;   // 0x2
    short m_4;   // 0x4
    short m_6;   // 0x6
    short m_8;   // 0x8
    short m_a;   // 0xa
    float m_c;   // 0xc
    float m_10;   // 0x10
};   // 0x14 bytes

/* a character's glyph (ours) */
class C2DGlyph {
public:
    unsigned short m_code;   // 0x0
    std::vector<C2DGlyphPiece> m_pieces;   // 0x4
    short m_10;   // 0x10
    short m_12;   // 0x12
    short m_14;   // 0x14
    short m_16;   // 0x16
    short m_18;   // 0x18
    unsigned char m_1a;   // 0x1a
    sceVu0FVECTOR m_20;   // 0x20
};   // 0x30 bytes

/* the font's unload (OVL1 0x35A650, the font unit's; no row named yet) */
class C2DFont;
extern "C" void func_0035A650(C2DFont *font);

class C2DFont {
public:
    C2DFont() { Reset(); }
    ~C2DFont() { func_0035A650(this); }

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
    int m_1c;   // 0x1c
    int m_displaytop;   // 0x20
    int m_displaybottom;   // 0x24
    int XPadding;   // 0x28
    float m_avgWidth;   // 0x2c
    float m_opacity;   // 0x30
    float m_scale_factor;   // 0x34
    void *m_faces;   // 0x38
    bool m_3c;   // 0x3c
    int m_40;   // 0x40
    int m_44;   // 0x44
    CPnt3D m_48;   // 0x48
    float m_54;   // 0x54
    CSaferStr m_name;   // 0x58
};   // 0x64 bytes

#endif
