/* c2dfont.cpp -- SOCOM II's font (OVL1 0x359820-0x35F8EF, the name ours): C2DFont's loader, its
 * glyph lookup and unload, the glyphs' pieces, and the Korean composer -- Hangul syllables (U+AC00-
 * U+D7A3) built at run time from 51 jamo drawn in the font texture, placed by the rdr's layout
 * tables (the faces rdr's "Korean" block: Jamo, CharLayouts, VowelRangeLayouts, LeadRangeLayouts,
 * LeadLayoutUsage). No SOCOM 1 counterpart: SOCOM 1's font was a vector of entries (README "Tells
 * from the HUD, round 12"). Its data opens 0x3F84F0 (the jamo tables, their names, the strings in
 * generation order). The unit's tail holds the glyph map's, the alternates' map's and the layout
 * vectors' template instances from 0x35C3A0. Defined in reverse of their image order (-inline
 * deferred).
 *
 * Byte-exact (the HUD lane, round 28): Unload, GetWidth, ReadRect, LoadElems, LoadLeadUsage,
 * AddPiece, CJamoGlyph's and CJamoPlace's constructors and destructors, the point reader's pieces
 * (0x35A140-0x35A230), and in the tail both maps' find, find_or_insert<int>, insert_node_at, clear
 * and destroy pieces, every resize but none of the inserts, three erases, the range init, the
 * deleters' destructors and clears. What settled them: the pieces are written through a reverse
 * iterator over the glyph's pieces (MSL's reverse_iterator shape: end() held, `current - 1` at
 * every access, the iterator living in the frame -- CRevIter below; include/vector has no
 * rbegin()); the table rows are arrays of a -1 index class, so their constructors are element
 * loops MW does not unroll; ReadPoint is cut to one level (GetInt's chain one inline deeper).
 * Open in the tail: find_or_insert<unsigned short> (0x35C3A0: the target copies a C2DGlyph
 * temporary into the pair through the range init and destroys both inline), the glyph tree's
 * swap (include/tree's swap, OPEN there too), the pair's and the piece vector's copy constructors
 * (0x35D370, 0x35D420: the target calls begin(), end(), the deleter's constructor and the
 * allocator out of line -- 0x35D490-0x35D4E0 -- where include/vector inlines them), the four
 * inserts and the layout vector's erase (0x35E6D0, CJamoPlace's copy assignment, out of line
 * there). Not emitted: FlatFindTag's copy 0x35A240 and the composer's member constructors
 * 0x35A250-0x35A2B0 (the target inlines CHangulFont() one level into Load, calling them; here it
 * inlines whole). 0x359740 (a list<const void *> piece only __list_imp::resize 0x3C0B14 calls) is
 * left out: whose tail it is is not settled.
 *
 * Round 55 (the HUD lane): LoadJamo, LoadVowelRangeLayouts, CHangulFont::Load, Compose and LoadChar
 * byte-exact -- the rectangle's corners a class (its copies go through $f0), ReadRect static, the
 * strings and ints read through FlatFindString / FindIntAt (inline results, numbered after the
 * caller's named locals; FindIntAt's default stored just ahead of each read), GetTexture one
 * ternary over both tests, the item type through Type(). Still open: LoadLeadRangeLayouts and
 * LoadCharLayouts (registers), AddJamo (2), GetGlyph (the frame).
 * The MSL headers' round 40 (THE LEVEL, this file's end): C2DFont::Load (CHangulFont() cut to its own body, its members' constructors called, the
 * tag assigned in its test, the handle a named local, FontReadInt's locals' frame slots after the named ones), the layout vectors' insert and erase. */

/* (this file's end: THE LEVEL) */
#define __MSL_POD_DTOR_ARGS
#include <map>
#include <string>
#include <vector>

#include "../zrdr/zrdr.h"
#include "../zsys/zsys.h"

/* SOCOM 1's CSaferStr (zutil.h: never null, a null text empties it) -- the font's name, and the
 * Korean loader's name lists */
class CSaferStr : public std::string {
public:
    CSaferStr() {}
    CSaferStr(const char *sz) { *this = sz; }
    void operator=(const char *sz)
    {
        if (sz)
            assign(sz);
        else
            erase();
    }
};

#include "c2dfont.h"

/* the texture classes as the font reads them: a handle's texture, its height */
namespace zdb {
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

class CTexHandle {
public:
    unsigned char _0[0xc];
    CTexture *m_texture;   // 0xc
};

class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
};

class CAssetMgr {
public:
    static CAssetList m_assets;
};
}

extern "C" int strcasecmp(const char *a, const char *b);

/* the jamo, Unicode's compatibility letters U+3131-U+3163 in order (ours; the rdr names them as
 * s_hangulJamoNames does): thirty consonants, then twenty-one vowels */
