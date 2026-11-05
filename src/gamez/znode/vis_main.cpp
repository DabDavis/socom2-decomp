/* vis_main.cpp -- zdb::CVisual, a node's visual: its reading, creation, construction and texture
 * selects (SOCOM 1's vis_main.cpp, by the press beta's asserts: UpdateCentroid and
 * GetTextureSelectBlock_ver2 carry its name): OVL1 0x3C3570?-0x3C469F, after vis_texfx.cpp and
 * before zwep_list.cpp. SOCOM 1's unit also held the VU code (VuUpdate .. Init), which SOCOM II
 * links earlier (0x3B3AB0-0x3B7FDF, with vis_main's static initialiser's statics); which file each
 * half was is not settled. Only some of its functions so far; the rest stay generated drafts or
 * placeholders. Defined in reverse of their image order (-inline deferred). */

#include <stack>
#include "znode.h"
#include "../zar/zar.h"
#include "../zsys/zsys_public.h"

namespace zdb {

/* a decal on a visual: what its view's list walk reads (vis_decal.cpp's class; SOCOM 1's, no DWARF
 * -- the names ours) */
class CDecal {
public:
    CDecal *m_next;   // 0x0
    CVisual *m_visual;   // 0x4
    unsigned char _8[0xc - 0x8];
    unsigned short m_view;   // 0xc
};

}

using namespace zdb;

/* MSL's __vector_pod<unsigned int>::init(n, value): n copies of the value in a new block -- the
 * template tail's, after its one caller (SOCOM 1's init__Q23std38__vector_pod<Ui,...>FUiRCUi, the
 * same 0xE8 bytes). include/vector does not model it yet, so it is written here over the storage's
 * three words, under its placeholder */
struct __pod_storage {
    unsigned int *&data() { return data_; }
    unsigned int &cap() { return capacity_; }
    static unsigned int *allocate(unsigned int n)
    {
        unsigned int *p = (unsigned int *)operator new(n * sizeof(unsigned int));

        if (!p) {
            fprintf(stderr, "Memory allocation failure");
            abort();
        }
        return p;
    }

    unsigned int capacity_;
    unsigned int size_;
    unsigned int *data_;
};

/* (pinned: MSL's is an out-of-class template member, never inlined into the unit's own code) */
#pragma auto_inline off
extern "C" void func_003C45B0(void *v, unsigned int n, const unsigned int &value)
{
    __pod_storage *s = (__pod_storage *)v;

    if (n) {
        if (n > (unsigned int)-1 / sizeof(unsigned int)) {
            fprintf(stderr, "vector length error\n");
            abort();
        }
        s->data() = s->allocate(n);
        s->size_ = n;
        s->cap() = n;
        for (unsigned int *first = s->data_; n > 0; --n, ++first)
            *first = value;
    }
}
#pragma auto_inline reset

/* constructed empty: the flags' defaults, no type, its parameters zeroed and the centroid at the
 * origin (SOCOM 1's, store for store). Byte-exact at -inline level=5 and not at 4 (0x20C against
 * 0x218 there): the target builds the dynamic-texture lists in place, CDynTexList's implicit
 * constructor over CTexList's (CTexList, vector, __vector_imp, __vector_pod: five high), which
 * level 4 inlines one level only. The byte at 4 is stored through an unsigned view (`li 255`, as
 * SOCOM 1's), where the destructor reads it signed */
CVisual::CVisual() : m_refs(0), m_lodMesh(0)
{
    m_flags = 0;
    m_bits.m_b1 = 1;
    m_bits.m_b19 = 1;
    m_bits.m_b3 = 1;
    m_bits.m_b4 = 1;
    m_bits.m_b5 = 1;
    m_bits.m_b6 = 1;
    m_bits.m_b7 = 1;
    m_bits.m_b9 = 0;
    m_bits.m_multipass = 1;
    m_bits.m_b15 = 1;
    m_bits.m_decals = 1;
    m_bits.m_b17 = 0;
    m_bits.m_b18 = 0;
    m_6 = 0;
    m_7 = 0;
    m_5 = 0;
    *(unsigned char *)&m_type = 255;
    m_1c = 3;
    m_8s.m_8 = 0;
    m_8s.m_a = 0;
    m_centroid = CPnt3D::zero;
    m_numDetails = 0;
    m_details = 0;
}

/* OVL1's copy of the pod vector's resize(n, value) (include/vector does not model it: MAIN's
 * 0x18B050 carries the name) */
extern "C" void func_0028B0E0(void *v, unsigned int n, const unsigned int &value);

