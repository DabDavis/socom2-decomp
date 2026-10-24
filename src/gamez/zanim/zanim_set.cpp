/* zanim_set.cpp -- CZAnimSet, a named set of anims, and CZAnimSetSave, the node states a set
 * restores (OVL1 0x26B430-...). The unit name is SOCOM 1's (the press beta's asserts name it for
 * GetAnimIndex). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zanim.h"

int CZAnimSet::GetAnimIndex(CZAnim *anim)
{
    int i = anim - m_anim_list;

    if (i >= m_anim_count)
        i = -1;
    return i;
}

void CZAnimSet::Name(const char *name)
{
    if (m_name)
        func_00180280(m_name, 0, 0);
    m_name = func_001801E0(name, 0, 0);
}

CZAnim *CZAnimSet::Anim(int index)
{
    if (index >= 0 && index < m_anim_count)
        return &m_anim_list[index];
    return 0;
}

CZAnim *CZAnimSet::Anim(const char *name)
{
    if (!name)
        return 0;
    CZAnim *anim = m_anim_list;

    for (int i = 0; i < m_anim_count; i++, anim++)
        if (!strcasecmp(name, anim->GetName()))
            return anim;
    return 0;
}

const char *CZAnimSet::GetName(int index)
{
    return m_name_table.GetName(index);
}

int CZAnimSet::AddName(const char *name)
{
    return m_name_table.AddName(name);
}

void CZAnimSet::AddAnimEx(CZAnimEx *ex)
{
    if (!m_AnimExList)
        m_AnimExList = new CZAnimExVector;
    m_AnimExList->Add(ex);
}

_zanim_si_script *CZAnimSet::SIScript(int index)
{
    if (index >= 0 && index < m_si_script_count)
        return &m_si_script[index];
    return 0;
}

void CZAnimSet::Init()
{
    m_WasStarted = 0;
    m_IsStarted = 0;
    m_IsAvailable = 1;
    m_name = 0;
    m_si_script_count = 0;
    m_si_script = 0;
    m_AnimExList = 0;
    m_anim_count = 0;
    m_anim_max = 0;
    m_anim_list = 0;
    m_node_states = 0;
    m_task_scheduler = 0;
    m_gravity = ZAnim.m_gravity;
}