enum JAMO {
    JAMO_KIYEOK, JAMO_SSANGKIYEOK, JAMO_KIYEOKSIOS, JAMO_NIEUN, JAMO_NIEUNCIEUC, JAMO_NIEUNHIEUH,
    JAMO_TIKEUT, JAMO_SSANGTIKEUT, JAMO_RIEUL, JAMO_RIEULKIYEOK, JAMO_RIEULMIEUM, JAMO_RIEULPIEUP,
    JAMO_RIEULSIOS, JAMO_RIEULTHIEUTH, JAMO_RIEULPHIEUPH, JAMO_RIEULHIEUH, JAMO_MIEUM, JAMO_PIEUP,
    JAMO_SSANGPIEUP, JAMO_PIEUPSIOS, JAMO_SIOS, JAMO_SSANGSIOS, JAMO_IEUNG, JAMO_CIEUC,
    JAMO_SSANGCIEUC, JAMO_CHIEUCH, JAMO_KHIEUKH, JAMO_THIEUTH, JAMO_PHIEUPH, JAMO_HIEUH,
    JAMO_A, JAMO_AE, JAMO_YA, JAMO_YAE, JAMO_EO, JAMO_E, JAMO_YEO, JAMO_YE, JAMO_O, JAMO_WA,
    JAMO_WAE, JAMO_OE, JAMO_YO, JAMO_U, JAMO_WEO, JAMO_WE, JAMO_WI, JAMO_YU, JAMO_EU, JAMO_YI,
    JAMO_I,
    JAMO_COUNT
};

/* a syllable's lead consonants, vowels and trailing consonants as jamo, in Unicode's order
 * (0x3F84F0, 0x3F8510, 0x3F8530) */
static JAMO s_hangulLeads[19] = {
    JAMO_KIYEOK, JAMO_SSANGKIYEOK, JAMO_NIEUN, JAMO_TIKEUT, JAMO_SSANGTIKEUT, JAMO_RIEUL,
    JAMO_MIEUM, JAMO_PIEUP, JAMO_SSANGPIEUP, JAMO_SIOS, JAMO_SSANGSIOS, JAMO_IEUNG, JAMO_CIEUC,
    JAMO_SSANGCIEUC, JAMO_CHIEUCH, JAMO_KHIEUKH, JAMO_THIEUTH, JAMO_PHIEUPH, JAMO_HIEUH
};
static JAMO s_hangulVowels[21] = {
    JAMO_A, JAMO_AE, JAMO_YA, JAMO_YAE, JAMO_EO, JAMO_E, JAMO_YEO, JAMO_YE, JAMO_O, JAMO_WA,
    JAMO_WAE, JAMO_OE, JAMO_YO, JAMO_U, JAMO_WEO, JAMO_WE, JAMO_WI, JAMO_YU, JAMO_EU, JAMO_YI,
    JAMO_I
};
static JAMO s_hangulTrails[27] = {
    JAMO_KIYEOK, JAMO_SSANGKIYEOK, JAMO_KIYEOKSIOS, JAMO_NIEUN, JAMO_NIEUNCIEUC, JAMO_NIEUNHIEUH,
    JAMO_TIKEUT, JAMO_RIEUL, JAMO_RIEULKIYEOK, JAMO_RIEULMIEUM, JAMO_RIEULPIEUP, JAMO_RIEULSIOS,
    JAMO_RIEULTHIEUTH, JAMO_RIEULPHIEUPH, JAMO_RIEULHIEUH, JAMO_MIEUM, JAMO_PIEUP, JAMO_PIEUPSIOS,
    JAMO_SIOS, JAMO_SSANGSIOS, JAMO_IEUNG, JAMO_CIEUC, JAMO_CHIEUCH, JAMO_KHIEUKH, JAMO_THIEUTH,
    JAMO_PHIEUPH, JAMO_HIEUH
};

/* the jamo's names in the rdr (0x3F8770) */
static const char *s_hangulJamoNames[JAMO_COUNT] = {
    "Kiyeok", "SsangKiyeok", "KiyeokSios", "Nieun", "NieunCieuc", "NieunHieuh", "Tikeut",
    "SsangTikeut", "Rieul", "RieulKiyeok", "RieulMieum", "RieulPieup", "RieulSios", "RieulThieuth",
    "RieulPhieuph", "RieulHieuh", "Mieum", "Pieup", "SsangPieup", "PieupSios", "Sios", "SsangSios",
    "Ieung", "Cieuc", "SsangCieuc", "Chieuch", "Khieukh", "Thieuth", "Phieuph", "Hieuh",
    "A", "Ae", "Ya", "Yae", "Eo", "E", "Yeo", "Ye", "O", "Wa", "Wae", "Oe", "Yo", "U", "Weo", "We",
    "Wi", "Yu", "Eu", "Yi", "I"
};

/* a layout's three parts (0x3F8858) */
static const char *s_hangulParts[3] = { "Lead", "Vowel", "Trail" };

/* a character code handed by value (ours: a halfword struct -- the composer's caller copies the code
 * into a temporary with lhu/sh and passes it with lh, the composer homes it and reads it with lhu) */
struct C2DChar {
    C2DChar() {}
    C2DChar(unsigned short c) : m_code(c) {}

    unsigned short m_code;
};

/* a jamo's rectangle in the font texture (ours): the upper left and lower right corners */
struct CFontCoord {
    CFontCoord(int v) : m_v(v) {}
    operator int() const { return m_v; }

    int m_v;
};

