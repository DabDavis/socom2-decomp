/* zsav_manager.cpp -- CSaveManager, the save file's driver (OVL1 0x33D370-0x33E7EF): its modules
 * registered and found by name, an .rdr saved under a key, the destructor and the constructor
 * (0x33E4D0, byte-identical to SOCOM 1's, written first over the same layout). The unit name is
 * SOCOM 1's (the press beta's asserts name a zsav_manager.cpp). Not written yet: GetHeader (0x33D4A0), Load (0x33D6D0) and Save
 * (0x33DFF0, five arguments where SOCOM 1's took four), their helpers (0x33DBA0, 0x33DC30,
 * 0x33DE00) and the list's template pieces after the constructor (0x33E510-0x33E7EF). Defined in
 * reverse of their image order (-inline deferred). */

#include "zsave.h"
#include <string.h>
#include <stdio.h>

class CRdrFile {
public:
    zar::CKey *Insert(zar::CZAR *zar, zar::CKey *key);
};

/* zar::CZAR's key creation, not named yet: the key of that name made under the open key */
extern "C" zar::CKey *func_00258740(zar::CZAR *zar, const char *name);

/* SOCOM 1's, byte-identical (0x33E4D0, after the destructor in the image) */
CSaveManager::CSaveManager(CIO *file)
    : m_pFile(file), m_TempBufferSize(0), m_TempBuffer(0)
{
}

/* SOCOM 1's: the buffer freed, the module list emptied */
CSaveManager::~CSaveManager()
{
    if (m_TempBuffer) {
        delete m_TempBuffer;
        m_TempBuffer = 0;
    }
}

/* SOCOM 1's: a module joins unless one of its name is there */
void CSaveManager::RegisterModule(CSaveModule *module)
{
    if (!FindModule(module->m_ModuleName)) {
        push_back(module);
        module->m_Manager = this;
    }
}

/* SOCOM 1's: `name`.rdr written under the key; SOCOM II's always answers false */
bool CSaveManager::SaveRdr(zar::CKey *key, const char *name, CRdrFile *file)
{
    char buf[80];

    sprintf(buf, "%s.rdr", name);
    if (key) {
        zar::CKey *k = func_00258740(m_pZar, buf);
        if (k) {
            file->Insert(m_pZar, k);
            m_pZar->CloseKey(k);
        }
    }
    return false;
}

/* the registered module of that name (SOCOM II's; the name ours). RegisterModule calls it out
 * of line: pinned */
#pragma auto_inline off
CSaveModule *CSaveManager::FindModule(const char *name)
{
    CSaveModule *found = 0;
    iterator i;

    i = begin();
    iterator e = end();

    /* the element read through the base iterator, cast out by value: the wrapper's reference
     * keeps the element's address in a saved register across strcmp, where the target reloads */
    for (; i != e; ++i) {
        if (!strcmp(((CSaveModule *)*i.i_)->m_ModuleName, name)) {
            found = (CSaveModule *)*i.i_;
            break;
        }
    }
    return found;
}
#pragma auto_inline reset
