/* zanim_set.cpp -- CZAnimSet, a named set of anims, and CZAnimSetSave, the node states a set
 * restores (OVL1 0x26B430-...). The unit name is SOCOM 1's (the press beta's asserts name it for
 * GetAnimIndex). Defined in reverse of their image order (-inline deferred). */

#include <string.h>

#include "zanim.h"

int CZAnimSet::GarbageCollect()
{
    func_002684D0();
    if (m_anim_list)
        for (int i = 0; i < m_anim_count; i++)
            m_anim_list[i].GarbageCollect();
    return 1;
}

/* every anim stopped, and every anim it chains to */
int CZAnimSet::Stop()
{
    for (int i = 0; i < m_anim_count; i++) {
        CZAnim *anim = Anim(i);

        func_002707C0(anim, false);
        anim->StopLights(true);
        anim->DeleteParticleSources();
        while (anim->m_sibling_anim) {
            anim = anim->m_sibling_anim;
            func_002707C0(anim, false);
            anim->StopLights(true);
            anim->DeleteParticleSources();
        }
    }
    m_IsStarted = 0;
    return 1;
}

/* the saved node states put back, the CZAnimEx list and every anim reset */
void CZAnimSet::Reset()
{
    CZAnimSetSave *save = m_node_states;

    if (save)
        for (CZAnimSetSave::const_iterator it = save->begin(); it != save->end(); ++it)
            save->Reset(*it);
    if (m_AnimExList)
        m_AnimExList->Reset();
    for (int i = 0; i < m_anim_count; i++)
        Anim(i)->Reset();
}

extern "C" int func_0026C240(CZAnimSet *set, int x)
{
    for (int i = 0; i < set->m_anim_count; i++)
        func_00270570(set->Anim(i), x);
    return 1;
}

/* SOCOM II's: SOCOM 1's NewAnim without the slot's allocation -- an anim given its slot,
 * cleared, named and set going; the set itself is not read */
extern "C" CZAnim *func_0026CAA0(CZAnimSet *set, CZAnim *anim, const char *name)
{
    if (!anim)
        return 0;
    ZAnim.m_CurAnim = anim;
    ZAnim.m_CurSeq = 0;
    memset(anim, 0, sizeof(CZAnim));
    anim->m_root_node_index = 0;
    if (!ZAnim.m_search_external_nodes)
        anim->node_search_scope = 1;
    else
        anim->node_search_scope = 0;
    anim->m_animset_index = ZAnim.m_CurAnimSet ? ZAnim.m_CurAnimSet - ZAnim.m_animset_list : 0;
    anim->Name(name);
    anim->State(1);
    anim->Priority(3.0f);
    anim->m_damage_seq_offset = 0;
    anim->m_activation_seq_offset = 0;
    anim->m_execution_seq_offset = 0;
    anim->m_cleanup_seq_offset = 0;
    return anim;
}

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
