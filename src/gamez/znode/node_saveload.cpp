/* node_saveload.cpp -- zdb::CSaveLoad, the world's and the asset libraries' loader: a ZED
 * archive opened by name, the world built from it, a library's textures, palettes and models read
 * from its three archives (OVL1 0x318CB0-0x31A17B). The unit name is SOCOM 1's (the press beta's
 * CSaveLoad::Load asserts node_saveload.cpp); its order is SOCOM 1's, CWorld::Read first, the
 * constructor and destructor last. SOCOM II inlines FilterMissionFolder into Load (its out-of-line
 * copy is gone) and adds the model reader and the folder filter for libraries. Defined in reverse
 * of their image order (-inline deferred). */

#include <stdio.h>
#include <string.h>

#include "node_world.h"
#include "../zcamera/zcamera.h"


namespace zdb {

/* an empty loader: no world, the archive closed, no file name (SOCOM 1's) */
CSaveLoad::CSaveLoad()
    : m_world(0), m_zfile(0, 0)
{
    m_zed_filename[0] = 0;
}

/* the archive closed, then destroyed (SOCOM 1's) */
CSaveLoad::~CSaveLoad()
{
    m_zfile.Close();
}

}

/* a library's folder under run/, out of its name: "common/<name>" for a local common library,
 * else its first directory -- and the next one too for a mission's (sp/m.., mp/m..) -- leading
 * separators skipped (new in SOCOM II; the name is ours) */
void FilterAssetLibFolder(const char *name, char *folder)
{
    if (strstr(name, "common/assetlib/local/")) {
        const char *p;

        strcpy(folder, "common/");
        p = name + 22;
        folder += 7;
        while (*p != '/' && *p != 0 && *p != '\\')
            *folder++ = *p++;
    } else {
        const char *p = name;

        /* the test made once ahead of the loop (the target reads name[0] for it, then enters the
         * loop's own test): a plain while or for compiles without it */
        if ((*p == '/' || *p == '\\') && *p != 0)
            while ((*p == '/' || *p == '\\') && *p != 0)
                p++;
        while (*p != '/' && *p != 0 && *p != '\\')
            *folder++ = *p++;
        if (strstr(name, "sp/m") || strstr(name, "mp/m")) {
            *folder++ = *p++;
            while (*p != '/' && *p != 0 && *p != '\\')
                *folder++ = *p++;
        }
    }
    *folder = 0;
}

/* the node of that name in a tree, the root first, then each child's tree in turn (SOCOM 1's;
 * SOCOM II's inlines itself once, its innermost copy calling the accessors and the iterator's
 * pieces out of line) */
zdb::CNode *find_node(zdb::CNode *node, const char *name)
{
    if (!node || !name)
        return 0;
    if (!strcasecmp(node->GetName(), name))
        return node;
    zdb::CNodeVector &children = node->GetChildren();

    for (zdb::CNodeVector::iterator it = children.begin(), e = children.end(); it != e; ++it) {
        zdb::CNode *found = find_node(*it, name);

        if (found)
            return found;
    }
    return 0;
}

namespace zdb {

/* the archive named by m_zed_filename opened at the world's version, closed again on failure
 * (inline: SOCOM 1's Load and LoadAssetLib_PS2 keep its result and test it twice; the name is
 * ours) */
inline bool OpenZed(CSaveLoad *io)
{
    bool ok = io->m_zfile.Open(io->m_zed_filename, CWorld::GetVersion(), 1, 16, 0);

    if (!ok)
        io->m_zfile.Close();
    return ok;
}

/* a library's archives read as the flags ask: its textures (1), palettes (4) and models (2), each
 * from run/<folder>/<root><ext>.zed, the root cut to four letters (SOCOM 1's, the folder now its
 * own; true unless one fails) */
bool CSaveLoad::LoadAssetLib_PS2(CWorld *world, CAssetLib *lib, int flags)
{
    bool result = true;

    m_world = world;
    if (flags & 1) {
        char root[512];
        char folder[256];

        strcpy(root, lib->RootName());
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_txr");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadTextures_PS2(lib))
                result = false;
            m_zfile.Close();
        }
    }
    if (flags & 4) {
        char root[512];
        char folder[256];

        strcpy(root, lib->RootName());
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_pal");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadPalettes_PS2(lib))
                result = false;
            m_zfile.Close();
        }
    }
    if (flags & 2) {
        char root[512];
        char folder[256];

        strcpy(root, lib->RootName());
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_geo");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadModels_PS2(lib))
                result = false;
            m_zfile.Close();
        }
    }
    return result;
}

/* the library's render phase (a character set's 3..10 made the custom one) and its textures, each
 * read unless a library or this one has it already; the archive's buffer kept as the library's
 * (SOCOM 1's) */
