/* zdb::CNode -- the scene graph's node (the zdb namespace is the mangled names'), its layout
 * SOCOM 1's DWARF as tools/dwarf1.py header writes it (MW puts the vtable pointer at 0x60, after
 * tag_NODE_PARAMS). Its virtual functions are SOCOM II's table, __vt__Q23zdb5CNode at 0x4067B0
 * (tools/s1vtables.py): SOCOM 1's fourteen with one new slot after the FindChild pair, so IsCell
 * and everything after it sit 4 bytes further than the demo's. SOCOM II keeps tag_NODE_PARAMS'
 * flags at 0x5C (zAnimNodeActiveTick reads m_active there). The header's name is ours. */
#ifndef ZNODE_H
#define ZNODE_H

#include <vector>

#include "../zmath/zmath.h"

class CZProjectile;

namespace zdb {

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
    unsigned int m_type;   // 0x58
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
    CNode *GetNode(const char *name);
};   // 0xc bytes

class CDIVector : public _opaque_vector_c {
public:
    unsigned char m_bStopAtFirstIntersection : 1;   // 0xc
    unsigned char m_bReturnBackFaceIntersections : 1;   // 0xc
    unsigned char m_padding : 6;   // 0xc
};   // 0x10 bytes

class CVisualVector : public _opaque_vector_c {
public:
};   // 0xc bytes

class CSaveLoad;

class CNode : public tag_NODE_PARAMS {
public:
    /* SOCOM II's vtable order (__vt__Q23zdb5CNode, 0x4067B0); the pointer is at 0x60 */
    virtual ~CNode();                                   /* +0x08 */
    virtual void _Copy(CNode *to) const;
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);                /* +0x18 */
    virtual void DeleteChild(CNode *child);
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);
    virtual CNode *FindChild(CNode *node, bool deep);   /* +0x28 */
    virtual void func_00315B90();                       /* +0x2C: new in SOCOM II, unread */
    virtual bool IsCell();
    virtual void Read(CSaveLoad &io);
    virtual void SetActive(bool on);
    virtual void UpdateBBox(bool up);
    virtual void UpdateBBoxFromDI(bool up);

    CNode();
    void SetName(const char *name);
    void SetNodeEx(CNodeEx *ex, bool b);
    float GetRadius();
    float GetRadiusSq();
    static CNode *CreateXRef(CModel *model, const CPnt3D *pos, const CQuat *rot);
    static CNode *CreateInstance(CModel *model, const CPnt3D *pos, const CQuat *rot);
    void TransformToWorld(CPnt3D &p) const;
    void BuildMTW(CMatrix &m);
    void SetMatrix(const CMatrix *m);
    void GetRotation(CPnt3D *out) const;
    void SetRotation(const CPnt3D *euler);
    void SetRotation(const CQuat &q);
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
    char m_customGlobalLight;   // 0x98
    unsigned char m_frameRendered;   // 0x99
    unsigned short m_flatten : 1;   // 0x9a
    unsigned short m_modified : 1;   // 0x9a
    unsigned short m_character : 1;   // 0x9a
    unsigned short m_character_infov : 1;   // 0x9a
    unsigned short m_unused : 12;   // 0x9a
    float m_Opacity;   // 0x9c
    /* SOCOM II's at 0xa0, where SOCOM 1's DWARF has m_Atom: CZAnim::State and CreateInstanceNode
     * test the byte at 0xa1 with a mask (4) and CreateInstanceNode sets that bit as a bitfield on
     * an instance -- both views, names unknown */
    unsigned char m_socom2_a0;   // 0xa0
    union {
        unsigned char m_socom2_a1;   // 0xa1
        struct {
            unsigned char lo : 2;
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
    CModel* m_model;   // 0xb0
    char* m_modelname;   // 0xb4
    unsigned int m_region_mask;   // 0xb8
};   // 0xc0 bytes

/* a model node: what CZAnim::CreateInstanceNode instances (dynamic_cast from CNode) */
/* a cell of the world's grid (SOCOM 1's names; its own members not read yet) */
class CCell : public CNode {
};

class CModel : public CNode {
public:
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

    unsigned char _c0[0x684 - 0xC0];   // 0xc0: SOCOM 1's m_camera .. m_shadow_vector, not yet needed
    CGrid m_grid;   // 0x684 (SOCOM 1: 0x670)
    CNodeVector m_landmarks;   // 0x6b98

    /* the landmark list, inline (CCamera::GetLandmarkFarPlane's loop keeps its address in a
     * register; the name is ours) */
    CNodeVector *GetLandmarks() { return &m_landmarks; }
};

extern CNodeVector *NodeUniverse;


}

#endif
