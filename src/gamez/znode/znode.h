/* zdb::CNode -- the scene graph's node (the zdb namespace is the mangled names'), its layout
 * SOCOM 1's DWARF as tools/dwarf1.py header writes it (MW puts the vtable pointer at 0x60, after
 * tag_NODE_PARAMS). Its virtual functions are SOCOM II's table, __vt__Q23zdb5CNode at 0x4067B0
 * (tools/s1vtables.py): SOCOM 1's fourteen with one new slot after the FindChild pair, so IsCell
 * and everything after it sit 4 bytes further than the demo's. SOCOM II keeps tag_NODE_PARAMS'
 * flags at 0x5C (zAnimNodeActiveTick reads m_active there). The header's name is ours. */
#ifndef ZNODE_H
#define ZNODE_H

#include <deque>
#include <eetypes.h>
#include <list>
#include <vector>

#include "../zmath/zmath.h"

class CZProjectile;
/* a quadword seen every way (zsys_public.h's union: the visuals' texture selects are arrays of them) */
union _word128;
namespace zar {
class CZAR;
}
class C2DVisual;
class CBBox;

/* the node count (SOCOM 1's numNodes, a global outside zdb, after NodeUniverse) */
extern int numNodes;

namespace zdb {

class tag_NODE_PARAMS;
/* a node's parameters set: from another's, or the defaults when given none (SOCOM 1's) */
void InitNodeParams(tag_NODE_PARAMS *to, const tag_NODE_PARAMS *from);

class CCamera;
class CGridAtom;
class CLightMap;
class CModel;
class CNode;
struct IntersectStruct;

/* a node's extension: SOCOM 1's DWARF has it as its vtable pointer alone, with seven virtual
 * functions in the demo's names; SOCOM II's table (__vt__Q23zdb7CNodeEx) keeps three, all inline
 * -- their out-of-line copies come out wherever an extension is constructed (CZAnim's at
 * 0x2742F0 and 0x274300; the third's linked copy is an earlier unit's, 0x216760). The last two
 * are not named */
class CNodeEx {
public:
    virtual int OnWeaponHit(CNode *node, IntersectStruct *hit, CZProjectile *projectile) { return -1; }
    virtual int Slot0C() { return 0; }
    virtual bool Slot10(float amount) { return false; }
};   // 4 bytes

class tag_NODE_PARAMS {
public:
    /* the facade cleared first (CNode's constructor, SOCOM 1's too, writes it with a word access
     * ahead of the vtable pointer) */
    tag_NODE_PARAMS() { m_facade = 0; }

    CMatrix m_matrix;   // 0x0
    CBBox m_bbox;   // 0x40
    unsigned short m_type;   // 0x58 (SOCOM II's CNode code reads a halfword; SOCOM 1's DWARF: an int)
    /* SOCOM II's: two bits set through a whole tree (OVL2 sets 2 on models it loads), the rest
     * unread here -- the names are ours */
    unsigned short m_socom2_mode : 2;   // 0x5a
    unsigned short m_socom2_5a_bit2 : 1;   // 0x5a (InitNodeParams copies it)
    unsigned short m_socom2_5a : 13;   // 0x5a (InitNodeParams clears it)
    unsigned int m_active : 1;   // 0x5c
    unsigned int m_dynamic_motion : 1;   // 0x5c
    unsigned int m_dynamic_light : 1;   // 0x5c
    unsigned int m_landmark : 1;   // 0x5c
    unsigned int m_light : 1;   // 0x5c
    unsigned int m_prelight : 1;   // 0x5c
    unsigned int m_fog : 1;   // 0x5c
    unsigned int m_transparent : 1;   // 0x5c
    unsigned int m_facade : 2;   // 0x5c
    unsigned int m_reflective : 1;   // 0x5c
    unsigned int m_bumpmap : 1;   // 0x5c
    unsigned int m_hasDI : 1;   // 0x5c
    unsigned int m_region_shift : 5;   // 0x5c
    unsigned int m_has_visuals_prior_to_export : 1;   // 0x5c
    unsigned int m_shadow : 1;   // 0x5c
    unsigned int m_worldchild : 1;   // 0x5c
    unsigned int m_char_common : 1;   // 0x5c
    unsigned int m_NOTUSED : 1;   // 0x5c
    unsigned int m_hasVisuals : 1;   // 0x5c
    unsigned int m_hasMesh : 1;   // 0x5c
    unsigned int m_scrolling_texture : 1;   // 0x5c
    unsigned int m_light_dynamic : 1;   // 0x5c
    unsigned int m_light_static : 1;   // 0x5c
    unsigned int m_clutter : 1;   // 0x5c
    unsigned int m_mtx_is_identity : 1;   // 0x5c
    unsigned int m_use_parent_bbox : 1;   // 0x5c
    unsigned int m_apply_clip : 1;   // 0x5c
};   // 0x60 bytes

struct _opaque_vector_c { unsigned char _[0xc]; };   // vector (a template instance, or unresolved)

class CNodeVector : public std::vector<CNode *> {
public:
    bool Remove(const CNode *node);
    bool Exists(const CNode *node);
    CNode *GetNode(const char *name);
};   // 0xc bytes

/* a node's collision piece: only its reference count placed (SOCOM 1's DWARF has it at 0x14 of
 * DI_PARAMS; SOCOM II's AddDI counts at 4) and its vtable pointer -- at 0xc, where SOCOM 1's
 * sat at 0x1c, after its parameters (DeleteDI deletes through the first slot) */
struct tag_DI_HEAD {
    /* SOCOM 1's DI_PARAMS constructor: the region and the count zeroed, then every flag field
     * through its setter -- a constant passed to an inline setter is masked at run time, `(0 & 3)`
     * -- SOCOM II's four new fields among them (bit 26 set, the three bits at 28 set to 4; CDI's
     * constructor inlines it, 0x2DC4C0; the setters' names are ours) */
    tag_DI_HEAD()
    {
        _0 = 0;
        m_refcount = 0;
        SetDiType(0);
        SetPtCount(0);
        SetMaterial(0);
        SetCameraType(0);
        SetAppFlags(0);
        SetInside(0);
        SetFlag27(false);
        SetBit26(true);
        SetBit31(false);
        SetShadow(0);
        SetBits28(4);
    }
    void SetDiType(int n) { m_flags.m_ditype = n; }
    void SetPtCount(int n) { m_flags.m_ptcount = n; }
    void SetMaterial(int n) { m_flags.m_material = n; }
    void SetCameraType(int n) { m_flags.m_cameratype = n; }
    void SetAppFlags(int n) { m_flags.m_appflags = n; }
    void SetInside(int n) { m_flags.m_inside = n; }
    void SetShadow(int n) { m_flags.m_shadow = n; }
    void SetBit26(bool b) { m_flags.m_reserved26 = b; }
    void SetFlag27(bool b) { m_flags.m_flag27 = b; }
    void SetBits28(int n) { m_flags.m_bits28 = n; }
    void SetBit31(bool b) { m_flags.m_bit31 = b; }

    unsigned int _0;   // 0x0 (SOCOM 1's m_region, after its normal)
    unsigned int m_refcount;   // 0x4
    /* SOCOM 1's DI_PARAMS flags, at 8 here (SetMaterial writes the material's eight bits at bit
     * 10, SetDIBBox the type's two at bit 0) */
    union {
        unsigned int _8;
        struct {
            unsigned int m_ditype : 2;   // 0x8
            unsigned int m_ptcount : 8;
            unsigned int m_material : 8;
            unsigned int m_cameratype : 2;
            unsigned int m_appflags : 3;
            unsigned int m_inside : 1;
            unsigned int m_shadow : 2;
            unsigned int m_reserved26 : 1;
            /* SOCOM II's: IntersectStruct::GetPolyFlag27 reads it (the name ours) */
            unsigned int m_flag27 : 1;
            /* SOCOM II's: the constructor sets 4 and clears bit 31 (the names ours) */
            unsigned int m_bits28 : 3;
            unsigned int m_bit31 : 1;
        } m_flags;
    };
};

class DiIntersect;
class CSaveLoad;

class CDI : public tag_DI_HEAD {
public:
    CDI();
    virtual ~CDI();
    /* SOCOM II's table (__vt__Q23zdb3CDI, 0x406650): the destructor, Allocate, Free, two pure
     * slots (SOCOM 1's GetIntersect and GetAltitude, each shape's own) and Read */
    virtual void Allocate(int n);
    virtual void Free();
    virtual int GetIntersect(DiIntersect *di) = 0;
    virtual int GetAltitude(DiIntersect *di) = 0;
    virtual bool Read(CSaveLoad &io);
    /* one holder fewer, the count left given back (SOCOM 1's, 0x14 bytes in both) */
    int Release();
    /* a polygon read from the archive's open key (SOCOM 1's static) */
    static CDI *Create(CSaveLoad &io);
    /* the faces a query skips (SOCOM 1's static: CGrid::GetAltitude sets 32) */
    static int m_backface_mask;
    /* the normal of the first three points (SOCOM 1's; SOCOM II keeps it in a static) */
    void SetEdgeVecsAndNorm();
    /* the edges a point's vertical crosses, their count; the two edge points through a point
     * (SOCOM 1's) */
    int GetEdgeIntersectsY(CPnt3D *p, CPnt3D *out);
    bool GetEdgeIntersects(CPnt3D *p, CPnt3D *a, CPnt3D *b);

