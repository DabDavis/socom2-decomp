/* node_io.cpp -- the scene graph read off a ZED archive: a node's own keys (its name, matrix and
 * bounds, its visuals, DI and children), a light's ranges, an instance of a model by name, a node
 * of the archive's type read and placed under its parent, a model and a library's model -- and
 * the matrix code before them, TransformToWorld and BuildMTW (OVL1 0x310980-0x311CBF).
 * THE UNIT, by SOCOM 1: the press beta's CNode::CreateInstance(CSaveLoad &) asserts node_io.cpp
 * (beta 0x2256A0, the only function there to name it), and the demo holds these functions in
 * this order -- TransformToWorld, BuildMTW, CLight::Read, CreateInstance, CNode::Read,
 * ReadDataBegin, Read(CSaveLoad &, CNode *), CModel::Read, CModel::Create, then std::find's and
 * std::remove_copy's instances for the model vector -- after the matrix library and before
 * CNodeVector::Remove. Function template instances close a unit: remove_copy<CModel *> (0x311C50)
 * ends this one, and node_vector.cpp starts at CNodeVector::Remove (0x311CC0). SOCOM II reads the
 * children apart (ReadChildren, the name ours) and drops CLight::_Write. Defined in reverse of
 * their image order (-inline deferred). */

#include <algorithm>

#include "node_world.h"

namespace zdb {

/* a lens flare (node_wave.cpp's class, node type 9: only its size matters here) */
class CLensFlare : public CNode {
public:
    CLensFlare();
    virtual ~CLensFlare();

