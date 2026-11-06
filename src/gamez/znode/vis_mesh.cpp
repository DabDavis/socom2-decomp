/* vis_mesh.cpp -- zdb::CMesh and CSubMesh, the skinned meshes (SOCOM 1's vis_mesh.cpp, by the
 * press beta's asserts): OVL1 0x3B8000?-0x3BA2FF, after vis_main.cpp's tail (tag_GlobalLight's
 * constructor, 0x3B7FD0) and before ColConvTo3D, as SOCOM 1's 0x30ED60-0x31045F. Only some of its
 * functions so far; the rest stay generated drafts or placeholders. Defined in reverse of their
 * image order (-inline deferred). */

#include "znode.h"
#include <string.h>
#include "../zsys/zsys_public.h"

namespace zdb {

class CMeshDecal;

/* the mesh decals' pools (vis_decal.cpp's class: only the take here) */
class CMeshDecalPool {
public:
    CMeshDecal *Get(bool b);
};

/* a decal on a mesh: what the mesh's walks touch (vis_decal.cpp's class; SOCOM 1's, no DWARF --
 * the names ours). A decal's life below zero is a permanent one */
class CMeshDecal {
public:
    void FreeMem();
    static void PutTempDecal(CMeshDecal *d);
    static void PutPermDecal(CMeshDecal *d);
    /* cleared for reuse: the life, the link and the vertex words (inline; SOCOM 1's stores alike) */
    void Reset()
    {
        m_life = 0.0f;
        m_next = 0;
        m_8 = 0;
        m_verts = 0;
        m_34 = 0;
        m_30 = 0;
    }

    static CMeshDecalPool m_tempPool;
    static CMeshDecalPool m_permPool;

    float m_life;   // 0x0
    CMeshDecal *m_next;   // 0x4
    unsigned int m_8;   // 0x8
    unsigned int m_c;   // 0xc
    void *m_verts;   // 0x10
    unsigned char _14[0x30 - 0x14];
    unsigned int m_30;   // 0x30
    unsigned int m_34;   // 0x34
};

}

using namespace zdb;

/* a mesh decal built over a strip of the mesh's vertices, posed by its matrices (SOCOM II's, not
 * named: vis_decal.cpp's first function, likely CMeshDecal's own AddDecal -- SOCOM 1's
 * CDecal::AddDecal takes the life first too). The life is the first parameter after the decal:
 * AddDecal's set-up moves it into $f12 ahead of the pointers, as only that order (or the life
 * first of all) gives; last, it is set up after the strip and the texture */
extern "C" void func_003BD9E0(CMeshDecal *d, float life, void *strip, CTexHandle *tex, CMesh *mesh,
                              std::vector<CMatrix *> *palette);

/* the decal list's end reached (inline: the walk's materialised test; the name ours) */
static inline bool DecalEnd(CMeshDecal *d) { return !d; }

/* constructed empty, as a mesh (type 1): one level of detail, used out to 1e12, room for three
 * (SOCOM 1's used FLT_MAX), no matrices and no decals */
CMesh::CMesh()
{
    m_type = 1;
    m_original = 0;
    m_owner = 0;
    m_1c = 3;

    CMeshLOD lod;

    lod.m_dist = 1e12f;
    lod.m_mesh = 0;
    lod.m_buffer = 0;
    m_lods.reserve(3);
    m_lods.push_back(lod);
    m_numMatrices = 0;
    m_74 = 0;
    m_decals = 0;
}

/* a matrix per sub-mesh, each new and the identity (SOCOM 1's) */
void CMesh::InitMatrices()
{
    for (unsigned int i = 0; i < m_numMatrices; i++)
        m_palette.push_back(new CMatrix);
}

/* another mesh's first level of detail appended: its select block copied and pointed at its
 * buffer, its textures listed, and the previous level used within the distance, squared
 * (SOCOM 1's) */
void CMesh::AddLOD(CMesh *mesh, float dist)
{
    m_lods.push_back(*mesh->m_lods.begin());

    _word128 *buffer = (*mesh->m_lods.begin()).m_buffer;
    _word128 *selects = (_word128 *)(*mesh->m_lods.begin()).m_mesh;
    unsigned int size = (selects->u16[0] + 1) << 4;
    _word128 *copy = (_word128 *)_malloc(size, 0, 0);

    memcpy(copy, selects, size);
    SetMeshBuffer(copy, buffer, selects);
    m_dynTexList.Add(&mesh->m_dynTexList);

    CMeshLOD &lod = m_lods.back();

    lod.m_mesh = (CMesh *)copy;
    lod.m_buffer = buffer;
    m_lods[m_lods.size() - 2].m_dist = dist * dist;
}