    CPnt4D *m_pnts;   // 0x10: its points, 16 bytes each (Allocate mallocs them; the name ours)
};

class CDIVector : public std::vector<CDI *> {
public:
    /* the first intersection stops a search, back faces are not returned (CNode's constructor) */
    CDIVector()
    {
        m_bStopAtFirstIntersection = 1;
        m_bReturnBackFaceIntersections = 0;
    }
    bool Exists(const CDI *di);
    /* a DI pushed, taken by value (AddDI's target pushes a copy, as AddVisual's; the name ours) */
    void Add(CDI *di) { push_back(di); }

public:
    unsigned char m_bStopAtFirstIntersection : 1;   // 0xc
    unsigned char m_bReturnBackFaceIntersections : 1;   // 0xc
    unsigned char m_padding : 6;   // 0xc
};   // 0x10 bytes

/* a node's visual: only what CNode's code reads -- a flag word (AddVisual: bit 0 without bit 8
 * makes the node transparent), a type at 4 (1 a mesh, 2 a sub-mesh), a reference count at
 * 0x18, and the vtable pointer at 0x20 (GetSubMesh's dynamic_cast). Not in SOCOM 1's DWARF: the
 * names are ours */
struct tag_VISUAL_HEAD {
    union {
        unsigned int m_flags;   // 0x0: tested by mask, the whole word loaded (AddVisual)
        struct {
            /* CVisual's constructor clears the word, then sets bits 1, 19, 3-7, 14-16 and clears 9,
             * 17 and 18, in that order (the wep lane; the names ours) */
            unsigned int _b0 : 1;
            unsigned int m_b1 : 1;
            unsigned int _b2 : 1;
            unsigned int m_b3 : 1;
            unsigned int m_b4 : 1;
            unsigned int m_b5 : 1;
            unsigned int m_b6 : 1;
            unsigned int m_b7 : 1;
            unsigned int _b8 : 1;
            unsigned int m_b9 : 1;
            unsigned int m_b10 : 1;   /* bit 10: CVisual::Read gives the word at 0x1c -2 for it, 3 without */
            unsigned int _b11 : 3;
            unsigned int m_multipass : 1;   /* bit 14: SetApplyMultipassEffects sets it by byte */
            unsigned int m_b15 : 1;
            unsigned int m_decals : 1;   /* bit 16: SetApplyDecals' */
            unsigned int m_b17 : 1;
            unsigned int m_b18 : 1;
            unsigned int m_b19 : 1;
            unsigned int _b20 : 12;
        } m_bits;
    };
    signed char m_type;   // 0x4
    /* the constructor zeroes the three bytes and two halfwords, and the centroid from CPnt3D::zero
     * (UpdateCentroid zeroes it); the vparams fetch reads these 0x18 bytes (the names ours) */
    unsigned char m_5;   // 0x5
    unsigned char m_6;   // 0x6
    unsigned char m_7;   // 0x7
    unsigned short m_8;   // 0x8
    unsigned short m_a;   // 0xa
    CPnt3D m_centroid;   // 0xc
};   // 0x18 bytes

class CTexHandle;
class BLENDMODE;
class CTexture;
class CTexPalette;

/* a library's textures (SOCOM 1's DWARF: CTexList : vector). Its code is vis code, out of line
 * (SOCOM 1's names, in SOCOM 1's order at 0x354510-0x354790). Declared ahead of CVisual, which
 * holds a CDynTexList */
class CTexList : public std::vector<CTexHandle *> {
public:
    /* every handle let go of, the list emptied */
    void Free();
    /* a texture's handle made and listed */
    CTexHandle *Append(CTexture *tex, bool b);
    /* the handle of a texture of that name, or none */
    CTexHandle *GetHandle(const char *name);
    /* the handle of that texture, or none (SOCOM 1's, right after the name overload) */
    CTexHandle *GetHandle(CTexture *tex);
};   // 0xc bytes

/* a library's palettes (SOCOM 1's DWARF: CTexPalList : vector) */
class CTexPalList : public std::vector<CTexPalette *> {
};   // 0xc bytes

/* a texture buffer's dynamic textures (SOCOM 1's DWARF: CDynTexList, two vectors) */
class CDynTexList {
public:
    void Add(CTexHandle *tex, bool b);
    /* a palette listed (SOCOM 1's overload, 0x357860 here) */
    void Add(CTexPalette *pal, bool b);

    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes

class CMesh;

}

/* the pod storage's sized construction: MSL's __vector_pod::init(n, value), out of line in
 * vis_main.cpp's template tail (0x3C45B0, SOCOM 1's init__Q23std38__vector_pod<Ui,...>FUiRCUi in
 * shape) -- include/vector does not model it yet, so it is called by its placeholder */
extern "C" void func_003C45B0(void *v, unsigned int n, const unsigned int &value);

namespace std {

/* the visual's per-view vector, the one pointer vector built with a size (MSL's vector(n, value),
 * the value a constant temporary; SOCOM 1's CVisual() calls init(0, ...) there too): the wep lane's
 * stand-in until include/vector has the constructor -- the same bases, so the same three-level
 * destructor and accessors as every pointer vector */
template <class Allocator>
class vector<zdb::CMesh *, Allocator> : public __vector_imp<zdb::CMesh *, Allocator, true> {
public:
    vector() {}
    explicit vector(unsigned int n, const unsigned int &value = 0) { func_003C45B0(this, n, value); }
    void clear() { __vector_imp<zdb::CMesh *, Allocator, true>::clear(); }
    void swap(vector &x) { __vector_imp<zdb::CMesh *, Allocator, true>::swap(x); }
};

}

void _free(void *p, const char *file, int line);

/* the owner of a visual's VU buffer, shared by the buffers of each view (SOCOM 1's CVisBase, no
 * DWARF; its constructor and destructor inline in SOCOM II -- CopyBuffer and 0x3B2CB0 build one,
 * ~CVisual deletes one). The block, its holders, its size and a flag that the block is not its own
 * to free (the names ours but the count's) */
class CVisBase {
public:
    ~CVisBase()
    {
        if (!m_static) {
            if (m_data)
                _free(m_data, 0, 0);
            m_data = 0;
        }
        m_instance_count--;
    }
    /* one holder fewer, the count left given back (re-read: as CVisual::Release) */
    int Release()
    {
        m_refs--;
        return m_refs;
    }

    void *m_data;   // 0x0
    int m_refs;   // 0x4
    unsigned int m_size;   // 0x8
    unsigned int m_c;   // 0xc
    unsigned char m_static;   // 0x10

    /* the owners alive (SOCOM 1's static) */
    static int m_instance_count;
};

namespace zdb {

class CVisual : public tag_VISUAL_HEAD {
public:
    /* the holders and a word set by bit 10, CVisual's own ahead of its table: its constructor
     * zeroes the count first of its members, after the vtable pointer (SOCOM 1's too) */
    int m_refs;   // 0x18
    int m_1c;   // 0x1c: -2 or 3 by bit 10 (CVisual::Read; the name ours)

    /* SOCOM II's table (__vt__Q23zdb7CVisual, 0x408380; the wep lane's): CMesh overrides the
     * bounding box, the centroid and Read, and adds InitMatrices after Get2DVisual */
    virtual ~CVisual();                                 /* +0x08 */
    virtual void Render(CMatrix *mtw);                  /* +0x0C */
    virtual void ComputeBBox(CBBox *box);               /* +0x10 */
    virtual void UpdateCentroid();                      /* +0x14 */
    /* the head's parameters and the detail layers read from the archive (SOCOM 1's) */
    virtual bool Read(zar::CZAR &zar);                  /* +0x18 */
    /* none: a visual that draws in 2D overrides it (inline; zanim_menu.cpp's copy, 0x274860, is
     * the linked one) */
    virtual C2DVisual *Get2DVisual() { return 0; }      /* +0x1C */
    /* constructed empty, its type -1 (0x3C4390); one made and read by the type the archive gives
     * (SOCOM 1's static) */
    CVisual();
    static CVisual *Create(zar::CZAR &zar);
    /* the type as a byte: ScrollTexture and the decal walk test it with lbu where GetMesh and
     * AddVisual's test of the member loads it signed (SOCOM 1's split the same way; the name is
     * ours) */
    unsigned char GetType() const { return m_type; }
    /* one holder fewer, the count left given back (SOCOM 1's, by name) */
    int Release();
    /* the texture light maps project by default (SOCOM 1's static) */
    static CTexHandle *m_lightMapTexture;
    /* the camera the visuals are drawn from (SOCOM 1's static; CWorld::Initalize sets it) */
    static CCamera *m_camera;
    /* the decals of one view let go of (SOCOM 1's, by name) */
    void DereferenceDecals(unsigned int vid);
    /* a decal projected onto the visual (SOCOM 1's, by name) */
    void ApplyDecalRender(CMatrix *mtw, CNode *node, float size, CPnt3D *pos, CPnt3D *normal,
                          CTexHandle *tex, CMatrix *uv);
    /* a texture select's name resolved against the world's textures, the default one failing it:
     * the handle and its palette listed, the select copied in (SOCOM 1's; the wep lane's) */
    CTexture *ResolveTextureName(_word128 *names, _word128 *select);
    /* a block of n texture selects opened at the cursor: its zeroed head, the end tag, and the
     * DMA reference and tag that send it (SOCOM 1's static, byte for byte) */
    /* the specular maps let go of (SOCOM 1's static; the wep lane's) */
    static void Flush();
    static std::vector<CLightMap *> m_specMapList;
    /* the view ids' stack primed, the map lists sized, the meshes' statics set (SOCOM 1's static;
     * the wep lane's) */
    static void Init();
    /* the free view ids (SOCOM 1's stack<unsigned int>; SOCOM II's calls the pointer deque's
     * push_back and pop_back, so it is spelled as that deque here), and the light, shadow and
     * projected maps' lists (SOCOM 1's statics: vis's initialiser 0x404570 builds them) */
    static std::deque<const void *> m_stack_vid;
    static std::vector<CLightMap *> m_lightMapList;
    static std::vector<CLightMap *> m_shadowMapList;
    static std::vector<void *> m_projectedMapList;
    static _word128 *GetTextureSelectBlock_ver2(unsigned int n, _word128 **cursor, _word128 *ref,
                                           _word128 *tag);