struct CFontRect {
    CFontRect() : left(0), top(0), right(0), bottom(0) {}
    CFontRect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}

    CFontCoord left;   // 0x0
    CFontCoord top;   // 0x4
    CFontCoord right;   // 0x8
    CFontCoord bottom;   // 0xc
};

/* a jamo: its rectangle, and the rectangles it takes in particular layouts (ours: 0x20 bytes, the
 * composer's table of 51) */
class CJamoGlyph {
public:
    CJamoGlyph() {}

    /* the rectangle for a layout id, if the jamo has one */
    CFontRect *FindAlt(const int &key)
    {
        std::map<int, CFontRect>::iterator f = m_alts.find(key);

        if (f != m_alts.end())
            return &(*f).second;
        return 0;
    }

    CFontRect m_rect;   // 0x0
    std::map<int, CFontRect> m_alts;   // 0x10
};

/* where one jamo of a syllable goes, and the alternates' layout ids it tries first (ours) */
class CJamoPlace {
public:
    C2DPlacement m_place;   // 0x0
    std::vector<int> m_keys;   // 0xc
};   // 0x18 bytes

/* a syllable's layout: its lead, vowel and trail (ours; the rdr's CharLayouts) */
class CLayout {
public:
    CJamoPlace *GetPart(int i) { return &m_parts[i]; }

    CJamoPlace m_parts[3];   // 0x0
};   // 0x48 bytes

/* a table entry: a row or layout index, none until read (ours: the lead table's out-of-line
 * constructor 0x35A260 stores -1; the rows below construct theirs in place) */
class CJamoIndex {
public:
    CJamoIndex() : m_v(-1) {}

    short m_v;
};

/* the layout for each trailing consonant, by the vowel's row (ours; VowelRangeLayouts) */
class CTrailRow {
public:
    CJamoIndex m_noTrail;   // 0x0
    CJamoIndex m_trails[27];   // 0x2
};   // 0x38 bytes

/* the vowel row for each vowel, by the lead's row (ours; LeadRangeLayouts) */
class CVowelRow {
public:
    CJamoIndex m_vowels[21];   // 0x0
};   // 0x2a bytes

/* an int the composer reads from the rdr (ours) */
class CFontInt {
public:
    CFontInt() : m_v(0) {}

    int m_v;
};

/* the Korean composer (ours: 0x6BC bytes, the font's m_faces) */
class CHangulFont {
public:
    void AddJamo(C2DGlyph *glyph, int part, JAMO which, CLayout *layout, int height);
    bool Compose(C2DGlyph *glyph, C2DChar code, C2DFont *font);
    void Load(const _zrdr *rdr);
    void LoadCharLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &jamoNames);
    void LoadJamo(std::vector<CSaferStr> &names, const _zrdr *rdr, JAMO which);
    void LoadLeadUsage(const _zrdr *rdr, const std::vector<CSaferStr> &names);
    void LoadLeadRangeLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &vowelNames);
    void LoadVowelRangeLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &layoutNames);

    CJamoGlyph *GetJamo(JAMO which) { return &m_jamo[which]; }

    CJamoGlyph m_jamo[JAMO_COUNT];   // 0x0
    std::vector<CLayout> m_layouts;   // 0x660
    std::vector<CTrailRow> m_trailRows;   // 0x66c
    std::vector<CVowelRow> m_vowelRows;   // 0x678
    CJamoIndex m_leadRows[19];   // 0x684
    CFontInt m_texture;   // 0x6ac
    CFontInt m_baseline;   // 0x6b0
    CFontInt m_lKerning;   // 0x6b4
    CFontInt m_rKerning;   // 0x6b8
};   // 0x6bc bytes

/* the block a tag names, at the top level only (ours: out of line under ReadPoint's cut) */
static inline _zrdr *FlatFindTag(const _zrdr *rdr, const char *tag)
{
    return zrdr_flat_findtag_startidx(rdr, tag, 0);
}

/* an int a tag names, a default where it names none, read into the caller's slot and returned
 * (ours: the target sets each default just ahead of its read and takes the value straight back) */
static inline int FindIntAt(const _zrdr *rdr, const char *tag, int *v, int def)
{
    *v = def;
    zrdr_findint(rdr, tag, v, 1);
    return *v;
}

/* a string a tag names, at the top level only (ours: LoadJamo's style name -- an inline's result,
 * numbered after the style's, as the target's registers have it) */
static inline const char *FlatFindString(const _zrdr *rdr, const char *tag)
{
    return zrdr_flat_findstring(rdr, tag, 0);
}

/* an array's i-th item as an int, none for a null array (ours) */
static inline _zrdr *GetRdrArrayItem(const _zrdr *rdr, int i)
{
    return rdr ? rdr->Item(i) : 0;
}

static inline int GetRdrInt(const _zrdr *e)
{
    return e ? e->Int() : 0;
}

static inline int GetInt(const _zrdr *rdr, unsigned int i)
{
    _zrdr *e = GetRdrArrayItem(rdr, i);

    return GetRdrInt(e);
}

/* a point (ours: the drop shadow's offset and a placement's position). In the unit's anonymous
 * namespace (the HUD's round 46): its out-of-line copies are then the unit's own, so MW knows what
 * they clobber when a later function calls them -- ReadPlacement keeps the found block in a2 across
 * the constructor, GetInt and the setters, as the target does (a global class's copies could be
 * another unit's at link time: MW assumes every caller-saved register lost and takes s1) */
