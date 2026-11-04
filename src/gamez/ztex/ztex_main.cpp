/* ztex_main.cpp -- zdb's textures: a library's handle list, the texture itself, its handle and
 * its palette, the batch relocator (OVL1 0x354510-, after valve_main.cpp). The unit name is SOCOM
 * 1's (the press beta's asserts put CTexture::Release_HTEX in a ztex_main.cpp); its functions in
 * SOCOM 1's order (CTexList's Free, Append and two GetHandles, CTexture's, po2, CTexHandle's
 * SetTexture/Destroy/Create, CTexPalette's, CBatchRelocator's). The unit ends at 0x35597F: the
 * texture manager, ztex_manager.cpp, follows (CBatchRelocator's four slots 0x355980-0x355A6F are its,
 * the vtable's unit). Not written yet: the drafts' rows (CopyTextureSelect, the select wrapper 0x354A90, Release_HTEX, Init, LoadGS,
 * CTexHandle::Destroy, CTexPalette's Read and destructor, CBatchRelocator's). Defined in reverse of
 * their image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "../znode/node_world.h"
#include "../zsys/zsys_public.h"

/* the GS TEX0 register (the SDK's libgraph sceGsTex0) */
struct sceGsTex0 {
    unsigned long TBP0 : 14;
    unsigned long TBW : 6;
    unsigned long PSM : 6;
    unsigned long TW : 4;
    unsigned long TH : 4;
    unsigned long TCC : 1;
    unsigned long TFX : 2;
    unsigned long CBP : 14;
    unsigned long CPSM : 4;
    unsigned long CSM : 1;
    unsigned long CSA : 5;
    unsigned long CLD : 3;
};

char *_strdup(const char *s, const char *file, int line);

/* the name every unnamed texture shares (by address: SOCOM 1's gp-relative static) */
extern char *data_003E02D8;

namespace zdb {

/* a palette made with its name: the next palette id its GS address (SOCOM 1's, the same code) */
CTexPalette::CTexPalette(const char *name)
{
    m_name = _strdup(name, 0, 0);
    m_loadimage = 0;
    m_gsaddr = m_palId;
    m_slots = 0;
    m_size = 0;
    m_buffer = 0;
    SetDynamic(false);
    SetUnused(0);
    m_AssetLib = 0;
    m_palId++;
}

}

/* the power of two a size rounds up to, as its exponent, from 1 (2) to 10 (1024) (SOCOM 1's po2,
 * a global function): one result assigned in every arm, the last two a ternary (written as an
 * else-if it is two words longer) */
int po2(unsigned int n)
{
    int r;

    if (n < 3)
        r = 1;
    else if (n < 5)
        r = 2;
    else if (n < 9)
        r = 3;
    else if (n < 17)
        r = 4;
    else if (n < 33)
        r = 5;
    else if (n < 65)
        r = 6;
    else if (n < 129)
        r = 7;
    else if (n < 257)
        r = 8;
    else
        r = n < 513 ? 9 : 10;
    return r;
}

