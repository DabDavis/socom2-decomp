/* database.cpp -- the game's database: its archive and the world's load from it (SOCOM 1's
 * Apps/FTS/database.cpp, between main.cpp and st_core.cpp; OVL1 0x1E8FE0-0x1EB03F, after ezctl.c's
 * dmaIOP). Its initialiser __sinit_database.cpp 0x3FD710 builds <iostream>'s pair and then
 * `archive`, a zar::CZAR with no name and no I/O, registering ~CZAR -- SOCOM 1's built its
 * MultiplayerMsgStrings table as well; the archive is read only by this unit's functions
 * (0x1E8FE0, 0x1E91C0, 0x1EABD0, 0x1EAC80, which main calls: the command line's taken up and let
 * go of). SOCOM 1's four, in its order: UnloadTheWorld 0x1E8FE0 (with FreeVector's copy 0x1E9120),
 * LoadWorld 0x1E91C0 (0x1090: CMenuState::Init and testLoad call it -- SOCOM 1's 0x96C),
 * testUnload 0x1EA6E0 and testLoad 0x1EA720 (both still drafts). The template tail
 * (0x1EA250-0x1EABCF, the pointer lists' and CNodeVector's pieces every later unit links to) and
 * SOCOM II's command-line functions (0x1EABD0-0x1EB03F) stay with the drafts. Defined in reverse
 * of their image order (-inline deferred). */

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
    ~CAppCamera();
    virtual void Tick(float dT);
    virtual bool AttachTo(CZSealBody *prey);

    unsigned char _4[0xb4 - 0x4];
    zdb::CNode *m_camera;   // 0xb4 (camera.h's zdb::CCamera *; SOCOM 1: 0x34)
};

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

/* CVisual's map textures beside m_lightMapTexture (SOCOM 1's statics, cleared in its order:
 * m_specMapTexture, m_envFrontMapTexture, m_envBackMapTexture -- znode.h's CVisual lacks them) */
extern "C" {
extern zdb::CTexHandle *data_004B4D90;
extern zdb::CTexHandle *data_004B4E48;
extern zdb::CTexHandle *data_004B4E50;

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
        data_004B4D90 = 0;
        data_004B4E48 = 0;
        data_004B4E50 = 0;
        FreeTickList();
        if (zdb::NodeUniverse) {
            delete zdb::NodeUniverse;
            zdb::NodeUniverse = 0;
        }
        thePipe.Flush();
    }
}