namespace {
class CFontPoint {
public:
    CFontPoint() : x(0), y(0) {}
    CFontPoint(const CFontPoint &p) : x(p.x), y(p.y) {}

    void SetX(int v) { x = v; }
    void SetY(int v) { y = v; }

    int x;   // 0x0
    int y;   // 0x4
};
}

/* a point from the pair a tag names (ours: inlined one level, its callees out of line after Load).
 * The tag's block found here, not by the caller (the HUD's round 46): the cut makes FlatFindTag a
 * call too -- its copy 0x35A240, called only by Load's and ReadPlacement's point reads, every other
 * FlatFindTag inlined */
static inline CFontPoint ReadPoint(const _zrdr *rdr, const char *tag)
{
    _zrdr *t = FlatFindTag(rdr, tag);
    CFontPoint p;

    p.SetX(GetInt(t, 0));
    p.SetY(GetInt(t, 1));
    return p;
}

/* the glyph's last piece, MSL's reverse_iterator shape (ours: rbegin() as the original wrote it is
 * likely; include/vector has none) -- end() held in the frame, `current - 1` at each access */
class CRevIter {
public:
    typedef std::vector<C2DGlyphPiece>::iterator It;

    explicit CRevIter(It x) : current(x) {}
    C2DGlyphPiece &operator*() const
    {
        It tmp = current;

        return *--tmp;
    }
    C2DGlyphPiece *operator->() const { return &(operator*()); }

    It current;
};

/* the index of a name in a list, -1 if absent (ours) */
static inline int FindName(const std::vector<CSaferStr> &names, const char *s)
{
    if (!s)
        return -1;
    for (int i = 0; i < names.size(); i++) {
        if (!strcasecmp(s, names[i].c_str()))
            return i;
    }
    return -1;
}

/* a rectangle from two rdr points (0x35A840) */
static CFontRect ReadRect(const _zrdr *ul, const _zrdr *lr);
/* a placement from an rdr block's DispPos, HorScale and VertScale (0x35AA80) */
/* (byte-exact since the HUD's round 46: the point read through ReadPoint(rdr, tag), the scale stored as it
 * is read -- p.sx a register across the VertScale read, $f20) */
C2DPlacement ReadPlacement(const _zrdr *rdr);

C2DGlyphPiece *C2DGlyph::AddPiece(const _zrdr *ul, const _zrdr *lr, int height)
{
    m_pieces.resize(m_pieces.size() + 1);

    CRevIter it(m_pieces.end());
    CFontRect r = ReadRect(ul, lr);

    it->m_0 = r.left;
    it->m_2 = r.right - r.left;
    it->m_4 = height - r.top - 1;
    it->m_6 = height - r.bottom;
    return &*it;
}

/* a character of the faces rdr's charoffsets: by its Code, its pieces in Elems, or an item list
 * (the character, its corners); its baseline, kernings and colour; its width returned */
/* (byte-exact since the HUD's round 55: the ints declared together in the target's slot order --
 * code 120, lk 116, tex 112, baseline 108, rk 104 -- and read through FindIntAt, as
 * CHangulFont::Load; the glyph declared ahead of the texture index; GetTexture's one ternary; the
 * first item's type through Type(), loaded once for the test and String()'s) */
int C2DFont::LoadChar(const _zrdr *rdr)
{
    int code, lk, tex, baseline, rk;
    C2DGlyph *g;
    int ti = FindIntAt(rdr, "texture", &tex, 1) - 1;
    zdb::CTexHandle *h = GetTexture(ti);
    int height = h ? h->m_texture->m_height : 0;
    code = 0;
    if (zrdr_findint(rdr, "Code", &code, 1)) {
        unsigned short c = code;

        g = &m_glyphs[c];
        g->m_code = c;
        g->LoadElems(rdr, height);
    } else {
        _zrdr *first = rdr->Item(0);

        if (first && first->Type() == ZRDR_STRING)
            code = *(unsigned char *)first->String();
        else if (first && first->Type() == ZRDR_INTEGER)
            code = first->Int();
        unsigned short c = code;

        g = &m_glyphs[c];
        g->m_code = c;
        g->AddPiece(rdr->Item(1), rdr->Item(2), height);
    }
    g->m_18 = ti;
    g->m_12 = FindIntAt(rdr, "baseline", &baseline, 0);
    int l = FindIntAt(rdr, "lKerning", &lk, 0);
    int r = FindIntAt(rdr, "rKerning", &rk, 0);

    g->m_14 = l;
    g->m_16 = r;
    if (zrdr_findtag(rdr, "color")) {
        PNT3D c;

        zrdr_findPNT3D(rdr, "color", &c);
        g->m_1a = 1;
        g->m_20.r = (int)c.x;
        g->m_20.g = (int)c.y;
        g->m_20.b = (int)c.z;
        g->m_20.a = 128;
    }
    if (code == ' ')
        g->m_10 = 8;
    return g->GetWidth();
}