/* the stack's blocks grown for 32 ids and emptied again, the three map lists sized (SOCOM 1's) */
void CVisual::Init()
{
    int i;

    for (i = 0; i < 32; i++)
        m_stack_vid.push((void *)i);
    for (i = 0; i < 32; i++)
        m_stack_vid.pop();
    func_0028B0E0(&m_lightMapList, 16, 0);
    func_0028B0E0(&m_shadowMapList, 4, 0);
    func_0028B0E0(&m_projectedMapList, 8, 0);
    CMesh::Init();
}

/* a visual of the archive's type, read; none if the read fails (SOCOM 1's) */
CVisual *CVisual::Create(zar::CZAR &zar)
{
    int type = -1;

    zar.Fetch("vtype", &type);
    CVisual *v = 0;
    switch (type) {
    case -1:
        v = new CVisual;
        break;
    case 0:
        v = new CVisual;
        break;
    case 1:
        v = new CMesh;
        break;
    case 2:
        v = new CSubMesh(0);
        break;
    }
    if (v && !v->Read(zar)) {
        delete v;
        v = 0;
    }
    return v;
}

/* the specular maps' storage given back (SOCOM 1's): FreeVector over the base pointer vector -- the
 * target's temporary is a vector<CLightMap *>, not the CLightMapVector the list is */
void CVisual::Flush()
{
    FreeVector<std::vector<CLightMap *> >(m_specMapList);
}

/* one holder fewer, the count left given back (SOCOM 1's) */
int CVisual::Release()
{
    m_refs--;
    return m_refs;
}

/* a plain visual's buffers let go of, each view's: their own blocks freed, their owner released
 * and deleted with its last holder; then the detail block, and the members (SOCOM 1's, the
 * owner's destructor inline here) */
CVisual::~CVisual()
{
    if (m_type == -1 || m_type == 0) {
        for (std::vector<CMesh *>::iterator it = m_lodMesh.begin(), e = m_lodMesh.end(); it != e; ++it) {
            _word128 *buf = (_word128 *)*it;

            if (buf) {
                _word128 *p = buf + 1;
                unsigned int i;
                unsigned int n = buf->u16[0];

                for (i = 0; i < n; i++, p++) {
                    if (p->u8[2] == 8 && p->u32[1])
                        _free((void *)p->u32[1], 0, 0);
                }
                CVisBase *base = buf ? (CVisBase *)buf->u32[2] : 0;

                if (base->Release() == 0)
                    delete base;
                *it = 0;
            }
        }
    }
    m_lodMesh.clear();
    if (m_details)
        _free(m_details, 0, 0);
    m_details = 0;
    m_details = 0;
    m_numDetails = 0;
}

/* a texture-select block's head: zeroed, then one flag set (the names ours) */
struct TEXSELECT_HEAD {
    unsigned int m_0;
    unsigned int m_4;
    unsigned int m_8;
    unsigned int m_c0 : 1;
    unsigned int m_ver2 : 1;
    unsigned int m_c2 : 30;
};

/* a texture-select block of its own: n selects between a head and an end, allocated, the cursor
 * set past the head, the head returned -- it stays in v0 from _calloc to the end, and the caller
 * (CTexture::SetupSpecialTextures) stores it from there (SOCOM 1's; _ver2 builds the same in
 * place at the cursor) */
_word128 *CVisual::GetTextureSelectBlock(unsigned int n, _word128 **cursor, _word128 *ref,
                                         _word128 *tag)
{
    unsigned int size = n + 2;
    _word128 *head = (_word128 *)_calloc(1, size * sizeof(_word128), 0, 0);

    *cursor = head + 1;
    ((TEXSELECT_HEAD *)head)->m_ver2 = 1;
    _word128 *end = *cursor + n;
    end->u32[0] = 0x15000000;
    end->u32[1] = 0;
    end->u32[2] = 0;
    end->u32[3] = 0;
    ref->u32[0] = n | 0x30000000;
    ref->u32[1] = (unsigned int)*cursor;
    ref->u32[2] = 0x11000000;
    ref->u32[3] = n | 0x50000000;
    tag->u32[0] = size | 0x30000000;
    tag->u32[1] = (unsigned int)head;
    tag->u32[2] = 0x01000101;
    tag->u32[3] = ((n + 1) << 16) | 0x6C008000;
    return head;
}

_word128 *CVisual::GetTextureSelectBlock_ver2(unsigned int n, _word128 **cursor, _word128 *ref,
                                         _word128 *tag)
{
    _word128 *head = (*cursor)++;

    head->u128 = 0;
    ((TEXSELECT_HEAD *)head)->m_ver2 = 1;
    _word128 *end = *cursor + n;
    end->u32[0] = 0x15000000;
    end->u32[1] = 0;
    end->u32[2] = 0;
    end->u32[3] = 0;
    ref->u32[0] = n | 0x30000000;
    ref->u32[1] = (unsigned int)*cursor;
    ref->u32[2] = 0x11000000;
    ref->u32[3] = n | 0x50000000;
    tag->u32[0] = (n + 2) | 0x30000000;
    tag->u32[1] = (unsigned int)head;
    tag->u32[2] = 0x01000101;
    tag->u32[3] = ((n + 1) << 16) | 0x6C008000;
    return head;
}