    /* the wep lane's, from the constructor (0x3C4390): two vectors before the blend words, and the
     * dynamic textures and two words after them -- CSubMesh's own members start at 0x68 */
    std::vector<void *> m_24;   // 0x24 (not read yet)
    /* per view, the mesh of the level of detail in use (CMesh::SetLOD; the name ours) -- a plain
     * visual's VU buffer per view (SOCOM 1's SetBuffer stores it there; ~CVisual frees them) */
    std::vector<CMesh *> m_lodMesh;   // 0x30
    /* the blend words its packets use, each written by CNode::SetBlendMode (the name ours) */
    std::vector<BLENDMODE *> m_blendmodes;   // 0x3c
    /* its textures and palettes (ResolveTextureName adds to it) */
    CDynTexList m_dynTexList;   // 0x48
    /* its detail layers: a count and one allocated block, each layer 0x10 bytes with its
     * vertices after them (CVisual::Read; the names ours) */
    struct DETAIL {
        unsigned int m_0;   // 0x0
        unsigned short m_count;   // 0x4: its entries, 0xc bytes each
        unsigned short m_6;   // 0x6
        unsigned int m_8;   // 0x8
        void *m_data;   // 0xc: set at load, into the block
    };
    DETAIL *m_details;   // 0x60
    unsigned int m_numDetails;   // 0x64
};   // 0x68 bytes

class CSubMesh : public CVisual {
public:
    CSubMesh(CMesh *mesh);
    int m_index;   // 0x68: its place in its mesh's matrices (the names ours)
    CVisual *m_mesh;   // 0x6c
};

class CMesh : public CVisual {
public:
    /* a copy of the mesh, its own data, for a node (SOCOM 1's took no node) */
    CMesh *Clone(CNode *owner);
    /* the wep lane's (vis_mesh.cpp, SOCOM 1's names): its table (__vt__Q23zdb5CMesh, 0x408350)
     * overrides these and adds InitMatrices (+0x20) */
    CMesh();   // 0x3B9760
    virtual ~CMesh();
    /* the meshes' statics set (SOCOM 1's static, 0x3B9820; CVisual::Init calls it) */
    static void Init();
    virtual void ComputeBBox(CBBox *box);
    virtual void UpdateCentroid();
    virtual bool Read(zar::CZAR &zar);
    virtual void InitMatrices();
    void SetMeshTextureSelects(_word128 *head, _word128 *names);
    void SetLOD(unsigned int view, unsigned int lod);

    /* a level of detail (SOCOM 1's CMesh::CMeshLOD, 0xc bytes: the mesh last) */
    class CMeshLOD {
    public:
        unsigned int m_0;   // 0x0
        float m_4;   // 0x4
        CMesh *m_mesh;   // 0x8
    };

    unsigned char _68[0x78 - 0x68];
    std::vector<CMeshLOD> m_lods;   // 0x78
    unsigned int m_numMatrices;   // 0x84
    /* its sub-meshes' matrices, by index: InitMatrices pushes one new CMatrix per sub-mesh (the
     * name ours; znode.h had only the vector's data, as m_palette at 0x90) */
    std::vector<CMatrix *> m_palette;   // 0x88
};

class CVisualVector : public std::vector<CVisual *> {
public:
    bool Exists(const CVisual *visual);
    /* a visual pushed, taken by value: AddVisual's target keeps its parameter in a register and
     * pushes this copy of it, where push_back on the parameter would spill it (the name ours) */
    void Add(CVisual *visual) { push_back(visual); }
};   // 0xc bytes

class CModelVector : public std::vector<CModel *> {
public:
    CModel *GetModel(const char *name);
};   // 0xc bytes

class CLightMapVector : public std::vector<CLightMap *> {
public:
    bool Exists(const CLightMap *light);
};   // 0xc bytes

class CSaveLoad;

class CNode : public tag_NODE_PARAMS {
public:
    /* SOCOM II's vtable order (__vt__Q23zdb5CNode, 0x4067B0); the pointer is at 0x60 */
    virtual ~CNode();                                   /* +0x08 */
    virtual CNode *_Copy(CNode *to) const;   /* +0x0C: into `to`, or a new node when 0 */
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);                /* +0x18 */
    virtual int DeleteChild(CNode *child);
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);
    virtual CNode *FindChild(CNode *node, bool deep);   /* +0x28 */
    /* +0x2C, new in SOCOM II: by name from an index on, the index after the hit given back (the
     * name is ours: an overload, between the two SOCOM 1 had) */
    virtual CNode *FindChild(const char *name, int start, int *index, bool deep);
    virtual bool IsCell();
    /* every override returns true (li v0,1: CNode's, CLight's, CModel's, CWorld's) */
    virtual bool Read(CSaveLoad &io);
    /* the old setting given back (SOCOM 1's DWARF: inline in znode.h, returning the bit) */
    virtual bool SetActive(bool on)
    {
        bool was = m_active;

        m_active = on;
        return was;
    }
    virtual CBBox *UpdateBBox(bool up);
    virtual void UpdateBBoxFromDI(bool up);

    CNode();
    void SetName(const char *name);
    CGridAtom *GetAtom(int i);
    void InsertAtom(CGridAtom *atom);
    void FreeAtom();
    int Release();
    CVisual *GetFirstVisual();
    CSubMesh *GetSubMesh();
    CMesh *GetMesh();
    CNode *Copy() const;
    void SetModelname(const char *name);
    int InheritRegionMasks(unsigned int regions, unsigned int lights);
    bool SetDescendantVisuals(bool set);
    bool SetDescendantDI(bool set);
    void SetModeTree(int mode);
    void SetBlendMode(const BLENDMODE &mode, bool b, bool visuals, bool recurse);
    int ScrollTexture(CNode *node, float u, float v, bool b, int index);
    void RemoveDecals(unsigned int vid);
    void BuildMatrixPalette(CMesh *mesh);
    void ApplyDecal(CNode *node, float size, CPnt3D *pos, CPnt3D *dir, CPnt3D *normal,
                    CTexHandle *tex, float depth, CPnt3D *up);
    /* inline: SetModeTree's innermost copy calls the out-of-line copies (0x3134A0, and 0x1EA880,
     * an earlier unit's) -- the names are ours */
    void SetMode(int mode) { m_socom2_mode = mode; }
    /* inline: DeleteLandmark's test loads the byte apart from the store's (the name ours) */
    bool IsLandmark() const { return m_landmark; }
    CNodeVector &GetChildren() { return m_child; }
    /* inline: find_node's innermost copy calls the out-of-line copy (0x319F10; the name ours) */
    char *GetName() { return m_name; }
    void ShrinkAtomBasePtrs();
    void AllocateAtomBasePtrs(int x, int z);
    void SetNodeEx(CNodeEx *ex, bool b);
    bool AddVisual(CVisual *visual);
    bool AddDI(CDI *di);
    void DeleteDI();
    void DeleteVisuals();
    static void DeleteDeferredVisuals();
    void ReserveDI(int n);
    void ReserveVisuals(int n);
    static int VisualEffectsTick(float dt, void *data);
    void SetApplyClutterFlag(bool on);
    void SetApplyMultipassEffects(bool on);
    void SetApplyDecals(bool on);
    void SetUseParentBBoxTree(bool on);
    void SetParentHasVisuals();
    float GetRadius();
    float GetRadiusSq();
    static CNode *CreateXRef(CModel *model, const CPnt3D *pos, const CQuat *rot);
    /* an empty one (SOCOM 1's, 0x317A90: CFlyout's constructor calls it) */
    static CNode *CreateXRef();
    static CNode *CreateInstance(CModel *model, const CPnt3D *pos, const CQuat *rot);
    static CNode *CreateInstance(const char *name, const CPnt3D *pos, const CQuat *rot);
    static CNode *Delete(CNode *node);
    static CNode *Create(const char *name);
    void CopyParams(CNode *to) const;
    void SetModel(CModel *model);
    void RemoveFromParent();
    bool Rendered(int frames);
    void SetDynamic(bool on, bool recurse);
    void SetDynamicMotion(bool on, bool recurse);
    void SetDynamicLight(bool on, bool recurse);
    void ComputeFacadeMatrix(const CMatrix *view, CMatrix *out);
    void TransformToWorld(CPnt3D &p) const;
    CMatrix *BuildMTW(CMatrix &m);
    /* the node's own matrix (BuildMTW's inner copies call it out of line; the name is ours) */
    CMatrix *GetMatrix() { return &m_matrix; }
    void SetMatrix(const CMatrix *m);
    CPnt3D *GetRotation(CPnt3D *out) const;
    void GetRotation(CQuat *q) const;       /* SOCOM II's */
    CPnt3D *GetScale(CPnt3D *out);
    float GetScale();
    void SetScale(CPnt3D *s);
    void SetScale(float s);
    void SetRotation(const CPnt3D *euler);
    void SetRotation(const CQuat &q);
    /* the pointer form, null tested (CreateXRef tests twice, as for SetPosition's) */
    void SetRotation(const CQuat *q)
    {
        if (q)
            SetRotation(*q);
    }
    void SetPosition(float x, float y, float z);
    /* by reference: the ZAnim light tick's point is read off the frame (sp offsets) through it,
     * where reading its fields directly reuses an address MW hoisted for the point's copies */
    void SetPosition(const CPnt3D &p) { SetPosition(p.x, p.y, p.z); }
    void SetPosition(const PNT3D *p)      /* the base pointer keeps MW from folding the null test */
    {
        if (p)
            SetPosition(p->x, p->y, p->z);
    }
    /* by value, copied out of the matrix's translation row: zAnimObjectTranslateStateTick keeps
     * the copy in memory, stored through one pointer (v0) -- which neither the three-float
     * constructor (scalarised), a brace-initialised PNT3D (zeroed first) nor a named return
     * value (four instructions more) gives */
    CPnt3D GetPosition() const
    {
        return *(const CPnt3D *)m_matrix.m_matrix[3];
    }
    /* the type as a byte: AddChild and UpdateGrid test it with lbu where a test of the member
     * itself loads the halfword (SOCOM 1's took a halfword of its int; the name is ours) */
    unsigned char GetType() const { return m_type; }
    /* by value: CZAnimSetSave::Add pushes the parent through a temporary (allocated after its
     * named locals), where the member itself would be passed by address */
    CNode *GetParent() const { return m_parent; }
    /* the node's bounds, or its parent's when it borrows them (SOCOM 1 links a local copy,
     * GetBBox__Q23zdb5CNodeCFv; SOCOM II's is zanim_cmd_tick.cpp's, 0x262550) */
    const CBBox *GetBBox() const
    {
        if (!m_use_parent_bbox)
            return &m_bbox;
        return m_parent->GetBBox();
    }