/* a glyph's pieces from its Elems: corners, then placement */
void C2DGlyph::LoadElems(const _zrdr *rdr, int height)
{
    _zrdr *elems = zrdr_findtag(rdr, "Elems");

    for (int i = 0; i < (elems ? (int)elems->Count() : 0); i++) {
        _zrdr *e = elems->Item(i);
        _zrdr *ul = zrdr_findtag(e, "UL");
        C2DGlyphPiece *p = AddPiece(ul, zrdr_findtag(e, "LR"), height);

        p->m_place = ReadPlacement(e);
    }
}

/* CharLayouts: each layout named, its three parts placed, each part's alternates' ids */
/* OPEN, 30 instructions, registers only (round 73, 46 before): the part counter is an INT (the target's
 * `slti s7,3`), the frame the target's with the placement named (`C2DPlacement pl = ...` puts it above the
 * name's string temporary, 248 over 232), and jamo declared bare ahead of the place and assigned in the loop
 * (39 to 38). Left: the place walks in s3 (the target's s0), j s2 (s3) and FindName's walk one lower --
 * the place declared bare and assigned (37), a part local bare, a named id, a copy of the place: no move. */
void CHangulFont::LoadCharLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &jamoNames)
{
    _zrdr *list = FlatFindTag(rdr, "CharLayouts");

    for (int i = 0; i < (list ? (int)list->Count() : 0); i++) {
        _zrdr *item = list->Item(i);

        names.push_back(CSaferStr(zrdr_flat_findstring(item, "Name", 0)));
        m_layouts.resize(m_layouts.size() + 1);
        _zrdr *jamo;
        CJamoPlace *place = m_layouts.back().m_parts;

        for (int k = 0; k < 3; k++, place++) {
            _zrdr *part = FlatFindTag(item, s_hangulParts[k]);

            C2DPlacement pl = ReadPlacement(part);

            place->m_place = pl;
            jamo = FlatFindTag(part, "Jamo");

            for (int j = 0; j < (jamo ? (int)jamo->Count() : 0); j++)
                place->m_keys.push_back(FindName(jamoNames, jamo->Item(j)->String()));
        }
    }
}

/* the Korean block: the jamo and their alternates, the layouts, the trail rows by vowel, the vowel
 * rows by lead, each lead's row; the texture, baseline and kernings every syllable takes */
/* (byte-exact since the HUD's round 55: the four ints declared together at the top, kerning first
 * -- their frame slots -- each read through FindIntAt, its default set just ahead of the read and
 * the value taken straight back, so the left kerning is loaded before the right one is read) */
void CHangulFont::Load(const _zrdr *rdr)
{
    _zrdr *jamo = FlatFindTag(rdr, "Jamo");
    std::vector<CSaferStr> jamoNames;

    for (int i = 0; i < JAMO_COUNT; i++)
        LoadJamo(jamoNames, FlatFindTag(jamo, s_hangulJamoNames[i]), (JAMO)i);
    std::vector<CSaferStr> layoutNames;

    LoadCharLayouts(layoutNames, rdr, jamoNames);
    std::vector<CSaferStr> vowelNames;

    LoadVowelRangeLayouts(vowelNames, rdr, layoutNames);
    std::vector<CSaferStr> leadNames;

    LoadLeadRangeLayouts(leadNames, rdr, vowelNames);
    LoadLeadUsage(rdr, leadNames);
    int lk, rk, baseline, tex;

    m_texture.m_v = FindIntAt(rdr, "texture", &tex, 1) - 1;
    m_baseline.m_v = FindIntAt(rdr, "baseline", &baseline, 0);
    int l = FindIntAt(rdr, "lKerning", &lk, 0);
    int r = FindIntAt(rdr, "rKerning", &rk, 0);

    m_lKerning.m_v = l;
    m_rKerning.m_v = r;
}

/* a jamo's rectangle, then its Styles: each style's layout name (added to the list if new) and the
 * rectangle the jamo takes there */
/* (byte-exact since the HUD's round 55: the rectangle's corners are a class (CFontCoord), so each
 * copy goes word by word through $f0; ReadRect static -- generated earlier, MW keeps a0 across it;
 * each UL block found ahead of the call, as the target's argument order has it; the entry's text
 * read before the name's null test; an int counter, re-indexing each pass, and the alternate's key
 * a temporary (`(int)j`, the target's copy stored at the call); the alternates' map indexed in the
 * loop, so only the jamo's offset is shared with the rectangle's store; the style's name an
 * inline's result, numbered after the style's -- the replay's `--move name=45.5`) */
void CHangulFont::LoadJamo(std::vector<CSaferStr> &names, const _zrdr *rdr, JAMO which)
{
    _zrdr *ul = FlatFindTag(rdr, "UL");
    CFontRect r = ReadRect(ul, FlatFindTag(rdr, "LR"));

    m_jamo[which].m_rect = r;
    _zrdr *styles = FlatFindTag(rdr, "Styles");

    for (int i = 0; i < (styles ? (int)styles->Count() : 0); i++) {
        _zrdr *style = styles->Item(i);
        const char *name = FlatFindString(style, "Name");
        int j;

        for (j = 0; j < names.size(); j++) {
            const char *s = names[j].c_str();

            if (name && !strcasecmp(name, s))
                break;
        }
        if (j >= names.size())
            names.push_back(CSaferStr(name));
        _zrdr *sul = FlatFindTag(style, "UL");

        r = ReadRect(sul, FlatFindTag(style, "LR"));
        m_jamo[which].m_alts[(int)j] = r;
    }
}

