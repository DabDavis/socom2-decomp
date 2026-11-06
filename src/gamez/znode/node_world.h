/* The zdb world's code around CWorld: the save/load driver (CSaveLoad), the asset classes it
 * fills, and the classes node_saveload.cpp, node_shadow.cpp, node_util.cpp and node_world.cpp
 * share. Kept apart from znode.h because units that include znode.h declare their own partial
 * CAssetList, CAssetMgr and DiIntersect (hud_main.cpp, fts_state.h, camera.h), and CSaveLoad holds
 * a zar::CZAR by value. The header's name is ours. */
#ifndef NODE_WORLD_H
#define NODE_WORLD_H

#include <algorithm>

#include "znode.h"
#include "../zar/zar.h"
/* FreeVector, for the texture classes' inlines below: included, not declared ahead -- a function
 * template declared before its definition is no longer inlined one level where it is called after
 * the definition (ztex_manager.cpp's four FreeVector users all missed so) */
#include "../zsys/zsys.h"

namespace zdb {

class CTexPalette;

/* both lists' storage given up, each through FreeVector on its base vector (the temporaries
 * three this-tests deep: SOCOM II instantiates FreeVector on the base std::vector) */
inline void CDynTexList::Free()
{
    FreeVector<std::vector<CTexHandle *> >(m_tex_list);
    FreeVector<std::vector<CTexPalette *> >(m_pal_list);
}

/* the libraries loaded, a list of them (SOCOM 1's DWARF: CAssetList : list, the model last found
 * cached at 0xc). node_assetlib.cpp's */
class CAssetList : public std::list<CAssetLib *> {
public:
    /* the walks run on the pointer list's own base iterator, its element cast out by value (SOCOM
     * 1's code calls the base's generic_iterator constructors): a fresh load after each call, where
     * the wrapper's reference keeps the element's address in a saved register; the end taken
     * first, the walker after it (FindTextureLib) */
    typedef std::__list_deleter<const void *, std::allocator<const void *> > list_base;
    typedef list_base::iterator lib_iterator;
    /* a library taken off through the base's remove, a level shorter than the wrapper's (the
     * temporaries of ReleaseLibRef's remove and push_back are stacked in the target's order only
     * so) */
    typedef std::__list_imp<const void *, std::allocator<const void *> > list_imp;

    CAssetList();
    ~CAssetList() { Clear(); }
    /* every library deleted, the list emptied (SOCOM 1's) */
    void Clear();
    CModel *GetModel(const char *name);
    CTexHandle *GetTexHandle(const char *name);
    CAssetLib *FindLib(_RenderPhase phase);
    CAssetLib *FindLib(const char *name);
    /* new in SOCOM II, the names ours: the library holding a texture, or a model, of that name
     * (SOCOM 1's AddTexture and AddModel walked the list themselves), and a texture of a phase
     * that the given texture's mip level is */
    CAssetLib *FindTextureLib(const char *name);
    CAssetLib *FindModelLib(const char *name);
    CTexHandle *FindMipLevel(_RenderPhase phase, CTexHandle *tex);

    /* the model cache emptied (inline, out of line at 0x311AE0 in CModel::Create; the name ours) */
    void ClearCache() { m_cache_model = 0; }

    CModel *m_cache_model;   // 0xc
};   // 0x10 bytes

/* the asset manager: its list of libraries (SOCOM 1's static, 0x45C3C0 here) and its libraries'
 * reference counting */
class CAssetMgr {
public:
    static CAssetList m_assets;
    /* the list (inline, out of line at 0x311AF0 in CModel::Create -- CWorld::GetAssetList's three
     * words under another name; the name ours) */
    static CAssetList *GetAssets() { return &m_assets; }

