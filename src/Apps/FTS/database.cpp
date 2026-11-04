/* database.cpp -- the game's database: its archive and the world's load from it (SOCOM 1's
 * Apps/FTS/database.cpp, between main.cpp and st_core.cpp; OVL1 0x1E8FE0-0x1EB03F, after ezctl.c's
 * dmaIOP). Its initialiser __sinit_database.cpp 0x3FD710 builds <iostream>'s pair and then
 * `archive`, a zar::CZAR with no name and no I/O, registering ~CZAR -- SOCOM 1's built its
 * MultiplayerMsgStrings table as well; the archive (data row 0x408AC0) is read by LoadWorld alone.
 * SOCOM 1's four, in its order: UnloadTheWorld 0x1E8FE0 (with FreeVector's copy 0x1E9120),
 * LoadWorld 0x1E91C0 (0x1090: CMenuState::Init and testLoad call it -- SOCOM 1's 0x96C; written
 * whole in round 35, with its template tail: the local list's pieces 0x1EA430-0x1EA6D0 and
 * reserve<CTickLight> 0x1EA8D0), testUnload 0x1EA6E0 and testLoad 0x1EA720. The rest of the
 * template tail (0x1EA250-0x1EABCF, the pointer lists' and CNodeVector's pieces every later unit
 * links to) and SOCOM II's command-line functions (0x1EABD0-0x1EB03F) stay with the drafts. Defined
 * in reverse of their image order (-inline deferred). */

#include <iostream>

#include "../../gamez/zar/zar.h"
#include "../../gamez/znode/node_world.h"

/* the database's archive (SOCOM 1's file static) */
static zar::CZAR archive(0, 0);

/* the world loaded (SOCOM 1's global, 0x408B78) */
extern zdb::CWorld *theWorld;

class CZSealBody;

/* the app camera (camera.h's: its teardown, its attach -- the second virtual -- and its camera) */
class CAppCamera {
public:
    CAppCamera(zdb::CCamera *camera);
    ~CAppCamera();
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    unsigned char _4[0xb4 - 0x4];
    zdb::CNode *m_camera;   // 0xb4 (camera.h's zdb::CCamera *; SOCOM 1: 0x34)
    unsigned char _b8[0x188 - 0xb8];
};   // 0x188 bytes (LoadWorld's new)

extern CAppCamera *appCamera;

/* the render pipe (zrndr_pipe.cpp's: only the flush) */
class CPipe {
public:
    void Flush();
};

extern CPipe thePipe;

void zBoneUnloadMap();

/* the tick lights' storage given up (an inline taller than the budget: inlined one level, its
 * FreeVector called; the name ours) */
static inline void FreeTickList() { FreeVector<std::vector<zdb::CTickLight> >(zdb::CLight::m_tick_list); }

extern "C" {
/* the frame's end (the video lane's, 0x3B05E0: the FIFO ended, the GS path synced) */
void func_003B05E0();
/* UnloadTheWorld below, called by its row's placeholder (the row stays func_ for its draft) */
void func_001E8FE0();
}

/* the database's name (zanim_menu.cpp's) */
const char *GetDatabase();

/* the reader archives (zrdr.h's: only the removal) */
class CRdrArchive {
public:
    static bool RemoveArchive(const char *name, const char *path);   /* 0x32C2B0 */
};

/* the weapons' world lists (zweapon.h's, SOCOM 1's) */
class CZWeapon {
public:
    static bool CreateWeaponList();
    static bool CreateMarkList();
    static bool CreateProjectileList(int count);
};

/* the mission (fts_mission.cpp's: only its open and close) */
class CMission {
public:
    bool Open(const char *name);
    void Close();
};

extern CMission theMission;

CZSealBody *ftsGetPlayer();

/* the sound system's listener (zsnd.h's CSnd: SOCOM 1's static, 0x48DB48 -- the camera's node) */
class CSnd {
public:
    static zdb::CNode *m_listener;
};

bool LoadWorld(const char *database);

/* the world loaded for a mission (SOCOM 1's): the UI's reader archive dropped, the world loaded,
 * the weapons' projectile, mark and weapon lists made, the mission opened (by name alone: SOCOM 1's
 * load flags from the debug switches are gone, the target leaves a2 unset), the camera attached to
 * the player and the listener set to the camera's node; loaded only with a player (OVL1 0x1EA720) */