/* a view's level of detail by distance, when it has more than one: the first that reaches past
 * the distance (SOCOM 1's; SOCOM II's walk re-reads the data pointer each pass: include/vector's
 * class end() takes `data() + size` into a const local) */
void CMesh::SetLOD(unsigned int view, float dist)
{
    if (m_lods.size() > 1) {
        for (std::vector<CMeshLOD>::iterator it = m_lods.begin(); it != m_lods.end(); ++it) {
            if (dist < (*it).m_dist) {
                m_lodMesh[view] = (*it).m_mesh;
                break;
            }
        }
    }
}

/* a view's level of detail, clamped to the last one (SOCOM 1's took it as given) */
void CMesh::SetLOD(unsigned int view, unsigned int lod)
{
    if (lod >= m_lods.size())
        lod = m_lods.size() - 1;
    m_lodMesh[view] = m_lods[lod].m_mesh;
}

/* the mesh decals' batch (vis_decal.cpp's, new in SOCOM II, not named): begun, flushed, ended; a
 * vertex index entered at its slot, a new vertex added (its strip's shorts, its matrix, its
 * projected point, its index and slot, the strip's walk) */
extern "C" void func_003BEBD0();
extern "C" void func_003BEBB0();
extern "C" void func_003BEA60();
extern "C" void func_003BEA20(int slot);
extern "C" void func_003BE5F0(short *s, unsigned int mtx, CPnt3D *p, unsigned short id, short slot,
                              CVisData *v);

/* the frame count (a 64-bit word: a mesh's palette is rebuilt when it was built before it), the
 * decal's depth test and its limit, the batch's vertex indices (new in SOCOM II, not named) */
extern "C" unsigned long data_004A4490;
extern "C" unsigned char data_003E14B8;
extern "C" float data_003E14B0;
extern "C" unsigned int *data_004B5018;

/* set while hookupVisuals trims a visual's level-of-detail views (CVisual::SetBuffer then leaves the
 * buffer's owner's count alone): this unit's bss byte, 0x4B5010, right ahead of data_004B5018 */
unsigned char CVisual::m_noBufferRefs;

/* the UV test (vis_decal.cpp's) */
bool UVCompare(CPnt3D *a, CPnt3D *b, CPnt3D *c);

/* a decal applied to one of the mesh's strips (a sub-mesh's index): the palette rebuilt when stale,
 * the strip's triangles walked -- the batch flushed into a decal at each strip's end or past 85
 * vertices -- each triangle's points (its shorts, ten times scaled) kept with their vertices and
 * indices; on a new matrix the UV projection composed with it and the normal brought into its
 * frame; a triangle facing against the normal (below -0.1) projected and, inside the texture (and
 * near its plane when the depth test is on), its vertices entered in the batch, new ones added; the
 * triangles taken (new in SOCOM II: CNode::ApplyDecal's sub-mesh arm, the name ours -- its call
 * there, node_main.cpp's 0x313B2C, loads the index before the mesh: the mesh is this, the node's
 * matrix the unused first argument).
 * OPEN (written in the weapons' round 33), 4 instructions long (364 against 360), the frame 16 bytes
 * short (832 against 848: every local 16 lower, the spilled normal / texture / UV at 204 / 200 / 196
 * where the target has 220 / 216 / 192 and the dead counter at 176 -- the target spills one node
 * more or keeps a 16-byte slot this does not); the target also keeps &vd in v's register from the
 * top (`addiu s4,sp,656` before the matrices' constructors: `v = &vd` then SelectIndexedStrip, as
 * CVisual::ApplyDecal's) and builds the outer product's operands as temporaries below the matrices
 * (c at 256, the two w-1 points at 240 / 224 -- a cross-product inline holding c gives that order but
 * 358 instructions); the flush's func_003BEBB0 is called before nidx is cleared there */
