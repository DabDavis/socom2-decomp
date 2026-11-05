/* ztex_manager.cpp -- zdb's texture manager (OVL1 0x355980-0x357E8F): the GS texture buffers, the
 * DMA chains that load their dynamic textures and palettes, the select registers rewritten when
 * a texture moves, and the manager that keeps the buffers. The press beta's asserts name the unit
 * (the 1.0 beta's `tu:` rows); its functions in SOCOM 1's order (MIPUpdateTextureSelects,
 * UpdateTextureSelects, ClearPktBufs, DestroyBuffers, doSetupBuffers, WipePktBuf, doDelBuffer,
 * doAddBuffer, CGSTexBuffer's Add(CAssetLib *), Load, refLoadImage, MakeChain, RelocateAll,
 * Hookup, Setup, Add(CTexList *), SetDynTexList, CDynTexList's three Adds, CGSTexBuffer's
 * destructor and constructor, CBatchRelocator::TakeDrawPacket, then the template tail).
 *
 * THE BOUNDARY with ztex_main.cpp is 0x355980, not in 0x355A70-0x355DE0: CBatchRelocator's vtable
 * (0x406C10) holds four slots at 0x355980-0x355A6F and TakeDrawPacket at 0x357C10, and a class's
 * vtable comes out first in the image of the unit holding its key function (README, CZFTSWeapon's
 * Recoil) -- SOCOM 1 has the same split, its four as inline copies ("?" binding) right after
 * CTexPalette's constructor and its TakeDrawPacket global ("T") after CGSTexBuffer's constructor.
 * The chain-dropping store at 0x355A20 between them is called out of line by this unit's own
 * functions (AddToBatch, Add(CAssetLib *), Hookup, Add(CTexList *)) -- a pinned one-liner. Those
 * five rows are still their drafts' (byte-exact as they stand).
 *
 * The buffers' packet vector is a vector<u_long128> (SOCOM 1's calls mangle its element `1`): a
 * class vector, its clear and destructor out of class -- the zrndr_pipe.cpp unit's template tail
 * emitted the copies the linker kept (0x33C350 the deleter's clear, 0x33C360 its destructor, and
 * the one-level pieces 0x33BA60-0x33BA70, 0x33B9C0), so this unit calls those.
 * Defined in reverse of their image order (-inline deferred). The names marked ours are. */

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "../znode/node_world.h"
#include "../zrdr/zrdr.h"
#include "../zsys/zsys_public.h"

char *_strdup(const char *s, const char *file, int line);

/* the render maps' and shadow maps' default sizes (initialised data, by address: the manager's
 * constructor copies them) */
extern unsigned int data_003E02E0;
extern unsigned int data_003E02E8;
extern unsigned int data_003E02F0;
extern unsigned int data_003E02F8;

/* the GS registers the selects and the image loads write (the SDK's libgraph) */
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

struct sceGsMiptbp1 {
    unsigned long TBP1 : 14;
    unsigned long TBW1 : 6;
    unsigned long TBP2 : 14;
    unsigned long TBW2 : 6;
    unsigned long TBP3 : 14;
    unsigned long TBW3 : 6;
    unsigned long pad60 : 4;
};

/* the uncached mirror of a main-memory address (the GIF reads the selects through it) */
#define UNCACHED(p) ((unsigned int)(p) | 0x20000000)

/* the GS packets' register words (the SDK's libgraph macros) */
#define SCE_GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm)                                            \
    ((unsigned long)(sbp) | ((unsigned long)(sbw) << 16) | ((unsigned long)(spsm) << 24) |             \
     ((unsigned long)(dbp) << 32) | ((unsigned long)(dbw) << 48) | ((unsigned long)(dpsm) << 56))
#define SCE_GS_SET_TRXREG(rrw, rrh) ((unsigned long)(rrw) | ((unsigned long)(rrh) << 32))
#define SCE_GIF_SET_TAG(nloop, eop, pre, prim, flg, nreg)                                              \
    ((unsigned long)(nloop) | ((unsigned long)(eop) << 15) | ((unsigned long)(pre) << 46) |           \
     ((unsigned long)(prim) << 47) | ((unsigned long)(flg) << 58) | ((unsigned long)(nreg) << 60))
#define SCE_GS_BITBLTBUF 0x50
#define SCE_GS_TRXPOS 0x51
#define SCE_GS_TRXREG 0x52
#define SCE_GS_TRXDIR 0x53
#define SCE_GS_TEXFLUSH 0x3f

