/* node_assetlib.cpp -- the asset manager: the list of loaded libraries and its recycle bin, a
 * library's models, textures and palettes, the lookups by name and render phase (OVL1
 * 0x320A00-0x321DAF). The unit name is SOCOM 1's: its static initialiser (0x402A00) builds the
 * same two lists SOCOM 1's __sinit_node_assetlib.cpp does, the anonymous-namespace recycle bin
 * first, then CAssetMgr::m_assets. Its order is SOCOM 1's, ReleaseLibRef first and the list's
 * destructor last; SOCOM II adds the lookups AddTexture and AddModel make, the multi-library
 * load, the mip walk, and its CAssetList constructor out of line. Defined in reverse of their
 * image order (-inline deferred). */

#include <algorithm>
#include <string.h>

#include "node_world.h"

/* the texture list's lookups not named yet (vis code, SOCOM 1's CTexList::GetHandle(const char *),
 * Append(CTexture *, bool) and Free, at SOCOM 1's call sites), and the texture test the mip walk
 * makes (SOCOM II's) */
extern "C" zdb::CTexHandle *func_00354670(zdb::CTexList *list, const char *name);
extern "C" zdb::CTexHandle *func_003545F0(zdb::CTexList *list, zdb::CTexture *tex, bool b);
extern "C" void func_00354510(zdb::CTexList *list);
extern "C" bool func_00357470(zdb::CTexture *tex, zdb::CTexture *parent);