int CMesh::ApplyDecal(CMatrix *mtw, unsigned int index, float size, CPnt3D *normal, CTexHandle *tex,
                      CMatrix *uv)
{
    bool more = false;
    short *vp[3];
    CPnt3D e1;
    CPnt3D e2;
    CPnt3D nn;
    int ids[3];
    CPnt3D n;
    CVisData vd;
    CPnt3D pts[3];
    CPnt3D proj[3];
    CMatrix m0;
    CMatrix m2;
    unsigned int nt = 0;
    unsigned int nverts = 0;
    int dead = 0;
    int count = 0;

    func_003BEBD0();
    if (m_74 < data_004A4490)
        m_owner->BuildMatrixPalette(0);

    CVisData *v = vd.SelectIndexedStrip(this, index);
    unsigned int nidx = 0;
    unsigned int cur = 99999999;

    while (v) {
        if (more || nverts >= 85) {
            if (nt) {
                AddDecal((void *)nt, tex, size);
                nt = 0;
            }
            nverts = 0;
            func_003BEBB0();
            nidx = 0;
        }
        for (unsigned int j = 0; j < 3; j++) {
            dead++;
            pts[j].x = 10.0f * (((short *)v->m_strip)[0] / 32768.0f);
            pts[j].y = 10.0f * (((short *)v->m_strip)[1] / 32768.0f);
            pts[j].z = 10.0f * (((short *)v->m_strip)[2] / 32768.0f);
            vp[j] = (short *)v->m_strip;
            ids[j] = *v->m_indices / 3;
            if (j < 2)
                v = v->NextIndex(&more);
        }

        unsigned int mtx = v->m_mark->u32[2];

        if (cur != mtx) {
            cur = mtx;

            CMatrix pm;

            sceVu0MulMatrix(pm.m_matrix, m_owner->m_matrix.m_matrix, m_palette[cur]->m_matrix);
            sceVu0MulMatrix(m2.m_matrix, uv->m_matrix, pm.m_matrix);

            CMatrix inv;

            Vu0MatrixInverseWithUniformScale(&inv, &pm);
            TransformVectors(&inv, normal, &n, 1);
        }
        pts[0].Sub(&pts[1], &e1);
        pts[2].Sub(&pts[1], &e2);

        CPnt4D c;

        sceVu0OuterProduct((float *)&c, (float *)&CPnt4D(e2.x, e2.y, e2.z, 1.0f),
                           (float *)&CPnt4D(e1.x, e1.y, e1.z, 1.0f));
        nn = CPnt3D(c.x, c.y, c.z);
        nn.Normalize();
        if (Dot(nn, n) < -0.1f) {
            m2.Transform(&pts[0], &proj[0], 3);
            if (UVCompare(&proj[0], &proj[1], &proj[2]) &&
                (!data_003E14B8 || fabsf(proj[0].z) <= data_003E14B0 || fabsf(proj[1].z) <= data_003E14B0 ||
                 fabsf(proj[2].z) <= data_003E14B0)) {
                for (unsigned int k = 0; k < 3; k++) {
                    int slot = -1;
                    unsigned int id = ids[k];

                    for (unsigned int m = 0; m < nidx; m++) {
                        if (id == data_004B5018[m]) {
                            slot = m;
                            break;
                        }
                    }
                    if (slot < 0) {
                        slot = nidx;
                        data_004B5018[nidx] = id;
                        nidx++;
                        func_003BE5F0(vp[k], cur, &proj[k], id, slot, v);
                    }
                    func_003BEA20(slot);
                    nverts++;
                }
                nt++;
                count++;
            }
        }
        v = v->NextIndex(&more);
    }
    if (nt)
        AddDecal((void *)nt, tex, size);
    func_003BEA60();
    return count;
}

/* a decal added over a strip of the mesh, last in its list: taken from the permanent pool for a
 * life below zero, else from the temporary one, cleared and built (SOCOM II's; the name ours).
 * Each arm takes its decal into a local of its own: with it MW pads the function's labels to 8
 * bytes (the nop ending each arm, the loop's), as the target's -- the plain arms, a ternary and the
 * helpers round 14 tried give none (CTurret's timer reset pads so for its arm's random factor) */
void CMesh::AddDecal(void *strip, CTexHandle *tex, float life)
{
    CMeshDecal *d;

    if (life < 0.0f) {
        CMeshDecal *t = CMeshDecal::m_permPool.Get(false);

        d = t;
    } else {
        CMeshDecal *t = CMeshDecal::m_tempPool.Get(true);

        d = t;
    }
    if (d) {
        CMeshDecal *last = m_decals;

        while (!DecalEnd(last) && last->m_next)
            last = last->m_next;
        d->Reset();
        func_003BD9E0(d, life, strip, tex, this, &m_palette);
        d->m_next = 0;
        if (last)
            last->m_next = d;
        else
            m_decals = d;
        FlushCache(0);
    }
}

/* a copy of the mesh for a node: the visual's and the mesh's members assigned, a reference to this
 * mesh taken, its own matrices, and its first level's selects copied and pointed at this mesh's
 * buffer (SOCOM 1's took no node) */
