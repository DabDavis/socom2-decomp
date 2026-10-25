/* util_main.cpp -- the package CFileIO::Open reads through, the IOP reboot notifier's registration
 * and the string helper after it (OVL1 0x39C330-0x39CD3F). SOCOM 1's demo holds COutput, CIOPManager::(Un)RegisterRebootCallback and
 * zUtilInit/Uninit as one run between CBufferIO's unit and CFileIO's, ending on the CRegistrant
 * vector's template instances; no assert names the file, so util_main.cpp is ours. theIOPManager
 * itself is zsys_init.cpp's (its static initialiser is that unit's).
 *
 * OPEN, not written yet: 0x39C500 (named COutput's destructor by s1:align, but it is the package's:
 * Close inlined, then the deleting flag -- its draft matches) and 0x39C580,
 * UnRegisterRebootCallback (0x39C590: std::remove_if over class iterators, then the erase at
 * 0x3503F0) and the template tail (remove_if 0x39C710, insert 0x39C7F0, 0x39CCB0, 0x39CD40). */

#include <vector>
#include <string.h>
#include "zutil.h"

extern "C" int toupper(int c);

class CIOPManager {
public:
    enum RebootPhase { PreReboot = 0, PostReboot = 1 };
    enum RebootMode { FirstBoot = 0 };
    typedef bool (*RebootCallback)(RebootPhase phase, RebootMode mode, void *data);

    /* a registered callback (SOCOM 1's CRegistrant; the members' names ours) */
    class CRegistrant {
    public:
        RebootCallback m_callback;   // 0x0
        void *m_data;                // 0x4
        int m_id;                    // 0x8
        bool m_delete;               // 0xc
    };   // 0x10 bytes

    int RegisterRebootCallback(RebootCallback cb, void *data);

    std::vector<CRegistrant> m_registrants;   // 0x0
};

/* the next registration's id (0x49E2F0; ours) */
extern int data_0049E2F0;

int CIOPManager::RegisterRebootCallback(RebootCallback cb, void *data)
{
    CRegistrant r;

    r.m_callback = cb;
    r.m_data = data;
    r.m_id = data_0049E2F0++;
    r.m_delete = false;
    m_registrants.push_back(r);
    return r.m_id;
}

/* a string made upper case in place (ours) */
void zUtilStrUpr(char *s)
{
    unsigned int len = strlen(s);
    unsigned int i;

    for (i = 0; i < len; i++)
        s[i] = toupper(s[i]);
}

/* The package CFileIO::Open reads through (SOCOM II's, the names ours; 0x39C330-0x39C57F, before
 * the reboot registration): a header read whole -- 252 bytes with the first entry, the count at
 * 0x98, the entries from 0xA0, 92 bytes each -- and the one file it was read from, a static
 * (0x4A1630). fts_mission.cpp's UpdateMissionArchive closes and opens it; CFileIO::Open finds a
 * name in it, seeks its file to the entry and shares the descriptor. Its destructor inlines Close,
 * so the two are one unit with the reboot registration after them. */
struct PAKENTRY {
    int _0;
    char name[0x40];   // 0x4
    int offset;        // 0x44
    int size;          // 0x48
    unsigned char _4c[0x5c - 0x4c];
};   // 0x5c bytes

struct PAKHEADER {
    unsigned char _0[0x98];
    unsigned int count;    // 0x98
    int _9c;
    PAKENTRY entries[1];   // 0xa0
};   // 0xfc bytes

class CPackage {
public:
    ~CPackage() { Close(); }
    PAKENTRY *Find(const char *name);
    int Seek(int offset, int whence);
    CFileIO *GetFile();
    bool Close();
    bool Open(const char *name);

    PAKHEADER *m_header;   // 0x0
    PAKENTRY *m_entry;     // 0x4: the entry CFileIO::Open opened

    static CFileIO s_file;   // 0x4A1630
};

bool CPackage::Open(const char *name)
{
    bool ok = s_file.Open(name, 1);

    if (ok) {
        PAKHEADER h;
        int rest;

        s_file.fread(&h, sizeof(h));
        rest = (h.count - 1) * sizeof(PAKENTRY);
        m_header = (PAKHEADER *)_malloc(rest + sizeof(h), 0, 0);
        memcpy(m_header, &h, sizeof(h));
        s_file.fread((char *)m_header + sizeof(h), rest);
    }
    return ok;
}

bool CPackage::Close()
{
    s_file.Close();
    if (m_header) {
        _free(m_header, 0, 0);
        m_header = 0;
        m_entry = 0;
    }
    return false;
}

CFileIO *CPackage::GetFile()
{
    return &s_file;
}

int CPackage::Seek(int offset, int whence)
{
    return s_file.fseek(offset, whence);
}

PAKENTRY *CPackage::Find(const char *name)
{
    PAKENTRY *e = m_header->entries;
    unsigned int i;

    for (i = 0; i < m_header->count; i++, e++)
        if (!strcasecmp(e->name, name))
            return e;
    return 0;
}