    CNode* m_parent;   // 0x64
    CNodeVector m_child;   // 0x68
    CDIVector m_di;   // 0x74
    CVisualVector m_visual;   // 0x84
    char* m_name;   // 0x90
    CNodeEx* m_nodeEx;   // 0x94
    /* the frame the node was last drawn in: SOCOM II's word, where SOCOM 1 kept a byte at 0x99
     * (CNode::Rendered compares it with zVid.frameNumber; the constructor clears the word).
     * SOCOM 1's flags at 0x9a moved to 0xa1 (m_modified is the bit SetPosition sets) */
    unsigned int m_frameRendered;   // 0x98
    float m_Opacity;   // 0x9c
    /* SOCOM II's at 0xa0, where SOCOM 1's DWARF has m_Atom: CZAnim::State and CreateInstanceNode
     * test the byte at 0xa1 with a mask (4) and CreateInstanceNode sets that bit as a bitfield on
     * an instance -- both views, names unknown */
    unsigned char m_socom2_a0;   // 0xa0
    union {
        unsigned char m_socom2_a1;   // 0xa1
        struct {
            unsigned char bit0 : 1;
            unsigned char bit1 : 1;   /* SetPosition ORs it into the byte */
            unsigned char bit2 : 1;
            unsigned char bit3 : 1;
            unsigned char bit4 : 1;
            unsigned char bit5 : 1;
            unsigned char bit6 : 1;
            /* LightList_Search sets it on a node whose first visual's material is of type 2 (the
             * name ours) */
            unsigned char bit7 : 1;
        } m_socom2_a1_bits;
    };
    unsigned char _socom2_a2[2];   // 0xa2
    int m_TickNum;   // 0xa4
    short m_AtomCnt;   // 0xa8
    short m_AtomAlloc;   // 0xaa
    short m_count;   // 0xac
    short m_vid;   // 0xae
    CGridAtom **m_Atom;   // 0xb0 (GetAtom; SOCOM 1: 0xa0)
    CModel* m_model;   // 0xb4 (SetModel; SOCOM 1: 0xb0)
    char* m_modelname;   // 0xb8 (SOCOM 1: 0xb4)
    unsigned int m_region_mask;   // 0xbc (SOCOM 1: 0xb8)

    /* visuals let go of while still drawn this frame, deleted later (SOCOM II's, node_main.cpp's
     * second static at 0x45C2D8; the name is ours) */
    static std::list<CVisual *> m_deferred_visuals;
};   // 0xc0 bytes

/* a light: a node with a range (SOCOM 1's DWARF) */
class CLight;

/* a light in the tick list (SOCOM 1's DWARF) */
class CTickLight {
public:
    CLight *m_pLight;   // 0x0
    unsigned char m_state;   // 0x4
    float m_scale;   // 0x8
};   // 0xc bytes

/* OPEN (its destructor, 0x317820): the target's inlines four this-tested levels, the size
 * cleared and the block freed -- a vector of a class is six high here (only one level
 * inlines), a vector of pointers clears nothing; SOCOM II's MSL may give a POD class its own
 * path. SOCOM 1's DWARF: a vector of CTickLight */
class CTickLightList : public std::vector<CTickLight> {
};   // 0xc bytes

class CLight : public CNode {
public:
    /* the lights being ticked (SOCOM 1's; node_main.cpp's first static, 0x45C2B8) */
    static CTickLightList m_tick_list;

    CLight();
    virtual ~CLight();
    virtual CNode *_Copy(CNode *to) const;
    virtual bool Read(CSaveLoad &io);
    virtual bool SetActive(bool on);
    virtual CBBox *UpdateBBox(bool up);
    /* inline (CLightMap::SetTexture's range is loaded ahead of its constant: the name ours) */
    float GetMaxRange() const { return m_maxRange; }
    /* the ranges and their derived terms, the bounds updated: inline, five copies in the ZAnim
     * light tick (the name is ours) */
    void SetRange(float min, float max)
    {
        m_minRange = min;
        m_maxRange = max;
        m_maxRangeSq = max * max;
        m_invMaxRange = 1.0f / m_maxRange;
        m_invDeltaRange = 1.0f / (m_maxRange - m_minRange);
        UpdateBBox(false);
    }

    float m_maxRangeSq;   // 0xc0
    float m_maxRange;   // 0xc4
    float m_minRange;   // 0xc8
    float m_invMaxRange;   // 0xcc
    float m_invDeltaRange;   // 0xd0
    float m_opacity;   // 0xd4
    CPnt3D m_diffuse;   // 0xd8
    unsigned int m_appliedToNode;   // 0xe4
    CPnt3D m_WorldPosition;   // 0xe8
};   // 0x100 bytes (16-aligned)

/* a texture: only its size placed (SOCOM 1's DWARF: CTexture : TEXTURE_PARAMS, the width first;
 * 0x70 bytes, the size CSaveLoad::LoadTextures_PS2 news) */
class CAssetLib;

class CTexture {
public:
    ~CTexture();
    bool Read(CSaveLoad &io);

    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
    unsigned char _4[0x10 - 4];   // the rest of TEXTURE_PARAMS
    CAssetLib *m_AssetLib;   // 0x10 (AddTexture sets it)
    char *m_name;   // 0x14
    void *m_buffer;   // 0x18 (SOCOM 1's DWARF, from here; the wep lane's)
    CTexPalette *m_palette;   // 0x1c
    unsigned char _20[0x40 - 0x20];
    /* the VU's DMA reference to its select (SOCOM 1's _word128; ResolveTextureName copies it) */
    u_long128 m_dmaRefVu;   // 0x40
    unsigned char _50[0x54 - 0x50];
    /* the first mip level's register (SOCOM 1's sceGsMiptbp1 *): a texture with mip levels has one
     * (the asset manager's mip walk tests it) */
    void *m_MIP1;   // 0x54
};

/* a texture's handle (SOCOM 1's DWARF) */
class CTexHandle {
public:
    static CTexHandle *Create(CTexture *tex);

    char *m_name;   // 0x0
    int m_count;   // 0x4
    unsigned int m_gsAddr;   // 0x8
    CTexture *m_texture;   // 0xc
    unsigned char m_libref;   // 0x10
};   // 0x14 bytes

/* the GS ALPHA register's fields (the SDK's sceGsAlpha): (A - B) * C + D, C or D the FIX alpha */
struct sceGsAlpha {
    unsigned long A : 2;
    unsigned long B : 2;
    unsigned long C : 2;
    unsigned long D : 2;
    unsigned long pad8 : 24;
    unsigned long FIX : 8;
    unsigned long pad40 : 24;
};

