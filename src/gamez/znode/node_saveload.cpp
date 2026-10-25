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

/* the archive's calls not named in the map yet (SOCOM 1's names; each called where SOCOM 1's
 * code calls that one): zar::CZAR::Close, Open (with a fifth argument, the s1~ note's),
 * Fetch(const char *, int *) and ReleaseDataBuffer */
extern "C" void func_00258CE0(zar::CZAR *zar);
extern "C" bool func_00258920(zar::CZAR *zar, const char *name, int version, unsigned int a,
                              unsigned int b, int c);
extern "C" bool func_0025AB70(zar::CZAR *zar, const char *name, int *value);
extern "C" void *func_00257F90(zar::CZAR *zar);

namespace zdb {

/* the asset calls not named yet: SOCOM 1's CNode::Read(CSaveLoad &, CNode *) and CModel::Create
 * (both static), CTexture's constructor, CAssetLib::AddTexture(CTexture *) and RootName, and the
 * map's AddPalette and its neighbour -- by their callers AddModel (0x3215D0: it sets the model's
 * library) and AddPalette (0x321500, the bool) */
extern "C" CNode *func_00311520(CSaveLoad *io, CNode *parent);
extern "C" CModel *func_003119A0(CSaveLoad *io, CAssetLib *lib);
extern "C" CTexture *func_00354D90(CTexture *tex, const char *name);
extern "C" void func_00321670(CAssetLib *lib, CTexture *tex);
extern "C" const char *func_00321900(CAssetLib *lib);
extern "C" void func_003215D0(CAssetLib *lib, CModel *model);
extern "C" void func_00321500(CAssetLib *lib, CTexPalette *pal, bool b);

/* an empty loader: no world, the archive closed, no file name (SOCOM 1's) */
CSaveLoad::CSaveLoad()
    : m_world(0), m_zfile(0, 0)
{
    m_zed_filename[0] = 0;
}

/* the archive closed, then destroyed (SOCOM 1's) */
CSaveLoad::~CSaveLoad()
{
    func_00258CE0(&m_zfile);
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
    bool ok = func_00258920(&io->m_zfile, io->m_zed_filename, CWorld::GetVersion(), 1, 16, 0);

    if (!ok)
        func_00258CE0(&io->m_zfile);
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

        strcpy(root, func_00321900(lib));
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_txr");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadTextures_PS2(lib))
                result = false;
            func_00258CE0(&m_zfile);
        }
    }
    if (flags & 4) {
        char root[512];
        char folder[256];

        strcpy(root, func_00321900(lib));
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_pal");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadPalettes_PS2(lib))
                result = false;
            func_00258CE0(&m_zfile);
        }
    }
    if (flags & 2) {
        char root[512];
        char folder[256];

        strcpy(root, func_00321900(lib));
        if (strlen(root) > 4)
            root[4] = 0;
        FilterAssetLibFolder(lib->m_name, folder);
        sprintf(m_zed_filename, "run/%s/%s%s.zed", folder, root, "_geo");
        if (!OpenZed(this)) {
            result = false;
        } else {
            if (!LoadModels_PS2(lib))
                result = false;
            func_00258CE0(&m_zfile);
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

    if (func_0025AB70(&m_zfile, "renderphase", &phase)) {
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
                    CTexture *tex = (CTexture *)operator new(0x70);

                    if (tex)
                        tex = func_00354D90(tex, name);
                    if (tex->Read(*this))
                        func_00321670(lib, tex);
                    else
                        delete tex;
                }
                m_zfile.CloseKey(k);
            }
        }
        m_zfile.CloseKey(key);
    }
    lib->m_texture_buffer = func_00257F90(&m_zfile);
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
                func_00321500(lib, pal, false);
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
                CModel *model = func_003119A0(this, 0);

                if (model)
                    func_003215D0(lib, model);
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

/* the world's tree: each key under world_tree read as a node of the world's (SOCOM 1's) */
bool CWorld::Read(CSaveLoad &io)
{
    zar::CZAR *zfile = &io.m_zfile;
    zar::CKey *key = zfile->OpenKey("world_tree");

    if (key) {
        for (zar::CKeyRing::iterator it = key->begin(); it != key->end(); ++it) {
            zfile->OpenKey(*it);
            func_00311520(&io, this);
            zfile->CloseKey(*it);
        }
        zfile->CloseKey(key);
    }
    return true;
}

}