CTexture *CVisual::ResolveTextureName(_word128 *names, _word128 *select)
{
    CTexture *tex = 0;
    CTexHandle *h = CWorld::GetTexHandle((char *)names + select->u32[1]);

    if (!h)
        h = CWorld::GetTexHandle("null_xmas.bmp");
    if (h) {
        tex = h->m_texture;
        m_dynTexList.Add(h, true);
        if (tex->m_palette)
            m_dynTexList.Add(tex->m_palette, true);
    }
    if (tex) {
        select->u128 = tex->m_dmaRefVu;
        select->u8[2] = 5;
    }
    return tex;
}

/* whether the views' buffers share their owners' counts (a byte in bss, 0x4B5010: set, SetBuffer
 * leaves the owner's count alone; not named) */
extern "C" unsigned char data_004B5010;

/* a texture found by name, the null texture's when it is missing, its handle and palette listed
 * (ours: SetBuffer's two lookups) */
static inline CTexture *FindTexture(CDynTexList &list, const char *name)
{
    CTexture *tex = 0;
    CTexHandle *h = CWorld::GetTexHandle(name);

    if (!h)
        h = CWorld::GetTexHandle("null_xmas.bmp");
    if (h) {
        tex = h->m_texture;
        list.Add(h, true);
        if (tex->m_palette)
            list.Add(tex->m_palette, true);
    }
    return tex;
}

/* a view's VU buffer kept for this visual (SOCOM 1's SetBuffer, 0x3C38C0): its owner counted (unless
 * the flag at 0x4B5010 says not), the buffer stored for the view, and once per buffer (the head's
 * flag) the owner noted and the head's list made ready -- texture names (6) resolved, the select
 * before a blend list (3) shortened by one, a texture handle's (5) texture remembered, a blend list
 * (3) under a texture copied into a block of its own when it has entries (kept in the blend words)
 * and made absolute otherwise, the plain references (1, 2, 4, 7) made absolute; the detail layers'
 * texture names resolved once (their flag at 6). The switch lists all thirteen kinds so MW builds the
 * target's 13-entry jump table (with only the handled kinds it builds 8, with case 12 alone a compare
 * chain); the lookup an inline shared by both loops and the select copied in by the caller, through
 * the kind pointer (ResolveTextureName inlined stores through the select: the kind and count
 * registers swap). OPEN, 0x368 against 0x370: the target takes &m_lodMesh's data word (this+56) once
 * at the top and spills it (sp+176) for the last store, where here the last store reads the data word
 * off this again (the frame 16 smaller); the detail walk's two counters and offset take s1/s4/s2 in
 * the target, s4/s3/s1 here. Tried: the vector, its pointer or data()'s reference in a local (each
 * keeps this+48 or the word itself, 8 long), the reference taken only for the last store */
void CVisual::SetBuffer(_word128 *buf, unsigned int vid, CVisBase *base)
{
    if (!data_004B5010)
        base->m_refs++;

    char *names = (char *)base->m_data;

    m_lodMesh[vid] = (CMesh *)buf;
    _word128 *h = (_word128 *)m_lodMesh[vid];

    if (h->u8[2])
        return;
    h->u32[2] = (unsigned int)base;

    CTexture *tex = 0;
    unsigned int n = h->u16[0];
    _word128 *p = h + 1;

    for (unsigned int i = 0; i < n; i++, p++) {
        unsigned char *kind = &p->u8[2];

        switch (*kind) {
        case 6:
            tex = FindTexture(m_dynTexList, names + p->u32[1]);
            if (tex) {
                p->u128 = tex->m_dmaRefVu;
                *kind = 5;
            }
            if (p[1].u8[2] == 3)
                p->u16[0]--;
            break;
        case 5:
            if (!tex)
                tex = ((CTexHandle *)p->u32[1])->m_texture;
            break;
        case 3:
            if (tex) {
                if (p->u16[6]) {
                    unsigned int k = p->u16[0];
                    _word128 *f = (_word128 *)(p->u32[1] + (unsigned int)names);
                    _word128 *v = (_word128 *)_malloc(k * 16, 0, 0);
                    _word128 *t = v;

                    do {
                        *t = *f;
                        f++;
                        t++;
                    } while (--k);
                    p->u32[1] = (unsigned int)v;
                    *kind = 8;
                    BLENDMODE *bm = (BLENDMODE *)v;

                    m_blendmodes.push_back(bm);
                } else
                    p->u32[1] += (unsigned int)names;
            }
            break;
        case 1:
        case 2:
        case 4:
        case 7:
            p->u32[1] += (unsigned int)names;
            break;
        case 0:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            break;
        }
    }

    DETAIL *d = m_details;

    if (d && !d->m_6) {
        for (unsigned int j = 0; j < m_numDetails; j++, d++) {
            for (unsigned int e = 0; e < d->m_count; e++) {
                d->m_data[e].m_tex = FindTexture(m_dynTexList, names + (unsigned int)d->m_data[e].m_tex);
            }
        }
        m_details->m_6 = 1;
    }
    ((_word128 *)m_lodMesh[vid])->u8[2] = 1;
}