    /* a library let go of: at no holders left it moves to the recycle bin (SOCOM 1's) */
    static void ReleaseLibRef(CAssetLib *lib);
    /* a library by name, held once more: the loaded one, the recycled one brought back, or one
     * loaded fresh (SOCOM 1's) */
    static CAssetLib *GetLoadedLibRef(const char *name);
    /* new in SOCOM II, the names ours: the libraries of a name list each held and listed, those
     * missing loaded; a library made and listed; a texture list's mip levels added to it */
    static void GetLoadedLibRefs(const char **names, int n, CAssetList *list);
    static CAssetLib *NewLib(const char *name, int unused);
    static void AddMipLevels(CDynTexList *list, _RenderPhase phase);
};

/* a model taken off the library's list (std::remove, the tail erased), the asset manager's model
 * cache emptied and the model no longer the library's -- none for no model (SOCOM 1's Create and
 * ~CModel wrote the first two out; SOCOM II's inline, the online lane's round 28: CModel::Create,
 * node_io.cpp, inlines its body alone -- the tree is taller than the level there -- and calls its
 * pieces out of line, 0x311AD0-0x311C30; the target's ~CModel calls its whole copy, 0x318AE0; the
 * name ours) */
inline void CAssetLib::RemoveModel(CModel *model)
{
    if (model) {
        CModelVector::iterator e = m_models.end();
        CModelVector::iterator it = std::remove(m_models.begin(), e, model);

        m_models.erase(it, e);
        CAssetMgr::GetAssets()->ClearCache();
        model->SetAssetLib(0);
    }
}

/* the library's own model of a name taken off (the world's round 29, ~CModel's: the tree a level
 * taller than RemoveModel's, so only this body is inlined -- RemoveModel and GetModel are called,
 * their out-of-line copies 0x318AE0 and 0x318C30 following ~CModel last call first; written out
 * in ~CModel, MW inlines RemoveModel's body there. The name ours) */
inline void CAssetLib::RemoveModel(const char *name)
{
    RemoveModel(GetModel(name));
}

/* the recycle bin emptied (SOCOM 1's) */
void EmptyAssetRecycleBin();

/* a palette (SOCOM 1's DWARF: CTexPalette : PALETTE_PARAMS, 0x20 bytes) */
class CTexPalette {
public:
    CTexPalette(const char *name);
    ~CTexPalette();
    bool Read(CSaveLoad &io);

    /* the bitfield setters (ours, inline: a constant through them is masked at run time, as the
     * constructor's byte read-modify-writes are) */
    void SetDynamic(bool b) { m_dynamic = b; }
    void SetUnused(unsigned int n) { m_unused = n; }

    /* the next palette's id (SOCOM 1's static) */
    static unsigned int m_palId;

    unsigned int m_gsaddr;   // 0x0 (PALETTE_PARAMS)
    unsigned int m_size : 16;   // 0x4 (PALETTE_PARAMS' bitfields)
    unsigned int m_format : 8;   // 0x4
    unsigned int m_combo_pal : 1;   // 0x4
    unsigned int m_dynamic : 1;   // 0x4
    unsigned int m_unused : 6;   // 0x4
    CAssetLib *m_AssetLib;   // 0x8 (AddPalette sets it)
    char *m_name;   // 0xc
    unsigned int m_id;   // 0x10
    void *m_buffer;   // 0x14
    unsigned int m_slots;   // 0x18
    void *m_loadimage;   // 0x1c (SOCOM 1's CLoadImage *)
};   // 0x20 bytes

/* a box-shaped DI: the box it stands for, borrowed (SOCOM 1's CDIBBox; 0x18 bytes, the size
 * SetDIBBox news) */
class CDIBBox : public CDI {
public:
    CDIBBox();
    virtual ~CDIBBox();
    virtual bool GetIntersect(DiIntersect *di);
    virtual int GetAltitude(DiIntersect *di);
    /* the unit box's six faces, built once behind a guard byte (SOCOM 1's static; OVL1 0x2DC7A0, named by the
     * engine core's round 23 -- declared by the integrator so camera.cpp's lane can write it under its name) */
    static void SetCDIBBox();

    const CBBox *m_bbox;   // 0x14 (after CDI's points pointer at 0x10)
};   // 0x18 bytes

/* a polygon DI: its points the polygon's (SOCOM 1's CDIPoly; SOCOM II's is CDI's 0x14 bytes, the
 * size CDI::Create news) */
class CDIPoly : public CDI {
public:
    CDIPoly();
    virtual ~CDIPoly();
    virtual bool GetIntersect(DiIntersect *di);
    virtual int GetAltitude(DiIntersect *di);
};   // 0x14 bytes

/* a box's eight corners (new in SOCOM II, int_main.cpp's; the name is ours) */
void GetBBoxCorners(CPnt3D *out, const CBBox *b);

/* what a render map projects (SOCOM 1's enum, by its mangled name; a byte under -enum min): a
 * shadow (0) makes its light a shadow */
enum _tag_render_map_type {
    RENDER_MAP_SHADOW = 0,
    RENDER_MAP_1 = 1,
    RENDER_MAP_2 = 2
};

/* a texture rendered from a node and projected back as a light map: a shadow (SOCOM 1's
 * CRenderMap : CLightMap, 0x2E0 bytes in both builds, the size AddShadow news). Its light is
 * itself (m_lightmap, set in the constructor and used for every light call after) */
class CRenderMap : public CLightMap {
public:
    CRenderMap(CNode *node, _tag_render_map_type type, const CTexture *tex);
    virtual ~CRenderMap();
    /* SOCOM 1's Update() grown a bool (0x31A180): whether the texture differs from the last map's
     * RenderWorld updated -- false only clears the packet's first word (the name ours) */
    void Update(bool newTexture);