/* LeadLayoutUsage: each lead consonant's vowel row by name, the Default's where it names none */
void CHangulFont::LoadLeadUsage(const _zrdr *rdr, const std::vector<CSaferStr> &names)
{
    _zrdr *usage = FlatFindTag(rdr, "LeadLayoutUsage");
    int def = FindName(names, zrdr_flat_findstring(usage, "Default", 0));

    for (unsigned int i = 0; i < 19; i++) {
        const char *s = zrdr_flat_findstring(usage, s_hangulJamoNames[s_hangulLeads[i]], 0);
        int row = FindName(names, s);

        m_leadRows[i].m_v = row >= 0 ? row : def;
    }
}

/* LeadRangeLayouts: each named row of vowel rows, by vowel, the Default's where one names none */
/* Round 73: 21 to 9 -- `int def; unsigned int k;` declared bare ahead of the row (initialised at its
 * declaration) and assigned later (round 72's FPR tell, here on the saved GPRs: a bare local numbers after
 * an initialised one): the row s0 and the counter s3 are the target's; left the two FindName strings, one
 * register here (s0) where the target keeps s1 and s2 (a named local for either: 9 or 18).
 * Was: OPEN, 21 instructions: registers only -- the target walks the row in s0 and keeps the two
 * FindName strings apart (s1, s2), the default in s4 and the counter in s3; this build shares
 * one register for the strings and puts the row in s4. The ternary store (rather than `if (v <
 * 0) v = def;`) gives the target's doubled counter step, as in LoadLeadUsage. */
void CHangulFont::LoadLeadRangeLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &vowelNames)
{
    _zrdr *list = FlatFindTag(rdr, "LeadRangeLayouts");

    for (int i = 0; i < (list ? (int)list->Count() : 0); i++) {
        _zrdr *item = list->Item(i);

        names.push_back(CSaferStr(zrdr_flat_findstring(item, "Name", 0)));
        m_vowelRows.resize(m_vowelRows.size() + 1);
        int def;
        unsigned int k;
        CJamoIndex *row = m_vowelRows.back().m_vowels;
        def = FindName(vowelNames, zrdr_flat_findstring(item, "Default", 0));

        for (k = 0; k < 21; k++) {
            int v = FindName(vowelNames, zrdr_flat_findstring(item, s_hangulJamoNames[s_hangulVowels[k]], 0));

            (row++)->m_v = v >= 0 ? v : def;
        }
    }
}

/* VowelRangeLayouts: each named row of layouts, by trailing consonant (none first), the Default's
 * where one names none */
/* (byte-exact since the HUD's round 55: every string read through FlatFindString, an inline's
 * result -- the registers LoadJamo's style name gave) */
void CHangulFont::LoadVowelRangeLayouts(std::vector<CSaferStr> &names, const _zrdr *rdr, const std::vector<CSaferStr> &layoutNames)
{
    _zrdr *list = FlatFindTag(rdr, "VowelRangeLayouts");

    for (int i = 0; i < (list ? (int)list->Count() : 0); i++) {
        _zrdr *item = list->Item(i);

        names.push_back(CSaferStr(FlatFindString(item, "Name")));
        m_trailRows.resize(m_trailRows.size() + 1);
        CTrailRow *row = &m_trailRows.back();
        int def = FindName(layoutNames, FlatFindString(item, "Default"));
        int none = FindName(layoutNames, FlatFindString(item, "NoTrail"));

        row->m_noTrail.m_v = none >= 0 ? none : def;
        for (unsigned int k = 0; k < 27; k++) {
            int v = FindName(layoutNames, FlatFindString(item, s_hangulJamoNames[s_hangulTrails[k]]));

            row->m_trails[k].m_v = v >= 0 ? v : def;
        }
    }
}

C2DPlacement ReadPlacement(const _zrdr *rdr)
{
    C2DPlacement p;
    CFontPoint pos = ReadPoint(rdr, "DispPos");
    float sx = 1.0f;

    zrdr_findreal(rdr, "HorScale", &sx, 1);
    p.sx = sx;
    float sy = 1.0f;

    zrdr_findreal(rdr, "VertScale", &sy, 1);
    p.sy = sy;
    p.x = pos.x;
    p.y = -pos.y;
    return p;
}

static CFontRect ReadRect(const _zrdr *ul, const _zrdr *lr)
{
    int left = GetInt(ul, 0);
    int right = GetInt(lr, 0);
    int top = GetInt(ul, 1);
    int bottom = GetInt(lr, 1);

    return CFontRect(left, top, right, bottom);
}

int C2DGlyph::GetWidth()
{
    int w = 0;

    for (int i = 0; i < m_pieces.size(); i++) {
        C2DGlyphPiece &p = m_pieces[i];
        int r = (int)((float)p.m_place.x + (float)p.m_2 * p.m_place.sx);

        if (w < r)
            w = r;
    }
    return w;
}