namespace zdb {

namespace {
/* the libraries no one holds, kept until the next load (SOCOM 1's) */
CAssetList g_assetsRecycle;
}

CAssetList CAssetMgr::m_assets;

/* a library deleted, its slot cleared (the operation the lists are emptied with): the null test
 * written out, each path returning its own zero (Clear's two `move v0,zero`, SOCOM 1's alike) */
struct DeleteLib {
    CAssetLib *operator()(CAssetLib *lib) const
    {
        if (!lib)
            return 0;
        delete lib;
        return 0;
    }
};

/* a model let go of by its library and deleted, its slot cleared */
struct DeleteModel {
    DeleteModel() {}
    CModel *operator()(CModel *model) const
    {
        model->m_AssetLib = 0;
        delete model;
        return 0;
    }
};

/* a palette deleted */
struct DeletePalette {
    DeletePalette() {}
    void operator()(CTexPalette *pal) const { delete pal; }
};

void EmptyAssetRecycleBin()
{
    g_assetsRecycle.Clear();
}

/* an empty, unlocked, autoloaded library of that name, held once (SOCOM 1's) */
CAssetLib::CAssetLib(const char *name)
    : m_name(0), m_socom2_4(0)
{
    m_texture_buffer = 0;
    m_model_buffer = 0;
    m_socom2_38 = 0;
    m_iRefCount = 1;
    m_autoload = 1;
    m_locked = 0;
    m_gearlib = 0;
    m_renderphase = WORLD;
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    m_name = _strdup(name, 0, 0);
}

/* the models let go of and deleted, the buffers freed, the textures and palettes freed, the names
 * freed (SOCOM 1's) */
CAssetLib::~CAssetLib()
{
    std::transform(m_models.begin(), m_models.end(), m_models.begin(), DeleteModel());
    m_models.clear();
    if (m_texture_buffer)
        _free(m_texture_buffer, 0, 0);
    m_texture_buffer = 0;
    if (m_model_buffer)
        _free(m_model_buffer, 0, 0);
    m_model_buffer = 0;
    if (m_socom2_38)
        _free(m_socom2_38, 0, 0);
    m_socom2_38 = 0;
    func_00354510(&m_textures);
    std::for_each(m_palettes.begin(), m_palettes.end(), DeletePalette());
    m_palettes.clear();
    if (m_name)
        _free(m_name, 0, 0);
    m_name = 0;
    if (m_socom2_4)
        _free(m_socom2_4, 0, 0);
    m_socom2_4 = 0;
}

/* the name's last path component (SOCOM 1's) */
char *CAssetLib::RootName()
{
    for (int i = strlen(m_name) - 1; i >= 0; i--)
        if (m_name[i] == '/' || m_name[i] == '\\')
            return &m_name[i + 1];
    return m_name;
}

/* the name the library's, or the same from the end, separators skipped (SOCOM 1's) */
bool CAssetLib::IsNamed(const char *name)
{
    if (!strcmp(name, m_name))
        return true;
    int i = strlen(name) - 1;
    int j = strlen(m_name) - 1;
    bool match = true;

    while (match) {
        while (i >= 0 && (name[i] == '/' || name[i] == '\\'))
            i--;
        while (j >= 0 && (m_name[j] == '/' || m_name[j] == '\\'))
            j--;
        if (i >= 0 && j >= 0) {
            if (name[i] != m_name[j])
                match = false;
            else {
                i--;
                j--;
            }
        } else if (i != j)
            match = false;
        else
            return match;
    }
    return match;
}

/* a loaded texture of that name added to the library (SOCOM 1's) */
CTexHandle *CAssetLib::AddTexture(const char *name)
{
    if (m_locked)
        return 0;
    if (!name)
        return 0;
    CTexHandle *h = CAssetMgr::m_assets.GetTexHandle(name);

    if (h)
        return AddTexture(h->m_texture);
    return 0;
}

/* a texture added, unless a library has one of its name already (SOCOM 1's) */
#pragma auto_inline off
CTexHandle *CAssetLib::AddTexture(CTexture *tex)
{
    if (m_locked)
        return 0;
    if (!tex)
        return 0;
    CTexHandle *h = 0;

    if (!CAssetMgr::m_assets.FindTextureLib(tex->m_name)) {
        h = func_003545F0(&m_textures, tex, true);
        tex->m_AssetLib = this;
    }
    return h;
}
#pragma auto_inline reset

#pragma auto_inline off
CAssetList *CWorld::GetAssetList()
{
    return &CAssetMgr::m_assets;
}
#pragma auto_inline reset

/* a model added, unless a library has one of its name already (SOCOM 1's; SOCOM II no longer moves
 * a model between libraries) */
CModel *CAssetLib::AddModel(CModel *model)
{
    if (m_locked)
        return 0;
    if (!CWorld::GetAssetList()->FindModelLib(model->m_name)) {
        m_models.push_back(model);
        model->m_AssetLib = this;
    }
    return model;
}

/* a palette added, unless it is listed already when asked to check (SOCOM 1's) */
CTexPalette *CAssetLib::AddPalette(CTexPalette *pal, bool check)
{
    bool add = true;

    if (check)
        for (CTexPalList::iterator it = m_palettes.begin(); it != m_palettes.end(); ++it)
            if (*it == pal) {
                add = false;
                break;
            }
    if (add) {
        m_palettes.push_back(pal);
        pal->m_AssetLib = this;
        return pal;
    }
    return 0;
}

CAssetList::CAssetList()
{
    m_cache_model = 0;
}

#pragma auto_inline off
void CAssetList::Clear()
{
    std::transform(begin(), end(), begin(), DeleteLib());
    erase(begin(), end());
    m_cache_model = 0;
}
#pragma auto_inline reset

/* a texture by name, from whichever library has it (SOCOM 1's) */
CTexHandle *CAssetList::GetTexHandle(const char *name)
{
    CTexHandle *h = 0;
    lib_iterator e = list_base::end();

    for (lib_iterator it = list_base::begin(); it != e; ++it)
        if ((h = func_00354670(&((CAssetLib *)*it)->m_textures, name)) != 0)
            break;
    return h;
}

CTexHandle *CAssetList::FindMipLevel(_RenderPhase phase, CTexHandle *tex)
{
    CTexHandle *found = 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e && !found; ++it)
        if (phase == 19 || ((CAssetLib *)*it)->m_renderphase == phase)
            for (CTexList::iterator t = ((CAssetLib *)*it)->m_textures.begin();
                 t != ((CAssetLib *)*it)->m_textures.end() && !found; ++t)
                if (func_00357470((*t)->m_texture, tex->m_texture))
                    found = *t;
    return found;
}

/* a library by name, or by its name's last component (SOCOM 1's) */
#pragma auto_inline off
CAssetLib *CAssetList::FindLib(const char *name)
{
    CAssetLib *lib = 0;

    if (!name)
        return 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e; ++it)
        if (((CAssetLib *)*it)->IsNamed(name)) {
            lib = (CAssetLib *)*it;
            break;
        }
    if (!lib) {
        lib_iterator e = end();

        for (lib_iterator it = begin(); it != e; ++it)
            if (!strcmp(((CAssetLib *)*it)->RootName(), name)) {
                lib = (CAssetLib *)*it;
                break;
            }
    }
    return lib;
}
#pragma auto_inline reset

