/* zdb::CNode -- the scene graph's node (the zdb namespace is the mangled names'), its layout
 * SOCOM 1's DWARF as tools/dwarf1.py header writes it, and its virtual functions are SOCOM 1's
 * vtable's, in its order (MW puts the pointer at 0x60, after tag_NODE_PARAMS). SOCOM II keeps tag_NODE_PARAMS' flags
 * at 0x5C (zAnimNodeActiveTick reads m_active there). The header's name is ours. */
#ifndef ZNODE_H
#define ZNODE_H

#include "../zmath/zmath.h"

namespace zdb {

class CGridAtom;
class CModel;
class CNode;
class CNodeEx;

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

class CNodeVector : public _opaque_vector_c {
public:
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
    /* SOCOM 1's vtable order (__vt__Q23zdb5CNode); the pointer is at 0x60 */
    virtual ~CNode();
    virtual void _Copy(CNode *to) const;
    virtual void UpdateGrid();
    virtual void ReserveChildren(int n);
    virtual void AddChild(CNode *child);
    virtual void DeleteChild(CNode *child);
    virtual void DeleteChildren();
    virtual CNode *FindChild(const char *name, bool deep);
    virtual CNode *FindChild(CNode *node, bool deep);
    virtual bool IsCell();
    virtual void Read(CSaveLoad &io);
    virtual void SetActive(bool on);
    virtual void UpdateBBox(bool up);
    virtual void UpdateBBoxFromDI(bool up);

    void TransformToWorld(CPnt3D &p) const;
    void BuildMTW(CMatrix &m);
    void SetPosition(float x, float y, float z);
    void SetPosition(const PNT3D *p)      /* the base pointer keeps MW from folding the null test */
    {
        if (p)
            SetPosition(p->x, p->y, p->z);
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
    CGridAtom** m_Atom;   // 0xa0
    int m_TickNum;   // 0xa4
    short m_AtomCnt;   // 0xa8
    short m_AtomAlloc;   // 0xaa
    short m_count;   // 0xac
    short m_vid;   // 0xae
    CModel* m_model;   // 0xb0
    char* m_modelname;   // 0xb4
    unsigned int m_region_mask;   // 0xb8
};   // 0xc0 bytes


}

#endif