namespace zdb {

/* the template tail's vector of u_long128 pieces as zrndr_pipe.cpp's tail emitted them (the copies
 * the linker kept): the deleter's clear, called by the inlined clear()s */

/* a batch relocator's draw: the packet kicked (SOCOM 1's TakeDrawPacket, its key function). Its
 * row is its draft's for now */

/* SOCOM 1's: every field cleared, the packet vector and the dynamic list empty, the overflow check
 * on */
CGSTexBuffer::CGSTexBuffer()
{
    m_checkForOverflow = true;
    m_startAddress = 0;
    m_nextAddress = 0;
    m_endAddress = 0;
    m_assetLib = 0;
    m_chainp = 0;
    m_name = 0;
}

/* SOCOM 1's: the name freed, the lists and the packet vector emptied */
CGSTexBuffer::~CGSTexBuffer()
{
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_dyntex_list.m_tex_list.clear();
    m_dyntex_list.m_pal_list.clear();
    m_pktbuf.clear();
}

/* another list's handles and palettes added, each once */
void CDynTexList::Add(CDynTexList *list)
{
    if (list) {
        for (CTexList::iterator i = list->m_tex_list.begin(); i != list->m_tex_list.end(); ++i)
            Add(*i, true);
        for (CTexPalList::iterator i = list->m_pal_list.begin(); i != list->m_pal_list.end(); ++i)
            Add(*i, true);
    }
}

/* a handle listed -- unless b and its texture is already there -- and its texture's palette after
 * it (SOCOM 1's inlined the palette's Add) */
void CDynTexList::Add(CTexHandle *tex, bool b)
{
    if (b && m_tex_list.GetHandle(tex->m_texture))
        return;
    if (m_tex_list.size() == m_tex_list.capacity())
        m_tex_list.reserve(m_tex_list.size() + 1);
    m_tex_list.push_back(tex);
    if (tex->m_texture->m_palette)
        Add(tex->m_texture->m_palette, b);
}

/* a palette listed, unless b and it is already there -- the test is CTexPalList::Exists written
 * out, its parameter a local of its own (the MSL headers round 24): with include/vector's pointer
 * iterator as the image's (end() four high) the inline Exists's end() is six deep here and cut at
 * 5 (end's body kept, the raw end and __iter called), and inline_depth(6) costs the unit three
 * other functions (~CGSTexBuffer, doAddBuffer, WipePktBuf grow). Written so, the end is computed
 * once for find's last and the test, and the copied pointer is the value find reads, loaded
 * ahead of the iterator copies into t1 as the target (the parameter itself loads it late, 13 off) */
void CDynTexList::Add(CTexPalette *pal, bool b)
{
    if (b) {
        CTexPalette *p = pal;
        CTexPalList::iterator it = std::find(m_pal_list.begin(), m_pal_list.end(), p);

        if (it != m_pal_list.end())
            return;
    }
    if (m_pal_list.size() == m_pal_list.capacity())
        m_pal_list.reserve(m_pal_list.size() + 1);
    m_pal_list.push_back(pal);
}

/* the buffer's dynamic list made another's (SOCOM 1's SetDynTexList: the lists emptied, the other
 * added, the chain dropped) */
void CGSTexBuffer::SetDynTexList(CDynTexList *list)
{
    m_dyntex_list.m_tex_list.clear();
    m_dyntex_list.m_pal_list.clear();
    m_dyntex_list.Add(list);
    m_chainp = 0;
}

/* a library's textures put on the buffer's dynamic list, each palette after its texture, the chain
 * dropped after each (SOCOM 1's; its palette Add inlined) */
void CGSTexBuffer::Add(CTexList *list, bool b)
{
    unsigned int n = list->size();

    for (unsigned int i = 0; i < n; i++) {
        CTexHandle *tex = (*list)[i];

        m_dyntex_list.Add(tex, b);
        DropChain();
        if (tex->m_texture->m_palette) {
            m_dyntex_list.Add(tex->m_texture->m_palette, b);
            DropChain();
        }
    }
}

/* the buffer placed between two fractions of the texture memory: when it moves, its next address
 * goes back to its start and its chain is dropped; its end given back (SOCOM 1's Setup, which
 * called its helpers; SOCOM II's a leaf) */
unsigned int CGSTexBuffer::Setup(float start, float end)
{
    unsigned int size = CTexture::m_endAddress - CTexture::m_baseAddress;
    unsigned int s = (unsigned int)((float)CTexture::m_baseAddress + start * (float)size);
    unsigned int e = (unsigned int)((float)CTexture::m_baseAddress + end * (float)size - 1.0f);

    if (m_startAddress != s || m_endAddress != e) {
        m_nextAddress = s;
        m_startAddress = s;
        m_endAddress = e;
        m_chainp = 0;
    }
    return m_endAddress;
}

/* the buffer placed between two GS addresses, its chain dropped (SOCOM II's; the cinematic state's
 * Init places one so) */
unsigned int CGSTexBuffer::Setup(unsigned int start, unsigned int end)
{
    m_nextAddress = start;
    m_startAddress = start;
    m_endAddress = end;
    m_chainp = 0;
    return m_endAddress;
}

/* the texture's palette looked for in a list, by id and library name, and hooked up when found
 * (Hookup asks three lists; the name ours). Hookup calls it out of line three times, where -inline
 * auto would take it whole: pinned */
#pragma auto_inline off
bool CTexture::FindPalette(CTexPalList *list)
{
    bool found = false;

    for (CTexPalList::iterator i = list->begin(); i != list->end(); ++i) {
        CTexPalette *pal = *i;

        if (m_palID == pal->m_id && !strcmp(pal->m_AssetLib->m_name, m_AssetLib->m_name)) {
            m_palette = pal;
            found = true;
            break;
        }
    }
    return found;
}
#pragma auto_inline reset

/* whether a texture is one of this one's mip levels (CAssetList::FindMipLevel's test; the name
 * ours) */
bool CTexture::HasMipLevel(CTexture *tex)
{
    bool found = false;

    for (int i = 0; i < m_reverse_miplist.size() && !found; i++)
        found = m_reverse_miplist[i] == tex;
    return found;
}

/* each dynamic texture's GS address copied to its handle, its palette found -- in the buffer's
 * list, the library's, any loaded library's (the last added to the buffer) -- and, for a mip
 * level, the textures whose MIPTBP registers point at it listed as its parents (SOCOM 1's Hookup;
 * SOCOM II's mip walk).
 * OPEN, register-only (42 instructions, the size right): the target keeps this in s2, lib s1, tex
 * s0, the walk i s4, the asset walk and the mip walk s3, the list's end s5; this gives i s0, tex s1,
 * lib s2, this s3, the walks s4 (MW's colouring, the world lane's round 14: the walk's iterator is a
 * scalariser temporary of degree 86, deferred with this, lib and tex and so coloured first of them;
 * the target colours it after this, which no numbering of a temporary gives). Settled on the way: FindPalette is called out of line (pinned), the
 * first find's result a bool local (andi), the second `lib && ...` a value, the asset list's end a
 * local kept in s5, the mip tests written out in the if (an inline bool helper materialises them),
 * and the pushed texture a copy made at the push (the target stores it to the stack only then; t
 * itself, or a cast of it, is stored where it is set) */
void CGSTexBuffer::Hookup(CAssetLib *lib)
{
    for (CTexList::iterator i = m_dyntex_list.m_tex_list.begin(); i != m_dyntex_list.m_tex_list.end(); ++i) {
        CTexture *tex = (*i)->m_texture;

        (*i)->m_gsAddr = tex->m_gsaddr;
        if (tex->m_palettized) {
            bool found = tex->FindPalette(&m_dyntex_list.m_pal_list);

            if (!found) {
                found = lib && tex->FindPalette(&lib->m_palettes);
                if (!found) {
                    CAssetList::lib_iterator e = CAssetMgr::m_assets.end();

                    for (CAssetList::lib_iterator l = CAssetMgr::m_assets.begin(); l != e; ++l) {
                        found = tex->FindPalette(&((CAssetLib *)*l)->m_palettes);
                        if (found) {
                            m_dyntex_list.Add(tex->m_palette, true);
                            DropChain();
                            break;
                        }
                    }
                }
            }
        }
        if (tex->m_is_mip_child) {
            for (CTexList::iterator j = m_dyntex_list.m_tex_list.begin(); j != m_dyntex_list.m_tex_list.end(); ++j) {
                CTexture *t = *j ? (*j)->m_texture : 0;
                unsigned int addr = tex->m_gsaddr;
                bool found = false;

                if (t->m_MIP1 && (((sceGsMiptbp1 *)t->m_MIP1)->TBP1 == addr ||
                                  ((sceGsMiptbp1 *)t->m_MIP1)->TBP2 == addr ||
                                  ((sceGsMiptbp1 *)t->m_MIP1)->TBP3 == addr))
                    found = true;
                if (t->m_MIP2 && (((sceGsMiptbp1 *)t->m_MIP2)->TBP1 == addr ||
                                  ((sceGsMiptbp1 *)t->m_MIP2)->TBP2 == addr ||
                                  ((sceGsMiptbp1 *)t->m_MIP2)->TBP3 == addr))
                    found = true;
                if (found) {
                    CTexture *p = t;

                    tex->m_reverse_miplist.push_back(p);
                }
            }
        }
    }
}

/* every dynamic palette and texture placed one after another from the buffer's start, a place
 * rewritten only where it changes (SOCOM 1's RelocateAll, palettes first) */
void CGSTexBuffer::RelocateAll()
{
    m_nextAddress = m_startAddress;
    for (CTexPalList::iterator i = m_dyntex_list.m_pal_list.begin(); i != m_dyntex_list.m_pal_list.end(); ++i) {
        CTexPalette *pal = *i;

        unsigned int addr = m_nextAddress;

        if (addr != pal->m_gsaddr)
            pal->m_gsaddr = addr;
        m_nextAddress = addr + ((unsigned int)pal->m_size >> 8);
    }
    for (CTexList::iterator i = m_dyntex_list.m_tex_list.begin(); i != m_dyntex_list.m_tex_list.end(); ++i) {
        unsigned int addr = m_nextAddress;
        CTexture *tex = (*i)->m_texture;

        if (addr != tex->m_gsaddr)
            tex->m_gsaddr = addr;
        m_nextAddress = addr + tex->m_gsWords;
    }
}

}

