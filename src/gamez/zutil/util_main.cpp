/* util_main.cpp -- the IOP reboot notifier's registration and the string helper after it (OVL1
 * 0x39C590-0x39CD3F). SOCOM 1's demo holds COutput, CIOPManager::(Un)RegisterRebootCallback and
 * zUtilInit/Uninit as one run between CBufferIO's unit and CFileIO's, ending on the CRegistrant
 * vector's template instances; no assert names the file, so util_main.cpp is ours. theIOPManager
 * itself is zsys_init.cpp's (its static initialiser is that unit's).
 *
 * OPEN, not written yet: COutput (0x39C500/80, with the package unit's file at 0x4A1630),
 * UnRegisterRebootCallback (0x39C590: std::remove_if over class iterators, then the erase at
 * 0x3503F0) and the template tail (remove_if 0x39C710, insert 0x39C7F0, 0x39CCB0, 0x39CD40). */

#include <vector>
#include <string.h>

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