/* everything loaded given up: the glyphs, the textures, the composer, then the scalars */
void C2DFont::Unload()
{
    m_glyphs.clear();
    FreeVector(m_glyphs);
    m_textures.clear();
    FreeVector(m_textures);
    delete m_faces;
    m_faces = 0;
    Reset();
}

/* the glyph for a character: the font's own; nothing for a carriage return; a Hangul syllable
 * composed; else a letter by the code's remainder, else the first glyph */
/* (round 55: the owned glyph released BEFORE the new one is made, as the target orders it --
 * Own(new C2DGlyph) evaluates the new first -- and each find into an iterator of its own,
 * initialised, so neither is reloaded for `&(*i).second`.) Round 56, from 15 (the frame alone):
 * the composer's C2DChar a NAMED local DECLARED FIRST, uninitialised (C2DChar() added, empty) and
 * assigned at the call -- named locals take the frame top-down in declaration order, so it lands
 * at 76 above both iterators (72, 68) as the target's; made at the call as a temporary it went in a
 * later group, under them, and the frame grew to 0x60 */
void C2DFont::GetGlyph(C2DGlyphRef *ref, unsigned short code)
{
    C2DChar ch;
    std::map<unsigned short, C2DGlyph>::iterator i = m_glyphs.find(code);

    if (i != m_glyphs.end()) {
        ref->Set(&(*i).second);
        return;
    }
    if (code == '\r') {
        ref->Set(0);
        return;
    }
    if (code >= 0xAC00 && code < 0xD7A4 && m_faces) {
        ref->m_glyph = 0;
        ref->Release();
        ref->m_owned = new C2DGlyph;
        ch = code;
        if (m_faces->Compose(ref->m_owned, ch, this))
            return;
        ref->Release();
    }
    std::map<unsigned short, C2DGlyph>::iterator j = m_glyphs.find((char)(code % 26 + 'A'));

    if (j != m_glyphs.end())
        ref->Set(&(*j).second);
    else if (m_glyphs.size())
        ref->Set(&(*m_glyphs.begin()).second);
}

/* the faces rdr's entry for the name: the textures, the opacity and scale, the glow texture, the
 * characters (their mean width), the Korean composer, the cell's top and bottom, the spacing and
 * the drop shadow */
/* OPEN, 4 bytes short (0x4F0 against 0x4F4): the drop shadow's offset -- the target calls the
 * findtag wrapper out of line (0x35A240, FlatFindTag's copy: ReadPoint is cut to one level and
 * its argument's call with it) and keeps its result in a2 across the point's out-of-line pieces,
 * which it generated first; here the wrapper inlines. ReadPlacement misses the same way (4). */
/* an int of the font's rdr, 0 without one (an inline: its local takes its frame slot after Load's named ones, the target's) */
static inline int FontReadInt(const _zrdr *rdr, const char *tag)
{
    int v = 0;

    zrdr_findint(rdr, tag, &v, 1);
    return v;
}

/* the drop shadow's block (ours: an inline two levels down, so the block is numbered after ReadPoint's local, the target's s0 and s1 -- a modelling device) */
static inline _zrdr *FontShadow1(const _zrdr *rdr)
{
    return zrdr_flat_findtag_startidx(rdr, "dropshadow", 0);
}

static inline _zrdr *FontShadow(const _zrdr *rdr)
{
    return FontShadow1(rdr);
}

void C2DFont::Load(const _zrdr *rdr, const char *name)
{
    m_name = name;
    _zrdr *face = zrdr_findtag(zrdr_findtag(rdr, "faces"), name);
    _zrdr *tex;

    if (!(tex = zrdr_flat_findtag_startidx(face, "texture", 0)))
        tex = zrdr_flat_findtag_startidx(face, "textures", 0);
    for (int i = 0; i < (tex ? (int)tex->Count() : 0); i++) {
        const char *s = tex->Item(i)->String();

        if (s) {
            zdb::CTexHandle *h = zdb::CAssetMgr::m_assets.GetTexHandle(s);

            m_textures.push_back(h);
        }
    }
    zrdr_findreal(face, "opacity", &m_opacity, 1);
    m_scale_factor = 1.0f;
    zrdr_findreal(face, "scale", &m_scale_factor, 1);
    const char *glow = zrdr_findstring(face, "glowtexture");

    if (glow)
        m_1c = zdb::CAssetMgr::m_assets.GetTexHandle(glow);
    _zrdr *chars = zrdr_findtag(face, "charoffsets");
    int n = chars ? (int)chars->Count() : 0;
    unsigned int total = 0;

    for (int i = 0; i < n; i++)
        total += LoadChar(chars->Item(i));
    _zrdr *korean = zrdr_flat_findtag_startidx(face, "Korean", 0);

    if (korean) {
        m_faces = new CHangulFont;
        m_faces->Load(korean);
    }
    m_avgWidth = (float)total / (float)n;
    float fbot = FontReadInt(face, "boty");
    float ftop = FontReadInt(face, "topy");

    m_displaytop = ftop;
    m_displaybottom = fbot;
    XPadding = FontReadInt(face, "xspacing");
    _zrdr *shadow = FontShadow(face);

    if (shadow) {
        m_3c = true;
        CFontPoint off = ReadPoint(shadow, "offset");

        m_40 = off.x;
        m_44 = off.y;
        zrdr_findPNT3D(shadow, "color", (PNT3D *)&m_48);
        float op = 1.0f;

        zrdr_findreal(shadow, "opacity", &op, 1);
        m_54 = op;
    }
}

