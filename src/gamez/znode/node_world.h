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

/* the libraries loaded, a list of them (SOCOM 1's DWARF: CAssetList : list, a cached model at
 * 0xc; only the lookups the world code calls are declared) */
class CAssetList {
public:
    CTexHandle *GetTexHandle(const char *name);
    CModel *GetModel(const char *name);
};

/* the asset manager: its list of libraries (SOCOM 1's static, 0x45C3C0 here) */
class CAssetMgr {
public:
    static CAssetList m_assets;
};

/* a palette (SOCOM 1's DWARF: CTexPalette : PALETTE_PARAMS, 0x20 bytes) */
class CTexPalette {
public:
    CTexPalette(const char *name);
    bool Read(CSaveLoad &io);

    unsigned char _0[0x20];
};   // 0x20 bytes

/* a box-shaped DI: the box it stands for, borrowed (SOCOM 1's CDIBBox; 0x18 bytes, the size
 * SetDIBBox news) */
class CDIBBox : public CDI {
public:
    unsigned char _10[4];   // 0x10
    const CBBox *m_bbox;   // 0x14
};   // 0x18 bytes

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
};

/* a ray's intersection set (SOCOM 1's DWARF; SOCOM II adds the mask at 0x44, so its hits are 4
 * bytes further on -- camera.h has the same; 0x4C bytes, the size CWorld's constructor news) */
class DiIntersect {
public:
    DiIntersect(int n);
    ~DiIntersect();
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
    unsigned int m_Unused : 13;
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

/* the texture manager: the buffers, and each render phase's buffer index (SOCOM 1's statics) */
class CTexManager {
public:
    static std::vector<CGSTexBuffer *> m_texmanager;
    static int TexPhase[55];
};

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
