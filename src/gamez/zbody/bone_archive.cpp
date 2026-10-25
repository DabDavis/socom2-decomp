/* bone_archive.cpp -- an archive of motion scripts (CScriptArchive; SOCOM 1's unit, the press beta's
 * asserts name it). OVL1 0x2875D0-0x28891F: both archives' callbacks reset, motion.rdr's reader,
 * the SEAL tables' registration, FindCommon, motion.rdr's three holders, LoadAll, the destructor
 * and Init. SOCOM 1's Fetch is gone; the commons moved in from CZSIScriptCommon's static list.
 * Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zbody.h"

/* the ZAR's walk over its keys with a callback (zar's, not named yet) */
extern "C" unsigned short func_0025A390(zar::CZAR *zar, int (*f)(zar::CZAR &, const char *, void *, int, void *),
                                        void *user);
/* the SEAL's anim tables: an archive's anim filed under its index, and every one of an index
 * taken out (OVL2, not named yet) */
extern "C" {
void func_005E5B10(int index, int id);
void func_005E5BF0(int index);
}

/* zrdr.h's reader and its release, declared as the archive holds them (zrdr.h's own _zrdr cannot
 * share a unit with zanim.h's) */
CRdrFile *zrdr_read(const char *name, const char *path, int flags);
int zrdr_free(CRdrFile *file);

void CScriptArchive::Init()
{
    m_name = 0;
    m_buffer = 0;
    m_bufsize = 0;
    SetPresent(false);
    m_nScripts = 0;
    m_rdr = 0;
    SetIndex(0);
}

CScriptArchive::~CScriptArchive()
{
    SetName(0);
    m_scripts.clear();
    zBoneUnloadAll(this);
    m_callbacks.Clear();
}

bool CScriptArchive::LoadAll()
{
    m_nScripts = func_0025A390(&m_zar, CZSIScript::CreateAndReadIt, this);
    return m_nScripts > 0;
}

CRdrFile *CScriptArchive::ReadRdr(const char *name, const char *path)
{
    if (!m_rdr)
        m_rdr = zrdr_read(name, path, 0);
    return m_rdr;
}

CRdrFile *CScriptArchive::SetRdr(CRdrFile *rdr)
{
    if (!m_rdr)
        m_rdr = rdr;
    return m_rdr;
}

void CScriptArchive::FreeRdr(CRdrFile *keep)
{
    if (m_rdr != keep)
        zrdr_free(m_rdr);
    m_rdr = 0;
}

CZSIScriptCommon *CScriptArchive::FindCommon(const char *name)
{
    for (std::vector<CZSIScriptCommon *>::iterator i = m_commons.begin(); i != m_commons.end(); ++i)
        if (!strcmp(name, (*i)->m_pName))
            return *i;
    return 0;
}

void CScriptArchive::Register()
{
    int index = m_index;

    for (std::vector<CZSIScriptCommon *>::iterator i = m_commons.begin(); i != m_commons.end(); ++i)
        func_005E5B10(index, (*i)->m_id);
}

void CScriptArchive::Unregister()
{
    func_005E5BF0(m_index);
    SetIndex(0);
}

/* both archives' callbacks to be looked up again (CMission::Init, after zBoneLoad) */
void zBoneResetCallbacks()
{
    CScriptArchive *archive = zBoneArchive();

    if (archive)
        archive->m_callbacks.ResetAnims();
    archive = zBoneMapArchive();
    if (archive)
        archive->m_callbacks.ResetAnims();
}