/* a Hangul syllable built: its lead, vowel and (if any) trail placed by the layout the tables
 * choose, the composer's baseline and kernings; false where a table has no entry */
/* (byte-exact since the HUD's round 55: GetTexture one ternary over `i >= 0 && i < size` -- the
 * target's branch to a zero at the block's end and the size test's `b` over the load; the code
 * stored to the glyph inside the subtraction's expression, so it is loaded once; the kernings read
 * into locals, the right one first, before either store) */
bool CHangulFont::Compose(C2DGlyph *glyph, C2DChar code, C2DFont *font)
{
    glyph->m_code = 0;
    glyph->m_pieces.clear();
    glyph->m_10 = -1;
    glyph->m_12 = 0;
    glyph->m_14 = 0;
    glyph->m_16 = 0;
    glyph->m_18 = 0;
    glyph->m_1a = 0;
    int s = (glyph->m_code = code.m_code) - 0xAC00;
    int lead = s / 588;
    int vrow = m_leadRows[lead].m_v;

    if (vrow < 0)
        return false;
    int vowel = s % 588 / 28;
    int trow = m_vowelRows[vrow].m_vowels[vowel].m_v;

    if (trow < 0)
        return false;
    int trail = s % 28;
    CTrailRow &tr = m_trailRows[trow];
    int lay;

    if (trail)
        lay = tr.m_trails[trail - 1].m_v;
    else
        lay = tr.m_noTrail.m_v;

    if (lay < 0)
        return false;
    CLayout *layout = &m_layouts[lay];
    zdb::CTexHandle *h = font->GetTexture(m_texture.m_v);
    int height = h ? h->m_texture->m_height : 0;

    AddJamo(glyph, 0, s_hangulLeads[lead], layout, height);
    AddJamo(glyph, 1, s_hangulVowels[vowel], layout, height);
    if (trail)
        AddJamo(glyph, 2, s_hangulTrails[trail - 1], layout, height);
    glyph->m_12 = m_baseline.m_v;
    int r = m_rKerning.m_v;
    int l = m_lKerning.m_v;

    glyph->m_14 = l;
    glyph->m_16 = r;
    return true;
}

/* one jamo's piece: its rectangle -- the first alternate the layout's part names, else its own --
 * at the part's placement */
/* (round 55, from 22: the jamo a plain named local -- `&m_jamo[which]`, not GetJamo's result,
 * igreplay --want: numbered just after the parameters -- gives every register.) Round 56, from 2:
 * the find's iterator INITIALISED in the loop (an iterator assigned from find() is reloaded at its
 * dereference) and the pieces' end taken into a named local before the reverse iterator is built
 * from it. The frame reads as allocated scope by scope, the later scopes first: the iterator
 * constructor's by-value parameter (an inline's scope) took the top slot over a function-scope f, a
 * loop-scoped f took it over that parameter -- with the end a named local the copy is the local's
 * (sp+140), and the loop's f lands under it (sp+136), the reverse iterator last (sp+132). */
void CHangulFont::AddJamo(C2DGlyph *glyph, int part, JAMO which, CLayout *layout, int height)
{
    glyph->m_pieces.resize(glyph->m_pieces.size() + 1);
    CJamoGlyph *jamo = &m_jamo[which];
    std::vector<C2DGlyphPiece>::iterator e = glyph->m_pieces.end();
    CRevIter it(e);
    CJamoPlace *place = layout->GetPart(part);
    const CFontRect *r = 0;

    std::vector<int> &keys = place->m_keys;

    for (int i = 0; i < keys.size(); i++) {
        std::map<int, CFontRect> &alts = jamo->m_alts;
        std::map<int, CFontRect>::iterator f = alts.find(keys[i]);

        if (f != alts.end()) {
            r = &(*f).second;
            break;
        }
    }
    if (!r)
        r = &jamo->m_rect;
    it->m_0 = r->left;
    it->m_2 = r->right - r->left;
    it->m_4 = height - r->top - 1;
    it->m_6 = height - r->bottom;
    it->m_place.x = place->m_place.x;
    it->m_place.y = place->m_place.y;
    it->m_place.sx = place->m_place.sx;
    it->m_place.sy = place->m_place.sy;
}

/* insert<CLayout> 0x35E100's end numbered before m: include/vector's __insert_end_pointer, a marked modelling device (the AI core's round 37) */
namespace std { template <> struct __insert_end_pointer<CLayout> { enum { value = 1 }; }; }

/* A MODELLING DEVICE, a "Level 6, or 8?" witness, with `__MSL_POD_DTOR_ARGS` at this file's head (include/vector; the MSL headers round 40): the pods'
 * constructors two levels taller cut CHangulFont() in C2DFont::Load 0x359C40 to its own body (its rows 0x35A250-0x35A2B0 called), and the element copies of
 * CLayout's vector come out whole (erase 0x35DF40, insert 0x35E100). */
#pragma inline_depth(7)
