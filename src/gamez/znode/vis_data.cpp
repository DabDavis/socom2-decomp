/* vis_data.cpp -- ColConvTo3D, zdb::CVisData (the cursor over a visual's VU select block) and the
 * world's per-band tables, the *_Object classes and their readers: OVL1 0x3BA300-0x3BD99F, right
 * after vis_mesh.cpp's tail (its CMeshLOD deleter's clear, 0x3BA2F0) and before vis_decal.cpp
 * (CMeshDecalPool::Uninit, 0x3BD9A0) -- as SOCOM 1's 0x310460-0x312DDF, between its vis_mesh.cpp and
 * vis_decal.cpp. No assert names it in either build: the file name is ours (the wep lane's round
 * 11). SOCOM 1's order: ColConvTo3D, CVisData, then the *_Object pairs and readers; its template
 * tail the band vectors' insert, resize, erase, the deleters' destructors and clears. Defined in
 * reverse of their image order (-inline deferred). */

#include "znode.h"
#include "node_world.h"
#include "../zar/zar.h"
#include "../zsys/zsys_public.h"

/* a texture handle the world's loads fall back on (0x4B4D90, BSS: cleared with
 * CVisual::m_lightMapTexture on a world's release -- not named yet) */
extern "C" zdb::CTexHandle *data_004B4D90;
/* a mesh's select block for a level of detail (vis_mesh.cpp's 0x3B9480, new in SOCOM II: not
 * named yet) */
extern "C" _word128 *func_003B9480(zdb::CMesh *mesh, unsigned int lod);