bool testLoad()
{
    const char *database = GetDatabase();

    CRdrArchive::RemoveArchive("readerc.zar", "run/ui");
    if (!LoadWorld(database))
        return false;
    CZWeapon::CreateProjectileList(100);
    CZWeapon::CreateMarkList();
    CZWeapon::CreateWeaponList();
    theMission.Open(database);
    appCamera->AttachTo(ftsGetPlayer());
    CSnd::m_listener = appCamera->m_camera;
    return ftsGetPlayer() != 0;
}

/* the mission's world let go of (SOCOM 1's, the frame ended first: SOCOM II's): the mission closed,
 * the listener cleared, the world unloaded (OVL1 0x1EA6E0) */
bool testUnload()
{
    func_003B05E0();
    theMission.Close();
    CSnd::m_listener = 0;
    func_001E8FE0();
    return true;
}

#include <list>
#include <string.h>
#include <stdio.h>

void _free(void *p, const char *file, int line);   /* (zsys_public.h's) */

/* an entry of LoadWorld's local list: a file's name, its size and its block, freed with it (the
 * names ours) */
struct CLoadEntry {
    ~CLoadEntry()
    {
        if (m_block)
            _free(m_block, 0, 0);
    }
    const char *GetName() { return m_name; }
    unsigned int GetSize() { return m_size; }
    /* the block handed over, the entry forgetting it */
    void *Release()
    {
        void *block = m_block;

        m_block = 0;
        return block;
    }

    char m_name[0x10];   // 0x0
    unsigned int m_size;   // 0x10
    void *m_block;   // 0x14
};

/* LoadWorld's local list of loaded files: searched by name, and emptied (built and never filled
 * in SOCOM II: the names ours). The list a MEMBER, not a base: ~CLoadList, inlined one level at
 * LoadWorld's end, calls Clear and then the member's ~list with -1, as the target (a base's
 * destructor is called with 0); its three walks inline one level in LoadWorld (each tree too
 * tall), so every iterator piece is a call */
class CLoadList {
public:
    typedef std::list<CLoadEntry *> list;

    ~CLoadList() { Clear(); }
    /* every entry deleted, then the list cleared: the destructor's (OVL1 0x1EA4B0, out of line after
     * LoadWorld -- the end taken once, kept in s1, the iterator assigned, so in memory) */
    void Clear()
    {
        list::iterator it;
        list::iterator e = m_list.end();

        for (it = m_list.begin(); it != e; ++it)
            delete *it;
        m_list.clear();
    }
    /* the same, LoadWorld's own call: inlined there one level, its pieces called (the end read at
     * each pass, the iterator stepped by the postfix ++ -- 0x1EA650) */
    void Free()
    {
        list::iterator it;

        for (it = m_list.begin(); it != m_list.end(); it++)
            delete *it;
        m_list.clear();
    }
    /* the block of the entry of that name, taken over, and its size: the size through a pointer
     * to int, which LoadWorld's unsigned locals are passed to type-punned -- only so does MW keep
     * them on the stack (3784(sp), 3780(sp)) where an out-parameter of the same type is promoted
     * to a register; zeroed first, so the store fills the first call's delay slot */
    void *Find(const char *name, int *size)
    {
        *size = 0;

        list::iterator it;

        for (it = m_list.begin(); it != m_list.end(); it++) {
            CLoadEntry *entry = *it;

            if (!strcasecmp(name, entry->GetName())) {
                *size = entry->GetSize();
                return entry->Release();
            }
        }
        return 0;
    }

    list m_list;
};

class CRdrFile;
void zBoneLoadMap(const char *dir, CRdrFile *rdr);
/* a reader nothing sets (OVL1 bss, read by LoadWorld alone) */
extern "C" CRdrFile *data_00437D18;
void FilterAssetLibFolder(const char *name, char *folder);
void LoadNodeVisuals(zar::CZAR *zar, zdb::CNode *node);