/* a GS alpha blend, its register word (SOCOM 1's DWARF: sceGsAlpha and the word, one union) */
class BLENDMODE {
public:
    union {
        sceGsAlpha m_alpha;   // 0x0
        unsigned long m_alphaWORD;   // 0x0
    };
};   // 8 bytes

/* a light that projects a texture (SOCOM 1's DWARF) */
class CLightMap : public CLight {
public:
    CLightMap();
    virtual ~CLightMap();
    virtual CNode *_Copy(CNode *to) const;
    void SetTexture(CTexture *tex);
    /* placed: the projection's translation the negated point, then the node's (inline; the
     * ZAnim light tick loads the three values once for both, through the reference form, as
     * SetPosition's; the names are ours) */
    void SetPos(float x, float y, float z)
    {
        m_worldToUV.m_matrix[3][0] = -x;
        m_worldToUV.m_matrix[3][1] = -y;
        m_worldToUV.m_matrix[3][2] = -z;
        SetPosition(x, y, z);
    }
    void SetPos(const CPnt3D &p) { SetPos(p.x, p.y, p.z); }

    CTexture *m_pTex;   // 0x100
    CMatrix m_worldToUV;   // 0x110
    CPnt4D m_dir;   // 0x150
    unsigned int m_isShadow : 1;   // 0x160
    unsigned int m_isReflection : 1;   // 0x160
    BLENDMODE m_blendmode;   // 0x168
    float m_SpecularFade;   // 0x170
    CNode *m_instance;   // 0x174
};   // 0x180 bytes

/* a model node: what CZAnim::CreateInstanceNode instances (dynamic_cast from CNode) */
/* a cell of the world's grid (defined after the light list it holds) */
class CCell;

/* the render phase an asset library belongs to (SOCOM 1's DWARF; a byte under -enum min). SOCOM II
 * has a phase after the custom set: the custom characters' (AddCustomCharacters finds their
 * library by it, AddTextureAssetCharacter gives its sets the decal textures as well); the name is
 * ours */
enum _RenderPhase {
    WORLD, HUD_OBSOLETE, CHARSETCOMMON, CHARSET1, CHARSET2, CHARSET3, CHARSET4, CHARSET5,
    CHARSET6, CHARSET7, CHARSET8, ALPHA, HUDSET1, HUDSET2, HUDSETSMALL, SHADOW, CHARSETCUSTOM,
    CHARSETCUSTOMCHARS, NUM_RENDER_PHASES
};

/* (CTexList and CTexPalList are declared ahead of CVisual) */

/* a library of models and textures (SOCOM 1's DWARF has its render phase at 0x2B, after the name
 * and three lists; CNode::CreateXRef reads it at 0x2F here: SOCOM II adds a word after the name --
 * AddModel pushes onto the vector at 8, AddPalette walks the one at 0x20, LoadTextures_PS2 stores
 * the texture buffer at 0x30) */
class CAssetLib {
public:
    char *m_name;   // 0x0
    union {
        unsigned char _4[4];   // 0x4: SOCOM II's
        char *m_socom2_4;   // a second string: the destructor frees it after the name (the name ours)
    };
    CModelVector m_models;   // 0x8 (SOCOM 1: 0x4)
    CTexList m_textures;   // 0x14
    CTexPalList m_palettes;   // 0x20
    unsigned char m_autoload;   // 0x2c
    unsigned char m_locked;   // 0x2d (AddModel refuses a locked library)
    unsigned char m_gearlib;   // 0x2e
    _RenderPhase m_renderphase;   // 0x2f (SOCOM 1: 0x2b)
    void *m_texture_buffer;   // 0x30
    void *m_model_buffer;   // 0x34
    /* SOCOM II's third buffer, freed with the other two (the name ours); the count follows it --
     * the constructor sets the word at 0x3c to 1 and ReleaseLibRef counts it down, and SOCOM II
     * news 0x40 bytes for a library */
    void *m_socom2_38;   // 0x38
    int m_iRefCount;   // 0x3c (SOCOM 1: 0x34)

    CAssetLib(const char *name);
    ~CAssetLib();
    CTexHandle *AddTexture(const char *name);
    CTexHandle *AddTexture(CTexture *tex);
    CModel *AddModel(CModel *model);
    CTexPalette *AddPalette(CTexPalette *pal, bool check);
    bool IsNamed(const char *name);
    char *RootName();
    /* the render phase by value (inline: CAssetList::FindLib(_RenderPhase) narrows its argument
     * into a register of its own only so; the name is ours) */
    _RenderPhase GetRenderPhase() const { return m_renderphase; }
};   // 0x40 bytes

/* a model's instances (SOCOM 1's DWARF: CRefList : list) */
class CRefList : public std::list<CNode *> {
public:
};   // 0xc bytes

/* a model (SOCOM 1's DWARF) */
class CModel : public CNode {
public:
    int m_variant;   // 0xc0
    unsigned char m_bForceExport;   // 0xc4
    CRefList m_list;   // 0xc8
    unsigned char m_bbox_valid;   // 0xd4
    CAssetLib *m_AssetLib;   // 0xd8

    /* an instance listed, taken by value, and the model's parent the first one: CreateInstance's
     * target keeps its node in a register and re-reads this copy for the parent after the push
     * (SOCOM 1's too; the name is ours) */
    void AddInstance(CNode *node)
    {
        m_list.push_back(node);
        m_parent = m_list.size() == 1 ? node : 0;
    }
    /* an instance taken off, by value (the remove's binder holds this copy's address, and the
     * parent test re-reads it: CNode::Delete; the name is ours) */
    /* an instance let go of (SOCOM 1's, by name; ~CNode calls it) */
    void Release(CNode *node);
    void RemoveInstance(CNode *node)
    {
        m_list.remove(node);
        if (m_parent == node)
            m_parent = 0;
    }
};

}

/* a rectangle in x/z (SOCOM 1's DWARF; a global class, as SOCOM 1's __ct__6CFRectFv says). Its
 * constructor zeroes it, out of line (0x306650, the same 0x18 bytes as SOCOM 1's) */
class CFRect {
public:
    CFRect();

    float top;   // 0x0
    float bottom;   // 0x4
    float left;   // 0x8
    float right;   // 0xc
};   // 0x10 bytes

namespace zdb {

/* a count in two dimensions (SOCOM 1's DWARF) */
class tag_SIZE {
public:
    int cx;   // 0x0
    int cy;   // 0x4
};   // 8 bytes

/* the grid's parameters, as the world's file stores them (SOCOM 1's DWARF) */
class tag_GRID_PARAMS {
public:
    int m_AtomCnt;   // 0x0
    int m_posts;   // 0x4
    float m_CellDim;   // 0x8
    tag_SIZE m_CellCount;   // 0xc
};   // 0x14 bytes

/* a cell in the ring-ordered walk (SOCOM 1's DWARF) */
class GRIDCELLATOM {
public:
    CGridAtom *cellAtom;   // 0x0
    int ring;   // 0x4
    int x;   // 0x8
    int z;   // 0xc
};   // 0x10 bytes

/* the cells nearest a point first, ring by ring (SOCOM 1's DWARF); the counts and the view cell
 * cleared by its constructor (CGrid's, both builds) */
class COrderedCell {
public:
    COrderedCell()
    {
        m_gridCellAtomCnt = 0;
        m_rt_gridCellAtomCnt = 0;
        m_viewX = 0;
        m_viewZ = 0;
    }