bool CSaveLoad::LoadTextures_PS2(CAssetLib *lib)
{
    int phase = -1;

    if (m_zfile.Fetch("renderphase", &phase)) {
        if (phase >= CHARSET1 && phase < ALPHA)
            phase = CHARSETCUSTOM;
        lib->m_renderphase = (_RenderPhase)phase;
    }
    zar::CKey *key;

    if ((key = m_zfile.OpenKey("textures")) != 0) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            zar::CKey *k = *it;

            if (m_zfile.OpenKey(k)) {
                const char *name = k->m_name;

                if (!CAssetMgr::m_assets.GetTexHandle(name) && !lib->AddTexture(name)) {
                    CTexture *tex = new CTexture(name);

                    if (tex->Read(*this))
                        lib->AddTexture(tex);
                    else
                        delete tex;
                }
                m_zfile.CloseKey(k);
            }
        }
        m_zfile.CloseKey(key);
    }
    lib->m_texture_buffer = m_zfile.ReleaseDataBuffer();
    return true;
}

/* the library's palettes, emptied first and each read and added (SOCOM 1's) */
#pragma auto_inline off
bool CSaveLoad::LoadPalettes_PS2(CAssetLib *lib)
{
    lib->m_palettes.clear();
    zar::CKey *key;

    if ((key = m_zfile.OpenKey("palettes")) != 0) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            zar::CKey *k = *it;

            if (m_zfile.OpenKey(k)) {
                const char *name = k->m_name;
                CTexPalette *pal = new CTexPalette(name);

                pal->Read(*this);
                lib->AddPalette(pal, false);
                m_zfile.CloseKey(k);
            }
        }
        m_zfile.CloseKey(key);
    }
    return true;
}
#pragma auto_inline reset

/* the library's models, each created from its key and added (SOCOM 1's LoadAssetLib_PS2 did this
 * itself) */
#pragma auto_inline off
bool CSaveLoad::LoadModels_PS2(CAssetLib *lib)
{
    zar::CKey *key;

    if ((key = m_zfile.OpenKey("models")) != 0) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            if (m_zfile.OpenKey(*it)) {
                CModel *model = CModel::Create(*this, 0);

                if (model)
                    lib->AddModel(model);
                m_zfile.CloseKey(*it);
            }
        }
        m_zfile.CloseKey(key);
    }
    return true;
}
#pragma auto_inline reset

/* a library loaded (SOCOM 1's: the PS2 reader) */
bool CSaveLoad::LoadAssetLib(CWorld *world, CAssetLib *lib, int flags)
{
    bool result = LoadAssetLib_PS2(world, lib, flags);

    return result;
}

/* a mission's archive path under the root: <root>/mp/<name> for a multiplayer map (mp...),
 * <root>/sp/<name> for a single-player mission (m...), else <root>/<name> (SOCOM 1's
 * FilterMissionFolder; SOCOM II inlines it into Load and keeps no out-of-line copy) */
inline void FilterMissionFolder(const char *mission, const char *root, const char *name, char *out)
{
    if (!root)
        root = "/";
    if (!strncmp(mission, "mp", 2))
        sprintf(out, "%s/mp/%s", root, name);
    else if (!strncmp(mission, "m", 1))
        sprintf(out, "%s/sp/%s", root, name);
    else
        sprintf(out, "%s/%s", root, name);
}

/* the world of that name loaded (SOCOM 1's; the press beta's asserts name it): its archive
 * run/<folder>/<name>/<name>.zed opened, the world made and initialised, the archive's world
 * settings read (the night switch, the lens effects, the world text's specs, the default material
 * and foliage sound, the scale, the shadow's direction and weight, the global lighting), the
 * camera's parameters, the grid, the asset libraries, the lights, the world's tree, the LOD,
 * material and scrolling-texture tables -- each scrolling band set on its model's node, or the
 * world model's -- and the clutter but for the UI's world; none if the archive will not open.
 * Written in the world lane's round 30, byte-exact: the scale through a local (both divisions ahead
 * of the stores, as SOCOM 1's), the world into a local before the material's _strdup (the target
 * holds it in s1 across the call), the camera's parameters declared uninitialised ahead of the
 * camera (numbered first: s2 / s1), the camera key assigned then tested (move; beqz s3) where the
 * other keys test the call's v0, and the libraries' key a second variable declared at the top (its
 * iterator then takes s1, the key s2) */