    CNode *m_node;   // 0x180: the node casting it, when an instance (type 2)
    const CTexture *m_texture;   // 0x184
    CLightMap *m_lightmap;   // 0x188
    /* the constructor's (the names ours): the texture's usable size and the bias its inverse
     * makes; three matrices, unit at first, the second and third given their translations; the VIF
     * codes and the GIF tag of the render; the map's type; the DMA reference to the packet; the
     * axis perpendicular to the shadow's direction */
    float m_size;   // 0x18c
    float m_bias;   // 0x190
    unsigned char _194[0x1a0 - 0x194];
    CMatrix m_mtx0;   // 0x1a0
    CMatrix m_mtx1;   // 0x1e0
    CMatrix m_mtx2;   // 0x220
    unsigned int m_vif0[4];   // 0x260
    unsigned char _270[0x280 - 0x270];
    unsigned int m_vif1[4];   // 0x280
    unsigned long m_giftag[2];   // 0x290
    _tag_render_map_type m_type;   // 0x2a0
    unsigned char _2a1[0x2b0 - 0x2a1];
    unsigned int m_dmaRef[4];   // 0x2b0
    float m_axis[4];   // 0x2c0
    void *m_packet;   // 0x2d0: 208 bytes, the GS packet it renders with (the names ours)
    unsigned char _2d4[0x2e0 - 0x2d4];
};   // 0x2e0 bytes

/* a hit along a ray (SOCOM 1's DWARF) */
struct IntersectStruct {
    CPnt3D m_pos;   // 0x0
    CDI *m_poly;   // 0xc
    CPnt3D m_norm;   // 0x10
    CNode *m_node;   // 0x1c

    /* the hit polygon's material, or the default one (SOCOM 1's) */
    int GetMaterial();
    /* the hit polygon's edges through the hit, in the world's frame (SOCOM 1's) */
    bool GetIntersectedEdgePoints(CPnt3D *a, CPnt3D *b);
    bool GetIntersectedEdgePointsY(CPnt3D *out);
    /* the hit polygon's flag at bit 27 (new in SOCOM II; the name ours) */
    bool GetPolyFlag27();
    /* the hit polygon's first word, or every bit for none (OVL1 0x2DC1B0; the SEAL's ground
     * probes pass it to DiIntersect::SetMask) */
    unsigned int GetRegion();
};

/* a ray's intersection set (SOCOM 1's DWARF; SOCOM II adds the mask at 0x44, so its hits are 4
 * bytes further on -- camera.h has the same; 0x4C bytes, the size CWorld's constructor news) */
class DiIntersect {
public:
    DiIntersect(int n);
    ~DiIntersect();
    /* the hit mask, its low bit forced on, or every bit for none (new in SOCOM II; the name ours) */
    void SetMask(unsigned int mask);
    IntersectStruct *GetHighestAltitude(const CPnt3D *p, float alt);
    IntersectStruct *GetNearestIntersect();
    /* the hits' indices sorted by their squared distance from the tail (SOCOM 1's) */
    void SortNearToFar(int *idx, float *dist);
    /* the model-space tip and tail into VU0's vf1 and vf2 for the polygon queries (SOCOM 1's, which
     * wrote them to VU0's data memory; int_shapes.cpp's, 0x2DD210) */
    void LoadTipAndTail();
    /* whether characters are hit, the old setting given back (inline: the queries save and
     * restore it -- read old, store new, store old; the name is ours) */
    bool SetIntersectCharacters(bool on)
    {
        bool was = m_IntersectCharacters;

        m_IntersectCharacters = on;
        return was;
    }
    /* the same, nothing given back (inline: the visibility pool's pre-tick clears it on each ray it
     * registers, out of line at OVL2 0x60BF10 behind that tick's one-level cut; the kit lane's,
     * round 25 -- the name ours) */
    void SetCharacterHits(bool on) { m_IntersectCharacters = on; }

