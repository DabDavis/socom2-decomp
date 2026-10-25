/* The zdb world's code around CWorld: the save/load driver (CSaveLoad), the asset classes it
 * fills, and the classes node_saveload.cpp, node_shadow.cpp, node_util.cpp and node_world.cpp
 * share. Kept apart from znode.h because units that include znode.h declare their own partial
 * CAssetList, CAssetMgr and DiIntersect (hud_main.cpp, fts_state.h, camera.h), and CSaveLoad holds
 * a zar::CZAR by value. The header's name is ours. */
#ifndef NODE_WORLD_H
#define NODE_WORLD_H

#include "znode.h"
#include "../zar/zar.h"

namespace zdb {

class CTexPalette;

class CDynTexList;

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

    CModel *m_cache_model;   // 0xc
};   // 0x10 bytes

/* the asset manager: its list of libraries (SOCOM 1's static, 0x45C3C0 here) and its libraries'
 * reference counting */
class CAssetMgr {
public:
    static CAssetList m_assets;

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

/* the recycle bin emptied (SOCOM 1's) */
void EmptyAssetRecycleBin();

/* a palette (SOCOM 1's DWARF: CTexPalette : PALETTE_PARAMS, 0x20 bytes) */
class CTexPalette {
public:
    CTexPalette(const char *name);
    ~CTexPalette();
    bool Read(CSaveLoad &io);

    unsigned int m_gsaddr;   // 0x0 (PALETTE_PARAMS)
    unsigned int m_params;   // 0x4 (PALETTE_PARAMS' bitfields)
    CAssetLib *m_AssetLib;   // 0x8 (AddPalette sets it)
    char *m_name;   // 0xc
    unsigned char _10[0x20 - 0x10];
};   // 0x20 bytes

/* a box-shaped DI: the box it stands for, borrowed (SOCOM 1's CDIBBox; 0x18 bytes, the size
 * SetDIBBox news) */
class CDIBBox : public CDI {
public:
    const CBBox *m_bbox;   // 0x14 (after CDI's points pointer at 0x10)
};   // 0x18 bytes

/* a polygon DI: its points the polygon's (SOCOM 1's CDIPoly; SOCOM II's is CDI's 0x14 bytes, the
 * size CDI::Create news) */
class CDIPoly : public CDI {
public:
    CDIPoly();
    virtual ~CDIPoly();
    virtual int GetIntersect(DiIntersect *di);
    virtual int GetAltitude(DiIntersect *di);
};   // 0x14 bytes

/* a box's eight corners (new in SOCOM II, int_main.cpp's; the name is ours) */
void GetBBoxCorners(CPnt3D *out, const CBBox *b);

/* what a render map projects (SOCOM 1's enum, by its mangled name; a byte under -enum min): a
 * shadow (0) makes its light a shadow */
enum _tag_render_map_type {
    RENDER_MAP_SHADOW = 0
};

/* a texture rendered from a node and projected back as a light map: a shadow (SOCOM 1's
 * CRenderMap : CLightMap, 0x2E0 bytes in both builds, the size AddShadow news). Its light is
 * itself (m_lightmap, set in the constructor and used for every light call after) */
class CRenderMap : public CLightMap {
public:
    CRenderMap(CNode *node, _tag_render_map_type type, const CTexture *tex);
    virtual ~CRenderMap();
    void Update();

    CNode *m_node;   // 0x180: the node casting it, when an instance (type 2)
    const CTexture *m_texture;   // 0x184
    CLightMap *m_lightmap;   // 0x188
    unsigned char _18c[0x2d0 - 0x18c];
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
    /* whether characters are hit, the old setting given back (inline: the queries save and
     * restore it -- read old, store new, store old; the name is ours) */
    bool SetIntersectCharacters(bool on)
    {
        bool was = m_IntersectCharacters;

        m_IntersectCharacters = on;
        return was;
    }

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

/* a texture buffer's dynamic textures (SOCOM 1's DWARF: CDynTexList, two vectors) */
class CDynTexList {
public:
    void Add(CTexHandle *tex, bool b);

    CTexList m_tex_list;   // 0x0
    CTexPalList m_pal_list;   // 0xc
};   // 0x18 bytes

/* a GS texture buffer: only what the world touches (its dynamic list at 0x28) */
class CGSTexBuffer {
public:
    unsigned char _0[0x18];
    int m_18;   // 0x18
    unsigned char _1c[0x28 - 0x1c];
    CDynTexList m_dyntex;   // 0x28
};

/* the texture manager: the buffers, and each render phase's buffer index (SOCOM 1's statics; SOCOM
 * 1's DWARF has m_texmanager a CTexManager whose first member is the buffers' vector, and its
 * methods are called on it) */
class CTexManager {
public:
    static std::vector<CGSTexBuffer *> m_texmanager;
    /* a new buffer's two sizes, first (SOCOM 1's 16-byte static after m_texmanager) */
    static float TexBufParams[4];
    static int TexPhase[55];
};

}

/* the texture manager's buffer made, by name and size, its index given back, and its buffer let go
 * of (vis code, not named yet: SOCOM 1's CTexManager::doAddBuffer and doDelBuffer, called on
 * m_texmanager); a buffer's dynamic list set, a library added, and its textures released (SOCOM 1's
 * CGSTexBuffer::SetDynTexList and Add; the third SOCOM II's) */
extern "C" int func_003566C0(void *texmanager, const char *name, float a, float b);
extern "C" void func_00356570(void *texmanager, zdb::CGSTexBuffer *buf);
extern "C" void func_00357820(zdb::CGSTexBuffer *buf, zdb::CDynTexList *list);
extern "C" void func_003569F0(zdb::CGSTexBuffer *buf, zdb::CAssetLib *lib, bool b);
extern "C" void func_00356BE0(zdb::CGSTexBuffer *buf, bool b);

namespace zdb {

/* a character set's textures in a texture buffer of their own, and the nodes drawn with them
 * (SOCOM 1's: its constructor takes the library, the first node and a dynamic list; SOCOM II's
 * news 0x18 bytes). The buffer is given back when it is deleted (inline: the world's purges
 * inline the destructor, the nodes' vector torn down after it); the names are ours */
class CTextureAssetCharacters {
public:
    CTextureAssetCharacters(CAssetLib *lib, CNode *node, CDynTexList *dyntex);
    ~CTextureAssetCharacters() { func_00356570(&CTexManager::m_texmanager, m_buffer); }
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

    unsigned char _0[0x70];
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