    float _c0[3];   // 0xc0
    std::vector<unsigned int> m_flares;   // 0xcc
};   // 0xd8 bytes (0xe0 with the matrix's alignment)

/* a model taken off the library's list (std::remove, the tail erased), the asset manager's model
 * cache emptied and the model no longer the library's -- SOCOM 1's Create wrote the first two out
 * (SOCOM II's inline: its tree is taller than the level, so CModel::Create inlines its body alone
 * and calls every piece out of line, 0x311AD0-0x311C30; the name ours) */
inline void CAssetLib::RemoveModel(CModel *model)
{
    CModelVector::iterator e = m_models.end();
    CModelVector::iterator it = std::remove(m_models.begin(), e, model);

    m_models.erase(it, e);
    CAssetMgr::GetAssets()->ClearCache();
    model->SetAssetLib(0);
}

/* a model read from the archive's open key, made the library's; one that fails to read is taken
 * off the library again and deleted (SOCOM 1's) */
CModel *CModel::Create(CSaveLoad &io, CAssetLib *lib)
{
    CModel *model = new CModel(0);

    if (lib && model)
        lib->AddModel(model);
    if (!model->Read(io)) {
        if (lib && model)
            lib->RemoveModel(model);
        delete model;
        model = 0;
    }
    return model;
}

/* a model's node read: its parameters and region mask, then the node's own keys, and the name of
 * the key it was read from (SOCOM 1's). The answer is a bool set true at the top and kept in a
 * saved register to the end (daddiu s1 in GetOpenKey's delay slot, SOCOM 1's too): every
 * spelling tried folds it into the return's li -- a bool, int, long or unsigned long local, set
 * before or after the key, an inline's answer -- but under opt_propagation off, which matches.
 * The pragma inside the body: set ahead of the definition, the template members instantiated
 * when the definition opens (the model vector's begin, end, erase and std::remove, CModel::Create's
 * copies) are compiled under it as well, and four of them miss */
bool CModel::Read(CSaveLoad &io)
{
#pragma opt_propagation off
    bool result = true;
    zar::CKey *key = io.m_zfile.GetOpenKey();

    if (key) {
        tag_NODE_PARAMS params;

        InitNodeParams(&params, 0);
        io.m_zfile.Fetch("nparams", &params, 96);
        io.m_zfile.Fetch("regionmask", &m_region_mask, 4);
        *(tag_NODE_PARAMS *)this = params;
        CNode::Read(io);
        SetName(key->m_name);
    }
    return result;
}
#pragma opt_propagation on

/* a node read from the archive's open key and added to the parent (or the world): an archive
 * without node types made by its parameters' type -- an instance of a model, a node, a light or a
 * lens flare -- one with them a plain node (SOCOM 1's) */
CNode *CNode::Read(CSaveLoad &io, CNode *parent)
{
    int type = 0;
    CNode *node = 0;
    bool instance = false;

    bool old = !io.m_zfile.Fetch("NodeType", &type);

    if (old) {
        io.m_version = 1;
        unsigned int regionmask = 0;
        tag_NODE_PARAMS params;

        InitNodeParams(&params, 0);
        io.m_zfile.Fetch("nparams", &params, 96);
        io.m_zfile.Fetch("regionmask", &regionmask, 4);
        switch (params.m_type) {
        case 2:
            instance = true;
            break;
        case 5:
        case 1:
            node = new CNode;
            break;
        case 8:
            node = new CLight;
            break;
        case 9:
            node = new CLensFlare;
            break;
        }
        if (instance) {
            CNode *inst = CreateInstance(io);

            if (inst) {
                if (parent)
                    parent->AddChild(inst);
                else
                    CWorld::m_world->AddChild(inst);
                node = inst;
            }
        } else if (node) {
            *(tag_NODE_PARAMS *)node = params;
            node->m_region_mask = regionmask;
            if (node->Read(io)) {
                if (parent)
                    parent->AddChild(node);
            } else {
                delete node;
                node = 0;
            }
        }
    } else {
        /* the key declared first: numbered ahead of the archive, which then takes s0 */
        zar::CKey *key;
        zar::CZAR *zfile = &io.m_zfile;

        io.m_version = 0;
        key = zfile->GetOpenKey();

        zfile->OpenKey(key);
        zfile->Fetch("NodeType", &type);
        node = new CNode;
        if (node && node->Read(io) && parent)
            parent->AddChild(node);
        zfile->CloseKey(key);
    }
    return node;
}

/* the node's name, then its visuals and DI, each created from its key and added (SOCOM 1's) */
#pragma auto_inline off
int CNode::ReadDataBegin(CSaveLoad &io)
{
    zar::CZAR *zfile = &io.m_zfile;

    SetName(zfile->GetOpenKey()->m_name);
    if (zar::CKey *key = zfile->OpenKey("visuals")) {
        ReserveVisuals(key->size());
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            if (zfile->OpenKey(*it)) {
                CVisual *visual = CVisual::Create(*zfile);

                if (visual)
                    AddVisual(visual);
                zfile->CloseKey(*it);
            }
        }
        zfile->CloseKey(key);
    }
    if (zar::CKey *key = zfile->OpenKey("di")) {
        ReserveDI(key->size());
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            if (zfile->OpenKey(*it)) {
                CDI *di = CDI::Create(io);

                if (di)
                    AddDI(di);
                zfile->CloseKey(*it);
            }
        }
        zfile->CloseKey(key);
    }
    return 0;
}

/* the node's children, each read as a node of its own under this one (SOCOM 1's Read did this
 * itself; the name is ours) */
int CNode::ReadChildren(CSaveLoad &io)
{
    zar::CZAR *zfile = &io.m_zfile;
    zar::CKey *key = zfile->OpenKey("children");

    if (key) {
        ReserveChildren(key->size());
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            zfile->OpenKey(*it);
            CNode::Read(io, this);
            zfile->CloseKey(*it);
        }
        zfile->CloseKey(key);
    }
    return 0;
}
#pragma auto_inline reset

/* the node read: in the node-typed archive its name, visuals, DI and children; else its matrix and
 * bounds as well, each from its own key (SOCOM 1's) */