    unsigned int m_Type : 16;   // 0x0
    unsigned int m_IntersectCharacters : 1;
    unsigned int m_AltitudeCharacters : 1;
    unsigned int m_ProximityCharacters : 1;
    unsigned int m_socom2_bit19 : 1;   // SOCOM II's: the constructor clears it (the name ours)
    unsigned int m_Unused : 12;
    CPnt3D m_Tail;   // 0x4
    CPnt3D m_Tip;   // 0x10
    CPnt3D m_MTail;   // 0x1c
    CPnt3D m_MTip;   // 0x28
    int m_Cnt;   // 0x34
    int m_BufCnt;   // 0x38
    CNode *m_Node;   // 0x3c
    CNode *m_TreeDoneNode;   // 0x40
    unsigned int m_mask;   // 0x44: SOCOM II's (0x2DC660 sets it, the low bit forced on; the name ours)
    IntersectStruct *m_Intersects;   // 0x48 (SOCOM 1: 0x44)
};   // 0x4c bytes

/* the decal pools (SOCOM 1's: the visual effects' units) */
class CDecalPool {
public:
    void RecycleTick();
};
class CMeshDecalPool {
public:
    void RecycleTick();
};
class CDecal {
public:
    static CDecalPool m_tempPool;
    static CDecalPool m_permPool;
};
class CMeshDecal {
public:
    static CMeshDecalPool m_tempPool;
};

/* CDynTexList is znode.h's (a cell holds one) */

/* a GS texture buffer (SOCOM 1's DWARF, 0x40 bytes): a stretch of GS memory its dynamic textures
 * and palettes are placed in one after another, and the DMA chain that loads them, built in its
 * packet buffer (ztex_manager.cpp's) */
class CGSTexBuffer {
public:
    CGSTexBuffer();
    ~CGSTexBuffer();
    /* the name copied (ours, inline: doAddBuffer frees the old one, then copies, as CTexHandle's) */
    void SetName(const char *name)
    {
        if (m_name)
            _free(m_name, 0, 0);
        m_name = 0;
        m_name = name ? _strdup(name, 0, 0) : 0;
    }
    unsigned int Setup(float start, float end);
    unsigned int Setup(unsigned int start, unsigned int end);
    void Add(CTexList *list, bool b);
    void Add(CAssetLib *lib, bool b);
    void SetDynTexList(CDynTexList *list);
    u_long128 *Load(bool now);
    /* the chain built into p (SOCOM 1's took a count as well; SOCOM II's Load passes none and its
     * MakeChain reads none) */
    u_long128 *MakeChain(u_long128 *p);
    void RelocateAll();
    void Hookup(CAssetLib *lib);
    void UpdateTextureSelects();
    /* the packet buffer emptied and given back, the chain dropped; emptied and made room in for n
     * quadwords (SOCOM II's; the names ours) */
    void WipePktBuf();
    void ReservePktBuf(unsigned int n);
    /* the chain dropped, to be rebuilt at the next load (SOCOM II's, out of line: SOCOM 1 stored
     * the zero in place; the name ours) */
    void DropChain();
    /* SOCOM II's inlines, the names ours. The dynamic lists emptied and the chain dropped (the
     * chain's zero stored in place, as SOCOM 1's Flush does); the packet buffer emptied and given
     * up, the chain dropped (WipePktBuf's body: zrndr_pipe.cpp's copy 0x33B960 is byte-identical to
     * 0x356B30); and all three -- what CPipe::Flush (for the two alpha buffers, each tested) and
     * CGameMenu::Clear (for buffer 0) inline ONE level, calling the three out of line: its tree
     * (Free, CDynTexList::Free, FreeVector's six) is taller than the level, while SOCOM 1's Flush
     * inlined it whole. zrndr_pipe.cpp's copies are the ones the linker kept (0x33BA90, 0x33BAA0,
     * 0x33B960, after Flush) */
    void ClearDynTex()
    {
        m_dyntex_list.Clear();
        m_chainp = 0;
    }
    void FreePktBuf()
    {
        m_pktbuf.clear();
        FreeVector(m_pktbuf);
        m_chainp = 0;
    }
    void Free()
    {
        ClearDynTex();
        m_dyntex_list.Free();
        FreePktBuf();
    }