/* an image's load into GS memory as a DMA chain: the GIF registers (BITBLTBUF, TRXPOS, TRXREG,
 * TRXDIR) in a packet of their own, then the pixels referenced in place, in two pieces past
 * 0x7FFF quadwords; the chain's end given back (SOCOM 1's refLoadImage, a global function).
 * Stepped a quadword at a time (MW then folds every other step into the offsets, as the target's
 * `addiu a0,a0,32` after each pair; `p += 2` after four words folds them all), the image tags
 * through libgraph's SCE_GIF_SET_TAG (its constant built once and shared by both tags), the size
 * test as `> 0x7fff` (`>=` tests into v0, not at) and the second piece's address a pointer sum
 * (the offset built in at) */
u_long128 *refLoadImage(u_long128 *p, unsigned int dbp, unsigned int psm, void *data, unsigned int size,
                        unsigned int w, unsigned int h)
{
    unsigned int dbw = w > 64 ? w >> 6 : 1;
    unsigned int qwc = size >> 4;
    unsigned int rest = 0;

    if (qwc > 0x7fff) {
        rest = qwc - 0x7fff;
        qwc = 0x7fff;
    }
    ((unsigned long *)p)[0] = 0x10000006;
    ((unsigned long *)p)[1] = 0;
    p++;
    ((unsigned long *)p)[0] = 4 | ((unsigned long)0x10000000 << 32);
    ((unsigned long *)p)[1] = 0xe;
    p++;
    ((unsigned long *)p)[0] = SCE_GS_SET_BITBLTBUF(0, 0, psm, dbp, dbw, psm);
    ((unsigned long *)p)[1] = SCE_GS_BITBLTBUF;
    p++;
    ((unsigned long *)p)[0] = 0;
    ((unsigned long *)p)[1] = SCE_GS_TRXPOS;
    p++;
    ((unsigned long *)p)[0] = SCE_GS_SET_TRXREG(w, h);
    ((unsigned long *)p)[1] = SCE_GS_TRXREG;
    p++;
    ((unsigned long *)p)[0] = 0;
    ((unsigned long *)p)[1] = SCE_GS_TRXDIR;
    p++;
    ((unsigned long *)p)[0] = SCE_GIF_SET_TAG(qwc, 1, 0, 0, 2, 0);
    ((unsigned long *)p)[1] = 0;
    p++;
    ((unsigned int *)p)[0] = qwc | 0x30000000;
    ((unsigned int *)p)[1] = (unsigned int)data;
    ((unsigned long *)p)[1] = 0;
    p++;
    if (rest) {
        ((unsigned long *)p)[0] = 0x10000001;
        ((unsigned long *)p)[1] = 0;
        p++;
        ((unsigned long *)p)[0] = SCE_GIF_SET_TAG(rest, 1, 0, 0, 2, 0);
        ((unsigned long *)p)[1] = 0;
        p++;
        ((unsigned int *)p)[0] = rest | 0x30000000;
        ((unsigned int *)p)[1] = (unsigned int)((char *)data + 0x80000);
        ((unsigned long *)p)[1] = 0;
        p++;
    }
    return p;
}