/* a view's VU buffer copied for this visual: a new owner of the same size (its constructor inline
 * in SOCOM II), the block copied, and when the head's list is open, each select of the list whose
 * kind is 5 made relative to the source's block and each of kind 8 given its own copy (SOCOM 1's).
 * The copies are do-while loops (no entry test), the flag's address taken once, the cursor
 * stepped in the loop's increment, the kinds a switch (tested 5 first, kind 8's code first). OPEN,
 * 8 bytes long and 7 instructions off: the switch's second test is `beq 8; b out` where the
 * target branches `bne 8, out` into kind 8's code (nested ifs not tried), and the block copy's
 * two loads are scheduled ahead of the length where the target loads them after it */
_word128 *CVisual::CopyBuffer(CVisBase **out, unsigned int vid)
{
    CVisBase *src = (CVisBase *)((_word128 *)m_lodMesh[vid])->u32[2];
    _word128 *sh;
    char *base = (char *)src->m_data;

    sh = (_word128 *)(base + src->m_c);

    CVisBase *b = new CVisBase(src->m_size, false);

    b->m_c = src->m_c;

    _word128 *d = (_word128 *)b->m_data;
    _word128 *s = (_word128 *)src->m_data;
    int n = (int)src->m_size / 16;

    do {
        *d = *s;
        s++;
        d++;
    } while (--n);

    _word128 *h = (_word128 *)((char *)b->m_data + b->m_c);
    unsigned char *open = &h->u8[2];

    if (*open == 1) {
        *open = 0;
        sh++;

        unsigned int i = 0;
        unsigned int count = h->u16[0];

        h++;
        for (; i < count; i++, h++, sh++) {
            switch (h->u8[2]) {
            case 8: {
                unsigned int k = h->u16[0];
                _word128 *v = (_word128 *)_malloc(k * 16, 0, 0);
                _word128 *t = v;
                _word128 *f = (_word128 *)h->u32[1];

                do {
                    *t = *f;
                    f++;
                    t++;
                } while (--k);
                h->u32[1] = (unsigned int)v;
                break;
            }
            case 5:
                h->u32[1] = sh->u32[1] - (unsigned int)base;
                break;
            }
        }
    }
    *out = b;
    return (_word128 *)((char *)b->m_data + b->m_c);
}

/* the head's parameters, then the detail layers: their count, the block's size and the block,
 * each layer's data pointer set into the block after the layers (SOCOM 1's) */
bool CVisual::Read(zar::CZAR &zar)
{
    zar.Fetch("vparams", this, 0x18);
    if (m_bits.m_b10)
        m_1c = -2;
    else
        m_1c = 3;
    if (m_details)
        _free(m_details, 0, 0);
    m_details = 0;
    m_numDetails = 0;
    zar.Fetch("detail_cnt", &m_numDetails, 4);
    if (m_numDetails) {
        unsigned int size;

        zar.Fetch("detail_size", &size, 4);
        m_details = (DETAIL *)_malloc(size, 0, 0);
        zar.Fetch("detail_buff", m_details, size);
        DETAIL *d = m_details;
        char *data = (char *)(d + m_numDetails);

        for (unsigned int i = 0; i < m_numDetails; i++, d++) {
            d->m_data = (tag_DETAIL_PARAMS::ENTRY *)data;
            data += d->m_count * 12;
        }
    }
    return true;
}

/* a decal unlinked from its view's list, its link cleared (SOCOM 1's; the image keeps it with the
 * other decal members at 0x3B37A0, ahead of the VU code -- which unit that stretch is, is not
 * settled, so it is written here with CVisual's) */
void CVisual::RemoveDecal(CDecal *decal)
{
    CDecal *d = (CDecal *)m_24[decal->m_view];
    CDecal *prev = 0;

    while (d) {
        if (d == decal) {
            CDecal *next = d->m_next;

            if (prev)
                prev->m_next = next;
            else
                m_24[decal->m_view] = next;
            d->m_next = 0;
            break;
        }
        prev = d;
        d = d->m_next;
    }
}
