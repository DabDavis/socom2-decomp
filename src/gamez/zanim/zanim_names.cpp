/* zanim_names.cpp -- the ZAnim name tables (OVL1 0x268960-0x269170). Not anim_main.cpp's:
 * CZAnimMain::Unload calls CZAnimNameTable::Unload, which -inline auto would inline from the
 * same unit (0xA0 bytes: it does, and Unload comes out 0x180 long), and matches only with the
 * call. The unit's name is ours. Defined in reverse of their image order. */

#include "zanim.h"

extern "C" void func_00258570(zar::CZAR *zar);
extern "C" const char *func_0026C8E0(CZAnimSet *set, int index);

const char *CZAnimNameIndexTable::GetName(CZAnimSet *set, int index)
{
    if (index >= 0 && index < m_count)
        return func_0026C8E0(set, m_index[index]);
    return 0;
}

void CZAnimNameTable::Unload()
{
    if (m_name) {
        for (int i = 0; i < m_count; i++)
            if (m_name[i])
                func_00180280(m_name[i], 0, 0);
        func_00180280(m_name, 0, 0);
    }
    m_name = 0;
    m_count = 0;
}

int CZAnimNameIndexTable::Load(zar::CZAR &zar)
{
    int count;

    func_00258570(&zar);
    zar.Fetch("Name_Index_Table_Count", &count, 4);
    if (count > 0) {
        Unload();
        m_index = (unsigned short *)_realloc(m_index, count * 2, 0, 0);
        zar.Fetch("Name_Index_Table", m_index, count * 2);
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
        func_00180280(m_index, 0, 0);
    m_index = 0;
    m_count = 0;
}