    int m_gridCellAtomCnt;   // 0x0
    int m_rt_gridCellAtomCnt;   // 0x4
    int m_viewX;   // 0x8
    int m_viewZ;   // 0xc
    GRIDCELLATOM m_Atoms[1600];   // 0x10
};   // 0x6410 bytes

class CWorld;
class CClutter;
class CClutterAnimManager;
class CSaveLoad;

/* the world's grid of cells (SOCOM 1's DWARF: CGrid : tag_GRID_PARAMS, 0x651C bytes). SOCOM II's
 * is 8 bytes shorter: its spare atoms are a free list threaded through their Next pointers (Create
 * links m_AtomBuf's atoms, gridRemoveNodeFromGrids pushes an atom back on m_FreeAtoms, Destroy
 * frees only m_Atoms and m_AtomBuf), so SOCOM 1's free-pointer array's count and index
 * (m_AtomFreeCnt and m_AtomFreePtr, 0x3c and 0x40) are gone, and every member from m_InvCellDim on
 * sits 8 bytes earlier: StartTraversal's m_RowCellIn at 0x4d (SOCOM 1: 0x55), Create's
 * m_InvCellDim, m_GridCellTol, m_ClutterTol and m_TickNum at 0x3c-0x48 (0x44-0x50), m_origin at
 * 0xf4 (0xfc), the ordered cells at 0x104 (0x10c) -- 0x6514 bytes, CWorld's landmarks right after
 * it at 0x6b98 */
class CGrid : public tag_GRID_PARAMS {
public:
    CGrid();
    ~CGrid();
    /* the grid read from the world's file and built (SOCOM 1's) */
    bool Read(CSaveLoad &io);
    bool ReadClutter(const char *path, const char *name);
    /* the cells' clutter let go of (SOCOM 1's) */
    void DeleteClutter();
    bool AddClutter(CClutter *clutter);
    /* the cells taken out of the world and the atoms freed (SOCOM 1's) */
    void Destroy();
    /* the cells made, one per grid square, each a child of the world (SOCOM 1's) */
    bool Create(CWorld *world, const tag_GRID_PARAMS *params);
    /* a node's place in the grid brought up to date (SOCOM 1's) */
    void Update(CNode *node);
    /* a walk over the cells a polygon in x/z covers: the boundary set, the first atom and the
     * next (SOCOM 1's) */
    /* the new walk's tick given back: both paths end on ++m_TickNum, its value left in v0 (a void
     * function allocates the loop's temporaries from v1) */
    int SetTraversalBoundary(const CPnt3D *pnts, int n, bool b);
    CGridAtom *StartTraversal();
    CGridAtom *GetNextAtom();
    /* the same walk in ring order from a view point (SOCOM 1's) */
    CGridAtom *StartTraversalOrdered();
    CGridAtom *GetNextAtomOrdered();
    /* a node put in the grid, and taken out (SOCOM 1's; Insert tail-calls gridAddNodeToGrids) */
    void Insert(CNode *node);
    void Remove(CNode *node);
    /* a point's cell, held inside the grid (SOCOM 1's) */
    void gridGetGridXandZIndex(int *ix, int *iz, float x, float z);
    /* the walk stepped to its next cell, false at its end (SOCOM 1's) */
    bool gridGetNextIndex();
    /* whether a cell lies outside the boundary (SOCOM 1's) */
    bool gridCellOut(int x, int z);
    void SetTraversalExtents(const CPnt3D *pnts, int n, bool b);
    void addOrderedCellAtom(int x, int z, bool force);
    void buildOrderedCellAtomList(const CPnt3D *view);
    /* the cells of a box: its first, then each next (SOCOM 1's) */
    CCell *SetCellBoundary(CPnt3D min, CPnt3D max);
    CCell *GetNextCell();
    void gridRemoveNodeFromGrids(CNode *node);
    void gridAddNodeToGrids(CNode *node);
    /* a cell's atom list head, the cell held inside the grid (SOCOM 1's) */
    CGridAtom **gridGetAtomBasePtr(int x, int z);
    /* the world's queries, cell by cell (SOCOM 1's) */
    void InitDescendantsVisuals();
    void InitDescendantsDI();
    /* a ray's hits gathered, and a point's altitude hits (grid_di.cpp's, SOCOM 1's) */
    bool GetIntersect(DiIntersect *di);
    bool GetAltitude(DiIntersect *di);

    /* the atoms a grid makes (SOCOM 1's static; Read takes the parameters' count from it) */
    static int N_ATOMS;

    CFRect m_extents;   // 0x14
    CWorld *m_world;   // 0x24
    CClutterAnimManager *m_ClutterAnimManager;   // 0x28
    unsigned char m_ClutterCreated;   // 0x2c
    CGridAtom **m_Atoms;   // 0x30: each cell's atom list
    CGridAtom *m_AtomBuf;   // 0x34
    CGridAtom *m_FreeAtoms;   // 0x38: SOCOM II's free list (SOCOM 1's an array of pointers)
    float m_InvCellDim;   // 0x3c
    float m_GridCellTol;   // 0x40
    float m_ClutterTol;   // 0x44
    int m_TickNum;   // 0x48
    unsigned char m_LineWalk;   // 0x4c
    unsigned char m_RowCellIn;   // 0x4d
    unsigned char m_CurCellOut;   // 0x4e
    CPnt3D m_Pnts[5];   // 0x50
    CPnt3D m_Edge[5];   // 0x8c
    int m_PntCnt;   // 0xc8
    int m_ix;   // 0xcc
    int m_ixb;   // 0xd0
    int m_xx;   // 0xd4
    int m_iz;   // 0xd8
    int m_izb;   // 0xdc
    int m_zz;   // 0xe0
    int m_iz_start;   // 0xe4
    int m_iz_next_row;   // 0xe8
    int m_iz_dir;   // 0xec
    CGridAtom *m_NextAtom;   // 0xf0
    CPnt3D m_origin;   // 0xf4
    int m_ring;   // 0x100
    COrderedCell m_orderedCells;   // 0x104
};   // 0x6514 bytes

}

class _zanim_cmd_hdr;
struct _zrdr;

/* the world's wind (a global class, as its mangled names say): SOCOM 1's DWARF, but SOCOM II puts
 * the wind vector first (Reset and TickGoal
 * read the goals at 0x10 and the angles at 0x28, 0x10 later than SOCOM 1's) */
class CWind {
public:
    void Reset();
    void TickGoal(float dt);
    void Tick(float dt);
    static bool CmdTickWind(_zanim_cmd_hdr *cmd, float *dt);
    static _zanim_cmd_hdr *CmdParseWind(_zrdr *rdr);
    static void RegisterAnimCommands();
    /* a gust's range, both bounds taken before either is stored (inline: CmdTickWind loads the
     * command's two values, then the world; the names are ours) */
    void SetAzimuthOffsetRange(float min, float max)
    {
        m_MinAzimuthOffset = min;
        m_MaxAzimuthOffset = max;
    }
    void SetZenithOffsetRange(float min, float max)
    {
        m_MinZenithOffset = min;
        m_MaxZenithOffset = max;
    }
    void SetSpeedOffsetRange(float min, float max)
    {
        m_MinSpeedOffset = min;
        m_MaxSpeedOffset = max;
    }

    PNT4D m_Wind;   // 0x0 (SOCOM 1: 0x1c; 4-aligned -- CWind is 0x68 bytes, and Tick copies into it
                    // element by element)
    float m_AzimuthGoal;   // 0x10
    float m_ZenithGoal;   // 0x14
    float m_SpeedGoal;   // 0x18
    float m_AzimuthGoalDelta;   // 0x1c
    float m_ZenithGoalDelta;   // 0x20
    float m_SpeedGoalDelta;   // 0x24
    float m_Azimuth;   // 0x28
    float m_Zenith;   // 0x2c
    float m_Speed;   // 0x30
    float m_AzimuthOffset;   // 0x34
    float m_MinAzimuthOffset;   // 0x38
    float m_MaxAzimuthOffset;   // 0x3c
    float m_ZenithOffset;   // 0x40
    float m_MinZenithOffset;   // 0x44
    float m_MaxZenithOffset;   // 0x48
    float m_AzimuthOffsetVelocity;   // 0x4c
    float m_ZenithOffsetVelocity;   // 0x50
    float m_SpeedOffset;   // 0x54
    float m_MinSpeedOffset;   // 0x58
    float m_MaxSpeedOffset;   // 0x5c
    float m_SpeedOffsetVelocity;   // 0x60
    unsigned char m_TickAzimuth;   // 0x64
    unsigned char m_TickZenith;   // 0x65
    unsigned char m_TickSpeed;   // 0x66
    unsigned char m_TickGoal;   // 0x67
};   // 0x68 bytes

namespace zdb {

/* the world's directional lights and ambient (SOCOM 1's DWARF) */
class _globalLight {
public:
    CPnt4D dir[3];   // 0x0
    CPnt4D col[3];   // 0x30
    CPnt4D ambient;   // 0x60
};   // 0x70 bytes

class DiIntersect;
class _cellData;
class CRenderMap;
class CTextureAssetCharacters;
/* the 0x70-byte objects the world makes for its shadow casters (new in SOCOM II, at 0x580; the
 * name is ours) */
class CShadowCaster;

/* the lights the world ticks (SOCOM 1's DWARF: CLightList : list) */
class CLightList : public std::list<CLight *> {
public:
    /* a light pushed, taken by value: GenerateLightList keeps the light in a register and pushes
     * this copy, a temporary below the frame's others (the name is ours) */
    void Add(CLight *light) { push_back(light); }
};   // 0xc bytes

/* a cell of the world's grid: SOCOM 1's DWARF has its lights, its clutter list (0xcc) and its
 * dynamic textures (0xe0) after the node; only the lights placed yet */
class CCell;

/* a prop scattered on the cells (SOCOM 1's grid_clutter.cpp class; 0x70 bytes, the size ReadClutter
 * news). Its placement is a matrix (the constructor makes it the unit one), or -- with m_quat set --
 * a rotation, a translation in the third row and a scale (GetMatrix builds the matrix from them,
 * GetTranslate picks the row); the names past SOCOM 1's are ours */
class CClutter {
public:
    CClutter(CNode *node);
    CMatrix *GetMatrix();
    CPnt3D *GetTranslate();
    /* its parameters (the first 0x60 bytes) and its inverse scale read from the archive's open key,
     * its node made dynamic first (SOCOM 1's; SOCOM II clears the 4-bit count before the second
     * read) */
    bool Read(zar::CZAR *zar);
    /* the flag word's setters: SOCOM II's constructor clears each field through one -- a
     * constant stored through an inline's parameter is masked at run time, `(0 & 1) << n` (the
     * names are ours) */
    void SetBit0(bool b) { m_bit0 = b; }
    void SetQuat(bool b) { m_quat = b; }
    void SetBit2(bool b) { m_bit2 = b; }
    void SetBit3(bool b) { m_bit3 = b; }
    void SetBits4(int n) { m_bits4 = n; }
    void SetBit12(bool b) { m_bit12 = b; }
    void SetBit13(bool b) { m_bit13 = b; }
    void SetCount(int n) { m_count = n; }
    void SetBits18(int n) { m_bits18 = n; }