namespace zdb {

/* a handle made on a texture, counted (SOCOM 1's). Never inlined in the target (CTexList::Append
 * calls it, as SOCOM 1's does), though -inline auto would take its three statements: pinned */
#pragma auto_inline off
CTexHandle *CTexHandle::Create(CTexture *tex)
{
    CTexHandle *h = new CTexHandle(tex);

    h->m_count++;
    return h;
}
#pragma auto_inline reset

/* the handle moved to another texture: the old one's handle count given back, the new one's name
 * copied and its count taken (SOCOM 1's) */
void CTexHandle::SetTexture(CTexture *tex)
{
    if (m_texture)
        m_texture->Release_HTEX();
    m_texture = tex;
    if (tex) {
        SetName(tex->m_name);
        tex->m_htex_count++;
    }
}

/* the GIF tag's fields (libgraph.h's sceGifTag, in our own words) */
struct GifTag {
    unsigned long NLOOP : 15;
    unsigned long EOP : 1;
    unsigned long pad16 : 16;
    unsigned long id : 14;
    unsigned long PRE : 1;
    unsigned long PRIM : 11;
    unsigned long FLG : 2;
    unsigned long NREG : 4;
    unsigned long REGS0 : 4;
    unsigned long REGS1 : 4;
};

/* the GS register words a select carries (the SDK's libgraph macros, the fields this unit sets) */
#define SCE_GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc)                                                    \
    ((unsigned long)(tbp) | ((unsigned long)(tbw) << 14) | ((unsigned long)(psm) << 20) |             \
     ((unsigned long)(tw) << 26) | ((unsigned long)(th) << 30) | ((unsigned long)(tcc) << 34))
#define SCE_GS_SET_ALPHA(a, b, c, d, fix)                                                              \
    ((unsigned long)(a) | ((unsigned long)(b) << 2) | ((unsigned long)(c) << 4) |                     \
     ((unsigned long)(d) << 6) | ((unsigned long)(fix) << 32))

/* a shadow or reflection texture (by its name) given a select of its own: six registers -- TEX0
 * at its GS address, TEX1, TEXA, CLAMP and ALPHA, the shadow's blend, the reflection's or the
 * plain one -- in a fresh VU select block (SOCOM 1's, the same code); any other texture is left
 * alone */
int CTexture::SetupSpecialTextures()
{
    bool shadow = false;
    bool reflection = false;

    if (strstr(m_name, "ShadowX_"))
        shadow = true;
    else if (strstr(m_name, "ReflectionX_"))
        reflection = true;
    if (!shadow && !reflection)
        return 0;

    unsigned int tbw = m_width > 64 ? m_width / 64 : 1;

    m_selectQwc = 6;
    if (m_vuSelect)
        _free(m_vuSelect, 0, 0);
    m_vuSelect = 0;
    m_vuSelect = CVisual::GetTextureSelectBlock(m_selectQwc, &m_gifSelect, (_word128 *)&m_dmaRef,
                                                (_word128 *)&m_dmaRefVu);
    m_loadimage = m_vuSelect;

    _word128 *p = m_gifSelect;

    p[0].u128 = 0;
    ((GifTag *)p)->NLOOP = m_selectQwc - 1;
    ((GifTag *)p)->EOP = 1;
    ((GifTag *)p)->NREG = 1;
    ((GifTag *)p)->REGS0 = 0xe;
    p[1].u64[1] = 6;
    p[1].u64[0] = SCE_GS_SET_TEX0(m_gsaddr, tbw, m_format, (unsigned int)po2(m_width),
                                  (unsigned int)po2(m_height), 1);
    p[2].u64[1] = 0x14;
    p[2].u64[0] = 0x61;
    p[3].u64[1] = 0x3b;
    p[3].u64[0] = (unsigned long)0x80 << 32;
    p[4].u64[1] = 8;
    p[4].u64[0] = 5;
    if (shadow)
        p[5].u64[0] = SCE_GS_SET_ALPHA(2, 1, 0, 1, 0);
    else if (reflection)
        p[5].u64[0] = SCE_GS_SET_ALPHA(0, 1, 2, 1, 20);
    else
        p[5].u64[0] = SCE_GS_SET_ALPHA(0, 1, 0, 1, 0);
    p[5].u64[1] = 0x42;
    m_gifSelect->u32[3] = (unsigned int)this;
    SetGifPtrs();
    return 0;
}

/* a texdat's header (ours): TEXTURE_PARAMS, then the gaps before the pixels and the select */
struct TexDatHeader {
    unsigned char params[0x10];   // 0x0
    int pixels;   // 0x10
    int select;   // 0x14
    int _18;   // 0x18
};

/* SOCOM 1's: the texture's "texdat" key fetched in place -- its parameters copied, its pixels and
 * GIF select found after them, the VU select built -- and its pixel format and palette id set.
 * Byte-exact only with constant propagation off (as SetGifPtrs): the target keeps `ok` in s0, set
 * true from the start (`daddiu s0,zero,1` in FindKey's delay slot; SOCOM 1's was false until the
 * fetch succeeded) and returns it with `move v0,s0`, where MW otherwise folds it to `li v0,1` --
 * `bool ok; ok = true;`, an inline returning true and volatile all fold or miss. The palette id goes
 * through `(int)`: the target sign-extends CBP before its halfword store */
#pragma opt_propagation off
bool CTexture::Read(CSaveLoad &io)
{
    bool ok = true;
    char *buf = 0;
    zar::CKey *key = io.m_zfile.FindKey("texdat");

    if (key && io.m_zfile.FetchLIP(key, (void **)&buf)) {
        memcpy(this, buf, 16);
        m_loadimage = _malloc(96, 0, 0);
        TexDatHeader *h = (TexDatHeader *)buf;
        buf += 0x18;
        buf += h->pixels;
        m_buffer = buf;
        buf += m_size;
        buf += h->select;
        m_gifSelect = (_word128 *)buf;
        m_vuSelect = CVisual::GetTextureSelectBlock_ver2(m_selectQwc, &m_gifSelect, (_word128 *)&m_dmaRef,
                                                         (_word128 *)&m_dmaRefVu);
        m_gsWords = Compute64WordMemoryUsage();
        m_gifSelect->u32[3] = (unsigned int)this;
        SetGifPtrs();
    }
    switch (m_texelBitSize) {
    case 32:
        m_format = 0;
        break;
    case 24:
        m_format = 1;
        break;
    default:
    case 16:
        if (m_palettized)
            m_format = 19;
        else
            m_format = 2;
        break;
    case 8:
        m_format = 19;
        break;
    case 4:
        m_format = 20;
        break;
    }
    m_palID = (int)((sceGsTex0 *)m_TEX0)->CBP;
    return ok;
}
#pragma opt_propagation reset

/* SOCOM 1's: every field cleared, a 16-bit bilinear texture of no size, the name copied. The
 * fields from the name on are initialised ahead of the mip list (the target's stores before its
 * three words), TEXTURE_PARAMS' in the body */
CTexture::CTexture(const char *name)
    : m_name(data_003E02D8), m_buffer(0), m_palette(0), m_gifSelect(0), m_vuSelect(0), m_format(0),
      m_palID(0), m_htex_count(0), m_gsWords(0)
{
    m_texelBitSize = 16;
    m_height = 0;
    m_width = 0;
    m_gsaddr = 0;
    m_selectQwc = 0;
    m_pal_offset = 0;
    SetTransparent(false);
    SetPalettized(false);
    SetIsMipChild(false);
    SetBumpmap(false);
    SetBilinear(true);
    SetTransp1Bit(false);
    SetDynamic(false);
    SetContext(false);
    m_AssetLib = 0;
    SetName(name);
    m_loadimage = 0;
    m_TEX0 = 0;
    m_MIP1 = 0;
    m_MIP2 = 0;
}

/* SOCOM 1's: the name and the image buffer freed, the mip list torn down. The name goes through
 * SetName(0), inlined -- its strdup arm left in place behind an unconditional branch -- then the
 * default again (SOCOM 1's does the same two stores) */
CTexture::~CTexture()
{
    SetName(0);
    m_name = data_003E02D8;
    if (m_loadimage)
        _free(m_loadimage, 0, 0);
    m_loadimage = 0;
}

/* SOCOM 1's: the old name freed unless it is the shared default */
void CTexture::SetName(const char *name)
{
    if (m_name && m_name != data_003E02D8)
        _free(m_name, 0, 0);
    m_name = name ? _strdup(name, 0, 0) : data_003E02D8;
}

/* translucent and blended: neither bump-mapped nor 1-bit (SOCOM II's; the name ours -- the
 * particles' draw asks it) */
bool CTexture::IsAlphaBlended()
{
    return m_transparent && !m_bumpmap && !m_transp_1bit;
}

/* SOCOM 1's: the texture's GS memory in 64-word blocks, rounded up -- a narrow low-depth texture
 * takes twice its texels */
unsigned int CTexture::Compute64WordMemoryUsage()
{
    unsigned int n = m_width * m_height;

    if (m_texelBitSize < 9 && m_height * 2 < m_width && m_texelBitSize * m_width < 2048)
        n *= 2;
    switch (m_texelBitSize) {
    case 4:
        n >>= 3;
        break;
    case 8:
        n >>= 2;
        break;
    case 16:
        if (m_palettized)
            n >>= 2;
        else
            n >>= 1;
        break;
    case 24:
    case 32:
        if (m_palettized)
            n >>= 2;
        else if (m_transp_1bit)
            n >>= 1;
        break;
    }
    if (n & 0x3f)
        n += 64 - (n & 0x3f);
    return n >> 6;
}

/* a GIF select's register entry for `reg` given new data -- and, unless 127, a new register --
 * searched from `start` (or the first entry); the entry, or none (SOCOM II's; the name ours: the
 * texture's wrapper 0x354A90 searches its own select from the top) */
_word128 *SetGifReg(_word128 *select, unsigned int reg, unsigned long data, int newreg, _word128 *start)
{
    _word128 *found = 0;
    unsigned int i = 0;
    unsigned short n = select->u16[0] & 0x7fff;

    /* the parameter stepped past the tag (the target keeps the cursor in a0) */
    select++;
    if (start) {
        i = start - select;
        select = start;
    }
    _word128 *p = select;
    for (; i < n && !found; i++, p++)
        if (p->u32[2] == reg) {
            p->u64[0] = data;
            if (newreg != 127)
                p->u32[2] = newreg;
            found = p;
        }
    return found;
}

/* a GIF select's register entry for a register address, or none: the tag's NLOOP entries after it
 * (ours, inline -- SetGifPtrs expands it three times) */
static inline _word128 *FindGifReg(_word128 *select, unsigned int reg)
{
    _word128 *p = select;
    unsigned int n = p->u16[0] & 0x7fff;

    p++;
    for (unsigned int i = 0; i < n; i++, p++)
        if (p->u32[2] == reg)
            return p;
    return 0;
}

/* SOCOM 1's: the select's TEX0 register for the texture's context, and its MIPTBP1/2 when it has
 * them. Byte-exact only with constant propagation off: the target tests each search's counter (0)
 * against the count in a register (`sltu at,a2,a1`) where MW otherwise folds it (`sltu at,zero,a1`)
 * -- no loop form tried keeps it (a while, a found pointer, a break and a test after, the counter a
 * parameter or declared first); the original's form is not known */
#pragma opt_propagation off
void CTexture::SetGifPtrs()
{
    _word128 *p;

    m_TEX0 = FindGifReg(m_gifSelect, 6 + m_context);
    p = FindGifReg(m_gifSelect, 0x34 + m_context);
    if (p)
        m_MIP1 = p;
    p = FindGifReg(m_gifSelect, 0x36 + m_context);
    if (p)
        m_MIP2 = p;
}
#pragma opt_propagation reset

/* CTexList::GetHandle(const char *)'s test: a handle's texture of that name, any case (ours) */
struct CTexHandleNamed {
    CTexHandleNamed(const char *name) : m_name(name) {}
    /* the two null tests branch, only the comparison is materialised (a `&&` of all three
     * materialises each test) */
    bool operator()(CTexHandle *h) const
    {
        CTexture *tex = h->m_texture;

        if (tex && tex->m_name)
            return strcasecmp(tex->m_name, m_name) == 0;
        return false;
    }