bool CNode::Read(CSaveLoad &io)
{
    if (io.m_version == 1) {
        ReadDataBegin(io);
        ReadChildren(io);
    } else {
        zar::CZAR *zfile = &io.m_zfile;

        SetName(zfile->GetOpenKey()->m_name);
        zfile->Fetch("matrix", this, 64);
        zfile->Fetch("bbox", &m_bbox, 24);
        if (zar::CKey *key = zfile->OpenKey("visuals")) {
            ReserveVisuals(key->size());
            for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
                if (zfile->OpenKey(*it)) {
                    AddVisual(CVisual::Create(*zfile));
                    zfile->CloseKey(*it);
                }
            }
            zfile->CloseKey(key);
        }
        if (zar::CKey *key = zfile->OpenKey("di")) {
            ReserveDI(key->size());
            for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
                if (zfile->OpenKey(*it)) {
                    CDI *di = CDI::Create(io);

                    if (di)
                        AddDI(di);
                    zfile->CloseKey(*it);
                }
            }
            zfile->CloseKey(key);
        }
        if (zar::CKey *key = zfile->OpenKey("children")) {
            ReserveChildren(key->size());
            for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
                zfile->OpenKey(*it);
                CNode::Read(io, this);
                zfile->CloseKey(*it);
            }
            zfile->CloseKey(key);
        }
    }
    return true;
}

/* an instance of the model the key names, with the key's parameters and region mask (type 2),
 * its own keys read after them -- none when the world has no such model (SOCOM 1's) */
CNode *CNode::CreateInstance(CSaveLoad &io)
{
    unsigned int regionmask = 0;
    tag_NODE_PARAMS params;

    InitNodeParams(&params, 0);
    char *modelname = 0;

    io.m_zfile.Fetch("model_name", &modelname);
    io.m_zfile.Fetch("nparams", &params, 96);
    io.m_zfile.Fetch("regionmask", &regionmask, 4);
    CModel *model = (CModel *)CWorld::GetModel(modelname);

    if (!model) {
        if (modelname)
            _free(modelname, 0, 0);
        modelname = 0;
        return 0;
    }
    if (modelname)
        _free(modelname, 0, 0);
    modelname = 0;
    CNode *node = CreateInstance(model, 0, 0);

    *(tag_NODE_PARAMS *)node = params;
    node->m_region_mask = regionmask;
    node->m_type = 2;
    node->ReadDataBegin(io);
    return node;
}

/* a light read: the node's keys, its colour, then its ranges, each setter updating their derived
 * terms and the bounds (SOCOM 1's) */
bool CLight::Read(CSaveLoad &io)
{
    float value = 0.0f;
    CPnt3D diffuse;

    ReadDataBegin(io);
    io.m_zfile.Fetch("diffuse", &diffuse, 12);
    m_diffuse = diffuse;
    io.m_zfile.Fetch("min_range", &value);
    SetMinRange(value);
    io.m_zfile.Fetch("max_range_sq", &value);
    SetMaxRangeSq(value);
    return true;
}

/* the node-to-world matrix: this node's, then each parent's up to the world (type 6, whose own
 * is the unit), multiplied in; none if the chain never reaches it. The recursion inlines itself
 * three times; the innermost copies call CMatrix's pieces out of line.
 * OPEN (right size, 44 differing): the third copy converts Multiply's arguments right to left
 * (this, b, then out: 0x310C60, 0x310C60, 0x310C50), where MW converts call arguments left to
 * right (probe build/scratch/inl/t10.cpp) -- so out first here, in every form tried (explicit
 * casts, implicit conversions to row pointers under a const prototype, a local) */
CMatrix *CNode::BuildMTW(CMatrix &m)
{
    if (m_type == 6) {
        m.SetIdentity();
        return &m;
    }
    m.Multiply(GetMatrix(), &m);
    if (m_parent && m_parent->m_type == 6)
        return &m;
    return m_parent ? m_parent->BuildMTW(m) : 0;
}

/* a point in the node's frame into the world's: each matrix up the tree, to the world's node
 * (type 6) */
void CNode::TransformToWorld(CPnt3D &p) const
{
    for (const CNode *n = this; n && n->m_type != 6; n = n->m_parent)
        n->m_matrix.Transform(&p, 1);
}

}