    CMatrix m_matrix;   // 0x0
    unsigned char _40[0x4c - 0x40];
    unsigned int m_bit0 : 1;   // 0x4c
    unsigned int m_quat : 1;   // 0x4c
    unsigned int m_bit2 : 1;   // 0x4c
    unsigned int m_bit3 : 1;   // 0x4c
    unsigned int m_bits4 : 2;   // 0x4c
    unsigned int m_bits6 : 6;   // 0x4c
    unsigned int m_bit12 : 1;   // 0x4c (SOCOM II's: SOCOM 1's last twenty bits are split)
    unsigned int m_bit13 : 1;   // 0x4c
    unsigned int m_count : 4;   // 0x4c (bits 14-17: Read clears it)
    unsigned int m_bits18 : 14;   // 0x4c
    unsigned char _50[0x5c - 0x50];
    int m_5c;   // 0x5c (the constructor sets -1)
    float m_scale_inverse;   // 0x60 (the archive's "scale_inverse")
    CNode *m_node;   // 0x64
    /* the animation moving the prop, cleared when it is deleted (ClearAnimations, l_Tick; the
     * name ours) */
    class CClutterAnim *m_anim;   // 0x68
    unsigned char _6c[0x70 - 0x6c];
};   // 0x70 bytes

/* a prop's animation (SOCOM 1's grid_clutter.cpp class): the prop it moves and a node it made.
 * Its destructor is inline in SOCOM II (SOCOM 1's out-of-line copy, 0x218980, has no counterpart):
 * the prop's rotation reset and the node deleted. The names past m_clutter are ours */
class CClutterAnim {
public:
    ~CClutterAnim()
    {
        if (m_clutter)
            ((CQuat *)m_clutter->m_matrix.m_matrix[1])->MakeYXZRot(0.0f, 0.0f, 0.0f);
        if (m_node)
            delete m_node;
        m_node = 0;
    }
    void TickForce(CPnt3D *force, CPnt3D *pos, float a, float b);

    CClutter *m_clutter;   // 0x0
    CNode *m_node;   // 0x4
    unsigned char _8[0x14 - 0x8];
    float m_14;   // 0x14
    float m_18;   // 0x18
    float m_1c;   // 0x1c
    float m_20;   // 0x20
};

/* a body pushing the clutter about (SOCOM 1's list element, 0x28 bytes, the size RegisterEffector
 * news: its owner, its position and velocity borrowed, two vectors zeroed and its radius; the
 * names ours) */
struct CClutterEffector {
    const void *m_owner;   // 0x0
    const CPnt3D *m_pos;   // 0x4
    const CPnt3D *m_vel;   // 0x8
    CPnt3D m_c;   // 0xc
    CPnt3D m_18;   // 0x18
    float m_radius;   // 0x24
};

/* a vector between two points, by value: Sub out of line into a temporary, copied out (ld/sd and
 * lwc1/swc1) -- the collision code's differences (grid_di.cpp, int_main.cpp; SOCOM II's CPnt3D
 * operator-, which zmath.h lacks; the name is ours) */
inline CPnt3D PntMinus(const CPnt3D &a, const CPnt3D &b)
{
    CPnt3D r;

    a.Sub(&b, &r);
    return r;
}

/* the clutter's animation: the props pushed about by effectors (SOCOM 1's grid_clutter.cpp class,
 * 0x70 bytes, its DWARF) */
class CClutterAnimManager {
public:
    CClutterAnimManager();
    ~CClutterAnimManager();
    /* every effector deleted, the list emptied (SOCOM 1's) */
    void ClearEffectors();
    /* every animation deleted, the list emptied (SOCOM 1's) */
    void ClearAnimations(bool detach);
    /* an owner's effector taken off (SOCOM 1's) */
    void UnRegisterEffector(const void *owner);

    unsigned char m_CellAnimate;   // 0x0
    CMatrix m_matrix;   // 0x10
    std::list<CClutterAnim *> m_ClutterAnims;   // 0x50
    std::list<CClutterEffector *> m_ClutterEffectors;   // 0x5c
};   // 0x70 bytes

/* a cell's clutter: the props scattered on it (SOCOM 1's DWARF: CClutterList : vector, 0x14
 * bytes). SOCOM II's is 0x1C bytes, a cell holds two (CCell::AddClutter puts a prop with
 * collision in the second), and their constructor clears the three members past SOCOM 1's (the
 * names ours) */
class CClutterList : public std::vector<CClutter *> {
public:
    CClutterList()
    {
        m_socom2_14 = 0;
        m_socom2_19 = 0;
        m_socom2_18 = 0;
    }
    /* every prop let go of, the list emptied (SOCOM 1's) */
    void Free();
    void Render(CCell *cell);
    bool Add(CClutter *clutter);

    unsigned char m_ContainsRepelClutter;   // 0xc
    float m_max_farFadeSq;   // 0x10
    int m_socom2_14;   // 0x14 (Free clears it)
    unsigned char m_socom2_18;   // 0x18
    unsigned char m_socom2_19;   // 0x19 (CCell's setter at 0x2D7AF0 writes it in both lists)
};   // 0x1c bytes

/* (CDynTexList is declared ahead of CVisual) */

/* a cell of the world's grid: SOCOM 1's DWARF has its lights, its clutter list (0xcc) and its
 * dynamic textures (0xe0) after the node, 0x100 bytes; SOCOM II's has two clutter lists, so its
 * dynamic textures sit at 0x104 -- 0x120 bytes, the size CGrid::Create news. Its own virtual,
 * new after CNode's (__vt__Q23zdb5CCell, slot 0x44), is SOCOM 1's Copy */
class CCell : public CNode {
public:
    CCell(const CPnt3D *pos, float size);
    virtual ~CCell();
    virtual bool IsCell();
    virtual CBBox *UpdateBBox(bool up);
    virtual CNode *Copy(CNode *to) const;
    /* the cell's centre (SOCOM 1's) */
    void GetMidPoint(CPnt3D *out);
    void RenderClutter(CCell *cell);
    /* both lists' flag at 0x19 set (new in SOCOM II; the name ours) */
    void SetClutterFlag(bool on);
    bool AddClutter(CClutter *clutter);
    void DeleteClutter();

    CLightList m_lightList;   // 0xc0 (GenerateLightList lists each light in the cells it reaches)
    CClutterList m_clutterList;   // 0xcc
    CClutterList m_clutterListDI;   // 0xe8: the props with collision (the name ours)
    CDynTexList m_dyntexlist;   // 0x104
};   // 0x120 bytes

/* a node's link into the grid's cells (SOCOM 1's DWARF) */
class CGridAtom {
public:
    /* the atom's node as a cell, or none (SOCOM 1's) */
    CCell *GetCell();

    CNode *Ent;   // 0x0
    CGridAtom *Next;   // 0x4
    CGridAtom *Prev;   // 0x8
};   // 0xc bytes

/* a tree's lights listed (SOCOM 1's free function) */
void LightList_Search(CNode *node, CLightList &lights);

/* the world's shadows (SOCOM 1's DWARF: CRenderMapVector : vector) */
class CRenderMapVector : public std::vector<CRenderMap *> {
};   // 0xc bytes

/* the world's characters with textures of their own (SOCOM 1's DWARF: a list) */
class CTextureAssetCharacterList : public std::list<CTextureAssetCharacters *> {
};   // 0xc bytes

/* the per-band render tables a world reads (SOCOM 1's DWARF: each a vector and its band count,
 * 0x10 bytes; their constructors, destructors and readers are vis code, out of line) */
class CLOD_Object {
public:
    CLOD_Object();
    ~CLOD_Object();
    bool Read(CSaveLoad *io);
    unsigned char _0[0x10];
};
/* a material of the world's table: 0x3C bytes, its type at 0x14 (LightList_Search tests 2; the
 * names ours) */
struct MATERIAL_ENTRY {
    unsigned char _0[0x14];
    int m_type;   // 0x14
    unsigned char _18[0x3c - 0x18];
};
class CMaterial_Object {
public:
    CMaterial_Object();
    ~CMaterial_Object();
    bool Read(CSaveLoad *io);
    /* a material by its 1-based index, none for 0 or past the end (inline: LightList_Search's
     * copy tests the index twice; the name ours) */
    MATERIAL_ENTRY *GetMaterial(unsigned int i)
    {
        if (i == 0)
            return 0;
        return i <= m_size ? &m_data[i - 1] : 0;
    }