    char *m_name;   // 0x0
    unsigned char m_checkForOverflow;   // 0x4
    unsigned int m_startAddress;   // 0x8
    unsigned int m_nextAddress;   // 0xc
    unsigned int m_endAddress;   // 0x10
    CAssetLib *m_assetLib;   // 0x14
    u_long128 *m_chainp;   // 0x18
    std::vector<u_long128> m_pktbuf;   // 0x1c
    CDynTexList m_dyntex_list;   // 0x28
};   // 0x40 bytes

/* the texture manager: the buffers (SOCOM 1's DWARF, 0x1c bytes; m_texmanager the one there is),
 * the render maps' and shadow maps' sizes, a new buffer's two default fractions and each render
 * phase's buffer indices (SOCOM 1's statics: SOCOM 1 had 17 phases, SOCOM II has 19, three
 * indices each -- DestroyBuffers zeroes 19 x 12 bytes) */
class CTexManager {
public:
    CTexManager();
    static void ClearPktBufs(CTexManager &tm, bool all);
    static bool DestroyBuffers(CTexManager &tm, bool all);
    void doSetupBuffers();
    static void WipePktBuf();
    void doDelBuffer(CGSTexBuffer *buf);
    int doAddBuffer(const char *name, float start, float end);
    /* the buffer by index (ours, inline) */
    CGSTexBuffer *operator[](unsigned int i) { return m_buffer[i]; }

    std::vector<CGSTexBuffer *> m_buffer;   // 0x0
    unsigned int m_renderMapNum;   // 0xc
    unsigned int m_renderMapNumBytes;   // 0x10
    unsigned int m_shadowMapSize;   // 0x14
    unsigned int m_reflectionMapSize;   // 0x18

    static CTexManager m_texmanager;
    /* a new buffer's two sizes, first (SOCOM 1's 16-byte static after m_texmanager) */
    static float TexBufParams[4];
    static int TexPhase[19][3];
};   // 0x1c bytes

}

namespace zdb {

/* a character set's textures in a texture buffer of their own, and the nodes drawn with them
 * (SOCOM 1's: its constructor takes the library, the first node and a dynamic list; SOCOM II's
 * news 0x18 bytes). The buffer is given back when it is deleted (inline: the world's purges
 * inline the destructor, the nodes' vector torn down after it); the names are ours */
class CTextureAssetCharacters {
public:
    CTextureAssetCharacters(CAssetLib *lib, CNode *node, CDynTexList *dyntex);
    ~CTextureAssetCharacters() { CTexManager::m_texmanager.doDelBuffer(m_buffer); }
    /* the visible nodes counted, each node's in-view bit set (SOCOM 1's) */
    int TestFOV(CCamera *cam);

    int m_visible;   // 0x0: the nodes in view, as TestFOV last counted them
    CAssetLib *m_lib;   // 0x4
    CNodeVector m_nodes;   // 0x8
    CGSTexBuffer *m_buffer;   // 0x14
};   // 0x18 bytes

/* a shadow caster: what the world keeps for each node casting a shadow (new in SOCOM II, 0x70
 * bytes, the size AddShadowCaster news; the name is ours) */
class CShadowCaster {
public:
    CShadowCaster(CNode *node);
    void Update();

    CMatrix m_matrix;   // 0x0: the unit matrix at first
    CPnt3D m_size;   // 0x40: the node's box's size
    CNode *m_node;   // 0x4c: the node casting the shadow (the constructor's argument)
    CTexture *m_texture;   // 0x50: its first visual's first dynamic texture (UpdateShadowCasters)
    CBBox m_bbox;   // 0x54: the box a node's sphere must meet to take its projection (the pipe's
                    //   BuildProjectedMapList)
    unsigned char _6c[0x70 - 0x6c];
};   // 0x70 bytes

/* the save/load driver: the world it builds, the archive it reads and the file's name (SOCOM 1's
 * DWARF; SOCOM II's archive is 0x14 bytes longer, so the name sits at 0xbc) */
class CSaveLoad {
public:
    CSaveLoad();
    ~CSaveLoad();
    CWorld *Load(const char *name);
    bool LoadAssetLib(CWorld *world, CAssetLib *lib, int flags);
    bool LoadAssetLib_PS2(CWorld *world, CAssetLib *lib, int flags);
    bool LoadTextures_PS2(CAssetLib *lib);
    bool LoadPalettes_PS2(CAssetLib *lib);
    /* new in SOCOM II: SOCOM 1 read the models in LoadAssetLib_PS2 itself (the name is ours) */
    bool LoadModels_PS2(CAssetLib *lib);

    CWorld *m_world;   // 0x0
    zar::CZAR m_zfile;   // 0x4
    char m_zed_filename[1024];   // 0xbc (SOCOM 1: 0xa8)
    int m_version;   // 0x4bc
};   // 0x4c0 bytes

}

#endif
