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