/* a render phase's texture buffer (the texture manager's buffer at the phase's index) */
static inline zdb::CGSTexBuffer *PhaseBuffer(int phase, int i)
{
    return zdb::CTexManager::m_texmanager[zdb::CTexManager::TexPhase[phase][i]];
}

/* the world loaded from the database (SOCOM 1's, much grown; OVL1 0x1E91C0): the node universe made
 * or emptied, the world read, the app camera made on its camera, the map's bone motions loaded (new
 * in SOCOM II: run/mp/ or run/sp/ by the name's first letters), each library's textures given to
 * its render phase's buffers and the map textures picked out, each library's models given their
 * visuals from run/<folder>/<name>_mdl.zed (the world model from worl_mdl.zed), the world's lists
 * built, the tick lights reserved and the light and specular maps given to the shadow buffers.
 * Byte-exact (round 35): CNodeUniverse's inline constructor (znode.h) sets the lock in the new's
 * null test; both asset walks keep their ends in locals (s3, s7), the second's iterator ASSIGNED
 * (in memory); the arrays declared full_libname, root, folder, name (the later the lower); the
 * model loop's node declared ahead of the count, for the target's s1/s2/s3 (ii, n, node) */
bool LoadWorld(const char *database)
{
    if (!zdb::NodeUniverse)
        zdb::NodeUniverse = new zdb::CNodeUniverse;
    zdb::NodeUniverse->clear();
    zdb::NodeUniverse->m_locked = 0;

    zdb::CSaveLoad load;

    theWorld = load.Load(database);
    if (!theWorld) {
        zdb::NodeUniverse->clear();
        zdb::NodeUniverse->m_locked = 1;
        return false;
    }
    if (zdb::NodeUniverse)
        zdb::NodeUniverse->m_locked = 1;
    appCamera = new CAppCamera(theWorld->m_camera);

    char path[512] = "run/";

    if (database) {
        if (!strncmp(database, "mp", 2))
            strcat(path, "mp/");
        else if (!strncmp(database, "m", 1))
            strcat(path, "sp/");
        else if (!strncmp(database, "t", 1))
            strcat(path, "sp/");
        else if (!strncmp(database, "a", 1))
            strcat(path, "sp/");
        else
            database = 0;
        if (database) {
            strcat(path, database);
            strcat(path, "/");
            zBoneLoadMap(path, data_00437D18);
        }
    }
    FlushCache(0);

    zdb::CAssetList &al = zdb::CAssetMgr::m_assets;

    zdb::CAssetList::iterator e = zdb::CAssetMgr::m_assets.end();

    for (zdb::CAssetList::iterator i = zdb::CAssetMgr::m_assets.begin(); i != e; ++i) {
        zdb::CAssetLib *pAssetLib = *i;

        if (strstr(pAssetLib->m_name, "font")) {
            PhaseBuffer(0, 0)->Add(pAssetLib, true);
            PhaseBuffer(0, 0)->Hookup(pAssetLib);
            PhaseBuffer(14, 0)->Add(pAssetLib, true);
            PhaseBuffer(14, 0)->Hookup(pAssetLib);
            PhaseBuffer(12, 0)->Add(pAssetLib, true);
            PhaseBuffer(12, 1)->Add(pAssetLib, true);
        } else if (pAssetLib->m_autoload) {
            zdb::_RenderPhase phase = pAssetLib->m_renderphase;

            switch (phase) {
            case 0:
                PhaseBuffer(0, 0)->Add(pAssetLib, true);
                PhaseBuffer(0, 0)->Hookup(pAssetLib);
                break;
            case 1:
                PhaseBuffer(1, 0)->Add(pAssetLib, true);
                PhaseBuffer(1, 0)->Hookup(pAssetLib);
                break;
            case 12:
            case 13:
                PhaseBuffer(phase, 0)->Add(pAssetLib, true);
                PhaseBuffer(phase, 1)->Add(pAssetLib, true);
                PhaseBuffer(phase, 0)->Hookup(pAssetLib);
                PhaseBuffer(phase, 1)->Hookup(pAssetLib);
                break;
            case 14:
                PhaseBuffer(14, 0)->Add(pAssetLib, true);
                PhaseBuffer(14, 0)->Hookup(pAssetLib);
                break;
            case 11:
                PhaseBuffer(11, 0)->Add(pAssetLib, true);
                PhaseBuffer(11, 1)->Add(pAssetLib, true);
                PhaseBuffer(11, 0)->Hookup(pAssetLib);
                PhaseBuffer(11, 1)->Hookup(pAssetLib);
                break;
            case 2:
                PhaseBuffer(2, 0)->Add(pAssetLib, true);
                PhaseBuffer(2, 1)->Add(pAssetLib, true);
                PhaseBuffer(2, 0)->Hookup(pAssetLib);
                PhaseBuffer(2, 1)->Hookup(pAssetLib);
                break;
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 16:
                PhaseBuffer(16, 0)->Add(pAssetLib, true);
                PhaseBuffer(16, 0)->m_checkForOverflow = false;
                PhaseBuffer(16, 1)->Add(pAssetLib, true);
                PhaseBuffer(16, 1)->m_checkForOverflow = false;
                PhaseBuffer(16, 2)->Add(pAssetLib, true);
                PhaseBuffer(16, 2)->m_checkForOverflow = false;
                PhaseBuffer(16, 0)->Hookup(pAssetLib);
                PhaseBuffer(16, 1)->Hookup(pAssetLib);
                PhaseBuffer(16, 2)->Hookup(pAssetLib);
                break;
            case 17:
                PhaseBuffer(17, 0)->Add(pAssetLib, true);
                PhaseBuffer(17, 0)->m_checkForOverflow = false;
                PhaseBuffer(17, 1)->Add(pAssetLib, true);
                PhaseBuffer(17, 1)->m_checkForOverflow = false;
                PhaseBuffer(17, 2)->Add(pAssetLib, true);
                PhaseBuffer(17, 2)->m_checkForOverflow = false;
                PhaseBuffer(17, 0)->Hookup(pAssetLib);
                PhaseBuffer(17, 1)->Hookup(pAssetLib);
                PhaseBuffer(17, 2)->Hookup(pAssetLib);
                break;
            }
            for (zdb::CTexList::iterator it = pAssetLib->m_textures.begin(); it != (*i)->m_textures.end(); ++it) {
                zdb::CTexHandle *tex = *it;

                if (tex && strstr(tex->m_name, "map")) {
                    if (strstr(tex->m_name, "light_map"))
                        zdb::CVisual::m_lightMapTexture = tex;
                    if (strstr(tex->m_name, "specular_map"))
                        zdb::CVisual::m_specMapTexture = tex;
                    if (strstr(tex->m_name, "env_front_map"))
                        zdb::CVisual::m_envFrontMapTexture = tex;
                    if (strstr(tex->m_name, "env_back_map"))
                        zdb::CVisual::m_envBackMapTexture = tex;
                }
            }
        }
    }

    bool result = true;
    zdb::CAssetList::iterator end = al.end();
    CLoadList zars;
    zdb::CAssetList::iterator it;

    for (it = al.begin(); it != end; ++it) {
        zdb::CAssetLib *pAssetLib = *it;

        if (pAssetLib->m_autoload) {
            char full_libname[1024];
            char root[512];
            char folder[256];
            char name[32];

            strcpy(root, pAssetLib->RootName());
            if (strlen(root) > 4)
                root[4] = 0;
            FilterAssetLibFolder(pAssetLib->m_name, folder);

            zdb::CNode *worldmodel = 0;

            sprintf(name, "%s_mdl.zed", root);
            sprintf(full_libname, "run/%s/%s", folder, name);

            unsigned int size;
            void *block = zars.Find(name, (int *)&size);

            if (block) {
                archive.m_databuffer = block;
                archive.m_24 = 1;
                archive.m_data_size = size;
            }
            result = archive.Open(full_libname, -1, 1, 16, 0);
            zdb::CNode *model;
            unsigned int n = pAssetLib->m_models.size();
            zdb::CModelVector &models = pAssetLib->m_models;

            for (unsigned int ii = 0; ii < n; ii++) {
                model = (zdb::CNode *)models[ii];

                if (!strcmp(model->m_name, "worldmodel"))
                    worldmodel = model;
                else if (result)
                    LoadNodeVisuals(&archive, model);
            }
            if (result)
                pAssetLib->m_model_buffer = archive.ReleaseDataBuffer();
            archive.Close(true);
            if (worldmodel) {
                sprintf(full_libname, "run/%s/worl_mdl.zed", folder);

                unsigned int wsize;
                void *wblock = zars.Find("worl_mdl.zed", (int *)&wsize);

                if (wblock) {
                    archive.m_databuffer = wblock;
                    archive.m_24 = 1;
                    archive.m_data_size = wsize;
                }
                result = archive.Open(full_libname, -1, 1, 16, 0);
                if (result)
                    LoadNodeVisuals(&archive, worldmodel);
                pAssetLib->m_socom2_38 = archive.ReleaseDataBuffer();
                archive.Close(true);
            }
        }
    }
    zars.Free();
    archive.Close();
    archive.SetFilename(0);
    theWorld->DismemberWorldModel();
    theWorld->GenerateLandmarkList();
    theWorld->InheritRegionMasks(0, 0);
    theWorld->GenerateLightList();
    theWorld->UpdateShadowCasters();
    zdb::CLight::m_tick_list.reserve(5);
    if (zdb::CVisual::m_lightMapTexture) {
        zdb::CGSTexBuffer *buf = PhaseBuffer(2, 0);

        buf->m_dyntex_list.Add(zdb::CVisual::m_lightMapTexture, true);
        buf->m_chainp = 0;
        buf = PhaseBuffer(2, 1);
        buf->m_dyntex_list.Add(zdb::CVisual::m_lightMapTexture, true);
        buf->m_chainp = 0;
        zdb::CWorld::m_world->AddTextureAssetCharacterAll(zdb::CVisual::m_lightMapTexture);
        buf = PhaseBuffer(2, 0);
        buf->m_dyntex_list.Add(zdb::CVisual::m_specMapTexture, true);
        buf->m_chainp = 0;
        buf = PhaseBuffer(2, 1);
        buf->m_dyntex_list.Add(zdb::CVisual::m_specMapTexture, true);
        buf->m_chainp = 0;
        zdb::CWorld::m_world->AddTextureAssetCharacterAll(zdb::CVisual::m_specMapTexture);
    }
    thePipe.Flush();
    return result;
}