CMesh *CMesh::Clone(CNode *owner)
{
    CMesh *mesh = new CMesh;

    *mesh = *this;
    mesh->m_refs = 0;
    m_refs++;
    mesh->m_original = dynamic_cast<CMesh *>((CVisual *)this);
    mesh->InitMatrices();
    mesh->m_owner = owner;
    m_74 = 0;

    _word128 *buffer = (*m_lods.begin()).m_buffer;
    _word128 *selects = (_word128 *)(*m_lods.begin()).m_mesh;
    unsigned int size = (selects->u16[0] + 1) << 4;
    _word128 *copy = (_word128 *)_malloc(size, 0, 0);

    memcpy(copy, selects, size);
    mesh->SetMeshBuffer(copy, buffer, selects);
    mesh->m_dynTexList.Add(&m_dynTexList);

    CMeshLOD &lod = mesh->m_lods.back();

    lod.m_mesh = (CMesh *)copy;
    lod.m_buffer = buffer;
    mesh->m_lodMesh[0] = (CMesh *)copy;
    return mesh;
}

/* its decals let go of, each to the pool its life says; a clone's reference given back, the
 * original deleted by its last holder, and the levels' select blocks freed; its matrices deleted
 * (SOCOM 1's gave every decal back as temporary and marked the original 0xDEADBEEF) */
CMesh::~CMesh()
{
    CMeshDecal *d;
    bool clone = m_original != 0;

    d = m_decals;

    if (d) {
        do {
            CMeshDecal *next = d->m_next;
            float life = d->m_life;

            d->FreeMem();
            d->Reset();
            if (life >= 0.0f)
                CMeshDecal::PutTempDecal(d);
            else
                CMeshDecal::PutPermDecal(d);
            d = next;
        } while (d);
    }
    m_decals = 0;
    if (clone) {
        if (m_original->Release() == 0) {
            delete m_original;
            m_original = 0;
        }
        while (m_lods.size()) {
            CMeshLOD &lod = m_lods.back();

            if (lod.m_mesh)
                _free(lod.m_mesh, 0, 0);
            lod.m_mesh = 0;
            m_lods.pop_back();
        }
    }
    while (m_palette.size()) {
        CMatrix *m = m_palette.back();

        m_palette.pop_back();
        if (m)
            delete m;
    }
}

/* a block of selects pointed at a buffer: its decals dropped, the buffer recorded in the head,
 * each matrix select given its matrix (the palette grown to reach it -- SOCOM 1's took the
 * palette as it was) and each data select offset into the buffer (SOCOM 1's; the names unused) */
void CMesh::SetMeshBuffer(_word128 *head, _word128 *base, _word128 *names)
{
    m_decals = 0;
    head->u32[2] = (unsigned int)base;
    unsigned int n = head->u16[0];

    head++;
    for (unsigned int i = 0; i < n; i++, head++) {
        switch (head->u8[2]) {
        case 9: {
            unsigned int idx = head->u32[1];

            while (idx >= m_palette.size())
                m_palette.push_back(new CMatrix);
            head->u32[1] = (unsigned int)m_palette[idx];
            break;
        }
        case 7:
        case 10:
        case 11:
        case 12:
            head->u32[1] += (unsigned int)base;
            break;
        case 5:
        case 6:
        case 8:
            break;
        }
    }
}

/* a block of texture selects resolved once: the head's count, its done flag set, each select of
 * type 6 (a name) resolved (SOCOM 1's) */
void CMesh::SetMeshTextureSelects(_word128 *head, _word128 *names)
{
    unsigned char *done = &head->u8[2];

    if (*done == 0) {
        unsigned int n = head->u16[0];

        *done = 1;
        head++;
        for (unsigned int i = 0; i < n; i++, head++) {
            switch (head->u8[2]) {
            case 6:
                ResolveTextureName(names, head);
                break;
            }
        }
    }
}

/* a decal unlinked from the mesh's list, its link cleared (SOCOM 1's) */
void CMesh::RemoveMeshDecal(CMeshDecal *decal)
{
    CMeshDecal *d = m_decals;
    CMeshDecal *prev = 0;

    while (d) {
        if (d == decal) {
            CMeshDecal *next = d->m_next;

            if (prev)
                prev->m_next = next;
            else
                m_decals = next;
            d->m_next = 0;
            break;
        }
        prev = d;
        d = d->m_next;
    }
}

/* every decal of the mesh let go of: freed, cleared, and given back to the pool its life says it
 * came from (SOCOM 1's gave each back as temporary) */
void CMesh::DereferenceDecals()
{
    CMeshDecal *d = m_decals;

    if (d) {
        do {
            bool temp;
            CMeshDecal *next = d->m_next;

            temp = d->m_life >= 0.0f;

            d->FreeMem();
            d->Reset();
            if (temp)
                CMeshDecal::PutTempDecal(d);
            else
                CMeshDecal::PutPermDecal(d);
            d = next;
        } while (d);
    }
    m_decals = 0;
}