/* the first library of a render phase (SOCOM 1's) */
CAssetLib *CAssetList::FindLib(_RenderPhase phase)
{
    CAssetLib *lib = 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e; ++it)
        if (((CAssetLib *)*it)->GetRenderPhase() == phase) {
            lib = (CAssetLib *)*it;
            break;
        }
    return lib;
}

#pragma auto_inline off
CAssetLib *CAssetList::FindModelLib(const char *name)
{
    CAssetLib *lib = 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e; ++it)
        if (((CAssetLib *)*it)->m_models.GetModel(name)) {
            lib = (CAssetLib *)*it;
            break;
        }
    return lib;
}
#pragma auto_inline reset

#pragma auto_inline off
CAssetLib *CAssetList::FindTextureLib(const char *name)
{
    CAssetLib *lib = 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e; ++it)
        if (func_00354670(&((CAssetLib *)*it)->m_textures, name)) {
            lib = (CAssetLib *)*it;
            break;
        }
    return lib;
}
#pragma auto_inline reset

/* a model by name, the last one found cached (SOCOM 1's) */
CModel *CAssetList::GetModel(const char *name)
{
    if (m_cache_model && name && !strcmp(m_cache_model->m_name, name))
        return m_cache_model;
    CModel *model = 0;
    lib_iterator e = end();

    for (lib_iterator it = begin(); it != e; ++it)
        if ((model = ((CAssetLib *)*it)->m_models.GetModel(name)) != 0)
            break;
    m_cache_model = model;
    return model;
}

#pragma auto_inline off
CAssetLib *CAssetMgr::NewLib(const char *name, int unused)
{
    CAssetLib *lib = new CAssetLib(name);

    m_assets.push_back(lib);
    return lib;
}
#pragma auto_inline reset

CAssetLib *CAssetMgr::GetLoadedLibRef(const char *name)
{
    CAssetLib *lib = m_assets.FindLib(name);

    if (lib) {
        lib->m_iRefCount++;
    } else if ((lib = g_assetsRecycle.FindLib(name)) != 0) {
        lib->m_iRefCount++;
        g_assetsRecycle.list_imp::remove((const void *const &)lib);
        m_assets.push_back(lib);
    } else {
        g_assetsRecycle.Clear();
        CSaveLoad io;

        lib = NewLib(name, 0);
        io.LoadAssetLib(0, lib, 255);
    }
    g_assetsRecycle.Clear();
    return lib;
}

void CAssetMgr::GetLoadedLibRefs(const char **names, int n, CAssetList *list)
{
    int i;

    for (i = 0; i < n; i++) {
        const char *name = names[i];
        CAssetLib *lib = m_assets.FindLib(name);

        if (lib) {
            lib->m_iRefCount++;
            list->push_back(lib);
        } else if ((lib = g_assetsRecycle.FindLib(name)) != 0) {
            lib->m_iRefCount++;
            g_assetsRecycle.list_imp::remove((const void *const &)lib);
            m_assets.push_back(lib);
            list->push_back(lib);
        }
    }
    g_assetsRecycle.Clear();
    for (i = 0; i < n; i++) {
        const char *name = names[i];
        CAssetLib *lib = list->FindLib(name);

        if (!lib) {
            CSaveLoad io;

            lib = NewLib(name, 0);
            io.LoadAssetLib(0, lib, 255);
            list->push_back(lib);
        }
    }
}

void CAssetMgr::ReleaseLibRef(CAssetLib *lib)
{
    if (lib) {
        lib->m_iRefCount--;
        if (lib->m_iRefCount <= 0) {
            m_assets.list_imp::remove((const void *const &)lib);
            m_assets.m_cache_model = 0;
            g_assetsRecycle.push_back(lib);
        }
    }
}

void CAssetMgr::AddMipLevels(CDynTexList *list, _RenderPhase phase)
{
    for (unsigned int i = 0; i < list->m_tex_list.size(); i++) {
        CTexHandle *h = list->m_tex_list[i];

        do {
            if (!h->m_texture->m_MIP1)
                break;
            h = m_assets.FindMipLevel(phase, h);
            if (h)
                list->Add(h, true);
        } while (h);
    }
}

}
