/* bone_init.cpp -- the motion archives: the common one and the map's, their loaders and accessors
 * (SOCOM 1's unit: the press beta's asserts name it, and SOCOM 1's is_init, bone_archive_version
 * and boneArchive are its statics). OVL1 0x288920-0x28936C: the motion.rdr buffer's accessors,
 * the map archive's unload and load, the common archive's load, zBoneUnloadAll (FreeVector's
 * pieces for the scripts', commons' and 0xFC vectors after it, in reverse first use), the archive
 * accessors; the callback list's implicit constructor and its vector's destructor close it, and
 * its initialiser (0x3FFCE0) builds both archives. SOCOM 1's zBoneOpen, zBoneLoadAll and
 * zBoneClose are one function per archive here. Defined in reverse of their image order
 * (-inline deferred).
 * The initialiser (0x3FFCE0) inlines CScriptArchive's constructor one level, calling the members'
 * constructors out of line (the scripts', 0x2892F0 -- the callback list's implicit constructor --,
 * the commons' twice, the transitions') and then Init: byte-exact since include/vector's pointer
 * vector constructor costs what the original's did (the MSL headers' round 4: past
 * inline_max_size, where it was 231-240 units here; a level more instead loses CHUD's, CCell's
 * and CVisual's constructors and __sinit_compass.cpp). */

#include <string.h>

#include "zbody.h"
#include "../zsys/zsys.h"

/* the ZAR's functions zar.h does not declare: SOCOM II's five-argument Open, the key count, the
 * archive name's setter and the data buffer handed over (called by their map names) */
extern "C" {
void func_00258920(zar::CZAR *zar, const char *name, int version, unsigned int a, unsigned int b, int c);
int func_0025B300(zar::CKey *key, int deep);
void func_00258E80(zar::CZAR *zar, const char *name);
void *ReleaseDataBuffer__Q23zar4CZARFv(zar::CZAR *zar);
}

namespace {
/* the common archive's patch ZAR was found: the map's archive is looked for only then */
bool is_init = true;
}

CScriptArchive boneArchive;
CScriptArchive mapBoneArchive;
int bone_archive_version;
CScriptArchive *curArchive;

CScriptArchive *zBoneCurArchive()
{
    CScriptArchive *archive = curArchive;

    if (!archive)
        archive = &boneArchive;
    return archive;
}

void zBoneSetCurArchive(CScriptArchive *archive)
{
    curArchive = archive;
}

CScriptArchive *zBoneArchive()
{
    return &boneArchive;
}

CScriptArchive *zBoneMapArchive()
{
    return &mapBoneArchive;
}

CScriptArchive *zBoneGetArchive(int which)
{
    switch (which) {
    case 0:
        return 0;
    case 1:
        return &boneArchive;
    case 2:
        return &mapBoneArchive;
    }
    return 0;
}

/* everything an archive holds freed: the motion.rdr buffer, the callbacks, the scripts, the
 * commons and the 0xFC records (SOCOM 1's zBoneUnloadAll, which knew its one archive) */
void zBoneUnloadAll(CScriptArchive *archive)
{
    if (archive->m_buffer) {
        _free(archive->m_buffer, 0, 0);
        archive->m_buffer = 0;
    }
    archive->m_callbacks.Clear();
    while (archive->m_scripts.size()) {
        CZSIScript *script = archive->m_scripts.back();

        archive->m_scripts.pop_back();
        delete script;
    }
    FreeVector(archive->m_scripts);
    curArchive = 0;
    archive->FreeRdr(0);
    for (std::vector<CZSIScriptCommon *>::iterator i = archive->m_commons.begin(); i != archive->m_commons.end(); ++i)
        delete *i;
    FreeVector(archive->m_commons);
    FreeVector(archive->m_byRdr);
    for (std::vector<MOTION_TRANSITION *>::iterator j = archive->m_transitions.begin(); j != archive->m_transitions.end(); ++j)
        delete *j;
    FreeVector(archive->m_transitions);
    archive->Init();
}

/* the archive's ZAR opened at a path unless it is open already; true once newly open */
static inline bool OpenArchive(CScriptArchive *archive, const char *path)
{
    bool ok = false;

    if (archive->m_present && !archive->m_zar.IsOpen()) {
        func_00258920(&archive->m_zar, path, bone_archive_version, 1, 16, 0);
        ok = archive->m_zar.IsOpen();
    }
    return ok;
}

/* the common archive loaded: the patch ZAR, else the disc's, every script read, the ZAR's data
 * kept as the motion.rdr buffer, motion.rdr's settings applied */
void zBoneLoad()
{
    CScriptArchive *archive = &boneArchive;

    curArchive = archive;
    if (!boneArchive.m_present)
        archive->SetPresent(true);
    if (!OpenArchive(archive, "run/motion_p.zar")) {
        is_init = false;
        OpenArchive(archive, "run/motion.zar");
    }
    curArchive = archive;
    int n = func_0025B300(archive->m_zar.GetOpenKey(), 1);

    if (n > 1) {
        archive->m_commons.reserve(n);
        archive->LoadAll();
        archive->m_bufsize = *(int *)(archive->m_zar._18 + 8);
        archive->m_buffer = ReleaseDataBuffer__Q23zar4CZARFv(&archive->m_zar);
    }
    archive->m_zar.Close();
    archive->m_index = 1;
    archive->ReadRdr("motion.rdr", "data/common/zrdr");
    archive->ReadMotionRdr();
    archive->FreeRdr(0);
}

/* the map's archive loaded from its directory, with the caller's motion.rdr (kept) */
void zBoneLoadMap(const char *dir, CRdrFile *rdr)
{
    if (!is_init)
        return;
    char path[128];
    CScriptArchive *archive = &mapBoneArchive;

    curArchive = archive;
    if (!mapBoneArchive.m_present)
        archive->SetPresent(true);
    strcpy(path, dir);
    strcat(path, "motion_s.zar");
    OpenArchive(archive, path);
    curArchive = archive;
    int n = func_0025B300(archive->m_zar.GetOpenKey(), 1);
    bool loaded;

    if (n < 2) {
        loaded = false;
    } else {
        archive->m_commons.reserve(n);
        bool ok = archive->LoadAll();

        archive->m_bufsize = *(int *)(archive->m_zar._18 + 8);
        archive->m_buffer = ReleaseDataBuffer__Q23zar4CZARFv(&archive->m_zar);
        loaded = ok;
    }
    archive->m_zar.Close();
    if (loaded) {
        archive->m_index = 2;
        archive->SetRdr(rdr);
        archive->ReadMotionRdr();
        archive->FreeRdr(rdr);
        archive->Register();
    }
}

/* the map's archive emptied and closed */
void zBoneUnloadMap()
{
    mapBoneArchive.Unregister();
    mapBoneArchive.m_zar.Close();
    zBoneUnloadAll(&mapBoneArchive);
    mapBoneArchive.m_callbacks.Clear();
    func_00258E80(&mapBoneArchive.m_zar, 0);
    mapBoneArchive.SetPresent(false);
}

void *zBoneMotionRdrBuffer()
{
    return boneArchive.m_buffer;
}

int zBoneMotionRdrSize()
{
    return boneArchive.m_bufsize;
}