    unsigned int m_capacity;   // 0x0 (the table's vector)
    unsigned int m_size;   // 0x4
    MATERIAL_ENTRY *m_data;   // 0x8
    unsigned char _c[4];   // 0xc
};
class CSubSurf_Object {
public:
    CSubSurf_Object();
    ~CSubSurf_Object();
    unsigned char _0[0x10];
};
class CPreLight_Object {
public:
    CPreLight_Object();
    ~CPreLight_Object();
    unsigned char _0[0x10];
};
class CScrollingTexture_Object {
public:
    CScrollingTexture_Object();
    ~CScrollingTexture_Object();
    bool Read(CSaveLoad *io);
    unsigned char _0[0x10];
};

/* the scene's root; SOCOM 1's DWARF has its 0x6BE0 bytes of members, and SOCOM II's is the same
 * size, laid out from its constructor: the wind moved to the front (so the camera is 0x68 later),
 * CProjectedMap_Object dropped, a vector added at 0x580, the lens effects four floats each, a
 * shadow weight, a foliage sound and the world text's specs added. CCamera::GetLandmarkFarPlane
 * reads the grid's extents at 0x698, GetDatabaseNode the landmark list at 0x6B98. */
class CAssetList;

class CWorld : public CNode {
public:
    static CWorld *m_world;
    /* a texture's handle by name, from the asset manager's list (SOCOM 1's four words, out of
     * line at 0x1EC940; the wep lane's) */
    static CTexHandle *GetTexHandle(const char *name);
    static CNode *GetModel(const char *name);
    /* the asset manager's library list (SOCOM 1's, 0xC bytes in both; out of line in SOCOM II's
     * node_assetlib.cpp, which AddModel and other units call) */
    static CAssetList *GetAssetList();
    /* the archive version the world's files carry (SOCOM 1's, the same 0x10001) */
    static int GetVersion();
    virtual ~CWorld();
    virtual bool Read(CSaveLoad &io);
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);
    virtual int DeleteChild(CNode *child);
    virtual void DeleteChildren();
    void DeleteLightMap(CLightMap *light);
    void AddLightMap(CLightMap *light);
    void AddShadow(CNode *node);
    /* a shadow caster made for the node and listed (new in SOCOM II: AddChild and node_main.cpp's
     * code call it; the name is ours) */
    void AddShadowCaster(CNode *node);
    /* the shadow casters brought up to date (new in SOCOM II; the name ours) */
    void UpdateShadowCasters();
    bool Initalize();
    void Update();
    void ReleaseLandmarkList();
    void ReleaseLightList();
    /* CWorld's own virtual functions, after CNode's (SOCOM II's table 0x406940, slots 0x44-0x4C,
     * as SOCOM 1's 0x40-0x48: GenerateLandmarkList calls the first through it) */
    virtual void AddLandmark(CNode *node);
    virtual void DeleteLandmark(CNode *node);
    virtual CNode *FindLandmark(const char *name);
    int GenerateLandmarkList();
    /* the lights gathered and put in the grid's cells (SOCOM 1's) */
    int GenerateLightList();
    /* the custom characters given their textures (new in SOCOM II; the name ours) */
    void AddCustomCharacters();
    /* a node drawn with a character set's textures (SOCOM 1's three) */
    void AddTextureAssetCharacter(CAssetLib *lib, CNode *node);
    void AddTextureAssetCharacter(char *name, CNode *node);
    void AddTextureAssetCharacter(_RenderPhase phase, CNode *node);
    void LandmarkList_Search(CNode *node, CStack &stack, CNodeVector &list);
    IntersectStruct *GetIntersect(const CPnt3D *p0, const CPnt3D *p1);
    /* SOCOM II's overloads take whether characters are hit (the names SOCOM 1's) */
    IntersectStruct *GetIntersect(const CPnt3D *p0, const CPnt3D *p1, bool characters);
    DiIntersect *GetIntersects(const CPnt3D *p0, const CPnt3D *p1);
    DiIntersect *GetIntersects(const CPnt3D *p0, const CPnt3D *p1, bool characters);
    IntersectStruct *GetAltitude(const CPnt3D *p, float alt);
    IntersectStruct *GetAltitude(const CPnt3D *p, float alt, bool characters);
    DiIntersect *GetAltitudes(const CPnt3D *p);
    /* the world's grid's, whichever world it is called on (SOCOM 1's: its this is not read) */
    int GetIntersectBatch(DiIntersect **di, int n);
    void InitDescendantsDI();
    void InitDescendantsVisuals();
    static int diTick(float dt, void *data);
    int RegIntersectDelayed(DiIntersect *di, const CPnt3D *tail, const CPnt3D *tip, unsigned int mask);
    int RegIgnoreNodeDelayed(DiIntersect *di, CNode *node);
    int ClearIgnoreNodeDelayed(DiIntersect *di);
    int RegProximityDelayed(DiIntersect *di, const CPnt3D *p, float radius, unsigned int mask);
    void AddTextureAssetDecals(CTexHandle *tex);
    void AddTextureAssetCharacterAll(CTexHandle *tex);
    void RemoveDecals();
    void ClearShadowList();
    void ClearLightMapList();
    /* the character sets' textures (SOCOM 1's names; the library overload inline in SOCOM II, the
     * purge and the release new in it, their names ours) */
    /* the world model's children made the world's (SOCOM 1's) */
    bool DismemberWorldModel();
    void Segregate_CCells(CNode *node, CNodeVector &cells, CNodeVector &others);
    /* the children's storage and their atom pointers cut down (SOCOM 1's) */
    void GarbageCollect();
    void WipeoutTextureAssetCharacters();
    void ReleaseTextureAssetCharacters();
    void RemoveEmptyTextureAssetCharacters();
    void DeleteTextureAssetCharacter(CAssetLib *lib, CNode *node);
    void DeleteTextureAssetCharacter(_RenderPhase phase, CNode *node);
    void DeleteTextureAssetCharacter(char *name, CNode *node);
    /* the foliage sound's name, copied (new in SOCOM II: Load sets it; the name is ours) */
    void SetFoliageSound(const char *name);
    /* SOCOM II's REMOVE_DECALS command (the names ours, after CWind's) */
    static bool CmdTickRemoveDecals(_zanim_cmd_hdr *cmd, float *dt);
    static _zanim_cmd_hdr *CmdParseRemoveDecals(_zrdr *rdr);
    static void RegisterAnimCommands();
    static void Uninit();
    static void Init();

    CWind m_Wind;   // 0xc0 (SOCOM 1: 0x4d4)
    CCamera *m_camera;   // 0x128 (CCamera::CmdBeginParams; SOCOM 1: 0xc0)
    unsigned int m_default_soiltype;   // 0x12c
    char *m_default_soiltype_name;   // 0x130 (Load: "DefaultMaterial")
    char *m_foliage_sound;   // 0x134: SOCOM II's (Load: "FoliageSound"; the name ours)
    unsigned char _138[4];   // 0x138
    float m_world_text_specs[4];   // 0x13c: SOCOM II's (Load: "WorldTextSpecs"; the name ours)
    DiIntersect *m_diInt;   // 0x14c
    DiIntersect *m_diIntDelay[256];   // 0x150
    int m_diIntDelayCnt;   // 0x550
    _cellData **m_cellData;   // 0x554
    int m_maxOverlap;   // 0x558
    CLightList m_lights;   // 0x55c
    CRenderMapVector m_shadows;   // 0x568
    CLightMapVector m_lightMaps;   // 0x574
    std::vector<CShadowCaster *> m_shadow_casters;   // 0x580: SOCOM II's (the name ours)
    CLOD_Object m_LOD_Object;   // 0x58c
    CMaterial_Object m_Material_Object;   // 0x59c
    CSubSurf_Object m_SubSurf_Object;   // 0x5ac
    CPreLight_Object m_PreLight_Object;   // 0x5bc
    CScrollingTexture_Object m_ScrollingTexture_Object;   // 0x5cc
    unsigned char m_night_mission;   // 0x5dc
    unsigned char _5dd[3];
    PNT4D m_lensfx_nvg;   // 0x5e0 (SOCOM 1: a CPnt3D)
    PNT4D m_lensfx_starlightscope;   // 0x5f0
    _globalLight m_gLight;   // 0x600
    unsigned char m_gLightDirUpdate;   // 0x670
    unsigned char m_gLightColUpdate;   // 0x671
    unsigned char _672[2];
    CPnt3D m_shadow_vector;   // 0x674
    float m_shadow_weight;   // 0x680: SOCOM II's (Load: "ShadowWeight"; the name ours)
    CGrid m_grid;   // 0x684 (SOCOM 1: 0x670)
    CNodeVector m_landmarks;   // 0x6b98
    unsigned int m_numNoFarClipNodes;   // 0x6ba4
    CTextureAssetCharacterList m_TextureAssetCharacters;   // 0x6ba8
    CTexList m_TextureCharacterCommon;   // 0x6bb4
    CTexList m_TextureDecals;   // 0x6bc0
    CTexture *m_shadowTex[4];   // 0x6bcc
    CTexHandle *m_shadowTexH;   // 0x6bdc

    /* the landmark list, inline (CCamera::GetLandmarkFarPlane's loop keeps its address in a
     * register; the name is ours) */
    CNodeVector *GetLandmarks() { return &m_landmarks; }
};   // 0x6be0 bytes

/* every node by index (SOCOM 1's DWARF: a CNodeVector and a lock byte) */
class CNodeUniverse : public CNodeVector {
public:
    CNode *GetElement(int i);
    int GetIndex(CNode *node);
    void RemoveNode(CNode *node);
    bool AddNode(CNode *node);

    unsigned char m_locked;   // 0xc
};   // 0x10 bytes

extern CNodeUniverse *NodeUniverse;


}

#endif