CWorld *CSaveLoad::Load(const char *name)
{
    zar::CKey *key;
    zar::CKey *libs;

    FilterMissionFolder(name, "/run", name, m_zed_filename);
    strcat(m_zed_filename, "/");
    strcat(m_zed_filename, name);
    strcat(m_zed_filename, ".zed");
    if (!OpenZed(this))
        return 0;
    m_world = new CWorld(name);
    m_world->Initalize();

    float scale = 1.0f;
    CPnt3D shadow = CPnt3D::zero;
    float weight = 0.25f;
    bool gridded = false;
    char *material = 0;
    char *sound = 0;
    int night = 0;
    PNT4D v;

    m_zfile.Fetch("NightMission", &night);
    m_world->m_night_mission = night != 0;
    if (m_zfile.Fetch("LensFX_NVG", &v, 16))
        m_world->m_lensfx_nvg = v;
    if (m_zfile.Fetch("LensFX_StarlightScope", &v, 16))
        m_world->m_lensfx_starlightscope = v;
    if (m_zfile.Fetch("WorldTextSpecs", &v, 16))
        *(PNT4D *)m_world->m_world_text_specs = v;
    m_zfile.Fetch("DefaultMaterial", &material);
    m_zfile.Fetch("FoliageSound", &sound);
    m_zfile.Fetch("MetersPerUnit", &scale);
    m_zfile.Fetch("GriddedTerrain", &gridded);
    m_zfile.Fetch("ShadowVector", &shadow, 12);
    m_zfile.Fetch("ShadowWeight", &weight);
    m_zfile.Fetch("GlobalLighting", &m_world->m_gLight, 0x70);
    float s = 1.0f / scale;

    CWorld::m_scale = s;
    CWorld::m_invscale = 1.0f / s;
    shadow.Normalize();
    m_world->m_shadow_vector = shadow;
    m_world->m_shadow_weight = weight;
    if (material) {
        CWorld *world = m_world;

        world->m_default_soiltype_name = _strdup(material, 0, 0);
        _free(material, 0, 0);
        material = 0;
    }
    if (sound) {
        m_world->SetFoliageSound(sound);
        _free(sound, 0, 0);
        sound = 0;
    } else {
        m_world->SetFoliageSound("none");
    }
    key = m_zfile.OpenKey("cameras");
    if (key) {
        tag_CAMERA_PARAMS *params;
        CCamera *camera = m_world->m_camera;

        params = camera;

        m_zfile.Fetch("camera", params, 0x90);
        camera->m_guardBand_min[3] = camera->m_near_plane;
        camera->m_guardBand_max[3] = camera->m_far_plane;
        CVisual::m_basefog_color = params->GetFogColor();
        CVisual::m_basefog_near = params->m_fog_near;
        camera->SetHalfHorizontalFOVRadians(0.61086524f, true);
        m_zfile.CloseKey(key);
    }
    m_world->m_grid.Read(*this);
    if ((libs = m_zfile.OpenKey("assetlibs")) != 0) {
        for (zar::CKeyRing::iterator it = libs->begin(); it != libs->end(); ++it)
            CAssetMgr::GetLoadedLibRef((*it)->m_name);
        m_zfile.CloseKey(libs);
    }
    if ((key = m_zfile.OpenKey("light_list")) != 0) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            if (m_zfile.OpenKey(*it)) {
                CNode::Read(*this, m_world);
                m_zfile.CloseKey(*it);
            }
        }
        m_zfile.CloseKey(key);
    }
    m_world->Read(*this);
    m_world->m_LOD_Object.Read(this);
    m_world->m_Material_Object.Read(this);
    CScrollingTexture_Object &scroll = m_world->m_ScrollingTexture_Object;

    scroll.Read(this);
    int n = scroll.size();

    for (int i = 0; i < n; i++) {
        CScrollingTexture_band *band = &scroll[i];
        CModel *model = (CModel *)CWorld::GetModel(band->m_modelname);
        CNode *worldmodel = CWorld::GetModel("worldmodel");
        CNode *node = find_node(model, band->m_nodename);

        if (!node && worldmodel) {
            CNode *found = find_node(worldmodel, band->m_nodename);

            if (found)
                found->ScrollTexture(0, band->m_du, band->m_dv, true, -1);
        } else if (model && model->m_list.size() && node) {
            for (CRefList::iterator it = model->m_list.begin(); it != model->m_list.end(); ++it)
                node->ScrollTexture(0, band->m_du, band->m_dv, true, -1);
        }
    }
    if (strcmp(m_world->m_name, "ui"))
        m_world->m_grid.ReadClutter(name, 0);
    m_zfile.Close();
    return m_world;
}

/* the world's tree: each key under world_tree read as a node of the world's (SOCOM 1's) */
bool CWorld::Read(CSaveLoad &io)
{
    zar::CZAR *zfile = &io.m_zfile;
    zar::CKey *key = zfile->OpenKey("world_tree");

    if (key) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            zfile->OpenKey(*it);
            CNode::Read(io, this);
            zfile->CloseKey(*it);
        }
        zfile->CloseKey(key);
    }
    return true;
}

}
