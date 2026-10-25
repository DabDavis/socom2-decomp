/* zrdr_archive.cpp -- CRdrArchive, the ZAR archives an .rdr is looked for in before the disc
 * (OVL1 0x32C040-0x32C64F): added and removed by name, opened and closed together, searched in
 * order. The unit name is SOCOM 1's (its __sinit_zrdr_archive.cpp builds CRdrArchive::m_list, as
 * SOCOM II's 0x402D70 builds the list at 0x488510). Defined in reverse of their image order
 * (-inline deferred). */

#include "zrdr.h"
#include <list>
#include <string.h>
#include <stdio.h>

/* SOCOM 1's CRdrArchiveList: the archives, in the order added */
class CRdrArchiveList : public std::list<CRdrArchive *> {
public:
    ~CRdrArchiveList() {}
};

CRdrArchiveList CRdrArchive::m_list;

/* zar::CZAR::ReOpen(int, unsigned int), not named yet: the archive opened again by its name
 * unless it is open (SOCOM 1's OpenAll and FindRdr call ReOpen there) */
extern "C" bool func_00258B40(zar::CZAR *zar, int a, unsigned int b);
/* CRdrFile::Load(zar::CZAR *, zar::CKey *, int) (zrdr_file.cpp) */
extern "C" CRdrFile *func_0032C980(zar::CZAR *zar, zar::CKey *key, int flags);

void CRdrArchive::OpenAll()
{
    CRdrArchiveList::iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e; ++i)
        func_00258B40(*i, 1, 1);
}

void CRdrArchive::CloseAll()
{
    CRdrArchiveList::iterator i, e = m_list.end();

    for (i = m_list.begin(); i != e; ++i)
        (*i)->Close();
}

/* the archive of that file name (SOCOM II's; the name ours) */
#pragma auto_inline off
CRdrArchive *CRdrArchive::Find(const char *name)
{
    if (!name)
        return 0;
    CRdrArchiveList::iterator i, e = m_list.end();
    for (i = m_list.begin(); i != e; ++i) {
        CRdrArchive *archive = *i;
        if (!strcasecmp(archive->m_filename, name))
            return archive;
    }
    return 0;
}
#pragma auto_inline reset

CRdrArchive::~CRdrArchive()
{
}

/* SOCOM 1's: every archive whose file name holds path/name closed and dropped */
bool CRdrArchive::RemoveArchive(const char *name, const char *path)
{
    char buf[256];
    bool removed = false;

    sprintf(buf, "%s/%s", path, name);
    CRdrArchiveList::iterator i, e = m_list.end();
    for (i = m_list.begin(); i != e;) {
        CRdrArchive *archive = *i;
        if (strstr(archive->m_filename, buf)) {
            i = m_list.erase(i);
            archive->Close();
            delete archive;
            removed = true;
        } else
            ++i;
    }
    return removed;
}

/* SOCOM 1's: path/name added unless it is there */
CRdrArchive *CRdrArchive::AddArchive(const char *name, const char *path)
{
    char buf[1024];
    CRdrArchive *archive = 0;

    if (path)
        sprintf(buf, "%s/%s", path, name);
    else
        strcpy(buf, name);
    archive = Find(buf);
    if (!archive) {
        archive = new CRdrArchive(buf);
        m_list.push_back(archive);
    }
    return archive;
}

/* the file name without its directory (ours; SOCOM 1's FindRdr inlines it too). Both builds
 * keep the null test of the buffer -- a local array's address, which MW folds away as a plain
 * truth test, even through an inline's parameter; through an int it stays */
static inline void _basename(char *out, const char *path)
{
    const char *p = strrchr(path, '/');

    if (!p)
        p = strrchr(path, '\\');
    if (!p) {
        if ((int)out)
            strcpy(out, path);
    } else if (p[1]) {
        if ((int)out)
            strcpy(out, p + 1);
    }
}

/* SOCOM 1's: the file looked for in each archive in turn, each opened for the look if it was
 * closed */
CRdrFile *CRdrArchive::FindRdr(const char *path, int flags)
{
    char name[1024];
    CRdrFile *file;
    _basename(name, path);
    file = 0;
    CRdrArchiveList::iterator i;
    for (i = m_list.begin(); !file && i != m_list.end(); ++i) {
        CRdrArchive *archive = *i;
        bool opened = false;
        if (!archive->IsOpen()) {
            func_00258B40(archive, 1, 1);
            opened = true;
        }
        file = 0;
        zar::CKey *key = archive->OpenKey(name);
        if (key) {
            file = func_0032C980(archive, key, flags);
            archive->CloseKey(key);
        }
        if (opened)
            archive->Close();
    }
    return file;
}