    const char *m_name;
};

/* CTexList::GetHandle(CTexture *)'s test: a handle on that texture (ours) */
struct CTexHandleOf {
    CTexHandleOf(CTexture *tex) : m_tex(tex) {}
    bool operator()(CTexHandle *h) const { return h->m_texture == m_tex; }

    CTexture *m_tex;
};

/* the handle of that texture, or none (SOCOM 1's). Never inlined in the target (Append and
 * CDynTexList::Add call it), though -inline auto would take it: pinned */
#pragma auto_inline off
CTexHandle *CTexList::GetHandle(CTexture *tex)
{
    iterator i = std::find_if(begin(), end(), CTexHandleOf(tex));

    if (i != end())
        return *i;
    return 0;
}
#pragma auto_inline reset

/* the handle of a texture of that name, or none (SOCOM 1's) */
CTexHandle *CTexList::GetHandle(const char *name)
{
    iterator i = std::find_if(begin(), end(), CTexHandleNamed(name));

    if (i != end())
        return *i;
    return 0;
}

/* a texture's handle listed -- with b, the one already listed reused (SOCOM 1's) */
CTexHandle *CTexList::Append(CTexture *tex, bool b)
{
    CTexHandle *h = 0;

    if (b)
        h = GetHandle(tex);
    if (!h) {
        h = CTexHandle::Create(tex);
        push_back(h);
    }
    return h;
}

/* CTexList::Free's operation: a handle destroyed, its slot left null (ours) */
struct CTexHandleDestroy {
    CTexHandleDestroy() {}
    CTexHandle *operator()(CTexHandle *h)
    {
        CTexHandle::Destroy(h);
        return 0;
    }
};

/* every handle let go of, the list emptied (SOCOM 1's: the std::transform of the list onto
 * itself -- two begin()s and an end(), right to left) */
void CTexList::Free()
{
    std::transform(begin(), end(), begin(), CTexHandleDestroy());
    clear();
}

}