namespace zdb {

/* the chain built: each dynamic palette and texture placed from the buffer's start -- unless it
 * would overflow the buffer -- and its pixels loaded where it has any, then the chain's end and a
 * TEXFLUSH (SOCOM 1's MakeChain, palettes first; SOCOM 1's took a count it did not use, SOCOM II's
 * takes none: Load sets no a2).
 * The first store goes through a0 only with the chain's cursor declared after it (declared first,
 * this is read from its saved copy), and the end tags step a quadword at a time (`p += 2` after
 * four words folds the step into the offsets) */
u_long128 *CGSTexBuffer::MakeChain(u_long128 *chain)
{
    m_nextAddress = m_startAddress;

    u_long128 *p = chain;

    for (CTexPalList::iterator i = m_dyntex_list.m_pal_list.begin(); i != m_dyntex_list.m_pal_list.end(); ++i) {
        CTexPalette *pal = *i;

        if (!m_checkForOverflow || m_nextAddress + ((unsigned int)pal->m_size >> 8) < m_endAddress) {
            unsigned int addr = m_nextAddress;

            if (addr != pal->m_gsaddr)
                pal->m_gsaddr = addr;
            m_nextAddress = addr + ((unsigned int)pal->m_size >> 8);
            if (pal->m_buffer)
                p = refLoadImage(p, pal->m_gsaddr, pal->m_format, pal->m_buffer, pal->m_size, 16, 16);
        }
    }
    for (CTexList::iterator i = m_dyntex_list.m_tex_list.begin(); i != m_dyntex_list.m_tex_list.end(); ++i) {
        CTexture *tex = (*i)->m_texture;

        if (tex && (!m_checkForOverflow || m_nextAddress + tex->m_gsWords < m_endAddress)) {
            unsigned int addr = m_nextAddress;

            if (addr != tex->m_gsaddr)
                tex->m_gsaddr = addr;
            m_nextAddress = addr + tex->m_gsWords;
            if (tex->m_buffer)
                p = refLoadImage(p, tex->m_gsaddr, tex->m_format, tex->m_buffer, tex->m_size, tex->m_width,
                                 tex->m_height);
        }
    }
    ((unsigned long *)p)[0] = 0x70000002;
    ((unsigned long *)p)[1] = 0;
    p++;
    ((unsigned long *)p)[0] = 0x8001 | ((unsigned long)0x10000000 << 32);
    ((unsigned long *)p)[1] = 0xe;
    p++;
    ((unsigned long *)p)[0] = 0;
    ((unsigned long *)p)[1] = SCE_GS_TEXFLUSH;
    FlushCache(0);
    return chain;
}

/* the buffer loaded: nothing to load gives no chain; else (hooked up first when now) the chain built
 * into a packet buffer big enough when there is none, or the textures relocated in the old one, and
 * when now the selects updated and the chain sent and waited for (SOCOM 1's Load, the same shape).
 * The emptiness test is a value (xor, sltiu, then the branch): a bool local, or an inline bool
 * returning the `&&`, both match; written in the `if` it branches on each size */
u_long128 *CGSTexBuffer::Load(bool now)
{
    bool empty = m_dyntex_list.m_tex_list.empty() && m_dyntex_list.m_pal_list.empty();

    if (empty)
        return 0;
    if (now)
        Hookup(m_assetLib);
    if (!m_chainp) {
        unsigned int n = (m_dyntex_list.m_tex_list.size() + m_dyntex_list.m_pal_list.size()) * 11 + 3;

        m_pktbuf.clear();
        if (m_pktbuf.capacity() < n)
            m_pktbuf.reserve(n);
        m_chainp = MakeChain(m_pktbuf.data_);
    } else {
        RelocateAll();
    }
    if (now) {
        UpdateTextureSelects();
        u_long128 *chain = m_chainp;

        sceDmaSync(zSys.DmaToGif, 0, 0);
        sceDmaSend(zSys.DmaToGif, chain);
        sceDmaSync(zSys.DmaToGif, 0, 0);
    }
    return m_chainp;
}

/* the packet buffer emptied and made room in for n quadwords (SOCOM II's; the name ours: the
 * cinematic state readies two buffers so) */
void CGSTexBuffer::ReservePktBuf(unsigned int n)
{
    m_pktbuf.clear();
    if (m_pktbuf.capacity() < n)
        m_pktbuf.reserve(n);
}

/* the packet buffer emptied and its storage given up, the chain dropped (SOCOM II's, out of line:
 * the menu and cinematic states call it; WipePktBuf calls it for every buffer) */
void CGSTexBuffer::WipePktBuf()
{
    m_pktbuf.clear();
    FreeVector(m_pktbuf);
    m_chainp = 0;
}

/* a library's textures and palettes put on the buffer's dynamic list, the chain dropped after each
 * (SOCOM 1's; SOCOM II walks a copy of the palette list). The texture walk reads the list through a
 * pointer kept in s3 (a reference or a pointer local, declared before or after the store, all
 * match; through lib it is 0x148) */
void CGSTexBuffer::Add(CAssetLib *lib, bool b)
{
    CTexList &texs = lib->m_textures;

    m_assetLib = lib;
    for (CTexList::iterator i = texs.begin(); i != texs.end(); ++i) {
        m_dyntex_list.Add(*i, b);
        DropChain();
    }
    CTexPalList pals(lib->m_palettes);

    for (CTexPalList::iterator i = pals.begin(); i != pals.end(); ++i) {
        m_dyntex_list.Add(*i, b);
        DropChain();
    }
}

/* the manager made: no buffers, room for 128, the render map sizes from their defaults, a new
 * buffer's default fractions (SOCOM 1's). Called out of line by the unit's own initialiser
 * (0x403580), which -inline auto would inline it into: pinned */
#pragma auto_inline off
CTexManager::CTexManager()
{
    m_renderMapNum = data_003E02E0;
    m_renderMapNumBytes = data_003E02E8;
    m_shadowMapSize = data_003E02F0;
    m_reflectionMapSize = data_003E02F8;
    TexBufParams[0] = 0.2f;
    TexBufParams[1] = 0.6f;
    TexBufParams[2] = 0.4f;
    TexBufParams[3] = 0.8f;
    m_buffer.reserve(128);
}
#pragma auto_inline reset

/* a buffer made by name between two fractions of the texture memory and listed, its index given
 * back (SOCOM 1's) */
int CTexManager::doAddBuffer(const char *name, float start, float end)
{
    CGSTexBuffer *buf = new CGSTexBuffer;

    buf->SetName(name);
    buf->Setup(start, end);
    if (m_buffer.size() == m_buffer.capacity())
        m_buffer.reserve(m_buffer.size() + 1);
    m_buffer.push_back(buf);
    return m_buffer.size() - 1;
}

/* a buffer taken off the list and deleted (SOCOM 1's) */
void CTexManager::doDelBuffer(CGSTexBuffer *buf)
{
    std::vector<CGSTexBuffer *>::iterator e = m_buffer.end();
    std::vector<CGSTexBuffer *>::iterator i = std::remove(m_buffer.begin(), e, buf);

    m_buffer.erase(i, e);
    delete buf;
}

/* every buffer's packet buffer wiped (SOCOM 1's, the wipe SOCOM II's out of line) */
void CTexManager::WipePktBuf()
{
    unsigned int n = m_texmanager.m_buffer.size();

    for (unsigned int i = 0; i < n; i++)
        if (m_texmanager.m_buffer[i])
            m_texmanager.m_buffer[i]->WipePktBuf();
}

/* the buffers made: dyntex.rdr's boundaries give the first fraction, the second halfway from it to
 * the top, the third and fourth halfway again; then each render phase's buffers between them, in
 * SOCOM II's set (SOCOM 1's doSetupBuffers, which took an unused name and inlined doAddBuffer; the
 * state core calls this one on the manager, its body naming the manager itself). The second
 * fraction reads the boundary itself (through TexBufParams[0] the load is CSE'd and [0] never
 * reloaded), the fractions are written 0-1-2-3 (the target's stores fall 0-1-3-2), and the loop
 * runs to `<= 13` (`< 14` tests into v0 where the target's slti writes at) */
void CTexManager::doSetupBuffers()
{
    char name[64];
    _zrdr *rdr = zrdr_read("dyntex.rdr", 0, 0);

    if (rdr) {
        PNT2D bounds;

        if (zrdr_findPNT2D(rdr, "boundaries", &bounds)) {
            TexBufParams[0] = bounds.x;
            TexBufParams[1] = bounds.x + 0.5f * (1.0f - bounds.x);
            TexBufParams[2] = TexBufParams[0] + 0.5f * (TexBufParams[1] - TexBufParams[0]);
            TexBufParams[3] = TexBufParams[1] + 0.5f * (1.0f - TexBufParams[1]);
        }
        zrdr_free((CRdrFile *)rdr);
    }
    TexPhase[15][0] = m_texmanager.doAddBuffer("TexShadow:A", 0.0f, TexBufParams[0]);
    for (int i = 12; i <= 13; i++) {
        sprintf(name, "TexHUD%d:B1", i - 12);
        TexPhase[i][0] = m_texmanager.doAddBuffer(name, TexBufParams[0], TexBufParams[1]);
        sprintf(name, "TexHUD%d:B2", i - 12);
        TexPhase[i][1] = m_texmanager.doAddBuffer(name, TexBufParams[1], 1.0f);
    }
    TexPhase[14][0] = m_texmanager.doAddBuffer("TexHUDSmall:A", 0.0f, TexBufParams[0]);
    TexPhase[2][0] = m_texmanager.doAddBuffer("TexCommonCS:B1", TexBufParams[0], TexBufParams[1]);
    TexPhase[2][1] = m_texmanager.doAddBuffer("TexCommonCS:B2", TexBufParams[1], 1.0f);
    TexPhase[11][0] = m_texmanager.doAddBuffer("TexAlpha:B1", TexBufParams[0], TexBufParams[1]);
    TexPhase[11][1] = m_texmanager.doAddBuffer("TexAlpha:B2", TexBufParams[1], 1.0f);
    TexPhase[0][0] = m_texmanager.doAddBuffer("TexWorld:B", TexBufParams[0], 1.0f);
    TexPhase[16][0] = m_texmanager.doAddBuffer("TexCustomCS:0", 0.0f, 0.001f);
    TexPhase[16][1] = m_texmanager.doAddBuffer("TexCustomCS:1", 0.0f, 0.001f);
    TexPhase[16][2] = m_texmanager.doAddBuffer("TexCustomCS:2", 0.0f, 0.001f);
    TexPhase[17][0] = m_texmanager.doAddBuffer("TexCustomNS:0", 0.0f, 0.001f);
    TexPhase[17][1] = m_texmanager.doAddBuffer("TexCustomNS:1", 0.0f, 0.001f);
    TexPhase[17][2] = m_texmanager.doAddBuffer("TexCustomNS:2", 0.0f, 0.001f);
    TexPhase[18][0] = m_texmanager.doAddBuffer("Subtitles", 0.0f, 1.0f);
}

/* the buffers deleted from the last, all of them or all but the first, and every render phase's
 * buffer indices cleared (SOCOM 1's) */
bool CTexManager::DestroyBuffers(CTexManager &tm, bool all)
{
    int keep = 1;

    if (all)
        keep = 0;
    while (tm.m_buffer.size() > keep) {
        delete tm.m_buffer.back();
        tm.m_buffer.pop_back();
    }
    for (int i = 0; i < 19; i++)
        for (int j = 0; j < 3; j++)
            TexPhase[i][j] = 0;
    return true;
}

/* a buffer's packet vector emptied and given up, its chain dropped (ours, inline: seven high with
 * FreeVector, so ClearPktBufs inlines it one level and calls clear() and FreeVector out of line --
 * the copies 0x33BA60 and 0x355FE0 -- where WipePktBuf, written out, inlines clear() whole and
 * FreeVector one level. zrndr_pipe.cpp's 0x33B960, the same code out of line, may be this inline's
 * copy) */
static inline void ClearPktBuf(CGSTexBuffer *buf)
{
    buf->m_pktbuf.clear();
    FreeVector(buf->m_pktbuf);
    buf->m_chainp = 0;
}

/* the packet buffers emptied and given up, the chains dropped -- all but the first buffer's unless
 * all (SOCOM 1's) */
void CTexManager::ClearPktBufs(CTexManager &tm, bool all)
{
    int n = 0;

    for (std::vector<CGSTexBuffer *>::iterator i = tm.m_buffer.begin(); i != tm.m_buffer.end(); ++i, ++n)
        if (n || all)
            ClearPktBuf(*i);
}

/* the buffer's palettes and textures moved to their places from its start, each texture's selects
 * rewritten to the new place (SOCOM 1's UpdateTextureSelects, the interrupt's after a load).
 * OPEN, 11 instructions, register-only: both walks' end tests load the size into a0 and the cursor
 * into a1 (here a1 and a0), and the palette test branches `beq next, address` where the operands
 * written that way load into the other registers (written `address != next`, the loads are the
 * target's and the beq's operands swap: 23 the other way). The texture walk matches with the next
 * address taken first and the texture before the old address. Tried: `!(i == end())` (0x120), `end()
 * != i` (0x144), the palette's address through a local (16), *i read twice (0x148) */
void CGSTexBuffer::UpdateTextureSelects()
{
    m_nextAddress = m_startAddress;
    for (CTexPalList::iterator i = m_dyntex_list.m_pal_list.begin(); i != m_dyntex_list.m_pal_list.end(); i++) {
        CTexPalette *pal = *i;

        if (pal->m_gsaddr != m_nextAddress)
            pal->m_gsaddr = m_nextAddress;
        m_nextAddress += (unsigned int)pal->m_size >> 8;
    }
    for (CTexList::iterator i = m_dyntex_list.m_tex_list.begin(); i != m_dyntex_list.m_tex_list.end(); i++) {
        unsigned int addr = m_nextAddress;
        CTexHandle *h = *i;
        CTexture *tex = h->m_texture;
        unsigned int old = h->m_gsAddr;

        h->m_gsAddr = addr;
        m_nextAddress = tex->UpdateTextureSelect(old, m_nextAddress);
    }
    asm volatile { sync }
}

/* the TEX0 register's texture and palette addresses rewritten -- the texture's where it was at the
 * old address, the palette's to its palette's -- written back cached and uncached when changed,
 * then each mip level's MIPTBP registers; the next free address given back (SOCOM II's) */
unsigned int CTexture::UpdateTextureSelect(unsigned int oldAddr, unsigned int newAddr)
{
    sceGsTex0 tex0 = *(sceGsTex0 *)m_TEX0;
    bool changed = false;

    if (tex0.TBP0 != newAddr) {
        tex0.TBP0 = newAddr;
        changed = true;
    }
    for (std::vector<CTexture *>::iterator i = m_reverse_miplist.begin(); i != m_reverse_miplist.end(); ++i)
        (*i)->MIPUpdateTextureSelects(oldAddr, newAddr);
    if (m_palette && tex0.CBP != m_palette->m_gsaddr) {
        tex0.CBP = m_palette->m_gsaddr;
        changed = true;
    }
    if (changed) {
        *(sceGsTex0 *)m_TEX0 = tex0;
        *(sceGsTex0 *)UNCACHED(m_TEX0) = tex0;
    }
    return newAddr + m_gsWords;
}

/* the MIPTBP registers' levels at the old address moved to the new one, each register written back
 * cached and uncached when changed (SOCOM 1's, which rewrote each field in place) */
void CTexture::MIPUpdateTextureSelects(unsigned int oldAddr, unsigned int newAddr)
{
    if (m_MIP1) {
        sceGsMiptbp1 mip = *(sceGsMiptbp1 *)m_MIP1;
        bool changed = false;

        if (mip.TBP1 == oldAddr) {
            mip.TBP1 = newAddr;
            changed = true;
        }
        if (mip.TBP2 == oldAddr) {
            mip.TBP2 = newAddr;
            changed = true;
        }
        if (mip.TBP3 == oldAddr) {
            mip.TBP3 = newAddr;
            changed = true;
        }
        if (changed) {
            *(sceGsMiptbp1 *)m_MIP1 = mip;
            *(sceGsMiptbp1 *)UNCACHED(m_MIP1) = mip;
        }
    }
    if (m_MIP2) {
        sceGsMiptbp1 mip = *(sceGsMiptbp1 *)m_MIP2;
        bool changed = false;

        if (mip.TBP1 == oldAddr) {
            mip.TBP1 = newAddr;
            changed = true;
        }
        if (mip.TBP2 == oldAddr) {
            mip.TBP2 = newAddr;
            changed = true;
        }
        if (mip.TBP3 == oldAddr) {
            mip.TBP3 = newAddr;
            changed = true;
        }
        if (changed) {
            *(sceGsMiptbp1 *)m_MIP2 = mip;
            *(sceGsMiptbp1 *)UNCACHED(m_MIP2) = mip;
        }
    }
}

/* the chain dropped (SOCOM II's, out of line from every caller here: pinned) */
#pragma auto_inline off
void CGSTexBuffer::DropChain()
{
    m_chainp = 0;
}
#pragma auto_inline reset

/* the manager (SOCOM 1's static: its initialiser builds it and registers its destructor) */
CTexManager CTexManager::m_texmanager;

}
