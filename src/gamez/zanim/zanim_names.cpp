/* zanim_names.cpp -- the ZAnim name tables (OVL1 0x268960-0x269170). Not anim_main.cpp's:
 * CZAnimMain::Unload calls CZAnimNameTable::Unload, which -inline auto would inline from the
 * same unit (0xA0 bytes: it does, and Unload comes out 0x180 long), and matches only with the
 * call. The unit's name is ours. Defined in reverse of their image order. */

#include <string.h>

#include "zanim.h"


inline void CZAnimNameTable::Append(const char *name)
{
    m_name = (char **)_realloc(m_name, (m_count + 1) * 4, 0, 0);
    m_name[m_count] = (char *)_malloc(strlen(name) + 1, 0, 0);
    strcpy(m_name[m_count], name);
    m_count++;
}

/* entry 0 of every table: a name that is no name */
int CZAnimNameTable::AddName(const char *name)
{
    if (!name)
        return 0;
    if (m_count == 0)
        Append("NA");
    for (int i = 0; i < m_count; i++)
        if (!strcmp(m_name[i], name))
            return i;
    Append(name);
    return m_count - 1;
}

int CZAnimNameIndexTable::AddName(CZAnimSet *set, const char *name)
{
    if (!name)
        return 0;
    int index = set->AddName(name);

    if (m_count == 0) {
        int none = set->AddName("NA");

        m_index = (unsigned short *)_realloc(m_index, (m_count + 1) * 2, 0, 0);
        m_index[m_count] = none;
        m_count++;
    }
    for (int i = 0; i < m_count; i++)
        if (index == m_index[i])
            return i;
    m_index = (unsigned short *)_realloc(m_index, (m_count + 1) * 2, 0, 0);
    m_index[m_count] = index;
    m_count++;
    return m_count - 1;
}

const char *CZAnimNameIndexTable::GetName(CZAnimSet *set, int index)
{
    if (index >= 0 && index < m_count)
        return set->GetName(m_index[index]);
    return 0;
}

void CZAnimNameTable::Unload()
{
    if (m_name) {
        for (int i = 0; i < m_count; i++)
            if (m_name[i])
                _free(m_name[i], 0, 0);
        _free(m_name, 0, 0);
    }
    m_name = 0;
    m_count = 0;
}

int CZAnimNameIndexTable::Load(zar::CZAR &archive)
{
    int count;

    archive.GetOpenKey();
    archive.Fetch("Name_Index_Table_Count", &count, 4);
    if (count > 0) {
        Unload();
        m_index = (unsigned short *)_realloc(m_index, count * 2, 0, 0);
        archive.Fetch("Name_Index_Table", m_index, count * 2);
        m_count = count;
    }
    return 1;
}

const char *CZAnimNameTable::GetName(int index)
{
    if (index >= 0 && index < m_count)
        return m_name[index];
    return 0;
}

void CZAnimNameIndexTable::Unload()
{
    if (m_index)
        _free(m_index, 0, 0);
    m_index = 0;
    m_count = 0;
}

/* AddName's own null test is the only one */
int CZAnimNameTable::Load(zar::CZAR &archive)
{
    int count;
    zar::CKey *root = archive.GetOpenKey();

    archive.Fetch("Name_Table_Count", &count, 4);
    if (count > 0) {
        Unload();
        zar::CKey *table = root->FindKey("Name_Table");

        if (table) {
            for (zar::CKeyRing::iterator it = table->begin(); it != table->end(); ++it) {
                if (archive.OpenKey(*it)) {
                    AddName((*it)->m_name);
                    archive.CloseKey(*it);
                }
            }
        }
    }
    return 1;
}