/* the world torn down (SOCOM 1's): the app camera deleted, the world's landmark and light lists
 * released, the map's motions unloaded (SOCOM II's), the world deleted, the recycle bin emptied,
 * the asset list cleared, the map textures forgotten, the tick lights' storage given up (FreeVector
 * called out of line through FreeTickList, inlined one level: its copy 0x1E9120 follows, byte-exact,
 * with the deleter's clear and destructor 0x1EAA70/0x1EAA80), the node universe deleted and the
 * pipe flushed.
 * OPEN, 0x130 against 0x138 (THE LEVEL: byte-exact at -inline level=6, every reference resolved):
 * the target inlines `delete NodeUniverse` whole -- five this-tests (~CNodeUniverse, ~CNodeVector,
 * ~vector, ~__vector_imp, the pod) and the block's delete -- where include/vector's pointer vector
 * destructor is four high with the pod's deallocate, so ~CNodeUniverse is six and level 5 calls
 * ~CNodeVector (its copy 0x1EA340, byte-exact here). Its row stays func_001E8FE0 so that m2c's draft
 * keeps matching it (a hand-written definition of the row's name would supersede the draft) */
void UnloadTheWorld()
{
    if (theWorld) {
        if (appCamera) {
            delete appCamera;
            appCamera = 0;
        }
        theWorld->ReleaseLandmarkList();
        theWorld->ReleaseLightList();
        zBoneUnloadMap();
        delete theWorld;
        theWorld = 0;
        zdb::EmptyAssetRecycleBin();
        zdb::CAssetMgr::m_assets.Clear();
        zdb::CVisual::m_lightMapTexture = 0;
        zdb::CVisual::m_specMapTexture = 0;
        zdb::CVisual::m_envFrontMapTexture = 0;
        zdb::CVisual::m_envBackMapTexture = 0;
        FreeTickList();
        if (zdb::NodeUniverse) {
            delete zdb::NodeUniverse;
            zdb::NodeUniverse = 0;
        }
        thePipe.Flush();
    }
}