namespace zdb {

/* The template tail: the band vectors' resizes, erases, the deleters' destructors and clears are
 * byte-exact from include/vector; the three class inserts are OPEN -- insert<CMaterial_band>
 * (0x3BC680) is 0x68 bytes long here: the temporary's deleter inlines clear's loop (a _free per
 * band) where the target calls clear (0x3BD870), so clear was generated after the insert there
 * (README, the tail's generation order), and its in-place branch numbers the end pointer and the
 * copy's source against the target's (the class inserts' open register, include/vector);
 * insert<CLOD_band> (0x3BBFE0, 0x5C4 against 0x5CC) and insert<CScrollingTexture_band> (0x3BB7E0,
 * 0x754 against 0x75C) are 8 bytes short each, as insert<CVehicleDef> 0x3AF040 is. */

/* the colour a material band starts with: 0, 1, 0, 0 (the unit's initialiser, 0x4049E0 -- new in
 * SOCOM II; SOCOM 1's vis_data.cpp had none) */
CPnt4D CMaterial_band::m_defaultColor(0.0f, 1.0f, 0.0f, 0.0f);

/* constructed empty, room for 256 bands (SOCOM 1's) */
CLOD_Object::CLOD_Object()
{
    m_bandMax = 256;
}

CLOD_Object::~CLOD_Object()
{
}

/* the bands read whole from the archive's LOD_Object key (SOCOM 1's; SOCOM II's takes the archive
 * without testing the driver) */
void CLOD_Object::Read(CSaveLoad *io)
{
    zar::CKey *key;
    zar::CZAR *z = &io->m_zfile;

    key = z->OpenKey("LOD_Object");
    if (!key) {
        z->GetOpenKey();
        return;
    }
    resize(key->m_size / sizeof(CLOD_band));
    z->Fetch(key, data_, key->m_size, 0);
    z->CloseKey(key);
}

CSubSurf_Object::CSubSurf_Object()
{
    m_bandMax = 256;
}

CSubSurf_Object::~CSubSurf_Object()
{
}

/* a copy: the fields one by one, and the texture name duplicated when the band owns it */
CMaterial_band &CMaterial_band::operator=(const CMaterial_band &b)
{
    m_rgb = b.m_rgb;
    m_opacity = b.m_opacity;
    m_intensity = b.m_intensity;
    m_18 = b.m_18;
    m_ownsName = b.m_ownsName;
    m_1c = b.m_1c;
    m_type = b.m_type;
    m_color = b.m_color;
    m_falloff = b.m_falloff;
    m_invFalloff = b.m_invFalloff;
    m_name = 0;
    if (b.m_ownsName)
        m_name = _strdup(b.m_name, 0, 0);
    else
        m_name = b.m_name;
    return *this;
}

CMaterial_Object::CMaterial_Object()
{
    m_bandMax = 256;
}

CMaterial_Object::~CMaterial_Object()
{
}

/* the palette: a key per material, each read whole (0x3C bytes) with its texture's name, the name
 * resolved into its handle -- the fallback texture when the asset manager has none -- and freed
 * (SOCOM II's own: SOCOM 1's read the bands whole from one key). OPEN, 20 instructions, registers
 * only: the target gives the name s0, the loop's two strength-reduced offsets s1/s2 and the band s3
 * (the name numbered after the compiler's induction temporaries); here the offsets take s0/s1, the
 * band s2 and the name s3. Tried (build/scratch/wep/v_matread.py): every local declared at the top,
 * key ahead of the archive (ScrollingTexture's order), the band a reference, the name declared ahead
 * of the band, at the loop's top or the function's -- all 20 or 31 */
void CMaterial_Object::Read(CSaveLoad *io)
{
    if (io) {
        zar::CZAR *z = &io->m_zfile;

        if (z) {
            zar::CKey *key = z->OpenKey("Material_Palette");

            if (key) {
                unsigned int n = key->size();

                resize(n);
                for (unsigned int i = 0; i < n; i++) {
                    zar::CKey *sub = (*key)[i];

                    z->OpenKey(sub);

                    CMaterial_band *band = &(*this)[i];

                    z->Fetch("dat", band, sizeof(CMaterial_band));
                    z->Fetch("tex_name", &band->m_name);
                    if (band->m_ownsName) {
                        char *name = band->m_name;

                        if (name)
                            band->m_tex = CAssetMgr::m_assets.GetTexHandle(name);
                        if (!band->m_tex)
                            band->m_tex = data_004B4D90;
                        band->m_type = 2;
                        if (name)
                            _free(name, 0, 0);
                        band->m_ownsName = false;
                    }
                    z->CloseKey(sub);
                }
                z->CloseKey(key);
            }
        }
    }
}

CPreLight_Object::CPreLight_Object()
{
    m_bandMax = 256;
}

CPreLight_Object::~CPreLight_Object()
{
}

/* nothing of its own: its filter planes' list torn down */
CPreLight_band::~CPreLight_band()
{
}

CScrollingTexture_Object::CScrollingTexture_Object()
{
    m_bandMax = 256;
}

CScrollingTexture_Object::~CScrollingTexture_Object()
{
}

/* the bands read whole from the archive's TextureScroll_Object key (SOCOM 1's) */
void CScrollingTexture_Object::Read(CSaveLoad *io)
{
    zar::CKey *key;
    zar::CZAR *z;

    if (io) {
        z = &io->m_zfile;
        if (z) {
            key = z->OpenKey("TextureScroll_Object");
            if (!key) {
                z->GetOpenKey();
                return;
            }
            resize(key->m_size / sizeof(CScrollingTexture_band));
            z->Fetch(key, data_, key->m_size, 0);
            z->CloseKey(key);
        }
    }
}

/* every word cleared, no strip looked for (SOCOM 1's, over SOCOM II's layout) */
void CVisData::Clear()
{
    m_strip = 0;
    m_colors = 0;
    m_head = 0;
    m_index = 0;
    m_indices = 0;
    m_cur = 0;
    m_first = 0;
    m_last = 0;
    m_34 = 0;
    m_numTris = 0;
    m_numVerts = 0;
    m_mesh = 0;
    m_14 = 0;
    m_id = 0xFFFF;
    m_mark = 0;
}

/* a view's select block opened at a vertex index, found by its triangles' indices (SOCOM 1's
 * SelectVertex, its block read off the view's mesh) */
CVisData *CVisData::SelectVertex(CVisual *vis, unsigned int view, unsigned int index)
{
    Clear();
    m_head = (_word128 *)vis->m_lodMesh[view];
    m_end = m_head + m_head->u16[0];
    m_cur = m_head + 1;
    m_index = index;
    return SearchVertex(false);
}

/* the same, counting the meshes' vertices */
CVisData *CVisData::SelectVertexStrips(CVisual *vis, unsigned int view, unsigned int index)
{
    Clear();
    m_head = (_word128 *)vis->m_lodMesh[view];
    m_end = m_head + m_head->u16[0];
    m_cur = m_head + 1;
    m_index = index;
    return SearchVertex(true);
}

/* the same, short of the block's last selects (as many as its head's eighth halfword says) */
CVisData *CVisData::SelectVertexHead(CVisual *vis, unsigned int view, unsigned int index)
{
    Clear();
    m_head = (_word128 *)vis->m_lodMesh[view];
    m_end = m_head + m_head->u16[0] - m_head->u16[7];
    m_cur = m_head + 1;
    m_index = index;
    return SearchVertex(false);
}

/* the same, over the selects its head's seventh and eighth halfwords bound. The first select is an
 * integer sum, the count shifted by hand ahead of the head (the target's `addu v0,v0,v1`): every
 * pointer form -- m_head + n + 1, &m_head[n + 1], n + m_head + 1, a local count, or the count scaled by
 * `* 16` or `* sizeof` -- adds the head first */
CVisData *CVisData::SelectVertexTail(CVisual *vis, unsigned int view, unsigned int index)
{
    Clear();
    m_head = (_word128 *)vis->m_lodMesh[view];
    m_end = m_head + m_head->u16[7];
    m_cur = (_word128 *)((m_head->u16[6] << 4) + (unsigned int)m_head) + 1;
    m_index = index;
    return SearchVertex(false);
}

/* from the select at to the mesh select holding the vertex index: the textures and colours on
 * the way taken, the mesh's range counted (its vertices, or its triangles' indices), its vertex
 * and index found; none past the block's end, the cursor cleared (SOCOM 1's took no flag) */
CVisData *CVisData::SearchVertex(bool strips)
{
    while (m_cur <= m_end) {
        switch (m_cur->u8[2]) {
        case 4:
            m_colors = (unsigned int *)m_cur->u32[1];
            m_color = (unsigned int *)m_cur->u32[1];
            break;
        case 2:
        case 10: {
            unsigned char *p = (unsigned char *)m_cur->u32[1];

            m_mesh = (MESH *)(p + 0x28);
            m_numVerts = m_mesh->m_numVerts;
            m_numTris = m_mesh->m_numTris;
            m_first = m_last;
            if (strips)
                m_last += m_numVerts;
            else
                m_last += m_numTris * 3;
            if (m_first >= m_index && m_index < m_last) {
                m_14 = p + 0x38;
                m_verts = m_vertex = (_word128 *)(p + 0x50);
                m_indices = (unsigned char *)(m_vertex + m_numVerts) + 8;
                m_20 = m_40 = m_indices + m_numTris * 4 + 8;
                if (m_index > m_first) {
                    for (unsigned int i = 0; i < m_index - m_first; i++) {
                        if (((unsigned int)m_indices & 3) == 3)
                            m_indices++;
                        m_indices++;
                    }
                }
                return this;
            }
            break;
        }
        case 3:
        case 5:
            m_texture = m_cur->u32[1];
            break;
        case 1:
            break;
        }
        m_cur++;
    }
    Clear();
    return 0;
}

/* every texture select from the block's first given the quadword: a select of type 6 (a name)
 * turned into one of type 5, the head's count of a select followed by a type 8 one less (SOCOM
 * 1's, from the cursor's head) */
void CVisData::ReplaceTextureData(_word128 *data)
{
    m_cur = m_head + 1;
    while (m_cur <= m_end) {
        if (m_cur->u8[2] == 5) {
            *m_cur = *data;
            if (m_cur[1].u8[2] == 8)
                m_cur->u16[0]--;
        } else if (m_cur->u8[2] == 6) {
            *m_cur = *data;
            m_cur->u8[2] = 5;
        }
        m_cur++;
    }
}

/* the strip of an id in a mesh's first level of detail (new in SOCOM II) */
CVisData *CVisData::SelectIndexedStrip(CMesh *mesh, unsigned int id)
{
    Clear();
    m_head = (_word128 *)func_003B9480(mesh, 0);
    m_end = m_head + m_head->u16[0];
    m_cur = m_head + 1;
    m_id = id;
    return SearchIndexedStrip();
}

/* n vertices on: the next mesh select's when the range runs out, else the index n further (every
 * fourth byte skipped) and its vertex and colour (SOCOM 1's) */
CVisData *CVisData::NextVertex(unsigned int n)
{
    m_index += n;
    if (m_index >= m_last) {
        m_cur++;
        return SearchVertex(false);
    }
    m_indices += n;
    if (((unsigned int)m_indices & 3) == 3)
        m_indices++;

    int i = *m_indices / 3;

    m_vertex = m_verts + i;
    m_color = m_colors + i;
    return this;
}

/* the next vertex of a mesh's own (new in SOCOM II) */
CVisData *CVisData::NextVertex()
{
    m_index++;
    if (m_index >= m_last) {
        m_cur++;
        return SearchVertex(true);
    }
    m_vertex++;
    m_color++;
    return this;
}

/* from the select at to the strip select of the id looked for: a strip select of another id ends
 * a run only at an index select (type 11); found, its counts and vertices taken and its index
 * list searched (new in SOCOM II) */
CVisData *CVisData::SearchIndexedStrip()
{
    bool found = false;

    while (m_cur <= m_end) {
        switch (m_cur->u8[2]) {
        case 10: {
            if (m_cur->u32[2] == m_id)
                found = true;

            _word128 *p = (_word128 *)m_cur->u32[1];

            if ((p->u16[4] & 3) == 2) {
                if (!found) {
                    _word128 *q = m_cur + 1;

                    /* both tests evaluated: the target ANDs their values */
                    while (!found & (q <= m_end)) {
                        if (q->u8[2] == 10) {
                            if (q->u32[2] == m_id)
                                found = true;
                        } else if (q->u8[2] == 11)
                            break;
                        q++;
                    }
                    if (!found)
                        m_cur = q;
                }
                if (found) {
                    m_numTris = p->u16[5];
                    m_numVerts = p->u16[7];
                    m_mark = m_cur;
                    m_verts = m_strip = p + 1;
                    return SearchIndices();
                }
            } else
                found = false;
            break;
        }
        }
        m_cur++;
    }
    Clear();
    return 0;
}

/* from the select at to the next index list (type 11), its indices from the start (new in SOCOM
 * II) */
CVisData *CVisData::SearchIndices()
{
    while (m_cur <= m_end) {
        switch (m_cur->u8[2]) {
        case 11: {
            unsigned char *p = (unsigned char *)m_cur->u32[1];

            m_index = 0;
            m_first = 0;
            m_last = m_numTris * 3;
            if (m_first >= m_index && m_index < m_last) {
                m_indices = p;
                m_indices += 8;
                return this;
            }
            break;
        }
        case 3:
        case 5:
            m_texture = m_cur->u32[1];
            break;
        case 1:
            break;
        }
        m_cur++;
    }
    Clear();
    return 0;
}

/* the next index of an indexed strip, and whether that moved on to the next strip (new in SOCOM
 * II) */
CVisData *CVisData::NextIndex(bool *next)
{
    *next = false;
    m_index++;
    if (m_index >= m_last) {
        m_cur++;
        *next = true;
        return SearchIndexedStrip();
    }
    m_indices++;
    if (((unsigned int)m_indices & 3) == 3)
        m_indices++;

    int i = *m_indices / 3;

    m_strip = m_verts + i;
    m_color = m_colors + i;
    return this;
}

/* the search taken up after another cursor's strip (new in SOCOM II) */
CVisData *CVisData::Resume(CVisData *from)
{
    m_cur = from->m_mark + 1;
    m_end = from->m_end;
    return this;
}

/* from the select at to the next strip select: its counts and vertices taken and the strip of an
 * id found in it; anything but a type 9 select on the way ends it (new in SOCOM II) */
CVisData *CVisData::SearchStrip(unsigned int id)
{
    while (m_cur <= m_end) {
        switch (m_cur->u8[2]) {
        case 9:
            break;
        case 10: {
            _word128 *p = (_word128 *)m_cur->u32[1];

            m_numTris = p->u16[5];
            m_numVerts = p->u16[7];
            m_id = m_cur->u32[2];
            m_verts = m_strip = p + 1;
            return FindStrip(id);
        }
        default:
            return 0;
        }
        m_cur++;
    }
    return 0;
}

/* the strip select at: its range counted and the vertex of an id found in it, none left at the
 * end (new in SOCOM II) */
CVisData *CVisData::FindStrip(unsigned int id)
{
    while (m_cur <= m_end) {
        switch (m_cur->u8[2]) {
        case 10:
            m_first = m_last;
            m_last += m_numVerts;
            for (unsigned int i = 0; i < m_numVerts; i++) {
                if (m_strip->u16[3] == id) {
                    m_cur++;
                    return this;
                }
                m_strip++;
            }
            m_strip = 0;
            m_cur++;
            return this;
        default:
            return 0;
        }
    }
    Clear();
    return 0;
}

}

using namespace zdb;

/* a packed 0xRRGGBB colour as three floats from 0 to 1, through a static 1/255 (SOCOM 1's, whose
 * static was set at its first call) */
CPnt3D ColConvTo3D(unsigned int col)
{
    static float scale = 1.0f / 255.0f;
    CPnt3D c;

    c.x = scale * (float)((col & 0xFF0000) >> 16);
    c.y = scale * (float)((col & 0xFF00) >> 8);
    c.z = scale * (float)(col & 0xFF);
    return c;
}
