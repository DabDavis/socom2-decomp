/* zdb::CNode -- the scene graph's node (the zdb namespace is the mangled names'), its layout
 * SOCOM 1's DWARF as tools/dwarf1.py header writes it (MW puts the vtable pointer at 0x60, after
 * tag_NODE_PARAMS). Its virtual functions are SOCOM II's table, __vt__Q23zdb5CNode at 0x4067B0
 * (tools/s1vtables.py): SOCOM 1's fourteen with one new slot after the FindChild pair, so IsCell
 * and everything after it sit 4 bytes further than the demo's. SOCOM II keeps tag_NODE_PARAMS'
 * flags at 0x5C (zAnimNodeActiveTick reads m_active there). The header's name is ours. */
#ifndef ZNODE_H
#define ZNODE_H

#include <list>
#include <vector>

#include "../zmath/zmath.h"

class CZProjectile;

namespace zdb {

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
    CMatrix m_matrix;   // 0x0
    CBBox m_bbox;   // 0x40
    unsigned short m_type;   // 0x58 (SOCOM II's CNode code reads a halfword; SOCOM 1's DWARF: an int)
    unsigned short m_socom2_5a;   // 0x5a
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
 * DI_PARAMS; SOCOM II's AddDI counts at 4) */
class CDI {
public:
    unsigned int _0;
    unsigned int m_refcount;   // 0x4
};

class CDIVector : public std::vector<CDI *> {
public:
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
    unsigned int m_flags;   // 0x0: tested by mask, the whole word loaded (AddVisual)
    signed char m_type;   // 0x4
    unsigned char _5[0x18 - 5];
    int m_refs;   // 0x18
    unsigned char _1c[0x20 - 0x1c];
};

class CVisual : public tag_VISUAL_HEAD {
public:
    virtual ~CVisual();
};

class CSubMesh : public CVisual {
public:
};

class CMesh : public CVisual {
public:
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
    virtual void Read(CSaveLoad &io);
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
    void ShrinkAtomBasePtrs();
    void AllocateAtomBasePtrs(int x, int z);
    void SetNodeEx(CNodeEx *ex, bool b);
    bool AddVisual(CVisual *visual);
    bool AddDI(CDI *di);
    static int VisualEffectsTick(float dt, void *data);
    void SetParentHasVisuals();
    float GetRadius();
    float GetRadiusSq();
    static CNode *CreateXRef(CModel *model, const CPnt3D *pos, const CQuat *rot);
    static CNode *CreateInstance(CModel *model, const CPnt3D *pos, const CQuat *rot);
    static CNode *CreateInstance(const char *name, const CPnt3D *pos, const CQuat *rot);
    static CNode *Delete(CNode *node);
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
    void GetRotation(CPnt3D *out) const;
    void SetRotation(const CPnt3D *euler);
    void SetRotation(const CQuat &q);
    /* the pointer form, null tested (CreateXRef tests twice, as for SetPosition's) */
    void SetRotation(const CQuat *q)
    {
        if (q)
            SetRotation(*q);
    }
    void SetPosition(float x, float y, float z);
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
            unsigned char hi : 5;
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
};   // 0xc0 bytes

/* a light: a node with a range (SOCOM 1's DWARF) */
class CLight : public CNode {
public:
    CLight();
    virtual ~CLight();
    virtual CNode *_Copy(CNode *to) const;
    virtual bool SetActive(bool on);
    virtual CBBox *UpdateBBox(bool up);
    /* inline (CLightMap::SetTexture's range is loaded ahead of its constant: the name ours) */
    float GetMaxRange() const { return m_maxRange; }

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

/* a texture: only its size placed (SOCOM 1's DWARF: CTexture : TEXTURE_PARAMS, the width first) */
class CTexture {
public:
    unsigned short m_width;   // 0x0
    unsigned short m_height;   // 0x2
};

/* a GS alpha blend, its register word (SOCOM 1's DWARF: sceGsAlpha and the word, one union) */
class BLENDMODE {
public:
    unsigned long m_alphaWORD;   // 0x0
};   // 8 bytes

/* a light that projects a texture (SOCOM 1's DWARF) */
class CLightMap : public CLight {
public:
    virtual ~CLightMap();
    void SetTexture(CTexture *tex);

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
/* a cell of the world's grid (SOCOM 1's names; its own members not read yet) */
class CCell : public CNode {
};

/* the render phase an asset library belongs to (SOCOM 1's DWARF; a byte under -enum min) */
enum _RenderPhase {
    WORLD, HUD_OBSOLETE, CHARSETCOMMON, CHARSET1, CHARSET2, CHARSET3, CHARSET4, CHARSET5,
    CHARSET6, CHARSET7, CHARSET8, ALPHA, HUDSET1, HUDSET2, HUDSETSMALL, SHADOW, CHARSETCUSTOM,
    NUM_RENDER_PHASES
};

/* a library of models and textures: only its render phase placed (SOCOM 1's DWARF has it at
 * 0x2B, after the name and three lists; CNode::CreateXRef reads it at 0x2F here) */
class CAssetLib {
public:
    unsigned char _0[0x2f];
    _RenderPhase m_renderphase;   // 0x2f (SOCOM 1: 0x2b)
};

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
    void RemoveInstance(CNode *node)
    {
        m_list.remove(node);
        if (m_parent == node)
            m_parent = 0;
    }
};

/* a rectangle in x/z (SOCOM 1's DWARF) */
class CFRect {
public:
    float top;   // 0x0
    float bottom;   // 0x4
    float left;   // 0x8
    float right;   // 0xc
};   // 0x10 bytes

/* the world's grid of cells: SOCOM 1's DWARF has CGrid : tag_GRID_PARAMS, 0x651C bytes, its
 * extents at 0x14. SOCOM II's extents sit 0x14 later in CWorld and its landmark list 0xC later,
 * so 8 bytes went from the grid or from what follows it; only the head is placed yet */
class CGrid {
public:
    unsigned char _0[0x14];   // tag_GRID_PARAMS
    CFRect m_extents;   // 0x14
    unsigned char _24[0x6514 - 0x24];
};

/* the scene's root; SOCOM 1's DWARF has its 0x6BE0 bytes of members. SOCOM II's grid is 0x14
 * bytes further on (CCamera::GetLandmarkFarPlane reads its extents at 0x698), its landmark list
 * 0xC (GetDatabaseNode reads it at 0x6B98). */
class CWorld : public CNode {
public:
    static CWorld *m_world;
    static CNode *GetModel(const char *name);
    void DeleteLightMap(CLightMap *light);

    unsigned char _c0[0x128 - 0xC0];   // 0xc0: SOCOM II's, not yet read
    CCamera *m_camera;   // 0x128 (CCamera::CmdBeginParams; SOCOM 1: 0xc0)
    unsigned char _12c[0x684 - 0x12C];   // SOCOM 1's m_default_soiltype .. m_shadow_vector
    CGrid m_grid;   // 0x684 (SOCOM 1: 0x670)
    CNodeVector m_landmarks;   // 0x6b98

    /* the landmark list, inline (CCamera::GetLandmarkFarPlane's loop keeps its address in a
     * register; the name is ours) */
    CNodeVector *GetLandmarks() { return &m_landmarks; }
};

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
