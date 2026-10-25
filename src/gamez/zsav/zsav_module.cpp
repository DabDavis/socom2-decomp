/* zsav_module.cpp -- CSaveModule, a subsystem's part of a save (OVL1 0x33D230-0x33D36F): built
 * with its name and registered with its manager, its .rdr saved under its key. The unit name is
 * SOCOM 1's (the press beta's asserts name a zsav_module.cpp; SOCOM 1's demo holds these five
 * functions in this order). Defined in reverse of their image order (-inline deferred). */

#include "zsave.h"

char *_strdup(const char *s, const char *file, int line);
void _free(void *p, const char *file, int line);

/* SOCOM 1's */
CSaveModule::CSaveModule(const char *name, CSaveManager *manager)
{
    m_ModuleName = _strdup(name, 0, 0);
    m_Priority = 0;
    m_Flags = 0;
    m_Manager = manager;
    if (m_Manager)
        m_Manager->RegisterModule(this);
}

/* SOCOM 1's */
CSaveModule::~CSaveModule()
{
    if (m_ModuleName) {
        _free(m_ModuleName, 0, 0);
        m_ModuleName = 0;
    }
}

/* SOCOM 1's: through the manager, under the module's key */
bool CSaveModule::SaveRdr(const char *name, CRdrFile *file)
{
    bool ok = false;

    if (m_Manager)
        ok = m_Manager->SaveRdr(m_pKey, name, file);
    return ok;
}

/* SOCOM 1's: nothing to read by default */
void CSaveModule::UnSerialize(const char *name, CRdrFile *file, int flags)
{
}

void CSaveModule::UnSerialize(const char *name, zar::CZAR *zar, int flags)
{
}
